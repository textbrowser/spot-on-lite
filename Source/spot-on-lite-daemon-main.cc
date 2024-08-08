/*
** Copyright (c) 2011 - 10^10^10, Alexis Megas.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from Spot-On-Lite without specific prior written permission.
**
** SPOT-ON-LITE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** SPOT-ON-LITE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

extern "C"
{
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
}

#include <QCoreApplication>
#include <QFileInfo>
#include <QtDebug>

#include <iostream>

#include "spot-on-lite-daemon.h"
#ifdef SPOTON_LITE_DAEMON_SHA_TEST
#include "spot-on-lite-daemon-sha.h"
#endif

char *spot_on_lite_daemon::s_congestion_control_file_name = nullptr;
char *spot_on_lite_daemon::s_local_socket_server_name = nullptr;
char *spot_on_lite_daemon::s_log_file_name = nullptr;
char *spot_on_lite_daemon::s_remote_identities_file_name = nullptr;
char *spot_on_lite_daemon::s_statistics_file_name = nullptr;

void spot_on_lite_daemon::handler_signal(int signal_number)
{
  if(signal_number == SIGINT || signal_number == SIGTERM)
    {
      kill(0, SIGTERM);

      if(spot_on_lite_daemon::s_remove_temporary_files)
	{
	  if(spot_on_lite_daemon::s_congestion_control_file_name)
	    unlink(spot_on_lite_daemon::s_congestion_control_file_name);

	  if(spot_on_lite_daemon::s_local_socket_server_name)
	    unlink(spot_on_lite_daemon::s_local_socket_server_name);

	  if(spot_on_lite_daemon::s_log_file_name)
	    unlink(spot_on_lite_daemon::s_log_file_name);

	  if(spot_on_lite_daemon::s_remote_identities_file_name)
	    unlink(spot_on_lite_daemon::s_remote_identities_file_name);

	  if(spot_on_lite_daemon::s_statistics_file_name)
	    unlink(spot_on_lite_daemon::s_statistics_file_name);
	}

      _exit(EXIT_SUCCESS);
    }

  auto pid = waitpid(-1, nullptr, WNOHANG);
  char a[32];

  for(size_t i = 0; i < sizeof(a); i++) // memset()
    a[i] = 0;

  if(pid > -1)
    for(size_t i = 1; i < sizeof(a); i++)
      {
	a[i] = static_cast<char> (pid % 10 + 48);
	pid /= 10;

	if(pid == 0)
	  break;
      }

  switch(signal_number)
    {
    case SIGCHLD:
      {
	a[0] = 'c';
	break;
      }
    case SIGUSR1:
      {
	a[0] = 'u';
	break;
      }
    case SIGUSR2:
      {
	return;
      }
    default:
      {
	a[0] = 'z'; // Error.
	break;
      }
    }

  auto rc = ::write(s_signal_fd[0], a, strlen(a));

  Q_UNUSED(rc);
}

#ifndef Q_OS_MACOS
static int make_daemon(void)
{
  struct rlimit rl = {};

  /*
  ** Turn into a daemon.
  */

  if(getrlimit(RLIMIT_NOFILE, &rl) != 0)
    {
      fprintf(stderr, "getrlimit() failed, %s, exiting.\n", strerror(errno));
      return 1;
    }

  umask(0);

  pid_t pid = 0;

  if((pid = fork()) < 0)
    {
      fprintf(stderr, "%s", "fork() failed, exiting.\n");
      return 1;
    }
  else if(pid != 0)
    exit(EXIT_SUCCESS);

  setsid();

  if(chdir("/") != 0)
    {
      fprintf(stderr, "chdir() failed, %s, exiting.\n", strerror(errno));
      return 1;
    }

  if(rl.rlim_max == RLIM_INFINITY)
    rl.rlim_max = 2048;

  for(rlim_t i = 0; i < rl.rlim_max; i++)
    close((int) i);

  auto fd0 = open("/dev/null", O_RDWR);
  auto fd1 = dup(0);
  auto fd2 = dup(1);

  if(fd0 != 0 || fd1 != 1 || fd2 != 2)
    {
      fprintf(stderr, "incorrect file descriptors: %d, %d, %d, exiting.\n",
	      fd0, fd1, fd2);
      return 1;
    }

  return 0;
}
#endif

static int prepare_signal_handlers(void)
{
  struct sigaction act = {};

  /*
  ** Ignore SIGHUP.
  */

  memset(&act, 0, sizeof(struct sigaction));
  act.sa_flags = 0;
  act.sa_handler = SIG_IGN;
  sigemptyset(&act.sa_mask);

  if(sigaction(SIGHUP, &act, nullptr))
    std::cerr << "sigaction() failure for SIGHUP. Ignoring." << std::endl;

  /*
  ** Ignore SIGPIPE.
  */

  memset(&act, 0, sizeof(struct sigaction));
  act.sa_flags = 0;
  act.sa_handler = SIG_IGN;
  sigemptyset(&act.sa_mask);

  if(sigaction(SIGPIPE, &act, nullptr))
    std::cerr << "sigaction() failure for SIGPIPE. Ignoring." << std::endl;

  /*
  ** Monitor some signals.
  */

  QList<int> list;

  list << SIGCHLD << SIGINT << SIGTERM << SIGUSR1 << SIGUSR2;

  for(int i = 0; i < list.size(); i++)
    {
      memset(&act, 0, sizeof(struct sigaction));
      act.sa_flags = 0;
      act.sa_handler = spot_on_lite_daemon::handler_signal;
      sigemptyset(&act.sa_mask);

      if(sigaction(list.at(i), &act, nullptr))
	{
	  std::cerr << "sigaction() failure for "
		    << list.at(i)
		    << ". Terminating."
		    << std::endl;
	  return 1;
	}
    }

  return 0;
}

static std::string s_version = "2023.06.07";

int main(int argc, char *argv[])
{
  for(int i = 0; i < argc; i++)
    if(argv && argv[i])
      {
	if(strcmp(argv[i], "--help") == 0)
	  {
	    std::string string;

	    string += "Usage: Spot-On-Lite-Daemon [OPTION]...\n";
	    string += "Spot-On-Lite-Daemon\n";
	    string += "   --configuration-file          file\n";
	    string += "   --help                        display helpful text\n";
	    string += "   --keep-terminal               ";
	    string += "do not become a daemon\n";
	    string += "   --statistics                  display vitals\n";
	    string += "   --validate-configuration-file file";
	    std::cout << string << std::endl;
	    return EXIT_SUCCESS;
	  }
	else if(strcmp(argv[i], "--statistics") == 0)
	  {
	    spot_on_lite_daemon::vitals();
	    return EXIT_SUCCESS;
	  }
	else if(strcmp(argv[i], "--version") == 0)
	  {
	    std::string string;

	    string += "Spot-On-Lite\n   Version " + s_version;
	    std::cout << string << std::endl;
	    return EXIT_SUCCESS;
	  }
      }

#ifdef SPOTON_LITE_DAEMON_SHA_TEST
  spot_on_lite_daemon_sha s;

  qDebug() << "SHA-512 test 1: "
	   << (s.sha_512("abc").toHex() ==
	       "ddaf35a193617abacc417349ae204131"
	       "12e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a8"
	       "36ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f")
	   << ".";
  qDebug() << "SHA-512 test 2: "
	   << (s.sha_512("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmg"
			 "hijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmn"
			 "opqrstnopqrstu").toHex() ==
	       "8e959b75dae313da8cf4f72814fc143f"
	       "8f7779c6eb9f7fa17299aeadb6889018501d289e4900f7e4"
	       "331b99dec4b5433ac7d329eeb6dd26545e96e55b874be909")
	   << ".";
#endif

  auto keep_terminal = false;

  for(int i = 0; i < argc; i++)
    if(argv && argv[i] && strcmp(argv[i], "--keep-terminal") == 0)
      keep_terminal = true;
    else if(argv && argv[i] &&
	    strcmp(argv[i], "--validate-configuration-file") == 0)
      {
	i += 1;

	if(argc > i && argv[i])
	  {
	    auto ok = true;
	    spot_on_lite_daemon daemon;

	    daemon.validate_configuration_file(argv[i], &ok);

	    if(ok)
	      return EXIT_SUCCESS;
	    else
	      return EXIT_FAILURE;
	  }
	else
	  {
	    std::cerr << "Invalid validate-configuration-file usage. Exiting."
		      << std::endl;
	    return EXIT_FAILURE;
	  }
      }

  QString configuration_file_name("");

  for(int i = 0; i < argc; i++)
    if(argv && argv[i] && strcmp(argv[i], "--configuration-file") == 0)
      {
	if(configuration_file_name.isEmpty())
	  {
	    i += 1;

	    if(argc > i && argv[i])
	      configuration_file_name = argv[i];
	    else
	      {
		std::cerr << "Invalid configuration-file usage. Exiting."
			  << std::endl;
		return EXIT_FAILURE;
	      }
	  }
      }

  QFileInfo const file_info(configuration_file_name);

  if(!file_info.isReadable())
    {
      qDebug() << "The configuration file \""
	       << configuration_file_name
	       << "\" is not readable. Exiting";
      return EXIT_FAILURE;
    }

#ifndef Q_OS_MACOS
  if(!keep_terminal)
    if(make_daemon())
      return EXIT_FAILURE;
#else
  Q_UNUSED(keep_terminal);
#endif

  if(prepare_signal_handlers())
    return EXIT_FAILURE;

  QCoreApplication qapplication(argc, argv);
  auto rc = EXIT_SUCCESS;

  try
    {
      spot_on_lite_daemon daemon(configuration_file_name);

      daemon.start();
      rc = qapplication.exec();
      kill(0, SIGTERM);
    }
  catch(const std::bad_alloc &exception)
    {
      std::cerr << "Spot-On-Lite-Daemon memory failure! "
		<< "Aborting!" << std::endl;
      rc = EXIT_FAILURE;
    }
  catch(...)
    {
      std::cerr << "Spot-On-Lite-Daemon exception. Aborting!" << std::endl;
      rc = EXIT_FAILURE;
    }

  return rc;
}

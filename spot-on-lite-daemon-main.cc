/*
** Alexis Megas.
*/

extern "C"
{
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
}

#include <QCoreApplication>
#include <QFileInfo>
#include <QtDebug>

#include <iostream>

#include "spot-on-lite-daemon.h"

void spot_on_lite_daemon::handler_signal(int signal_number)
{
  switch(signal_number)
    {
    case SIGUSR1:
      {
	break;
      }
    default:
      {
	kill(0, signal_number);
	exit(0);
	return;
      }
    }

  char a = 1;
  ssize_t rc = ::write(s_signal_usr1_fd[0], &a, sizeof(a));

  (void) rc;
}

static int make_daemon(void)
{
  struct rlimit rl;

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

  int fd0 = open("/dev/null", O_RDWR);
  int fd1 = dup(0);
  int fd2 = dup(1);

  if(fd0 != 0 || fd1 != 1 || fd2 != 2)
    {
      fprintf(stderr, "incorrect file descriptors: %d, %d, %d, exiting.\n",
	      fd0, fd1, fd2);
      return 1;
    }

  return 0;
}

static int prepare_signal_handlers(void)
{
  struct sigaction act;

  /*
  ** Ignore SIGCHLD.
  */

  act.sa_handler = SIG_IGN;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  if(sigaction(SIGCHLD, &act, 0))
    std::cerr << "sigaction() failure for SIGCHLD. Ignoring." << std::endl;

  /*
  ** Ignore SIGHUP.
  */

  act.sa_handler = SIG_IGN;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  if(sigaction(SIGHUP, &act, 0))
    std::cerr << "sigaction() failure for SIGHUP. Ignoring." << std::endl;

  /*
  ** Ignore SIGPIPE.
  */

  act.sa_handler = SIG_IGN;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  if(sigaction(SIGPIPE, &act, 0))
    std::cerr << "sigaction() failure for SIGPIPE. Ignoring." << std::endl;

  /*
  ** Monitor SIGTERM, SIGUSR1.
  */

  QList<int> list;

  list << SIGTERM << SIGUSR1;

  while(!list.isEmpty())
    {
      struct sigaction act;

      act.sa_handler = spot_on_lite_daemon::handler_signal;
      sigemptyset(&act.sa_mask);
      act.sa_flags = 0;

      if(sigaction(list.first(), &act, 0))
	{
	  std::cerr << "sigaction() failure for "
		    << list.first()
		    << ". Terminating." << std::endl;
	  return 1;
	}

      list.removeFirst();
    }

  return 0;
}

int main(int argc, char *argv[])
{
  for(int i = 0; i < argc; i++)
    if(argv && argv[i] && strcmp(argv[i], "--help") == 0)
      {
        std::string string;

        string += "Usage: Spot-On-Lite-Daemon [OPTION]...\n";
        string += "Spot-On-Lite-Daemon\n\n";
	string += "  --configuration-file          file\n";
        string += "  --help                        display helpful text\n";
	string += "  --validate-configuration-file file\n";
        std::cout << string << std::endl;
        return EXIT_SUCCESS;
      }

  for(int i = 0; i < argc; i++)
    if(argv && argv[i] && strcmp(argv[i], "--validate-configuration-file") == 0)
      {
	i += 1;

	if(argc > i && argv[i])
	  {
	    bool ok = true;
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

  QFileInfo fileInfo(configuration_file_name);

  if(!fileInfo.isReadable())
    {
      qDebug() << "The configuration file \""
	       << configuration_file_name
	       << "\" is not readable. Exiting";
      return EXIT_FAILURE;
    }

  if(make_daemon())
    return EXIT_FAILURE;

  if(prepare_signal_handlers())
    return EXIT_FAILURE;

  QCoreApplication qapplication(argc, argv);
  int rc = EXIT_SUCCESS;

  try
    {
      spot_on_lite_daemon daemon(configuration_file_name);

      daemon.start();
      rc = qapplication.exec();
    }
  catch(const std::bad_alloc &)
    {
      std::cerr << "Spot-On-Lite-Daemon memory failure! "
		<< "Aborting!" << std::endl;
      rc = EXIT_FAILURE;
    }

  return rc;
}

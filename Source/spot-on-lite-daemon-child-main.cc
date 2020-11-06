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
#include <signal.h>
#include <unistd.h>
}

#include <QCoreApplication>
#include <QtDebug>

#include <iostream>

#include "spot-on-lite-daemon-child-client.h"

#ifdef SPOTON_LITE_DAEMON_CHILD_ECL_SUPPORTED
#ifdef FALSE
#undef FALSE
#endif
#ifdef SLOT
#undef SLOT
#endif
#ifdef TRUE
#undef TRUE
#endif
#ifdef slots
#undef slots
#endif
#include <ecl/ecl.h>

extern "C"
{
  extern void init_lib_SPOT_ON_LITE_DAEMON_SHA(cl_object);
}
#endif

static void handler_signal(int signal_number)
{
  Q_UNUSED(signal_number);
  exit(0);
}

static int prepare_signal_handlers(void)
{
  struct sigaction act;

  /*
  ** Ignore SIGHUP.
  */

  memset(&act, 0, sizeof(struct sigaction));
  act.sa_handler = SIG_IGN;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  if(sigaction(SIGHUP, &act, nullptr))
    std::cerr << "sigaction() failure for SIGHUP. Ignoring." << std::endl;

  /*
  ** Ignore SIGPIPE.
  */

  memset(&act, 0, sizeof(struct sigaction));
  act.sa_handler = SIG_IGN;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  if(sigaction(SIGPIPE, &act, nullptr))
    std::cerr << "sigaction() failure for SIGPIPE. Ignoring." << std::endl;

  /*
  ** Monitor SIGUSR2.
  */

  memset(&act, 0, sizeof(struct sigaction));
  act.sa_handler = handler_signal;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  if(sigaction(SIGUSR2, &act, nullptr))
    std::cerr << "sigaction() failure for SIGUSR2. Ignoring." << std::endl;

  return 0;
}

int main(int argc, char *argv[])
{
  if(prepare_signal_handlers())
    return EXIT_FAILURE;

#ifdef SPOTON_LITE_DAEMON_CHILD_ECL_SUPPORTED
  cl_boot(argc, argv);
  ecl_init_module(NULL, init_lib_SPOT_ON_LITE_DAEMON_SHA);
  atexit(cl_shutdown);
#endif

  QString certificates_file_name("");
  QString configuration_file_name("");
  QString congestion_control_file_name("");
  QString end_of_message_marker("");
  QString local_server_file_name("");
  QString log_file_name("");
  QString peer_address("");
  QString peer_scope_identity("");
  QString protocol("");
  QString remote_identities_file_name("");
  QString server_identity("");
  QString ssl_control_string("");
  int identities_lifetime = -1;
  int local_so_rcvbuf_so_sndbuf = -1;
  int maximum_accumulated_bytes = -1;
  int rc = EXIT_SUCCESS;
  int sd = -1;
  int silence = -1;
  int ssl_key_size = -1;
  quint16 peer_port = 0;

  for(int i = 0; i < argc; i++)
    if(argv && argv[i] && strcmp(argv[i], "--certificates-file") == 0)
      {
	if(certificates_file_name.isEmpty())
	  {
	    i += 1;

	    if(argc > i && argv[i])
	      certificates_file_name = argv[i];
	    else
	      {
		std::cerr << "Invalid certificates-file usage. Exiting."
			  << std::endl;
		return EXIT_FAILURE;
	      }
	  }
      }
    else if(argv && argv[i] && strcmp(argv[i], "--configuration-file") == 0)
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
    else if(argv && argv[i] &&
	    strcmp(argv[i], "--congestion-control-file") == 0)
      {
	if(congestion_control_file_name.isEmpty())
	  {
	    i += 1;

	    if(argc > i && argv[i])
	      congestion_control_file_name = argv[i];
	    else
	      {
		std::cerr << "Invalid congestion-control-file usage. Exiting."
			  << std::endl;
		return EXIT_FAILURE;
	      }
	  }
      }
    else if(argv && argv[i] && strcmp(argv[i], "--end-of-message-marker") == 0)
      {
	if(end_of_message_marker.isEmpty())
	  {
	    i += 1;

	    if(argc > i && argv[i])
	      end_of_message_marker = QString::fromUtf8
		(QByteArray::fromBase64(argv[i]));
	    else
	      {
		std::cerr << "Invalid end-of-message-marker usage. Exiting."
			  << std::endl;
		return EXIT_FAILURE;
	      }
	  }
      }
    else if(argv && argv[i] &&
	    strcmp(argv[i], "--identities-lifetime") == 0)
      {
	if(identities_lifetime == -1)
	  {
	    i += 1;

	    if(argc > i && argv[i])
	      identities_lifetime = std::atoi(argv[i]);
	    else
	      {
		std::cerr << "Invalid identities-lifetime usage. Exiting."
			  << std::endl;
		return EXIT_FAILURE;
	      }
	  }
      }
    else if(argv && argv[i] && strcmp(argv[i], "--local-server-file") == 0)
      {
	if(local_server_file_name.isEmpty())
	  {
	    i += 1;

	    if(argc > i && argv[i])
	      local_server_file_name = argv[i];
	    else
	      {
		std::cerr << "Invalid local-server-file usage. Exiting."
			  << std::endl;
		return EXIT_FAILURE;
	      }
	  }
      }
    else if(argv && argv[i] &&
	    strcmp(argv[i], "--local-so-rcvbuf-so-sndbuf") == 0)
      {
	if(local_so_rcvbuf_so_sndbuf == -1)
	  {
	    i += 1;

	    if(argc > i && argv[i])
	      local_so_rcvbuf_so_sndbuf = std::atoi(argv[i]);
	    else
	      {
		std::cerr << "Invalid local-so-rcvbuf-so-sndbuf usage. "
			  << "Exiting."
			  << std::endl;
		return EXIT_FAILURE;
	      }
	  }
      }
    else if(argv && argv[i] && strcmp(argv[i], "--log-file") == 0)
      {
	if(log_file_name.isEmpty())
	  {
	    i += 1;

	    if(argc > i && argv[i])
	      log_file_name = argv[i];
	    else
	      {
		std::cerr << "Invalid log-file usage. Exiting."
			  << std::endl;
		return EXIT_FAILURE;
	      }
	  }
      }
    else if(argv && argv[i] &&
	    strcmp(argv[i], "--maximum-accumulated-bytes") == 0)
      {
	if(maximum_accumulated_bytes == -1)
	  {
	    i += 1;

	    if(argc > i && argv[i])
	      maximum_accumulated_bytes = std::atoi(argv[i]);
	    else
	      {
		std::cerr << "Invalid maximum-accumulated-bytes usage. Exiting."
			  << std::endl;
		return EXIT_FAILURE;
	      }
	  }
      }
    else if(argv && argv[i] && strcmp(argv[i], "--peer-address") == 0)
      {
	if(peer_address.isEmpty())
	  {
	    i += 1;

	    if(argc > i && argv[i])
	      peer_address = QByteArray::fromBase64(argv[i]);
	    else
	      {
		std::cerr << "Invalid peer-address usage. Exiting."
			  << std::endl;
		return EXIT_FAILURE;
	      }
	  }
      }
    else if(argv && argv[i] && strcmp(argv[i], "--peer-port") == 0)
      {
	if(peer_port == 0)
	  {
	    i += 1;

	    if(argc > i && argv[i])
	      peer_port = static_cast<quint16> (std::atoi(argv[i]));
	    else
	      {
		std::cerr << "Invalid peer-port usage. Exiting."
			  << std::endl;
		return EXIT_FAILURE;
	      }
	  }
      }
    else if(argv && argv[i] && strcmp(argv[i], "--peer-scope-identity") == 0)
      {
	if(peer_scope_identity.isEmpty())
	  {
	    i += 1;

	    if(argc > i && argv[i])
	      peer_scope_identity = QString::fromUtf8
		(QByteArray::fromBase64(argv[i]));
	    else
	      {
		std::cerr << "Invalid peer-scope-identity usage. Exiting."
			  << std::endl;
		return EXIT_FAILURE;
	      }
	  }
      }
    else if(argv && argv[i] &&
	    strcmp(argv[i], "--remote-identities-file") == 0)
      {
	if(remote_identities_file_name.isEmpty())
	  {
	    i += 1;

	    if(argc > i && argv[i])
	      remote_identities_file_name = argv[i];
	    else
	      {
		std::cerr << "Invalid remote-identities-file usage. Exiting."
			  << std::endl;
		return EXIT_FAILURE;
	      }
	  }
      }
    else if(argv && argv[i] && strcmp(argv[i], "--server-identity") == 0)
      {
	if(server_identity.isEmpty())
	  {
	    i += 1;

	    if(argc > i && argv[i])
	      server_identity = argv[i];
	    else
	      {
		std::cerr << "Invalid server_identity usage. Exiting."
			  << std::endl;
		return EXIT_FAILURE;
	      }
	  }
      }
    else if(argv && argv[i] && strcmp(argv[i], "--silence-timeout") == 0)
      {
	if(silence == -1)
	  {
	    i += 1;

	    if(argc > i && argv[i])
	      silence = std::atoi(argv[i]);
	    else
	      {
		std::cerr << "Invalid silence-timeout usage. Exiting."
			  << std::endl;
		return EXIT_FAILURE;
	      }
	  }
      }
    else if(argv && argv[i] && strcmp(argv[i], "--socket-descriptor") == 0)
      {
	if(sd == -1)
	  {
	    i += 1;

	    if(argc > i && argv[i])
	      sd = std::atoi(argv[i]);
	    else
	      {
		std::cerr << "Invalid socket-descriptor usage. Exiting."
			  << std::endl;
		return EXIT_FAILURE;
	      }
	  }
      }
    else if(argv && argv[i] && strcmp(argv[i], "--ssl-tls-control-string") == 0)
      {
	if(ssl_control_string.isEmpty())
	  {
	    i += 1;

	    if(argc > i && argv[i])
	      ssl_control_string = argv[i];
	    else
	      {
		/*
		** Not an error.
		*/
	      }
	  }
      }
    else if(argv && argv[i] && strcmp(argv[i], "--ssl-tls-key-size") == 0)
      {
	if(ssl_key_size == -1)
	  {
	    i += 1;

	    if(argc > i && argv[i])
	      ssl_key_size = atoi(argv[i]);
	    else
	      {
		/*
		** Not an error.
		*/
	      }
	  }
      }
    else if(argv && argv[i] && strcmp(argv[i], "--tcp") == 0)
      {
	if(protocol.isEmpty())
	  protocol = "tcp";
      }
    else if(argv && argv[i] && strcmp(argv[i], "--udp") == 0)
      {
	if(protocol.isEmpty())
	  protocol = "udp";
      }

  try
    {
      if(protocol == "tcp" || protocol == "udp")
	{
	  QCoreApplication qapplication(argc, argv);
	  spot_on_lite_daemon_child_client client
	    (QByteArray(),
	     certificates_file_name,
	     configuration_file_name,
	     congestion_control_file_name,
	     end_of_message_marker,
	     local_server_file_name,
	     log_file_name,
	     peer_address,
	     peer_scope_identity,
	     protocol,
	     remote_identities_file_name,
	     server_identity,
	     ssl_control_string,
	     identities_lifetime,
	     local_so_rcvbuf_so_sndbuf,
	     maximum_accumulated_bytes,
	     silence,
	     sd,
	     ssl_key_size,
	     peer_port);

	  rc = qapplication.exec();
	}
    }
  catch(const std::bad_alloc &)
    {
      std::cerr << "Spot-On-Lite-Daemon-Child memory failure! "
		<< "Aborting!" << std::endl;
      rc = EXIT_FAILURE;
    }
  catch(...)
    {
      std::cerr << "Spot-On-Lite-Daemon-Child exception. Aborting!"
		<< std::endl;
      rc = EXIT_FAILURE;
    }

  return rc;
}

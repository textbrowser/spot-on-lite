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
**    derived from Spot-On without specific prior written permission.
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

#include "spot-on-lite-daemon-child-tcp-client.h"

static int prepare_signal_handlers(void)
{
  struct sigaction act;

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

  return 0;
}

int main(int argc, char *argv[])
{
  if(prepare_signal_handlers())
    return EXIT_FAILURE;

  QString congestion_control_file_name("");
  QString local_server_file_name("");
  QString log_file_name("");
  QString protocol("tcp");
  QString ssl_control_string("");
  int maximum_accumulated_bytes = -1;
  int rc = EXIT_SUCCESS;
  int sd = -1;
  int silence = -1;
  int ssl_key_size = -1;

  for(int i = 0; i < argc; i++)
    if(argv && argv[i] && strcmp(argv[i], "--congestion-control-file") == 0)
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

  QCoreApplication qapplication(argc, argv);

  try
    {
      if(protocol == "tcp")
	{
	  spot_on_lite_daemon_child_tcp_client client
	    (congestion_control_file_name,
	     local_server_file_name,
	     log_file_name,
	     ssl_control_string,
	     maximum_accumulated_bytes,
	     silence,
	     sd,
	     ssl_key_size);

	  rc = qapplication.exec();
	}
    }
  catch(const std::bad_alloc &)
    {
      std::cerr << "Spot-On-Lite-Daemon-Child memory failure! "
		<< "Aborting!" << std::endl;
      rc = EXIT_FAILURE;
    }

  return rc;
}

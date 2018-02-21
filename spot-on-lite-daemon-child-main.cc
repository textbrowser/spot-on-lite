/*
** Alexis Megas.
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

  QString log_file_name("");
  QString ssl_control_string("");
  QString protocol("tcp");
  int maximum_accumulated_bytes = -1;
  int rc = EXIT_SUCCESS;
  int sd = -1;
  int silence = -1;
  int sslKeySize = -1;

  for(int i = 0; i < argc; i++)
    if(argv && argv[i] && strcmp(argv[i], "--log-file") == 0)
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
	if(sslKeySize == -1)
	  {
	    i += 1;

	    if(argc > i && argv[i])
	      sslKeySize = atoi(argv[i]);
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
	    (log_file_name,
	     ssl_control_string,
	     maximum_accumulated_bytes,
	     silence,
	     sd,
	     sslKeySize);

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

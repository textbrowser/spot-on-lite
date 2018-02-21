/*
** Alexis Megas.
*/

extern "C"
{
#include <sys/wait.h>
#include <unistd.h>
}

#include <QStringList>

#include "spot-on-lite-daemon.h"
#include "spot-on-lite-daemon-tcp-listener.h"

spot_on_lite_daemon_tcp_listener::spot_on_lite_daemon_tcp_listener
(const QString &configuration, QObject *parent):QTcpServer(parent)
{
  /*
  ** The configuration is assumed to be correct.
  */

  connect(&m_start_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_start_timeout(void)));
  m_configuration = configuration;
  m_start_timer.start(5000);
}

spot_on_lite_daemon_tcp_listener::
~spot_on_lite_daemon_tcp_listener()
{
}

#if QT_VERSION >= 0x050000
void spot_on_lite_daemon_tcp_listener::incoming_connection
(qintptr socketDescriptor)
#else
void spot_on_lite_daemon_tcp_listener::incoming_connection
(int socketDescriptor)
#endif
{
  if(!spot_on_lite_daemon::instance())
    {
      ::close(static_cast<int> (socketDescriptor));
      return;
    }

  QStringList list(m_configuration.split(",", QString::KeepEmptyParts));
  int maximum_accumulated_bytes = spot_on_lite_daemon::instance()->
    maximum_accumulated_bytes();
  pid_t pid = 0;
  std::string command
    (spot_on_lite_daemon::instance()->child_process_file_name().toStdString());
  std::string ldLibraryPath
    (spot_on_lite_daemon::instance()->child_process_ld_library_path().
     toStdString());
  std::string log_file_name
    (spot_on_lite_daemon::instance()->log_file_name().toStdString());

  if((pid = fork()) == 0)
    {
      if((pid = fork()) < 0)
	_exit(EXIT_FAILURE);
      else if(pid > 0)
	_exit(EXIT_SUCCESS);

      int sd = dup(static_cast<int> (socketDescriptor));

      ::close(static_cast<int> (socketDescriptor));

      if(sd == -1)
	_exit(EXIT_FAILURE);

      const char *envp[] = {ldLibraryPath.data(), NULL};

      if(execle(command.data(),
		command.data(),
		"--log-file",
		log_file_name.data(),
		"--maximum--accumulated-bytes",
		QString::number(maximum_accumulated_bytes).toStdString().data(),
		"--silence-timeout",
		list.value(5).toStdString().data(),
		"--socket-descriptor",
		QString::number(sd).toStdString().data(),
		"--ssl-tls-control-string",
		list.value(3).toStdString().data(),
		"--ssl-tls-key-size",
		list.value(4).toStdString().data(),
		"--tcp",
		NULL,
		envp) == -1)
	{
	  ::close(sd);
	  _exit(EXIT_FAILURE);
	}

      _exit(EXIT_SUCCESS);
    }
  else
    {
      ::close(static_cast<int> (socketDescriptor));
      waitpid(pid, NULL, 0);
    }
}

void spot_on_lite_daemon_tcp_listener::slot_start_timeout(void)
{
  if(isListening())
    return;

  /*
  ** 0 - IP Address
  ** 1 - Port
  ** 2 - Backlog
  ** 3 - SSL Control String
  ** 4 - SSL Key Size (Bits)
  ** 5 - Silence Timeout (Seconds)
  */

  QStringList list(m_configuration.split(",", QString::KeepEmptyParts));

  if(listen(QHostAddress(list.value(0)),
	    static_cast<quint16> (list.value(1).toInt())))
    setMaxPendingConnections(list.value(2).toInt());
}

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
#include <sys/socket.h>
#include <unistd.h>
}

#include <QStringList>

#include "spot-on-lite-daemon.h"
#include "spot-on-lite-daemon-tcp-listener.h"

spot_on_lite_daemon_tcp_listener::spot_on_lite_daemon_tcp_listener
(const QString &configuration, spot_on_lite_daemon *parent):QTcpServer(parent)
{
  /*
  ** The configuration is assumed to be correct.
  */

  connect(&m_start_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_start_timeout(void)));
  m_configuration = configuration;
  m_parent = parent;
  m_start_timer.start(5000);
}

spot_on_lite_daemon_tcp_listener::
~spot_on_lite_daemon_tcp_listener()
{
}

#if QT_VERSION >= 0x050000
void spot_on_lite_daemon_tcp_listener::incomingConnection
(qintptr socket_descriptor)
#else
void spot_on_lite_daemon_tcp_listener::incomingConnection
(int socket_descriptor)
#endif
{
  if(!m_parent)
    {
      ::close(static_cast<int> (socket_descriptor));
      return;
    }

  int sd = dup(static_cast<int> (socket_descriptor));

  ::close(static_cast<int> (socket_descriptor));

  if(sd == -1)
    return;

  QStringList list(m_configuration.split(",", QString::KeepEmptyParts));
  int listener_sd = static_cast<int> (socketDescriptor());
  int maximum_accumulated_bytes = m_parent->maximum_accumulated_bytes();
  int so_linger = list.value(6).toInt();
  pid_t pid = 0;
  std::string certificates_file_name
    (m_parent->certificates_file_name().toStdString());
  std::string command
    (m_parent->child_process_file_name().toStdString());
  std::string congestion_control_file_name
    (m_parent->congestion_control_file_name().toStdString());
  std::string ld_library_path
    (m_parent->child_process_ld_library_path().toStdString());
  std::string local_server_file_name
    (m_parent->local_server_file_name().toStdString());
  std::string log_file_name(m_parent->log_file_name().toStdString());
  std::string remote_identities_file_name
    (m_parent->remote_identities_file_name().toStdString());
  std::string server_identity(QString("%1:%2").
			      arg(serverAddress().toString()).
			      arg(serverPort()).toStdString());

  if((pid = fork()) == 0)
    {
      ::close(listener_sd);

      if(so_linger > -1)
	{
	  socklen_t length = 0;
	  struct linger l;

	  l.l_linger = so_linger;
	  l.l_onoff = 1;
	  length = static_cast<socklen_t> (sizeof(l));
	  setsockopt(sd, SOL_SOCKET, SO_LINGER, &l, length);
	}

      const char *envp[] = {ld_library_path.data(), NULL};

      if(execle(command.data(),
		command.data(),
		"--certificates-file",
		certificates_file_name.data(),
		"--congestion-control-file",
		congestion_control_file_name.data(),
		"--end-of-message-marker",
		list.value(7).toUtf8().toBase64().data(),
		"--identities-lifetime",
		list.value(9).toStdString().data(),
		"--local-server-file",
		local_server_file_name.data(),
		"--local-so-sndbuf",
		list.value(8).toStdString().data(),
		"--log-file",
		log_file_name.data(),
		"--maximum--accumulated-bytes",
		QString::number(maximum_accumulated_bytes).toStdString().data(),
		"--remote-identities-file",
		remote_identities_file_name.data(),
		"--server-identity",
		server_identity.data(),
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
      ::close(sd);

      /*
	while(waitpid(pid, NULL, 0) == -1)
	  if(errno != EINTR)
	    break;
      */
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
  */

  QStringList list(m_configuration.split(",", QString::KeepEmptyParts));

  if(listen(QHostAddress(list.value(0)), list.value(1).toUShort()))
    setMaxPendingConnections(list.value(2).toInt());
}

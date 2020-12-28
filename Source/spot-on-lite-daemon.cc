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
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
}

#include <iostream>
#include <limits>

#include <QCoreApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QHostAddress>
#include <QLocalServer>
#include <QLocalSocket>
#include <QSettings>
#include <QSocketNotifier>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStringList>
#include <QtConcurrent>
#include <QtDebug>

#include "spot-on-lite-daemon-tcp-listener.h"
#include "spot-on-lite-daemon-udp-listener.h"
#include "spot-on-lite-daemon.h"

int spot_on_lite_daemon::s_signal_fd[2];

spot_on_lite_daemon::spot_on_lite_daemon
(const QString &configuration_file_name):QObject()
{
  if(::socketpair(AF_UNIX, SOCK_STREAM, 0, s_signal_fd))
    qFatal("spot_on_lite_daemon::spot_on_lite_daemon(): "
	   "socketpair() failure. Exiting.");

  m_configuration_file_name = configuration_file_name;
  m_congestion_control_lifetime = 90; // Seconds
  m_congestion_control_timer.start(15000); // 15 Seconds
  m_local_so_rcvbuf_so_sndbuf = 32768; // 32 KiB
  m_local_socket_server_directory_name = QDir::tempPath();
  m_maximum_accumulated_bytes = 8 * 1024 * 1024; // 8 MiB
  m_signal_socket_notifier = new QSocketNotifier
    (s_signal_fd[1], QSocketNotifier::Read, this);
  m_start_timer.start(5000);
  m_statistics_file_name = QDir::tempPath() +
    QDir::separator() +
    "spot-on-lite-daemon-statistics.sqlite";
  connect(&m_congestion_control_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_purge_congestion_control_timeout(void)));
  connect(&m_peer_process_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_peer_process_timeout(void)));
  connect(&m_start_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_start_timeout(void)));
  connect(m_signal_socket_notifier,
	  SIGNAL(activated(int)),
	  this,
	  SLOT(slot_signal(void)));
}

spot_on_lite_daemon::spot_on_lite_daemon(void):QObject()
{
  m_congestion_control_lifetime = 90; // Seconds
  m_local_so_rcvbuf_so_sndbuf = 0;
  m_local_socket_server_directory_name = QDir::tempPath();
  m_maximum_accumulated_bytes = 0;
  m_signal_socket_notifier = nullptr;
}

spot_on_lite_daemon::~spot_on_lite_daemon()
{
  if(!m_congestion_control_file_name.isEmpty())
    QFile::remove(m_congestion_control_file_name);

  if(!m_remote_identities_file_name.isEmpty())
    QFile::remove(m_remote_identities_file_name);

  if(!m_statistics_file_name.isEmpty())
    QFile::remove(m_statistics_file_name);

  m_congestion_control_future.cancel();
  m_congestion_control_future.waitForFinished();
  m_congestion_control_timer.stop();

  if(m_local_server)
    QLocalServer::removeServer(m_local_server->fullServerName());

  m_peer_process_timer.stop();
  m_start_timer.stop();
}

QString spot_on_lite_daemon::certificates_file_name(void) const
{
  return m_certificates_file_name;
}

QString spot_on_lite_daemon::child_process_file_name(void) const
{
  return m_child_process_file_name;
}

QString spot_on_lite_daemon::child_process_ld_library_path(void) const
{
  return m_child_process_ld_library_path;
}

QString spot_on_lite_daemon::configuration_file_name(void) const
{
  return m_configuration_file_name;
}

QString spot_on_lite_daemon::congestion_control_file_name(void) const
{
  return m_congestion_control_file_name;
}

QString spot_on_lite_daemon::local_server_file_name(void) const
{
  if(m_local_server)
    return m_local_server->fullServerName();
  else
    return "";
}

QString spot_on_lite_daemon::log_file_name(void) const
{
  return m_log_file_name;
}

QString spot_on_lite_daemon::remote_identities_file_name(void) const
{
  return m_remote_identities_file_name;
}

int spot_on_lite_daemon::maximum_accumulated_bytes(void) const
{
  return m_maximum_accumulated_bytes;
}

void spot_on_lite_daemon::log(const QString &error) const
{
  QString e(error.trimmed());

  if(e.isEmpty())
    return;

  QFile file(m_log_file_name);

  if(file.open(QIODevice::Append | QIODevice::WriteOnly))
    {
      QDateTime dateTime(QDateTime::currentDateTime());

      file.write(dateTime.toString().toStdString().data());
      file.write("\n");
      file.write(e.toStdString().data());
      file.write("\n");
      file.close();
    }
}

void spot_on_lite_daemon::prepare_listeners(void)
{
  while(!m_listeners.isEmpty())
    m_listeners.takeFirst()->deleteLater();

  for(int i = 0; i < m_listeners_properties.size(); i++)
    if(m_listeners_properties.at(i).contains("tcp"))
      {
	auto *listener = new spot_on_lite_daemon_tcp_listener
	  (m_listeners_properties.at(i), this);

	connect(this,
		SIGNAL(child_died(const pid_t)),
		listener,
		SLOT(slot_child_died(const pid_t)));
	m_listeners << listener;
      }
    else
      m_listeners << new spot_on_lite_daemon_udp_listener
	(m_listeners_properties.at(i), this);
}

void spot_on_lite_daemon::prepare_local_socket_server(void)
{
  QFileInfo file_info(QString("%1/Spot-On-Lite-Daemon-Local-Server.%2").
		      arg(m_local_socket_server_directory_name).
		      arg(QCoreApplication::applicationPid()));

  if(file_info.exists() && file_info.isReadable() && file_info.isWritable() &&
     m_local_server && m_local_server->isListening())
    return;

  if(m_local_server)
    {
      m_local_server->close();
      m_local_server->deleteLater();
    }

  m_local_server = new QLocalServer(this);
  m_local_server->listen
    (QString("%1/Spot-On-Lite-Daemon-Local-Server.%2").
     arg(m_local_socket_server_directory_name).
     arg(QCoreApplication::applicationPid()));
  m_local_sockets.clear();
  connect(m_local_server,
	  SIGNAL(newConnection(void)),
	  this,
	  SLOT(slot_new_local_connection(void)));

  if(!m_local_server->isListening())
    {
      m_local_server->deleteLater();
      m_local_sockets.clear();
    }
}

void spot_on_lite_daemon::prepare_peers(void)
{
  for(int i = 0; i < m_peers_properties.size(); i++)
    {
      if(m_peer_pids.contains(i))
	if(!kill(m_peer_pids.value(i), 0))
	  continue;

      QString server_identity;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
      QStringList list
	(m_peers_properties.at(i).split(",", Qt::KeepEmptyParts));
#else
      QStringList list
	(m_peers_properties.at(i).split(",", QString::KeepEmptyParts));
#endif
      auto so_linger = list.value(6).toInt();
      pid_t pid = 0;
      std::string command(m_child_process_file_name.toStdString());
#ifdef Q_OS_MAC
      std::string ld_library_path
	(m_child_process_ld_library_path.remove("DYLD_LIBRARY_PATH=").
	 toStdString());
#else
      std::string ld_library_path
	(m_child_process_ld_library_path.remove("LD_LIBRARY_PATH").
	 toStdString());
#endif

      server_identity = QString("%1:%2").arg(list.value(0)).arg(list.value(1));

      if((pid = fork()) == 0)
	{
	  const char *envp[] = {ld_library_path.data(), nullptr};

	  if(execle(command.data(),
		    command.data(),
		    "--certificates-file",
		    m_certificates_file_name.toStdString().data(),
		    "--configuration-file",
		    m_configuration_file_name.toStdString().data(),
		    "--congestion-control-file",
		    m_congestion_control_file_name.toStdString().data(),
		    "--end-of-message-marker",
		    list.value(7).toUtf8().toBase64().data(),
		    "--identities-lifetime",
		    list.value(9).toStdString().data(),
		    "--local-server-file",
		    local_server_file_name().toStdString().data(),
		    "--local-so-rcvbuf-so-sndbuf",
		    list.value(8).toStdString().data(),
		    "--log-file",
		    m_log_file_name.toStdString().data(),
		    "--maximum-accumulated-bytes",
		    QString::number(m_maximum_accumulated_bytes).
		    toStdString().data(),
		    "--remote-identities-file",
		    m_remote_identities_file_name.toStdString().data(),
		    "--server-identity",
		    server_identity.toStdString().data(),
		    "--silence-timeout",
		    list.value(5).toStdString().data(),
		    "--so-linger",
		    QString::number(so_linger).toStdString().data(),
		    "--socket-descriptor",
		    "-1",
		    "--ssl-tls-control-string",
		    list.value(3).toStdString().data(),
		    "--ssl-tls-key-size",
		    list.value(4).toStdString().data(),
		    m_peers_properties.at(i).contains("tcp") ?
		    "--tcp" : "--udp",
		    NULL,
		    envp) == -1)
	    _exit(EXIT_FAILURE);

	  _exit(EXIT_SUCCESS);
	}
      else if(pid > 0)
	m_peer_pids[i] = pid;
    }
}

void spot_on_lite_daemon::process_configuration_file(bool *ok)
{
  QHash<QString, char> listeners;
  QHash<QString, char> peers;
  QSettings settings(m_configuration_file_name, QSettings::IniFormat);
  auto o = true;

  foreach(const auto &key, settings.allKeys())
    if(key == "certificates_file")
      {
	QFileInfo file_info(settings.value(key).toString());

	file_info = QFileInfo(file_info.absolutePath());

	if(!file_info.isDir())
	  {
	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): "
		      << "The parent directory \""
		      << file_info.absoluteFilePath().toStdString()
		      << "\" of the certificates file is "
		      << "not a directory. Ignoring entry."
		      << std::endl;
	  }
	else if(!file_info.isWritable())
	  {
	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): "
		      << "The parent directory \""
		      << file_info.absoluteFilePath().toStdString()
		      << "\" of the certificates file must be "
		      << "writable. Ignoring entry."
		      << std::endl;
	  }
	else
	  m_certificates_file_name = settings.value(key).toString();
      }
    else if(key == "child_process_file")
      {
	QFileInfo file_info(settings.value(key).toString());

	if(file_info.isExecutable() &&
	   file_info.isFile() &&
	   file_info.isReadable())
	  m_child_process_file_name = file_info.absoluteFilePath();
	else
	  {
	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): "
		      << "The child process file \""
		      << file_info.absoluteFilePath().toStdString()
		      << "\" must be a readable executable. Ignoring entry."
		      << std::endl;
	  }
      }
    else if(key == "child_process_ld_library_path")
      m_child_process_ld_library_path =
	settings.value(key).toString().trimmed();
    else if(key == "congestion_control_file")
      {
	QFileInfo file_info(settings.value(key).toString());

	file_info = QFileInfo(file_info.absolutePath());

	if(!file_info.isDir())
	  {
	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): "
		      << "The parent directory \""
		      << file_info.absoluteFilePath().toStdString()
		      << "\" of the congestion control file is "
		      << "not a directory. Ignoring entry."
		      << std::endl;
	  }
	else if(!file_info.isWritable())
	  {
	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): "
		      << "The parent directory \""
		      << file_info.absoluteFilePath().toStdString()
		      << "\" of the congestion control file must be "
		      << "writable. Ignoring entry."
		      << std::endl;
	  }
	else
	  {
	    m_congestion_control_file_name = settings.value(key).toString();

	    if(!m_congestion_control_file_name.isEmpty())
	      QFile::remove(m_congestion_control_file_name);
	  }
      }
    else if(key == "congestion_control_lifetime")
      {
	auto congestion_control_lifetime = settings.value(key).toInt(&o);

	if(congestion_control_lifetime < 1 || !o)
	  {
	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): The "
		      << "congestion_control_lifetime value \""
		      << settings.value(key).toString().toStdString()
		      << "\" is invalid. "
		      << "Expecting a value "
		      << "in the range [1, "
		      << std::numeric_limits<int>::max()
		      << "]. Ignoring entry."
		      << std::endl;
	  }
	else
	  m_congestion_control_lifetime.fetchAndStoreAcquire
	    (congestion_control_lifetime);
      }
    else if(key == "local_so_rcvbuf_so_sndbuf")
      {
	auto so_rcvbuf_so_sndbuf = settings.value(key).toInt(&o);

	if(so_rcvbuf_so_sndbuf < 4096 || !o)
	  {
	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): The "
		      << "local_so_rcvbuf_so_sndbuf value \""
		      << settings.value(key).toString().toStdString()
		      << "\" is invalid. "
		      << "Expecting a value that is greater than or "
		      << "equal to 4096. Ignoring entry."
		      << std::endl;
	  }
	else
	  m_local_so_rcvbuf_so_sndbuf = so_rcvbuf_so_sndbuf;
      }
    else if(key == "local_socket_server_directory")
      {
	QFileInfo file_info(settings.value(key).toString());

	if(!file_info.isDir())
	  {
	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): "
		      << "The directory \""
		      << file_info.absoluteFilePath().toStdString()
		      << "\" is not a directory. "
		      << "Ignoring entry."
		      << std::endl;
	  }
	else if(!file_info.isReadable() || !file_info.isWritable())
	  {
	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): "
		      << "The directory \""
		      << file_info.absoluteFilePath().toStdString()
		      << "\" must be readable and writable. Ignoring entry."
		      << std::endl;
	  }
	else
	  m_local_socket_server_directory_name = settings.value(key).toString();
      }
    else if(key == "log_file")
      {
	QFileInfo file_info(settings.value(key).toString());

	file_info = QFileInfo(file_info.absolutePath());

	if(!file_info.isDir())
	  {
	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): "
		      << "The parent directory \""
		      << file_info.absoluteFilePath().toStdString()
		      << "\" of the log file is not a directory. "
		      << "Ignoring entry."
		      << std::endl;
	  }
	else if(!file_info.isWritable())
	  {
	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): "
		      << "The parent directory \""
		      << file_info.absoluteFilePath().toStdString()
		      << "\" of the log file is not writable. "
		      << "Ignoring entry."
		      << std::endl;
	  }
	else
	  m_log_file_name = settings.value(key).toString();
      }
    else if(key == "maximum_accumulated_bytes")
      {
	auto maximum_accumulated_bytes = settings.value(key).toInt(&o);

	if(maximum_accumulated_bytes < 1024 || !o)
	  {
	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): The "
		      << "maximum_accumulated_bytes value \""
		      << settings.value(key).toString().toStdString()
		      << "\" is invalid. "
		      << "Expecting a value "
		      << "in the range [1024, "
		      << std::numeric_limits<int>::max()
		      << "]. Ignoring entry."
		      << std::endl;
	  }
	else
	  m_maximum_accumulated_bytes = maximum_accumulated_bytes;
      }
    else if(key.startsWith("listener") || key.startsWith("peer"))
      {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
	QStringList list
	  (settings.value(key).toString().split(",", Qt::KeepEmptyParts));
#else
	QStringList list
	  (settings.value(key).toString().split(",", QString::KeepEmptyParts));
#endif

	/*
	** 0  - IP Address
	** 1  - Port
	** 2  - Backlog
	** 3  - SSL Control String
	** 4  - SSL Key Size (Bits)
	** 5  - Silence Timeout (Seconds)
	** 6  - SO Linger (Seconds)
	** 7  - End-of-Message-Marker
	** 8  - Local SO_RCVBUF / SO_SNDBUF
	** 9  - Identities Lifetime (Seconds)
	** 10 - Protocol
	*/

	int expected = 11;

	if(list.size() != expected)
	  {
	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): The "
		      << "key \""
		      << key.toStdString()
		      << "\" does not contain "
		      << expected
		      << " attributes. Ignoring entry."
		      << std::endl;
	    continue;
	  }

	for(int i = 0; i < list.size(); i++)
	  if(i != 7) // EOM-Marker
	    list.replace(i, list.at(i).trimmed());

	bool entry_ok = true;

	if(list.at(0).isEmpty())
	  {
	    entry_ok = false;

	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): The "
		      << "listener/peer \""
		      << key.toStdString()
		      << "\" IP address is invalid. Ignoring entry."
		      << std::endl;
	  }

	auto port = list.at(1).toInt(&o);

	if(!o || port < 0 || port > 65535)
	  {
	    entry_ok = false;

	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): The "
		      << "listener/peer \""
		      << key.toStdString()
		      << "\" port number is invalid. Expecting a port number "
		      << "in the range [0, 65535]. Ignoring entry."
		      << std::endl;
	  }

	auto backlog = list.at(2).toInt(&o);

	if(backlog < 1 || !o)
	  {
	    entry_ok = false;

	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): The "
		      << "listener/peer \""
		      << key.toStdString()
		      << "\" backlog is invalid. Expecting a value "
		      << "in the range [1, "
		      << std::numeric_limits<int>::max()
		      << "]. Ignoring entry."
		      << std::endl;
	  }

	if(!list.at(4).isEmpty())
	  {
	    auto keySize = list.at(4).toInt(&o);

	    if(!(keySize == 2048 ||
		 keySize == 3072 ||
		 keySize == 4096 ||
		 keySize == 7680) ||
	       !o)
	      {
		entry_ok = false;

		if(ok)
		  *ok = false;

		std::cerr << "spot_on_lite_daemon::"
			  << "process_configuration_file(): The "
			  << "listener/peer \""
			  << key.toStdString()
			  << "\" SSL/TLS key size is invalid. "
			  << "Expecting a value "
			  << "in the set {2048, 3072, 4096, 7680}. "
			  << "Ignoring entry."
			  << std::endl;
	      }
	  }

	auto silence = list.at(5).toInt(&o);

	if(!o || silence < 15 || silence > 3600)
	  {
	    entry_ok = false;

	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): The "
		      << "listener/peer \""
		      << key.toStdString()
		      << "\" silence value is invalid. Expecting a value "
		      << "in the range [15, 3600]. Ignoring entry."
		      << std::endl;
	  }

	auto so_linger = list.at(6).toInt(&o);

	if(!o || so_linger < -1 || so_linger > 65535)
	  {
	    entry_ok = false;

	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): The "
		      << "listener/peer \""
		      << key.toStdString()
		      << "\" linger value is invalid. Expecting a value "
		      << "in the range [-1, 65535]. Ignoring entry."
		      << std::endl;
	  }

	auto so_rcvbuf_so_sndbuf = list.at(8).toInt(&o);

	if(!o || so_rcvbuf_so_sndbuf < 4096)
	  {
	    entry_ok = false;

	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): The "
		      << "listener/peer \""
		      << key.toStdString()
		      << "\" local so_rcv_so_sndbuf value is invalid. "
		      << "Expecting a value that is greater than or "
		      << "equal to 4096. Ignoring entry."
		      << std::endl;
	  }

	auto identities_lifetime = list.at(9).toInt(&o);

	if(identities_lifetime < 30 || identities_lifetime > 600 || !o)
	  {
	    entry_ok = false;

	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): The "
		      << "listener/peer \""
		      << key.toStdString()
		      << "\" identities lifetime value is invalid. "
		      << "Expecting a value "
		      << "in the range [90, 600]. Ignoring entry."
		      << std::endl;
	  }
	
	QString protocol(list.value(10));

	if(!(protocol == "tcp" || protocol == "udp"))
	  {
	    entry_ok = false;

	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): The "
		      << "listener/peer \""
		      << key.toStdString()
		      << "\" protocol value is invalid. "
		      << "Expecting a value of tcp or udp. "
		      << "Ignoring entry."
		      << std::endl;
	  }

	if(key.startsWith("listener"))
	  {
	    if(entry_ok && listeners.contains(list.at(0) + list.at(1)))
	      {
		if(ok)
		  *ok = false;

		std::cerr << "spot_on_lite_daemon::"
			  << "process_configuration_file(): The "
			  << "listener ("
			  << list.at(0).toStdString()
			  << ":"
			  << list.at(1).toStdString()
			  << ") is a duplicate. Ignoring entry."
			  << std::endl;
	      }
	    else if(entry_ok)
	      m_listeners_properties << settings.value(key).toString();

	    if(entry_ok)
	      listeners[list.at(0) + list.at(1)] = 0;
	  }
	else
	  {
	    if(entry_ok && peers.contains(list.at(0) + list.at(1)))
	      {
		if(ok)
		  *ok = false;

		std::cerr << "spot_on_lite_daemon::"
			  << "process_configuration_file(): The "
			  << "peer ("
			  << list.at(0).toStdString()
			  << ":"
			  << list.at(1).toStdString()
			  << ") is a duplicate. Ignoring entry."
			  << std::endl;
	      }
	    else if(entry_ok)
	      m_peers_properties << settings.value(key).toString();

	    if(entry_ok)
	      peers[list.at(0) + list.at(1)] = 0;
	  }
      }
    else if(key == "remote_identities_file")
      {
	QFileInfo file_info(settings.value(key).toString());

	file_info = QFileInfo(file_info.absolutePath());

	if(!file_info.isDir())
	  {
	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): "
		      << "The parent directory \""
		      << file_info.absoluteFilePath().toStdString()
		      << "\" of the remote identities file is "
		      << "not a directory. Ignoring entry."
		      << std::endl;
	  }
	else if(!file_info.isWritable())
	  {
	    if(ok)
	      *ok = false;

	    std::cerr << "spot_on_lite_daemon::"
		      << "process_configuration_file(): "
		      << "The parent directory \""
		      << file_info.absoluteFilePath().toStdString()
		      << "\" of the remote identities file must be "
		      << "writable. Ignoring entry."
		      << std::endl;
	  }
	else
	  {
	    m_remote_identities_file_name = settings.value(key).toString();

	    if(!m_remote_identities_file_name.isEmpty())
	      QFile::remove(m_remote_identities_file_name);
	  }
      }
}

void spot_on_lite_daemon::purge_congestion_control(void)
{
  {
    auto db = QSqlDatabase::addDatabase
      ("QSQLITE", "congestion_control_database");

    db.setDatabaseName(m_congestion_control_file_name);

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA journal_mode = OFF");
	query.exec("PRAGMA synchronous = OFF");
	query.exec
	  (QString("DELETE FROM congestion_control WHERE "
		   "%1 - date_time_inserted > %2").
	   arg(QDateTime::currentDateTime().toTime_t()).
	   arg(m_congestion_control_lifetime.fetchAndAddAcquire(0)));
      }

    db.close();
  }

  QSqlDatabase::removeDatabase("congestion_control_database");
}

void spot_on_lite_daemon::slot_local_socket_disconnected(void)
{
  auto *socket = qobject_cast<QLocalSocket *> (sender());

  if(!socket)
    return;

  m_local_sockets.remove(socket);
  socket->deleteLater();
}

void spot_on_lite_daemon::slot_new_local_connection(void)
{
  if(!m_local_server)
    return;

  auto *socket = m_local_server->nextPendingConnection();

  if(!socket)
    return;
  else
    socket->setReadBufferSize(m_maximum_accumulated_bytes);

  auto optlen = static_cast<socklen_t> (sizeof(m_local_so_rcvbuf_so_sndbuf));
  auto sockfd = static_cast<int> (socket->socketDescriptor());

  setsockopt
    (sockfd, SOL_SOCKET, SO_RCVBUF, &m_local_so_rcvbuf_so_sndbuf, optlen);
  setsockopt
    (sockfd, SOL_SOCKET, SO_SNDBUF, &m_local_so_rcvbuf_so_sndbuf, optlen);
  m_local_sockets[socket] = 0;
  connect(socket,
	  SIGNAL(disconnected(void)),
	  this,
	  SLOT(slot_local_socket_disconnected(void)));
  connect(socket,
	  SIGNAL(readyRead(void)),
	  this,
	  SLOT(slot_ready_read(void)));
}

void spot_on_lite_daemon::slot_peer_process_timeout(void)
{
  prepare_peers();
}

void spot_on_lite_daemon::slot_purge_congestion_control_timeout(void)
{
  if(m_congestion_control_future.isFinished())
    m_congestion_control_future = QtConcurrent::run
      (this, &spot_on_lite_daemon::purge_congestion_control);
}

void spot_on_lite_daemon::slot_ready_read(void)
{
  auto *socket = qobject_cast<QLocalSocket *> (sender());

  if(!socket)
    return;

  QByteArray data;

  while(socket->bytesAvailable() > 0)
    data.append(socket->readAll());

  QHashIterator<QLocalSocket *, char> it(m_local_sockets);

  while(it.hasNext())
    {
      it.next();

      if(it.key() &&
	 it.key() != socket &&
	 it.key()->state() == QLocalSocket::ConnectedState)
	{
	  int maximum = m_local_so_rcvbuf_so_sndbuf -
	    static_cast<int> (it.key()->bytesToWrite());

	  if(maximum > 0)
	    it.key()->write(data.mid(0, maximum));
	}
    }
}

void spot_on_lite_daemon::slot_signal(void)
{
  if(!m_signal_socket_notifier)
    return;

  m_signal_socket_notifier->setEnabled(false);

  char a[32];

  memset(a, 0, sizeof(a));

  ssize_t rc = ::read(s_signal_fd[1], a, sizeof(a));

  if(rc > 0)
    {
      if(a[0] == 'c' || a[0] == 'u')
	{
	  if(a[0] == 'c')
	    {
	      QString string(a);

	      string = string.mid(1);
	      std::reverse(string.begin(), string.end());

	      auto pid = static_cast<pid_t> (string.toLongLong());

	      if(kill(pid, 0) == -1 && errno == ESRCH)
		emit child_died(pid);
	    }
	  else
	    start();

	  m_signal_socket_notifier->setEnabled(true);
	}
      else
	QCoreApplication::exit(0);
    }
  else
    QCoreApplication::exit(0);
}

void spot_on_lite_daemon::slot_start_timeout(void)
{
  prepare_local_socket_server();
}

void spot_on_lite_daemon::start(void)
{
  kill(0, SIGUSR2); // Terminate existing children.
  m_listeners_properties.clear();

  if(m_local_server)
    {
      m_local_server->close();
      m_local_server->removeServer(m_local_server->fullServerName());
      m_local_server->deleteLater();
      m_local_server = nullptr;
    }

  m_local_sockets.clear();
  m_peer_pids.clear();
  m_peer_process_timer.start(2500);
  m_peers_properties.clear();
  process_configuration_file(nullptr);
  prepare_listeners();
  prepare_local_socket_server();
}

void spot_on_lite_daemon::validate_configuration_file
(const QString &configuration_file_name, bool *ok)
{
  m_configuration_file_name = configuration_file_name;
  m_listeners_properties.clear();
  m_peers_properties.clear();
  process_configuration_file(ok);

  for(const auto &i : m_listeners_properties)
    qDebug() << i.split(',');

  for(const auto &i : m_peers_properties)
    qDebug() << i.split(',');
}

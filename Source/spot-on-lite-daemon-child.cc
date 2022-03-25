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

#include <QtGlobal>

extern "C"
{
#include <errno.h>
#ifdef Q_OS_LINUX
#include <linux/sockios.h>
#endif
#ifdef SPOTON_LITE_DAEMON_OPENSSL_SUPPORTED
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#endif
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
}

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QLocalSocket>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSslKey>
#include <QSslSocket>
#include <QStringList>
#include <QTimer>
#include <QUdpSocket>
#include <QUuid>
#include <QtConcurrent>

#include <limits>

#include "spot-on-lite-common.h"
#include "spot-on-lite-daemon-child.h"

QAtomicInteger<quint64> spot_on_lite_daemon_child::s_db_id = 0;

static QString socket_type_to_string
(const QAbstractSocket::SocketType socket_type)
{
  if(socket_type == QAbstractSocket::TcpSocket)
    return "TCP";
  else
    return "UDP";
}

static int hash_algorithm_key_length(const QByteArray &a)
{
  auto algorithm(a.toLower().trimmed());

  if(algorithm == "sha-512" || algorithm == "sha3-512")
    return 64;
  else
    return 0;
}

#ifdef SPOTON_LITE_DAEMON_ENABLE_IDENTITIES_CONTAINER
#if SPOTON_LITE_DAEMON_IDENTITIES_CONTAINER_MAXIMUM_SIZE > 0
static int MAXIMUM_REMOTE_IDENTITIES =
  SPOTON_LITE_DAEMON_IDENTITIES_CONTAINER_MAXIMUM_SIZE;
#endif
#endif
static int END_OF_MESSAGE_MARKER_WINDOW = 10000;
static int MAXIMUM_TCP_WRITE_SIZE = 8192;
static int MAXIMUM_UDP_WRITE_SIZE = 508;
static int s_certificate_version = 2; // TLS 1.3 is sensitive.

spot_on_lite_daemon_child::spot_on_lite_daemon_child
(const QByteArray &initial_data,
 const QString &certificates_file_name,
 const QString &configuration_file_name,
 const QString &congestion_control_file_name,
 const QString &end_of_message_marker,
 const QString &local_server_file_name,
 const QString &log_file_name,
 const QString &peer_address,
 const QString &peer_scope_identity,
 const QString &protocol,
 const QString &remote_identities_file_name,
 const QString &schedule,
 const QString &server_identity,
 const QString &ssl_control_string,
 const int certificate_lifetime,
 const int identities_lifetime,
 const int local_so_rcvbuf_so_sndbuf,
 const int maximum_accumulated_bytes,
 const int silence,
 const int so_linger,
 const int socket_descriptor,
 const int ssl_key_size,
 const quint16 peer_port):QObject()
{
  m_attempt_local_connection_timer.setInterval(2500);
  m_attempt_remote_connection_timer.setInterval(2500);
  m_bytes_read = 0ULL;
  m_bytes_written = 0ULL;
  m_certificate_lifetime = qBound
    (1, certificate_lifetime, std::numeric_limits<int>::max());
  m_certificates_file_name = certificates_file_name;
  m_client_role = socket_descriptor < 0;
  m_configuration_file_name = configuration_file_name;
  m_congestion_control_file_name = congestion_control_file_name;
  m_end_of_message_marker = end_of_message_marker.toUtf8();
  m_general_timer.start(5000);
  m_identity_lifetime = 1000 * static_cast<unsigned int>
    (qBound(5, identities_lifetime, 600));
  m_local_content_last_parsed = QDateTime::currentMSecsSinceEpoch();
  m_local_server_file_name = local_server_file_name;
  m_local_so_rcvbuf_so_sndbuf = qMax(4096, local_so_rcvbuf_so_sndbuf);
  m_log_file_name = log_file_name;
  m_maximum_accumulated_bytes = maximum_accumulated_bytes;

  if(m_maximum_accumulated_bytes < 1024)
    m_maximum_accumulated_bytes = 8 * 1024 * 1024;

  m_peer_address = QHostAddress(peer_address);
  m_peer_address.setScopeId(peer_scope_identity);
  m_peer_port = peer_port;
  m_pid = QCoreApplication::applicationPid();
  m_protocol = protocol.toLower().trimmed() == "tcp" ?
    QAbstractSocket::TcpSocket : QAbstractSocket::UdpSocket;
  m_remote_content_last_parsed = QDateTime::currentMSecsSinceEpoch();
  m_remote_identities_file_name = remote_identities_file_name;

  if(m_protocol == QAbstractSocket::TcpSocket)
    m_remote_socket = new QSslSocket(this);
  else
    m_remote_socket = new QUdpSocket(this);

  m_remote_socket->setReadBufferSize(m_maximum_accumulated_bytes);
  m_server_identity = server_identity;
  m_silence = silence == 0 ? silence : 1000 * qBound(15, silence, 3600);
  m_so_linger = qBound(-1, so_linger, std::numeric_limits<int>::max());
  m_spot_on_lite = m_client_role;
  m_ssl_control_string = ssl_control_string.trimmed();
  m_ssl_key_size = ssl_key_size;
  m_statistics_file_name = QDir::tempPath() +
    QDir::separator() +
    "spot-on-lite-daemon-statistics.sqlite";
  save_statistic("pid", QString::number(QCoreApplication::applicationPid()));

  if(!(m_ssl_key_size == 256 ||
       m_ssl_key_size == 384 ||
       m_ssl_key_size == 521 ||
       m_ssl_key_size == 2048 ||
       m_ssl_key_size == 3072 ||
       m_ssl_key_size == 4096 ||
       m_ssl_key_size == 7680))
    {
      m_ssl_control_string.clear();
      m_ssl_key_size = 0;
    }

  if(m_client_role)
    {
      connect(&m_attempt_remote_connection_timer,
	      SIGNAL(timeout(void)),
	      this,
	      SLOT(slot_attempt_remote_connection(void)));
      connect(m_remote_socket,
	      SIGNAL(connected(void)),
	      this,
	      SLOT(slot_connected(void)));
      save_statistic("type", "client");
    }
  else
    {
      if(!m_remote_socket->
	 setSocketDescriptor(socket_descriptor,
			     m_protocol == QAbstractSocket::TcpSocket ?
			     QAbstractSocket::ConnectedState :
			     QAbstractSocket::BoundState))
	{
	  /*
	  ** Fatal error!
	  */

	  ::close(socket_descriptor);
	  log("spot_on_lite_daemon_child::"
	      "spot_on_lite_daemon_child(): "
	      "invalid socket descriptor.");
	  QTimer::singleShot(2500, this, SLOT(slot_disconnected(void)));
	  return;
	}

      m_attempt_local_connection_timer.start();
      m_capabilities_timer.start(qMax(7500, m_silence / 2));
      m_remote_socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
      save_statistic("ip_information",
		     m_remote_socket->peerAddress().toString() +
		     ":" +
		     QString::number(m_remote_socket->peerPort()) +
		     ":" +
		     socket_type_to_string(m_remote_socket->socketType()));
      save_statistic("type", "server");
    }

  m_expired_identities_timer.start(15000);

  if(m_silence > 0)
    m_keep_alive_timer.start(m_silence);

  m_local_socket.setReadBufferSize(m_maximum_accumulated_bytes);
  connect(&m_attempt_local_connection_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_attempt_local_connection(void)));
  connect(&m_capabilities_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_broadcast_capabilities(void)));
  connect(&m_expired_identities_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_remove_expired_identities(void)));
  connect(&m_general_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_general_timer_timeout(void)));
  connect(&m_keep_alive_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_keep_alive_timer_timeout(void)));
  connect(&m_local_socket,
	  SIGNAL(connected(void)),
	  &m_attempt_local_connection_timer,
	  SLOT(stop(void)));
  connect(&m_local_socket,
	  SIGNAL(connected(void)),
	  this,
	  SLOT(slot_local_socket_connected(void)));
  connect(&m_local_socket,
	  SIGNAL(disconnected(void)),
	  this,
	  SLOT(slot_local_socket_disconnected(void)));
  connect(&m_local_socket,
	  SIGNAL(readyRead(void)),
	  this,
	  SLOT(slot_local_socket_ready_read(void)));
  connect(m_remote_socket,
	  SIGNAL(disconnected(void)),
	  this,
	  SLOT(slot_disconnected(void)));
  connect(m_remote_socket,
	  SIGNAL(error(QAbstractSocket::SocketError)),
	  this,
	  SLOT(slot_error(QAbstractSocket::SocketError)));
  connect(m_remote_socket,
	  SIGNAL(readyRead(void)),
	  this,
	  SLOT(slot_ready_read(void)));
  connect(this,
	  SIGNAL(read_signal(void)),
	  this,
	  SLOT(slot_local_socket_ready_read(void)));
  connect(this,
	  SIGNAL(write_signal(const QByteArray &)),
	  this,
	  SLOT(slot_write_data(const QByteArray &)));

  if(!m_ssl_control_string.isEmpty() && m_ssl_key_size > 0)
    {
#ifdef SPOTON_LITE_DAEMON_OPENSSL_SUPPORTED
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(Q_OS_OPENBSD)
      auto rc = SSL_library_init(); // Always returns 1.
#else
      auto rc = OPENSSL_init_ssl(0, nullptr);
#endif
#else
      int rc = 0;
#endif

      if(m_protocol == QAbstractSocket::TcpSocket && rc == 1)
	{
	  connect(qobject_cast<QSslSocket *> (m_remote_socket),
		  SIGNAL(peerVerifyError(const QSslError &)),
		  this,
		  SLOT(slot_peer_verify_error(const QSslError &)));
	  connect(qobject_cast<QSslSocket *> (m_remote_socket),
		  SIGNAL(sslErrors(const QList<QSslError> &)),
		  this,
		  SLOT(slot_ssl_errors(const QList<QSslError> &)));
	}

      if(m_client_role && rc == 1)
	{
	  generate_ssl_tls();

	  auto list(m_server_identity.split(":"));

	  m_peer_address = QHostAddress(list.value(0));
	  m_peer_port = static_cast<quint16> (list.value(1).toInt());

	  if(m_protocol == QAbstractSocket::TcpSocket)
	    qobject_cast<QSslSocket *> (m_remote_socket)->
	      connectToHostEncrypted(m_peer_address.toString(), m_peer_port);
	  else
	    {
	      m_remote_socket->connectToHost(m_peer_address, m_peer_port);
	      m_remote_socket->waitForConnected(10000);
#ifdef SPOTON_LITE_DAEMON_DTLS_SUPPORTED
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
	      prepare_dtls();

	      if(m_dtls)
		/*
		** The client initiates DTLS.
		*/

		if(!m_dtls->
		   doHandshake(qobject_cast<QUdpSocket *> (m_remote_socket)))
		  {
		    log(QString("spot_on_lite_daemon_child::"
				"spot_on_lite_daemon_child(): "
				"doHandshake() failure (%1).").
			arg(m_dtls->dtlsErrorString()));

		    if(!(m_dtls->dtlsError() == QDtlsError::NoError ||
			 m_dtls->dtlsError() == QDtlsError::TlsNonFatalError))
		      {
			/*
			** Fatal error!
			*/

			QTimer::singleShot
			  (2500, this, SLOT(slot_disconnected(void)));
			m_dtls->abortHandshake
			  (qobject_cast<QUdpSocket *> (m_remote_socket));
			return;
		      }
		  }
#endif
#endif
	    }
	}
      else if(rc == 1)
	{
	  auto list(local_certificate_configuration());

	  if(list.isEmpty())
	    generate_ssl_tls();
	  else
	    prepare_ssl_tls_configuration(list);

	  if(m_protocol == QAbstractSocket::TcpSocket)
	    qobject_cast<QSslSocket *> (m_remote_socket)->
	      startServerEncryption();
#ifdef SPOTON_LITE_DAEMON_DTLS_SUPPORTED
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
	  else
	    {
	      prepare_dtls();

	      if(m_dtls)
		/*
		** The server's DTLS response.
		*/

		if(!m_dtls->
		   doHandshake(qobject_cast<QUdpSocket *> (m_remote_socket),
			       initial_data))
		  {
		    log(QString("spot_on_lite_daemon_child::"
				"spot_on_lite_daemon_child: "
				"doHandshake() failure (%1).").
			arg(m_dtls->dtlsErrorString()));

		    if(!(m_dtls->dtlsError() == QDtlsError::NoError ||
			 m_dtls->dtlsError() == QDtlsError::TlsNonFatalError))
		      {
			/*
			** Fatal error!
			*/

			m_dtls->abortHandshake
			  (qobject_cast<QUdpSocket *> (m_remote_socket));
			QTimer::singleShot
			  (2500, this, SLOT(slot_disconnected(void)));
			return;
		      }
		  }
	    }
#endif
#endif
	}
    }
  else if(m_client_role)
    {
      auto list(m_server_identity.split(":"));

      m_peer_address = QHostAddress(list.value(0));
      m_peer_port = static_cast<quint16> (list.value(1).toInt());
      m_remote_socket->connectToHost(m_peer_address, m_peer_port);
    }
  else if(m_protocol == QAbstractSocket::UdpSocket)
    process_read_data(initial_data);

  process_configuration_file();
  save_statistic
    ("arguments", QCoreApplication::instance()->arguments().join(' '));
  save_statistic("name", QCoreApplication::instance()->arguments().value(0));
  spot_on_lite_common::set_schedule(schedule);
}

spot_on_lite_daemon_child::~spot_on_lite_daemon_child()
{
  purge_statistics();
  stop_threads_and_timers();
}

QHash<QByteArray, QString>spot_on_lite_daemon_child::
remote_identities(bool *ok) const
{
  if(ok)
    *ok = true;

  QHash<QByteArray, QString> hash;

#ifdef SPOTON_LITE_DAEMON_ENABLE_IDENTITIES_CONTAINER
  QReadLocker lock(&m_remote_identities_mutex);
  QHashIterator<QByteArray, QDateTime> it(m_remote_identities);

  while(it.hasNext())
    {
      it.next();
      hash[it.key()] = "sha-512";
    }
#else
  create_remote_identities_database();

  auto db_connection_id = db_id();

  {
    auto db = QSqlDatabase::addDatabase
      ("QSQLITE", QString::number(db_connection_id));

    db.setDatabaseName(m_remote_identities_file_name);

    if(db.open())
      {
	QSqlQuery query(db);

	query.setForwardOnly(true);
	query.prepare
	  ("SELECT algorithm, identity FROM remote_identities "
	   "WHERE pid = ? ORDER BY date_time_inserted DESC");
	query.addBindValue(m_pid);

	if(query.exec())
	  while(query.next())
	    hash[QByteArray::fromBase64(query.value(1).toByteArray())] =
	      query.value(0).toString();
	else if(ok)
	  *ok = false;
      }
    else if(ok)
      *ok = false;

    db.close();
  }

  QSqlDatabase::removeDatabase(QString::number(db_connection_id));
#endif
  return hash;
}

QList<QByteArray> spot_on_lite_daemon_child::
local_certificate_configuration(void) const
{
  QList<QByteArray> list;
  auto db_connection_id = db_id();

  {
    auto db = QSqlDatabase::addDatabase
      ("QSQLITE", QString::number(db_connection_id));

    db.setDatabaseName(m_certificates_file_name);

    if(db.open())
      {
	QSqlQuery query(db);

	query.setForwardOnly(true);
	query.prepare("SELECT certificate, private_key FROM certificates "
		      "WHERE server_identity = ?");
	query.addBindValue(m_server_identity);

	if(query.exec() && query.next())
	  list << QByteArray::fromBase64(query.value(0).toByteArray())
	       << QByteArray::fromBase64(query.value(1).toByteArray());
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(QString::number(db_connection_id));
  return list;
}

QList<QSslCipher> spot_on_lite_daemon_child::default_ssl_ciphers(void) const
{
  QList<QSslCipher> list;

  if(m_ssl_control_string.isEmpty())
    return list;

#ifdef SPOTON_LITE_DAEMON_OPENSSL_SUPPORTED
  QStringList protocols;
  SSL *ssl = nullptr;
  SSL_CTX *ctx = nullptr;
  const char *next = nullptr;
  int index = 0;

#if OPENSSL_VERSION_NUMBER < 0x10100000L
  protocols << "TlsV1_2"
	    << "TlsV1_1"
	    << "TlsV1_0";

  if(!m_ssl_control_string.toLower().contains("!sslv3"))
    protocols << "SslV3";
#else
  protocols << "TlsV1_3"
	    << "TlsV1_2"
	    << "TlsV1_1"
	    << "TlsV1_0";
#endif

  while(!protocols.isEmpty())
    {
      auto protocol(protocols.takeFirst());

      index = 0;
      next = nullptr;

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
      ctx = SSL_CTX_new(TLS_client_method());
#else
      if(protocol == "TlsV1_2")
	{
#ifdef TLS1_2_VERSION
	  if(!(ctx = SSL_CTX_new(TLSv1_2_client_method())))
	    {
	      log("spot_on_lite_daemon_child::"
		  "default_ssl_ciphers(): "
		  "SSL_CTX_new(TLSv1_2_client_method()) failure.");
	      goto done_label;
	    }
#endif
	}
      else if(protocol == "TlsV1_1")
	{
#ifdef TLS1_1_VERSION
	  if(!(ctx = SSL_CTX_new(TLSv1_1_method())))
	    {
	      log("spot_on_lite_daemon_child::"
		  "default_ssl_ciphers(): SSL_CTX_new(TLSv1_1_method()) "
		  "failure.");
	      goto done_label;
	    }
#endif
	}
      else if(protocol == "TlsV1_0")
	{
	  if(!(ctx = SSL_CTX_new(TLSv1_method())))
	    {
	      log
		("spot_on_lite_daemon_child::"
		 "default_ssl_ciphers(): SSL_CTX_new(TLSv1_method()) "
		 "failure.");
	      goto done_label;
	    }
	}
      else
	{
#ifndef OPENSSL_NO_SSL3_METHOD
	  if(!(ctx = SSL_CTX_new(SSLv3_method())))
	    {
	      log
		("spot_on_lite_daemon_child::"
		 "default_ssl_ciphers(): SSL_CTX_new(SSLv3_method()) "
		 "failure.");
	      goto done_label;
	    }
#endif
	}
#endif

      if(!ctx)
	{
	  log("spot_on_lite_daemon_child::"
	      "default_ssl_ciphers(): ctx is zero!");
	  continue;
	}

      if(SSL_CTX_set_cipher_list(ctx,
				 m_ssl_control_string.toLatin1().
				 constData()) == 0)
	{
	  log("spot_on_lite_daemon_child::"
	      "default_ssl_ciphers(): SSL_CTX_set_cipher_list() failure.");
	  goto done_label;
	}

      if(!(ssl = SSL_new(ctx)))
	{
	  log("spot_on_lite_daemon_child::"
	      "default_ssl_ciphers(): SSL_new() failure.");
	  goto done_label;
	}

      do
	{
	  if((next = SSL_get_cipher_list(ssl, index)))
	    {
	      QSslCipher cipher;

#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
	      if(protocol == "TlsV1_3")
		{
		  cipher = QSslCipher(next, QSsl::TlsV1_3OrLater);

		  if(cipher.isNull())
		    cipher = QSslCipher(next, QSsl::TlsV1_3);
		}
	      else
#endif
	      if(protocol == "TlsV1_2")
		cipher = QSslCipher(next, QSsl::TlsV1_2);
	      else if(protocol == "TlsV1_1")
		cipher = QSslCipher(next, QSsl::TlsV1_1);
	      else if(protocol == "TlsV1_0")
		cipher = QSslCipher(next, QSsl::TlsV1_0);

	      if(cipher.isNull())
		cipher = QSslCipher(next, QSsl::UnknownProtocol);

	      if(!cipher.isNull())
		list.append(cipher);
	    }

	  index += 1;
	}
      while(next);

    done_label:
      SSL_CTX_free(ctx);
      SSL_free(ssl);
      ctx = nullptr;
      ssl = nullptr;
    }

  if(list.isEmpty())
    log("spot_on_lite_daemon_child::default_ssl_ciphers(): "
	"empty cipher list.");
#endif

  return list;
}

bool spot_on_lite_daemon_child::memcmp(const QByteArray &a, const QByteArray &b)
{
  auto length = qMax(a.length(), b.length());
  quint64 rc = 0;

  for(int i = 0; i < length; i++)
    rc |= (i < a.length() ? static_cast<quint64> (a.at(i)) : 0ULL) ^
      (i < b.length() ? static_cast<quint64> (b.at(i)) : 0ULL);

  return rc == 0;
}

bool spot_on_lite_daemon_child::record_congestion
(const QByteArray &data) const
{
  auto added = false;
  auto db_connection_id = db_id();

  {
    auto db = QSqlDatabase::addDatabase
      ("QSQLITE", QString::number(db_connection_id));

    db.setDatabaseName(m_congestion_control_file_name);

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("CREATE TABLE IF NOT EXISTS congestion_control ("
		   "date_time_inserted BIGINT NOT NULL, "
		   "hash TEXT NOT NULL PRIMARY KEY)");
	query.exec("PRAGMA journal_mode = OFF");
	query.exec("PRAGMA synchronous = OFF");
	query.prepare("INSERT INTO congestion_control "
		      "(date_time_inserted, hash) "
		      "VALUES (?, ?)");
	query.addBindValue
	  (QDateTime::currentDateTime().currentMSecsSinceEpoch());
#if QT_VERSION >= 0x050100
	query.addBindValue
	  (QCryptographicHash::hash(data, QCryptographicHash::Sha3_384).
	   toBase64());
#else
	query.addBindValue
	  (QCryptographicHash::hash(data, QCryptographicHash::Sha384).
	   toBase64());
#endif
	added = query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(QString::number(db_connection_id));
  return added;
}

int spot_on_lite_daemon_child::bytes_accumulated(void) const
{
  int size = 0;

  {
    QReadLocker lock(&m_local_content_mutex);

    size += m_local_content.length();
  }

  size += m_remote_content.length();
  return size;
}

int spot_on_lite_daemon_child::bytes_in_send_queue(void) const
{
  int count = 0;

#ifdef Q_OS_FREEBSD
  if(ioctl(static_cast<int> (m_remote_socket->socketDescriptor()),
	   FIONWRITE,
	   &count) == -1)
    count = 0;
#elif defined(Q_OS_LINUX)
  if(ioctl(static_cast<int> (m_remote_socket->socketDescriptor()),
	   SIOCOUTQ,
	   &count) == -1)
    count = 0;
#elif defined(Q_OS_MAC)
  auto length = static_cast<socklen_t> (sizeof(count));

  if(getsockopt(static_cast<int> (m_remote_socket->socketDescriptor()),
		SOL_SOCKET,
		SO_NWRITE,
		&count,
		&length) == -1)
    count = 0;
#elif defined(Q_OS_OPENBSD)
  if(ioctl(static_cast<int> (m_remote_socket->socketDescriptor()),
	   TIOCOUTQ,
	   &count) == -1)
    count = 0;
#endif

  return count;
}

quint64 spot_on_lite_daemon_child::db_id(void)
{
  return s_db_id.fetchAndAddOrdered(1);
}

void spot_on_lite_daemon_child::create_remote_identities_database(void) const
{
  auto db_connection_id = db_id();

  {
    auto db = QSqlDatabase::addDatabase
      ("QSQLITE", QString::number(db_connection_id));

    db.setDatabaseName(m_remote_identities_file_name);

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("CREATE TABLE IF NOT EXISTS remote_identities ("
		   "algorithm TEXT NOT NULL, "
		   "date_time_inserted BIGINT NOT NULL, "
		   "identity TEXT NOT NULL PRIMARY KEY, "
		   "pid BIGINT NOT NULL)");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(QString::number(db_connection_id));
}

void spot_on_lite_daemon_child::data_received
(const QByteArray &data,
 const QHostAddress &peer_address,
 const quint16 peer_port)
{
#ifdef SPOTON_LITE_DAEMON_DTLS_SUPPORTED
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
  if(m_dtls && m_protocol == QAbstractSocket::UdpSocket)
    {
      auto socket = qobject_cast<QUdpSocket *> (m_remote_socket);

      if(!socket)
	{
	  slot_disconnected();
	  return;
	}

      if(m_dtls->isConnectionEncrypted())
	{
	  process_read_data(m_dtls->decryptDatagram(socket, data));

	  if(m_dtls->dtlsError() == QDtlsError::RemoteClosedConnectionError)
	    slot_disconnected();
	}
      else
	{
	  QPair<QHostAddress, quint64> pair(peer_address, peer_port);

	  if(!m_verified_udp_clients.contains(pair))
	    {
	      if(m_dtls_client_verifier.
		 verifyClient(socket, data, peer_address, peer_port))
		m_verified_udp_clients[pair] = 0;
	      else if(m_dtls_client_verifier.dtlsError() !=
		      QDtlsError::NoError)
		{
		  slot_disconnected();
		  return;
		}
	      else
		/*
		** Not verified.
		*/

		return;
	    }

	  if(!m_dtls->doHandshake(socket, data))
	    {
	      log(QString("spot_on_lite_daemon_child::data_received(): "
			  "doHandshake() failure (%1).").
		  arg(m_dtls->dtlsErrorString()));

	      if(!(m_dtls->dtlsError() == QDtlsError::NoError ||
		   m_dtls->dtlsError() == QDtlsError::TlsNonFatalError))
		{
		  m_dtls->abortHandshake(socket);
		  slot_disconnected();
		}
	    }
	}
    }
  else if(m_protocol == QAbstractSocket::UdpSocket)
    process_read_data(data);
#else
  Q_UNUSED(peer_address);
  Q_UNUSED(peer_port);

  if(m_protocol == QAbstractSocket::UdpSocket)
    process_read_data(data);
#endif
#else
  Q_UNUSED(peer_address);
  Q_UNUSED(peer_port);

  if(m_protocol == QAbstractSocket::UdpSocket)
    process_read_data(data);
#endif
}

void spot_on_lite_daemon_child::generate_certificate
(void *key, QByteArray &certificate, const long int days, QString &error)
{
#ifdef SPOTON_LITE_DAEMON_OPENSSL_SUPPORTED
  BIO *memory = nullptr;
  BUF_MEM *bptr;
  EC_KEY *ecc = nullptr;
  EVP_PKEY *pk = nullptr;
  RSA *rsa = nullptr;
  X509 *x509 = nullptr;
  X509_NAME *name = nullptr;
  X509_NAME *subject = nullptr;
  X509_NAME_ENTRY *common_name_entry = nullptr;
  auto organization = reinterpret_cast<const unsigned char *>
    ("Spot-On-Lite Self-Signed Certificate");
  char *buffer = nullptr;
  int length = 0;
  int rc = 0;
  unsigned char *common_name = nullptr;

  if(!error.isEmpty())
    goto done_label;

  if(m_ssl_key_size < 1024)
    ecc = (EC_KEY *) key;
  else
    rsa = (RSA *) key;

  if(m_ssl_key_size < 1024 && !ecc)
    {
      error = "ecc container is zero";
      goto done_label;
    }

  if(m_ssl_key_size > 1024 && !rsa)
    {
      error = "rsa container is zero";
      goto done_label;
    }

  if(!(pk = EVP_PKEY_new()))
    {
      error = "EVP_PKEY_new() failure";
      goto done_label;
    }

  if(!(x509 = X509_new()))
    {
      error = "X509_new() failure";
      goto done_label;
    }

  if(ecc)
    if(EVP_PKEY_assign_EC_KEY(pk, ecc) == 0)
      {
	error = "EVP_PKEY_assign_EC_KEY() returned zero";
	goto done_label;
      }

  if(rsa)
    if(EVP_PKEY_assign_RSA(pk, rsa) == 0)
      {
	error = "EVP_PKEY_assign_RSA() returned zero";
	goto done_label;
      }

  /*
  ** Set some attributes.
  */

  if(X509_set_version(x509, s_certificate_version) == 0)
    {
      error = "X509_set_version() returned zero";
      goto done_label;
    }

  if(X509_gmtime_adj(X509_get_notBefore(x509), 0) == nullptr)
    {
      error = "X509_gmtime_adj() returned zero";
      goto done_label;
    }

  if(X509_gmtime_adj(X509_get_notAfter(x509), days) == nullptr)
    {
      error = "X509_gmtime_adj() returned zero";
      goto done_label;
    }

  if(std::numeric_limits<int>::max() -
     m_remote_socket->localAddress().toString().toLatin1().length() < 1)
    common_name = nullptr;
  else
    common_name = static_cast<unsigned char *>
      (calloc(static_cast<size_t> (m_remote_socket->localAddress().
				   toString().toLatin1().length() + 1),
	      sizeof(unsigned char)));

  if(!common_name)
    {
      error = "calloc() returned zero or invalid local address";
      goto done_label;
    }

  length = m_remote_socket->localAddress().toString().toLatin1().length();
  memcpy(common_name,
	 m_remote_socket->localAddress().toString().toLatin1().constData(),
	 static_cast<size_t> (length));
  common_name_entry = X509_NAME_ENTRY_create_by_NID
    (nullptr, NID_commonName, V_ASN1_PRINTABLESTRING, common_name, length);

  if(!common_name_entry)
    {
      error = "X509_NAME_ENTRY_create_by_NID() returned zero";
      goto done_label;
    }

  subject = X509_NAME_new();

  if(!subject)
    {
      error = "X509_NAME_new() returned zero";
      goto done_label;
    }

  if(X509_NAME_add_entry_by_txt(subject,
				"O",
				MBSTRING_ASC,
				organization,
				-1,
				-1,
				0) != 1)
    {
      error = "X509_NAME_add_entry_by_txt() failure";
      goto done_label;
    }

  if(X509_NAME_add_entry(subject, common_name_entry, -1, 0) != 1)
    {
      error = "X509_NAME_add_entry() failure";
      goto done_label;
    }

  if(X509_set_subject_name(x509, subject) != 1)
    {
      error = "X509_set_subject_name() failed";
      goto done_label;
    }

  if((name = X509_get_subject_name(x509)) == nullptr)
    {
      error = "X509_get_subject_name() returned zero";
      goto done_label;
    }

  if(X509_set_issuer_name(x509, name) == 0)
    {
      error = "X509_set_issuer_name() returned zero";
      goto done_label;
    }

  if(X509_set_pubkey(x509, pk) == 0)
    {
      error = "X509_set_pubkey() returned zero";
      goto done_label;
    }

  if(m_ssl_key_size < 1024)
    rc = X509_sign(x509, pk, EVP_sha512());
  else
#if OPENSSL_VERSION_NUMBER < 0x10101000L || defined(Q_OS_OPENBSD)
    rc = X509_sign(x509, pk, EVP_sha512());
#else
    rc = X509_sign(x509, pk, EVP_sha3_512());
#endif

  if(rc == 0)
    {
      error = "X509_sign() returned zero";
      goto done_label;
    }

  /*
  ** Write the certificate to memory.
  */

  if(!(memory = BIO_new(BIO_s_mem())))
    {
      error = "BIO_new() returned zero";
      goto done_label;
    }

  if(PEM_write_bio_X509(memory, x509) == 0)
    {
      error = "PEM_write_bio_X509() returned zero";
      goto done_label;
    }

  BIO_get_mem_ptr(memory, &bptr);

  if(bptr->length + 1 <= 0 ||
     std::numeric_limits<size_t>::max() - bptr->length < 1 ||
     !(buffer = static_cast<char *> (calloc(bptr->length + 1,
					    sizeof(char)))))
    {
      error = "calloc() failure or bptr->length + 1 is irregular";
      goto done_label;
    }

  memcpy(buffer, bptr->data, bptr->length);
  buffer[bptr->length] = 0;
  certificate = buffer;

 done_label:
  BIO_free(memory);

  if(!error.isEmpty())
    memzero(certificate);

  if(ecc)
    EC_KEY_up_ref(ecc); // Reference counter.

  if(rsa)
    RSA_up_ref(rsa); // Reference counter.

  EVP_PKEY_free(pk);
  X509_NAME_ENTRY_free(common_name_entry);
  X509_NAME_free(subject);
  X509_free(x509);
  free(buffer);
  free(common_name);
#else
  Q_UNUSED(certificate);
  Q_UNUSED(days);
  Q_UNUSED(error);
  Q_UNUSED(key);
#endif
}

void spot_on_lite_daemon_child::generate_ssl_tls(void)
{
#ifdef SPOTON_LITE_DAEMON_OPENSSL_SUPPORTED
  BIGNUM *f4 = nullptr;
  BIO *private_memory = nullptr;
  BIO *public_memory = nullptr;
  BUF_MEM *bptr;
  EC_KEY *ecc = nullptr;
  EVP_PKEY *pk = nullptr;
  QByteArray certificate;
  QByteArray private_key;
  QByteArray public_key;
  QString error("");
  RSA *rsa = nullptr;
  auto days = 24L * 60L * 60L * static_cast<long int> (m_certificate_lifetime);
  char *private_buffer = nullptr;
  char *public_buffer = nullptr;
  int ecc_group = 0;

  if(m_ssl_key_size <= 0)
    {
      error = "m_ssl_key_size is less than or equal to zero";
      goto done_label;
    }
  else if(m_ssl_key_size > 7680)
    {
      error = "m_ssl_key_size is greater than 7680";
      goto done_label;
    }

  if(m_ssl_key_size < 1024)
    goto ecc_label;
  else
    goto rsa_label;

 ecc_label:

  switch(m_ssl_key_size)
    {
    case 256:
      {
	ecc_group = OBJ_txt2nid("prime256v1");
	break;
      }
    case 384:
      {
	ecc_group = OBJ_txt2nid("secp384r1");
	break;
      }
    default:
      {
	ecc_group = OBJ_txt2nid("secp521r1");
	break;
      }
    }

  if(ecc_group == NID_undef)
    {
      error = "OBJ_txt2nid() failure";
      goto done_label;
    }

  if(!(ecc = EC_KEY_new_by_curve_name(ecc_group)))
    {
      error = "EC_KEY_new_by_curve_name() returned zero";
      goto done_label;
    }

  if(!(private_memory = BIO_new(BIO_s_mem())))
    {
      error = "BIO_new() returned zero";
      goto done_label;
    }

  if(!(public_memory = BIO_new(BIO_s_mem())))
    {
      error = "BIO_new() returned zero";
      goto done_label;
    }

  EC_KEY_set_asn1_flag(ecc, OPENSSL_EC_NAMED_CURVE);

  if(EC_KEY_generate_key(ecc) == 0)
    {
      error = "EC_KEY_generate_key() failure";
      goto done_label;
    }

  if(!(pk = EVP_PKEY_new()))
    {
      error = "EVP_PKEY_new() returned zero";
      goto done_label;
    }

  if(EVP_PKEY_assign_EC_KEY(pk, ecc) == 0)
    {
      error = "EVP_PKEY_assign_EC_KEY() failure";
      goto done_label;
    }

  if(!(ecc = EVP_PKEY_get1_EC_KEY(pk)))
    {
      error = "EVP_PKEY_get1_EC_KEY() failure";
      goto done_label;
    }

  if(!PEM_write_bio_PrivateKey(private_memory,
			       pk,
			       nullptr,
			       nullptr,
			       0,
			       nullptr,
			       nullptr))
    {
      error = "PEM_write_bio_PrivateKey() failure";
      goto done_label;
    }

  if(!PEM_write_bio_PUBKEY(public_memory, pk))
    {
      error = "PEM_write_bio_PUBKEY() failure";
      goto done_label;
    }

  BIO_get_mem_ptr(private_memory, &bptr);

  if(bptr->length + 1 <= 0 ||
     std::numeric_limits<size_t>::max() - bptr->length < 1 ||
     !(private_buffer = static_cast<char *> (calloc(bptr->length + 1,
						    sizeof(char)))))
    {
      error = "calloc() failure or bptr->length + 1 is irregular";
      goto done_label;
    }

  memcpy(private_buffer, bptr->data, bptr->length);
  private_buffer[bptr->length] = 0;
  private_key = private_buffer;
  BIO_get_mem_ptr(public_memory, &bptr);

  if(bptr->length + 1 <= 0 ||
     std::numeric_limits<size_t>::max() - bptr->length < 1 ||
     !(public_buffer = static_cast<char *> (calloc(bptr->length + 1,
						   sizeof(char)))))
    {
      error = "calloc() failure or bptr->length + 1 is irregular";
      goto done_label;
    }

  memcpy(public_buffer, bptr->data, bptr->length);
  public_buffer[bptr->length] = 0;
  public_key = public_buffer;

  if(error.isEmpty())
    generate_certificate(ecc, certificate, days, error);

  goto done_label;

 rsa_label:

  if(!(f4 = BN_new()))
    {
      error = "BN_new() returned zero";
      goto done_label;
    }

  if(BN_set_word(f4, RSA_F4) != 1)
    {
      error = "BN_set_word() failure";
      goto done_label;
    }

  if(!(rsa = RSA_new()))
    {
      error = "RSA_new() returned zero";
      goto done_label;
    }

  if(RSA_generate_key_ex(rsa, m_ssl_key_size, f4, nullptr) == -1)
    {
      error = "RSA_generate_key_ex() returned negative one";
      goto done_label;
    }

  if(!(private_memory = BIO_new(BIO_s_mem())))
    {
      error = "BIO_new() returned zero";
      goto done_label;
    }

  if(!(public_memory = BIO_new(BIO_s_mem())))
    {
      error = "BIO_new() returned zero";
      goto done_label;
    }

  if(PEM_write_bio_RSAPrivateKey(private_memory,
				 rsa,
				 nullptr,
				 nullptr,
				 0,
				 nullptr,
				 nullptr) == 0)
    {
      error = "PEM_write_bio_RSAPrivateKey() returned zero";
      goto done_label;
    }

  if(PEM_write_bio_RSAPublicKey(public_memory, rsa) == 0)
    {
      error = "PEM_write_bio_RSAPublicKey() returned zero";
      goto done_label;
    }

  BIO_get_mem_ptr(private_memory, &bptr);

  if(bptr->length + 1 <= 0 ||
     std::numeric_limits<size_t>::max() - bptr->length < 1 ||
     !(private_buffer = static_cast<char *> (calloc(bptr->length + 1,
						   sizeof(char)))))
    {
      error = "calloc() failure or bptr->length + 1 is irregular";
      goto done_label;
    }

  memcpy(private_buffer, bptr->data, bptr->length);
  private_buffer[bptr->length] = 0;
  private_key = private_buffer;
  BIO_get_mem_ptr(public_memory, &bptr);

  if(bptr->length + 1 <= 0 ||
     std::numeric_limits<size_t>::max() - bptr->length < 1 ||
     !(public_buffer = static_cast<char *> (calloc(bptr->length + 1,
						  sizeof(char)))))
    {
      error = "calloc() failure or bptr->length + 1 is irregular";
      goto done_label;
    }

  memcpy(public_buffer, bptr->data, bptr->length);
  public_buffer[bptr->length] = 0;
  public_key = public_buffer;
  generate_certificate(rsa, certificate, days, error);

 done_label:

  if(!error.isEmpty())
    {
      memzero(certificate);
      memzero(private_key);
      memzero(public_key);
      log(QString("spot_on_lite_daemon_child::"
		  "generate_ssl_tls(): error (%1) occurred.").arg(error));
    }
  else
    {
      prepare_ssl_tls_configuration
	(QList<QByteArray> () << certificate << private_key);
      record_certificate(certificate, private_key, public_key);
    }

  BIO_free(private_memory);
  BIO_free(public_memory);
  BN_free(f4);
  EC_KEY_free(ecc);
  EVP_PKEY_free(pk);
  RSA_free(rsa);
  free(private_buffer);
  free(public_buffer);
#endif
}

void spot_on_lite_daemon_child::log(const QString &error) const
{
  auto e(error.trimmed());

  if(e.isEmpty())
    return;
  else
    qDebug() << e;

  if(m_log_file_name.isEmpty())
    return;

  QFile file(m_log_file_name);

  if(file.open(QIODevice::Append | QIODevice::WriteOnly))
    {
      auto dateTime(QDateTime::currentDateTime());

      file.write(dateTime.toString().toStdString().data());
      file.write("\n");
      file.write(e.toStdString().data());
      file.write("\n");
      file.close();
    }
}

void spot_on_lite_daemon_child::memzero(QByteArray &bytes) const
{
  memset(bytes.data(), 0, static_cast<size_t> (bytes.length()));
  bytes.clear();
}

#ifdef SPOTON_LITE_DAEMON_DTLS_SUPPORTED
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
void spot_on_lite_daemon_child::prepare_dtls(void)
{
  if(m_dtls)
    m_dtls->deleteLater();

  if(m_protocol != QAbstractSocket::UdpSocket || m_ssl_key_size == 0)
    return;

  if(m_client_role)
    m_dtls = new QDtls(QSslSocket::SslClientMode, this);
  else
    m_dtls = new QDtls(QSslSocket::SslServerMode, this);

  connect(m_dtls,
	  SIGNAL(handshakeTimeout(void)),
	  this,
	  SLOT(slot_handshake_timeout(void)));
  m_dtls->setDtlsConfiguration(m_ssl_configuration);
  m_dtls->setPeer(m_peer_address, m_peer_port);
}
#endif
#endif

void spot_on_lite_daemon_child::prepare_local_socket(void)
{
  {
    QWriteLocker lock(&m_local_content_mutex);

    m_local_content.clear();
    m_local_content.squeeze();
    m_local_content_last_parsed = QDateTime::currentMSecsSinceEpoch();
  }

  m_local_socket.abort();
  m_local_socket.connectToServer(m_local_server_file_name);
  save_statistic("bytes_accumulated", QString::number(bytes_accumulated()));
}

void spot_on_lite_daemon_child::prepare_ssl_tls_configuration
(const QList<QByteArray> &list)
{
  m_ssl_configuration.setLocalCertificate(QSslCertificate(list.value(0)));

  if(!m_ssl_configuration.localCertificate().isNull())
    {
      QSslKey key(list.value(1), QSsl::Ec);

      if(key.isNull())
	key = QSslKey(list.value(1), QSsl::Rsa);

      m_ssl_configuration.setPrivateKey(key);

      if(!m_ssl_configuration.privateKey().isNull())
	{
	  m_ssl_configuration.setPeerVerifyMode(QSslSocket::VerifyNone);
	  m_ssl_configuration.setSslOption
	    (QSsl::SslOptionDisableCompression, true);
	  m_ssl_configuration.setSslOption
	    (QSsl::SslOptionDisableEmptyFragments, true);
	  m_ssl_configuration.setSslOption
	    (QSsl::SslOptionDisableLegacyRenegotiation, true);
	  m_ssl_configuration.setSslOption
	    (QSsl::SslOptionDisableSessionTickets, true);
#if QT_VERSION >= 0x050200
	  m_ssl_configuration.setSslOption
	     (QSsl::SslOptionDisableSessionPersistence, true);
	  m_ssl_configuration.setSslOption
	     (QSsl::SslOptionDisableSessionSharing, true);
#endif
	  set_ssl_ciphers
	    (m_ssl_configuration.supportedCiphers(), m_ssl_configuration);

	  if(m_protocol == QAbstractSocket::TcpSocket)
	    qobject_cast<QSslSocket *>
	      (m_remote_socket)->setSslConfiguration(m_ssl_configuration);

#ifdef SPOTON_LITE_DAEMON_DTLS_SUPPORTED
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
	  if(m_protocol == QAbstractSocket::UdpSocket)
	    {
	      if(m_client_role)
		{
		  m_ssl_configuration.setPeerVerifyMode
		    (QSslSocket::VerifyNone);
		  m_ssl_configuration.setProtocol(QSsl::DtlsV1_2OrLater);
		}
	      else
		{
		  m_ssl_configuration.setDtlsCookieVerificationEnabled(false);
		  m_ssl_configuration.setPeerVerifyMode
		    (QSslSocket::VerifyNone);
		  m_ssl_configuration.setProtocol(QSsl::DtlsV1_2OrLater);
		}
	    }
#endif
#endif
	}
      else
	/*
	** Error!
	*/

	log("spot_on_lite_daemon_child::"
	    "prepare_ssl_tls_configuration(): empty private key.");
    }
}

void spot_on_lite_daemon_child::process_configuration_file(void)
{
  QSettings settings(m_configuration_file_name, QSettings::IniFormat);

  foreach(const auto &key, settings.allKeys())
    if(key == "type_capabilities" ||
       key == "type_identity" ||
       key == "type_spot_on_lite_client")
      m_message_types[key] = settings.value(key).toByteArray();
}

void spot_on_lite_daemon_child::process_local_content(void)
{
  {
    QReadLocker lock(&m_local_content_mutex);

    if(m_local_content.isEmpty())
      return;
  }

  {
    QWriteLocker lock(&m_local_content_mutex);

    if(m_end_of_message_marker.isEmpty())
      {
	emit write_signal(m_local_content);
	m_local_content.clear();
	m_local_content.squeeze();
	m_local_content_last_parsed = QDateTime::currentMSecsSinceEpoch();
	lock.unlock();
	save_statistic
	  ("bytes_accumulated", QString::number(bytes_accumulated()));
	return;
      }
  }

  QHash<QByteArray, QString> identities;

  do
    {
      auto ok = true;

      identities = remote_identities(&ok);

      if(!ok)
	{
	  struct timespec ts = {};

	  ts.tv_nsec = 250000000; // 250 Milliseconds
	  ts.tv_sec = 0;
	  nanosleep(&ts, nullptr);
	}
      else
	break;
    }
  while(!m_process_local_content_future.isCanceled());

  QVector<QByteArray> vector;
  auto type_identity(m_message_types.value("type_identity"));
  int index = 0;

  {
    QWriteLocker lock(&m_local_content_mutex);

    while((index = m_local_content.indexOf(m_end_of_message_marker)) >= 0)
      {
	m_local_content_last_parsed = QDateTime::currentMSecsSinceEpoch();

	if(m_process_local_content_future.isCanceled())
	  goto done_label;

	auto bytes
	  (m_local_content.mid(0, index + m_end_of_message_marker.length()));

	vector.append(bytes);
	m_local_content.remove(0, bytes.length());
	m_local_content.squeeze();
      }
  }

  save_statistic("bytes_accumulated", QString::number(bytes_accumulated()));

  if(m_process_local_content_future.isCanceled() || vector.isEmpty())
    goto done_label;

  for(const auto &bytes : vector)
    {
      if(bytes.contains("type=" + type_identity + "&content="))
	{
	  if(m_spot_on_lite)
	    /*
	    ** An identity was shared by some other local Spot-On-Lite
	    ** process. If this process represents a Spot-On-Lite client,
	    ** share the identity with it.
	    */

	    emit write_signal(bytes);

	  continue;
	}

      if(identities.isEmpty() || m_spot_on_lite)
	{
	  /*
	  ** Identities have not been recorded or this is a Spot-On-Lite
	  ** process.
	  */

	  emit write_signal(bytes);
	  continue;
	}

      if((index = bytes.indexOf("content=")) >= 0)
	{
	  QByteArray hash;
	  auto data(bytes.mid(8 + index).trimmed());

	  if(data.contains("\n")) // Spot-On
	    {
	      auto list(data.split('\n'));

	      if(list.size() >= 3)
		{
		  data = QByteArray::fromBase64(list.at(0)) +
		         QByteArray::fromBase64(list.at(1));
		  hash = QByteArray::fromBase64(list.at(2)); // Destination.
		}
	      else
		continue;
	    }
	  else
	    {
	      data = QByteArray::fromBase64(data);
	      hash = data.mid(data.length() - 64);
	      data = data.mid(0, data.length() - hash.length());
	    }

	  QHashIterator<QByteArray, QString> it(identities);

	  while(it.hasNext() && !m_process_local_content_future.isCanceled())
	    {
	      it.next();

	      if(memcmp(hash, m_sha_512.sha_512_hmac(data, it.key())))
		{
		  /*
		  ** Found!
		  */

		  emit write_signal(bytes);
		  break;
		}
	    }
	}
      else
	emit write_signal(bytes);
    }

 done_label:

  if(m_process_local_content_future.isCanceled())
    {
      {
	QWriteLocker lock(&m_local_content_mutex);

	m_local_content.clear();
	m_local_content.squeeze();
	m_local_content_last_parsed = QDateTime::currentMSecsSinceEpoch();
      }

      save_statistic
	("bytes_accumulated", QString::number(bytes_accumulated()));
    }
  else
    {
      QReadLocker lock(&m_local_content_mutex);

      if(!m_local_content.isEmpty())
	emit read_signal();
    }
}

void spot_on_lite_daemon_child::process_read_data(const QByteArray &data)
{
  /*
  ** Process data received from the remote socket.
  */

  if(data.isEmpty())
    return;

  if(m_client_role || m_end_of_message_marker.isEmpty())
    {
      if(m_silence > 0)
	m_keep_alive_timer.start();

      if(m_local_socket.state() == QLocalSocket::ConnectedState &&
	 record_congestion(data))
	{
	  auto maximum = m_local_so_rcvbuf_so_sndbuf -
	    static_cast<int> (m_local_socket.bytesToWrite());

	  if(maximum > 0)
	    {
	      auto rc = m_local_socket.write(data.mid(0, maximum));

	      if(rc > 0)
		{
		  m_bytes_written.fetchAndAddOrdered(static_cast<quint64> (rc));
		  save_statistic
		    ("bytes_written",
		     QString::number(m_bytes_written.fetchAndAddOrdered(0ULL)));
		}
	    }
	}

      if(m_end_of_message_marker.isEmpty())
	return;
    }

  if(m_silence > 0)
    m_keep_alive_timer.start();

  if(m_remote_content.length() >= m_maximum_accumulated_bytes)
    {
      m_remote_content.clear();
      m_remote_content.squeeze();
      m_remote_content_last_parsed = QDateTime::currentMSecsSinceEpoch();
    }

  m_remote_content.append
    (data.mid(0,
	      qAbs(m_maximum_accumulated_bytes - m_remote_content.length())));
  process_remote_content();
  save_statistic("bytes_accumulated", QString::number(bytes_accumulated()));
}

void spot_on_lite_daemon_child::process_remote_content(void)
{
  if(m_end_of_message_marker.isEmpty() ||
     m_local_socket.state() != QLocalSocket::ConnectedState)
    return;

  QByteArray data;
  auto type_capabilities
    (m_message_types.value("type_capabilities"));
  auto type_identity(m_message_types.value("type_identity"));
  auto type_spot_on_lite_client
    (m_message_types.value("type_spot_on_lite_client"));
  int index = 0;

  while((index = m_remote_content.indexOf(m_end_of_message_marker)) >= 0)
    {
      data = m_remote_content.mid(0, index + m_end_of_message_marker.length());
      m_remote_content.remove(0, data.length());
      m_remote_content.squeeze();
      m_remote_content_last_parsed = QDateTime::currentMSecsSinceEpoch();

      if(m_client_role)
	{
	  if(data.contains("type=" + type_identity + "&content="))
	    record_remote_identity(data);

	  continue;
	}

      if(data.contains("type=" + type_capabilities + "&content="))
	continue;
      else if(data.contains("type=" + type_identity + "&content="))
	{
	  record_remote_identity(data);
	  continue;
	}
      else if(data.contains("type=" + type_spot_on_lite_client + "&content="))
	{
	  m_spot_on_lite = true;
	  continue;
	}

      if(record_congestion(data))
	{
	  auto maximum = m_local_so_rcvbuf_so_sndbuf -
	    static_cast<int> (m_local_socket.bytesToWrite());

	  if(maximum > 0)
	    {
	      auto rc = m_local_socket.write(data.mid(0, maximum));

	      if(rc > 0)
		m_bytes_written.fetchAndAddOrdered(static_cast<quint64> (rc));
	    }
	}
    }

  save_statistic("bytes_accumulated", QString::number(bytes_accumulated()));
  save_statistic("bytes_written",
		 QString::number(m_bytes_written.fetchAndAddOrdered(0ULL)));
}

void spot_on_lite_daemon_child::purge_containers(void)
{
  {
    QWriteLocker lock(&m_local_content_mutex);

    m_local_content.clear();
    m_local_content.squeeze();
    m_local_content_last_parsed = QDateTime::currentMSecsSinceEpoch();
  }

  m_remote_content.clear();
  m_remote_content.squeeze();
  m_remote_content_last_parsed = QDateTime::currentMSecsSinceEpoch();
  purge_remote_identities();
  save_statistic("bytes_accumulated", QString::number(bytes_accumulated()));
}

void spot_on_lite_daemon_child::purge_remote_identities(void)
{
#ifdef SPOTON_LITE_DAEMON_ENABLE_IDENTITIES_CONTAINER
  QWriteLocker lock(&m_remote_identities_mutex);

  m_remote_identities.clear();
#else
  auto db_connection_id = db_id();

  {
    auto db = QSqlDatabase::addDatabase
      ("QSQLITE", QString::number(db_connection_id));

    db.setDatabaseName(m_remote_identities_file_name);

    if(db.open())
      {
	QSqlQuery query(db);

	query.prepare("DELETE FROM remote_identities WHERE pid = ?");
	query.addBindValue(m_pid);
	query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(QString::number(db_connection_id));
#endif
}

void spot_on_lite_daemon_child::purge_statistics(void)
{
  auto db_connection_id = db_id();

  {
    auto db = QSqlDatabase::addDatabase
      ("QSQLITE", QString::number(db_connection_id));

    db.setDatabaseName(m_statistics_file_name);

    if(db.open())
      {
	QSqlQuery query(db);

	query.prepare("DELETE FROM statistics WHERE pid = ?");
	query.addBindValue(m_pid);
	query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(QString::number(db_connection_id));
}

void spot_on_lite_daemon_child::record_certificate
(const QByteArray &certificate,
 const QByteArray &private_key,
 const QByteArray &public_key)
{
  if(m_client_role)
    return;

  auto db_connection_id = db_id();

  {
    auto db = QSqlDatabase::addDatabase
      ("QSQLITE", QString::number(db_connection_id));

    db.setDatabaseName(m_certificates_file_name);

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("CREATE TABLE IF NOT EXISTS certificates ("
		   "certificate BLOB NOT NULL, "
		   "private_key BLOB NOT NULL, "
		   "public_key BLOB NOT NULL, "
		   "server_identity TEXT NOT NULL PRIMARY KEY)");
	query.prepare
	  ("INSERT INTO certificates "
	   "(certificate, private_key, public_key, server_identity) "
	   "VALUES (?, ?, ?, ?)");
	query.addBindValue(certificate.toBase64());
	query.addBindValue(private_key.toBase64());
	query.addBindValue(public_key.toBase64());
	query.addBindValue(m_server_identity);
	query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(QString::number(db_connection_id));
}

void spot_on_lite_daemon_child::record_remote_identity
(const QByteArray &data)
{
  QByteArray algorithm;
  QByteArray identity;
  auto index = data.indexOf("content=");

  if(index >= 0)
    identity = data.mid(8 + index).trimmed();
  else
    identity = data.trimmed();

  if((index = identity.indexOf(";")) > 0)
    {
      algorithm = identity.mid(index + 1).toLower().trimmed();

      if(!(algorithm == "sha-512"))
	algorithm = "sha-512";

      identity = identity.mid(0, index);
    }
  else
    algorithm = "sha-512";

  identity = QByteArray::fromBase64(identity);

  if(hash_algorithm_key_length(algorithm) == identity.length())
    {
#ifdef SPOTON_LITE_DAEMON_ENABLE_IDENTITIES_CONTAINER
      QWriteLocker lock(&m_remote_identities_mutex);

      if(!m_remote_identities.contains(identity))
	{
	  if(MAXIMUM_REMOTE_IDENTITIES > m_remote_identities.size())
	    m_remote_identities[identity] = QDateTime::currentDateTime();
	}
      else
	m_remote_identities[identity] = QDateTime::currentDateTime();
#else
      create_remote_identities_database();

      auto db_connection_id = db_id();

      {
	auto db = QSqlDatabase::addDatabase
	  ("QSQLITE", QString::number(db_connection_id));

	db.setDatabaseName(m_remote_identities_file_name);

	if(db.open())
	  {
	    QSqlQuery query(db);

	    query.prepare("INSERT OR REPLACE INTO remote_identities "
			  "(algorithm, date_time_inserted, identity, pid) "
			  "VALUES (?, ?, ?, ?)");
	    query.addBindValue(algorithm);
	    query.addBindValue
	      (QDateTime::currentDateTime().currentMSecsSinceEpoch());
	    query.addBindValue(identity.toBase64());
	    query.addBindValue(m_pid);
	    query.exec();
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase(QString::number(db_connection_id));
#endif
      share_identity(data + m_end_of_message_marker);
    }
}

void spot_on_lite_daemon_child::remove_expired_identities(void)
{
#ifdef SPOTON_LITE_DAEMON_ENABLE_IDENTITIES_CONTAINER
  QWriteLocker lock(&m_remote_identities_mutex);
  QMutableHashIterator<QByteArray, QDateTime> it(m_remote_identities);

  while(it.hasNext())
    {
      auto now = QDateTime::currentDateTime().currentMSecsSinceEpoch();

      it.next();

      if(now > it.value().currentMSecsSinceEpoch() &&
	 now - it.value().currentMSecsSinceEpoch() > m_identity_lifetime)
	it.remove();
    }
#else
  auto db_connection_id = db_id();

  {
    auto db = QSqlDatabase::addDatabase
      ("QSQLITE", QString::number(db_connection_id));

    db.setDatabaseName(m_remote_identities_file_name);

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec
	  (QString("DELETE FROM remote_identities WHERE "
		   "%1 - date_time_inserted > %2").
	   arg(QDateTime::currentDateTime().currentMSecsSinceEpoch()).
	   arg(m_identity_lifetime));
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(QString::number(db_connection_id));
#endif
}

void spot_on_lite_daemon_child::save_statistic
(const QString &key, const QString &value)
{
  spot_on_lite_common::save_statistic
    (key, m_statistics_file_name, value, m_pid, db_id());
}

void spot_on_lite_daemon_child::
set_ssl_ciphers(const QList<QSslCipher> &ciphers,
		QSslConfiguration &configuration) const
{
  auto preferred(default_ssl_ciphers());

  for(auto i = preferred.size() - 1; i >= 0; i--)
    if(!ciphers.contains(preferred.at(i)))
      preferred.removeAt(i);

  if(preferred.isEmpty())
    configuration.setCiphers(ciphers);
  else
    configuration.setCiphers(preferred);
}

void spot_on_lite_daemon_child::share_identity(const QByteArray &data)
{
  auto rc = m_local_socket.write(data);

  if(rc > 0)
    {
      m_bytes_written.fetchAndAddOrdered(static_cast<quint64> (rc));
      save_statistic
	("bytes_written",
	 QString::number(m_bytes_written.fetchAndAddOrdered(0ULL)));
    }
}

void spot_on_lite_daemon_child::slot_attempt_local_connection(void)
{
  prepare_local_socket();
}

void spot_on_lite_daemon_child::slot_attempt_remote_connection(void)
{
  /*
  ** Attempt a client connection.
  */

  if(m_remote_socket->state() != QAbstractSocket::UnconnectedState)
    return;

  if(!m_ssl_control_string.isEmpty() && m_ssl_key_size > 0)
    {
      if(m_protocol == QAbstractSocket::TcpSocket)
	qobject_cast<QSslSocket *> (m_remote_socket)->connectToHostEncrypted
	  (m_peer_address.toString(), m_peer_port);
      else
	{
	  m_remote_socket->connectToHost(m_peer_address, m_peer_port);

	  if(!m_remote_socket->waitForConnected(10000))
	    return;

#ifdef SPOTON_LITE_DAEMON_DTLS_SUPPORTED
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
	  prepare_dtls();

	  if(m_dtls)
	    /*
	    ** The client initiates DTLS.
	    */

	    if(!m_dtls->
	       doHandshake(qobject_cast<QUdpSocket *> (m_remote_socket)))
	      {
		log(QString("spot_on_lite_daemon_child::"
			    "slot_attempt_remote_connection(): "
			    "doHandshake() failure (%1).").
		    arg(m_dtls->dtlsErrorString()));

		if(!(m_dtls->dtlsError() == QDtlsError::NoError ||
		     m_dtls->dtlsError() == QDtlsError::TlsNonFatalError))
		  {
		    m_dtls->abortHandshake
		      (qobject_cast<QUdpSocket *> (m_remote_socket));
		    slot_disconnected();
		  }
	      }
#endif
#endif
	}
    }
  else
    m_remote_socket->connectToHost(m_peer_address, m_peer_port);
}

void spot_on_lite_daemon_child::slot_broadcast_capabilities(void)
{
  /*
  ** Capabilities
  */

  QByteArray data;
  QByteArray results;
  auto type_capabilities(m_message_types.value("type_capabilities"));
  auto uuid(QUuid::createUuid());

  data.append(uuid.toString().toUtf8());
  data.append("\n");
  data.append(QByteArray::number(m_maximum_accumulated_bytes));
  data.append("\n");
  data.append("full");
  results.append("POST HTTP/1.1\r\n"
		 "Content-Length: %1\r\n"
		 "Content-Type: application/x-www-form-urlencoded\r\n"
		 "\r\n"
		 "type=" + type_capabilities + "&content=%2\r\n"
		 "\r\n\r\n");
  results.replace
    ("%1",
     QByteArray::number(data.toBase64().length() +
			QString("type=" +
				type_capabilities +
				"&content=\r\n\r\n\r\n").length()));
  results.replace("%2", data.toBase64());
  write(results);

  if(m_client_role)
    {
      /*
      ** We are Spot-On-Lite!
      */

      QByteArray data;
      QByteArray results;
      auto type_spot_on_lite_client
	(m_message_types.value("type_spot_on_lite_client"));

      data.append("Spot-On-Lite");
      results.append("POST HTTP/1.1\r\n"
		     "Content-Length: %1\r\n"
		     "Content-Type: application/x-www-form-urlencoded\r\n"
		     "\r\n"
		     "type=" + type_spot_on_lite_client + "&content=%2\r\n"
		     "\r\n\r\n");
      results.replace
	("%1",
	 QByteArray::
	 number(data.length() +
		QString("type=" +
			type_spot_on_lite_client +
			"&content=\r\n\r\n\r\n").length()));
      results.replace("%2", data);
      write(results);
    }
}

void spot_on_lite_daemon_child::slot_connected(void)
{
  m_attempt_local_connection_timer.start();
  m_attempt_remote_connection_timer.stop();
  m_capabilities_timer.start(qMax(7500, m_silence / 2));
  m_remote_content_last_parsed = QDateTime::currentMSecsSinceEpoch();
  m_remote_socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
  save_statistic("ip_information",
		 m_remote_socket->peerAddress().toString() +
		 ":" +
		 QString::number(m_remote_socket->peerPort()) +
		 ":" +
		 socket_type_to_string(m_remote_socket->socketType()));

  auto sd = static_cast<int> (m_remote_socket->socketDescriptor());

  if(m_so_linger > -1)
    {
      socklen_t length = 0;
      struct linger l = {};

      memset(&l, 0, sizeof(l));
      l.l_linger = m_so_linger;
      l.l_onoff = 1;
      length = static_cast<socklen_t> (sizeof(l));
      setsockopt(sd, SOL_SOCKET, SO_LINGER, &l, length);
    }

  auto optlen = static_cast<socklen_t> (sizeof(m_maximum_accumulated_bytes));

  setsockopt(sd, SOL_SOCKET, SO_RCVBUF, &m_maximum_accumulated_bytes, optlen);
  setsockopt(sd, SOL_SOCKET, SO_SNDBUF, &m_maximum_accumulated_bytes, optlen);
}

void spot_on_lite_daemon_child::slot_disconnected(void)
{
  if(m_client_role)
    {
      if(!m_attempt_remote_connection_timer.isActive())
	m_attempt_remote_connection_timer.start();

      m_capabilities_timer.stop();
      m_local_socket.abort();
      m_remote_socket->abort();
      purge_containers();
    }
  else
    {
      m_local_socket.abort();
      m_remote_socket->abort();
      purge_statistics();
      stop_threads_and_timers();

      if(m_protocol != QAbstractSocket::UdpSocket)
	QCoreApplication::exit(0);
      else
	deleteLater();
    }
}

void spot_on_lite_daemon_child::slot_error(QAbstractSocket::SocketError error)
{
  Q_UNUSED(error);
}

void spot_on_lite_daemon_child::slot_general_timer_timeout(void)
{
  {
    QWriteLocker lock(&m_local_content_mutex);

    if(QDateTime::currentMSecsSinceEpoch() - m_local_content_last_parsed >
       END_OF_MESSAGE_MARKER_WINDOW)
      {
	m_local_content.clear();
	m_local_content.squeeze();
	m_local_content_last_parsed = QDateTime::currentMSecsSinceEpoch();
      }
  }

  if(QDateTime::currentMSecsSinceEpoch() - m_remote_content_last_parsed >
     END_OF_MESSAGE_MARKER_WINDOW)
    {
      m_remote_content.clear();
      m_remote_content.squeeze();
      m_remote_content_last_parsed = QDateTime::currentMSecsSinceEpoch();
    }

  save_statistic("bytes_accumulated", QString::number(bytes_accumulated()));

  struct rusage rusage = {};

  if(getrusage(RUSAGE_SELF, &rusage) == 0)
    save_statistic("memory", QString::number(rusage.ru_maxrss));
  else
    save_statistic("memory", "0");
}

#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
void spot_on_lite_daemon_child::slot_handshake_timeout(void)
{
#ifdef SPOTON_LITE_DAEMON_DTLS_SUPPORTED
  if(m_dtls && m_protocol == QAbstractSocket::UdpSocket)
    m_dtls->handleTimeout(qobject_cast<QUdpSocket *> (m_remote_socket));
#endif
}
#else
void spot_on_lite_daemon_child::slot_handshake_timeout(void)
{
}
#endif

void spot_on_lite_daemon_child::slot_keep_alive_timer_timeout(void)
{
  if(m_remote_socket->peerAddress().isNull())
    log(QString("spot_on_lite_daemon_child::"
		"slot_keep_alive_timer_timeout(): peer %1:%2 "
		"aborting!").
	arg(m_peer_address.toString()).
	arg(m_peer_port));
  else
    log(QString("spot_on_lite_daemon_child::"
		"slot_keep_alive_timer_timeout(): peer %1:%2 "
		"aborting!").
	arg(m_remote_socket->peerAddress().toString()).
	arg(m_remote_socket->peerPort()));

  if(m_client_role)
    {
      m_local_socket.abort();
      m_remote_socket->abort();

      if(!m_attempt_remote_connection_timer.isActive())
	m_attempt_remote_connection_timer.start();

      purge_containers();
    }
  else
    {
      m_local_socket.abort();
      m_remote_socket->abort();
      purge_statistics();
      stop_threads_and_timers();

      if(m_protocol != QAbstractSocket::UdpSocket)
	QCoreApplication::exit(0);
      else
	deleteLater();
    }
}

void spot_on_lite_daemon_child::slot_local_socket_connected(void)
{
  {
    QWriteLocker lock(&m_local_content_mutex);

    m_local_content_last_parsed = QDateTime::currentMSecsSinceEpoch();
  }

  auto optlen = static_cast<socklen_t> (sizeof(m_local_so_rcvbuf_so_sndbuf));
  auto sd = static_cast<int> (m_local_socket.socketDescriptor());

  setsockopt(sd, SOL_SOCKET, SO_RCVBUF, &m_local_so_rcvbuf_so_sndbuf, optlen);
  setsockopt(sd, SOL_SOCKET, SO_SNDBUF, &m_local_so_rcvbuf_so_sndbuf, optlen);

  /*
  ** We may have remote content.
  */

  process_remote_content();
}

void spot_on_lite_daemon_child::slot_local_socket_disconnected(void)
{
  if(!m_attempt_local_connection_timer.isActive())
    m_attempt_local_connection_timer.start();
}

void spot_on_lite_daemon_child::slot_local_socket_ready_read(void)
{
  while(m_local_socket.bytesAvailable() > 0)
    {
      auto data(m_local_socket.readAll());

      if(!data.isEmpty())
	{
	  m_bytes_read.fetchAndAddOrdered(static_cast<quint64> (data.length()));

	  {
	    QWriteLocker lock(&m_local_content_mutex);

	    if(m_local_content.length() >= m_maximum_accumulated_bytes)
	      {
		m_local_content.clear();
		m_local_content.squeeze();
		m_local_content_last_parsed =
		  QDateTime::currentMSecsSinceEpoch();
	      }

	    m_local_content.append
	      (data.mid(0, qAbs(m_maximum_accumulated_bytes -
				m_local_content.length())));
	  }

	}
    }

  save_statistic("bytes_accumulated", QString::number(bytes_accumulated()));
  save_statistic
    ("bytes_read", QString::number(m_bytes_read.fetchAndAddOrdered(0ULL)));

  {
    QReadLocker lock(&m_local_content_mutex);

    if(m_local_content.isEmpty())
      return;
  }

  if(m_process_local_content_future.isFinished())
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    m_process_local_content_future = QtConcurrent::run
      (this, &spot_on_lite_daemon_child::process_local_content);
#else
    m_process_local_content_future = QtConcurrent::run
      (&spot_on_lite_daemon_child::process_local_content, this);
#endif
}

void spot_on_lite_daemon_child::slot_peer_verify_error(const QSslError &error)
{
  Q_UNUSED(error);
}

void spot_on_lite_daemon_child::slot_ready_read(void)
{
  while(m_remote_socket->bytesAvailable() > 0)
    {
      auto data(m_remote_socket->readAll());

      if(!data.isEmpty())
	{
	  m_bytes_read.fetchAndAddOrdered
	    (static_cast<quint64> (data.length()));
	  save_statistic
	    ("bytes_read",
	     QString::number(m_bytes_read.fetchAndAddOrdered(0ULL)));
	}

#ifdef SPOTON_LITE_DAEMON_DTLS_SUPPORTED
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
      if(!data.isEmpty() && m_dtls && m_protocol == QAbstractSocket::UdpSocket)
	{
	  auto socket = qobject_cast<QUdpSocket *> (m_remote_socket);

	  if(!socket)
	    {
	      slot_disconnected();
	      return;
	    }

	  if(m_dtls->isConnectionEncrypted())
	    {
	      data = m_dtls->decryptDatagram(socket, data);

	      if(m_dtls->dtlsError() ==
		 QDtlsError::RemoteClosedConnectionError)
		{
		  slot_disconnected();
		  return;
		}
	    }
	  else
	    {
	      if(!m_dtls->doHandshake(socket, data))
		{
		  log
		    (QString("spot_on_lite_daemon_child::"
			     "slot_ready_read(): doHandshake() failure (%1).").
		     arg(m_dtls->dtlsErrorString()));

		  if(!(m_dtls->dtlsError() == QDtlsError::NoError ||
		       m_dtls->dtlsError() == QDtlsError::TlsNonFatalError))
		    {
		      m_dtls->abortHandshake(socket);
		      slot_disconnected();
		      return;
		    }
		}

	      continue;
	    }
	}
#endif
#endif

      process_read_data(data);
    }
}

void spot_on_lite_daemon_child::slot_remove_expired_identities(void)
{
  if(m_expired_identities_future.isFinished())
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    m_expired_identities_future = QtConcurrent::run
      (this, &spot_on_lite_daemon_child::remove_expired_identities);
#else
    m_expired_identities_future = QtConcurrent::run
      (&spot_on_lite_daemon_child::remove_expired_identities, this);
#endif
}

void spot_on_lite_daemon_child::
slot_ssl_errors(const QList<QSslError> &errors)
{
  Q_UNUSED(errors);

  if(m_protocol == QAbstractSocket::TcpSocket)
    qobject_cast<QSslSocket *> (m_remote_socket)->ignoreSslErrors();
}

void spot_on_lite_daemon_child::slot_write_data(const QByteArray &data)
{
  write(data);
}

void spot_on_lite_daemon_child::stop_threads_and_timers(void)
{
  m_attempt_local_connection_timer.stop();
  m_attempt_remote_connection_timer.stop();
  m_capabilities_timer.stop();
  m_expired_identities_future.cancel();
  m_expired_identities_timer.stop();
  m_general_timer.stop();
  m_keep_alive_timer.stop();
  m_process_local_content_future.cancel();

  /*
  ** Wait for threads to complete.
  */

  m_expired_identities_future.waitForFinished();
  m_process_local_content_future.waitForFinished();
}

void spot_on_lite_daemon_child::write(const QByteArray &data)
{
  switch(m_protocol)
    {
    case QAbstractSocket::TcpSocket:
      {
	int i = 0;

	while(data.size() > i)
	  {
	    auto maximum = m_maximum_accumulated_bytes - bytes_in_send_queue();

	    if(maximum > 0)
	      {
		auto rc = static_cast<int>
		  (m_remote_socket->
		   write(data.mid(i, qMin(MAXIMUM_TCP_WRITE_SIZE, maximum))));

		if(rc > 0)
		  {
		    i += rc;
		    m_bytes_written.fetchAndAddOrdered
		      (static_cast<quint64> (rc));
		    m_remote_socket->flush();
		  }
		else
		  break;
	      }
	    else
	      break;
	  }

	break;
      }
    case QAbstractSocket::UdpSocket:
      {
	int i = 0;

	while(data.size() > i)
	  {
	    qint64 rc = 0;

#ifdef SPOTON_LITE_DAEMON_DTLS_SUPPORTED
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
	    if(m_dtls)
	      rc = m_dtls->writeDatagramEncrypted
		(qobject_cast<QUdpSocket *> (m_remote_socket),
		 data.mid(MAXIMUM_UDP_WRITE_SIZE, i));
	    else
#endif
#endif
	    if(m_client_role)
	      rc = m_remote_socket->write(data.mid(MAXIMUM_UDP_WRITE_SIZE, i));
	    else
	      rc = qobject_cast<QUdpSocket *> (m_remote_socket)->writeDatagram
		(data.mid(MAXIMUM_UDP_WRITE_SIZE, i),
		 m_peer_address,
		 m_peer_port);

	    if(rc > 0)
	      {
		i += static_cast<int> (rc);
		m_bytes_written.fetchAndAddOrdered(static_cast<quint64> (rc));
	      }
	    else
	      break;
	  }

	break;
      }
    default:
      {
	break;
      }
    }

  save_statistic("bytes_written",
		 QString::number(m_bytes_written.fetchAndAddOrdered(0ULL)));
}

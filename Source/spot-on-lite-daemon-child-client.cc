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
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
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
#if QT_VERSION >= 0x050000
#include <QtConcurrent>
#endif
#include <QtCore>

#include <limits>

#include "spot-on-lite-daemon-child-client.h"

QReadWriteLock spot_on_lite_daemon_child_client::s_db_id_mutex;
quint64 spot_on_lite_daemon_child_client::s_db_id = 0;

static int hash_algorithm_key_length(const QByteArray &algorithm)
{
  if(algorithm.toLower().trimmed() == "sha-512")
    return 64;
  else
    return 0;
}

#ifdef __arm__
static int ARM_MAXIMUM_REMOTE_IDENTITIES = 128;
#endif
static qint64 END_OF_MESSAGE_MARKER_WINDOW = 10000;

spot_on_lite_daemon_child_client::spot_on_lite_daemon_child_client
(const QByteArray &initial_data,
 const QString &certificates_file_name,
 const QString &congestion_control_file_name,
 const QString &end_of_message_marker,
 const QString &local_server_file_name,
 const QString &log_file_name,
 const QString &peer_address,
 const QString &peer_scope_identity,
 const QString &protocol,
 const QString &remote_identities_file_name,
 const QString &server_identity,
 const QString &ssl_control_string,
 const int identities_lifetime,
 const int local_so_sndbuf,
 const int maximum_accumulated_bytes,
 const int silence,
 const int socket_descriptor,
 const int ssl_key_size,
 const quint16 peer_port):QObject()
{
  m_attempt_local_connection_timer.setInterval(2500);
  m_attempt_remote_connection_timer.setInterval(2500);
  m_certificates_file_name = certificates_file_name;
  m_client_role = socket_descriptor < 0;
  m_congestion_control_file_name = congestion_control_file_name;
  m_end_of_message_marker = end_of_message_marker;
  m_general_timer.start(5000);
  m_identity_lifetime = qBound(5, identities_lifetime, 600);
  m_local_content_last_parsed = QDateTime::currentMSecsSinceEpoch();
  m_local_server_file_name = local_server_file_name;
  m_local_so_sndbuf = qBound(4096, local_so_sndbuf, 65536);
  m_local_socket = new QLocalSocket(this);
  m_local_socket->setReadBufferSize(m_maximum_accumulated_bytes);
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
  m_silence = 1000 * qBound(15, silence, 3600);
  m_ssl_control_string = ssl_control_string.trimmed();
  m_ssl_key_size = ssl_key_size;
  m_statistics_file_name = "/tmp/spot-on-lite-daemon-statistics.sqlite";

  if(!(m_ssl_key_size == 2048 ||
       m_ssl_key_size == 3072 ||
       m_ssl_key_size == 4096))
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
	  log("spot_on_lite_daemon_child_client::"
	      "spot_on_lite_daemon_child_client(): "
	      "invalid socket descriptor.");
	  QTimer::singleShot(2500, this, SLOT(slot_disconnected(void)));
	  return;
	}

      m_attempt_local_connection_timer.start();
      m_capabilities_timer.start(m_silence / 2);
      m_remote_socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    }

  m_expired_identities_timer.start(15000);
  m_keep_alive_timer.start(m_silence);
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
  connect(m_local_socket,
	  SIGNAL(connected(void)),
	  &m_attempt_local_connection_timer,
	  SLOT(stop(void)));
  connect(m_local_socket,
	  SIGNAL(connected(void)),
	  this,
	  SLOT(slot_local_socket_connected(void)));
  connect(m_local_socket,
	  SIGNAL(disconnected(void)),
	  this,
	  SLOT(slot_local_socket_disconnected(void)));
  connect(m_local_socket,
	  SIGNAL(readyRead(void)),
	  this,
	  SLOT(slot_local_socket_ready_read(void)));
  connect(m_remote_socket,
	  SIGNAL(disconnected(void)),
	  this,
	  SLOT(slot_disconnected(void)));
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
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(Q_OS_OPENBSD)
      SSL_library_init(); // Always returns 1.
#else
      OPENSSL_init_ssl(0, NULL);
#endif
      if(m_protocol == QAbstractSocket::TcpSocket)
	connect(qobject_cast<QSslSocket *> (m_remote_socket),
		SIGNAL(sslErrors(const QList<QSslError> &)),
		this,
		SLOT(slot_ssl_errors(const QList<QSslError> &)));

      if(m_client_role)
	{
	  generate_ssl_tls();

	  QStringList list(m_server_identity.split(":"));

	  m_peer_address = QHostAddress(list.value(0));
	  m_peer_port = static_cast<quint16> (list.value(1).toInt());

	  if(m_protocol == QAbstractSocket::TcpSocket)
	    qobject_cast<QSslSocket *> (m_remote_socket)->
	      connectToHostEncrypted(m_peer_address.toString(), m_peer_port);
	  else
	    {
	      m_remote_socket->connectToHost(m_peer_address, m_peer_port);
	      m_remote_socket->waitForConnected(10000);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
	      prepare_dtls();

	      if(m_dtls)
		if(!m_dtls->
		   doHandshake(qobject_cast<QUdpSocket *> (m_remote_socket)))
		  log(QString("spot_on_lite_daemon_child_client::"
			      "spot_on_lite_daemon_child_client(): "
			      "doHandshake() failure (%1).").
		      arg(m_dtls->dtlsErrorString()));
#endif
	    }
	}
      else
	{
	  QList<QByteArray> list(local_certificate_configuration());

	  if(list.isEmpty())
	    generate_ssl_tls();
	  else
	    prepare_ssl_tls_configuration(list);

	  if(m_protocol == QAbstractSocket::TcpSocket)
	    qobject_cast<QSslSocket *> (m_remote_socket)->
	      startServerEncryption();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
	  else
	    {
	      prepare_dtls();

	      if(m_dtls)
		if(!m_dtls->
		   doHandshake(qobject_cast<QUdpSocket *> (m_remote_socket),
			       initial_data))
		  log(QString("spot_on_lite_daemon_child_client::"
			      "spot_on_lite_daemon_child_client: "
			      "doHandshake() failure (%1).").
		      arg(m_dtls->dtlsErrorString()));
	    }
#else
	  Q_UNUSED(initial_data);
#endif
	}
    }
  else if(m_client_role)
    {
      QStringList list(m_server_identity.split(":"));

      m_peer_address = QHostAddress(list.value(0));
      m_peer_port = static_cast<quint16> (list.value(1).toInt());
      m_remote_socket->connectToHost(m_peer_address, m_peer_port);
    }

  save_statistic("client?", QVariant(m_client_role).toString());
}

spot_on_lite_daemon_child_client::~spot_on_lite_daemon_child_client()
{
  purge_statistics();
  stop_threads_and_timers();
}

QHash<QByteArray, QString>spot_on_lite_daemon_child_client::
remote_identities(bool *ok)
{
  if(ok)
    *ok = true;

  QHash<QByteArray, QString> hash;

#ifdef __arm__
  QReadLocker lock(&m_remote_identities_mutex);
  QHashIterator<QByteArray, qint64> it(m_remote_identities);

  while(it.hasNext())
    {
      it.next();
      hash[it.key()] = "sha-512";
    }
#else
  create_remote_identities_database();

  quint64 db_connection_id = db_id();

  {
    QSqlDatabase db = QSqlDatabase::addDatabase
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

QList<QByteArray> spot_on_lite_daemon_child_client::
local_certificate_configuration(void)
{
  QList<QByteArray> list;
  quint64 db_connection_id = db_id();

  {
    QSqlDatabase db = QSqlDatabase::addDatabase
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

QList<QSslCipher> spot_on_lite_daemon_child_client::
default_ssl_ciphers(void) const
{
  QList<QSslCipher> list;

  if(m_ssl_control_string.isEmpty())
    return list;

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
  protocols << "TlsV1_2"
	    << "TlsV1_1"
	    << "TlsV1_0";
#endif

  while(!protocols.isEmpty())
    {
      QString protocol(protocols.takeFirst());

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
	      log("spot_on_lite_daemon_child_client::"
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
	      log("spot_on_lite_daemon_child_client::"
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
		("spot_on_lite_daemon_child_client::"
		 "default_ssl_ciphers(): SSL_CTX_new(TLSv1_method()) failure.");
	      goto done_label;
	    }
	}
      else
	{
#ifndef OPENSSL_NO_SSL3_METHOD
	  if(!(ctx = SSL_CTX_new(SSLv3_method())))
	    {
	      log
		("spot_on_lite_daemon_child_client::"
		 "default_ssl_ciphers(): SSL_CTX_new(SSLv3_method()) failure.");
	      goto done_label;
	    }
#endif
	}
#endif

      if(!ctx)
	{
	  log("spot_on_lite_daemon_child_client::"
	      "default_ssl_ciphers(): ctx is zero!");
	  continue;
	}

      if(SSL_CTX_set_cipher_list(ctx,
				 m_ssl_control_string.toLatin1().
				 constData()) == 0)
	{
	  log("spot_on_lite_daemon_child_client::"
	      "default_ssl_ciphers(): SSL_CTX_set_cipher_list() failure.");
	  goto done_label;
	}

      if(!(ssl = SSL_new(ctx)))
	{
	  log("spot_on_lite_daemon_child_client::"
	      "default_ssl_ciphers(): SSL_new() failure.");
	  goto done_label;
	}

      do
	{
	  if((next = SSL_get_cipher_list(ssl, index)))
	    {
#if QT_VERSION < 0x050000
	      QSslCipher cipher;

	      if(protocol == "SslV3")
		cipher = QSslCipher(next, QSsl::SslV3);
	      else if(protocol == "TlsV1_0")
		cipher = QSslCipher(next, QSsl::TlsV1);
	      else
		cipher = QSslCipher(next, QSsl::UnknownProtocol);
#else
	      QSslCipher cipher;

	      if(protocol == "TlsV1_2")
		cipher = QSslCipher(next, QSsl::TlsV1_2);
	      else if(protocol == "TlsV1_1")
		cipher = QSslCipher(next, QSsl::TlsV1_1);
	      else if(protocol == "TlsV1_0")
		cipher = QSslCipher(next, QSsl::TlsV1_0);
	      else
		cipher = QSslCipher(next, QSsl::SslV3);
#endif

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
    log("spot_on_lite_daemon_child_client::default_ssl_ciphers(): "
	"empty cipher list.");

  return list;
}

bool spot_on_lite_daemon_child_client::memcmp(const QByteArray &a,
					      const QByteArray &b)
{
  int length = qMax(a.length(), b.length());
  int rc = 0;

  for(int i = 0; i < length; i++)
    rc |= a.mid(i, 1).at(0) ^ b.mid(i, 1).at(0);

  return rc == 0;
}

bool spot_on_lite_daemon_child_client::record_congestion(const QByteArray &data)
{
  bool added = false;
  quint64 db_connection_id = db_id();

  {
    QSqlDatabase db = QSqlDatabase::addDatabase
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
	query.addBindValue(QDateTime::currentDateTime().toTime_t());
#if QT_VERSION >= 0x050100
	query.addBindValue
	  (QCryptographicHash::hash(data, QCryptographicHash::Sha3_384).
	   toBase64());	
#elif QT_VERSION >= 0x050000
	query.addBindValue(QCryptographicHash::
			   hash(data, QCryptographicHash::Sha384).toBase64());
#else
	query.addBindValue(QCryptographicHash::
			   hash(data, QCryptographicHash::Sha1).toBase64());
#endif
	added = query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(QString::number(db_connection_id));
  return added;
}

quint64 spot_on_lite_daemon_child_client::db_id(void)
{
  QWriteLocker lock(&s_db_id_mutex);

  s_db_id += 1;
  return s_db_id;
}

void spot_on_lite_daemon_child_client::create_remote_identities_database(void)
{
  quint64 db_connection_id = db_id();

  {
    QSqlDatabase db = QSqlDatabase::addDatabase
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

void spot_on_lite_daemon_child_client::create_statistics_database(void)
{
  quint64 db_connection_id = db_id();

  {
    QSqlDatabase db = QSqlDatabase::addDatabase
      ("QSQLITE", QString::number(db_connection_id));

    db.setDatabaseName(m_statistics_file_name);

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("CREATE TABLE IF NOT EXISTS statistics ("
		   "pid BIGINT NOT NULL, "
		   "statistic TEXT NOT NULL, "
		   "value TEXT NOT NULL, "
		   "PRIMARY KEY (pid, statistic))");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(QString::number(db_connection_id));
}

void spot_on_lite_daemon_child_client::data_received(const QByteArray &data)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
  if(m_dtls && m_protocol == QAbstractSocket::UdpSocket)
    {
      QUdpSocket *socket = qobject_cast<QUdpSocket *> (m_remote_socket);

      if(m_dtls->isConnectionEncrypted())
	process_read_data(m_dtls->decryptDatagram(socket, data));
      else if(!m_dtls->doHandshake(socket, data))
	log(QString("spot_on_lite_daemon_child_client::data_received(): "
		    "doHandshake() failure (%1).").
	    arg(m_dtls->dtlsErrorString()));
    }
#else
  Q_UNUSED(data);
#endif
}

void spot_on_lite_daemon_child_client::generate_certificate
(RSA *rsa,
 QByteArray &certificate,
 const long int days,
 QString &error)
{
  BIO *memory = nullptr;
  BUF_MEM *bptr;
  EVP_PKEY *pk = nullptr;
  X509 *x509 = nullptr;
  X509_NAME *name = nullptr;
  X509_NAME *subject = nullptr;
  X509_NAME_ENTRY *common_name_entry = nullptr;
  char *buffer = nullptr;
  int length = 0;
  unsigned char *common_name = nullptr;

  if(!error.isEmpty())
    goto done_label;

  if(!rsa)
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

  if(EVP_PKEY_assign_RSA(pk, rsa) == 0)
    {
      error = "EVP_PKEY_assign_RSA() returned zero";
      goto done_label;
    }

  /*
  ** Set some attributes.
  */

  if(X509_set_version(x509, 3) == 0)
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
    (nullptr,
     NID_commonName, V_ASN1_PRINTABLESTRING,
     common_name, length);

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

  if(X509_sign(x509, pk, EVP_sha512()) == 0)
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
    {
      certificate.replace
	(0, certificate.length(), QByteArray(certificate.length(), 0));
      certificate.clear();
    }

  if(rsa)
    RSA_up_ref(rsa); // Reference counter.

  EVP_PKEY_free(pk);
  X509_NAME_ENTRY_free(common_name_entry);
  X509_NAME_free(subject);
  X509_free(x509);
  free(buffer);
  free(common_name);
}

void spot_on_lite_daemon_child_client::generate_ssl_tls(void)
{
  BIGNUM *f4 = nullptr;
  BIO *private_memory = nullptr;
  BIO *public_memory = nullptr;
  BUF_MEM *bptr;
  QByteArray certificate;
  QByteArray private_key;
  QByteArray public_key;
  QString error("");
  RSA *rsa = nullptr;
  char *private_buffer = nullptr;
  char *public_buffer = nullptr;
  long int days = 5L * 24L * 60L * 60L * 365L; // Five years.

  if(m_ssl_key_size <= 0)
    {
      error = "m_ssl_key_size is less than or equal to zero";
      goto done_label;
    }
  else if(m_ssl_key_size > 4096)
    {
      error = "m_ssl_key_size is greater than 4096";
      goto done_label;
    }

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
      certificate.replace
	(0, certificate.length(), QByteArray(certificate.length(), 0));
      certificate.clear();
      private_key.replace
	(0, private_key.length(), QByteArray(private_key.length(), 0));
      private_key.clear();
      public_key.replace
	(0, public_key.length(), QByteArray(public_key.length(), 0));
      public_key.clear();
      log(QString("spot_on_lite_daemon_child_client::"
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
  RSA_free(rsa);
  free(private_buffer);
  free(public_buffer);
}

void spot_on_lite_daemon_child_client::log(const QString &error) const
{
  QString e(error.trimmed());

  if(e.isEmpty())
    return;
  else
    qDebug() << e;

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

#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
void spot_on_lite_daemon_child_client::prepare_dtls(void)
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

void spot_on_lite_daemon_child_client::prepare_local_socket(void)
{
  {
    QWriteLocker lock(&m_local_content_mutex);

    m_local_content.clear();
  }

  m_local_socket->abort();
  m_local_socket->connectToServer(m_local_server_file_name);
}

void spot_on_lite_daemon_child_client::prepare_ssl_tls_configuration
(const QList<QByteArray> &list)
{
  m_ssl_configuration.setLocalCertificate(QSslCertificate(list.value(0)));

#if QT_VERSION < 0x050000
  if(m_ssl_configuration.localCertificate().isValid())
#else
  if(!m_ssl_configuration.localCertificate().isNull())
#endif
    {
      m_ssl_configuration.setPrivateKey(QSslKey(list.value(1), QSsl::Rsa));

      if(!m_ssl_configuration.privateKey().isNull())
	{
#if QT_VERSION >= 0x040800
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
#endif
#if QT_VERSION >= 0x050000
	  set_ssl_ciphers
	    (m_ssl_configuration.supportedCiphers(), m_ssl_configuration);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
	  if(m_protocol == QAbstractSocket::UdpSocket)
	    {
	      if(m_client_role)
		{
		  m_ssl_configuration.setPeerVerifyMode(QSslSocket::VerifyNone);
		  m_ssl_configuration.setProtocol(QSsl::DtlsV1_2OrLater);
		}
	      else
		{
		  m_ssl_configuration.setDtlsCookieVerificationEnabled(false);
		  m_ssl_configuration.setPeerVerifyMode(QSslSocket::VerifyNone);
		  m_ssl_configuration.setProtocol(QSsl::DtlsV1_2OrLater);
		}
	    }
#endif
#else
	  if(m_protocol == QAbstractSocket::TcpSocket)
	    set_ssl_ciphers
	      (qobject_cast<QSslSocket *> (m_remote_socket)->supportedCiphers(),
	       m_ssl_configuration);
#endif
	  if(m_protocol == QAbstractSocket::TcpSocket)
	    qobject_cast<QSslSocket *>
	      (m_remote_socket)->setSslConfiguration(m_ssl_configuration);
	}
      else
	/*
	** Error!
	*/

	log("spot_on_lite_daemon_child_client::"
	    "prepare_ssl_tls_configuration(): empty private key.");
    }
}

void spot_on_lite_daemon_child_client::process_data(void)
{
  {
    QReadLocker lock(&m_local_content_mutex);

    if(m_local_content.isEmpty())
      return;
  }

  QHash<QByteArray, QString> identities;

  do
    {
      bool ok = true;

      identities = remote_identities(&ok);

      if(!ok)
	{
	  struct timespec ts;

	  ts.tv_nsec = 250000000; // 250 Milliseconds
	  ts.tv_sec = 0;
	  nanosleep(&ts, nullptr);
	}
      else
	break;
    }
  while(!m_process_data_future.isCanceled());

#ifndef __arm__
  save_statistic("identities", QString::number(identities.size()));
#endif

  {
    QWriteLocker lock(&m_local_content_mutex);

    if(identities.isEmpty() || m_end_of_message_marker.isEmpty())
      {
	emit write_signal(m_local_content);
	m_local_content.clear();
	m_local_content_last_parsed = QDateTime::currentMSecsSinceEpoch();
	return;
      }
  }

  QVector<QByteArray> vector;
  int index = 0;

  {
    QWriteLocker lock(&m_local_content_mutex);

    while((index = m_local_content.indexOf(m_end_of_message_marker)) >= 0)
      {
	m_local_content_last_parsed = QDateTime::currentMSecsSinceEpoch();

	if(m_process_data_future.isCanceled())
	  goto done_label;

	QByteArray bytes
	  (m_local_content.mid(0, index + m_end_of_message_marker.length()));

	vector.append(bytes);
	m_local_content.remove(0, bytes.length());
      }

#ifndef __arm__
    save_statistic
      ("m_local_content remaining",
       QString::number(m_local_content.length()));
#endif
  }

  if(m_process_data_future.isCanceled() || vector.isEmpty())
    goto done_label;

#ifndef __arm__
  save_statistic("vector size", QString::number(vector.size()));
#endif

  for(int i = 0; i < vector.size(); i++)
    {
      QByteArray bytes(vector.at(i));

      if((index = bytes.indexOf("content=")) >= 0)
	{
	  QByteArray data(bytes.mid(8 + index).trimmed());
	  QByteArray hash;

	  if(data.contains("\n")) // Spot-On
	    {
	      QList<QByteArray> list(data.split('\n'));

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

	  while(it.hasNext() && !m_process_data_future.isCanceled())
	    {
	      it.next();

	      if(memcmp(hash, m_sha_512.sha_512_hmac(data, it.key())))
		{
		  emit write_signal(bytes);
		  break;
		}
	    }
	}
      else
	emit write_signal(bytes);
    }

 done_label:

  if(m_process_data_future.isCanceled())
    {
      QWriteLocker lock(&m_local_content_mutex);

      m_local_content.clear();
    }
  else
    {
      QReadLocker lock(&m_local_content_mutex);

      if(!m_local_content.isEmpty())
	emit read_signal();
    }
}

void spot_on_lite_daemon_child_client::process_read_data(const QByteArray &d)
{
  QByteArray data(d);

  if(data.isEmpty())
    return;
  else
    data = data.mid(0, m_maximum_accumulated_bytes);

  if(m_client_role || m_end_of_message_marker.isEmpty())
    {
      m_keep_alive_timer.start();

      if(record_congestion(data))
	m_local_socket->write(data);

      return;
    }

  m_keep_alive_timer.start();

  if(m_remote_content.length() >= m_maximum_accumulated_bytes)
    m_remote_content.clear();

  m_remote_content.
    append(data.mid(0, qAbs(m_maximum_accumulated_bytes -
			    m_remote_content.length())));
  process_remote_content();
#ifndef __arm__
  save_statistic
    ("m_remote_content", QString::number(m_remote_content.length()));
#endif
}

void spot_on_lite_daemon_child_client::process_remote_content(void)
{
  if(m_client_role ||
     m_end_of_message_marker.isEmpty() ||
     m_local_socket->state() != QLocalSocket::ConnectedState)
    return;

  QByteArray data;
  int index = 0;

  while((index = m_remote_content.indexOf(m_end_of_message_marker)) >= 0)
    {
      data = m_remote_content.mid(0, index + m_end_of_message_marker.length());
      m_remote_content.remove(0, data.length());
      m_remote_content_last_parsed = QDateTime::currentMSecsSinceEpoch();

      if(data.contains("type=0014&content="))
	continue;
      else if(data.contains("type=0095a&content="))
	{
	  record_remote_identity(data);
	  continue;
	}

      if(record_congestion(data))
	m_local_socket->write(data);
    }

#ifndef __arm__
  save_statistic
    ("m_remote_content remaining",
     QString::number(m_remote_content.length()));
#endif
}

void spot_on_lite_daemon_child_client::purge_containers(void)
{
  {
    QWriteLocker lock(&m_local_content_mutex);

    m_local_content.clear();
    m_local_content_last_parsed = 0;
  }

  m_remote_content.clear();
  m_remote_content_last_parsed = 0;
  purge_remote_identities();
}

void spot_on_lite_daemon_child_client::purge_remote_identities(void)
{
#ifdef __arm__
  QWriteLocker lock(&m_remote_identities_mutex);

  m_remote_identities.clear();
#else
  quint64 db_connection_id = db_id();

  {
    QSqlDatabase db = QSqlDatabase::addDatabase
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

void spot_on_lite_daemon_child_client::purge_statistics(void)
{
  quint64 db_connection_id = db_id();

  {
    QSqlDatabase db = QSqlDatabase::addDatabase
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

void spot_on_lite_daemon_child_client::record_certificate
(const QByteArray &certificate,
 const QByteArray &private_key,
 const QByteArray &public_key)
{
  if(m_client_role)
    return;

  quint64 db_connection_id = db_id();

  {
    QSqlDatabase db = QSqlDatabase::addDatabase
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

void spot_on_lite_daemon_child_client::record_remote_identity
(const QByteArray &data)
{
  if(m_client_role)
    /*
    ** Only server sockets should record identities.
    */

    return;

  QByteArray algorithm;
  QByteArray identity;
  int index = data.indexOf("content=");

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
#ifdef __arm__
      QWriteLocker lock(&m_remote_identities_mutex);

      if(!m_remote_identities.contains(identity))
	{
	  if(ARM_MAXIMUM_REMOTE_IDENTITIES > m_remote_identities.size())
	    m_remote_identities[identity] =
	      QDateTime::currentDateTime().toTime_t();
	}
      else
	m_remote_identities[identity] = QDateTime::currentDateTime().toTime_t();
#else
      create_remote_identities_database();

      quint64 db_connection_id = db_id();

      {
	QSqlDatabase db = QSqlDatabase::addDatabase
	  ("QSQLITE", QString::number(db_connection_id));

	db.setDatabaseName(m_remote_identities_file_name);

	if(db.open())
	  {
	    QSqlQuery query(db);

	    query.prepare("INSERT OR REPLACE INTO remote_identities "
			  "(algorithm, date_time_inserted, identity, pid) "
			  "VALUES (?, ?, ?, ?)");
	    query.addBindValue(algorithm);
	    query.addBindValue(QDateTime::currentDateTime().toTime_t());
	    query.addBindValue(identity.toBase64());
	    query.addBindValue(m_pid);
	    query.exec();
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase(QString::number(db_connection_id));
#endif
    }
}

void spot_on_lite_daemon_child_client::remove_expired_identities(void)
{
#ifdef __arm__
  QWriteLocker lock(&m_remote_identities_mutex);
  QMutableHashIterator<QByteArray, qint64> it(m_remote_identities);

  while(it.hasNext())
    {
      it.next();

      if(QDateTime::currentDateTime().toTime_t() - it.value() >
	 m_identity_lifetime)
	it.remove();
    }
#else
  quint64 db_connection_id = db_id();

  {
    QSqlDatabase db = QSqlDatabase::addDatabase
      ("QSQLITE", QString::number(db_connection_id));

    db.setDatabaseName(m_remote_identities_file_name);

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec
	  (QString("DELETE FROM remote_identities WHERE "
		   "%1 - date_time_inserted > %2").
	   arg(QDateTime::currentDateTime().toTime_t()).
	   arg(m_identity_lifetime));
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(QString::number(db_connection_id));
#endif
}

void spot_on_lite_daemon_child_client::save_statistic
(const QString &key, const QString &value)
{
  create_statistics_database();

  quint64 db_connection_id = db_id();

  {
    QSqlDatabase db = QSqlDatabase::addDatabase
      ("QSQLITE", QString::number(db_connection_id));

    db.setDatabaseName(m_statistics_file_name);

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA journal_mode = OFF");
	query.exec("PRAGMA synchronous = OFF");
	query.prepare("INSERT OR REPLACE INTO statistics "
		      "(pid, statistic, value) "
		      "VALUES (?, ?, ?)");
	query.addBindValue(m_pid);
	query.addBindValue(key);
	query.addBindValue(value);
	query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(QString::number(db_connection_id));
}

void spot_on_lite_daemon_child_client::
set_ssl_ciphers(const QList<QSslCipher> &ciphers,
		QSslConfiguration &configuration) const
{
  QList<QSslCipher> preferred(default_ssl_ciphers());

  for(int i = preferred.size() - 1; i >= 0; i--)
    if(!ciphers.contains(preferred.at(i)))
      preferred.removeAt(i);

  if(preferred.isEmpty())
    configuration.setCiphers(ciphers);
  else
    configuration.setCiphers(preferred);
}

void spot_on_lite_daemon_child_client::slot_attempt_local_connection(void)
{
  prepare_local_socket();
}

void spot_on_lite_daemon_child_client::slot_attempt_remote_connection(void)
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

#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
	  prepare_dtls();

	  if(m_dtls)
	    if(!m_dtls->
	       doHandshake(qobject_cast<QUdpSocket *> (m_remote_socket)))
	      log(QString("spot_on_lite_daemon_child_client::"
			  "slot_attempt_remote_connection(): "
			  "doHandshake() failure (%1).").
		  arg(m_dtls->dtlsErrorString()));
#endif
	}
    }
  else
    m_remote_socket->connectToHost(m_peer_address, m_peer_port);
}

void spot_on_lite_daemon_child_client::slot_broadcast_capabilities(void)
{
  /*
  ** Capabilities
  */

  QByteArray data;
  QByteArray results;
  static QUuid uuid(QUuid::createUuid());

  data.append(uuid.toString());
  data.append("\n");
  data.append(QByteArray::number(m_maximum_accumulated_bytes));
  data.append("\n");
  data.append("full");
  results.append("POST HTTP/1.1\r\n"
		 "Content-Type: application/x-www-form-urlencoded\r\n"
		 "Content-Length: %1\r\n"
		 "\r\n"
		 "type=0014&content=%2\r\n"
		 "\r\n\r\n");
  results.replace
    ("%1",
     QByteArray::number(data.toBase64().length() +
			QString("type=0014&content=\r\n\r\n\r\n").length()));
  results.replace("%2", data.toBase64());
  write(results);
}

void spot_on_lite_daemon_child_client::slot_connected(void)
{
  m_attempt_local_connection_timer.start();
  m_attempt_remote_connection_timer.stop();
  m_capabilities_timer.start(m_silence / 2);
  m_remote_content_last_parsed = QDateTime::currentMSecsSinceEpoch();
  m_remote_socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

  int sd = static_cast<int> (m_remote_socket->socketDescriptor());
  socklen_t optlen = sizeof(m_maximum_accumulated_bytes);

  setsockopt(sd, SOL_SOCKET, SO_RCVBUF, &m_maximum_accumulated_bytes, optlen);
  setsockopt(sd, SOL_SOCKET, SO_SNDBUF, &m_maximum_accumulated_bytes, optlen);
}

void spot_on_lite_daemon_child_client::slot_disconnected(void)
{
  if(m_client_role)
    {
      if(!m_attempt_remote_connection_timer.isActive())
	m_attempt_remote_connection_timer.start();

      m_capabilities_timer.stop();
      m_local_socket->abort();
      m_remote_socket->abort();
      purge_containers();
    }
  else
    {
      m_local_socket->abort();
      m_remote_socket->abort();
      purge_statistics();
      stop_threads_and_timers();

      if(m_protocol != QAbstractSocket::UdpSocket)
	QCoreApplication::exit(0);
      else
	deleteLater();
    }
}

void spot_on_lite_daemon_child_client::slot_general_timer_timeout(void)
{
  {
    QWriteLocker lock(&m_local_content_mutex);

    if(QDateTime::currentMSecsSinceEpoch() - m_local_content_last_parsed >
       END_OF_MESSAGE_MARKER_WINDOW)
      {
	m_local_content.clear();
	m_local_content_last_parsed = QDateTime::currentMSecsSinceEpoch();
      }

#ifndef __arm__
    save_statistic
      ("m_local_content", QString::number(m_local_content.length()));
#endif
  }

  if(QDateTime::currentMSecsSinceEpoch() - m_remote_content_last_parsed >
     END_OF_MESSAGE_MARKER_WINDOW)
    {
      m_remote_content.clear();
      m_remote_content_last_parsed = QDateTime::currentMSecsSinceEpoch();
    }

#ifndef __arm__
  save_statistic
    ("m_remote_content", QString::number(m_remote_content.length()));
#endif
}

#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
void spot_on_lite_daemon_child_client::slot_handshake_timeout(void)
{
  if(m_dtls && m_protocol == QAbstractSocket::UdpSocket)
    m_dtls->handleTimeout(qobject_cast<QUdpSocket *> (m_remote_socket));
}
#else
void spot_on_lite_daemon_child_client::slot_handshake_timeout(void)
{
}
#endif

void spot_on_lite_daemon_child_client::slot_keep_alive_timer_timeout(void)
{
  if(m_remote_socket->peerAddress().isNull())
    log(QString("spot_on_lite_daemon_child_client::"
		"slot_keep_alive_timer_timeout(): peer %1:%2 "
		"aborting!").
	arg(m_peer_address.toString()).
	arg(m_peer_port));
  else
    log(QString("spot_on_lite_daemon_child_client::"
		"slot_keep_alive_timer_timeout(): peer %1:%2 "
		"aborting!").
	arg(m_remote_socket->peerAddress().toString()).
	arg(m_remote_socket->peerPort()));

  if(m_client_role)
    {
      m_local_socket->abort();
      m_remote_socket->abort();

      if(!m_attempt_remote_connection_timer.isActive())
	m_attempt_remote_connection_timer.start();

      purge_containers();
    }
  else
    {
      m_local_socket->abort();
      m_remote_socket->abort();
      purge_statistics();
      stop_threads_and_timers();

      if(m_protocol != QAbstractSocket::UdpSocket)
	QCoreApplication::exit(0);
      else
	deleteLater();
    }
}

void spot_on_lite_daemon_child_client::slot_local_socket_connected(void)
{
  {
    QWriteLocker lock(&m_local_content_mutex);

    m_local_content_last_parsed = QDateTime::currentMSecsSinceEpoch();
  }

  int sd = static_cast<int> (m_local_socket->socketDescriptor());
  socklen_t optlen = sizeof(m_local_so_sndbuf);

  setsockopt(sd, SOL_SOCKET, SO_RCVBUF, &m_local_so_sndbuf, optlen);
  setsockopt(sd, SOL_SOCKET, SO_SNDBUF, &m_local_so_sndbuf, optlen);

  /*
  ** We may have remote content.
  */

  process_remote_content();
}

void spot_on_lite_daemon_child_client::slot_local_socket_disconnected(void)
{
  if(!m_attempt_local_connection_timer.isActive())
    m_attempt_local_connection_timer.start();
}

void spot_on_lite_daemon_child_client::slot_local_socket_ready_read(void)
{
  while(m_local_socket->bytesAvailable() > 0)
    {
      QByteArray data(m_local_socket->readAll());

      if(!data.isEmpty())
	{
	  QWriteLocker lock(&m_local_content_mutex);

	  if(m_local_content.length() >= m_maximum_accumulated_bytes)
	    m_local_content.clear();

	  m_local_content.append
	    (data.mid(0, qAbs(m_maximum_accumulated_bytes -
			      m_local_content.length())));
#ifndef __arm__
	  save_statistic
	    ("m_local_content", QString::number(m_local_content.length()));
#endif
	}
    }

  QReadLocker lock(&m_local_content_mutex);

  if(m_local_content.isEmpty())
    return;
  else
    lock.unlock();

  if(m_process_data_future.isFinished())
    m_process_data_future = QtConcurrent::run
      (this, &spot_on_lite_daemon_child_client::process_data);
}

void spot_on_lite_daemon_child_client::slot_ready_read(void)
{
  QByteArray data(m_remote_socket->readAll());

#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
  if(!data.isEmpty() && m_dtls && m_protocol == QAbstractSocket::UdpSocket)
    {
      QUdpSocket *socket = qobject_cast<QUdpSocket *> (m_remote_socket);

      if(m_dtls->isConnectionEncrypted())
	data = m_dtls->decryptDatagram(socket, data);
      else
	{
	  if(!m_dtls->doHandshake(socket, data))
	    log(QString("spot_on_lite_daemon_child_client::"
			"slot_ready_read(): doHandshake() failure (%1).").
		arg(m_dtls->dtlsErrorString()));

	  return;
	}
    }
#endif

  process_read_data(data);
}

void spot_on_lite_daemon_child_client::slot_remove_expired_identities(void)
{
  if(m_expired_identities_future.isFinished())
    m_expired_identities_future = QtConcurrent::run
      (this, &spot_on_lite_daemon_child_client::remove_expired_identities);
}

void spot_on_lite_daemon_child_client::
slot_ssl_errors(const QList<QSslError> &errors)
{
  Q_UNUSED(errors);

  if(m_protocol == QAbstractSocket::TcpSocket)
    qobject_cast<QSslSocket *> (m_remote_socket)->ignoreSslErrors();
}

void spot_on_lite_daemon_child_client::slot_write_data(const QByteArray &data)
{
  write(data);
}

void spot_on_lite_daemon_child_client::stop_threads_and_timers(void)
{
  m_attempt_local_connection_timer.stop();
  m_attempt_remote_connection_timer.stop();
  m_capabilities_timer.stop();
  m_expired_identities_future.cancel();
  m_expired_identities_timer.stop();
  m_general_timer.stop();
  m_keep_alive_timer.stop();
  m_process_data_future.cancel();

  /*
  ** Wait for threads to complete.
  */

  m_expired_identities_future.waitForFinished();
  m_process_data_future.waitForFinished();
}

void spot_on_lite_daemon_child_client::write(const QByteArray &data)
{
  switch(m_protocol)
    {
    case QAbstractSocket::TcpSocket:
      {
	int i = 0;
	static const int maximum_packet_size = 32768;

	while(data.size() > i)
	  {
	    m_remote_socket->write(data.mid(i, maximum_packet_size));
	    i += maximum_packet_size;
	  }

	break;
      }
    case QAbstractSocket::UdpSocket:
      {
	int i = 0;
	static const int maximum_datagram_size = 508;

	while(data.size() > i)
	  {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
	    if(m_dtls)
	      m_dtls->writeDatagramEncrypted
		(qobject_cast<QUdpSocket *> (m_remote_socket),
		 data.mid(i, maximum_datagram_size));
	    else
#endif
	    if(m_client_role)
	      m_remote_socket->write(data.mid(i, maximum_datagram_size));
	    else
	      qobject_cast<QUdpSocket *> (m_remote_socket)->writeDatagram
		(data.mid(i, maximum_datagram_size),
		 m_peer_address,
		 m_peer_port);

	    i += maximum_datagram_size;
	  }

	break;
      }
    default:
      {
	break;
      }
    }
}

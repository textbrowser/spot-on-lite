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
#include <errno.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
}

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QHostAddress>
#include <QSslConfiguration>
#include <QSslKey>
#include <QStringList>
#include <QTimer>

#include <limits>

#include "spot-on-lite-daemon-child-tcp-client.h"

static QByteArray EOM = "\r\n\r\n\r\n";

spot_on_lite_daemon_child_tcp_client::
spot_on_lite_daemon_child_tcp_client
(const QString &log_file_name,
 const QString &ssl_control_string,
 const int maximum_accumulated_bytes,
 const int silence,
 const int socket_descriptor,
 const int ssl_key_size):QSslSocket()
{
  m_can_use_ssl = false;
  m_log_file_name = log_file_name;
  m_maximum_accumulated_bytes = maximum_accumulated_bytes;

  if(m_maximum_accumulated_bytes < 1024)
    m_maximum_accumulated_bytes = 8 * 1024 * 1024;

  m_silence = 1000 * qBound(15, silence, 3600);
  m_ssl_control_string = ssl_control_string.trimmed();
  m_ssl_key_size = ssl_key_size;

  if(!(m_ssl_key_size == 2048 ||
       m_ssl_key_size == 3072 ||
       m_ssl_key_size == 4096))
    {
      m_ssl_control_string.clear();
      m_ssl_key_size = 0;
    }

  if(!setSocketDescriptor(socket_descriptor))
    {
      /*
      ** Fatal error!
      */

      QTimer::singleShot(2500, this, SLOT(slot_disconnected(void)));
      return;
    }

  connect(&m_keep_alive_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_disconnected(void)));
  connect(this,
	  SIGNAL(disconnected(void)),
	  this,
	  SLOT(slot_disconnected(void)));
  connect(this,
	  SIGNAL(keep_alive(void)),
	  this,
	  SLOT(slot_keep_alive(void)));
  connect(this,
	  SIGNAL(readyRead(void)),
	  this,
	  SLOT(slot_ready_read(void)));
  m_keep_alive_timer.start(m_silence);

  if(!m_ssl_control_string.isEmpty() && m_ssl_key_size > 0)
    {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
      SSL_library_init(); // Always returns 1.
#else
      OPENSSL_init_ssl(0, NULL);
#endif
      generate_ssl_tls();

      if(m_can_use_ssl)
	startServerEncryption();
    }
}

spot_on_lite_daemon_child_tcp_client::
~spot_on_lite_daemon_child_tcp_client()
{
}

QList<QSslCipher> spot_on_lite_daemon_child_tcp_client::
default_ssl_ciphers(void) const
{
  QList<QSslCipher> list;

  if(m_ssl_control_string.isEmpty())
    return list;

  QStringList protocols;
  SSL *ssl = 0;
  SSL_CTX *ctx = 0;
  const char *next = 0;
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
      next = 0;

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
      ctx = SSL_CTX_new(TLS_client_method());
#else
      if(protocol == "TlsV1_2")
	{
#ifdef TLS1_2_VERSION
	  if(!(ctx = SSL_CTX_new(TLSv1_2_client_method())))
	    {
	      log("spot_on_lite_daemon_child_tcp_client::"
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
	      log("spot_on_lite_daemon_child_tcp_client::"
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
		("spot_on_lite_daemon_child_tcp_client::"
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
		("spot_on_lite_daemon_child_tcp_client::"
		 "default_ssl_ciphers(): SSL_CTX_new(SSLv3_method()) failure.");
	      goto done_label;
	    }
#endif
	}
#endif

      if(!ctx)
	{
	  log("spot_on_lite_daemon_child_tcp_client::"
	      "default_ssl_ciphers(): ctx is zero!");
	  continue;
	}

      if(SSL_CTX_set_cipher_list(ctx,
				 m_ssl_control_string.toLatin1().
				 constData()) == 0)
	{
	  log("spot_on_lite_daemon_child_tcp_client::"
	      "default_ssl_ciphers(): SSL_CTX_set_cipher_list() failure.");
	  goto done_label;
	}

      if(!(ssl = SSL_new(ctx)))
	{
	  log("spot_on_lite_daemon_child_tcp_client::"
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
      ctx = 0;
      ssl = 0;
    }

  if(list.isEmpty())
    log("spot_on_lite_daemon_child_tcp_client::default_ssl_ciphers(): "
	"empty cipher list.");

  return list;
}

void spot_on_lite_daemon_child_tcp_client::
generate_certificate(RSA *rsa,
		     QByteArray &certificate,
		     const long int days,
		     QString &error)
{
  BIO *memory = 0;
  BUF_MEM *bptr;
  EVP_PKEY *pk = 0;
  X509 *x509 = 0;
  X509_NAME *name = 0;
  X509_NAME *subject = 0;
  X509_NAME_ENTRY *common_name_entry = 0;
  char *buffer = 0;
  int length = 0;
  unsigned char *common_name = 0;

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

  if(X509_gmtime_adj(X509_get_notBefore(x509), 0) == 0)
    {
      error = "X509_gmtime_adj() returned zero";
      goto done_label;
    }

  if(X509_gmtime_adj(X509_get_notAfter(x509), days) == 0)
    {
      error = "X509_gmtime_adj() returned zero";
      goto done_label;
    }

  if(std::numeric_limits<int>::max() -
     localAddress().toString().toLatin1().length() < 1)
    common_name = 0;
  else
    common_name = static_cast<unsigned char *>
      (calloc(static_cast<size_t> (localAddress().toString().
				   toLatin1().length() + 1),
	      sizeof(unsigned char)));

  if(!common_name)
    {
      error = "calloc() returned zero or invalid local address";
      goto done_label;
    }

  length = localAddress().toString().toLatin1().length();
  memcpy(common_name,
	 localAddress().toString().toLatin1().constData(),
	 static_cast<size_t> (length));
  common_name_entry = X509_NAME_ENTRY_create_by_NID
    (0,
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

  if((name = X509_get_subject_name(x509)) == 0)
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

void spot_on_lite_daemon_child_tcp_client::generate_ssl_tls(void)
{
  BIGNUM *f4 = 0;
  BIO *private_memory = 0;
  BIO *public_memory = 0;
  BUF_MEM *bptr;
  QByteArray certificate;
  QByteArray private_key;
  QByteArray public_key;
  QString error("");
  RSA *rsa = 0;
  char *private_buffer = 0;
  char *public_buffer = 0;
  long int days = 60L * 60L * 24L * 365L; // One year.

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

  if(RSA_generate_key_ex(rsa, m_ssl_key_size, f4, 0) == -1)
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

  if(PEM_write_bio_RSAPrivateKey(private_memory, rsa, 0, 0, 0, 0, 0) == 0)
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

  if(days > 0)
    generate_certificate(rsa, certificate, days, error);

 done_label:

  if(!error.isEmpty())
    {
      m_can_use_ssl = false;
      certificate.replace
	(0, certificate.length(), QByteArray(certificate.length(), 0));
      certificate.clear();
      private_key.replace
	(0, private_key.length(), QByteArray(private_key.length(), 0));
      private_key.clear();
      public_key.replace
	(0, public_key.length(), QByteArray(public_key.length(), 0));
      public_key.clear();
      log(QString("spot_on_lite_daemon_child_tcp_client::"
		  "generate_ssl_tls(): error (%1) occurred.").arg(error));
    }
  else
    {
      QSslConfiguration configuration;

      configuration.setLocalCertificate(QSslCertificate(certificate));

#if QT_VERSION < 0x050000
      if(configuration.localCertificate().isValid())
#else
      if(!configuration.localCertificate().isNull())
#endif
	{
	  configuration.setPrivateKey(QSslKey(private_key, QSsl::Rsa));

	  if(!configuration.privateKey().isNull())
	    {
#if QT_VERSION >= 0x040800
	      configuration.setSslOption
		(QSsl::SslOptionDisableCompression, true);
	      configuration.setSslOption
		(QSsl::SslOptionDisableEmptyFragments, true);
	      configuration.setSslOption
		(QSsl::SslOptionDisableLegacyRenegotiation, true);
	      configuration.setSslOption
		(QSsl::SslOptionDisableSessionTickets, true);
#if QT_VERSION >= 0x050200
	      configuration.setSslOption
		(QSsl::SslOptionDisableSessionPersistence, true);
	      configuration.setSslOption
		(QSsl::SslOptionDisableSessionSharing, true);
#endif
#endif
	      m_can_use_ssl = true;
#if QT_VERSION >= 0x050000
	      set_ssl_ciphers(configuration.supportedCiphers(), configuration);
#else
	      set_ssl_ciphers(supportedCiphers(), configuration);
#endif
	      setSslConfiguration(configuration);
	    }
	  else
	    {
	      m_can_use_ssl = false;

	      /*
	      ** Error!
	      */

	      log("spot_on_lite_daemon_child_tcp_client::"
		  "generate_ssl_tls(): empty private key.");
	    }
	}
    }

  BIO_free(private_memory);
  BIO_free(public_memory);
  BN_free(f4);
  RSA_free(rsa);
  free(private_buffer);
  free(public_buffer);
}

void spot_on_lite_daemon_child_tcp_client::log(const QString &error) const
{
  if(error.trimmed().isEmpty())
    return;
  else
    qDebug() << error;

  QFile file(m_log_file_name);

  if(file.open(QIODevice::Append | QIODevice::WriteOnly))
    {
      QDateTime dateTime(QDateTime::currentDateTime());

      file.write(dateTime.toString().toStdString().data());
      file.write("\n");
      file.write(error.trimmed().toStdString().data());
      file.write("\n");
      file.close();
    }
}

void spot_on_lite_daemon_child_tcp_client::
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

void spot_on_lite_daemon_child_tcp_client::slot_disconnected(void)
{
  QCoreApplication::exit(0);
}

void spot_on_lite_daemon_child_tcp_client::slot_keep_alive(void)
{
  m_keep_alive_timer.start();
}

void spot_on_lite_daemon_child_tcp_client::slot_ready_read(void)
{
  if(!isEncrypted() && m_can_use_ssl)
    {
      readAll();
      return;
    }

  QByteArray data(readAll().trimmed());

  if(!data.isEmpty())
    {
      emit keep_alive();

      if(m_content.length() >= m_maximum_accumulated_bytes)
	m_content.clear();

      m_content.append
	(data.mid(0, qAbs(m_maximum_accumulated_bytes - m_content.length())));
    }

  int index = 0;

  if((index = m_content.indexOf(EOM)) > 0)
    {
      data = m_content.mid(0, EOM.length() + index);
      m_content.remove(0, data.length());
    }
}

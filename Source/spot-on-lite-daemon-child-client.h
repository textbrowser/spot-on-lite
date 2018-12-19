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

#ifndef _spot_on_lite_daemon_child_client_h_
#define _spot_on_lite_daemon_child_client_h_

extern "C"
{
#include <openssl/rsa.h>
}

#include <QFuture>
#include <QReadWriteLock>
#include <QSslCipher>
#include <QSslSocket>
#include <QTimer>

#include "spot-on-lite-daemon-sha.h"

class QLocalSocket;

class spot_on_lite_daemon_child_client: public QSslSocket
{
  Q_OBJECT

 public:
  spot_on_lite_daemon_child_client
    (const QString &certificates_file_name,
     const QString &congestion_control_file_name,
     const QString &end_of_message_marker,
     const QString &local_server_file_name,
     const QString &log_file_name,
     const QString &remote_identities_file_name,
     const QString &server_identity,
     const QString &ssl_control_string,
     const int identities_lifetime,
     const int local_so_sndbuf,
     const int maximum_accumulated_bytes,
     const int silence,
     const int socket_descriptor,
     const int ssl_key_size);
  ~spot_on_lite_daemon_child_client();
  static bool memcmp(const QByteArray &a, const QByteArray &b);

 private:
  QByteArray m_local_content;
  QByteArray m_remote_content;
  QFuture<void> m_expired_identities_future;
  QFuture<void> m_process_data_future;
#ifdef __arm__
  QHash<QByteArray, qint64> m_remote_identities;
#endif
  QLocalSocket *m_local_socket;
  QReadWriteLock m_local_content_mutex;
  QReadWriteLock m_db_id_mutex;
#ifdef __arm__
  QReadWriteLock m_remote_identities_mutex;
#endif
  QString m_certificates_file_name;
  QString m_congestion_control_file_name;
  QString m_end_of_message_marker;
  QString m_local_server_file_name;
  QString m_log_file_name;
  QString m_remote_identities_file_name;
  QString m_server_identity;
  QString m_ssl_control_string;
  QTimer m_attempt_local_connection_timer;
  QTimer m_attempt_remote_connection_timer;
  QTimer m_capabilities_timer;
  QTimer m_expired_identities_timer;
  QTimer m_keep_alive_timer;
  bool m_client_role;
  int m_identity_lifetime;
  int m_local_so_sndbuf;
  int m_maximum_accumulated_bytes;
  int m_silence;
  int m_ssl_key_size;
  quint64 m_db_id;
  spot_on_lite_daemon_sha m_sha_512;
  QHash<QByteArray, QString> remote_identities(bool *ok);
  QList<QByteArray> local_certificate_configuration(void);
  QList<QSslCipher> default_ssl_ciphers(void) const;
  bool record_congestion(const QByteArray &data);
  quint64 db_id(void);
  void create_remote_identities_database(void);
  void generate_certificate(RSA *rsa,
			    QByteArray &certificate,
			    const long int days,
			    QString &error);
  void generate_ssl_tls(void);
  void log(const QString &error) const;
  void prepare_local_socket(void);
  void prepare_ssl_tls_configuration(const QList<QByteArray> &list);
  void process_data(void);
  void process_remote_content(void);
  void purge_containers(void);
  void purge_remote_identities(void);
  void record_certificate(const QByteArray &certificate,
			  const QByteArray &private_key,
			  const QByteArray &public_key);
  void record_remote_identity(const QByteArray &data);
  void remove_expired_identities(void);
  void set_ssl_ciphers(const QList<QSslCipher> &ciphers,
		       QSslConfiguration &configuration) const;
  void stop_threads_and_timers(void);

 private slots:
  void slot_attempt_local_connection(void);
  void slot_attempt_remote_connection(void);
  void slot_broadcast_capabilities(void);
  void slot_connected(void);
  void slot_disconnected(void);
  void slot_keep_alive_timer_timeout(void);
  void slot_local_socket_connected(void);
  void slot_local_socket_disconnected(void);
  void slot_local_socket_ready_read(void);
  void slot_ready_read(void);
  void slot_remove_expired_identities(void);
  void slot_ssl_errors(const QList<QSslError> &errors);
  void slot_write_data(const QByteArray &data);

 signals:
  void read_signal(void);
  void write_signal(const QByteArray &data);
};

#endif

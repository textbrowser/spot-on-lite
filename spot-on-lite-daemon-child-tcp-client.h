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

#ifndef _spot_on_lite_daemon_child_tcp_client_h_
#define _spot_on_lite_daemon_child_tcp_client_h_

extern "C"
{
#include <openssl/rsa.h>
}

#include <QSslCipher>
#include <QSslSocket>
#include <QTimer>

class spot_on_lite_daemon_child_tcp_client: public QSslSocket
{
  Q_OBJECT

 public:
  spot_on_lite_daemon_child_tcp_client
    (const QString &congestion_control_file_name,
     const QString &log_file_name,
     const QString &ssl_control_string,
     const int maximum_accumulated_bytes,
     const int silence,
     const int socket_descriptor,
     const int ssl_key_size);
  ~spot_on_lite_daemon_child_tcp_client();

 private:
  QByteArray m_content;
  QString m_congestion_control_file_name;
  QString m_log_file_name;
  QString m_ssl_control_string;
  QTimer m_keep_alive_timer;
  bool m_can_use_ssl;
  int m_maximum_accumulated_bytes;
  int m_silence;
  int m_ssl_key_size;
  QList<QSslCipher> default_ssl_ciphers(void) const;
  bool record_congestion(const QByteArray &bytes) const;
  void generate_certificate(RSA *rsa,
			    QByteArray &certificate,
			    const long int days,
			    QString &error);
  void generate_ssl_tls(void);
  void log(const QString &error) const;
  void set_ssl_ciphers(const QList<QSslCipher> &ciphers,
		       QSslConfiguration &configuration) const;

 private slots:
  void slot_disconnected(void);
  void slot_keep_alive(void);
  void slot_ready_read(void);

 signals:
  void keep_alive(void);
};

#endif

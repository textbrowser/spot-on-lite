/*
** Alexis Megas.
*/

#ifndef _spot_on_lite_daemon_child_tcp_client_h_
#define _spot_on_lite_daemon_child_tcp_client_h_

extern "C"
{
#include <openssl/rsa.h>
}

#include <QFuture>
#include <QReadWriteLock>
#include <QSslCipher>
#include <QSslSocket>
#include <QTimer>

class spot_on_lite_daemon_child_tcp_client: public QSslSocket
{
  Q_OBJECT

 public:
  spot_on_lite_daemon_child_tcp_client
    (const QString &log_file_name,
     const QString &ssl_control_string,
     const int maximum_accumulated_bytes,
     const int silence,
     const int socket_descriptor,
     const int ssl_key_size);
  ~spot_on_lite_daemon_child_tcp_client();

 private:
  QByteArray m_content;
  QFuture<void> m_future;
  QReadWriteLock m_contentLock;
  QString m_log_file_name;
  QString m_ssl_control_string;
  QTimer m_keep_alive_timer;
  bool m_can_use_ssl;
  int m_maximum_accumulated_bytes;
  int m_silence;
  int m_ssl_key_size;
  QList<QSslCipher> default_ssl_ciphers(void) const;
  void generate_certificate(RSA *rsa,
			    QByteArray &certificate,
			    const long int days,
			    QString &error);
  void generate_ssl_tls(void);
  void log(const QString &error) const;
  void parse(const QByteArray &data);
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

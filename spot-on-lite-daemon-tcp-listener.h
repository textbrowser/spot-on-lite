/*
** Alexis Megas.
*/

#ifndef _spot_on_lite_daemon_tcp_listener_h_
#define _spot_on_lite_daemon_tcp_listener_h_

#include <QTcpServer>
#include <QTimer>

class spot_on_lite_daemon_tcp_listener: public QTcpServer
{
  Q_OBJECT

 public:
  spot_on_lite_daemon_tcp_listener(const QString &configuration,
				   QObject *parent);
  ~spot_on_lite_daemon_tcp_listener();

 protected:
#if QT_VERSION >= 0x050000
  void incoming_connection(qintptr socket_descriptor);
#else
  void incoming_connection(int socket_descriptor);
#endif

 private:
  QString m_configuration;
  QTimer m_start_timer;

 private slots:
  void slot_start_timeout(void);

 signals:
#if QT_VERSION < 0x050000
  void new_connection(const int socket_descriptor,
		      const QHostAddress &address,
		      const quint16 port);
#else
  void new_connection(const qintptr socket_descriptor,
		      const QHostAddress &address,
		      const quint16 port);
#endif
};

#endif

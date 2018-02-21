/*
** Alexis Megas.
*/

#ifndef _spot_on_lite_daemon_h_
#define _spot_on_lite_daemon_h_

#include <QObject>
#include <QVector>

class QSocketNotifier;
class spot_on_lite_daemon_tcp_listener;

class spot_on_lite_daemon: public QObject
{
  Q_OBJECT

 public:
  spot_on_lite_daemon(const QString &configuration_file_name);
  spot_on_lite_daemon(void);
  ~spot_on_lite_daemon();
  QString child_process_file_name(void) const;
  QString child_process_ld_library_path(void) const;
  QString log_file_name(void) const;
  int maximum_accumulated_bytes(void) const;
  static spot_on_lite_daemon *instance(void);
  static void handler_signal(int signal_number);
  void log(const QString &error) const;
  void start(void);
  void validate_configuration_file(const QString &configuration_file_name,
				   bool *ok);

 private:
  QList<spot_on_lite_daemon_tcp_listener *> m_listeners;
  QSocketNotifier *m_signal_usr1_socket_notifier;
  QString m_configuration_file_name;
  QString m_child_process_file_name;
  QString m_child_process_ld_library_path;
  QString m_log_file_name;
  QVector<QString> m_listeners_properties;
  int m_maximum_accumulated_bytes;
  static int s_signal_usr1_fd[2];
  static spot_on_lite_daemon *s_instance;
  void prepare_listeners(void);
  void process_configuration_file(bool *ok);

 private slots:
  void slot_signal_usr1(void);
};

#endif

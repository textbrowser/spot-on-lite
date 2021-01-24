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

#ifndef _spot_on_lite_daemon_h_
#define _spot_on_lite_daemon_h_

#include <QAtomicInt>
#include <QFuture>
#include <QLocalServer>
#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QTimer>
#include <QVector>

class QSocketNotifier;

class spot_on_lite_daemon: public QObject
{
  Q_OBJECT

 public:
  spot_on_lite_daemon(const QString &configuration_file_name);
  spot_on_lite_daemon(void);
  ~spot_on_lite_daemon();
  QString certificates_file_name(void) const;
  QString child_process_file_name(void) const;
  QString child_process_ld_library_path(void) const;
  QString configuration_file_name(void) const;
  QString congestion_control_file_name(void) const;
  QString local_server_file_name(void) const;
  QString log_file_name(void) const;
  QString remote_identities_file_name(void) const;
  int maximum_accumulated_bytes(void) const;
  static void handler_signal(int signal_number);
  void log(const QString &error) const;
  void start(void);
  void validate_configuration_file
    (const QString &configuration_file_name, bool *ok);

 private:
  QAtomicInt m_congestion_control_lifetime;
  QFuture<void> m_congestion_control_future;
  QHash<QLocalSocket *, char> m_local_sockets;
  QHash<int, pid_t> m_peer_pids;
  QList<QObject *> m_listeners;
  QPointer<QLocalServer> m_local_server;
  QSocketNotifier *m_signal_socket_notifier;
  QString m_certificates_file_name;
  QString m_child_process_file_name;
  QString m_child_process_ld_library_path;
  QString m_configuration_file_name;
  QString m_congestion_control_file_name;
  QString m_local_socket_server_directory_name;
  QString m_log_file_name;
  QString m_remote_identities_file_name;
  QString m_statistics_file_name;
  QTimer m_congestion_control_timer;
  QTimer m_general_timer;
  QTimer m_peer_process_timer;
  QTimer m_start_timer;
  QVector<QString> m_listeners_properties;
  QVector<QString> m_peers_properties;
  int m_local_so_rcvbuf_so_sndbuf;
  int m_maximum_accumulated_bytes;
  static int s_signal_fd[2];
  size_t memory(void) const;
  void prepare_listeners(void);
  void prepare_local_socket_server(void);
  void prepare_peers(void);
  void process_configuration_file(bool *ok);
  void purge_congestion_control(void);

 private slots:
  void slot_general_timeout(void);
  void slot_local_socket_disconnected(void);
  void slot_new_local_connection(void);
  void slot_peer_process_timeout(void);
  void slot_purge_congestion_control_timeout(void);
  void slot_ready_read(void);
  void slot_signal(void);
  void slot_start_timeout(void);

 signals:
  void child_died(const pid_t pid);
};

#endif

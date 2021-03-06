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

#ifndef _spot_on_lite_daemon_udp_listener_h_
#define _spot_on_lite_daemon_udp_listener_h_

#include <QPointer>
#include <QTimer>
#include <QUdpSocket>

class spot_on_lite_daemon;
class spot_on_lite_daemon_child;

class spot_on_lite_daemon_udp_listener: public QUdpSocket
{
  Q_OBJECT

 public:
  spot_on_lite_daemon_udp_listener(const QString &configuration,
				   spot_on_lite_daemon *parent);
  ~spot_on_lite_daemon_udp_listener();

 private:
  QHash<QString, QPointer<spot_on_lite_daemon_child> > m_clients;
  QString m_configuration;
  QTimer m_general_timer;
  int m_max_pending_connections;
  spot_on_lite_daemon *m_parent;
  void new_connection(const QByteArray &data,
		      const QHostAddress &peer_address,
		      const quint16 peer_port);

 private slots:
  void slot_general_timeout(void);
  void slot_ready_read(void);
};

#endif

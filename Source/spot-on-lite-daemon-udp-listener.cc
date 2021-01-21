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
#include <sys/socket.h>
#include <unistd.h>
}

#include <QStringList>

#include "spot-on-lite-daemon-child.h"
#include "spot-on-lite-daemon-udp-listener.h"
#include "spot-on-lite-daemon.h"

spot_on_lite_daemon_udp_listener::spot_on_lite_daemon_udp_listener
(const QString &configuration, spot_on_lite_daemon *parent):QUdpSocket(parent)
{
  /*
  ** The configuration is assumed to be correct.
  */

  connect(&m_general_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_general_timeout(void)));
  connect(this,
	  SIGNAL(readyRead(void)),
	  this,
	  SLOT(slot_ready_read(void)));
  m_configuration = configuration;
  m_general_timer.start(5000);
  m_max_pending_connections = 30;
  m_parent = parent;
}

spot_on_lite_daemon_udp_listener::~spot_on_lite_daemon_udp_listener()
{
}

void spot_on_lite_daemon_udp_listener::new_connection
(const QByteArray &data,
 const QHostAddress &peer_address,
 const quint16 peer_port)
{
  if(!m_parent)
    return;

  auto sd = dup(static_cast<int> (socketDescriptor()));

  if(sd == -1)
    return;

  QPointer<spot_on_lite_daemon_child> client;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
  auto list(m_configuration.split(",", Qt::KeepEmptyParts));
#else
  auto list(m_configuration.split(",", QString::KeepEmptyParts));
#endif
  auto maximum_accumulated_bytes = m_parent->maximum_accumulated_bytes();
  auto optlen = static_cast<socklen_t> (sizeof(maximum_accumulated_bytes));
  auto server_identity
    (QString("%1:%2").arg(localAddress().toString()).arg(localPort()));

  setsockopt(sd, SOL_SOCKET, SO_RCVBUF, &maximum_accumulated_bytes, optlen);
  setsockopt(sd, SOL_SOCKET, SO_SNDBUF, &maximum_accumulated_bytes, optlen);
  client = new spot_on_lite_daemon_child
    (data,
     m_parent->certificates_file_name(),
     m_parent->configuration_file_name(),
     m_parent->congestion_control_file_name(),
     list.value(7),
     m_parent->local_server_file_name(),
     m_parent->log_file_name(),
     peer_address.toString(),
     peer_address.scopeId(),
     "udp",
     m_parent->remote_identities_file_name(),
     server_identity,
     list.value(3),
     list.value(9).toInt(),
     list.value(8).toInt(),
     m_parent->maximum_accumulated_bytes(),
     list.value(5).toInt(),
     list.value(6).toInt(),
     sd,
     list.value(4).toInt(),
     peer_port);
  m_clients[QString::number(peer_port) +
	    peer_address.scopeId() +
	    peer_address.toString()] = client;
}

void spot_on_lite_daemon_udp_listener::slot_general_timeout(void)
{
  QMutableHashIterator
    <QString, QPointer<spot_on_lite_daemon_child> > it(m_clients);

  while(it.hasNext())
    {
      it.next();

      if(!it.value())
	it.remove();
    }

  if(state() != QAbstractSocket::BoundState)
    {
      auto maximum_accumulated_bytes = m_parent ?
	m_parent->maximum_accumulated_bytes() : 8388608;
      auto optlen = static_cast<socklen_t> (sizeof(maximum_accumulated_bytes));
      auto sd = static_cast<int> (socketDescriptor());

      setsockopt(sd, SOL_SOCKET, SO_RCVBUF, &maximum_accumulated_bytes, optlen);
      setsockopt(sd, SOL_SOCKET, SO_SNDBUF, &maximum_accumulated_bytes, optlen);

      /*
      ** 0 - IP Address
      ** 1 - Port
      ** 2 - Backlog
      */

      auto flags = QUdpSocket::ReuseAddressHint | QUdpSocket::ShareAddress;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
      auto list(m_configuration.split(",", Qt::KeepEmptyParts));
#else
      auto list(m_configuration.split(",", QString::KeepEmptyParts));
#endif

      if(bind(QHostAddress(list.value(0)), list.value(1).toUShort(), flags))
	m_max_pending_connections = list.value(2).toInt();
    }
}

void spot_on_lite_daemon_udp_listener::slot_ready_read(void)
{
  while(hasPendingDatagrams())
    {
      QByteArray data;
      QHostAddress peer_address;
      auto size = qMax(static_cast<qint64> (0), pendingDatagramSize());
      quint16 peer_port = 0;

      data.resize(static_cast<int> (size));

      if(readDatagram(data.data(), size, &peer_address, &peer_port) <= 0)
	continue;

      if(m_clients.size() >= m_max_pending_connections)
	continue;
      else if(peer_address.isNull())
	continue;

      if(!m_clients.contains(QString::number(peer_port) +
			     peer_address.scopeId() +
			     peer_address.toString()))
	new_connection(data, peer_address, peer_port);
      else
	{
	  QPointer<spot_on_lite_daemon_child> client
	    (m_clients.value(QString::number(peer_port) +
			     peer_address.scopeId() +
			     peer_address.toString(),
			     nullptr));

	  if(client)
	    client->data_received(data, peer_address, peer_port);
	}
    }
}

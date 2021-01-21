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

#ifndef _spot_on_lite_monitor_h_
#define _spot_on_lite_monitor_h_

#include <QFuture>
#include <QTableWidget>
#include <QTimer>

class spot_on_lite_monitor_table: public QTableWidget
{
  Q_OBJECT

 public:
  spot_on_lite_monitor_table(QWidget *parent):QTableWidget(parent)
  {
  }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
  QModelIndex indexFromItem(const QTableWidgetItem *item) const
#else
  QModelIndex indexFromItem(QTableWidgetItem *item) const
#endif
  {
    return QTableWidget::indexFromItem(item);
  }

  QTableWidgetItem *itemFromIndex(const QModelIndex &index) const
  {
    return QTableWidget::itemFromIndex(index);
  }
};

#include "ui_spot-on-lite-monitor.h"

class spot_on_lite_monitor: public QMainWindow
{
  Q_OBJECT

 public:
  enum Columns
    {
     ARGUMENTS = 9,
     BYTES_ACCUMULATED = 5,
     BYTES_READ = 6,
     BYTES_WRITTEN = 7,
     IP_INFORMATION = 3,
     MEMORY = 4,
     NAME = 0,
     PID = 1,
     STATUS = 2,
     TYPE = 8,
     ZZZ = 999
    };

  spot_on_lite_monitor(void);
  ~spot_on_lite_monitor();

 private:
  QFuture<void> m_future;
  QMap<qint64, QTableWidgetItem *> m_pid_to_index;
  QTimer m_path_timer;
  Ui_spot_on_lite_monitor m_ui;
  void read_statistics_database(void);

 private slots:
  void slot_added(const QMap<Columns, QString> &values);
  void slot_changed(const QMap<Columns, QString> &values);
  void slot_deleted(const qint64 pid);
  void slot_path_timeout(void);
  void slot_quit(void);
  void slot_select_path(void);

 signals:
  void added(const QMap<Columns, QString> &values);
  void changed(const QMap<Columns, QString> &values);
  void deleted(const qint64 pid);
};

#endif

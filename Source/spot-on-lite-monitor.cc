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

#include <QApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QtConcurrent>

extern "C"
{
#include <signal.h>
}

#include "spot-on-lite-monitor.h"

static spot_on_lite_monitor::Columns field_name_to_column
(const QString &field_name)
{
  if(field_name == "arguments")
    return spot_on_lite_monitor::ARGUMENTS;
  else if(field_name == "bytes_accumulated")
    return spot_on_lite_monitor::BYTES_ACCUMULATED;
  else if(field_name == "bytes_read")
    return spot_on_lite_monitor::BYTES_READ;
  else if(field_name == "bytes_written")
    return spot_on_lite_monitor::BYTES_WRITTEN;
  else if(field_name == "ip_information")
    return spot_on_lite_monitor::IP_INFORMATION;
  else if(field_name == "memory")
    return spot_on_lite_monitor::MEMORY;
  else if(field_name == "name")
    return spot_on_lite_monitor::NAME;
  else if(field_name == "pid")
    return spot_on_lite_monitor::PID;
  else if(field_name == "status")
    return spot_on_lite_monitor::STATUS;
  else if(field_name == "type")
    return spot_on_lite_monitor::TYPE;
  else
    return spot_on_lite_monitor::ZZZ;
}

int main(int argc, char *argv[])
{
  Q_UNUSED(field_name_to_column);

  QApplication qapplication(argc, argv);
  auto rc = EXIT_SUCCESS;

  try
    {
      spot_on_lite_monitor spot_on_lite_monitor;

      spot_on_lite_monitor.show();
      rc = qapplication.exec();
    }
  catch(const std::bad_alloc &exception)
    {
      rc = EXIT_FAILURE;
    }
  catch(...)
    {
      rc = EXIT_FAILURE;
    }

  return rc;
}

spot_on_lite_monitor::spot_on_lite_monitor(void):QMainWindow()
{
  QDir home_dir(QDir::home());

  home_dir.mkdir(".spot-on-lite-monitor");
  qRegisterMetaType<QMap<Columns, QString> > ("QMap<Columns, QString>");
  m_future = QtConcurrent::run
    (this, &spot_on_lite_monitor::read_statistics_database);
  m_ui.setupUi(this);
  m_ui.processes->sortByColumn(PID, Qt::AscendingOrder);
  connect(m_ui.action_Quit,
	  SIGNAL(triggered(void)),
	  this,
	  SLOT(slot_quit(void)));
  connect(this,
	  SIGNAL(added(const QMap<Columns, QString> &)),
	  this,
	  SLOT(slot_added(const QMap<Columns, QString> &)));
  connect(this,
	  SIGNAL(changed(const QMap<Columns, QString> &)),
	  this,
	  SLOT(slot_changed(const QMap<Columns, QString> &)));
  connect(this,
	  SIGNAL(deleted(const qint64)),
	  this,
	  SLOT(slot_deleted(const qint64)));
}

spot_on_lite_monitor::~spot_on_lite_monitor()
{
  m_future.cancel();
  m_future.waitForFinished();
}

void spot_on_lite_monitor::read_statistics_database(void)
{
  QMap<qint64, QMap<Columns, QString> > processes;
  const QString db_connection_id("1");
  const QString db_path(QDir::tempPath() +
			QDir::separator() +
			"spot-on-lite-daemon-statistics.sqlite");

  while(true)
    {
      if(m_future.isCanceled())
	break;

      QThread::msleep(100);

      {
	auto db = QSqlDatabase::addDatabase("QSQLITE", db_connection_id);

	db.setDatabaseName(db_path);

	if(db.open())
	  {
	    QSet<qint64> deleted_processes(processes.keys().toSet());
	    QSqlQuery query(db);

	    query.setForwardOnly(true);

	    if(query.exec("SELECT arguments, "  // 0
			  "bytes_accumulated, " // 1
			  "bytes_read, "        // 2
			  "bytes_written, "     // 3
			  "ip_information, "    // 4
			  "memory, "            // 5
			  "name, "              // 6
			  "pid, "               // 7
			  "type "               // 8
			  "FROM statistics"))
	      while(query.next())
		{
		  QMap<Columns, QString> values;
		  QString status("Active");

		  values[ARGUMENTS] = query.value(0).toString();
		  values[BYTES_ACCUMULATED] = query.value(1).toString();
		  values[BYTES_READ] = query.value(2).toString();
		  values[BYTES_WRITTEN] = query.value(3).toString();
		  values[IP_INFORMATION] = query.value(4).toString();
		  values[MEMORY] = query.value(5).toString();
		  values[NAME] = query.value(6).toString();
		  values[PID] = query.value(7).toString();
		  values[TYPE] = query.value(8).toString();

		  auto pid = values.value(PID).toLongLong();

		  if(kill(static_cast<pid_t> (pid), 0) != 0)
		    status = "Dead";

		  values[STATUS] = status;
		  deleted_processes.remove(pid);

		  if(!processes.contains(pid))
		    {
		      if(!values.value(ARGUMENTS).isEmpty() &&
			 !values.value(NAME).isEmpty() &&
			 !values.value(TYPE).isEmpty())
			{
			  processes[pid] = values;
			  emit added(values);
			}
		    }
		  else if(processes.value(pid) != values)
		    {
		      emit changed(values);
		      processes[pid] = values;
		    }
		}

	    for(auto pid : deleted_processes)
	      emit deleted(pid);
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase(db_connection_id);
    }
}

void spot_on_lite_monitor::slot_added(const QMap<Columns, QString> &values)
{
  m_ui.processes->setSortingEnabled(false);
  m_ui.processes->setRowCount(m_ui.processes->rowCount() + 1);

  QMapIterator<Columns, QString> it(values);
  auto pid = values.value(PID).toLongLong();
  auto row = m_ui.processes->rowCount() - 1;

  while(it.hasNext())
    {
      it.next();

      auto item = new QTableWidgetItem(it.value());

      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      m_ui.processes->setItem(row, it.key(), item);

      if(!m_pid_to_index.contains(pid))
	m_pid_to_index[pid] = m_ui.processes->indexFromItem(item);
    }
  
  if(m_ui.processes->item(row, STATUS)->text() == "Active")
    m_ui.processes->item(row, STATUS)->setBackground
      (QBrush(QColor("lightgreen")));
  else
    m_ui.processes->item(row, STATUS)->setBackground
      (QBrush(QColor(240, 128, 128)));

  m_ui.processes->setSortingEnabled(true);
  statusBar()->showMessage
    (tr("%1 Process(es)").arg(m_ui.processes->rowCount()));
}

void spot_on_lite_monitor::slot_changed(const QMap<Columns, QString> &values)
{
  auto index = m_pid_to_index.value(values.value(PID).toLongLong());

  if(!index.isValid())
    return;

  m_ui.processes->setSortingEnabled(false);
  m_ui.processes->itemFromIndex(index.siblingAtColumn(BYTES_ACCUMULATED))->
    setText(values.value(BYTES_ACCUMULATED));
  m_ui.processes->itemFromIndex(index.siblingAtColumn(BYTES_READ))->
    setText(values.value(BYTES_READ));
  m_ui.processes->itemFromIndex(index.siblingAtColumn(BYTES_WRITTEN))->setText
    (values.value(BYTES_WRITTEN));
  m_ui.processes->itemFromIndex(index.siblingAtColumn(IP_INFORMATION))->setText
    (values.value(IP_INFORMATION));
  m_ui.processes->itemFromIndex(index.siblingAtColumn(MEMORY))->setText
    (values.value(MEMORY));
  m_ui.processes->itemFromIndex(index.siblingAtColumn(STATUS))->setText
    (values.value(STATUS));

  if(m_ui.processes->itemFromIndex(index.siblingAtColumn(STATUS))->
     text() == "Active")
    m_ui.processes->itemFromIndex(index.siblingAtColumn(STATUS))->setBackground
      (QBrush(QColor("lightgreen")));
  else
    m_ui.processes->itemFromIndex(index.siblingAtColumn(STATUS))->setBackground
      (QBrush(QColor(240, 128, 128)));

  m_ui.processes->setSortingEnabled(true);
}

void spot_on_lite_monitor::slot_deleted(const qint64 pid)
{
  auto index = m_pid_to_index.value(pid);

  m_pid_to_index.remove(pid);

  if(!index.isValid())
    return;

  m_ui.processes->removeRow(index.row());
  statusBar()->showMessage
    (tr("%1 Process(es)").arg(m_ui.processes->rowCount()));
}

void spot_on_lite_monitor::slot_quit(void)
{
  QApplication::instance()->quit();
}

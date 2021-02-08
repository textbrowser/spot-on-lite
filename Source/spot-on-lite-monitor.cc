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
#include <QFileDialog>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QtConcurrent>

#ifdef Q_OS_UNIX
extern "C"
{
#include <signal.h>
#include <sys/types.h>
}
#endif

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
  QDir home_dir(home_path());

  home_dir.mkdir(".spot-on-lite-monitor");
  qRegisterMetaType<QMap<Columns, QString> > ("QMap<Columns, QString>");
  m_daemon_pid = -1;
  m_future = QtConcurrent::run
    (this, &spot_on_lite_monitor::read_statistics_database);
  m_path_timer.start(1500);
  m_ui.setupUi(this);
  m_ui.processes->sortByColumn(PID, Qt::AscendingOrder);
  statusBar()->showMessage
    (tr("%1 Process(es)").arg(m_ui.processes->rowCount()));
  connect(&m_path_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_path_timeout(void)));
  connect(m_ui.action_Quit,
	  SIGNAL(triggered(void)),
	  this,
	  SLOT(slot_quit(void)));
  connect(m_ui.configuration_file,
	  SIGNAL(returnPressed(void)),
	  this,
	  SLOT(slot_set_path(void)));
  connect(m_ui.configuration_file_select,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_select_path(void)));
  connect(m_ui.launch_executable,
	  SIGNAL(returnPressed(void)),
	  this,
	  SLOT(slot_set_path(void)));
  connect(m_ui.launch_executable_file_select,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_select_path(void)));
  connect(m_ui.off_on,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_start_or_stop(void)));
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

  QSettings settings(ini_path(), QSettings::IniFormat);

  m_ui.configuration_file->setText
    (settings.value("configuration_file").toString().trimmed());

  if(m_ui.configuration_file->text().isEmpty())
    {
      QFileInfo file_info("/usr/local/spot-on-lite/spot-on-lite-daemon.conf");

      if(file_info.isReadable())
	m_ui.configuration_file->setText(file_info.absoluteFilePath());
      else
	{
	  file_info = QFileInfo("spot-on-lite-daemon.conf");

	  if(file_info.isReadable())
	    m_ui.configuration_file->setText(file_info.absoluteFilePath());
	}
    }

  m_ui.launch_executable->setText
    (settings.value("launch_executable").toString().trimmed());

  if(m_ui.launch_executable->text().isEmpty())
    {
      QFileInfo file_info("/usr/local/spot-on-lite/Spot-On-Lite-Daemon");

      if(file_info.isExecutable() && file_info.isReadable())
	m_ui.launch_executable->setText(file_info.absoluteFilePath());
      else
	{
	  file_info = QFileInfo("Spot-On-Lite-Daemon");

	  if(file_info.isExecutable() && file_info.isReadable())
	  m_ui.launch_executable->setText(file_info.absoluteFilePath());
	}
    }

  m_ui.processes->setFocus();
  restoreGeometry(settings.value("geometry").toByteArray());
  slot_deleted(m_daemon_pid);
  slot_path_timeout();
}

spot_on_lite_monitor::~spot_on_lite_monitor()
{
  m_future.cancel();
  m_future.waitForFinished();

  QSettings settings(ini_path(), QSettings::IniFormat);

  settings.setValue("geometry", saveGeometry());
}

QString spot_on_lite_monitor::home_path(void)
{
#ifdef Q_OS_UNIX
  return QDir::homePath();
#else
  return QDir::currentPath();
#endif
}

QString spot_on_lite_monitor::ini_path(void)
{
  return home_path() +
    QDir::separator() +
    ".spot-on-lite-monitor" +
    QDir::separator() +
    "Spot-On-Lite-Monitor.INI";
}

void spot_on_lite_monitor::read_statistics_database(void)
{
  QMap<qint64, QMap<Columns, QString> > processes;
  const QString db_connection_id("1");
  const auto db_path(QDir::tempPath() +
		     QDir::separator() +
		     "spot-on-lite-daemon-statistics.sqlite");

  while(true)
    {
      if(m_future.isCanceled())
	break;

      QThread::msleep(100);

      if(!QFileInfo(db_path).isReadable())
	{
	  for(auto pid : processes.keys())
	    emit deleted(pid);

	  continue;
	}

      {
	auto db = QSqlDatabase::addDatabase("QSQLITE", db_connection_id);

	db.setDatabaseName(db_path);

	if(db.open())
	  {
	    QSqlQuery query(db);
	    auto deleted_processes(processes.keys());

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

#ifdef Q_OS_UNIX
		  if(kill(static_cast<pid_t> (pid), 0) != 0)
		    status = "Dead";
#else
#endif

		  values[STATUS] = status;

		  auto index = deleted_processes.indexOf(pid);

		  if(index >= 0)
		    deleted_processes.removeAt(index);

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
	m_pid_to_index[pid] = item;
    }

  if(m_ui.processes->item(row, STATUS)->text() == "Active")
    m_ui.processes->item(row, STATUS)->setBackground
      (QBrush(QColor(144, 238, 144)));
  else
    m_ui.processes->item(row, STATUS)->setBackground
      (QBrush(QColor(240, 128, 128)));

  m_ui.processes->setSortingEnabled(true);
  statusBar()->showMessage
    (tr("%1 Process(es)").arg(m_ui.processes->rowCount()));

  if(values.value(STATUS) == "Active" && values.value(TYPE) == "daemon")
    {
      m_daemon_pid = static_cast<pid_t> (values.value(PID).toLongLong());
      m_ui.off_on->setChecked(true);
      m_ui.off_on->setStyleSheet
	("QToolButton {background-color: rgb(144, 238, 144);}");
      m_ui.off_on->setText(tr("Online"));
    }
}

void spot_on_lite_monitor::slot_changed(const QMap<Columns, QString> &values)
{
  auto item = m_pid_to_index.value(values.value(PID).toLongLong());

  if(!item)
    return;

  auto row = item->row();

  if(row < 0)
    return;

  m_ui.processes->setSortingEnabled(false);
  m_ui.processes->item(row, BYTES_ACCUMULATED)->setText
    (values.value(BYTES_ACCUMULATED));
  m_ui.processes->item(row, BYTES_READ)->setText(values.value(BYTES_READ));
  m_ui.processes->item(row, BYTES_WRITTEN)->setText
    (values.value(BYTES_WRITTEN));
  m_ui.processes->item(row, IP_INFORMATION)->setText
    (values.value(IP_INFORMATION));
  m_ui.processes->item(row, MEMORY)->setText(values.value(MEMORY));
  m_ui.processes->item(row, STATUS)->setText(values.value(STATUS));

  if(m_ui.processes->item(row, STATUS)->text() == "Active")
    m_ui.processes->item(row, STATUS)->setBackground
      (QBrush(QColor(144, 238, 144)));
  else
    m_ui.processes->item(row, STATUS)->setBackground
      (QBrush(QColor(240, 128, 128)));

  m_ui.processes->setSortingEnabled(true);
}

void spot_on_lite_monitor::slot_deleted(const qint64 pid)
{
  if(m_daemon_pid == pid)
    {
      m_daemon_pid = -1;
      m_ui.off_on->setChecked(false);
      m_ui.off_on->setStyleSheet
	("QToolButton {background-color: rgb(240, 128, 128);}");
      m_ui.off_on->setText(tr("Offline"));
    }

  auto item = m_pid_to_index.value(pid);

  m_pid_to_index.remove(pid);

  if(!item)
    return;

  m_ui.processes->removeRow(item->row());
  statusBar()->showMessage
    (tr("%1 Process(es)").arg(m_ui.processes->rowCount()));
}

void spot_on_lite_monitor::slot_path_timeout(void)
{
  QColor color(144, 238, 144); // Light green!
  QFileInfo file_info(m_ui.configuration_file->text());

  if(!file_info.isReadable())
    color = QColor(240, 128, 128);

  QPalette palette(m_ui.configuration_file->palette());

  palette.setColor(m_ui.configuration_file->backgroundRole(), color);
  m_ui.configuration_file->setPalette(palette);
  file_info = QFileInfo(m_ui.launch_executable->text());

  if(file_info.isExecutable() && file_info.isReadable())
    color = QColor(144, 238, 144);
  else
    color = QColor(240, 128, 128);

  palette = m_ui.launch_executable->palette();
  palette.setColor(m_ui.launch_executable->backgroundRole(), color);
  m_ui.launch_executable->setPalette(palette);
}

void spot_on_lite_monitor::slot_quit(void)
{
  QApplication::instance()->quit();
}

void spot_on_lite_monitor::slot_select_path(void)
{
  QFileDialog dialog(this);

  dialog.setAcceptMode(QFileDialog::AcceptOpen);
  dialog.setDirectory(QDir::homePath());
  dialog.setFileMode(QFileDialog::ExistingFile);
  dialog.setLabelText(QFileDialog::Accept, tr("Select"));
  dialog.setOption(QFileDialog::DontUseNativeDialog);

  if(m_ui.configuration_file_select == sender())
    dialog.setWindowTitle
      (tr("Spot-On-Lite Monitor: Configuration File Selection"));
  else
    dialog.setWindowTitle
      (tr("Spot-On-Lite Monitor: Launch Executable Selection"));

  if(dialog.exec() == QDialog::Accepted)
    {
      QApplication::processEvents();

      if(m_ui.configuration_file_select == sender())
	m_ui.configuration_file->setText(dialog.selectedFiles().value(0));
      else
	m_ui.launch_executable->setText(dialog.selectedFiles().value(0));

      QSettings settings(ini_path(), QSettings::IniFormat);

      settings.setValue
	("configuration_file", m_ui.configuration_file->text().trimmed());
      settings.setValue
	("launch_executable", m_ui.launch_executable->text().trimmed());
    }

  QApplication::processEvents();
}

void spot_on_lite_monitor::slot_set_path(void)
{
  auto lineedit = qobject_cast<QLineEdit *> (sender());

  if(lineedit)
    lineedit->selectAll();

  QSettings settings(ini_path(), QSettings::IniFormat);

  settings.setValue
    ("configuration_file", m_ui.configuration_file->text().trimmed());
  settings.setValue
    ("launch_executable", m_ui.launch_executable->text().trimmed());
}

void spot_on_lite_monitor::slot_start_or_stop(void)
{
  if(m_ui.off_on->isChecked())
    QProcess::startDetached
      (m_ui.launch_executable->text().trimmed(),
       QStringList() << "--configuration-file"
                     << m_ui.configuration_file->text().trimmed(),
       QDir::tempPath());
  else
    {
#ifdef Q_OS_UNIX
      if(m_daemon_pid > -1)
	::kill(m_daemon_pid, SIGTERM);
#else
#endif
    }
}

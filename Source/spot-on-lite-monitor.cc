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
#ifdef Q_OS_ANDROID
#if (QT_VERSION >= QT_VERSION_CHECK(6, 1, 0))
#include <QJniObject>
#endif
#endif
#include <QScrollBar>
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
    return spot_on_lite_monitor::Columns::ARGUMENTS;
  else if(field_name == "bytes_accumulated")
    return spot_on_lite_monitor::Columns::BYTES_ACCUMULATED;
  else if(field_name == "bytes_read")
    return spot_on_lite_monitor::Columns::BYTES_READ;
  else if(field_name == "bytes_written")
    return spot_on_lite_monitor::Columns::BYTES_WRITTEN;
  else if(field_name == "ip_information")
    return spot_on_lite_monitor::Columns::IP_INFORMATION;
  else if(field_name == "memory")
    return spot_on_lite_monitor::Columns::MEMORY;
  else if(field_name == "name")
    return spot_on_lite_monitor::Columns::NAME;
  else if(field_name == "pid")
    return spot_on_lite_monitor::Columns::PID;
  else if(field_name == "status")
    return spot_on_lite_monitor::Columns::STATUS;
  else if(field_name == "type")
    return spot_on_lite_monitor::Columns::TYPE;
  else
    return spot_on_lite_monitor::Columns::ZZZ;
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
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  m_future = QtConcurrent::run
    (this, &spot_on_lite_monitor::read_statistics_database);
#else
  m_future = QtConcurrent::run
    (&spot_on_lite_monitor::read_statistics_database, this);
#endif
  m_path_timer.start(1500);
  m_ui.setupUi(this);
  m_ui.processes->sortByColumn
    (static_cast<int> (Columns::PID), Qt::AscendingOrder);
  m_ui.release_notes->setSource(QUrl("qrc:/ReleaseNotes.html"));
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
  connect(m_ui.refresh,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_refresh_configuration_file(void)));
  connect(m_ui.save,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_save_configuration_file(void)));
  connect(this,
	  SIGNAL(added(const QMap<Columns, QString> &, const QString &)),
	  this,
	  SLOT(slot_added(const QMap<Columns, QString> &, const QString &)));
  connect(this,
	  SIGNAL(changed(const QMap<Columns, QString> &, const QString &)),
	  this,
	  SLOT(slot_changed(const QMap<Columns, QString> &, const QString &)));
  connect(this,
	  SIGNAL(deleted(const qint64)),
	  this,
	  SLOT(slot_deleted(const qint64)));

  QSettings settings(ini_path(), QSettings::IniFormat);

  m_ui.configuration_file->setText
    (settings.value("configuration_file").toString().trimmed());

  if(m_ui.configuration_file->text().isEmpty())
    {
      QFileInfo file_info("/opt/spot-on-lite/spot-on-lite-daemon.conf");

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
      QFileInfo file_info("/opt/spot-on-lite/Spot-On-Lite-Daemon");

      if(file_info.isExecutable() && file_info.isReadable())
	m_ui.launch_executable->setText(file_info.absoluteFilePath());
      else
	{
	  file_info = QFileInfo("Spot-On-Lite-Daemon");

	  if(file_info.isExecutable() && file_info.isReadable())
	  m_ui.launch_executable->setText(file_info.absoluteFilePath());
	}
    }

  m_ui.processes->resizeColumnsToContents();
  m_ui.processes->setFocus();
  prepare_icons();
  restoreGeometry(settings.value("geometry").toByteArray());
  slot_deleted(m_daemon_pid);
  slot_path_timeout();
  slot_refresh_configuration_file();
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

void spot_on_lite_monitor::prepare_icons(void)
{
  m_ui.action_Quit->setIcon(QIcon::fromTheme("application-exit"));
  m_ui.configuration_file_select->setIcon(QIcon::fromTheme("document-open"));
  m_ui.launch_executable_file_select->setIcon
    (QIcon::fromTheme("document-open"));
  m_ui.refresh->setIcon(QIcon::fromTheme("view-refresh"));
  m_ui.save->setIcon(QIcon::fromTheme("document-save"));
}

void spot_on_lite_monitor::read_statistics_database(void)
{
  QDateTime last_modified;
  QMap<qint64, QMap<Columns, QString> > processes;
  QString const db_connection_id("1");
  auto const db_path(QDir::tempPath() +
		     QDir::separator() +
		     "spot-on-lite-daemon-statistics.sqlite");

  while(true)
    {
      if(m_future.isCanceled())
	break;

      QThread::msleep(100);

      QFileInfo const file_info(db_path);

      if(!file_info.isReadable())
	{
	  foreach(auto pid, processes.keys())
	    emit deleted(pid);

	  last_modified = QDateTime();
	  continue;
	}

      if(file_info.lastModified() > last_modified)
	last_modified = file_info.lastModified();
      else
	continue;

      {
	auto db = QSqlDatabase::addDatabase("QSQLITE", db_connection_id);

	db.setDatabaseName(db_path);

	if(db.open())
	  {
	    QSqlQuery query(db);
	    QString tool_tip("");
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
		  auto status(tr("Active"));

		  values[Columns::ARGUMENTS] = query.value(0).toString();
		  values[Columns::BYTES_ACCUMULATED] =
		    query.value(1).toString();
		  values[Columns::BYTES_READ] = query.value(2).toString();
		  values[Columns::BYTES_WRITTEN] = query.value(3).toString();
		  values[Columns::IP_INFORMATION] = query.value(4).toString();
		  values[Columns::MEMORY] = query.value(5).toString();
		  values[Columns::NAME] = query.value(6).toString();
		  values[Columns::PID] = query.value(7).toString();
		  values[Columns::TYPE] = query.value(8).toString();

		  auto const pid = values.value(Columns::PID).toLongLong();

#ifdef Q_OS_UNIX
		  if(kill(static_cast<pid_t> (pid), 0) != 0)
		    status = tr("Dead");
#else
#endif
		  values[Columns::STATUS] = status;

		  auto const index = deleted_processes.indexOf(pid);

		  if(index >= 0)
		    deleted_processes.removeAt(index);

		  tool_tip = tr
		    ("<html>"
		     "<b>Name:</b> %1<br>"
		     "<b>PID:</b> %2<br>"
		     "<b>Status:</b> %3<br>"
		     "<b>IP Information:</b> %4<br>"
		     "<b>Memory:</b> %5<br>"
		     "<b>Bytes Accumulated:</b> %6<br>"
		     "<b>Bytes Read:</b> %7<br>"
		     "<b>Bytes Written:</b> %8<br>"
		     "<b>Type:</b> %9<br>"
		     "<b>Arguments:</b> %10"
		     "</html>").
		    arg(values.value(Columns::NAME)).
		    arg(values.value(Columns::PID)).
		    arg(values.value(Columns::STATUS)).
		    arg(values.value(Columns::IP_INFORMATION)).
		    arg(values.value(Columns::MEMORY)).
		    arg(values.value(Columns::BYTES_ACCUMULATED)).
		    arg(values.value(Columns::BYTES_READ)).
		    arg(values.value(Columns::BYTES_WRITTEN)).
		    arg(values.value(Columns::TYPE)).
		    arg(values.value(Columns::ARGUMENTS));

		  if(!processes.contains(pid))
		    {
		      if(!values.value(Columns::ARGUMENTS).isEmpty() &&
			 !values.value(Columns::NAME).isEmpty() &&
			 !values.value(Columns::TYPE).isEmpty())
			{
			  processes[pid] = values;
			  emit added(values, tool_tip);
			}
		    }
		  else if(processes.value(pid) != values)
		    {
		      emit changed(values, tool_tip);
		      processes[pid] = values;
		    }
		}

	    foreach(auto const pid, deleted_processes)
	      emit deleted(pid);
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase(db_connection_id);
    }
}

void spot_on_lite_monitor::show_message(const QString &text)
{
  if(text.trimmed().isEmpty())
    return;

  if(statusBar())
    statusBar()->showMessage(text.trimmed(), 5000);
}

void spot_on_lite_monitor::slot_added
(const QMap<Columns, QString> &values, const QString &tool_tip)
{
  m_ui.processes->setSortingEnabled(false);
  m_ui.processes->setRowCount(m_ui.processes->rowCount() + 1);

  QMapIterator<Columns, QString> it(values);
  auto const pid = values.value(Columns::PID).toLongLong();
  auto const row = m_ui.processes->rowCount() - 1;

  while(it.hasNext())
    {
      it.next();

      auto item = new QTableWidgetItem(it.value());

      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      item->setToolTip(tool_tip);
      m_ui.processes->setItem(row, static_cast<int> (it.key()), item);

      if(!m_pid_to_index.contains(pid))
	m_pid_to_index[pid] = item;
    }

  if(m_ui.processes->item(row, static_cast<int> (Columns::STATUS)) &&
     m_ui.processes->item(row, static_cast<int> (Columns::STATUS))->text() ==
     tr("Active"))
    m_ui.processes->item(row, static_cast<int> (Columns::STATUS))->
      setBackground(QBrush(QColor(144, 238, 144)));
  else if(m_ui.processes->item(row, static_cast<int> (Columns::STATUS)))
    m_ui.processes->item(row, static_cast<int> (Columns::STATUS))->
      setBackground(QBrush(QColor(240, 128, 128)));

  m_ui.processes->setSortingEnabled(true);
  statusBar()->showMessage
    (tr("%1 Process(es)").arg(m_ui.processes->rowCount()));

  if(values.value(Columns::STATUS) == tr("Active") &&
     values.value(Columns::TYPE) == "daemon")
    {
      m_daemon_pid = static_cast<pid_t>
	(values.value(Columns::PID).toLongLong());
      m_ui.off_on->setChecked(true);
      m_ui.off_on->setText(tr("Online"));
    }

  m_ui.processes->resizeColumnsToContents();
}

void spot_on_lite_monitor::slot_changed
(const QMap<Columns, QString> &values, const QString &tool_tip)
{
  auto item = m_pid_to_index.value(values.value(Columns::PID).toLongLong());

  if(!item)
    return;

  auto const row = item->row();

  if(row < 0)
    return;

  m_ui.processes->setSortingEnabled(false);

  QMapIterator<Columns, QString> it(values);

  while(it.hasNext())
    {
      it.next();

      auto item = m_ui.processes->item(row, static_cast<int> (it.key()));

      if(item)
	{
	  item->setText(it.value());
	  item->setToolTip(tool_tip);
	}
    }

  if(m_ui.processes->item(row, static_cast<int> (Columns::STATUS)) &&
     m_ui.processes->item(row, static_cast<int> (Columns::STATUS))->text() ==
     tr("Active"))
    m_ui.processes->item(row, static_cast<int> (Columns::STATUS))->
      setBackground(QBrush(QColor(144, 238, 144)));
  else if(m_ui.processes->item(row, static_cast<int> (Columns::STATUS)))
    m_ui.processes->item(row, static_cast<int> (Columns::STATUS))->
      setBackground(QBrush(QColor(240, 128, 128)));

  m_ui.processes->setSortingEnabled(true);
}

void spot_on_lite_monitor::slot_deleted(const qint64 pid)
{
  if(m_daemon_pid == pid)
    {
      m_daemon_pid = -1;
      m_ui.off_on->setChecked(false);
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

  auto palette(m_ui.configuration_file->palette());

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
#ifdef Q_OS_ANDROID
#if (QT_VERSION >= QT_VERSION_CHECK(6, 1, 0))
  auto activity = QJniObject(QNativeInterface::QAndroidApplication::context());

  activity.callMethod<void> ("finishAndRemoveTask");
#endif
#endif
}

void spot_on_lite_monitor::slot_refresh_configuration_file(void)
{
  QFile file(m_ui.configuration_file->text());

  if(file.open(QIODevice::ReadOnly))
    {
      auto const h = m_ui.configuration_file_contents->horizontalScrollBar()->
	value();
      auto const v = m_ui.configuration_file_contents->verticalScrollBar()->
	value();

      m_ui.configuration_file_contents->setPlainText(file.readAll());
      m_ui.configuration_file_contents->horizontalScrollBar()->setValue(h);
      m_ui.configuration_file_contents->verticalScrollBar()->setValue(v);
    }
  else
    show_message(tr("Cannot read the configuration file."));
}

void spot_on_lite_monitor::slot_save_configuration_file(void)
{
  if(m_ui.configuration_file->text().trimmed().isEmpty())
    {
      show_message(tr("Invalid configuration file name."));
      return;
    }

  QFile file(m_ui.configuration_file->text().trimmed());

  if(file.open(QIODevice::WriteOnly))
    {
      auto const text
	(m_ui.configuration_file_contents->toPlainText().trimmed().toUtf8());

      if(file.write(text) == static_cast<qint64> (text.length()))
	show_message
	  (tr("Contents saved in %1.").arg(file.fileName()));
      else
	show_message(tr("Incomplete save. QFile::write() failure."));
    }
  else
    show_message
      (tr("Cannot open %1 file for writing.").arg(file.fileName()));
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
	{
	  m_ui.configuration_file->setText(dialog.selectedFiles().value(0));
	  slot_refresh_configuration_file();
	}
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

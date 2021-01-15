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

#include "spot-on-lite-monitor.h"

int main(int argc, char *argv[])
{
  QApplication qapplication(argc, argv);
  int rc = EXIT_SUCCESS;

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
  m_ui.setupUi(this);
  m_future = QtConcurrent::run
    (this, &spot_on_lite_monitor::read_statistics_database);
  connect(this,
	  SIGNAL(new_text(const QString &)),
	  this,
	  SLOT(slot_new_text(const QString &)));
}

spot_on_lite_monitor::~spot_on_lite_monitor()
{
  m_future.cancel();
  m_future.waitForFinished();
}

void spot_on_lite_monitor::read_statistics_database(void)
{
  const QString columns("PID\t"
			"Name\t"
			"Type\t"
			"IP Information\t"
			"Bytes Accumulated\t"
			"Bytes Read\t"
			"Bytes Written\t"
			"Memory\t"
			"Arguments\n\n");
  const QString db_connection_id("1");
  const QString db_path(QDir::tempPath() +
			QDir::separator() +
			"spot-on-lite-daemon-statistics.sqlite");

  while(true)
    {
      if(m_future.isCanceled())
	break;

      QThread::sleep(100);

      {
	auto db = QSqlDatabase::addDatabase("QSQLITE", db_connection_id);

	db.setDatabaseName(db_path);

	if(db.open())
	  {
	    QSqlQuery query(db);
	    QString str("");

	    query.setForwardOnly(true);

	    if(query.exec("SELECT * FROM statistics"))
	      while(query.next())
		{
		}
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase(db_connection_id);
    }
}

void spot_on_lite_monitor::slot_new_text(const QString &text)
{
  m_ui.text->setPlainText(text);
}

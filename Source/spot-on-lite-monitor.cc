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

static spot_on_lite_monitor::Columns statistic_to_column
(const QString &statistic)
{
  if(statistic == "arguments")
    return spot_on_lite_monitor::ARGUMENTS;
  else if(statistic == "bytes_accumulated")
    return spot_on_lite_monitor::BYTES_ACCUMULATED;
  else if(statistic == "bytes_read")
    return spot_on_lite_monitor::BYTES_READ;
  else if(statistic == "bytes_written")
    return spot_on_lite_monitor::BYTES_WRITTEN;
  else if(statistic == "ip_information")
    return spot_on_lite_monitor::IP_INFORMATION;
  else if(statistic == "memory")
    return spot_on_lite_monitor::MEMORY;
  else if(statistic == "name")
    return spot_on_lite_monitor::NAME;
  else if(statistic == "pid")
    return spot_on_lite_monitor::PID;
  else if(statistic == "type")
    return spot_on_lite_monitor::TYPE;
  else
    return spot_on_lite_monitor::ZZZ;
}

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
}

spot_on_lite_monitor::~spot_on_lite_monitor()
{
  m_future.cancel();
  m_future.waitForFinished();
}

void spot_on_lite_monitor::read_statistics_database(void)
{
  const QString db_connection_id("1");

  while(true)
    {
      if(m_future.isCanceled())
	break;

      QThread::sleep(100);

      {
	auto db = QSqlDatabase::addDatabase("QSQLITE", db_connection_id);

	db.setDatabaseName(QDir::tempPath() +
			   QDir::separator() +
			   "spot-on-lite-daemon-statistics.sqlite");

	if(db.open())
	  {
	    QList<qint64> added;
	    QSqlQuery query(db);

	    query.setForwardOnly(true);

	    if(query.exec("SELECT pid, statistic, value FROM statistics"))
	      while(query.next())
		{
		  qint64 pid = query.value(0).toLongLong();

		  if(!m_cache.contains(pid))
		    added << pid;
		}
	  }

	db.close();
      }

      QSqlDatabase::removeDatabase(db_connection_id);
    }
}

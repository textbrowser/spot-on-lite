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

#ifndef _spot_on_lite_common_h_
#define _spot_on_lite_common_h_

extern "C"
{
#ifdef SPOTON_LITE_DAEMON_SCHEDULING_ENABLED
#include <sched.h>
#endif
}

#include <QSqlDatabase>
#include <QSqlQuery>

class spot_on_lite_common
{
 public:
  static int set_schedule(const QString &schedule)
  {
#ifdef SPOTON_LITE_DAEMON_SCHEDULING_ENABLED
    if(schedule.trimmed().isEmpty())
      return 0;

    QString policy("");
    auto list(schedule.trimmed().split(':'));
    int p = 0;

    policy = list.value(0).trimmed().toLower();

    if(policy == "batch")
      {
#ifdef Q_OS_LINUX
	p = SCHED_BATCH;
#else
	return -1;
#endif
      }
    else if(policy == "fifo")
      p = SCHED_FIFO;
    else if(policy == "idle")
      {
#ifdef Q_OS_LINUX
	p = SCHED_IDLE;
#else
	return -1;
#endif
      }
    else if(policy == "other")
      p = SCHED_OTHER;
    else if(policy == "rr")
      p = SCHED_RR;
    else
      return -1;

    auto const priority = qBound
      (sched_get_priority_min(p),
       list.value(1).trimmed().toInt(),
       sched_get_priority_max(p));
    pid_t pid = 0;
    struct sched_param parameters = {};

    parameters.sched_priority = priority;
    return sched_setscheduler(pid, p, &parameters);
#else
    Q_UNUSED(schedule);
    return 0;
#endif
  }

  static void save_statistic(const QList<QVariant> &list)
  {
    save_statistic(list.value(0).toString(),
		   list.value(1).toString(),
		   list.value(2).toString(),
		   list.value(3).toLongLong(),
		   list.value(4).toULongLong());
  }

  static void save_statistic(const QString &key,
			     const QString &file_name,
			     const QString &value,
			     const qint64 pid,
			     const quint64 db_connection_id)
  {
    {
      auto db = QSqlDatabase::addDatabase
	("QSQLITE", QString::number(db_connection_id));

      db.setDatabaseName(file_name);

      if(db.open())
	{
	  QSqlQuery query(db);

	  query.exec("CREATE TABLE IF NOT EXISTS statistics ("
		     "arguments TEXT, "
		     "bytes_accumulated TEXT, "
		     "bytes_read TEXT, "
		     "bytes_written TEXT, "
		     "ip_information TEXT, "
		     "memory TEXT, "
		     "name TEXT, "
		     "pid BIGINT NOT NULL PRIMARY KEY, "
		     "start_time BIGINT, "
		     "type TEXT)");
	  query.exec("PRAGMA journal_mode = OFF");
	  query.exec("PRAGMA synchronous = OFF");

	  if(key == "pid")
	    {
	      query.prepare
		("INSERT OR REPLACE INTO statistics (pid) VALUES (?)");
	      query.addBindValue(pid);
	    }
	  else
	    {
	      query.prepare
		(QString("UPDATE statistics SET %1 = ? WHERE pid = ?").
		 arg(key));
	      query.addBindValue(value);
	      query.addBindValue(pid);
	    }

	  query.exec();
	}

      db.close();
    }

    QSqlDatabase::removeDatabase(QString::number(db_connection_id));
  }

 private:
  spot_on_lite_common(void);
};

#endif

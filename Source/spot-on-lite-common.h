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

#include <QSqlDatabase>
#include <QSqlQuery>

class spot_on_lite_common
{
 public:
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

/*
    GSP for the tf blockchain game
    Copyright (C) 2019-2020  Autonomous Worlds Ltd

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "dbtest.hpp"

#include "moneysupply.hpp"
#include "globaldata.hpp"

#include "schema.hpp"

#include <glog/logging.h>

namespace pxd
{

TestDatabase::TestDatabase ()
  : db("test", SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_MEMORY)
{
  SetDatabase (db);
}

Database::IdT
TestDatabase::GetNextId ()
{
  return nextId++;
}

Database::IdT
TestDatabase::GetLogId ()
{
  return nextLogId++;
}

DBTestWithSchema::DBTestWithSchema ()
{
  LOG (INFO) << "Setting up game-state schema in test database...";
  SetupDatabaseSchema (*db);

  MoneySupply ms(db);
  ms.InitialiseDatabase ();
  
  GlobalData gd(db);
  gd.InitialiseDatabase ();   
}

TemporaryDatabaseChanges::TemporaryDatabaseChanges (Database& d,
                                                    benchmark::State& s)
  : db(d), benchmarkState(s)
{
  benchmarkState.PauseTiming ();
  auto stmt = db.Prepare ("SAVEPOINT `TemporaryDatabaseChanges`");
  stmt.Execute ();
  benchmarkState.ResumeTiming ();
}

TemporaryDatabaseChanges::~TemporaryDatabaseChanges ()
{
  benchmarkState.PauseTiming ();
  auto stmt = db.Prepare ("ROLLBACK TO `TemporaryDatabaseChanges`");
  stmt.Execute ();
  benchmarkState.ResumeTiming ();
}

} // namespace pxd

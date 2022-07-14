/*
    GSP for the Taurion blockchain game
    Copyright (C) 2020  Autonomous Worlds Ltd

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

#include "globaldata.hpp"

#include <glog/logging.h>

namespace pxd
{

namespace
{

struct GlobalDataResult : public Database::ResultType
{
  RESULT_COLUMN (int64_t, id, 1);    
  RESULT_COLUMN (int64_t, lasttournamenttime, 2);
  RESULT_COLUMN (int64_t, chimultiplier,3);
};

} // anonymous namespace

int64_t
GlobalData::GetLastTournamentTime ()
{
  auto stmt = db.Prepare (R"(
    SELECT *
      FROM `globaldata`
      WHERE `id` = 0
  )");


  auto res = stmt.Query<GlobalDataResult> ();
  CHECK (res.Step ());

  const int64_t lasttournamenttime = res.Get<GlobalDataResult::lasttournamenttime> ();
  CHECK (!res.Step ());

  return lasttournamenttime;
}

void GlobalData::SetLastTournamentTime(int64_t newTime)
{
  auto stmt = db.Prepare (R"(
    UPDATE `globaldata` SET
      (`lasttournamenttime`) = (?1) WHERE id=0
  )");

  stmt.Reset ();
  stmt.Bind (1, newTime);
  stmt.Execute ();
}

int64_t
GlobalData::GetChiMultiplier ()
{
  auto stmt = db.Prepare (R"(
    SELECT *
      FROM `globaldata`
      WHERE `id` = 0
  )");

  auto res = stmt.Query<GlobalDataResult> ();
  CHECK (res.Step ());

  const int64_t chimultiplier = res.Get<GlobalDataResult::chimultiplier> ();
  CHECK (!res.Step ());

  return chimultiplier;
}

void
GlobalData::InitialiseDatabase ()
{
  auto stmt = db.Prepare (R"(
    INSERT INTO `globaldata`
      (`id`, `lasttournamenttime`, `chimultiplier`) VALUES (?1, ?2, ?3)
  )");

  stmt.Reset ();
  stmt.Bind (1, 0);
  stmt.Bind (2, 0);
  stmt.Bind (3, 1000);
  stmt.Execute ();
    
}
} // namespace pxd

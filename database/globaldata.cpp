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
  RESULT_COLUMN (int64_t, chimultiplier,2);
  RESULT_COLUMN (std::string, version,3);
  RESULT_COLUMN (std::string, url,4);
  RESULT_COLUMN (std::string, queuedata,5);
};

/**
 * Fetches the single globaldata row (id=0) and steps onto it.  The caller
 * reads the column(s) it needs and then asserts there is no further row.
 */
Database::Result<GlobalDataResult>
FetchGlobalDataRow (Database& db)
{
  auto stmt = db.Prepare (R"(
    SELECT *
      FROM `globaldata`
      WHERE `id` = 0
  )");

  auto res = stmt.Query<GlobalDataResult> ();
  CHECK (res.Step ());

  return res;
}

/**
 * Updates a single column of the globaldata row (id=0) to the given value.
 */
template <typename T>
  void
  SetGlobalDataColumn (Database& db, const std::string& column, const T& value)
{
  auto stmt = db.Prepare (
      "UPDATE `globaldata` SET (`" + column + "`) = (?1) WHERE id=0");

  stmt.Reset ();
  stmt.Bind (1, value);
  stmt.Execute ();
}

} // anonymous namespace

int64_t
GlobalData::GetChiMultiplier ()
{
  auto res = FetchGlobalDataRow (db);

  const int64_t chimultiplier = res.Get<GlobalDataResult::chimultiplier> ();
  CHECK (!res.Step ());

  return chimultiplier;
}

void GlobalData::SetChiMultiplier(int64_t newMultiplier)
{
  SetGlobalDataColumn (db, "chimultiplier", newMultiplier);
}

std::string
GlobalData::GetVersion ()
{
  auto res = FetchGlobalDataRow (db);

  const std::string version = res.Get<GlobalDataResult::version> ();
  CHECK (!res.Step ());

  return version;
}

void GlobalData::SetVersion(std::string version)
{
  SetGlobalDataColumn (db, "version", version);
}

std::string
GlobalData::GetUrl ()
{
  auto res = FetchGlobalDataRow (db);

  const std::string url = res.Get<GlobalDataResult::url> ();
  CHECK (!res.Step ());

  return url;
}

void GlobalData::SetUrl(std::string url)
{
  SetGlobalDataColumn (db, "url", url);
}

std::string
GlobalData::GetQueueData ()
{
  auto res = FetchGlobalDataRow (db);

  const std::string queuedata = res.Get<GlobalDataResult::queuedata> ();
  CHECK (!res.Step ());

  return queuedata;
}

void GlobalData::SetQueueData(std::string queuedata)
{
  SetGlobalDataColumn (db, "queuedata", queuedata);
}

void
GlobalData::InitialiseDatabase ()
{
  auto stmt = db.Prepare (R"(
    INSERT INTO `globaldata`
      (`id`, `chimultiplier`, `version`, `url`, `queuedata`) VALUES (?1, ?2, ?3, ?4, ?5)
  )");

  std::string vd = "1.1.5";
  std::string ud = "xaya.io";
  std::string none = "";

  stmt.Reset ();
  stmt.Bind (1, 0);
  stmt.Bind (2, 1000);
  stmt.Bind (3, vd);
  stmt.Bind (4, ud);
  stmt.Bind (5, none);
  stmt.Execute ();
    
}
} // namespace pxd

/*
    GSP for the TF blockchain game
    Copyright (C) 2024  Autonomous Worlds Ltd

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

#include "ongoings.hpp"

#include <glog/logging.h>

namespace pxd
{

OngoingOperation::OngoingOperation (Database& d, const unsigned startHeight)
  : db(d), id(d.GetNextId ()), tracker(d.TrackHandle ("ongoing", id)),
    height(0), owner(""), dirtyFields(true)
{
  VLOG (1) << "Created new ongoing operation with ID " << id;
  data.SetToDefault ();
  data.Mutable ().set_startheight (startHeight);
}

OngoingOperation::OngoingOperation (Database& d,
                                    const Database::Result<OngoingResult>& res)
  : db(d), dirtyFields(false)
{
  id = res.Get<OngoingResult::id> ();
  tracker = d.TrackHandle ("ongoing", id);

  height = res.Get<OngoingResult::height> ();
  owner = res.Get<OngoingResult::owner> ();
  data = res.GetProto<OngoingResult::proto> ();

  VLOG (2) << "Fetched ongoing operation with ID " << id << " from database";
}

OngoingOperation::~OngoingOperation ()
{
  if (!dirtyFields && !data.IsDirty ())
    {
      VLOG (2) << "Ongoing operation " << id << " is not dirty, no update";
      return;
    }

  CHECK_NE (id, Database::EMPTY_ID);

  VLOG (2) << "Updating dirty ongoing operation " << id << " in the database";

  auto stmt = db.Prepare (R"(
    INSERT OR REPLACE INTO `ongoing_operations`
      (`id`, `height`, `owner`, `proto`)
      VALUES (?1, ?2, ?3, ?4)
  )");

  stmt.Bind (1, id);
  stmt.Bind (2, height);
  stmt.Bind (3, owner);
  stmt.BindProto (4, data);

  stmt.Execute ();
}

OngoingsTable::Handle
OngoingsTable::CreateNew (const unsigned startHeight)
{
  return Handle (new OngoingOperation (db, startHeight));
}

OngoingsTable::Handle
OngoingsTable::GetFromResult (const Database::Result<OngoingResult>& res)
{
  return Handle (new OngoingOperation (db, res));
}

OngoingsTable::Handle
OngoingsTable::GetFromResult (const Database::Result<OngoingResult>& res,
                             const RoConfig& cfg)
{
  return Handle (new OngoingOperation (db, res));
}

OngoingsTable::Handle
OngoingsTable::GetById (const Database::IdT id)
{
  auto stmt = db.Prepare (R"(
    SELECT * FROM `ongoing_operations` WHERE `id` = ?1
  )");
  stmt.Bind (1, id);
  auto res = stmt.Query<OngoingResult> ();
  if (!res.Step ())
    return nullptr;

  auto op = GetFromResult (res);
  CHECK (!res.Step ());
  return op;
}

Database::Result<OngoingResult>
OngoingsTable::QueryAll ()
{
  auto stmt = db.Prepare (R"(
    SELECT * FROM `ongoing_operations` ORDER BY `id`
  )");
  return stmt.Query<OngoingResult> ();
}

Database::Result<OngoingResult>
OngoingsTable::QueryForHeight (const unsigned h)
{
  auto stmt = db.Prepare (R"(
    SELECT * FROM `ongoing_operations` WHERE `height` <= ?1 ORDER BY `id`
  )");
  stmt.Bind (1, h);
  return stmt.Query<OngoingResult> ();
}

Database::Result<OngoingResult>
OngoingsTable::QueryForOwner (const std::string& owner)
{
  auto stmt = db.Prepare (R"(
    SELECT * FROM `ongoing_operations` WHERE `owner` = ?1 ORDER BY `id`
  )");
  stmt.Bind (1, owner);
  return stmt.Query<OngoingResult> ();
}

namespace
{

struct CountResult : public Database::ResultType
{
  RESULT_COLUMN (int64_t, cnt, 1);
};

} // anonymous namespace

unsigned
OngoingsTable::CountForOwner (const std::string& owner)
{
  auto stmt = db.Prepare (R"(
    SELECT COUNT(*) AS `cnt` FROM `ongoing_operations` WHERE `owner` = ?1
  )");
  stmt.Bind (1, owner);

  auto res = stmt.Query<CountResult> ();
  CHECK (res.Step ());
  const unsigned count = res.Get<CountResult::cnt> ();
  CHECK (!res.Step ());

  return count;
}

void
OngoingsTable::DeleteForHeight (const unsigned h)
{
  auto stmt = db.Prepare (R"(
    DELETE FROM `ongoing_operations` WHERE `height` = ?1
  )");
  stmt.Bind (1, h);
  stmt.Execute ();
}

void
OngoingsTable::DeleteForOwner (const std::string& owner)
{
  auto stmt = db.Prepare (R"(
    DELETE FROM `ongoing_operations` WHERE `owner` = ?1
  )");
  stmt.Bind (1, owner);
  stmt.Execute ();
}

void
OngoingsTable::DeleteById (const Database::IdT id)
{
  auto stmt = db.Prepare (R"(
    DELETE FROM `ongoing_operations` WHERE `id` = ?1
  )");
  stmt.Bind (1, id);
  stmt.Execute ();
}

} // namespace pxd

/*
    GSP for the TF blockchain game
    Copyright (C) 2019-2021  Autonomous Worlds Ltd

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

#include "reward.hpp"

#include <xayautil/random.hpp>

#include <glog/logging.h>

namespace pxd
{

RewardInstance::RewardInstance (Database& d, const std::string& o)
  : id(d.GetNextId ()),
    tracker(d.TrackHandle ("reward", id)),
    dirtyFields(true),
    owner(o),    
    isNew(true),
    db(d)
{
  VLOG (1)
      << "Created new reward with ID " << id << ": "
      << "owner=" << o;
 
  data.SetToDefault ();
  Validate ();
}

RewardInstance::RewardInstance (Database& d, const Database::Result<RewardInstanceResult>& res)
  : dirtyFields(false), isNew(false), db(d)
{
  id = res.Get<RewardInstanceResult::id> ();
  tracker = db.TrackHandle ("reward", id);
  owner = res.Get<RewardInstanceResult::owner> ();

  data = res.GetProto<RewardInstanceResult::proto> ();

  VLOG (2) << "Fetched reward with ID " << id << " from database result";
  Validate ();
}

RewardInstance::~RewardInstance ()
{
  Validate ();

  if (isNew || data.IsDirty () || dirtyFields)
  {
      VLOG (2)
          << "Reward " << id
          << " has been modified including proto data, updating DB";
      auto stmt = db.Prepare (R"(
        INSERT OR REPLACE INTO `rewards`
          (`id`,`owner`, `proto`)
          VALUES
          (?1,
           ?2,
           ?3)
      )");

      BindFieldValues (stmt);
      stmt.BindProto (3, data);
      stmt.Execute ();

      return;
  }
	
  VLOG (2) << "Reward " << id << " is not dirty, no update";
}

void
RewardInstance::Validate () const
{
  CHECK_NE (id, Database::EMPTY_ID);
}

void
RewardInstance::BindFieldValues (Database::Statement& stmt) const
{
  stmt.Bind (1, id);
  stmt.Bind (2, owner);
}

RewardsTable::Handle
RewardsTable::CreateNew (const std::string& owner)
{
  return Handle (new RewardInstance (db, owner));
}

RewardsTable::Handle
RewardsTable::GetFromResult (const Database::Result<RewardInstanceResult>& res)
{
  return Handle (new RewardInstance (db, res));
}

RewardsTable::Handle
RewardsTable::GetFromResult (const Database::Result<RewardInstanceResult>& res, const RoConfig& cfg)
{
  return Handle (new RewardInstance (db, res));
}

RewardsTable::Handle
RewardsTable::GetById (const Database::IdT id)
{
  auto stmt = db.Prepare ("SELECT * FROM `rewards` WHERE `id` = ?1");
  stmt.Bind (1, id);
  auto res = stmt.Query<RewardInstanceResult> ();
  if (!res.Step ())
    return nullptr;

  auto c = GetFromResult (res);
  CHECK (!res.Step ());
  return c;
}

Database::Result<RewardInstanceResult>
RewardsTable::QueryAll ()
{
  auto stmt = db.Prepare ("SELECT * FROM `rewards` ORDER BY `id`");
  return stmt.Query<RewardInstanceResult> ();
}

Database::Result<RewardInstanceResult>
RewardsTable::QueryForOwner (const std::string& owner)
{
  auto stmt = db.Prepare (R"(
    SELECT * FROM `rewards` WHERE `owner` = ?1 ORDER BY `id`
  )");
  stmt.Bind (1, owner);
  return stmt.Query<RewardInstanceResult> ();
}

namespace
{

struct CountResult : public Database::ResultType
{
  RESULT_COLUMN (int64_t, cnt, 1);
};

} // anonymous namespace

unsigned
RewardsTable::CountForOwner (const std::string& owner)
{
  auto stmt = db.Prepare (R"(
    SELECT COUNT(*) AS `cnt`
      FROM `rewards`
      WHERE `owner` = ?1
      ORDER BY `id`
  )");
  stmt.Bind (1, owner);

  auto res = stmt.Query<CountResult> ();
  CHECK (res.Step ());
  const unsigned count = res.Get<CountResult::cnt> ();
  CHECK (!res.Step ());

  return count;
}

void
RewardsTable::DeleteById (const Database::IdT id)
{
  VLOG (1) << "Deleting rewards with ID " << id;

  auto stmt = db.Prepare (R"(
    DELETE FROM `rewards` WHERE `id` = ?1
  )");
  stmt.Bind (1, id);
  stmt.Execute ();
}


} // namespace pxd
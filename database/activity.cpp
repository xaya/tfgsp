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

#include "activity.hpp"

#include <glog/logging.h>

namespace pxd
{

ActivityInstance::ActivityInstance (Database& d, const std::string& o, const uint32_t start_block, const uint32_t duration, const std::string& name, const std::string& related_item_guid, Database::IdT related_item_or_class_id)
  : id(d.GetNextId ()),
    tracker(d.TrackHandle ("activity", id)),
    dirtyFields(true),
    owner(o),    
    isNew(true),
    db(d)
{
  VLOG (1)
      << "Created new activity with ID " << id << ": "
      << "owner=" << o;
 
  data.SetToDefault ();
  MutableProto ().set_startblock(start_block);
  MutableProto ().set_duration(duration);
  MutableProto ().set_name(name);
  MutableProto ().set_relateditemguid(related_item_guid);
  MutableProto ().set_relateditemorclassid(related_item_or_class_id);
  
  Validate ();
}

ActivityInstance::ActivityInstance (Database& d, const Database::Result<ActivityResult>& res)
  : dirtyFields(false), isNew(false), db(d)
{
  id = res.Get<ActivityResult::id> ();
  tracker = db.TrackHandle ("activity", id);
  owner = res.Get<ActivityResult::owner> ();

  data = res.GetProto<ActivityResult::proto> ();

  VLOG (2) << "Fetched activity with ID " << id << " from database result";
  Validate ();
}

ActivityInstance::~ActivityInstance ()
{
  Validate ();

  if (isNew || data.IsDirty () || dirtyFields)
  {
      VLOG (2)
          << "Activity " << id
          << " has been modified including proto data, updating DB";
      auto stmt = db.Prepare (R"(
        INSERT OR REPLACE INTO `activities`
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
	
  VLOG (2) << "Activity " << id << " is not dirty, no update";
}

void
ActivityInstance::Validate () const
{
  CHECK_NE (id, Database::EMPTY_ID);
}

void
ActivityInstance::BindFieldValues (Database::Statement& stmt) const
{
  stmt.Bind (1, id);
  stmt.Bind (2, owner);
}

ActivityTable::Handle
ActivityTable::CreateNew (const std::string& owner, const uint32_t start_block, const uint32_t duration, const std::string& name, const std::string& related_item_guid, Database::IdT related_item_or_class_id)
{
  return Handle (new ActivityInstance (db, owner, start_block, duration, name, related_item_guid, related_item_or_class_id));
}

ActivityTable::Handle
ActivityTable::GetFromResult (const Database::Result<ActivityResult>& res, const RoConfig& cfg)
{
  return Handle (new ActivityInstance (db, res));
}

ActivityTable::Handle
ActivityTable::GetById (const Database::IdT id, const RoConfig& cfg)
{
  auto stmt = db.Prepare ("SELECT * FROM `activities` WHERE `id` = ?1");
  stmt.Bind (1, id);
  auto res = stmt.Query<ActivityResult> ();
  if (!res.Step ())
    return nullptr;

  auto c = GetFromResult (res, cfg);
  CHECK (!res.Step ());
  return c;
}

Database::Result<ActivityResult>
ActivityTable::QueryAll ()
{
  auto stmt = db.Prepare ("SELECT * FROM `activities` ORDER BY `id`");
  return stmt.Query<ActivityResult> ();
}

Database::Result<ActivityResult>
ActivityTable::QueryForOwner (const std::string& owner)
{
  auto stmt = db.Prepare (R"(
    SELECT * FROM `activities` WHERE `owner` = ?1 ORDER BY `id`
  )");
  stmt.Bind (1, owner);
  return stmt.Query<ActivityResult> ();
}

void
ActivityTable::DeleteById (const Database::IdT id)
{
  VLOG (1) << "Deleting activity with ID " << id;

  auto stmt = db.Prepare (R"(
    DELETE FROM `activities` WHERE `id` = ?1
  )");
  stmt.Bind (1, id);
  stmt.Execute ();
}


} // namespace pxd
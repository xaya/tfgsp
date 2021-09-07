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

#include "recipe.hpp"

#include <glog/logging.h>

namespace pxd
{

RecipeInstance::RecipeInstance (Database& d, const std::string& o, const std::string& cr)
  : id(d.GetNextId ()),
    tracker(d.TrackHandle ("recipe", id)),  
	dirtyFields(true),
    owner(o),	
	isNew(true),
	db(d)
{
  VLOG (1)
      << "Created new recipe instance with ID " << id << ": "
      << "crafter recipe=" << cr;
 
  data.SetToDefault ();
  Validate ();
}

RecipeInstance::RecipeInstance (Database& d, const Database::Result<RecipeInstanceResult>& res)
  : dirtyFields(false), isNew(false), db(d)
{
  id = res.Get<RecipeInstanceResult::id> ();
  tracker = d.TrackHandle ("recepie", id);
  owner = res.Get<RecipeInstanceResult::owner> ();

  data = res.GetProto<RecipeInstanceResult::proto> ();

  VLOG (2) << "Fetched recipe instance with ID " << id << " from database result";
  Validate ();
}

RecipeInstance::~RecipeInstance ()
{
  Validate ();

  if (isNew || data.IsDirty () || dirtyFields)
  {
      VLOG (2)
          << "Recipe Instance " << id
          << " has been modified including proto data, updating DB";
      auto stmt = db.Prepare (R"(
        INSERT OR REPLACE INTO `recepies`
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
	
  VLOG (2) << "Recipe Instance " << id << " is not dirty, no update";
}

void
RecipeInstance::Validate () const
{
  CHECK_NE (id, Database::EMPTY_ID);
}

void
RecipeInstance::BindFieldValues (Database::Statement& stmt) const
{
  stmt.Bind (1, id);
  stmt.Bind (2, owner);
}

RecipeInstanceTable::Handle
RecipeInstanceTable::CreateNew (const std::string& owner, const std::string& cr)
{
  return Handle (new RecipeInstance (db, owner, cr));
}

RecipeInstanceTable::Handle
RecipeInstanceTable::GetFromResult (const Database::Result<RecipeInstanceResult>& res)
{
  return Handle (new RecipeInstance (db, res));
}

RecipeInstanceTable::Handle
RecipeInstanceTable::GetById (const Database::IdT id)
{
  auto stmt = db.Prepare ("SELECT * FROM `recepies` WHERE `id` = ?1");
  stmt.Bind (1, id);
  auto res = stmt.Query<RecipeInstanceResult> ();
  if (!res.Step ())
    return nullptr;

  auto c = GetFromResult (res);
  CHECK (!res.Step ());
  return c;
}

Database::Result<RecipeInstanceResult>
RecipeInstanceTable::QueryAll ()
{
  auto stmt = db.Prepare ("SELECT * FROM `recepies` ORDER BY `id`");
  return stmt.Query<RecipeInstanceResult> ();
}

Database::Result<RecipeInstanceResult>
RecipeInstanceTable::QueryForOwner (const std::string& owner)
{
  auto stmt = db.Prepare (R"(
    SELECT * FROM `recepies` WHERE `owner` = ?1 ORDER BY `id`
  )");
  stmt.Bind (1, owner);
  return stmt.Query<RecipeInstanceResult> ();
}

void
RecipeInstanceTable::DeleteById (const Database::IdT id)
{
  VLOG (1) << "Deleting recepie with ID " << id;

  auto stmt = db.Prepare (R"(
    DELETE FROM `recepies` WHERE `id` = ?1
  )");
  stmt.Bind (1, id);
  stmt.Execute ();
}


} // namespace pxd
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

#include "tournament.hpp"

#include <glog/logging.h>

namespace pxd
{

TournamentInstance::TournamentInstance (Database& d, const std::string& blueprintGuid)
  : id(d.GetNextId ()),
    tracker(d.TrackHandle ("tournament", id)),
    dirtyFields(true), 
    isNew(true),
    db(d)
{
  VLOG (1)
      << "Created new tournament with ID " << id << ": "
      << "guid=" << blueprintGuid;
 
  data.SetToDefault ();
  Validate ();
}

TournamentInstance::TournamentInstance (Database& d, const Database::Result<TournamentResult>& res)
  : dirtyFields(false), isNew(false), db(d)
{
  id = res.Get<TournamentResult::id> ();
  tracker = db.TrackHandle ("tournament", id);

  data = res.GetProto<TournamentResult::proto> ();

  VLOG (2) << "Fetched tournament with ID " << id << " from database result";
  Validate ();
}

TournamentInstance::~TournamentInstance ()
{
  Validate ();

  if (isNew || data.IsDirty () || dirtyFields)
  {
      VLOG (2)
          << "Tournament " << id
          << " has been modified including proto data, updating DB";
      auto stmt = db.Prepare (R"(
        INSERT OR REPLACE INTO `tournaments`
          (`id`,`proto`,)
          VALUES
          (?1,
           ?2)
      )");

      BindFieldValues (stmt);
      stmt.BindProto (2, data);
      stmt.Execute ();

      return;
  }
	
  VLOG (2) << "Tournament " << id << " is not dirty, no update";
}

void
TournamentInstance::Validate () const
{
  CHECK_NE (id, Database::EMPTY_ID);
}

void
TournamentInstance::BindFieldValues (Database::Statement& stmt) const
{
  stmt.Bind (1, id);
}

TournamentTable::Handle
TournamentTable::CreateNew (const std::string& blueprintGuid)
{
  return Handle (new TournamentInstance (db, blueprintGuid));
}

TournamentTable::Handle
TournamentTable::GetFromResult (const Database::Result<TournamentResult>& res)
{
  return Handle (new TournamentInstance (db, res));
}

TournamentTable::Handle
TournamentTable::GetById (const Database::IdT id)
{
  auto stmt = db.Prepare ("SELECT * FROM `tournaments` WHERE `id` = ?1");
  stmt.Bind (1, id);
  auto res = stmt.Query<TournamentResult> ();
  if (!res.Step ())
    return nullptr;

  auto c = GetFromResult (res);
  CHECK (!res.Step ());
  return c;
}

Database::Result<TournamentResult>
TournamentTable::QueryAll ()
{
  auto stmt = db.Prepare ("SELECT * FROM `tournaments` ORDER BY `id`");
  return stmt.Query<TournamentResult> ();
}

void
TournamentTable::DeleteById (const Database::IdT id)
{
  VLOG (1) << "Deleting tournament with ID " << id;

  auto stmt = db.Prepare (R"(
    DELETE FROM `tournaments` WHERE `id` = ?1
  )");
  stmt.Bind (1, id);
  stmt.Execute ();
}


} // namespace pxd
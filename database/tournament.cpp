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

TournamentInstance::TournamentInstance (Database& d, const std::string& blueprintGuid, const RoConfig& cfg)
  : id(d.GetNextId ()),
    tracker(d.TrackHandle ("tournament", id)),
    dirtyFields(true), 
    isNew(true),
    db(d)
{
  VLOG (1)
      << "Created new tournament with ID " << id << ": "
      << "guid=" << blueprintGuid;
 
  instance.SetToDefault ();
  data.SetToDefault ();
  
  const auto& tournamentBlueprintList = cfg->tournamentbluprints();
  
  for(auto& tournamentBluprint: tournamentBlueprintList)
  {
      if(tournamentBluprint.second.authoredid() == blueprintGuid)
      {
          MutableProto().set_authoredid(tournamentBluprint.second.authoredid());
          MutableProto().set_name(tournamentBluprint.second.name());
          MutableProto().set_duration(tournamentBluprint.second.duration());
          MutableProto().set_teamcount(tournamentBluprint.second.teamcount());
          MutableProto().set_teamsize(tournamentBluprint.second.teamsize());
          MutableProto().set_minsweetness(tournamentBluprint.second.minsweetness());
          MutableProto().set_maxsweetness(tournamentBluprint.second.maxsweetness());
          MutableProto().set_maxrewardquality(tournamentBluprint.second.maxrewardquality());
          MutableProto().set_baserewardstableid(tournamentBluprint.second.baserewardstableid());
          MutableProto().set_baserollcount(tournamentBluprint.second.baserollcount());
          MutableProto().set_winnerrewardstableid(tournamentBluprint.second.winnerrewardstableid());
          MutableProto().set_winnerrollcount(tournamentBluprint.second.winnerrollcount());
          MutableProto().set_joincost(tournamentBluprint.second.joincost());
          MutableInstance().set_blocksleft(tournamentBluprint.second.duration());
          
          name = blueprintGuid;
      }
  }
  
  MutableInstance().set_state((int)pxd::TournamentState::Listed);
  Validate ();
}

TournamentInstance::TournamentInstance (Database& d, const Database::Result<TournamentResult>& res)
  : dirtyFields(false), isNew(false), db(d)
{
  id = res.Get<TournamentResult::id> ();
  name = res.Get<TournamentResult::name> ();
  
  tracker = db.TrackHandle ("tournament", id);

  if (res.IsNull<TournamentResult::instance> ())
    instance.SetToDefault ();
  else
    instance = res.GetProto<TournamentResult::instance> ();

  data = res.GetProto<TournamentResult::proto> ();

  VLOG (2) << "Fetched tournament with ID " << id << " from database result";
  Validate ();
}

TournamentInstance::~TournamentInstance ()
{
  Validate ();

  if (isNew || data.IsDirty () || instance.IsDirty () || dirtyFields)
  {
      VLOG (2)
          << "Tournament " << id
          << " has been modified including proto data, updating DB";
      auto stmt = db.Prepare (R"(
        INSERT OR REPLACE INTO `tournaments`
          (`id`, `name`, `proto`, `instance`)
          VALUES
          (?1,
           ?2,
           ?3,
           ?4)
      )");

      BindFieldValues (stmt);
      stmt.BindProto (3, data);
      stmt.BindProto (4, instance);
    
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
  stmt.Bind (2, name);
}

TournamentTable::Handle
TournamentTable::CreateNew (const std::string& blueprintGuid, const RoConfig& cfg)
{
  return Handle (new TournamentInstance (db, blueprintGuid, cfg));
}

TournamentTable::Handle
TournamentTable::GetFromResult (const Database::Result<TournamentResult>& res, const RoConfig& cfg)
{
  return Handle (new TournamentInstance (db, res));
}

TournamentTable::Handle
TournamentTable::GetByAuthIdName (const std::string authIdName, const RoConfig& cfg)
{
  auto stmt = db.Prepare ("SELECT * FROM `tournaments` WHERE `name` = ?1");
  stmt.Bind (1, authIdName);
  auto res = stmt.Query<TournamentResult> ();
  if (!res.Step ())
    return nullptr;

  auto c = GetFromResult (res, cfg);
  CHECK (!res.Step ());
  return c;
}

TournamentTable::Handle
TournamentTable::GetById (const Database::IdT id, const RoConfig& cfg)
{
  auto stmt = db.Prepare ("SELECT * FROM `tournaments` WHERE `id` = ?1");
  stmt.Bind (1, id);
  auto res = stmt.Query<TournamentResult> ();
  if (!res.Step ())
    return nullptr;

  auto c = GetFromResult (res, cfg);
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
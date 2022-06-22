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

#include "specialtournament.hpp"
#include <glog/logging.h>

namespace pxd
{

SpecialTournamentInstance::SpecialTournamentInstance (Database& d, const int32_t& _tier, const RoConfig& cfg)
  : id(d.GetNextId ()),
    tracker(d.TrackHandle ("specialtournament", id)),
    dirtyFields(true), 
    isNew(true),
    db(d)
{
  VLOG (1)
      << "Created new special tournament with ID " << id << ": "
      << "tier=" << tier;
 
  data.SetToDefault ();
  

  tier = _tier;
  MutableProto().set_tier(_tier);
  MutableProto().set_crownholder("ryumaster");
  MutableProto().set_state((int)pxd::SpecialTournamentState::Listed);
  Validate ();
}

SpecialTournamentInstance::SpecialTournamentInstance (Database& d, const Database::Result<SpecialTournamentResult>& res)
  : dirtyFields(false), isNew(false), db(d)
{
  id = res.Get<SpecialTournamentResult::id> ();
  tier = res.Get<SpecialTournamentResult::tier> ();  
  tracker = db.TrackHandle ("specialtournament", id);
  data = res.GetProto<SpecialTournamentResult::proto> ();

  VLOG (2) << "Fetched special tournament with ID " << id << " from database result";
  Validate ();
}

SpecialTournamentInstance::~SpecialTournamentInstance ()
{
  Validate ();

  if (isNew || data.IsDirty () || dirtyFields)
  {
      VLOG (2)
          << "Special Tournament " << id
          << " has been modified including proto data, updating DB";
      auto stmt = db.Prepare (R"(
        INSERT OR REPLACE INTO `specialtournaments`
          (`id`, `tier`, `proto`)
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
	
  VLOG (2) << "Tournament " << id << " is not dirty, no update";
}

void
SpecialTournamentInstance::Validate () const
{
  CHECK_NE (id, Database::EMPTY_ID);
}

void
SpecialTournamentInstance::BindFieldValues (Database::Statement& stmt) const
{
  stmt.Bind (1, id);
  stmt.Bind (2, tier);
}

SpecialTournamentTable::Handle
SpecialTournamentTable::CreateNew (const int32_t& _tier, const RoConfig& cfg)
{
  return Handle (new SpecialTournamentInstance (db, _tier, cfg));
}

SpecialTournamentTable::Handle
SpecialTournamentTable::GetFromResult (const Database::Result<SpecialTournamentResult>& res, const RoConfig& cfg)
{
  return Handle (new SpecialTournamentInstance (db, res));
}

SpecialTournamentTable::Handle
SpecialTournamentTable::GetByTier (const int32_t _tier, const RoConfig& cfg)
{
  auto stmt = db.Prepare ("SELECT * FROM `specialtournaments` WHERE `tier` = ?1");
  stmt.Bind (1, _tier);
  auto res = stmt.Query<SpecialTournamentResult> ();
  if (!res.Step ())
    return nullptr;

  auto c = GetFromResult (res, cfg);
  CHECK (!res.Step ());
  return c;
}

SpecialTournamentTable::Handle
SpecialTournamentTable::GetById (const Database::IdT id, const RoConfig& cfg)
{
  auto stmt = db.Prepare ("SELECT * FROM `specialtournaments` WHERE `id` = ?1");
  stmt.Bind (1, id);
  auto res = stmt.Query<SpecialTournamentResult> ();
  if (!res.Step ())
    return nullptr;

  auto c = GetFromResult (res, cfg);
  CHECK (!res.Step ());
  return c;
}

Database::Result<SpecialTournamentResult>
SpecialTournamentTable::QueryAll ()
{
  auto stmt = db.Prepare ("SELECT * FROM `specialtournaments` ORDER BY `id`");
  return stmt.Query<SpecialTournamentResult> ();
}


} // namespace pxd
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

#include <string>
#include <vector>

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
          (`id`, `name`, `proto`, `instance`, `state`)
          VALUES
          (?1,
           ?2,
           ?3,
           ?4,
           ?5)
      )");

      BindFieldValues (stmt);
      stmt.BindProto (3, data);
      stmt.BindProto (4, instance);
      /* Mirror the proto's state into its own column so active tournaments can
         be filtered without deserialising the instance BLOB.  The destructor
         writes whenever dirty and every state transition dirties the instance,
         so the column stays in sync automatically.  */
      stmt.Bind (5, (int) GetInstance ().state ());

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

Database::Result<TournamentResult>
TournamentTable::QueryActive ()
{
  /* Only Listed/Pending/Running tournaments do per-block work; Completed rows
     are skipped here (they draw no RNG and run no logic) so the per-block tick
     no longer scales with the unbounded Completed backlog (DEF3).  */
  auto stmt = db.Prepare (
      "SELECT * FROM `tournaments` WHERE `state` != "
      + std::to_string ((int) pxd::TournamentState::Completed)
      + " ORDER BY `id`");
  return stmt.Query<TournamentResult> ();
}

Database::Result<TournamentResult>
TournamentTable::QueryActiveForBlueprint (const std::string& authoredid)
{
  /* The per-blueprint probe in ReopenMissingTournaments: only the active
     (non-Completed) instances of one blueprint matter.  The `name` column holds
     the blueprint's authoredid (set at CreateNew), so the
     `tournaments_by_name_state` index lets this seek straight to that blueprint's
     active rows instead of rescanning every active tournament once per blueprint
     every block (DEF2).  Ordered by ID for determinism.  */
  auto stmt = db.Prepare (
      "SELECT * FROM `tournaments` WHERE `name` = ?1 AND `state` != "
      + std::to_string ((int) pxd::TournamentState::Completed)
      + " ORDER BY `id`");
  stmt.Bind (1, authoredid);
  return stmt.Query<TournamentResult> ();
}

namespace
{

/** Result type for the cheap Completed-count probe.  */
struct CompletedCountResult : public Database::ResultType
{
  RESULT_COLUMN (int64_t, cnt, 1);
};

} // anonymous namespace

void
TournamentTable::PruneCompleted (const unsigned keep)
{
  /* Windowed retention GC: keep the most recent `keep` Completed tournaments
     (by id) and delete the older excess.

     First a cheap covering-index COUNT over `tournaments_by_state` tells us how
     many Completed rows exist (no row/proto reads).  Only when that exceeds the
     cap -- the rare case at steady state, since the cap holds it down -- do we
     fetch and delete the oldest excess.  This keeps the per-block cost O(1)-ish
     in the common (under-cap) path instead of scanning the whole Completed
     backlog every block.  */
  unsigned completedCount;
  {
    auto stmt = db.Prepare (
        "SELECT COUNT(*) AS `cnt` FROM `tournaments` WHERE `state` = "
        + std::to_string ((int) pxd::TournamentState::Completed));
    auto res = stmt.Query<CompletedCountResult> ();
    CHECK (res.Step ());
    completedCount = (unsigned) res.Get<CompletedCountResult::cnt> ();
    CHECK (!res.Step ());
  }

  if (completedCount <= keep)
    return;

  const unsigned toDelete = completedCount - keep;

  /* Fetch only the oldest `toDelete` ids (ascending id == oldest first) and
     delete them.  Reads no proto BLOBs.  */
  std::vector<Database::IdT> ids;
  ids.reserve (toDelete);
  {
    auto stmt = db.Prepare (
        "SELECT `id` FROM `tournaments` WHERE `state` = "
        + std::to_string ((int) pxd::TournamentState::Completed)
        + " ORDER BY `id` ASC LIMIT " + std::to_string (toDelete));
    auto res = stmt.Query<TournamentResult> ();
    while (res.Step ())
      ids.push_back (res.Get<TournamentResult::id> ());
  }

  for (const auto id : ids)
    DeleteById (id);
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
/*
    GSP for the tf blockchain game
    Copyright (C) 2019-2020  Autonomous Worlds Ltd

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
#include <cmath>
#include <utility>
#include <fstream>

#include <xayautil/hash.hpp>
#include "logic.hpp"
#include "database/reward.hpp"
#include "database/recipe.hpp"
#include "database/ongoings.hpp"
#include "database/globaldata.hpp"
#include "database/params.hpp"

#include "proto/tournament_result.pb.h"

#include "moveprocessor.hpp"

#include "database/schema.hpp"

#include <glog/logging.h>

#include <sstream>

#include <chrono>
#include <thread>

namespace
{

/** Clock used for timing the callbacks.  */
using PerformanceTimer = std::chrono::high_resolution_clock;

/** Duration type used for reporting callback timings.  */
using CallbackDuration = std::chrono::microseconds;
/** Unit (as string) for the callback timings.  */
constexpr const char* CALLBACK_DURATION_UNIT = "us";

} // anonymous namespace

namespace pxd
{

SQLiteGameDatabase::SQLiteGameDatabase (xaya::SQLiteDatabase& d, PXLogic& g)
  : game(g)
{
  SetDatabase (d);
}

Database::IdT
SQLiteGameDatabase::GetNextId ()
{
  Database::IdT next = game.Ids ("pxd").GetNext ();
  VLOG (1) << "GetNextId: " << next;
  return next;
}

Database::IdT
SQLiteGameDatabase::GetLogId ()
{
  return game.Ids ("log").GetNext ();
}

void
PXLogic::UpdateState (Database& db, xaya::Random& rnd,
                      const xaya::Chain chain,
                      const Json::Value& blockData)
{
  const auto start = PerformanceTimer::now ();
      
  const auto& blockMeta = blockData["block"];
  CHECK (blockMeta.isObject ());
  const auto& heightVal = blockMeta["height"];
  CHECK (heightVal.isUInt64 ());
  const unsigned height = heightVal.asUInt64 ();
  const auto& timestampVal = blockMeta["timestamp"];
  CHECK (timestampVal.isInt64 ());
  const int64_t timestamp = timestampVal.asInt64 ();

  Context ctx(chain, height, timestamp);

  UpdateState (db, rnd, ctx, blockData);
  
  const auto end = PerformanceTimer::now ();
  LOG (INFO) << "Update state took " << std::chrono::duration_cast<CallbackDuration> (end - start).count ();   
}

void
PXLogic::UpdateState (Database& db, xaya::Random& rnd,
                      const Context& ctx, const Json::Value& blockData)
{
   /* Seed the per-block RNG from the confirmed block hash.  Skipped on REGTEST
      (low height) so the golden-replay deterministic no-reseed behaviour holds. */
   if (ctx.Chain () != xaya::Chain::REGTEST)
   {
    const auto& blockMeta = blockData["block"];
    CHECK (blockMeta.isObject ());
    const auto& hashVal = blockMeta["hash"].asString();	
	
	xaya::SHA256 seed;
	seed << hashVal;
	rnd.Seed (seed.Finalise ());	   
   }	   
	
  /** We run this very early, as we want to create tournament instances from blueprints ASAP.
  We are going to open tournaments instances if blueprint is not present ir game or already full and running */
  ReopenMissingTournaments(db, ctx);  
      
  SetFreeTransfiguringFighters(db, ctx);	  
	  
  MoveProcessor mvProc(db, rnd, ctx);
  mvProc.ProcessAdmin (blockData["admin"]);
  mvProc.ProcessAll (blockData["moves"]);
  
  /** The first thing we do, is try and resolve all pending ongoing operations
  from the last block. So if there count is minimal 1, they will guarantee
  to get solved before any new moves submitted by player */    
  TickAndResolveOngoings(db, ctx, rnd);
  
  /** We need to see if any fighters is on exchange, and if its expiration
      is already off and we need to de-lest it automatically*/
  CheckFightersForSale(db, ctx);
  
  /** We process tournaments almost lastly, to make sure participiant count updates properly
  */
  ProcessTournaments(db, ctx, rnd);   
  
#ifdef ENABLE_SLOW_ASSERTS
  ValidateStateSlow (db, ctx);
#endif // ENABLE_SLOW_ASSERTS
}

void
PXLogic::SetupSchema (xaya::SQLiteDatabase& db)
{
  SetupDatabaseSchema (db);
}

void
PXLogic::GetInitialStateBlock (unsigned& height,
                               std::string& hashHex) const
{
  const xaya::Chain chain = GetChain ();
  switch (chain)
    {
    case xaya::Chain::POLYGON:
      /* Fresh Treatfighter relaunch genesis on Polygon.  Empty hash accepts
         any block at this height (verified libxayagame pattern).
         This is a recent height (Polygon tip was ~89'248'003 on 2026-06-27)
         chosen for a fast initial sync.
         TODO(launch): re-set to the Polygon tip at actual launch time to
         minimise backfill.  Consensus-critical: never change after launch.  */
      height = 89'246'000;
      hashHex = "";
      break;

    case xaya::Chain::MAIN:
      height = 4'982'119;
      hashHex
          = "3bcde52bb65f0da9f3fd952a3b4911d9a525d116d46a5a223f0bf7dc056ca9a9";
      break;

    case xaya::Chain::TEST:
      height = 112'000;
      hashHex
          = "9c5b83a5caaf7f4ce17cc1f38fdb1ed3e3e3e98e43d23d19a4810767d7df38b9";
      break;

    case xaya::Chain::REGTEST:
      height = 0;
      hashHex
          = "6f750b36d22f1dc3d0a6e483af45301022646dfc3b3ba2187865f5a7d6d83ab1";
      break;

    default:
      LOG (FATAL) << "Unexpected chain: " << xaya::ChainToString (chain);
    }
}

void
PXLogic::InitialiseState (xaya::SQLiteDatabase& db)
{
  SQLiteGameDatabase dbObj(db, *this);

  GlobalData gd(dbObj);
  gd.InitialiseDatabase ();

  GameParams gp(dbObj);
  gp.InitialiseDatabase ();

  /* The initialisation uses up some auto IDs, namely for placed buildings.
     We start "regular" IDs at a later value to avoid shifting them always
     when we tweak initialisation, and thus having to potentially update test
     data and other stuff.  */
  Ids ("pxd").ReserveUpTo (1'000);
}

void
PXLogic::UpdateState (xaya::SQLiteDatabase& db, const Json::Value& blockData)
{  
  SQLiteGameDatabase dbObj(db, *this);
  UpdateState (dbObj, GetContext ().GetRandom (),
               GetChain (), blockData);

  if (dumpStateToFile)
  {
    Json::Value fState = GetStateAsJson(db);
    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    builder["indentation"] = "   ";  
  
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    std::ofstream outputFileStream("./statedump.json");
    writer -> write(fState, &outputFileStream);

    LOG (WARNING) << "Dumping state to file";
  }
}

Json::Value
PXLogic::GetStateAsJson (const xaya::SQLiteDatabase& db)
{
  SQLiteGameDatabase dbObj(const_cast<xaya::SQLiteDatabase&> (db), *this);

  std::time_t result = std::time(nullptr);

  const Context ctx(GetChain (),
                    Context::NO_HEIGHT, (int64_t)result);
  
  GameStateJson gsj(dbObj, ctx);

  return gsj.FullState ();
}

Json::Value
PXLogic::GetCustomStateData (xaya::Game& game, const JsonStateFromRawDb& cb)
{
  return SQLiteGame::GetCustomStateData (game, "data",
      [this, &cb] (const xaya::SQLiteDatabase& db, const xaya::uint256& hash,
                   const unsigned height)
        {
          SQLiteGameDatabase dbObj(const_cast<xaya::SQLiteDatabase&> (db),
                                   *this);
          return cb (dbObj, hash, height);
        });
}

Json::Value
PXLogic::GetCustomStateData (xaya::Game& game,
                             const JsonStateFromDatabaseWithBlock& cb)
{
  return GetCustomStateData (game,
    [this, &cb] (Database& db, const xaya::uint256& hash, const unsigned height)
        {
          /* The block height of the state being queried is supplied by the
             callback -- use it so height-derived JSON (a fighter's / ongoing's
             "blocks left") is correct and Context::Height() never trips its
             NO_HEIGHT guard on getuser/getxayaplayers/getexchange/gettournaments
             while any ongoing operation is active. */
          const Context ctx(GetChain (), height, Context::NO_TIMESTAMP);
          GameStateJson gsj(db, ctx);
          return cb (gsj, hash, height);
        });
}

Json::Value
PXLogic::GetCustomStateData (xaya::Game& game, const JsonStateFromDatabase& cb)
{
  return GetCustomStateData (game,
    [&cb] (GameStateJson& gsj, const xaya::uint256& hash, const unsigned height)
    {
      return cb (gsj);
    });
}

void
PXLogic::ValidateStateSlow (Database& db, const Context& ctx)
{
  LOG (INFO) << "Performing slow validation of the game-state database...";
}

} // namespace pxd

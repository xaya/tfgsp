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

#ifndef PXD_LOGIC_HPP
#define PXD_LOGIC_HPP

#include "context.hpp"
#include "gamestatejson.hpp"

#include "database/database.hpp"
#include "database/xayaplayer.hpp"

#include <xayagame/sqlitegame.hpp>
#include <xayagame/sqlitestorage.hpp>
#include <xayautil/random.hpp>

#include <json/json.h>

#include <sqlite3.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>

#include <fpm/fixed.hpp>
#include <fpm/math.hpp>

#include <gflags/gflags.h>

namespace pxd
{

class PXLogic;

/**
 * Database instance that uses an SQLiteGame instance for everything.
 */
class SQLiteGameDatabase : public Database
{

private:

  /** The underlying SQLiteGame instance.  */
  PXLogic& game;

public:

  explicit SQLiteGameDatabase (xaya::SQLiteDatabase& d, PXLogic& g);

  SQLiteGameDatabase () = delete;
  SQLiteGameDatabase (const SQLiteGameDatabase&) = delete;
  void operator= (const SQLiteGameDatabase&) = delete;

  Database::IdT GetNextId () override;
  Database::IdT GetLogId () override;

};

/**
 * The game logic implementation for tf.  This is the main class that
 * acts as the game-specific code, interacting with libxayagame and the Xaya
 * daemon.  By itself, it is combining the various other classes and functions
 * that implement the real game logic.
 */
class PXLogic : public xaya::SQLiteGame
{

public:

  bool dumpStateToFile;

  /**
   * Scans tournament instances and tournament blueprints to see if we need
   * to open any new instance
   */     
  static void ReopenMissingTournaments(Database& db, const Context& ctx);
  
  /**
   * We keep this extremely simple for now, marking all transfiguring fighters as available
   * on new block
   */     
  static void SetFreeTransfiguringFighters(Database& db, const Context& ctx);  
  
  /**
   * Scans all fighters, finds ones on the exchange, tests if expired, delists
   */    
  static void CheckFightersForSale(Database& db, const Context& ctx);
  
  /**
   * Handles the actual logic for the game-state update.  This is extracted
   * here out of UpdateState, so that it can be accessed from unit tests
   * independently of SQLiteGame.
   */
  static void UpdateState (Database& db, xaya::Random& rnd,
                           xaya::Chain chain,
                           const Json::Value& blockData);  
						   
  /** Rolls one activity-reward table entry at resolve, recursing through
      RewardType::List sub-tables, and credits each leaf reward directly into the
      player/fighter via CreditActivityReward (no claim tx).  */
  static void GenerateActivityReward(const uint32_t fighterID, const std::string& blueprintAuthID,
  const uint32_t tournamentID,  const pxd::proto::AuthoredActivityReward& rw, const Context& ctx, Database& db,
  std::unique_ptr<XayaPlayer>& a, xaya::Random& rnd, const uint32_t posInTableList,
  const std::string& basedRewardsTableAuthId, const std::string& sweetenerAuthID,
  const uint32_t recursionDepth = 0);

  /** Change B (Epic 4x): scales every reward entry's weight by `divisor`, EXCEPT
      an Epic generated recipe (type==GeneratedRecipe && quality==Epic) which keeps
      its authored weight -- so Epic's relative odds become ~1/divisor.  Pure, so
      it is unit-testable directly.  With divisor==1 it returns the raw weights. */
  static std::vector<uint32_t> ScaledRewardWeights (
      const pxd::proto::ActivityReward& table, uint32_t divisor);

  /** One weighted draw over the scaled distribution.  Returns the chosen index
      into table.rewards(), or -1 if the scaled total is 0 (no reward -- matches
      the legacy totalWeight==0 no-draw path).  Consumes exactly one rnd.NextInt
      when the scaled total is > 0. */
  static int32_t PickWeightedRewardIndex (
      const pxd::proto::ActivityReward& table, uint32_t divisor, xaya::Random& rnd);

  /** Credits one rolled leaf reward directly into player/fighter state at resolve.
      Candy/move/armor/animation credit unconditionally (move/armor/anim drop if
      their target fighter is gone -- HALT-01); recipe rewards credit if a recipe
      slot is free, else HOLD as a bounded overflow row that auto-drains later.
      Bumps rewards_serial on each landed reward.  Adds no rnd draw beyond the
      recipe generation that already happened at resolve.  The reward source
      (Expedition/Tournament/Sweetener) is derived from the id args.  */
  static void CreditActivityReward(std::unique_ptr<XayaPlayer>& a, const uint32_t fighterID,
  const std::string& blueprintAuthID, const uint32_t tournamentID,
  const pxd::proto::AuthoredActivityReward& rw, const Context& ctx, Database& db, xaya::Random& rnd,
  const uint32_t posInTableList, const std::string& basedRewardsTableAuthId, const std::string& sweetenerAuthID);

  /** Drains held recipe-overflow rewards (owner=player rows carrying a generated
      recipe) into ownership, oldest-first, while recipe slots remain.  Event-driven:
      call only when the player frees a recipe slot or right after crediting.
      O(pending), bounded by max_unclaimed_reward_amount; adds no rnd draw.  */
  static void DrainPendingRecipeRewards(std::unique_ptr<XayaPlayer>& a, const Context& ctx, Database& db);

  /**
   * When cooking sweetener operation reaches 0 blocks, we either
   * resolve it or cancel, based on cirsumstances. Made public for unit tests */         
  static void ResolveSweetener(std::unique_ptr<XayaPlayer>& a, std::string sweetenerAuthID, const uint32_t fighterID, const uint32_t rewardID, 
  Database& db, const Context& ctx, xaya::Random& rnd);  
  
private:
  
  /**
   * When cooking recepie operation reaches 0 blocks, we either
   * resolve it or reverse, based on cirsumstances */                         
  static void ResolveCookingRecepie(std::unique_ptr<XayaPlayer>& a, const uint32_t recepieID, Database& db, const Context& ctx, xaya::Random& rnd);

  /**
   * When expedition block count operation reaches 0 blocks, we either
   * resolve it or close as completed, based on cirsumstances */   
  static void ResolveExpedition(std::unique_ptr<XayaPlayer>& a, const std::string blueprintAuthID, const uint32_t fighterID, Database& db, const Context& ctx, xaya::Random& rnd);

  /**
   * When deconstruction block count operation reaches 0 blocks, we either
   * resolve it or close as completed, based on cirsumstances */   
  static void ResolveDeconstruction(std::unique_ptr<XayaPlayer>& a, const uint32_t fighterID, Database& db, const Context& ctx, xaya::Random& rnd);

  /**
   * For every ongoing operation we reduce its block count by 1
   * and if it reaches 0, we are sending it for the resolution, and erase from array
   */     
  static void TickAndResolveOngoings(Database& db, const Context& ctx, xaya::Random& rnd);

  /**
   * Scans all tournaments to either start them or finilize
   */  
  static void ProcessTournaments(Database& db, const Context& ctx, xaya::Random& rnd); 
  
  /**
   * Inner helper function, to resolve 1:1 clash points between 2 fighters
   */    
  static void ProcessFighterPair(int64_t fighter1, int64_t fighter2, std::map<uint32_t, proto::TournamentResult*>& fighterResults, std::map<std::string, fpm::fixed_24_8>& participatingPlayerTotalScore, FighterTable& fighters, const Context& ctx, xaya::Random& rnd);

  /**
   * When fighters are fighting against each other in tournament, used to calculate results
   * between the 2 moves of fighters
   */  

  static fpm::fixed_24_8 ExecuteOneMoveAgainstAnother(const Context& ctx, std::string lmv, std::string rmv);
  
  /**
   * Some crazy class ported as function from original code for ratings and scores calculation
   */    
  
  static void CreateEloRating(const Context& ctx, fpm::fixed_24_8& ratingA, fpm::fixed_24_8& ratingB, fpm::fixed_24_8& scoreA, fpm::fixed_24_8& scoreB, fpm::fixed_24_8& expectedA, 
  fpm::fixed_24_8& expectedB, fpm::fixed_24_8& newRatingA, fpm::fixed_24_8& newRatingB);


  /**
   * Updates the state with a custom FameUpdater.  This is used for mocking
   * the instance in tests.
   */
  static void UpdateState (Database& db, xaya::Random& rnd,
                           const Context& ctx, const Json::Value& blockData);

  /**
   * Performs (potentially slow) validations on the current database state.
   * This is used when compiled with --enable-slow-asserts after each block
   * update, for testing purposes.  It should not be run in production builds
   * because it may really slow down syncing.  If an error is detected, then
   * this CHECK-fails the binary.
   */
  static void ValidateStateSlow (Database& db, const Context& ctx);

  friend class PXLogicTests;
  friend class PXRpcServer;
  friend class SQLiteGameDatabase;

protected:

  void SetupSchema (xaya::SQLiteDatabase& db) override;

  void GetInitialStateBlock (unsigned& height,
                             std::string& hashHex) const override;
  void InitialiseState (xaya::SQLiteDatabase& db) override;

  void UpdateState (xaya::SQLiteDatabase& db,
                    const Json::Value& blockData) override;

  Json::Value GetStateAsJson (const xaya::SQLiteDatabase& db) override;

public:

  /**
   * Type for a callback that retrieves some JSON data from the database
   * directly (not using GameStateJson).
   */
  using JsonStateFromRawDb
      = std::function<Json::Value (Database& db, const xaya::uint256& hash,
                                   unsigned height)>;

  /** Type for a callback that retrieves JSON data from the database.  */
  using JsonStateFromDatabase = std::function<Json::Value (GameStateJson& gsj)>;

  /** Extended callback that expects also block hash and height.  */
  using JsonStateFromDatabaseWithBlock
    = std::function<Json::Value (GameStateJson& gsj,
                                 const xaya::uint256& hash, unsigned height)>;

  PXLogic () = default;

  PXLogic (const PXLogic&) = delete;
  void operator= (const PXLogic&) = delete;

  /* Bring the base SQLiteGame overload (raw xaya::SQLiteDatabase callback,
     3-arg form) into scope for the state-hash RPCs; it overloads with the
     TF-specific 2-arg GetCustomStateData declarations below.  */
  using xaya::SQLiteGame::GetCustomStateData;

  /**
   * Returns custom game-state data as JSON, with a callback that
   * directly receives the database (and does not go through the
   * GameStateJson class).
   */
  Json::Value GetCustomStateData (xaya::Game& game,
                                  const JsonStateFromRawDb& cb);

  /**
   * Returns custom game-state data as JSON.  The provided callback is invoked
   * with a GameStateJson instance to retrieve the "main" state data that is
   * returned in the JSON "data" field.
   */
  Json::Value GetCustomStateData (xaya::Game& game,
                                  const JsonStateFromDatabaseWithBlock& cb);

  /**
   * Variant of the other overload that extracts data with a callback that does
   * not need block hash or height.
   */
  Json::Value GetCustomStateData (xaya::Game& game,
                                  const JsonStateFromDatabase& cb);

};

} // namespace pxd

#endif // PXD_LOGIC_HPP

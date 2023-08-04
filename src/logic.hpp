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
#include "params.hpp"

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
   * Scans all tournaments to either start them or finilize; public for unit test access purposes
   */  
  static void ProcessSpecialTournaments(Database& db, const Context& ctx, xaya::Random& rnd);   
  
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
						   
  /** Helper function that generates and pushes new reward instance into the database and returns unique auto ID */
  static std::vector<uint32_t> GenerateActivityReward(const uint32_t fighterID, const std::string blueprintAuthID, 
  const uint32_t tournamentID,  const pxd::proto::AuthoredActivityReward rw, const Context& ctx, Database& db, 
  std::unique_ptr<XayaPlayer>& a, xaya::Random& rnd, const uint32_t posInTableList, 
  const std::string basedRewardsTableAuthId, const std::string sweetenerAuthID);						   
  
private:
  
  /**
   * When cooking recepie operation reaches 0 blocks, we either
   * resolve it or reverse, based on cirsumstances */                         
  static void ResolveCookingRecepie(std::unique_ptr<XayaPlayer>& a, const uint32_t recepieID, Database& db, const Context& ctx, xaya::Random& rnd);

  /**
   * When cooking sweetener operation reaches 0 blocks, we either
   * resolve it or cancel, based on cirsumstances */         
  static void ResolveSweetener(std::unique_ptr<XayaPlayer>& a, std::string sweetenerAuthID, const uint32_t fighterID, const uint32_t rewardID, 
  Database& db, const Context& ctx, xaya::Random& rnd);

  /**
   * When expedition block count operation reaches 0 blocks, we either
   * resolve it or close as completed, based on cirsumstances */   
  static void ResolveExpedition(std::unique_ptr<XayaPlayer>& a, const std::string blueprintAuthID, const uint32_t fighterID, Database& db, const Context& ctx, xaya::Random& rnd);

  /**
   * When deconstruction block count operation reaches 0 blocks, we either
   * resolve it or close as completed, based on cirsumstances */   
  static void ResolveDeconstruction(std::unique_ptr<XayaPlayer>& a, const uint32_t fighterID, Database& db, const Context& ctx, xaya::Random& rnd);

  /**
   * Resolve one instance of special tournament match 
   * */   
  static void ResolveSpecialTournamentFight(std::string attackerName, std::vector<int64_t> attackerTeam, std::string defenderName, std::vector<int64_t> defenderTeam, int64_t ID, FighterTable& fighters, const Context& ctx, xaya::Random& rnd, int64_t& scoreAttacker, int64_t& scoreDefender);

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
   * For the special tournament, we need to precalculate, which tier player can participate in
   */  
  static void RecalculatePlayerTiers(Database& db, const Context& ctx);   

  /**
   * Inner helper function, to resolve 1:1 clash points between 2 fighters
   */    
  static void ProcessFighterPair(int64_t fighter1, int64_t fighter2, bool isSpecial, std::map<uint32_t, proto::TournamentResult*> fighterResults, std::map<std::string, fpm::fixed_24_8>& participatingPlayerTotalScore, FighterTable& fighters, const Context& ctx, xaya::Random& rnd);

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

  static void EloGetNewRatings();


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

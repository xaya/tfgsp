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

private:

  /**
   * When cooking recepie operation reaches 0 blocks, we either
   * resolve it or reverse, based on cirsumstances */                         
  static void ResolveCookingRecepie(std::unique_ptr<XayaPlayer>& a, const std::string receipeAuthID, Database& db, const Context& ctx, xaya::Random& rnd);

  /**
   * For every ongoing operation we reduce its block count by 1
   * and if it reaches 0, we are sending it for the resolution, and erase from array
   */     
  static void TickAndResolveOngoings(Database& db, const Context& ctx, xaya::Random& rnd);

  /**
   * Handles the actual logic for the game-state update.  This is extracted
   * here out of UpdateState, so that it can be accessed from unit tests
   * independently of SQLiteGame.
   */
  static void UpdateState (Database& db, xaya::Random& rnd,
                           xaya::Chain chain,
                           const Json::Value& blockData);

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

/*
    GSP for the tf blockchain game
    Copyright (C) 2024  Autonomous Worlds Ltd

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

/*
   Tier 2 reorg test: the PRODUCTION reorg path.

   Where reorg_tests.cpp drives PXLogic::UpdateState statically and exercises the
   sqlite session-changeset undo PRIMITIVE directly, this test wires a real
   xaya::Game on top of the real PXLogic (a xaya::SQLiteGame) with its real
   SQLiteStorage, and drives blocks through Game::BlockAttach / Game::BlockDetach
   -- the exact code path a live node runs on a Polygon reorg.  BlockAttach
   records the per-block undo data; BlockDetach replays it in reverse.  We assert
   the game-table content round-trips EXACTLY across attach/detach, and that a
   fork switch (detach then attach a different block) yields the right state.

   This closes the integration gap above the primitive: that PXLogic's overrides
   (SetupSchema / InitialiseState / UpdateState) are wired into SQLiteGame's undo
   machinery correctly.  State is built through real moves (an account is
   auto-created for any move sender, moveprocessor.cpp:1302).

   Uses the vendored xayagame test harness (xgametestutils.hpp), since
   GameTestWithBlockchain is not part of the installed libxayagame headers and
   Game::BlockAttach/Detach are private (friend GameTestFixture only).  REGTEST
   chain: it has a defined genesis hash (POLYGON's is empty, which the
   storage-init pattern cannot parse); the undo path itself is chain-independent.
*/

#include "logic.hpp"

#include "xgametestutils.hpp"

#include <xayagame/game.hpp>
#include <xayagame/sqlitestorage.hpp>

#include <xayautil/uint256.hpp>

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <json/json.h>

#include <sqlite3.h>

#include <algorithm>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace pxd
{
namespace
{

constexpr const char* GAME_ID = "tf";
/* A valid CHI address (from the golden scenario); reused for every account --
   the account NAME is what differentiates the states, not the address.  */
constexpr const char* ADDR = "CGUpAcjsb6MDktSYg8yRDxDutr7FhWtdWC";

constexpr std::uint64_t FNV_OFFSET = 1469598103934665603ull;
constexpr std::uint64_t FNV_PRIME = 1099511628211ull;

std::uint64_t
FnvMix (std::uint64_t h, const void* data, const std::size_t len)
{
  const auto* p = static_cast<const unsigned char*> (data);
  for (std::size_t i = 0; i < len; ++i)
    {
      h ^= p[i];
      h *= FNV_PRIME;
    }
  return h;
}

/**
 * Exposes SQLiteGame::GetDatabaseForTesting (protected) so the test can hash the
 * underlying database directly.
 */
class TestPXLogic : public PXLogic
{
public:
  using xaya::SQLiteGame::GetDatabaseForTesting;
};

/* ************************************************************************** */

class ReorgGameTests : public xaya::GameTestWithBlockchain
{

protected:

  xaya::Game game;
  TestPXLogic rules;

  ReorgGameTests ()
    : xaya::GameTestWithBlockchain(GAME_ID), game(GAME_ID)
  {
    rules.Initialise (":memory:");
    /* MAIN (not REGTEST): it has a defined genesis hash, and avoids the REGTEST
       height cross-checks that SetStorage would enable -- those need a live RPC
       client, which we don't have (null).  The undo path is chain-independent.  */
    rules.InitialiseGameContext (xaya::Chain::MAIN, GAME_ID, nullptr);

    /* SetStorage opens the database (storage->Initialise), so it must precede
       GetInitialState, which writes the initial state into that open DB.  */
    game.SetStorage (rules.GetStorage ());
    game.SetGameLogic (rules);

    unsigned height;
    std::string hashHex;
    const xaya::GameStateData state
        = rules.GetInitialState (height, hashHex, nullptr);

    xaya::uint256 genHash;
    CHECK (genHash.FromHex (hashHex));
    SetStartingBlock (height, genHash);

    /* Record the genesis as the storage's current state so the first attach's
       parent matches.  */
    rules.GetStorage ().BeginTransaction ();
    rules.GetStorage ().SetCurrentGameState (genHash, state);
    rules.GetStorage ().CommitTransaction ();

    ForceState (game, State::UP_TO_DATE);
  }

  /** Raw sqlite handle behind the SQLiteGame's database.  */
  sqlite3*
  RawHandle ()
  {
    sqlite3* h = nullptr;
    rules.GetDatabaseForTesting ().AccessDatabase (
        [&h] (sqlite3* raw) { h = raw; return 0; });
    return h;
  }

  /** A one-move block: account `name` sends an init move (auto-creating it).
      Equivalent JSON: [{"name":name,"move":{"a":{"init":{"address":ADDR}}}}].  */
  static Json::Value
  AccountInit (const std::string& name)
  {
    Json::Value init(Json::objectValue);
    init["address"] = ADDR;
    Json::Value a(Json::objectValue);
    a["init"] = init;
    Json::Value moveObj(Json::objectValue);
    moveObj["a"] = a;

    Json::Value mv(Json::objectValue);
    mv["name"] = name;
    mv["move"] = moveObj;

    Json::Value arr(Json::arrayValue);
    arr.append (mv);
    return arr;
  }

  /**
   * Per-table content hash of the GAME tables (excludes sqlite_* and the
   * libxayagame xayagame_* bookkeeping/undo tables, which legitimately differ
   * block to block).  Row hashes are sorted before folding so the table hash is
   * the hash of the row set.
   */
  std::map<std::string, std::uint64_t>
  GameStateHash ()
  {
    sqlite3* h = RawHandle ();

    std::vector<std::string> tables;
    {
      sqlite3_stmt* st = nullptr;
      CHECK_EQ (sqlite3_prepare_v2 (h,
                  "SELECT name FROM sqlite_master WHERE type='table' "
                  "AND name NOT LIKE 'sqlite_%' AND name NOT LIKE 'xayagame_%' "
                  "ORDER BY name",
                  -1, &st, nullptr), SQLITE_OK);
      while (sqlite3_step (st) == SQLITE_ROW)
        tables.push_back (
            reinterpret_cast<const char*> (sqlite3_column_text (st, 0)));
      sqlite3_finalize (st);
    }

    std::map<std::string, std::uint64_t> out;
    for (const auto& t : tables)
      {
        std::vector<std::uint64_t> rowHashes;

        sqlite3_stmt* st = nullptr;
        const std::string q = "SELECT * FROM \"" + t + "\"";
        CHECK_EQ (sqlite3_prepare_v2 (h, q.c_str (), -1, &st, nullptr),
                  SQLITE_OK);
        const int ncol = sqlite3_column_count (st);

        while (sqlite3_step (st) == SQLITE_ROW)
          {
            std::uint64_t rh = FNV_OFFSET;
            for (int c = 0; c < ncol; ++c)
              {
                const int ty = sqlite3_column_type (st, c);
                const unsigned char tb = static_cast<unsigned char> (ty);
                rh = FnvMix (rh, &tb, 1);
                switch (ty)
                  {
                  case SQLITE_INTEGER:
                    { const sqlite3_int64 v = sqlite3_column_int64 (st, c);
                      rh = FnvMix (rh, &v, sizeof (v)); break; }
                  case SQLITE_FLOAT:
                    { const double v = sqlite3_column_double (st, c);
                      rh = FnvMix (rh, &v, sizeof (v)); break; }
                  case SQLITE_TEXT:
                    { const void* p = sqlite3_column_text (st, c);
                      const int n = sqlite3_column_bytes (st, c);
                      rh = FnvMix (rh, p, static_cast<std::size_t> (n)); break; }
                  case SQLITE_BLOB:
                    { const void* p = sqlite3_column_blob (st, c);
                      const int n = sqlite3_column_bytes (st, c);
                      rh = FnvMix (rh, p, static_cast<std::size_t> (n)); break; }
                  default: break;
                  }
              }
            rowHashes.push_back (rh);
          }
        sqlite3_finalize (st);

        std::sort (rowHashes.begin (), rowHashes.end ());
        std::uint64_t th = FNV_OFFSET;
        for (const std::uint64_t rh : rowHashes)
          th = FnvMix (th, &rh, sizeof (rh));
        const std::uint64_t cnt = rowHashes.size ();
        th = FnvMix (th, &cnt, sizeof (cnt));
        out[t] = th;
      }
    return out;
  }

  /** True iff player `name` currently exists in the database.  */
  bool
  PlayerExists (const std::string& name)
  {
    sqlite3* h = RawHandle ();
    sqlite3_stmt* st = nullptr;
    CHECK_EQ (sqlite3_prepare_v2 (
                  h, "SELECT 1 FROM xayaplayers WHERE name = ?1",
                  -1, &st, nullptr), SQLITE_OK);
    sqlite3_bind_text (st, 1, name.c_str (), -1, SQLITE_TRANSIENT);
    const bool exists = (sqlite3_step (st) == SQLITE_ROW);
    sqlite3_finalize (st);
    return exists;
  }

};

/* ************************************************************************** */

/**
 * Attach two blocks (each creating an account), detach both, and assert the
 * game state is restored EXACTLY at every step through the real BlockAttach /
 * BlockDetach undo path -- with the Game staying up-to-date throughout.
 */
TEST_F (ReorgGameTests, AttachDetachRestoresStateExactly)
{
  const auto h0 = GameStateHash ();
  EXPECT_FALSE (PlayerExists ("domob"));

  AttachBlock (game, xaya::BlockHash (1), AccountInit ("domob"));
  const auto h1 = GameStateHash ();
  EXPECT_EQ (GetState (game), State::UP_TO_DATE);
  EXPECT_TRUE (PlayerExists ("domob"));
  EXPECT_NE (h1, h0) << "attaching a move-bearing block did not change state";

  AttachBlock (game, xaya::BlockHash (2), AccountInit ("bob"));
  const auto h2 = GameStateHash ();
  EXPECT_TRUE (PlayerExists ("bob"));
  EXPECT_NE (h2, h1);

  DetachBlock (game);
  EXPECT_EQ (GetState (game), State::UP_TO_DATE);
  EXPECT_EQ (GameStateHash (), h1) << "detach did not restore the prior state";
  EXPECT_FALSE (PlayerExists ("bob")) << "bob survived the detach";
  EXPECT_TRUE (PlayerExists ("domob"));

  DetachBlock (game);
  EXPECT_EQ (GameStateHash (), h0) << "detach to genesis was not exact";
  EXPECT_FALSE (PlayerExists ("domob")) << "domob survived the detach";
}

/**
 * Fork handling: from a common parent, attaching one block then reorging to a
 * DIFFERENT block must produce the alternate branch's state (and not leak the
 * abandoned branch's).
 */
TEST_F (ReorgGameTests, ForkSwitchProducesAlternateState)
{
  const auto h0 = GameStateHash ();

  /* Branch A: domob.  */
  AttachBlock (game, xaya::BlockHash (10), AccountInit ("domob"));
  const auto hA = GameStateHash ();
  EXPECT_TRUE (PlayerExists ("domob"));

  /* Reorg back to the common parent and take branch B: carol.  */
  DetachBlock (game);
  EXPECT_EQ (GameStateHash (), h0);

  AttachBlock (game, xaya::BlockHash (11), AccountInit ("carol"));
  const auto hB = GameStateHash ();
  EXPECT_EQ (GetState (game), State::UP_TO_DATE);

  EXPECT_TRUE (PlayerExists ("carol")) << "branch B move not applied";
  EXPECT_FALSE (PlayerExists ("domob")) << "abandoned branch A leaked into B";
  EXPECT_NE (hB, hA) << "fork branches produced identical state";
  EXPECT_NE (hB, h0);
}

/* ************************************************************************** */

} // anonymous namespace
} // namespace pxd

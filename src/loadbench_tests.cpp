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
   P-E1 synthetic-load benchmark.

   This is NOT a correctness test (that is goldenreplay_tests).  It measures the
   per-block cost of the ongoing-operations tick as a function of the number of
   players, to prove the Phase 2 efficiency work:

     * Stage 1 (skip-when-empty write guard, already applied in
       PXLogic::TickAndResolveOngoings): players with no ongoings are no longer
       marked dirty, so the per-block ROW-WRITE count drops from O(all players)
       to O(players with ongoings).  Metric: "rows/block".
     * Stage 1 is NOT yet "event-driven": TickAndResolveOngoings still
       QueryAll()-scans every player every block, so per-block WALL-TIME still
       rises with N.  Metric: "ms/block".  Stage 2 (height-keyed ongoing table)
       is what flattens this.
     * Undo-log growth (what actually bloats the on-disk DB in production) is the
       size of the per-block SQLite session changeset that xaya::SQLiteGame
       stores into xayagame_undo.  Metric: "undo-bytes/block".

   It is deliberately a SEPARATE check_PROGRAM that is NOT in TESTS, so it is
   compiled by `make check` (stays from bit-rotting) but never auto-run.  Run it
   on demand:

       TF_LOADBENCH=1 ./loadbench_tests
       TF_LOADBENCH=1 TF_LOADBENCH_MAXN=1000 TF_LOADBENCH_M=20 ./loadbench_tests

   Env knobs:
       TF_LOADBENCH        must be set, else every case GTEST_SKIPs.
       TF_LOADBENCH_MAXN   skip configs whose player count exceeds this (fast iter).
       TF_LOADBENCH_M      override the number of measured blocks per config.

   The instrumentation reproduces, in-process, exactly what xaya::SQLiteGame does
   around each block: open a sqlite3 session attached to all tables, run one
   block of UpdateState, then read the changeset.  No real Xaya Core is needed.
*/

#include "logic.hpp"

#include "jsonutils.hpp"
#include "moveprocessor.hpp"
#include "testutils.hpp"

#include "database/dbtest.hpp"
#include "database/xayaplayer.hpp"
#include "database/ongoings.hpp"

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <json/json.h>

#include <sqlite3.h>

#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace pxd
{
namespace
{

/* ************************************************************************** */

/* Per-table PHYSICAL write counter, driven by sqlite3's preupdate hook.  Unlike
   a session changeset (which coalesces identical-content rewrites to nothing),
   the preupdate hook fires for every INSERT/UPDATE/DELETE actually executed --
   so it reveals redundant rewrites of unchanged rows (the WAL/page churn).  The
   pointer is non-null only while a *measured* block runs.  */
std::map<std::string, long long>* g_physWrites = nullptr;

extern "C" void
PreupdateCb (void* /*ctx*/, sqlite3* /*h*/, int /*op*/, const char* /*zDb*/,
             const char* zName, sqlite3_int64 /*k1*/, sqlite3_int64 /*k2*/)
{
  if (g_physWrites != nullptr && zName != nullptr)
    ++(*g_physWrites)[zName];
}

/** One point in the load sweep.  */
struct BenchConfig
{
  /** Total players seeded.  */
  int players;
  /** Of those, how many carry an active (never-resolving) ongoing.  */
  int active;
  /** Measured blocks (overridable via TF_LOADBENCH_M).  */
  int blocks;
  const char* label;
};

/**
 * The sweep.  The "idle" rows vary N with K=0 -- the headline P-E1 case
 * ("writes every block even when nothing is happening").  The "active" rows
 * hold N large and vary K -- the legitimately-necessary work that must keep
 * scaling with K (and only K) after the fix.
 */
const std::vector<BenchConfig> kConfigs = {
  {     0, 0, 30, "idle-0"      },
  {   100, 0, 30, "idle-100"    },
  {  1000, 0, 30, "idle-1k"     },
  { 10000, 0, 30, "idle-10k"    },
  { 50000, 0, 30, "idle-50k"    },
  { 10000,    10, 30, "active-10k-10"   },
  { 10000,   100, 30, "active-10k-100"  },
  { 10000,  1000, 30, "active-10k-1k"   },
};

/** Aggregated per-block metrics (means over the measured window).  */
struct Metrics
{
  double rowsPerBlock = 0.0;
  double undoBytesPerBlock = 0.0;
  double msPerBlock = 0.0;
  /** Mean per-block changeset (coalesced / net-change) row-ops, by table.  */
  std::map<std::string, double> rowsPerTable;
  /** Mean per-block PHYSICAL row-writes (incl. identical content), by table.  */
  std::map<std::string, double> physPerTable;
};

/* ************************************************************************** */

/**
 * Self-contained load fixture.  Mirrors the block-driving primitives of
 * GoldenReplayTests (SetHeight / BuildBlockData / UpdateState / AdvanceBlocks)
 * but adds bulk seeding and per-block SQLite-session instrumentation.  Each
 * parameterised case gets a fresh in-memory DB.
 */
class LoadBenchTests : public DBTestWithSchema,
                       public testing::WithParamInterface<BenchConfig>
{

protected:

  TestRandom rnd;
  ContextForTesting ctx;

  XayaPlayersTable xayaplayers;

  LoadBenchTests ()
    : xayaplayers(db)
  {
    /* Measure the PRODUCTION chain (POLYGON), NOT REGTEST.  REGTEST takes a
       test-only branch in ProcessSpecialTournaments (logic.cpp:986) that calls
       RecalculatePlayerTiers -- an O(players) full rewrite -- EVERY block, which
       never runs in production and would make this benchmark measure a test
       artifact rather than the real per-block cost.  We start just past the
       POLYGON genesis height so the production reseed/fork branches apply.  */
    ctx.SetChain (xaya::Chain::POLYGON);
    SetHeight (89246050);
  }

  void
  SetHeight (const unsigned h)
  {
    ctx.SetHeight (h);
  }

  Json::Value
  BuildBlockData (const Json::Value& moves)
  {
    Json::Value blockData(Json::objectValue);
    blockData["admin"] = Json::Value (Json::arrayValue);
    blockData["moves"] = moves;

    Json::Value meta(Json::objectValue);
    meta["height"] = ctx.Height ();
    meta["timestamp"] = 1500000000;
    /* The POLYGON reseed branch (logic.cpp:2105) hashes block["hash"]; supply a
       deterministic dummy so it never reads a missing key.  */
    meta["hash"]
        = "0000000000000000000000000000000000000000000000000000000000000001";
    blockData["block"] = meta;

    return blockData;
  }

  /** Runs the full UpdateState pipeline (moves + ongoing tick) for one block. */
  void
  UpdateState (const std::string& movesStr)
  {
    PXLogic::UpdateState (db, rnd, ctx.Chain (),
                          BuildBlockData (ParseJson (movesStr)));
  }

  /** Raw sqlite3 handle behind the test database.  */
  sqlite3*
  RawHandle ()
  {
    sqlite3* h = nullptr;
    (*db).AccessDatabase ([&h] (sqlite3* raw) { h = raw; return 0; });
    return h;
  }

  /** Reads a single-integer PRAGMA from the raw handle.  */
  long long
  Pragma (const char* name)
  {
    sqlite3* h = RawHandle ();
    const std::string q = std::string ("PRAGMA ") + name + ";";
    sqlite3_stmt* st = nullptr;
    CHECK_EQ (sqlite3_prepare_v2 (h, q.c_str (), -1, &st, nullptr), SQLITE_OK);
    long long v = 0;
    if (sqlite3_step (st) == SQLITE_ROW)
      v = sqlite3_column_int64 (st, 0);
    sqlite3_finalize (st);
    return v;
  }

  /** Live (non-freelist) on-disk size of the main DB in bytes.  */
  long long
  MainDbBytes ()
  {
    const long long pageCount = Pragma ("page_count");
    const long long pageSize = Pragma ("page_size");
    const long long freelist = Pragma ("freelist_count");
    return (pageCount - freelist) * pageSize;
  }

  /** Creates `n` fresh players (each, realistically, also gets the tutorial
      recipes + fighters that XayaPlayer's new-account ctor mints).  */
  void
  SeedPlayers (const int n)
  {
    for (int i = 0; i < n; ++i)
      {
        xayaplayers.CreateNew ("p" + std::to_string (i), ctx.RoConfig (),
                               rnd);
        if (n >= 10000 && (i + 1) % 10000 == 0)
          std::cout << "    seeded " << (i + 1) << "/" << n << " players"
                    << std::endl;
      }
  }

  /** Gives the first `k` players an ongoing that never resolves within the
      measured window, so they are legitimately re-written every block (the
      O(K) work the fix must preserve).  */
  void
  SeedActiveOngoings (const int k)
  {
    for (int i = 0; i < k; ++i)
      {
        /* H3: ongoing now lives in the ongoing_operations table.  Give it a
           far-future absolute resolve height so it is never reached within the
           measured window (the O(K) work the event-driven path still does). */
        OngoingsTable ongoings (db);
        auto op = ongoings.CreateNew (ctx.Height ());
        op->SetOwner ("p" + std::to_string (i));
        op->SetHeight (ctx.Height () + 1000000000);
        op->MutableProto ().set_type (static_cast<uint32_t> (pxd::OngoingType::DECONSTRUCTION));
        op->MutableProto ().set_fighterdatabaseid (0);
        op.reset ();
      }
  }

  /**
   * Advances `m` blocks, wrapping each in a sqlite3 session exactly as
   * xaya::SQLiteGame does, and returns the mean per-block metrics.  Block 0 is
   * dropped as warm-up (page-cache / first-touch effects).
   */
  Metrics
  InstrumentedAdvance (const int m)
  {
    sqlite3* h = RawHandle ();

    /* Blocks to discard as warm-up before measuring (drops one-time post-seed
       transients so we measure true steady state).  Override: TF_LOADBENCH_WARMUP. */
    int warmup = 1;
    if (const char* w = std::getenv ("TF_LOADBENCH_WARMUP"))
      warmup = std::atoi (w);
    CHECK_LT (warmup, m);

    long long sumRows = 0;
    long long sumUndo = 0;
    double sumMs = 0.0;
    int counted = 0;
    std::map<std::string, long long> tableOps;
    std::map<std::string, long long> physOps;

    sqlite3_preupdate_hook (h, PreupdateCb, nullptr);

    for (int i = 0; i < m; ++i)
      {
        SetHeight (ctx.Height () + 1);

        /* Count physical writes only for measured (post-warm-up) blocks.  */
        g_physWrites = (i >= warmup) ? &physOps : nullptr;

        sqlite3_session* sess = nullptr;
        CHECK_EQ (sqlite3session_create (h, "main", &sess), SQLITE_OK);
        CHECK_EQ (sqlite3session_attach (sess, nullptr), SQLITE_OK);

        const int changesBefore = sqlite3_total_changes (h);
        const auto t0 = std::chrono::steady_clock::now ();

        UpdateState ("[]");

        const auto t1 = std::chrono::steady_clock::now ();
        const int changesAfter = sqlite3_total_changes (h);

        int nBuf = 0;
        void* pBuf = nullptr;
        CHECK_EQ (sqlite3session_changeset (sess, &nBuf, &pBuf), SQLITE_OK);

        const double ms
            = std::chrono::duration<double, std::milli> (t1 - t0).count ();

        if (i >= warmup)
          {
            sumRows += (changesAfter - changesBefore);
            sumUndo += nBuf;
            sumMs += ms;
            ++counted;

            /* Attribute changeset row-ops to their table.  */
            sqlite3_changeset_iter* it = nullptr;
            if (sqlite3changeset_start (&it, nBuf, pBuf) == SQLITE_OK)
              {
                while (sqlite3changeset_next (it) == SQLITE_ROW)
                  {
                    const char* zTab = nullptr;
                    int nCol = 0, op = 0, bIndirect = 0;
                    sqlite3changeset_op (it, &zTab, &nCol, &op, &bIndirect);
                    if (zTab != nullptr)
                      ++tableOps[zTab];
                  }
                sqlite3changeset_finalize (it);
              }
          }

        sqlite3_free (pBuf);
        sqlite3session_delete (sess);
      }

    g_physWrites = nullptr;
    sqlite3_preupdate_hook (h, nullptr, nullptr);

    CHECK_GT (counted, 0);
    Metrics res;
    res.rowsPerBlock = static_cast<double> (sumRows) / counted;
    res.undoBytesPerBlock = static_cast<double> (sumUndo) / counted;
    res.msPerBlock = sumMs / counted;
    for (const auto& kv : tableOps)
      res.rowsPerTable[kv.first] = static_cast<double> (kv.second) / counted;
    for (const auto& kv : physOps)
      res.physPerTable[kv.first] = static_cast<double> (kv.second) / counted;
    return res;
  }

};

/* ************************************************************************** */

TEST_P (LoadBenchTests, TickCost)
{
  if (std::getenv ("TF_LOADBENCH") == nullptr)
    GTEST_SKIP () << "set TF_LOADBENCH=1 to run the load benchmark";

  BenchConfig cfg = GetParam ();

  if (const char* maxN = std::getenv ("TF_LOADBENCH_MAXN"))
    if (cfg.players > std::atoi (maxN))
      GTEST_SKIP () << "players " << cfg.players << " > TF_LOADBENCH_MAXN";

  if (const char* mEnv = std::getenv ("TF_LOADBENCH_M"))
    cfg.blocks = std::atoi (mEnv);

  std::cout << "[ " << cfg.label << " ] seeding " << cfg.players
            << " players (" << cfg.active << " active) ..." << std::endl;

  SeedPlayers (cfg.players);
  SeedActiveOngoings (cfg.active);

  const long long dbBefore = MainDbBytes ();
  const Metrics m = InstrumentedAdvance (cfg.blocks);
  const long long dbAfter = MainDbBytes ();

  /* Human-readable summary.  */
  std::cout << "    rows/block=" << m.rowsPerBlock
            << "  undo-bytes/block=" << m.undoBytesPerBlock
            << "  ms/block=" << m.msPerBlock
            << "  main-db KiB: " << (dbBefore / 1024) << " -> "
            << (dbAfter / 1024) << std::endl;

  /* Net-change (undo changeset) attribution.  */
  std::cout << "    net rows/block:";
  for (const auto& kv : m.rowsPerTable)
    std::cout << "  " << kv.first << "=" << kv.second;
  std::cout << std::endl;

  /* PHYSICAL write attribution (incl. redundant identical-content rewrites)
     via the preupdate hook.  Only emitted when the hook actually fired -- some
     libsqlite3 builds export the symbol but compile the callback out, in which
     case rely on rows/block (sqlite3_total_changes) for the physical count.  */
  if (!m.physPerTable.empty ())
    {
      std::cout << "    phys writes/block:";
      for (const auto& kv : m.physPerTable)
        std::cout << "  " << kv.first << "=" << kv.second;
      std::cout << std::endl;
    }

  /* Machine-readable CSV row (grep '^LOADBENCH,' to extract).  */
  std::cout << "LOADBENCH," << cfg.label << "," << cfg.players << ","
            << cfg.active << "," << cfg.blocks << "," << m.rowsPerBlock << ","
            << m.undoBytesPerBlock << "," << m.msPerBlock << ","
            << (dbAfter / 1024) << std::endl;

  SUCCEED ();
}

INSTANTIATE_TEST_SUITE_P (Sweep, LoadBenchTests, testing::ValuesIn (kConfigs),
                          [] (const testing::TestParamInfo<BenchConfig>& info) {
                            /* gtest test names must be [A-Za-z0-9_].  */
                            std::string s (info.param.label);
                            for (char& c : s)
                              if (!std::isalnum (static_cast<unsigned char> (c)))
                                c = '_';
                            return s;
                          });

/* ************************************************************************** */

} // anonymous namespace
} // namespace pxd

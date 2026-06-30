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
#include "database/tournament.hpp"
#include "database/fighter.hpp"
#include "database/recipe.hpp"

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <json/json.h>

#include <sqlite3.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
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
    /* Measure the PRODUCTION chain (POLYGON), NOT REGTEST, and start just past
       the POLYGON genesis height so the production reseed/fork branches apply
       and the benchmark reflects the real per-block cost.  */
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
/* ================ SCALE / BLOCK-GROWTH stress benchmark =================== */
/*
   ScaleBenchTests measures how the per-block UpdateState cost GROWS over time
   under sustained full activity at scale, to confirm + quantify DEF3:

     completed-tournament rows are NEVER deleted, yet every block
       * ProcessTournaments      QueryAll-scans the whole `tournaments` table
       * ReopenMissingTournaments QueryAll-scans it once PER blueprint (~24x)
     => ~25 full scans (with proto deserialisation) of an unbounded, ever-
        growing table on every single block.  No other per-block full scan in
        the pipeline grows here (fighters stays ~2*N; rewards/recepies are not
        scanned per block), so a rising ms/block isolates the tournaments-table
        growth = the DEF3 vector.

   It drives a REALISTIC MIX through real moves:
     1. TOURNAMENTS (primary): N players in fixed pairs.  Each pair enters a
        2x2 re-enterable blueprint ("The Chocolate Chip Training Rounds",
        authoredid e694d5f8-..., NOT the FTUE one).  Roster fills (4) -> runs
        `duration` blocks -> Completed -> fighters freed -> re-enter.  Every
        completion adds one permanent Completed row.  Entries are STAGGERED so
        completions happen on (almost) every block (smooth growth curve).
     2. COOKING (secondary): a dedicated cohort cooks the non-bound starter
        recipe (one cook each, staggered across the run).
     3. MARKET (secondary): dedicated seller/buyer pairs list + buy a non-bound
        fighter (one trade each, staggered).

   Instance creation + fighter rating/sweetness "keep-eligible" resets are done
   in the harness OUTSIDE the timed region, so the measured ms/block is exactly
   PXLogic::UpdateState and the entries/cooks/trades themselves run as real
   moves inside it.  Env: TF_SCALE=1 (gate), TF_SCALE_N (players, def 2000),
   TF_SCALE_M (blocks, def 200).  check_PROGRAM, never auto-run.
*/

/** Re-enterable 2x2 blueprint: Rank1a, duration 15, sweetness 1..3.  */
constexpr const char* kScaleBlueprint = "e694d5f8-e454-7774-ca76-fc2637a9407f";
constexpr int kScaleDuration = 15;          /* blocks the tournament runs.   */
constexpr int kScaleCycle = kScaleDuration + 1;  /* full-roster -> re-enter.  */

/** The four candies required to cook the non-bound starter recipe 1bbc7d99.  */
constexpr const char* kCookRecipe = "1bbc7d99-7fce-24a4-c9a3-dfaf4b744efa";
const std::vector<std::string> kCookCandies = {
  "f9c0a4d2-f049-29a4-eb1e-2269de35798e",
  "7ddba6f2-b08a-f954-4a97-eae6af175005",
  "8aac4645-29cf-fe04-59c0-415508b0432e",
  "3d47dfc5-ce51-9c54-68f1-6819b251e6c7",
};

/** One sampled block of the time series.  */
struct ScaleSample
{
  int block = 0;
  double ms = 0.0;
  long long dbKiB = 0;
  int undoBytes = 0;
  long long tournaments = 0, fighters = 0, recepies = 0, rewards = 0;
  long long completed = -1;   /* filled only at growth-curve points.  */
};

class ScaleBenchTests : public DBTestWithSchema
{
protected:
  TestRandom rnd;
  ContextForTesting ctx;

  XayaPlayersTable xayaplayers;
  FighterTable fighters;
  TournamentTable tournaments;
  RecipeInstanceTable recipes;

  ScaleBenchTests ()
    : xayaplayers(db), fighters(db), tournaments(db), recipes(db)
  {
    ctx.SetChain (xaya::Chain::POLYGON);
    ctx.SetHeight (89246050);
  }

  sqlite3*
  RawHandle ()
  {
    sqlite3* h = nullptr;
    (*db).AccessDatabase ([&h] (sqlite3* raw) { h = raw; return 0; });
    return h;
  }

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
    return (Pragma ("page_count") - Pragma ("freelist_count"))
           * Pragma ("page_size");
  }

  /** SELECT COUNT(*) for a table (cheap; no proto deserialisation).  */
  long long
  CountRows (const char* table)
  {
    sqlite3* h = RawHandle ();
    const std::string q = std::string ("SELECT COUNT(*) FROM ") + table + ";";
    sqlite3_stmt* st = nullptr;
    CHECK_EQ (sqlite3_prepare_v2 (h, q.c_str (), -1, &st, nullptr), SQLITE_OK);
    long long v = 0;
    if (sqlite3_step (st) == SQLITE_ROW)
      v = sqlite3_column_int64 (st, 0);
    sqlite3_finalize (st);
    return v;
  }

  /** Ground-truth Completed-row count (deserialises every row; only used at the
      growth-curve sample points).  */
  long long
  CountCompletedTournaments ()
  {
    long long c = 0;
    auto res = tournaments.QueryAll ();
    while (res.Step ())
      {
        auto h = tournaments.GetFromResult (res, ctx.RoConfig ());
        if ((int) h->GetInstance ().state ()
            == (int) pxd::TournamentState::Completed)
          ++c;
        h.reset ();
      }
    return c;
  }

  /** Deterministic distinct 64-hex hash per height (reseeds the POLYGON RNG so
      combat outcomes vary realistically block to block).  */
  static std::string
  HashForHeight (const unsigned hgt)
  {
    char buf[65];
    std::snprintf (buf, sizeof (buf), "%064x", hgt);
    return std::string (buf);
  }

  Json::Value
  BuildBlockData (const Json::Value& moves)
  {
    Json::Value blockData (Json::objectValue);
    blockData["admin"] = Json::Value (Json::arrayValue);
    blockData["moves"] = moves;

    Json::Value meta (Json::objectValue);
    meta["height"] = ctx.Height ();
    meta["timestamp"] = 1500000000;
    meta["hash"] = HashForHeight (ctx.Height ());
    blockData["block"] = meta;
    return blockData;
  }

  /** Runs ONE block (moves + full ongoing/tournament tick) wrapped in a
      sqlite3 session exactly as xaya::SQLiteGame does, timing ONLY the
      UpdateState call.  Returns wall-ms and undo-changeset bytes.  */
  void
  RunMeasuredBlock (const Json::Value& blockData, double& msOut, int& undoOut)
  {
    sqlite3* h = RawHandle ();
    sqlite3_session* sess = nullptr;
    CHECK_EQ (sqlite3session_create (h, "main", &sess), SQLITE_OK);
    CHECK_EQ (sqlite3session_attach (sess, nullptr), SQLITE_OK);

    const auto t0 = std::chrono::steady_clock::now ();
    PXLogic::UpdateState (db, rnd, ctx.Chain (), blockData);
    const auto t1 = std::chrono::steady_clock::now ();

    int nBuf = 0;
    void* pBuf = nullptr;
    CHECK_EQ (sqlite3session_changeset (sess, &nBuf, &pBuf), SQLITE_OK);
    sqlite3_free (pBuf);
    sqlite3session_delete (sess);

    msOut = std::chrono::duration<double, std::milli> (t1 - t0).count ();
    undoOut = nBuf;
  }

  /** Forces a fighter back to a tournament-eligible baseline (status
      Available, rating 1000, sweetness 1) so a pair can re-enter the
      sweetness-1..3 blueprint wave after wave despite combat rating drift.
      Harness fixup -- runs OUTSIDE the timed region.  */
  void
  KeepEligible (const int fid)
  {
    auto f = fighters.GetById (fid, ctx.RoConfig ());
    if (f == nullptr)
      return;
    f->SetStatus (pxd::FighterStatus::Available);
    f->MutableProto ().set_rating (1000);
    f->MutableProto ().set_sweetness (1);
    f->MutableProto ().set_tournamentinstanceid (0);
    f.reset ();
  }

  /** Processes moves with NO block tick (for one-off setup like init).  */
  void
  ProcessMovesOnly (const Json::Value& moves)
  {
    MoveProcessor mvProc (db, rnd, ctx);
    mvProc.ProcessAll (moves);
  }

  /* Move builders ------------------------------------------------------- */

  static Json::Value
  TournamentEntryMove (const std::string& name, const int tid,
                       const int fid0, const int fid1)
  {
    Json::Value e (Json::objectValue);
    e["tid"] = tid;
    Json::Value fc (Json::arrayValue);
    fc.append (fid0);
    fc.append (fid1);
    e["fc"] = fc;
    Json::Value tm (Json::objectValue);
    tm["e"] = e;
    Json::Value mv (Json::objectValue);
    mv["tm"] = tm;
    Json::Value out (Json::objectValue);
    out["name"] = name;
    out["move"] = mv;
    return out;
  }

  static Json::Value
  CookMove (const std::string& name, const int rid)
  {
    Json::Value r (Json::objectValue);
    r["rid"] = rid;
    r["fid"] = 0;
    Json::Value ca (Json::objectValue);
    ca["r"] = r;
    Json::Value mv (Json::objectValue);
    mv["ca"] = ca;
    Json::Value out (Json::objectValue);
    out["name"] = name;
    out["move"] = mv;
    return out;
  }

  static Json::Value
  SellMove (const std::string& name, const int fid, const int price)
  {
    Json::Value s (Json::objectValue);
    s["fid"] = fid;
    s["d"] = 3;
    s["p"] = price;
    Json::Value f (Json::objectValue);
    f["s"] = s;
    Json::Value mv (Json::objectValue);
    mv["f"] = f;
    Json::Value out (Json::objectValue);
    out["name"] = name;
    out["move"] = mv;
    return out;
  }

  static Json::Value
  BuyMove (const std::string& name, const int fid)
  {
    Json::Value b (Json::objectValue);
    b["fid"] = fid;
    Json::Value f (Json::objectValue);
    f["b"] = b;
    Json::Value mv (Json::objectValue);
    mv["f"] = f;
    Json::Value out (Json::objectValue);
    out["name"] = name;
    out["move"] = mv;
    return out;
  }
};

/* ------------------------------------------------------------------------- */

TEST_F (ScaleBenchTests, BlockGrowthUnderFullActivity)
{
  if (std::getenv ("TF_SCALE") == nullptr)
    GTEST_SKIP () << "set TF_SCALE=1 to run the scale / block-growth benchmark";

  int N = 2000;
  if (const char* e = std::getenv ("TF_SCALE_N")) N = std::atoi (e);
  int M = 200;
  if (const char* e = std::getenv ("TF_SCALE_M")) M = std::atoi (e);
  CHECK_GE (N, 2);
  CHECK_GE (M, 8);

  const int pairs = N / 2;
  const int cookCount = std::min (N, std::max (50, N / 10));
  const int tradeCount = std::min (N / 2, std::max (20, N / 40));

  std::cout << "[ scale ] N=" << N << " players (" << pairs << " tournament "
            << "pairs), M=" << M << " blocks; cooks=" << cookCount
            << " trades=" << tradeCount << std::endl;

  /* ----- seed tournament players ----- */
  for (int i = 0; i < N; ++i)
    {
      xayaplayers.CreateNew ("p" + std::to_string (i), ctx.RoConfig (), rnd);
      if ((i + 1) % 20000 == 0)
        std::cout << "    seeded " << (i + 1) << "/" << N << " players"
                  << std::endl;
    }

  /* ----- seed cooking cohort (separate players; one cook each) ----- */
  std::vector<std::pair<std::string, int>> cookJobs;  /* (player, recipeId) */
  cookJobs.reserve (cookCount);
  for (int i = 0; i < cookCount; ++i)
    {
      const std::string nm = "ck" + std::to_string (i);
      auto p = xayaplayers.CreateNew (nm, ctx.RoConfig (), rnd);
      p->AddBalance (1000);
      for (const auto& candy : kCookCandies)
        p->GetInventory ().SetFungibleCount (
            BaseMoveProcessor::GetCandyKeyNameFromID (candy, ctx), 1000);
      p.reset ();
      const int rid
          = recipes.CreateNew (nm, kCookRecipe, ctx.RoConfig ())->GetId ();
      cookJobs.emplace_back (nm, rid);
    }

  /* ----- seed market cohort (seller+buyer pairs; one trade each) ----- */
  struct TradeJob { std::string seller, buyer; int fid; };
  std::vector<TradeJob> tradeJobs;
  tradeJobs.reserve (tradeCount);
  {
    Json::Value inits (Json::arrayValue);
    for (int i = 0; i < tradeCount; ++i)
      {
        const std::string sl = "sl" + std::to_string (i);
        const std::string by = "by" + std::to_string (i);
        xayaplayers.CreateNew (sl, ctx.RoConfig (), rnd)->AddBalance (100000);
        xayaplayers.CreateNew (by, ctx.RoConfig (), rnd)->AddBalance (100000);
        /* A non-account-bound, Available fighter the seller may list.  */
        const int rid
            = recipes.CreateNew (sl, kCookRecipe, ctx.RoConfig ())->GetId ();
        const int fid
            = fighters.CreateNew (sl, rid, ctx.RoConfig (), rnd)->GetId ();
        {
          auto f = fighters.GetById (fid, ctx.RoConfig ());
          f->MutableProto ().set_isaccountbound (false);
          f->SetStatus (pxd::FighterStatus::Available);
          f.reset ();
        }
        tradeJobs.push_back ({sl, by, fid});
        for (const std::string& who : {sl, by})
          {
            Json::Value init (Json::objectValue);
            Json::Value a (Json::objectValue), in (Json::objectValue);
            in["address"] = "CGUpAcjsb6MDktSYg8yRDxDutr7FhWtdWC";
            a["init"] = in;
            Json::Value mv (Json::objectValue);
            mv["a"] = a;
            init["name"] = who;
            init["move"] = mv;
            inits.append (init);
          }
      }
    if (!inits.empty ())
      ProcessMovesOnly (inits);
  }

  /* ----- discover each tournament player's two starter fighter ids via a
           single raw scan (no proto deserialisation) ----- */
  std::unordered_map<std::string, std::vector<int>> ownerFighters;
  ownerFighters.reserve (static_cast<size_t> (N) * 2);
  {
    sqlite3* h = RawHandle ();
    sqlite3_stmt* st = nullptr;
    CHECK_EQ (sqlite3_prepare_v2 (
                  h, "SELECT id, owner FROM fighters ORDER BY owner, id",
                  -1, &st, nullptr), SQLITE_OK);
    while (sqlite3_step (st) == SQLITE_ROW)
      {
        const int id = static_cast<int> (sqlite3_column_int64 (st, 0));
        const unsigned char* o = sqlite3_column_text (st, 1);
        if (o == nullptr) continue;
        ownerFighters[reinterpret_cast<const char*> (o)].push_back (id);
      }
    sqlite3_finalize (st);
  }

  /* Per-pair fighter ids [a0,a1,b0,b1] and a staggered first-entry offset.  */
  std::vector<std::array<int, 4>> pairFt (pairs);
  std::vector<int> nextEligible (pairs);
  int usablePairs = 0;
  for (int k = 0; k < pairs; ++k)
    {
      const auto& fa = ownerFighters["p" + std::to_string (2 * k)];
      const auto& fb = ownerFighters["p" + std::to_string (2 * k + 1)];
      if (fa.size () >= 2 && fb.size () >= 2)
        {
          pairFt[k] = {fa[0], fa[1], fb[0], fb[1]};
          ++usablePairs;
        }
      else
        pairFt[k] = {-1, -1, -1, -1};
      nextEligible[k] = k % kScaleCycle;        /* stagger first entries.  */
    }
  std::cout << "    usable tournament pairs: " << usablePairs << "/" << pairs
            << "; main-db at start: " << (MainDbBytes () / 1024) << " KiB"
            << std::endl;

  /* ----- non-vacuity counters ----- */
  long long entriesAttempted = 0, entriesConfirmed = 0;
  long long cooksAttempted = 0, cooksConfirmed = 0;
  long long tradesAttempted = 0, tradesConfirmed = 0;

  const int curveAt[5] = {0, M / 4, M / 2, (3 * M) / 4, M - 1};
  auto isCurve = [&] (const int b) {
    for (int i = 0; i < 5; ++i) if (curveAt[i] == b) return true;
    return false;
  };

  int cookCursor = 0, tradeCursor = 0;
  const int cooksPerBlock = std::max (1, (cookCount + M - 1) / M);
  const int tradesPerBlock = std::max (1, (tradeCount + M - 1) / M);

  std::vector<ScaleSample> series;
  series.reserve (M);

  for (int b = 0; b < M; ++b)
    {
      ctx.SetHeight (ctx.Height () + 1);

      Json::Value moves (Json::arrayValue);
      std::vector<std::pair<int, int>> thisBlockInstances;  /* (tid, pairIdx) */

      /* --- (1) tournaments: every due pair gets a fresh instance + entry --- */
      for (int k = 0; k < pairs; ++k)
        {
          if (pairFt[k][0] < 0 || nextEligible[k] > b)
            continue;
          for (int f = 0; f < 4; ++f)
            KeepEligible (pairFt[k][f]);     /* untimed eligibility reset.  */
          const int tid
              = tournaments.CreateNew (kScaleBlueprint, ctx.RoConfig ())
                    ->GetId ();
          moves.append (TournamentEntryMove ("p" + std::to_string (2 * k), tid,
                                             pairFt[k][0], pairFt[k][1]));
          moves.append (TournamentEntryMove ("p" + std::to_string (2 * k + 1),
                                             tid, pairFt[k][2], pairFt[k][3]));
          thisBlockInstances.emplace_back (tid, k);
          nextEligible[k] = b + kScaleCycle;
          ++entriesAttempted;
        }

      /* --- (2) cooking: a staggered batch each block --- */
      std::vector<std::pair<std::string, int>> thisBlockCooks;
      for (int c = 0; c < cooksPerBlock && cookCursor < cookCount; ++c, ++cookCursor)
        {
          moves.append (CookMove (cookJobs[cookCursor].first,
                                  cookJobs[cookCursor].second));
          thisBlockCooks.push_back (cookJobs[cookCursor]);
          ++cooksAttempted;
        }

      /* --- (3) market: a staggered list+buy each block --- */
      std::vector<TradeJob> thisBlockTrades;
      for (int t = 0; t < tradesPerBlock && tradeCursor < tradeCount; ++t, ++tradeCursor)
        {
          const auto& tj = tradeJobs[tradeCursor];
          moves.append (SellMove (tj.seller, tj.fid, 500));
          moves.append (BuyMove (tj.buyer, tj.fid));
          thisBlockTrades.push_back (tj);
          ++tradesAttempted;
        }

      /* --- measured block --- */
      double ms = 0.0;
      int undo = 0;
      RunMeasuredBlock (BuildBlockData (moves), ms, undo);

      /* --- confirm non-vacuity for this block --- */
      for (const auto& ti : thisBlockInstances)
        {
          auto h = tournaments.GetById (ti.first, ctx.RoConfig ());
          if (h != nullptr && h->GetInstance ().fighters_size () == 4)
            ++entriesConfirmed;
          if (h != nullptr) h.reset ();
        }
      for (const auto& cj : thisBlockCooks)
        {
          auto r = recipes.GetById (cj.second);
          /* a started cook clears the recipe instance's owner.  */
          if (r != nullptr && r->GetOwner ().empty ())
            ++cooksConfirmed;
          if (r != nullptr) r.reset ();
        }
      for (const auto& tj : thisBlockTrades)
        {
          auto f = fighters.GetById (tj.fid, ctx.RoConfig ());
          if (f != nullptr && f->GetOwner () == tj.buyer)
            ++tradesConfirmed;
          if (f != nullptr) f.reset ();
        }

      /* --- sample --- */
      ScaleSample s;
      s.block = b;
      s.ms = ms;
      s.undoBytes = undo;
      s.dbKiB = MainDbBytes () / 1024;
      s.tournaments = CountRows ("tournaments");
      s.fighters = CountRows ("fighters");
      s.recepies = CountRows ("recepies");
      s.rewards = CountRows ("rewards");
      if (isCurve (b))
        s.completed = CountCompletedTournaments ();
      series.push_back (s);

      if (M >= 20 && (b + 1) % std::max (1, M / 10) == 0)
        std::cout << "    block " << (b + 1) << "/" << M
                  << "  ms=" << s.ms << "  tournaments=" << s.tournaments
                  << "  db=" << s.dbKiB << " KiB" << std::endl;
    }

  /* ===================== report ===================== */
  const long long finalCompleted = CountCompletedTournaments ();

  std::cout << "\n==== SCALE BENCH (N=" << N << ", M=" << M << ") ====\n";
  std::cout << "NON-VACUITY:\n"
            << "  tournament pairs entered (attempted/confirmed): "
            << entriesAttempted << " / " << entriesConfirmed << "\n"
            << "  tournaments COMPLETED (final Completed rows):   "
            << finalCompleted << "\n"
            << "  cooks (attempted/confirmed):                    "
            << cooksAttempted << " / " << cooksConfirmed << "\n"
            << "  trades (attempted/confirmed):                   "
            << tradesAttempted << " / " << tradesConfirmed << "\n"
            << "  REJECTED moves (entries+cooks+trades):          "
            << (entriesAttempted - entriesConfirmed)
               + (cooksAttempted - cooksConfirmed)
               + (tradesAttempted - tradesConfirmed)
            << std::endl;

  std::cout << "\nGROWTH CURVE (block / ms / tournaments-rows / completed / "
            << "main-db KiB / fighters / rewards):\n";
  for (const int b : curveAt)
    {
      const ScaleSample& s = series[b];
      std::cout << "  ~" << (b * 100 / (M - 1)) << "%  blk=" << s.block
                << "  ms=" << s.ms << "  tourn=" << s.tournaments
                << "  completed=" << s.completed << "  db=" << s.dbKiB
                << "  fighters=" << s.fighters << "  rewards=" << s.rewards
                << std::endl;
    }

  /* first-10% vs last-10% mean ms (the growth factor).  */
  const int win = std::max (1, M / 10);
  double firstMs = 0.0, lastMs = 0.0;
  for (int i = 0; i < win; ++i) firstMs += series[i].ms;
  for (int i = 0; i < win; ++i) lastMs += series[M - 1 - i].ms;
  firstMs /= win;
  lastMs /= win;
  std::cout << "\nMEAN ms/block  first-10% (" << win << " blks)=" << firstMs
            << "   last-10%=" << lastMs << "   growth-factor="
            << (firstMs > 0 ? lastMs / firstMs : 0.0) << "x" << std::endl;

  /* machine-readable CSV.  */
  std::cout << "SCALEBENCH_HDR,N,block,ms,dbKiB,undoBytes,tournaments,"
            << "fighters,recepies,rewards,completed\n";
  for (const ScaleSample& s : series)
    std::cout << "SCALEBENCH," << N << "," << s.block << "," << s.ms << ","
              << s.dbKiB << "," << s.undoBytes << "," << s.tournaments << ","
              << s.fighters << "," << s.recepies << "," << s.rewards << ","
              << s.completed << "\n";
  std::cout << "SCALEBENCH_SUMMARY,N=" << N << ",M=" << M
            << ",firstMs=" << firstMs << ",lastMs=" << lastMs
            << ",growthFactor=" << (firstMs > 0 ? lastMs / firstMs : 0.0)
            << ",completed=" << finalCompleted
            << ",finalTournRows=" << series[M - 1].tournaments << std::endl;

  /* ===================== assertions ===================== */
  EXPECT_GT (entriesConfirmed, 0) << "no tournament entry was confirmed";
  EXPECT_GT (entriesConfirmed, entriesAttempted * 0.9)
      << "too many tournament entries were rejected -- benchmark is vacuous";
  EXPECT_GT (finalCompleted, 0)
      << "no tournament ever completed -- DEF3 vector not exercised";
  /* The Completed-row count must GROW substantially across the run.  */
  EXPECT_GT (series[curveAt[4]].completed, series[curveAt[1]].completed)
      << "completed-tournament count did not grow -- benchmark is vacuous";
  EXPECT_GT (cooksConfirmed, 0) << "no cook was confirmed";
  /* market is best-effort: warn but do not fail the benchmark.  */
  if (tradesAttempted > 0 && tradesConfirmed == 0)
    std::cout << "WARNING: no trade confirmed (market signal vacuous)"
              << std::endl;

  SUCCEED ();
}

/* ************************************************************************** */

} // anonymous namespace
} // namespace pxd

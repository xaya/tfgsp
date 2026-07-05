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
   Reorg / undo correctness harness -- "super good testing" for chain
   reorganisations, at scale, with real in-flight building activity.

   WHAT THIS PROVES
   ----------------
   A Xaya GSP must survive blockchain reorgs: when Xaya Core rolls back a block,
   xaya::SQLiteGame replays the per-block SQLite *session changeset* in reverse
   (sqlite3changeset_invert + sqlite3changeset_apply) to restore the previous
   state exactly, then re-applies the new branch.  If any state our rules write
   is not faithfully captured + reverted by that mechanism, a reorg silently
   forks the node off consensus.  This test exercises that exact mechanism --
   the same sqlite3 session primitive xaya::SQLiteGame uses -- directly against
   PXLogic::UpdateState, with:

     * SCALE     -- 1000s of seeded players (TF_REORG_N to crank to 10000s), so
                    undo is validated against a large, realistic state, not a toy.
     * BUILDING  -- real cook / expedition / deconstruction moves that spawn
                    multi-block "ongoing" operations, plus the tick that RESOLVES
                    them (mints fighters, grants rewards).  Reorging across a
                    resolution boundary is the genuinely hard case.
     * EXACTNESS -- after undo, a content hash of EVERY table (every row, every
                    column) must equal the pre-block snapshot.  Because the
                    schema gives every table a PRIMARY KEY, the session module
                    tracks them all; a table that fails to round-trip is a real
                    production reorg bug and is named in the failure.
     * EVENT-DRIVEN -- an idle block's changeset must stay O(active ongoings),
                    NOT O(players): the per-block undo log does not grow with the
                    player base.  This is the P-E1 efficiency guarantee, checked
                    here to survive a reorg as well.
     * DETERMINISM -- after undoing a block, re-applying the identical block must
                    reproduce the identical state (the reorg "re-attach" path).
                    On POLYGON the per-block RNG is reseeded from the block hash
                    (logic.cpp:2049), so a fixed hash makes the forward step
                    reproducible; this catches non-determinism from any OTHER
                    source (map ordering, time, float).

   CHAIN CHOICE: POLYGON, started just past the relaunch genesis.  REGTEST takes
   a test-only branch (logic.cpp:975) that rewrites every player's tier EVERY
   block -- which would both mask the event-driven property and make scale
   seeding pointless.  POLYGON is the production path and is what actually reorgs.

   ID-COUNTER FIDELITY: the production Database hands out ids from a DB-backed
   counter that a reorg changeset reverts; the in-memory TestDatabase counter
   (dbtest.cpp:40) is NOT reverted by a raw changeset apply.  For the
   re-apply/determinism checks we snapshot and restore that counter around the
   undo, faithfully emulating what production does -- otherwise re-applied rows
   would get fresh ids and spuriously differ.  Undo-only checks need no such
   help (they never re-insert).

   This is a SEPARATE binary (like goldenreplay_tests) for process isolation and
   because it needs -DSQLITE_ENABLE_SESSION.  It IS in TESTS (reorg safety is a
   correctness gate), defaulting to a fast moderate scale; crank it on demand:

       ./reorg_tests
       TF_REORG_N=20000 ./reorg_tests          # 10000s of players
       TF_REORG_N=20000 ./reorg_tests --gtest_filter='*Idle*'
*/

#include "logic.hpp"

#include "jsonutils.hpp"
#include "moveprocessor.hpp"
#include "testutils.hpp"

#include "database/dbtest.hpp"
#include "database/battlelosses.hpp"
#include "database/fighter.hpp"
#include "database/params.hpp"
#include "database/recipe.hpp"
#include "database/tournament.hpp"
#include "database/xayaplayer.hpp"

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <json/json.h>

#include <sqlite3.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace pxd
{
namespace
{

/* ************************************************************************** */
/* FNV-1a content hashing.  A 64-bit hash per table over its full row set is an
   effectively collision-free equality test, yet O(1) in memory regardless of
   how many players we seed -- so the same assertions hold at 1k or 100k.  */

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

/* ************************************************************************** */

/* Conflict callback for sqlite3changeset_apply.  Inverting a changeset captured
   on THIS database and applying it straight back must not conflict; we count
   any conflict so the test can assert zero (a conflict means the undo is
   unsound), and OMIT so the run continues to the -- then failing -- hash check. */
struct ApplyCtx
{
  int conflicts = 0;
};

extern "C" int
ReorgConflictCb (void* pCtx, int /*eConflict*/, sqlite3_changeset_iter* /*it*/)
{
  ++static_cast<ApplyCtx*> (pCtx)->conflicts;
  return SQLITE_CHANGESET_OMIT;
}

/* ************************************************************************** */

/**
 * Self-contained reorg fixture.  Mirrors the block-driving primitives of the
 * golden/loadbench fixtures (SetHeight / BuildBlockData / UpdateState) and adds
 * the session-changeset undo primitive plus whole-database content hashing.
 */
class ReorgTests : public DBTestWithSchema
{

protected:

  using IdT = Database::IdT;

  TestRandom rnd;
  ContextForTesting ctx;

  XayaPlayersTable xayaplayers;
  RecipeInstanceTable tbl2;
  FighterTable tbl3;
  TournamentTable tbl5;

  /** Fighter ids set up for the "building" actors (see SetupActors).  */
  struct Actors
  {
    int bobDeconstructFt = 0;
  };

  ReorgTests ()
    : xayaplayers(db), tbl2(db), tbl3(db), tbl5(db)
  {
    /* Production chain, started just past the relaunch genesis -- see the file
       header for why REGTEST is unsuitable here.  */
    ctx.SetChain (xaya::Chain::POLYGON);
    SetHeight (89246050);
  }

  void
  SetHeight (const unsigned h)
  {
    ctx.SetHeight (h);
  }

  /** Default seeded-player count; override with TF_REORG_N to reach 10000s.  */
  int
  ScaleN () const
  {
    if (const char* n = std::getenv ("TF_REORG_N"))
      return std::atoi (n);
    return 1000;
  }

  /**
   * Deterministic, distinct 64-hex block hash per height.  POLYGON reseeds the
   * per-block RNG from this (logic.cpp:2049): distinct per height (realistic),
   * yet fixed for a given height so re-applying a block reproduces it exactly.
   */
  static std::string
  HashForHeight (const unsigned h)
  {
    char buf[65];
    std::snprintf (buf, sizeof (buf), "%064x", h);
    return std::string (buf);
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
    meta["hash"] = HashForHeight (ctx.Height ());
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

  /** Advances n empty blocks, bumping height each block (ticks ongoings).  */
  void
  AdvanceBlocks (const unsigned n)
  {
    for (unsigned i = 0; i < n; ++i)
      {
        SetHeight (ctx.Height () + 1);
        UpdateState ("[]");
      }
  }

  /** Raw sqlite3 handle behind the test database.  */
  sqlite3*
  RawHandle ()
  {
    sqlite3* h = nullptr;
    (*db).AccessDatabase ([&h] (sqlite3* raw) { h = raw; return 0; });
    return h;
  }

  /* ----- id-counter snapshot/restore (production-fidelity, see header) ---- */

  /** Reads the in-memory next-id counter without consuming an id.  */
  IdT
  PeekNextId ()
  {
    const IdT v = db.GetNextId ();
    db.SetNextId (v);
    return v;
  }

  void
  RestoreNextId (const IdT v)
  {
    db.SetNextId (v);
  }

  /* ----- bulk seeding -------------------------------------------------- */

  /** Creates n background players (each also mints the tutorial recipes +
      fighters in XayaPlayer's new-account ctor) -- the scale dimension.  */
  void
  SeedPlayers (const int n)
  {
    for (int i = 0; i < n; ++i)
      {
        xayaplayers.CreateNew ("p" + std::to_string (i), ctx.RoConfig (), rnd);
        if (n >= 10000 && (i + 1) % 10000 == 0)
          std::cout << "    seeded " << (i + 1) << "/" << n << " players"
                    << std::endl;
      }
  }

  /**
   * Sets up a handful of "actor" players with proven-buildable state, mirroring
   * the (passing) golden-replay scenario.  Actors are created FIRST so domob
   * deterministically owns recipe instance 1.  Returns the fighter ids used by
   * the building moves.  All write handles are released before returning, so a
   * subsequent snapshot sees fully-flushed state.
   */
  Actors
  SetupActors ()
  {
    /* domob first -> owns recipes 1 & 2 and tutorial fighters 3 & 4.  */
    xayaplayers.CreateNew ("domob", ctx.RoConfig (), rnd)->AddBalance (1000);
    xayaplayers.CreateNew ("bob",   ctx.RoConfig (), rnd)->AddBalance (1000);

    Actors a;
    a.bobDeconstructFt = tbl3.CreateNew ("bob",  1, ctx.RoConfig (), rnd)->GetId ();

    /* domob gets the candy for First Recipe + a payout address.  */
    {
      auto p = xayaplayers.GetByName ("domob", ctx.RoConfig ());
      p->GetInventory ().SetFungibleCount ("Common_Gumdrop", 1);
      p->GetInventory ().SetFungibleCount ("Common_Icing", 1);
    }
    Process (R"([
      {"name": "domob",
       "move": {"a": {"init": {"address": "CGUpAcjsb6MDktSYg8yRDxDutr7FhWtdWC"}}}}
    ])");

    /* Re-assert domob's ownership of recipe instance 1 (the proven cook setup
       in logic_tests.cpp / goldenreplay does the same).  */
    tbl2.GetById (1)->SetOwner ("domob");

    return a;
  }

  /** The block of "building" moves: domob cooks First Recipe (resolves next
      block, minting a fighter) and bob deconstructs (a long ongoing that stays
      active) -- a short resolving ongoing plus a long live one.  (The tutorial
      expedition golden uses is hard-rejected off REGTEST, logic-side, so it
      cannot be exercised here.)  */
  std::string
  BuildMoves (const Actors& a) const
  {
    std::ostringstream s;
    s << "["
      << R"({"name":"domob","move":{"ca":{"r":{"rid":1,"fid":0}}}},)"
      << R"({"name":"bob","move":{"f":{"d":{"fid":)"
      << a.bobDeconstructFt << R"(}}}})"
      << "]";
    return s.str ();
  }

  /** Processes moves only (no block tick), for arranging preconditions.  */
  void
  Process (const std::string& str)
  {
    MoveProcessor mvProc(db, rnd, ctx);
    mvProc.ProcessAll (ParseJson (str));
  }

  int
  OngoingsOf (const std::string& name)
  {
    return xayaplayers.GetByName (name, ctx.RoConfig ())->GetOngoingsSize ();
  }

  /* ----- tournament permadeath setup (for the permadeath reorg tests) --- */

  /** The two teams' entered fighter ids + the tournament instance id, left one
      block short of resolution by SetupTournamentAtBrink().  */
  struct TournamentSetup
  {
    uint32_t tid = 0;
    uint32_t d1 = 0, d2 = 0;   /* domob's entered fighters */
    uint32_t a1 = 0, a2 = 0;   /* andy's entered fighters  */
  };

  /** Winner/loser (+ the loser's entered ids) read from a resolved tournament. */
  struct Resolved
  {
    std::string winner, loser;
    std::vector<uint32_t> loserFighters;
  };

  /**
   * Gives every fighter in teamA a single move of one type and every fighter in
   * teamB a single move of a DIFFERENT type.  Combat is move-type RPS, so every
   * cross-team pairing then resolves the same, non-draw way -> the tournament
   * always produces a decisive winner, independent of the per-block RNG (which
   * only shuffles move ORDER).  This removes the sole flakiness -- a tied score,
   * which fires no permadeath -- from an otherwise fully real resolution.
   */
  void
  AssignDistinctMoveTypes (const std::vector<uint32_t>& teamA,
                           const std::vector<uint32_t>& teamB)
  {
    std::string moveA, moveB;
    int typeA = -1;
    for (const auto& mb : ctx.RoConfig ()->fightermoveblueprints ())
      {
        const int ty = static_cast<int32_t> (mb.second.movetype ());
        if (moveA.empty ())
          { moveA = mb.second.authoredid (); typeA = ty; }
        else if (ty != typeA)
          { moveB = mb.second.authoredid (); break; }
      }
    CHECK (!moveA.empty () && !moveB.empty ())
        << "roconfig lacks two distinct fighter-move types";

    const auto setMove = [&] (const std::vector<uint32_t>& team,
                              const std::string& mv)
    {
      for (const uint32_t fid : team)
        {
          auto f = tbl3.GetById (fid, ctx.RoConfig ());
          f->MutableProto ().clear_moves ();
          f->MutableProto ().add_moves (mv);
          f.reset ();
        }
    };
    setMove (teamA, moveA);
    setMove (teamB, moveB);
  }

  /** Enters `who`'s two fighters into tournament `tid` via the real move
      processor (no block tick).  */
  void
  JoinTournament (const std::string& who, const uint32_t tid,
                  const uint32_t a, const uint32_t b)
  {
    std::ostringstream m;
    m << R"([{"name":")" << who << R"(","move":{"tm":{"e":{"tid":)" << tid
      << R"(,"fc":[)" << a << "," << b << R"(]}}}}])";
    Process (m.str ());
  }

  /**
   * Creates domob + andy, two non-account-bound fighters each, enters both full
   * teams into the rank-1 tournament, and ticks it to Running with exactly one
   * block left -- so the NEXT block resolves it (and fires permadeath).  The
   * outcome is forced decisive via AssignDistinctMoveTypes.
   */
  TournamentSetup
  SetupTournamentAtBrink ()
  {
    /* The Rank-1 entry tournament (2x2, sweetness 1-3, joincost 0, duration 15).
       NOT the FTUE tutorial tournament -- that one is hard-blocked from being
       joined off REGTEST (moveprocessor_activity.cpp), whereas this is the real
       entry-level PvP tournament a fresh sweetness-1 fighter can actually enter. */
    const std::string RANK1 = "e694d5f8-e454-7774-ca76-fc2637a9407f";

    const auto mkFighter = [&] (const std::string& owner) -> uint32_t
    {
      auto f = tbl3.CreateNew (owner, 1, ctx.RoConfig (), rnd);
      const uint32_t id = f->GetId ();
      f->MutableProto ().set_isaccountbound (false);
      f.reset ();
      return id;
    };

    xayaplayers.CreateNew ("domob", ctx.RoConfig (), rnd).reset ();
    xayaplayers.CreateNew ("andy",  ctx.RoConfig (), rnd).reset ();

    TournamentSetup s;
    s.d1 = mkFighter ("domob");
    s.d2 = mkFighter ("domob");
    s.a1 = mkFighter ("andy");
    s.a2 = mkFighter ("andy");
    AssignDistinctMoveTypes ({s.d1, s.d2}, {s.a1, s.a2});

    /* One tick creates the tournament instance (ReopenMissingTournaments). */
    AdvanceBlocks (1);
    auto tut = tbl5.GetByAuthIdName (RANK1, ctx.RoConfig ());
    CHECK (tut != nullptr) << "rank-1 tournament instance not auto-created";
    s.tid = tut->GetId ();
    tut.reset ();

    JoinTournament ("domob", s.tid, s.d1, s.d2);
    JoinTournament ("andy",  s.tid, s.a1, s.a2);

    /* Advance until Running with exactly one block remaining. */
    for (int guard = 0; guard < 64; ++guard)
      {
        auto trn = tbl5.GetById (s.tid, ctx.RoConfig ());
        const auto st = static_cast<pxd::TournamentState> (
            static_cast<int32_t> (trn->GetInstance ().state ()));
        const int bl = static_cast<int32_t> (trn->GetInstance ().blocksleft ());
        trn.reset ();
        if (st == pxd::TournamentState::Running && bl == 1)
          return s;
        CHECK (st != pxd::TournamentState::Completed)
            << "tournament resolved before it could be isolated for reorg";
        AdvanceBlocks (1);
      }
    ADD_FAILURE () << "tournament never reached the brink of resolution";
    return s;
  }

  /** Reads winner/loser + the loser's entered fighter ids from the resolved
      tournament.  The caller must first assert a decisive (non-empty) winner. */
  Resolved
  IdentifyResult (const TournamentSetup& s)
  {
    auto trn = tbl5.GetById (s.tid, ctx.RoConfig ());
    Resolved r;
    r.winner = trn->GetInstance ().winnerid ();
    trn.reset ();
    r.loser = (r.winner == "domob") ? "andy" : "domob";
    r.loserFighters = (r.winner == "domob")
        ? std::vector<uint32_t> {s.a1, s.a2}
        : std::vector<uint32_t> {s.d1, s.d2};
    return r;
  }

  /* ----- the reorg primitives ------------------------------------------ */

  /**
   * Runs one block via UpdateState wrapped in a sqlite3 session attached to all
   * tables (exactly as xaya::SQLiteGame does), and returns the captured
   * changeset bytes.  *netRowsOut, if given, receives the net row-change count.
   */
  std::vector<unsigned char>
  RunBlockCapture (const std::string& moves, int* netRowsOut = nullptr)
  {
    sqlite3* h = RawHandle ();

    sqlite3_session* sess = nullptr;
    CHECK_EQ (sqlite3session_create (h, "main", &sess), SQLITE_OK);
    CHECK_EQ (sqlite3session_attach (sess, nullptr), SQLITE_OK);

    const int before = sqlite3_total_changes (h);
    SetHeight (ctx.Height () + 1);
    UpdateState (moves);
    const int after = sqlite3_total_changes (h);

    int nBuf = 0;
    void* pBuf = nullptr;
    CHECK_EQ (sqlite3session_changeset (sess, &nBuf, &pBuf), SQLITE_OK);
    sqlite3session_delete (sess);

    std::vector<unsigned char> cs(
        static_cast<unsigned char*> (pBuf),
        static_cast<unsigned char*> (pBuf) + nBuf);
    sqlite3_free (pBuf);

    if (netRowsOut != nullptr)
      *netRowsOut = after - before;
    return cs;
  }

  /** Inverts a captured changeset and applies it, undoing the block.  Asserts
      the apply succeeds with zero conflicts (an unsound undo otherwise).  */
  void
  ApplyInverse (const std::vector<unsigned char>& cs)
  {
    sqlite3* h = RawHandle ();

    int nInv = 0;
    void* pInv = nullptr;
    CHECK_EQ (sqlite3changeset_invert (static_cast<int> (cs.size ()),
                                       cs.data (), &nInv, &pInv),
              SQLITE_OK);

    ApplyCtx actx;
    const int rc = sqlite3changeset_apply (h, nInv, pInv, nullptr,
                                           ReorgConflictCb, &actx);
    sqlite3_free (pInv);

    CHECK_EQ (rc, SQLITE_OK) << "changeset apply (undo) failed";
    EXPECT_EQ (actx.conflicts, 0) << "undo produced changeset conflicts";
  }

  /** Per-table operation counts inside a changeset (INSERT/UPDATE/DELETE rows).
      Used to assert the event-driven property (idle block writes O(1), not
      O(players), to the xayaplayers table).  */
  std::map<std::string, int>
  ChangesetOps (const std::vector<unsigned char>& cs)
  {
    std::map<std::string, int> ops;
    if (cs.empty ())
      return ops;

    sqlite3_changeset_iter* it = nullptr;
    if (sqlite3changeset_start (&it, static_cast<int> (cs.size ()),
                                const_cast<unsigned char*> (cs.data ()))
        == SQLITE_OK)
      {
        while (sqlite3changeset_next (it) == SQLITE_ROW)
          {
            const char* zTab = nullptr;
            int nCol = 0, op = 0, bIndirect = 0;
            sqlite3changeset_op (it, &zTab, &nCol, &op, &bIndirect);
            if (zTab != nullptr)
              ++ops[zTab];
          }
        sqlite3changeset_finalize (it);
      }
    return ops;
  }

  /* ----- whole-database content snapshot ------------------------------- */

  /**
   * Content hash of every user table: per row, FNV over (type, value) of each
   * column; row hashes are SORTED before folding, so the table hash is the hash
   * of the row *set* (robust to any physical reordering an undo might cause).
   * The row count is folded in too.  Returns table-name -> hash.
   */
  std::map<std::string, std::uint64_t>
  WholeDbHashes ()
  {
    sqlite3* h = RawHandle ();

    std::vector<std::string> tables;
    {
      sqlite3_stmt* st = nullptr;
      CHECK_EQ (sqlite3_prepare_v2 (h,
                  "SELECT name FROM sqlite_master WHERE type='table' "
                  "AND name NOT LIKE 'sqlite_%' ORDER BY name",
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
                    {
                      const sqlite3_int64 v = sqlite3_column_int64 (st, c);
                      rh = FnvMix (rh, &v, sizeof (v));
                      break;
                    }
                  case SQLITE_FLOAT:
                    {
                      const double v = sqlite3_column_double (st, c);
                      rh = FnvMix (rh, &v, sizeof (v));
                      break;
                    }
                  case SQLITE_TEXT:
                    {
                      const void* p = sqlite3_column_text (st, c);
                      const int n = sqlite3_column_bytes (st, c);
                      rh = FnvMix (rh, p, static_cast<std::size_t> (n));
                      break;
                    }
                  case SQLITE_BLOB:
                    {
                      const void* p = sqlite3_column_blob (st, c);
                      const int n = sqlite3_column_bytes (st, c);
                      rh = FnvMix (rh, p, static_cast<std::size_t> (n));
                      break;
                    }
                  case SQLITE_NULL:
                  default:
                    break;
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

  /**
   * Per-row content hashes for one table, keyed by its primary-key text (the
   * "name" column if present, else column 0).  A diagnostic for localising a
   * divergence to the exact row.
   */
  std::map<std::string, std::uint64_t>
  TableRowHashesByKey (const std::string& table)
  {
    sqlite3* h = RawHandle ();
    std::map<std::string, std::uint64_t> out;

    sqlite3_stmt* st = nullptr;
    const std::string q = "SELECT * FROM \"" + table + "\"";
    CHECK_EQ (sqlite3_prepare_v2 (h, q.c_str (), -1, &st, nullptr), SQLITE_OK);
    const int ncol = sqlite3_column_count (st);

    int keyCol = 0;
    for (int c = 0; c < ncol; ++c)
      {
        const char* nm = sqlite3_column_name (st, c);
        if (nm != nullptr && std::string (nm) == "name")
          {
            keyCol = c;
            break;
          }
      }

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
        const unsigned char* k = sqlite3_column_text (st, keyCol);
        const std::string key = (k == nullptr)
            ? ("#" + std::to_string (out.size ()))
            : reinterpret_cast<const char*> (k);
        out[key] = rh;
      }
    sqlite3_finalize (st);
    return out;
  }

  /** Asserts two whole-DB snapshots are equal, naming any diverging table.  */
  void
  ExpectStateEq (const std::map<std::string, std::uint64_t>& a,
                 const std::map<std::string, std::uint64_t>& b,
                 const std::string& msg)
  {
    if (a == b)
      return;

    ADD_FAILURE () << msg << ": whole-database state diverged";
    std::set<std::string> names;
    for (const auto& kv : a) names.insert (kv.first);
    for (const auto& kv : b) names.insert (kv.first);
    for (const auto& t : names)
      {
        const auto ia = a.find (t);
        const auto ib = b.find (t);
        const std::uint64_t va = (ia == a.end ()) ? 0 : ia->second;
        const std::uint64_t vb = (ib == b.end ()) ? 0 : ib->second;
        if (va != vb)
          ADD_FAILURE () << "  table '" << t << "' did not round-trip "
                         << "(" << va << " vs " << vb << ")";
      }
  }

};

/* ************************************************************************** */

/**
 * An idle block at scale must (a) write O(active ongoings)=0 player rows, NOT
 * O(players) -- the P-E1 event-driven guarantee -- and (b) reorg back to a
 * byte-identical state.  The cheap-undo property must survive reorgs.
 */
TEST_F (ReorgTests, IdleBlockReorgIsExactAndCheapAtScale)
{
  const int n = ScaleN ();
  std::cout << "[ idle-reorg ] seeding " << n << " players ..." << std::endl;
  SeedPlayers (n);

  /* Settle past the genesis-block tournament reopen so steady-state idle blocks
     are genuinely idle.  */
  AdvanceBlocks (3);

  const auto before = WholeDbHashes ();

  int netRows = 0;
  const auto cs = RunBlockCapture ("[]", &netRows);
  const auto ops = ChangesetOps (cs);

  std::cout << "    idle net rows/block=" << netRows
            << "  changeset bytes=" << cs.size ()
            << "  xayaplayers ops=" << ops.count ("xayaplayers")
            << std::endl;

  /* Event-driven: an idle block does NOT rewrite the player table, regardless
     of how many players exist.  This is the headline P-E1 win.  */
  EXPECT_EQ (ops.count ("xayaplayers"), 0u)
      << "idle block wrote player rows -- per-block cost scales with N";

  /* And the whole-block changeset stays a small constant, not O(N).  */
  EXPECT_LT (netRows, 64)
      << "idle block changeset scales with the player base";

  ApplyInverse (cs);
  ExpectStateEq (WholeDbHashes (), before, "idle reorg at scale");
}

/**
 * The core test: build ongoings, tick to RESOLVE them, then reorg both blocks
 * away (deep multi-block undo) and assert exact restoration -- and that
 * re-applying the same blocks reproduces the same state (re-attach path).
 */
TEST_F (ReorgTests, BuildResolveAndDeepReorgRestoresState)
{
  const int n = ScaleN ();
  const Actors actors = SetupActors ();
  std::cout << "[ build-reorg ] seeding " << n << " background players ..."
            << std::endl;
  SeedPlayers (n);

  AdvanceBlocks (3);

  /* Baseline (actors set up, nothing in flight).  Snapshot the id counter so we
     can faithfully re-apply later (see header).  */
  const auto b0 = WholeDbHashes ();
  const IdT idAtB0 = PeekNextId ();

  /* --- Block 1: building.  Spawns the cook + deconstruct ongoings. --- */
  const auto cs1 = RunBlockCapture (BuildMoves (actors));
  const auto b1 = WholeDbHashes ();
  const auto b1PlayerRows = TableRowHashesByKey ("xayaplayers");
  EXPECT_GE (OngoingsOf ("domob"), 1) << "cook did not start";
  EXPECT_GE (OngoingsOf ("bob"),   1) << "deconstruction did not start";
  {
    const auto ops = ChangesetOps (cs1);
    EXPECT_GT (ops.count ("ongoing_operations"), 0u)
        << "building did not touch ongoing_operations";
  }

  /* --- Block 2: idle tick.  domob's cook resolves (fighter minted); bob's
         15-block deconstruct stays active. --- */
  const auto cs2 = RunBlockCapture ("[]");
  const auto b2 = WholeDbHashes ();
  EXPECT_EQ (OngoingsOf ("domob"), 0) << "cook did not resolve";
  EXPECT_GE (OngoingsOf ("bob"),   1) << "deconstruction resolved too early";

  /* --- Reorg: peel block 2 then block 1, asserting each intermediate. --- */
  ApplyInverse (cs2);
  ExpectStateEq (WholeDbHashes (), b1, "undo resolution tick -> B1");
  EXPECT_GE (OngoingsOf ("domob"), 1) << "cook ongoing not restored by undo";

  ApplyInverse (cs1);
  ExpectStateEq (WholeDbHashes (), b0, "undo building -> B0");
  EXPECT_EQ (OngoingsOf ("domob"), 0) << "cook ongoing not removed by undo";
  EXPECT_EQ (OngoingsOf ("bob"),   0) << "deconstruct ongoing not removed by undo";

  /* --- Re-attach: replay the identical branch, restoring the id counter the
         way a production reorg reverts its DB-backed counter. --- */
  RestoreNextId (idAtB0);
  SetHeight (89246053);                 /* same heights as the first pass */
  const auto cs1b = RunBlockCapture (BuildMoves (actors));
  /* DIAGNOSTIC: name any player row that did not reproduce, to localise a
     re-apply divergence to its exact owner + cause.  */
  {
    const auto reRows = TableRowHashesByKey ("xayaplayers");
    for (const auto& kv : b1PlayerRows)
      {
        const auto it = reRows.find (kv.first);
        if (it == reRows.end ())
          std::cout << "    DIFF: player '" << kv.first << "' missing on re-apply"
                    << std::endl;
        else if (it->second != kv.second)
          std::cout << "    DIFF: player '" << kv.first << "' row diverged"
                    << std::endl;
      }
    for (const auto& kv : reRows)
      if (b1PlayerRows.find (kv.first) == b1PlayerRows.end ())
        std::cout << "    DIFF: player '" << kv.first << "' new on re-apply"
                  << std::endl;
  }
  ExpectStateEq (WholeDbHashes (), b1, "re-apply building is deterministic");
  const auto cs2b = RunBlockCapture ("[]");
  ExpectStateEq (WholeDbHashes (), b2, "re-apply resolution is deterministic");

  /* The replayed changesets must themselves match (same undo log on re-attach),
     which is what keeps a reorg-and-back a true no-op on disk.  */
  EXPECT_EQ (cs1, cs1b) << "re-applied building changeset differs";
  EXPECT_EQ (cs2, cs2b) << "re-applied resolution changeset differs";
}

/**
 * Deeper reorg: a stack of alternating building / idle / tick blocks, fully
 * unwound in reverse, must return exactly to the pre-stack state -- with every
 * intermediate snapshot matching on the way back down.
 */
TEST_F (ReorgTests, MultiBlockReorgUnwindsExactly)
{
  const int n = ScaleN ();
  const Actors actors = SetupActors ();
  SeedPlayers (n);
  AdvanceBlocks (3);

  /* Heights/blocks: build, tick, idle, tick, idle.  */
  const std::vector<std::string> branch = {
    BuildMoves (actors), "[]", "[]", "[]", "[]"
  };

  std::vector<std::map<std::string, std::uint64_t>> snaps;
  std::vector<std::vector<unsigned char>> sets;
  snaps.push_back (WholeDbHashes ());            /* snaps[0] == pre-stack */

  for (const auto& mv : branch)
    {
      sets.push_back (RunBlockCapture (mv));
      snaps.push_back (WholeDbHashes ());
    }

  /* Unwind in reverse; after undoing block i we must be back at snaps[i].  */
  for (int i = static_cast<int> (sets.size ()) - 1; i >= 0; --i)
    {
      ApplyInverse (sets[i]);
      ExpectStateEq (WholeDbHashes (), snaps[i],
                     "multi-block unwind to depth " + std::to_string (i));
    }
}

/**
 * EXCH-10: a completed fighter-exchange TRADE (real coins moving between two
 * players plus a fighter ownership transfer) must revert EXACTLY when the block
 * that contains it is reorged out.  This is the money-on-the-marketplace
 * equivalent of the building/undo test above: it drives a real list+buy through
 * the production move processor, captures the block's changeset, then inverts it
 * and asserts the whole database returns byte-for-byte to the pre-trade state --
 * ownership back to the seller, both balances restored, sale history gone.
 */
TEST_F (ReorgTests, TradeReorgRestoresStateExactly)
{
  /* Seller + buyer, both pre-existing so the trade block creates no new
     accounts (which would perturb the in-memory id counter).  */
  xayaplayers.CreateNew ("seller", ctx.RoConfig (), rnd)->AddBalance (1000);
  xayaplayers.CreateNew ("buyer",  ctx.RoConfig (), rnd)->AddBalance (5000);

  /* A fighter owned by the seller, made tradeable (CreateNew defaults the status
     to Available; clear the account-bound flag so it may be listed).  */
  const auto fid = tbl3.CreateNew ("seller", 1, ctx.RoConfig (), rnd)->GetId ();
  {
    auto ft = tbl3.GetById (fid, ctx.RoConfig ());
    ft->MutableProto ().set_isaccountbound (false);
    ft->SetStatus (pxd::FighterStatus::Available);
  }

  Process (R"([
    {"name":"seller","move":{"a":{"init":{"address":"CGUpAcjsb6MDktSYg8yRDxDutr7FhWtdWC"}}}},
    {"name":"buyer", "move":{"a":{"init":{"address":"CGUpAcjsb6MDktSYg8yRDxDutr7FhWtdWC"}}}}
  ])");

  SeedPlayers (ScaleN ());
  AdvanceBlocks (3);

  const auto b0 = WholeDbHashes ();
  const IdT idAtB0 = PeekNextId ();

  /* One block: seller lists the fighter, buyer buys it (same block -- the buy
     move processes after the sell move in array order).  */
  std::ostringstream blk;
  blk << "["
      << R"({"name":"seller","move":{"f":{"s":{"fid":)" << fid << R"(,"d":3,"p":500}}}},)"
      << R"({"name":"buyer","move":{"f":{"b":{"fid":)" << fid << R"(}}}})"
      << "]";
  const auto cs = RunBlockCapture (blk.str ());

  /* The trade really happened: ownership transferred and coins moved both ways
     (buyer debited the price, seller credited the reduced payout).  */
  {
    auto ft = tbl3.GetById (fid, ctx.RoConfig ());
    ASSERT_NE (ft, nullptr);
    EXPECT_EQ (ft->GetOwner (), "buyer") << "buy did not transfer ownership";
  }
  EXPECT_LT (xayaplayers.GetByName ("buyer", ctx.RoConfig ())->GetBalance (), 5000)
      << "buyer was not charged";
  EXPECT_GT (xayaplayers.GetByName ("seller", ctx.RoConfig ())->GetBalance (), 1000 - 100)
      << "seller was not paid";

  /* Reorg the trade out: every table must return exactly to its pre-trade
     content.  */
  ApplyInverse (cs);
  RestoreNextId (idAtB0);
  ExpectStateEq (WholeDbHashes (), b0, "undo fighter trade -> pre-trade state");

  {
    auto ft = tbl3.GetById (fid, ctx.RoConfig ());
    ASSERT_NE (ft, nullptr);
    EXPECT_EQ (ft->GetOwner (), "seller") << "ownership not reverted by undo";
  }
  EXPECT_EQ (xayaplayers.GetByName ("buyer",  ctx.RoConfig ())->GetBalance (), 5000)
      << "buyer balance not reverted by undo";
  EXPECT_EQ (xayaplayers.GetByName ("seller", ctx.RoConfig ())->GetBalance (), 1000)
      << "seller balance not reverted by undo";
}

/**
 * Tournament permadeath -- DESTROY branch (Change A/C).  Resolving a tournament
 * with capture disabled DELETES one of the losing team's entered fighters,
 * writes a battle_losses row, and bumps the loser's rewards_serial.  Reorging
 * that resolution block out must restore the deleted fighter row byte-for-byte,
 * drop the loss record, revert the serial -- i.e. the whole database returns
 * exactly to the pre-resolution snapshot.  A row DELETE round-trip is the case
 * the trade test (an ownership UPDATE) does not exercise.
 */
TEST_F (ReorgTests, TournamentDestroyReorgRestoresStateExactly)
{
  GameParams (db).SetParam ("tournament_loss_kills_enabled", 1);
  GameParams (db).SetParam ("tournament_capture_pct", 0);      /* always destroy */

  const auto s = SetupTournamentAtBrink ();

  /* Snapshot the exact pre-resolution state + the id counter (the resolution
     block mints rewards and reopens a tournament, both allocating ids). */
  const auto b0 = WholeDbHashes ();
  const IdT idAtB0 = PeekNextId ();
  const unsigned hBrink = ctx.Height ();     /* resolution runs at hBrink+1 */

  /* The resolution block: the tournament resolves and permadeath fires. */
  const auto cs = RunBlockCapture ("[]");
  const auto bResolved = WholeDbHashes ();

  const auto r = IdentifyResult (s);
  ASSERT_NE (r.winner, "") << "tournament drew -- no permadeath to test";

  /* Exactly one of the losing team was destroyed; the loss was recorded. */
  int alive = 0;
  for (const uint32_t fid : r.loserFighters)
    if (tbl3.GetById (fid, ctx.RoConfig ()) != nullptr) ++alive;
  EXPECT_EQ (alive, 1) << "exactly one of the losing team should be destroyed";
  {
    BattleLossesTable bl(db);
    auto lr = bl.QueryForOwner (r.loser);
    ASSERT_TRUE (lr.Step ()) << "no battle_losses row written";
    EXPECT_EQ (lr.Get<BattleLossResult::outcome> (), 0) << "not marked destroyed";
    EXPECT_EQ (lr.Get<BattleLossResult::opponent> (), r.winner);
  }

  /* Reorg the resolution out: the deleted fighter row, the loss record, the
     serial bump and everything else must round-trip exactly. */
  ApplyInverse (cs);
  RestoreNextId (idAtB0);
  ExpectStateEq (WholeDbHashes (), b0, "undo tournament destroy -> pre-resolution");

  for (const uint32_t fid : r.loserFighters)
    {
      auto f = tbl3.GetById (fid, ctx.RoConfig ());
      ASSERT_NE (f, nullptr) << "destroyed fighter not restored by undo";
      EXPECT_EQ (f->GetOwner (), r.loser) << "restored fighter has wrong owner";
    }
  {
    BattleLossesTable bl(db);
    EXPECT_FALSE (bl.QueryForOwner (r.loser).Step ())
        << "battle_losses row not reverted by undo";
  }

  /* Re-attach: replay the identical resolution block (same height -> same
     block-hash-seeded RNG).  The RNG-SELECTED permadeath outcome -- which
     fighter is the victim (rnd.NextInt(roster.size())) and destroy-vs-capture
     (rnd.NextInt(256)) -- must reproduce EXACTLY, so both the resolved state and
     the block's changeset come out byte-identical (a reorg-and-back is a true
     no-op on disk).  The sibling build/resolve test proves this for the
     cook/deconstruct RNG; this extends it to the permadeath draws.  (Cross-
     machine determinism is covered separately by golden replay / WASM==native.) */
  RestoreNextId (idAtB0);
  SetHeight (hBrink);
  const auto cs2 = RunBlockCapture ("[]");
  ExpectStateEq (WholeDbHashes (), bResolved,
                 "re-apply tournament destroy is deterministic");
  EXPECT_EQ (cs, cs2)
      << "re-applied resolution changeset differs (nondeterministic permadeath)";
}

/**
 * Tournament permadeath -- CAPTURE branch (Change A/C).  With capture forced on,
 * one of the losing team's fighters transfers to the winner (SetOwner) instead
 * of being destroyed.  Reorging the block must return ownership to the loser and
 * drop the loss record -- the whole database back to the pre-resolution
 * snapshot.  This is the tournament-path analogue of the trade reorg test.
 */
TEST_F (ReorgTests, TournamentCaptureReorgRestoresStateExactly)
{
  GameParams (db).SetParam ("tournament_loss_kills_enabled", 1);
  GameParams (db).SetParam ("tournament_capture_pct", 256);    /* always capture */

  const auto s = SetupTournamentAtBrink ();

  const auto b0 = WholeDbHashes ();
  const IdT idAtB0 = PeekNextId ();
  const unsigned hBrink = ctx.Height ();     /* resolution runs at hBrink+1 */

  const auto cs = RunBlockCapture ("[]");
  const auto bResolved = WholeDbHashes ();

  const auto r = IdentifyResult (s);
  ASSERT_NE (r.winner, "") << "tournament drew -- no permadeath to test";

  /* Exactly one losing fighter captured to the winner; both still exist. */
  int captured = 0;
  for (const uint32_t fid : r.loserFighters)
    {
      auto f = tbl3.GetById (fid, ctx.RoConfig ());
      ASSERT_NE (f, nullptr) << "capture must not destroy the fighter";
      if (f->GetOwner () == r.winner) ++captured;
    }
  EXPECT_EQ (captured, 1) << "exactly one of the losing team should be captured";
  {
    BattleLossesTable bl(db);
    auto lr = bl.QueryForOwner (r.loser);
    ASSERT_TRUE (lr.Step ()) << "no battle_losses row written";
    EXPECT_EQ (lr.Get<BattleLossResult::outcome> (), 1) << "not marked captured";
    EXPECT_EQ (lr.Get<BattleLossResult::opponent> (), r.winner);
  }

  /* Reorg the resolution out: ownership reverts, loss record gone, state exact. */
  ApplyInverse (cs);
  RestoreNextId (idAtB0);
  ExpectStateEq (WholeDbHashes (), b0, "undo tournament capture -> pre-resolution");

  for (const uint32_t fid : r.loserFighters)
    {
      auto f = tbl3.GetById (fid, ctx.RoConfig ());
      ASSERT_NE (f, nullptr);
      EXPECT_EQ (f->GetOwner (), r.loser) << "captured fighter's owner not reverted";
    }
  {
    BattleLossesTable bl(db);
    EXPECT_FALSE (bl.QueryForOwner (r.loser).Step ())
        << "battle_losses row not reverted by undo";
  }

  /* Re-attach: replay the identical resolution block; the RNG-selected victim +
     capture draws must reproduce exactly, so the resolved state and the block
     changeset are byte-identical.  (See the destroy test for the full rationale.) */
  RestoreNextId (idAtB0);
  SetHeight (hBrink);
  const auto cs2 = RunBlockCapture ("[]");
  ExpectStateEq (WholeDbHashes (), bResolved,
                 "re-apply tournament capture is deterministic");
  EXPECT_EQ (cs, cs2)
      << "re-applied resolution changeset differs (nondeterministic permadeath)";
}

/* ************************************************************************** */

} // anonymous namespace
} // namespace pxd

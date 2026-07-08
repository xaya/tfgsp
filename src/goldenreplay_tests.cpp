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
   Golden-replay regression harness.

   This test drives a fixed, deterministic sequence of moves through the
   *real* PXLogic::UpdateState pipeline on REGTEST (where the RNG is never
   re-seeded mid-run -- see logic.cpp ~2082) and snapshots the resulting
   full game state as canonical JSON.  The snapshot is compared byte-for-byte
   against a checked-in golden file (src/goldenreplay.golden.json).

   Purpose: it is the safety net for the Phase 2 efficiency rewrite (the
   ongoing/tick redesign that fixes the P-E1 DB bloat).  Because the rewrite
   must be rule-preserving, replaying this exact script after the rewrite must
   produce the *identical* golden snapshot.  Any divergence is a behaviour
   change and fails the test.

   Determinism rests on three pins: the REGTEST chain (no per-block reseed),
   the single fixed-seed TestRandom, and an identical move + height script.

   Regenerating the golden after an intentional, reviewed change:
       GOLDEN_REGEN=1 ./tests --gtest_filter='GoldenReplay*'
   then copy the freshly written src/goldenreplay.golden.json into the repo.
*/

#include "logic.hpp"

#include "gamestatejson.hpp"
#include "jsonutils.hpp"
#include "moveprocessor.hpp"
#include "testutils.hpp"

#include "database/dbtest.hpp"
#include "database/fighter.hpp"
#include "database/globaldata.hpp"
#include "database/recipe.hpp"
#include "database/reward.hpp"
#include "database/tournament.hpp"
#include "database/xayaplayer.hpp"
#include "database/params.hpp"
#include "database/battlelosses.hpp"

#include <xayautil/jsonutils.hpp>

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <json/json.h>

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

namespace pxd
{
namespace
{

/** Relative path (from the test's working dir = builddir/src) to the golden. */
constexpr const char* GOLDEN_FILE = "goldenreplay.golden.json";
/** Where the actual snapshot is always dumped, for diffing on failure. */
constexpr const char* ACTUAL_FILE = "goldenreplay.actual.json";

/* ************************************************************************** */

/**
 * Self-contained fixture mirroring the essential bits of PXLogicTests
 * (which lives only in logic_tests.cpp and is therefore not reusable across
 * translation units).  It drives the public static PXLogic::UpdateState
 * against an in-memory test database.
 */
class GoldenReplayTests : public DBTestWithSchema
{

protected:

  TestRandom rnd;
  ContextForTesting ctx;

  XayaPlayersTable xayaplayers;
  RecipeInstanceTable tbl2;
  FighterTable tbl3;
  RewardsTable tbl4;
  TournamentTable tbl5;

  GameStateJson converter;
  GlobalData gd;

  GoldenReplayTests ()
    : xayaplayers(db), tbl2(db), tbl3(db), tbl4(db), tbl5(db),
      converter(db, ctx), gd(db)
  {
    SetHeight (42);
  }

  /** Sets the block height used for the next processed block.  */
  void
  SetHeight (const unsigned h)
  {
    ctx.SetHeight (h);
  }

  /** Processes moves only (no block tick), as the move processor would.  */
  void
  Process (const std::string& str)
  {
    MoveProcessor mvProc(db, rnd, ctx);
    mvProc.ProcessAll (ParseJson (str));
  }

  /** Process moves with a WCHI dev-address payment attached (for the paid pc/nr
   *  verbs), mirroring MoveProcessorTests::ProcessWithDevPayment.  */
  void
  ProcessWithDevPayment (const std::string& str, const Amount amount)
  {
    Json::Value val = ParseJson (str);
    const std::string devAddr = ctx.RoConfig ()->params ().dev_address ();
    for (auto& entry : val)
      entry["out"][devAddr] = xaya::ChiAmountToJson (amount);

    MoveProcessor mvProc(db, rnd, ctx);
    mvProc.ProcessAll (val);
  }

  /** Builds block data (admin + moves + block meta) for UpdateState.  */
  Json::Value
  BuildBlockData (const Json::Value& moves)
  {
    Json::Value blockData(Json::objectValue);
    blockData["admin"] = Json::Value (Json::arrayValue);
    blockData["moves"] = moves;

    Json::Value meta(Json::objectValue);
    meta["height"] = ctx.Height ();
    meta["timestamp"] = 1500000000;
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

  /**
   * Captures the entire observable game state as canonical JSON.
   *
   * FullState() is the single full-state serializer (deterministic ORDER BY
   * everywhere).  We strip statehex/stateblock -- those are process-level
   * static globals updated only every 100 blocks (gamestatejson.hpp), so they
   * are stale/irrelevant at an arbitrary height.
   */
  std::string
  BuildSnapshot ()
  {
    Json::Value full = converter.FullState ();
    full.removeMember ("statehex");
    full.removeMember ("stateblock");

    Json::Value root(Json::objectValue);
    root["state"] = full;

    Json::StreamWriterBuilder wbuilder;
    wbuilder["indentation"] = "  ";
    wbuilder["enableYAMLCompatibility"] = false;
    return Json::writeString (wbuilder, root);
  }

  /* ----- the scripted scenario ------------------------------------------ */

  /**
   * Runs the whole deterministic scenario.  It exercises every multi-block
   * "ongoing" operation -- the P-E1 hot path that the Phase 2 efficiency
   * rewrite replaces -- and deliberately leaves the final state with a mix of
   * RESOLVED ongoings (locks generation/outcome determinism: cooked fighter,
   * expedition rewards) and still-ACTIVE ongoings (locks the in-flight ongoing
   * representation that the storage redesign must preserve).  It also lists a
   * fighter on the exchange.  Move payloads, blueprint ids and block durations
   * are all lifted from the existing passing unit tests in logic_tests.cpp.
   *
   * All preconditions are arranged with direct table writes (as the existing
   * tests do); the interesting *actions* go through the real MoveProcessor /
   * PXLogic::UpdateState pipeline.
   */
  void
  RunScenario ()
  {
    /* NB: always call ctx.RoConfig() inline -- it returns a reference into a
       unique_ptr member that RefreshInstances() (triggered by SetHeight in
       AdvanceBlocks) rebuilds, so a captured reference would dangle, and
       RoConfig's copy ctor is deleted so it cannot be cached by value.  */

    /* domob is created FIRST, so it auto-owns recipes id 1 & 2 and tutorial
       fighters id 3 & 4 (see XayaPlayersTable::CreateNew).  */
    xayaplayers.CreateNew ("domob", ctx.RoConfig (), rnd)->AddBalance (1000);
    xayaplayers.CreateNew ("andy",  ctx.RoConfig (), rnd)->AddBalance (1000);
    xayaplayers.CreateNew ("bob",   ctx.RoConfig (), rnd)->AddBalance (1000);
    xayaplayers.CreateNew ("carol", ctx.RoConfig (), rnd)->AddBalance (1000);
    xayaplayers.CreateNew ("dave",  ctx.RoConfig (), rnd)->AddBalance (1000);

    /* A known Available fighter for each non-domob player (recipe id 1 is used
       only as the generation template -- it stays a valid row after domob
       consumes its ownership by cooking).  */
    const int andyFt  = tbl3.CreateNew ("andy",  1, ctx.RoConfig (), rnd)->GetId ();
    const int bobFt   = tbl3.CreateNew ("bob",   1, ctx.RoConfig (), rnd)->GetId ();
    const int carolFt = tbl3.CreateNew ("carol", 1, ctx.RoConfig (), rnd)->GetId ();
    const int daveFt  = tbl3.CreateNew ("dave",  1, ctx.RoConfig (), rnd)->GetId ();

    /* Give domob the candy for First Recipe and a payout address.  */
    {
      auto a = xayaplayers.GetByName ("domob", ctx.RoConfig ());
      a->GetInventory ().SetFungibleCount ("Common_Gumdrop", 1);
      a->GetInventory ().SetFungibleCount ("Common_Icing", 1);
    }
    Process (R"([
      {"name": "domob",
       "move": {"a": {"init": {"address": "CGUpAcjsb6MDktSYg8yRDxDutr7FhWtdWC"}}}}
    ])");

    /* Sweetener preconditions for carol's fighter (rating + sweetness tier and
       the consumed items), mirroring SweetenerCookAndProperRewardsClaimed.  */
    {
      auto ft = tbl3.GetById (carolFt, ctx.RoConfig ());
      ft->MutableProto ().set_rating (1210);
      ft->MutableProto ().set_sweetness ((int) pxd::Sweetness::Bittersweet);
    }
    {
      auto a = xayaplayers.GetByName ("carol", ctx.RoConfig ());
      a->GetInventory ().SetFungibleCount ("Sweetener_R2", 1);
      a->GetInventory ().SetFungibleCount ("Common_Icing", 10);
      a->GetInventory ().SetFungibleCount ("Common_Fruit Slice", 10);
    }

    /* ---- Start every ongoing type (no block has ticked yet) ------------- */

    /* COOK_RECIPE (domob, First Recipe -> resolves in 1 block).  Re-assert
       domob's ownership of recipe instance 1 first (the proven cook setup in
       logic_tests.cpp does the same).  */
    tbl2.GetById (1)->SetOwner ("domob");
    Process (R"([
      {"name": "domob", "move": {"ca": {"r": {"rid": 1, "fid": 0}}}}
    ])");

    /* EXPEDITION that resolves in 1 block, leaving 3 unclaimed rewards.  */
    Process ("[{\"name\":\"andy\",\"move\":{\"exp\":{\"f\":"
             "{\"eid\":\"c064e7f7-acbf-4f74-fab8-cccd7b2d4004\",\"fid\":"
             + std::to_string (andyFt) + "}}}}]");

    /* DECONSTRUCTION (bob, 15 blocks -> still active at snapshot).  */
    Process ("[{\"name\":\"bob\",\"move\":{\"f\":{\"d\":{\"fid\":"
             + std::to_string (bobFt) + "}}}}]");

    /* COOK_SWEETENER (carol, 22 blocks -> still active at snapshot).  */
    Process ("[{\"name\":\"carol\",\"move\":{\"ca\":{\"s\":"
             "{\"sid\":\"d596403b-b76f-52c4-6956-4bfd55231de0\",\"fid\":"
             + std::to_string (carolFt) + ",\"rid\":1}}}}]");

    /* EXPEDITION (dave, blueprint takes 17 blocks -> still active).  */
    Process ("[{\"name\":\"dave\",\"move\":{\"exp\":{\"f\":"
             "{\"eid\":\"93ad71bb-cd8f-dc24-7885-2c3fd0013245\",\"fid\":"
             + std::to_string (daveFt) + "}}}}]");

    /* Guard: every ongoing actually started (catches a broken precondition
       turning into a silently-missing ongoing).  */
    EXPECT_EQ (xayaplayers.GetByName ("domob", ctx.RoConfig ())->GetOngoingsSize (), 1);
    EXPECT_EQ (xayaplayers.GetByName ("andy",  ctx.RoConfig ())->GetOngoingsSize (), 1);
    EXPECT_EQ (xayaplayers.GetByName ("bob",   ctx.RoConfig ())->GetOngoingsSize (), 1);
    EXPECT_EQ (xayaplayers.GetByName ("carol", ctx.RoConfig ())->GetOngoingsSize (), 1);
    EXPECT_EQ (xayaplayers.GetByName ("dave",  ctx.RoConfig ())->GetOngoingsSize (), 1);

    /* ---- Tick a few blocks: short ongoings resolve, long ones stay live -- */
    AdvanceBlocks (3);

    /* domob's COOK_RECIPE and andy's EXPEDITION have resolved; bob/carol/dave
       remain active.  */
    EXPECT_EQ (xayaplayers.GetByName ("domob", ctx.RoConfig ())->GetOngoingsSize (), 0);
    EXPECT_EQ (xayaplayers.GetByName ("andy",  ctx.RoConfig ())->GetOngoingsSize (), 0);
    EXPECT_EQ (xayaplayers.GetByName ("bob",   ctx.RoConfig ())->GetOngoingsSize (), 1);
    EXPECT_EQ (xayaplayers.GetByName ("carol", ctx.RoConfig ())->GetOngoingsSize (), 1);
    EXPECT_EQ (xayaplayers.GetByName ("dave",  ctx.RoConfig ())->GetOngoingsSize (), 1);

    /* ---- Exchange: domob lists a (now non-account-bound) tutorial fighter - */
    {
      auto ft = tbl3.GetById (4, ctx.RoConfig ());
      ft->MutableProto ().set_isaccountbound (false);
    }
    Process (R"([
      {"name": "domob", "move": {"f": {"s": {"fid": 4, "d": 3, "p": 500}}}}
    ])");

    /* One more block so the listing and all per-block bookkeeping settle.  */
    AdvanceBlocks (1);

    /* ================================================================
       Extended launch-regression coverage (2026-07-08): pin the high-risk
       move types the golden never exercised — pc (WCHI crystal buy), nr (WCHI
       name reroll), transfigure (fighter/candy/recipe sacrifice), and a
       tournament RESOLVE WITH PERMADEATH in BOTH branches (capture + destroy).
       Appended AFTER the existing ongoings so their earlier RNG/state is
       unchanged; the extra blocks resolve the still-in-flight ongoings, so a
       fresh expedition at the very end re-establishes the "active ongoing" pin.
       Every added move is GUARDED (EXPECT) so a silent rejection can't pin
       false coverage.  Helper to mint a non-account-bound (loseable/tradeable)
       fighter is inlined per block. */

    const std::string CANDY = "c2774273-0cd4-2494-fa02-76cffe1ef1ef";   // any real candy authoredid

    /* ---- pc: buy a crystal bundle (WCHI-paid, resolves immediately) ---- */
    {
      auto a = xayaplayers.GetByName ("andy", ctx.RoConfig ());
      const Amount balBefore = a->GetBalance ();
      a.reset ();
      ProcessWithDevPayment (R"([{"name":"andy","move":{"pc":"T1"}}])", 14000000);   // 0.14 COIN
      a = xayaplayers.GetByName ("andy", ctx.RoConfig ());
      EXPECT_GT (a->GetBalance (), balBefore);   // guard: crystals minted
      a.reset ();
    }

    /* ---- nr: reroll a fighter's name (WCHI-paid) ---- */
    {
      auto ft = tbl3.GetById (andyFt, ctx.RoConfig ());
      ft->MutableProto ().set_isnamererolled (false);
      ft.reset ();
      std::ostringstream m;
      m << R"([{"name":"andy","move":{"nr":)" << andyFt << R"(}}])";
      ProcessWithDevPayment (m.str (), 14000000);
      ft = tbl3.GetById (andyFt, ctx.RoConfig ());
      EXPECT_TRUE (ft->GetProto ().isnamererolled ());   // guard: reroll consumed
      ft.reset ();
    }

    /* ---- transfigure: sacrifice a fighter + candy + recipe; target flips Transfiguring -> Available ---- */
    {
      xayaplayers.CreateNew ("erin", ctx.RoConfig (), rnd)->AddBalance (100);
      const auto mkFree = [&] (const std::string& owner) -> uint32_t {
        auto f = tbl3.CreateNew (owner, 1, ctx.RoConfig (), rnd);
        f->MutableProto ().set_isaccountbound (false);
        const uint32_t id = f->GetId ();
        f.reset ();
        return id;
      };
      const uint32_t tgt = mkFree ("erin");
      const uint32_t sac = mkFree ("erin");
      const uint32_t rec = tbl2.CreateNew ("erin",
          "5864a19b-c8c0-2d34-eaef-9455af0baf2c", ctx.RoConfig ())->GetId ();   // First Recipe
      {
        auto e = xayaplayers.GetByName ("erin", ctx.RoConfig ());
        e->GetInventory ().SetFungibleCount (
            BaseMoveProcessor::GetCandyKeyNameFromID (CANDY, ctx), 50);
        e.reset ();
      }
      std::ostringstream m;
      m << R"([{"name":"erin","move":{"f":{"t":{"fid":)" << tgt
        << R"(,"o":3,"if":[)" << sac << R"(],"ic":[{"a":10,"n":")" << CANDY
        << R"("}],"ir":[)" << rec << R"(]}}}}])";
      Process (m.str ());
      /* These guards pin parse-accept + the fuel-cost DESTRUCTION (sacrifice fighter + candy +
         recipe are consumed) + the Transfiguring->Available status cycle. An all-Common sacrifice
         may land fuelPowerUnit<1, so the inner o:3 move-reroll ROLL itself is not asserted here
         (that success path is covered by CoinOperationTests.TransfigureWrongValues). */
      {
        auto t = tbl3.GetById (tgt, ctx.RoConfig ());
        EXPECT_EQ (t->GetStatus (), FighterStatus::Transfiguring);   // guard: accepted
        t.reset ();
      }
      EXPECT_EQ (tbl3.GetById (sac, ctx.RoConfig ()), nullptr);      // guard: sacrifice deleted
      AdvanceBlocks (1);                                             // Transfiguring -> Available
    }

    const auto mkLoseable = [&] (const std::string& owner) -> uint32_t {
      auto f = tbl3.CreateNew (owner, 1, ctx.RoConfig (), rnd);
      f->MutableProto ().set_isaccountbound (false);
      const uint32_t id = f->GetId ();
      f.reset ();
      return id;
    };
    const auto joinTournament = [&] (const std::string& who, uint32_t tid,
                                     uint32_t x, uint32_t y) {
      std::ostringstream m;
      m << R"([{"name":")" << who << R"(","move":{"tm":{"e":{"tid":)" << tid
        << R"(,"fc":[)" << x << "," << y << R"(]}}}}])";
      Process (m.str ());
    };
    const auto countLoss = [&] (const std::string& a, const std::string& b, int outcome) -> int {
      BattleLossesTable losses (db);
      int n = 0;
      for (const std::string& owner : {a, b})
        {
          auto r = losses.QueryForOwner (owner);
          while (r.Step ())
            if (static_cast<int> (r.Get<BattleLossResult::outcome> ()) == outcome) ++n;
        }
      return n;
    };

    /* ---- Tournament permadeath, CAPTURE branch (capture_pct=256) on the 2x2 tutorial tier ---- */
    {
      GameParams (db).SetParam ("tournament_loss_kills_enabled", 1);
      GameParams (db).SetParam ("tournament_capture_pct", 256);   // roll < 256 always -> capture
      xayaplayers.CreateNew ("eve", ctx.RoConfig (), rnd).reset ();
      const uint32_t e1 = mkLoseable ("eve"), e2 = mkLoseable ("eve");
      xayaplayers.CreateNew ("finn", ctx.RoConfig (), rnd).reset ();
      const uint32_t f1 = mkLoseable ("finn"), f2 = mkLoseable ("finn");
      AdvanceBlocks (1);   // ReopenMissingTournaments ensures a Listed instance exists (may pre-date this block)
      const uint32_t tid =
          tbl5.GetByAuthIdName ("cbd2e78a-37ce-b864-793d-8dd27788a774", ctx.RoConfig ())->GetId ();
      joinTournament ("eve", tid, e1, e2);
      joinTournament ("finn", tid, f1, f2);
      AdvanceBlocks (3);   // roster full -> Running (blocksleft 2, Min1 unscaled) -> resolves
      EXPECT_EQ (countLoss ("eve", "finn", 1), 1);   // loser-side: exactly one captured (outcome 1)
      EXPECT_EQ (countLoss ("eve", "finn", 2), 1);   // winner-side: one capture-gain record (outcome 2)
    }

    /* ---- Tournament permadeath, DESTROY branch (capture_pct=0) on the Chocolate Chip 2x2 tier ---- */
    {
      GameParams (db).SetParam ("tournament_capture_pct", 0);   // roll < 0 never -> destroy
      xayaplayers.CreateNew ("gale", ctx.RoConfig (), rnd).reset ();
      const uint32_t g1 = mkLoseable ("gale"), g2 = mkLoseable ("gale");
      xayaplayers.CreateNew ("hana", ctx.RoConfig (), rnd).reset ();
      const uint32_t h1 = mkLoseable ("hana"), h2 = mkLoseable ("hana");
      AdvanceBlocks (1);
      const uint32_t tid =
          tbl5.GetByAuthIdName ("e694d5f8-e454-7774-ca76-fc2637a9407f", ctx.RoConfig ())->GetId ();
      joinTournament ("gale", tid, g1, g2);
      joinTournament ("hana", tid, h1, h2);
      AdvanceBlocks (17);   // Chocolate Chip Duration 15 (Min1 unscaled) + slack -> resolves
      EXPECT_EQ (countLoss ("gale", "hana", 0), 1);   // exactly one destroyed
      GameParams (db).SetParam ("tournament_capture_pct", 128);   // restore seeded default
    }

    /* ---- Re-establish a fresh in-flight ongoing so the snapshot keeps the
           "resolved + still-active" mix, decoupled from the block count above. ---- */
    {
      xayaplayers.CreateNew ("iris", ctx.RoConfig (), rnd).reset ();
      /* Recipe instance 1 is deleted when bob's deconstruction resolves above (the extra blocks
         push past its resolution height; ResolveDeconstruction deletes the fighter's source
         recipe, logic_resolve.cpp:543), so build iris's fighter from a FRESH recipe instance as
         the stat template — otherwise CreateNew can't resolve the template and yields a
         sweetness-0 fighter that fails the expedition's MinSweetness gate. */
      const int irisRec = tbl2.CreateNew ("iris",
          "5864a19b-c8c0-2d34-eaef-9455af0baf2c", ctx.RoConfig ())->GetId ();
      const uint32_t fid = tbl3.CreateNew ("iris", irisRec, ctx.RoConfig (), rnd)->GetId ();
      std::ostringstream m;
      m << R"([{"name":"iris","move":{"exp":{"f":{"eid":"93ad71bb-cd8f-dc24-7885-2c3fd0013245","fid":)"
        << fid << R"(}}}}])";
      Process (m.str ());
      auto a = xayaplayers.GetByName ("iris", ctx.RoConfig ());
      EXPECT_EQ (a->GetOngoingsSize (), 1);   // guard: active expedition pinned in the snapshot
      a.reset ();
    }
  }

};

/* ************************************************************************** */

namespace
{

std::string
ReadFileIfExists (const std::string& path, bool& found)
{
  std::ifstream in(path, std::ios::binary);
  if (!in)
    {
      found = false;
      return "";
    }
  found = true;
  std::ostringstream ss;
  ss << in.rdbuf ();
  return ss.str ();
}

void
WriteFile (const std::string& path, const std::string& content)
{
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  CHECK (out) << "Failed to open " << path << " for writing";
  out << content;
}

} // anonymous namespace

TEST_F (GoldenReplayTests, FullScenario)
{
  RunScenario ();
  const std::string actual = BuildSnapshot ();

  /* Always dump the actual snapshot so a mismatch can be diffed.  */
  WriteFile (ACTUAL_FILE, actual);

  if (std::getenv ("GOLDEN_REGEN") != nullptr)
    {
      WriteFile (GOLDEN_FILE, actual);
      LOG (WARNING) << "GOLDEN_REGEN set: wrote " << actual.size ()
                    << " bytes to " << GOLDEN_FILE;
      SUCCEED () << "regenerated golden";
      return;
    }

  bool found = false;
  const std::string golden = ReadFileIfExists (GOLDEN_FILE, found);
  ASSERT_TRUE (found)
      << "Golden file " << GOLDEN_FILE << " missing.  Generate it with "
         "GOLDEN_REGEN=1 and commit it.  Actual snapshot written to "
      << ACTUAL_FILE;

  EXPECT_EQ (actual, golden)
      << "Game-state snapshot diverged from golden.  Diff " << ACTUAL_FILE
      << " against " << GOLDEN_FILE << ".  If the change is intentional and "
         "reviewed, regenerate with GOLDEN_REGEN=1.";
}

/* ************************************************************************** */

} // anonymous namespace
} // namespace pxd

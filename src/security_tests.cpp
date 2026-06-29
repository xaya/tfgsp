/*
    GSP for the tf blockchain game
    Copyright (C) 2020  Autonomous Worlds Ltd

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
  Regression tests for the Security Pass A hardening.  Each test feeds an
  attacker-controlled move that previously halted the chain (throw / abort /
  out-of-bounds) or drained the economy, and asserts the GSP now handles it
  gracefully and deterministically.  See docs/security-audit.md.
*/

#include "moveprocessor.hpp"
#include "logic.hpp"

#include "jsonutils.hpp"
#include "testutils.hpp"

#include "database/dbtest.hpp"
#include "database/recipe.hpp"
#include "database/fighter.hpp"
#include "database/xayaplayer.hpp"

#include <xayautil/jsonutils.hpp>

#include <gtest/gtest.h>

#include <json/json.h>

#include <string>

namespace pxd
{
namespace
{

class SecurityTests : public DBTestWithSchema
{

protected:

  TestRandom rnd;
  ContextForTesting ctx;
  XayaPlayersTable xayaplayers;
  RecipeInstanceTable tbl2;
  FighterTable tbl3;

  explicit SecurityTests ()
    : xayaplayers(db), tbl2(db), tbl3(db)
  {}

  /** Runs the given JSON-array of moves through the confirmed move pipeline. */
  void
  Process (const std::string& str)
  {
    MoveProcessor mvProc(db, rnd, ctx);
    mvProc.ProcessAll (ParseJson (str));
  }

  /** Runs a full PXLogic::UpdateState block with the given moves array. */
  void
  UpdateState (const std::string& movesStr)
  {
    Json::Value blockData(Json::objectValue);
    blockData["admin"] = Json::Value (Json::arrayValue);
    blockData["moves"] = ParseJson (movesStr);

    Json::Value meta(Json::objectValue);
    meta["height"] = ctx.Height ();
    meta["timestamp"] = 1500000000;
    blockData["block"] = meta;

    PXLogic::UpdateState (db, rnd, ctx.Chain (), blockData);
  }

  /**
   * Like Process, but attaches an "out" entry paying the given amount to the
   * configured dev address on every move.
   */
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
};

/* ************************************************************************** */
/* Crash-class (C1-C5): chain-halt prevention.                                */

TEST_F (SecurityTests, AdversarialMovesDoNotHaltChain)
{
  /* Each of these previously threw Json::LogicError, aborted via CHECK, or
     read out of bounds in the move pipeline; all must now be handled without
     crashing the process (a throw would propagate and fail the test; an abort
     or SEGV would kill the binary).  */
  const std::vector<std::string> moves = {
      R"([{"name":"atk","move":[0]}])",                                  // C1 scalar element
      R"([{"name":"atk","move":["x"]}])",                                // C1
      R"([{"name":"atk","move":[1,"x",[],true,null]}])",                 // C1 mixed
      R"([{"name":"atk","move":{"ps":","}}])",                           // C5 OOB stoi
      R"([{"name":"atk","move":{"ps":"x,"}}])",                          // C5
      R"([{"name":"atk","move":{"exp":{"f":{"eid":"x","fid":["a"]}}}}])", // C2 asInt on array elem
      R"([{"name":"atk","move":{"exp":{"f":{"eid":"x","fid":[3000000000]}}}}])", // C2
      R"([{"name":"atk","move":{"ca":{"d":{"rid":["x"]}}}}])",           // C3
      R"([{"name":"atk","move":{"ca":{"d":{"rid":[{}]}}}}])",            // C3
      R"([{"name":"atk","move":{"exp":{"c":{"eid":[{}]}}}}])",           // C4 asString on object elem
      R"([{"name":"atk","move":{"exp":{"c":{"eid":[[1]]}}}}])",          // C4
  };

  for (const auto& mv : moves)
    EXPECT_NO_THROW (Process (mv)) << "move: " << mv;

  /* The attacker's account was still created (the pipeline ran to completion).  */
  EXPECT_TRUE (xayaplayers.GetByName ("atk", ctx.RoConfig ()) != nullptr);
}

TEST_F (SecurityTests, ValidArrayMoveStillProcesses)
{
  /* The non-object-skip guard must not break legitimate array-form moves: a
     valid sub-move inside an array (mixed with junk elements) still applies.  */
  Process (R"([
    {"name": "domob", "move": [
       0,
       {"a": {"x": 42, "init": {"address": "CGUpAcjsb6MDktSYg8yRDxDutr7FhWtdWC"}}},
       "junk"
    ]}
  ])");

  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig ());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetProto ().address (), "CGUpAcjsb6MDktSYg8yRDxDutr7FhWtdWC");
}

/* ************************************************************************** */
/* Cook-recipe fighter-deletion guard (griefing + dangling-reference halt).   */

TEST_F (SecurityTests, CookCannotDeleteAnotherPlayersFighter)
{
  /* Attacker bob owns a quality-0 recipe (does NOT validate its fid) plus the
     balance + candy to cook it.  */
  xayaplayers.CreateNew ("bob", ctx.RoConfig (), rnd)->AddBalance (100);
  auto bob = xayaplayers.GetByName ("bob", ctx.RoConfig ());
  bob->GetInventory ().SetFungibleCount ("Common_Gumdrop", 1);
  bob->GetInventory ().SetFungibleCount ("Common_Icing", 1);
  bob.reset ();
  tbl2.GetById (1)->SetOwner ("bob");

  /* Victim alice owns an Available fighter.  */
  xayaplayers.CreateNew ("alice", ctx.RoConfig (), rnd);
  auto vf = tbl3.CreateNew ("alice", 1, ctx.RoConfig (), rnd);
  const auto victimId = vf->GetId ();
  vf->SetStatus (pxd::FighterStatus::Available);
  vf.reset ();

  /* Bob cooks his recipe but points fid at alice's fighter.  */
  Process (R"([{"name":"bob","move":{"ca":{"r":{"rid":1,"fid":)"
           + std::to_string (victimId) + R"(}}}}])");

  auto stillThere = tbl3.GetById (victimId, ctx.RoConfig ());
  ASSERT_TRUE (stillThere != nullptr) << "alice's fighter was deleted by bob";
  EXPECT_EQ (stillThere->GetOwner (), "alice");
}

TEST_F (SecurityTests, CookCannotDeleteBusyFighter)
{
  /* A fighter that is a tournament participant (status != Available) must not
     be consumable by a cook, or its dangling roster reference halts the
     chain at tournament resolution.  */
  xayaplayers.CreateNew ("bob", ctx.RoConfig (), rnd)->AddBalance (100);
  auto bob = xayaplayers.GetByName ("bob", ctx.RoConfig ());
  bob->GetInventory ().SetFungibleCount ("Common_Gumdrop", 1);
  bob->GetInventory ().SetFungibleCount ("Common_Icing", 1);
  bob.reset ();
  tbl2.GetById (1)->SetOwner ("bob");

  auto bf = tbl3.CreateNew ("bob", 1, ctx.RoConfig (), rnd);
  const auto busyId = bf->GetId ();
  bf->SetStatus (pxd::FighterStatus::Tournament);
  bf.reset ();

  Process (R"([{"name":"bob","move":{"ca":{"r":{"rid":1,"fid":)"
           + std::to_string (busyId) + R"(}}}}])");

  auto stillThere = tbl3.GetById (busyId, ctx.RoConfig ());
  ASSERT_TRUE (stillThere != nullptr) << "a busy fighter was cook-deleted";
}

/* ************************************************************************** */
/* C8: crystal-bundle replay must mint exactly one bundle per payment.        */

TEST_F (SecurityTests, CrystalBundleArrayReplayMintsOnce)
{
  xayaplayers.CreateNew ("domob", ctx.RoConfig (), rnd)
      ->AddBalance (250 + ctx.RoConfig ()->params ().starting_crystals ());

  Process (R"([
    {"name": "domob", "move": {"a": {"x": 42, "init": {"address": "CGUpAcjsb6MDktSYg8yRDxDutr7FhWtdWC"}}}}
  ])");

  /* Advance one block before the paid moves. */
  UpdateState ("[]");

  auto before = xayaplayers.GetByName ("domob", ctx.RoConfig ());
  const Amount balanceBefore = before->GetBalance ();
  before.reset ();

  /* One payment, but the move replays the same crystal bundle twice.  Only the
     first may mint; the second sees the consumed budget and is rejected.  */
  ProcessWithDevPayment (R"([{
    "name": "domob",
    "move": [{"pc": "T1"}, {"pc": "T1"}]
  }])", 0.14 * COIN);

  auto after = xayaplayers.GetByName ("domob", ctx.RoConfig ());
  ASSERT_TRUE (after != nullptr);
  EXPECT_EQ (after->GetBalance () - balanceBefore, 100)
      << "crystal bundle was minted more than once for a single payment";
}

/* ************************************************************************** */
/* Pass B: attacker-controlled arrays are capped, not super-linear.           */

TEST_F (SecurityTests, OversizedMoveArraysAreCappedNotFatal)
{
  /* A huge expedition party array (> MAX_MOVE_ARRAY) is rejected before the
     dedup / per-element lookups; a huge sub-move array (> MAX_SUBMOVES) is
     capped.  Neither may crash or stall.  */
  std::string bigFid = "[";
  for (int i = 0; i < 5000; ++i)
    bigFid += (i ? "," : "") + std::to_string (i);
  bigFid += "]";
  EXPECT_NO_THROW (Process (
      R"([{"name":"atk","move":{"exp":{"f":{"eid":"x","fid":)" + bigFid
      + R"(}}}}])"));

  std::string bigArr = "[";
  for (int i = 0; i < 5000; ++i)
    bigArr += std::string (i ? "," : "") + R"({"pg":"x"})";
  bigArr += "]";
  EXPECT_NO_THROW (Process (
      R"([{"name":"atk","move":)" + bigArr + R"(}])"));

  EXPECT_TRUE (xayaplayers.GetByName ("atk", ctx.RoConfig ()) != nullptr);
}

/* ************************************************************************** */

} // anonymous namespace
} // namespace pxd

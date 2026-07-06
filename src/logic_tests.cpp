/*
    GSP for the tf blockchain game
    Copyright (C) 2019-2021  Autonomous Worlds Ltd

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

#include "logic.hpp"
#include "database/recipe.hpp"
#include "moveprocessor.hpp"

#include "jsonutils.hpp"
#include "testutils.hpp"

#include "database/dbtest.hpp"
#include "database/xayaplayer.hpp"
#include "database/reward.hpp"
#include "database/params.hpp"
#include "database/battlelosses.hpp"
#include "database/tournament.hpp"

#include "proto/activity_rewards.pb.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <json/json.h>

#include <string>
#include <vector>
#include <sstream>


namespace pxd
{

/* ************************************************************************** */

/**
 * Test fixture for testing PXLogic::UpdateState.  It sets up a test database
 * independent from SQLiteGame, so that we can more easily test custom
 * situations as needed.
 */
class PXLogicTests : public DBTestWithSchema
{

protected:

  TestRandom rnd;
  ContextForTesting ctx;
  
  XayaPlayersTable xayaplayers;
  RecipeInstanceTable tbl2;
  FighterTable tbl3;
  RewardsTable tbl4;
  TournamentTable tbl5;

  /** GameStateJson instance used in testing.  */
  GameStateJson converter;  
  
  GlobalData gd;
  
  
  PXLogicTests () : xayaplayers(db), tbl2(db), tbl3(db), tbl4(db), tbl5(db), converter(db, ctx), gd(db)
  {
    SetHeight (42);
  }
  
  /**
   * Expects that the current state matches the given one, after parsing
   * the expected state's string as JSON.  Furthermore, the expected value
   * is assumed to be *partial* -- keys that are not present in the expected
   * value may be present with any value in the actual object.  If a key is
   * present in expected but has value null, then it must not be present
   * in the actual data, though.
   */  
  void
  ExpectStateTournamentsOnlyJson (const std::string& expectedStr, const std::string& userName)
  {
    const Json::Value actual = converter.UserTournaments (userName);
    LOG (WARNING) << "Actual tournament JSON for the game state:\n" << actual;
    LOG (WARNING) << "EXPECTED tournament JSON for the game state:\n" << expectedStr;
    ASSERT_TRUE (PartialJsonEqual (actual, ParseJson (expectedStr)));
  }   
  
  /**
   * Processes the given data (which is passed as string and converted to
   * JSON before processing it).
   */
  void
  Process (const std::string& str)
  {
    MoveProcessor mvProc(db, rnd, ctx);
    mvProc.ProcessAll (ParseJson (str));
  }  

  /**
   * Sets the block height for processing the next block.
   */
  void
  SetHeight (const unsigned h)
  {
    ctx.SetHeight (h);
  }

  /**
   * Calls PXLogic::UpdateState with our test instances of the database,
   * params and RNG.  The given string is parsed as JSON array and used
   * as moves in the block data.
   */
  void
  UpdateState (const std::string& movesStr)
  {
    UpdateStateJson (ParseJson (movesStr));
  }

   /**
   * Builds a blockData JSON value from the given moves.
   */
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

  /**
   * Updates the state as with UpdateState, but with moves given
   * already as JSON value.
   */
  void
  UpdateStateJson (const Json::Value& moves)
  {
    /* H3: ongoings now resolve by ABSOLUTE block height, so each processed block
       must advance the height (real blocks always do).  This mirrors the
       goldenreplay harness, which bumps height before every block. */
    ctx.SetHeight (ctx.Height () + 1);
    UpdateStateWithData (BuildBlockData (moves));
  }

  /**
   * Calls PXLogic::UpdateState with the given block data and our params, RNG
   * and stuff.  This is a more general variant of UpdateState(std::string),
   * where the block data can be modified to include extra stuff (e.g. a block
   * height of our choosing).
   */
  void
  UpdateStateWithData (const Json::Value& blockData)
  {
    PXLogic::UpdateState (db, rnd, ctx.Chain (), blockData);
  }

  /**
   * Calls PXLogic::UpdateState with the given moves and a provided (mocked)
   * FameUpdater instance.
   */
  void
  UpdateStateWithFame (const std::string& moveStr)
  {
    const auto blockData = BuildBlockData (ParseJson (moveStr));
    PXLogic::UpdateState (db, rnd, ctx, blockData);
  }

  /**
   * Calls game-state validation.
   */
  void
  ValidateState ()
  {
    PXLogic::ValidateStateSlow (db, ctx);
  }

  /** Outcome of running the 2v2 tutorial tournament to resolution.  */
  struct TwoPlayerTournament
  {
    std::string winner;                  // the resolved winnerid
    std::string loser;                   // the other player
    std::vector<uint32_t> loserEntered;  // the loser's two entered fighter ids
  };

  /**
   * Creates two players ("domob", "andy"), each entering two fighters (made
   * account-bound iff `accountBound`) into the tutorial tournament, ticks it to
   * resolution, and reports the resolved winner/loser + the loser's entered ids.
   * Mirrors TournamentResolvedTest's join/resolve sequence.  Callers set the
   * permadeath knobs via GameParams before this returns has resolved (this sets
   * none itself, so seeded defaults apply unless the caller overrode them).
   */
  TwoPlayerTournament
  RunTwoPlayerTournament (bool accountBound = false,
                          bool distinctRatings = false)
  {
    const std::string TUT = "cbd2e78a-37ce-b864-793d-8dd27788a774";

    auto mkFighter = [&] (const std::string& owner) -> uint32_t
    {
      auto f = tbl3.CreateNew (owner, 1, ctx.RoConfig (), rnd);
      const uint32_t id = f->GetId ();
      f->MutableProto ().set_isaccountbound (accountBound);
      f.reset ();
      return id;
    };

    xayaplayers.CreateNew ("domob", ctx.RoConfig (), rnd).reset ();
    const uint32_t d1 = mkFighter ("domob");
    const uint32_t d2 = mkFighter ("domob");

    UpdateState ("[]");

    auto tut = tbl5.GetByAuthIdName (TUT, ctx.RoConfig ());
    const uint32_t TID = tut->GetId ();
    tut.reset ();

    const auto joinMove = [&] (const std::string& who, uint32_t a, uint32_t b)
    {
      std::ostringstream m;
      m << R"([{"name": ")" << who << R"(", "move": {"tm": {"e": {"tid": )"
        << TID << R"(, "fc": [)" << a << "," << b << "]}}}}]";
      Process (m.str ());
    };

    joinMove ("domob", d1, d2);

    xayaplayers.CreateNew ("andy", ctx.RoConfig (), rnd).reset ();
    const uint32_t a1 = mkFighter ("andy");
    const uint32_t a2 = mkFighter ("andy");

    UpdateState ("[]");
    joinMove ("andy", a1, a2);

    if (distinctRatings)
      {
        /* Give each team one clearly stronger (2000) and one weaker (1000)
           fighter so the permadeath shield has an unambiguous highest-rated to
           protect.  The stronger one is the SECOND-created (HIGHER-id) fighter --
           loserEntered[1] -- so a rating-based shield and a (hypothetical) lowest-
           id shield would protect DIFFERENT fighters; this makes the test actually
           prove the shield keys on rating, not on id order.  The 1000-point gap
           dwarfs any ELO delta combat applies before the shield reads the rating. */
        for (const uint32_t hi : {d2, a2})
          tbl3.GetById (hi, ctx.RoConfig ())->MutableProto ().set_rating (2000);
        for (const uint32_t lo : {d1, a1})
          tbl3.GetById (lo, ctx.RoConfig ())->MutableProto ().set_rating (1000);
      }

    for (unsigned i = 0; i < 3; ++i)
      UpdateState ("[]");

    auto trn = tbl5.GetById (TID, ctx.RoConfig ());
    const std::string winner = trn->GetInstance ().winnerid ();
    trn.reset ();

    TwoPlayerTournament out;
    out.winner = winner;
    out.loser = (winner == "domob") ? "andy" : "domob";
    out.loserEntered = (winner == "domob")
        ? std::vector<uint32_t> {a1, a2}
        : std::vector<uint32_t> {d1, d2};
    return out;
  }

  /* ---- 3-team (3v3v3) permadeath fixtures -------------------------------- */

  /** The three teams' entered fighter ids + the instance id, left one block
      short of resolution by SetupThreeTeamAtBrink().  "winner" sweeps; "bob" and
      "carl" are the two losing teams (ascending map order: bob < carl).  winner
      is always 3x Speedy and carl 3x Heavy; bob's move types + ratings are
      caller-supplied (the `bobArms` passed to SetupThreeTeamAtBrink). */
  struct ThreeTeam
  {
    uint32_t tid = 0;
    std::vector<uint32_t> winner;   // 3x Speedy -> effectively beats every Heavy/Blocking opponent
    std::vector<uint32_t> bob;      // caller-armed (see bobArms) -- the primary losing team under test
    std::vector<uint32_t> carl;     // 3x Heavy (all net -5, a 3-way tie) -- the second losing team
  };

  /** Returns any fighter-move blueprint authoredid whose movetype == `type`. */
  std::string
  MoveOfType (const int type)
  {
    for (const auto& mb : ctx.RoConfig ()->fightermoveblueprints ())
      if (static_cast<int32_t> (mb.second.movetype ()) == type)
        return mb.second.authoredid ();
    CHECK (false) << "no fighter-move blueprint of type " << type;
    return "";
  }

  /** Arms `fid` with a SINGLE move of the given movetype (so combat is pure
      type-RPS, independent of the per-pair move-shuffle RNG), sweetness 4 (to
      clear the 3v3 tournament's 4-6 gate), the given rating, and clears
      account-binding so it is a loseable, tournament-eligible fighter. */
  void
  ArmFighter (const uint32_t fid, const int moveType, const uint32_t rating)
  {
    auto f = tbl3.GetById (fid, ctx.RoConfig ());
    f->MutableProto ().clear_moves ();
    f->MutableProto ().add_moves (MoveOfType (moveType));
    f->MutableProto ().set_sweetness (4);
    f->MutableProto ().set_rating (rating);
    f->MutableProto ().set_isaccountbound (false);
    f.reset ();
  }

  /** Enters `who`'s three fighters into tournament `tid` via the real move
      processor (no block tick). */
  void
  JoinThree (const std::string& who, const uint32_t tid,
             const std::vector<uint32_t>& team)
  {
    std::ostringstream m;
    m << R"([{"name":")" << who << R"(","move":{"tm":{"e":{"tid":)" << tid
      << R"(,"fc":[)" << team[0] << "," << team[1] << "," << team[2]
      << R"(]}}}}])";
    Process (m.str ());
  }

  /** Bob armed so the UNIQUE worst performer is b3 (the HIGHEST-id / last
      candidate), de-aliasing worst-performance from id/roster order:
        - b1 Blocking (net 0), b2 Blocking rating 3000 (net 0, the SHIELDED
          top-rated fighter, a MIDDLE id), b3 Heavy (net -3, the unique worst).
      candidates after shielding b2 = {b1, b3}; only worst-performance picks b3 --
      an id/roster-order rule would wrongly pick b1 (candidates[0]).
      (Effective combat edges under the RPS-loser-wins scorer: Speedy beats
      Heavy+Blocking; Blocking beats Heavy+Tricky; Heavy beats Tricky+Distance.
      Move types: Heavy=0, Speedy=1, Blocking=4.) */
  std::vector<std::pair<int, uint32_t>>
  WorstAtB3 ()
  {
    return { {4, 1000}, {4, 3000}, {0, 1000} };
  }

  /**
   * Builds a 3v3v3 permadeath scenario and ticks it to the brink of resolution
   * (Running, exactly one block left), so the caller sets the permadeath params
   * then does ONE UpdateState to resolve + fire permadeath.
   *
   * A single move per fighter makes combat a pure, RNG-independent function of
   * move type.  IMPORTANT: the combat scorer (ProcessFighterPair) credits the
   * RPS-LOSING move with the win -- a fighter is scored a WIN when its move type
   * is BEATEN by the opponent's.  All types below are chosen with that inversion
   * in mind (EFFECTIVE edges: Speedy beats Heavy+Blocking; Blocking beats
   * Heavy+Tricky; Heavy beats Tricky+Distance):
   *   - "winner": 3x Speedy -> effectively beats all six Heavy/Blocking opponents
   *     -> total 18, the unique max -> the decisive winner (fixed).
   *   - "carl":   3x Heavy (each loses to winner's Speedy and to bob's Blocking,
   *     draws bob's Heavy -> all net -5, a 3-way tie) -- the second losing team (fixed).
   *   - "bob":    caller-supplied via `bobArms` = three {moveType, rating} pairs,
   *     in roster order b1,b2,b3 (ascending ids) -- the primary team under test.
   */
  ThreeTeam
  SetupThreeTeamAtBrink (const std::vector<std::pair<int, uint32_t>>& bobArms)
  {
    CHECK_EQ (bobArms.size (), 3u) << "bob needs exactly TeamSize (3) fighters";
    const std::string T3 = "99258908-ce4f-50e4-2880-99f0027b8d2b";

    xayaplayers.CreateNew ("winner", ctx.RoConfig (), rnd).reset ();
    xayaplayers.CreateNew ("bob",    ctx.RoConfig (), rnd).reset ();
    xayaplayers.CreateNew ("carl",   ctx.RoConfig (), rnd).reset ();

    const auto mk = [&] (const std::string& owner) -> uint32_t
    {
      return tbl3.CreateNew (owner, 1, ctx.RoConfig (), rnd)->GetId ();
    };

    ThreeTeam s;
    for (int i = 0; i < 3; ++i)
      { const uint32_t id = mk ("winner"); ArmFighter (id, 1, 1000); s.winner.push_back (id); }  // Speedy

    for (const auto& arm : bobArms)
      { const uint32_t id = mk ("bob"); ArmFighter (id, arm.first, arm.second); s.bob.push_back (id); }

    for (int i = 0; i < 3; ++i)
      { const uint32_t id = mk ("carl"); ArmFighter (id, 0, 1000); s.carl.push_back (id); }  // Heavy

    /* One tick auto-creates the sole Listed instance (ReopenMissingTournaments);
       capture its id NOW, before the joins fill it and a fresh Listed instance
       for the same blueprint is spun up. */
    UpdateState ("[]");
    auto tt = tbl5.GetByAuthIdName (T3, ctx.RoConfig ());
    CHECK (tt != nullptr) << "3v3 tournament instance not auto-created";
    s.tid = tt->GetId ();
    tt.reset ();

    JoinThree ("winner", s.tid, s.winner);
    JoinThree ("bob",    s.tid, s.bob);
    JoinThree ("carl",   s.tid, s.carl);

    /* Advance until Running with exactly one block left (duration is 60). */
    for (int guard = 0; guard < 128; ++guard)
      {
        auto trn = tbl5.GetById (s.tid, ctx.RoConfig ());
        const auto st = static_cast<pxd::TournamentState> (
            static_cast<int32_t> (trn->GetInstance ().state ()));
        const int bl = static_cast<int32_t> (trn->GetInstance ().blocksleft ());
        trn.reset ();
        if (st == pxd::TournamentState::Running && bl == 1)
          return s;
        CHECK (st != pxd::TournamentState::Completed)
            << "3v3 tournament resolved before it could be isolated";
        UpdateState ("[]");
      }
    ADD_FAILURE () << "3v3 tournament never reached the brink of resolution";
    return s;
  }

  /** Net wins (wins - losses) recorded for `fid` in the resolved tournament's
      results, or 0 if the fighter has no result row. */
  int32_t
  NetWinsOf (const uint32_t tid, const uint32_t fid)
  {
    auto trn = tbl5.GetById (tid, ctx.RoConfig ());
    int32_t net = 0;
    for (const auto& r : trn->GetInstance ().results ())
      if (static_cast<uint32_t> (r.fighterid ()) == fid)
        { net = static_cast<int32_t> (r.wins ()) - static_cast<int32_t> (r.losses ()); break; }
    trn.reset ();
    return net;
  }

};

namespace
{
    
/* ************************************************************************** */

using ValidateStateTests = PXLogicTests;

/* Change B (Epic 4x): the divisor spares only the Epic (q4) generated recipe;
   every other entry (incl. Rare q3) is scaled up, quartering Epic's share. */
TEST_F (ValidateStateTests, EpicDivisorSparesOnlyEpicRecipe)
{
  pxd::proto::ActivityReward table;

  auto* candy = table.add_rewards ();
  candy->set_type ((uint32_t) pxd::RewardType::Candy);
  candy->set_weight (10);

  auto* epic = table.add_rewards ();
  epic->set_type ((uint32_t) pxd::RewardType::GeneratedRecipe);
  epic->set_generatedrecipequality ((uint32_t) pxd::Quality::Epic);
  epic->set_weight (2);

  auto* rare = table.add_rewards ();
  rare->set_type ((uint32_t) pxd::RewardType::GeneratedRecipe);
  rare->set_generatedrecipequality ((uint32_t) pxd::Quality::Rare);
  rare->set_weight (5);

  // divisor 1 -> unchanged (byte-identical to legacy behaviour).
  EXPECT_EQ (PXLogic::ScaledRewardWeights (table, 1),
             (std::vector<uint32_t> {10, 2, 5}));

  // divisor 4 -> every NON-Epic entry x4 (candy AND Rare q3), Epic q4 spared.
  // Epic share 2/17 (11.8%) -> 2/62 (3.2%) ~ quartered.
  EXPECT_EQ (PXLogic::ScaledRewardWeights (table, 4),
             (std::vector<uint32_t> {40, 2, 20}));
}

/* Change A + C: tournament permadeath + 50/50 capture, forced via the
   capture_pct knob so the outcome is deterministic without controlling combat. */
TEST_F (ValidateStateTests, TournamentPermadeathAlwaysCaptures)
{
  GameParams (db).SetParam ("tournament_capture_pct", 256);        // always capture
  GameParams (db).SetParam ("tournament_loss_kills_enabled", 1);

  const auto t = RunTwoPlayerTournament ();

  int transferred = 0;
  for (const uint32_t fid : t.loserEntered)
    {
      auto f = tbl3.GetById (fid, ctx.RoConfig ());
      if (f != nullptr && f->GetOwner () == t.winner) ++transferred;
    }
  EXPECT_EQ (transferred, 1);                 // exactly one captured to the winner

  BattleLossesTable losses(db);
  auto lr = losses.QueryForOwner (t.loser);
  ASSERT_TRUE (lr.Step ());
  EXPECT_EQ (lr.Get<BattleLossResult::outcome> (), 1);   // captured
  EXPECT_EQ (lr.Get<BattleLossResult::opponent> (), t.winner);
}

TEST_F (ValidateStateTests, TournamentPermadeathAlwaysDestroys)
{
  GameParams (db).SetParam ("tournament_capture_pct", 0);          // always destroy
  GameParams (db).SetParam ("tournament_loss_kills_enabled", 1);

  const auto t = RunTwoPlayerTournament ();

  int alive = 0;
  for (const uint32_t fid : t.loserEntered)
    if (tbl3.GetById (fid, ctx.RoConfig ()) != nullptr) ++alive;
  EXPECT_EQ (alive, 1);                        // exactly one of the two destroyed

  BattleLossesTable losses(db);
  auto lr = losses.QueryForOwner (t.loser);
  ASSERT_TRUE (lr.Step ());
  EXPECT_EQ (lr.Get<BattleLossResult::outcome> (), 0);   // destroyed
}

TEST_F (ValidateStateTests, TournamentPermadeathDisabledKeepsEveryone)
{
  GameParams (db).SetParam ("tournament_loss_kills_enabled", 0);   // disabled

  const auto t = RunTwoPlayerTournament ();

  for (const uint32_t fid : t.loserEntered)
    EXPECT_NE (tbl3.GetById (fid, ctx.RoConfig ()), nullptr);      // all survive
  BattleLossesTable losses(db);
  EXPECT_FALSE (losses.QueryForOwner (t.loser).Step ());           // no loss recorded
}

TEST_F (ValidateStateTests, TournamentPermadeathProtectsStarters)
{
  GameParams (db).SetParam ("tournament_capture_pct", 256);        // would always capture
  GameParams (db).SetParam ("tournament_loss_kills_enabled", 1);

  const auto t = RunTwoPlayerTournament (/*accountBound=*/true);

  for (const uint32_t fid : t.loserEntered)
    {
      auto f = tbl3.GetById (fid, ctx.RoConfig ());
      ASSERT_NE (f, nullptr);                  // not destroyed
      EXPECT_EQ (f->GetOwner (), t.loser);     // not captured
    }
  BattleLossesTable losses(db);
  EXPECT_FALSE (losses.QueryForOwner (t.loser).Step ());   // starter loss not recorded
}

/* Change A/C refinement: the permadeath victim is NEVER the team's strongest.
   Each team is given one clearly higher-rated fighter (the HIGHER-id
   loserEntered[1]) and one weaker (loserEntered[0]); whoever loses must lose the
   WEAKER one -- the stronger is shielded by RATING (not by id order). */
TEST_F (ValidateStateTests, TournamentPermadeathShieldsStrongest)
{
  GameParams (db).SetParam ("tournament_capture_pct", 0);          // always destroy
  GameParams (db).SetParam ("tournament_loss_kills_enabled", 1);

  const auto t = RunTwoPlayerTournament (/*accountBound=*/false,
                                         /*distinctRatings=*/true);

  const uint32_t strongest = t.loserEntered[1];   // higher id, rating 2000 -> shielded
  const uint32_t weakest   = t.loserEntered[0];   // lower id, rating 1000 -> victim

  EXPECT_NE (tbl3.GetById (strongest, ctx.RoConfig ()), nullptr)
      << "the strongest fighter must be shielded, not lost";
  EXPECT_EQ (tbl3.GetById (weakest, ctx.RoConfig ()), nullptr)
      << "the weakest fighter should be the destroyed victim";

  BattleLossesTable losses(db);
  auto lr = losses.QueryForOwner (t.loser);
  ASSERT_TRUE (lr.Step ());
  EXPECT_EQ (lr.Get<BattleLossResult::fighterid> (), (int64_t) weakest)
      << "battle_losses must record the weakest fighter as the victim";
}

/* Change A/C refinement: with 3+ loseable fighters, the victim is the WORST
   performer (fewest net wins) among the NON-shielded candidates -- not the
   shielded top-rated fighter, and NOT chosen by id/roster order.  bob is armed so
   the unique worst (b3, net -3) is the HIGHEST-id / last candidate: an id-order
   rule would wrongly pick b1 (candidates[0]), so this test fails unless the code
   really selects by worst performance. */
TEST_F (ValidateStateTests, TournamentPermadeathTakesWorstPerformer)
{
  GameParams (db).SetParam ("tournament_capture_pct", 0);          // always destroy
  GameParams (db).SetParam ("tournament_loss_kills_enabled", 1);

  const auto s = SetupThreeTeamAtBrink (WorstAtB3 ());
  UpdateState ("[]");                                              // resolve + permadeath

  auto trn = tbl5.GetById (s.tid, ctx.RoConfig ());
  ASSERT_EQ (trn->GetInstance ().winnerid (), "winner");          // decisive winner
  trn.reset ();

  /* Non-vacuity: b3 must really be the UNIQUE worst performer on bob's team, and
     it is the HIGHEST id -- so only worst-performance (not id order) selects it. */
  EXPECT_LT (NetWinsOf (s.tid, s.bob[2]), NetWinsOf (s.tid, s.bob[0]));
  EXPECT_LT (NetWinsOf (s.tid, s.bob[2]), NetWinsOf (s.tid, s.bob[1]));
  EXPECT_GT (s.bob[2], s.bob[0]) << "victim must be the higher-id candidate";

  // b3 (worst) destroyed; b1 (better performer) and b2 (shielded top-rated) survive.
  EXPECT_EQ (tbl3.GetById (s.bob[2], ctx.RoConfig ()), nullptr)
      << "the worst performer must be the victim";
  EXPECT_NE (tbl3.GetById (s.bob[0], ctx.RoConfig ()), nullptr)
      << "a better-performing lower-id fighter must survive";
  EXPECT_NE (tbl3.GetById (s.bob[1], ctx.RoConfig ()), nullptr)
      << "the shielded top-rated fighter must survive";

  BattleLossesTable losses(db);
  auto lr = losses.QueryForOwner ("bob");
  ASSERT_TRUE (lr.Step ());
  EXPECT_EQ (lr.Get<BattleLossResult::fighterid> (), (int64_t) s.bob[2]);
  EXPECT_EQ (lr.Get<BattleLossResult::outcome> (), 0);            // destroyed

  // Every non-winning team loses exactly one: carl also lost exactly one fighter.
  int carlAlive = 0;
  for (const uint32_t fid : s.carl)
    if (tbl3.GetById (fid, ctx.RoConfig ()) != nullptr) ++carlAlive;
  EXPECT_EQ (carlAlive, 2) << "each non-winning team loses exactly one fighter";
}

/* Change A/C refinement: the shield OVERRIDES worst-performance.  bob's top-rated
   fighter (b1, rating 3000) is armed to ALSO be the unique WORST performer
   (net -3); the shield must still protect it, so the victim is one of the net-0
   teammates.  Remove the shield and b1 (the unique worst) would die -> this test
   deterministically fails, proving the shield exists. */
TEST_F (ValidateStateTests, TournamentPermadeathShieldOverridesWorst)
{
  GameParams (db).SetParam ("tournament_capture_pct", 0);          // always destroy
  GameParams (db).SetParam ("tournament_loss_kills_enabled", 1);

  // b1 Heavy rating 3000 (net -3, top-rated AND worst); b2,b3 Blocking (net 0).
  const auto s = SetupThreeTeamAtBrink ({ {0, 3000}, {4, 1000}, {4, 1000} });
  UpdateState ("[]");                                              // resolve + permadeath

  auto trn = tbl5.GetById (s.tid, ctx.RoConfig ());
  ASSERT_EQ (trn->GetInstance ().winnerid (), "winner");
  trn.reset ();

  /* Non-vacuity: b1 must really be the UNIQUE worst -- so its survival can only be
     the shield overriding worst-performance, nothing else. */
  EXPECT_LT (NetWinsOf (s.tid, s.bob[0]), NetWinsOf (s.tid, s.bob[1]));
  EXPECT_LT (NetWinsOf (s.tid, s.bob[0]), NetWinsOf (s.tid, s.bob[2]));

  // The shielded top-rated fighter survives despite being the worst performer.
  auto shielded = tbl3.GetById (s.bob[0], ctx.RoConfig ());
  ASSERT_NE (shielded, nullptr) << "the shield must protect the top-rated fighter even when it is worst";
  EXPECT_EQ (shielded->GetOwner (), "bob");
  shielded.reset ();

  // The team still loses exactly one -- one of the two net-0 teammates.
  int bobAlive = 0;
  for (const uint32_t fid : s.bob)
    if (tbl3.GetById (fid, ctx.RoConfig ()) != nullptr) ++bobAlive;
  EXPECT_EQ (bobAlive, 2) << "bob still loses exactly one fighter";

  BattleLossesTable losses(db);
  auto lr = losses.QueryForOwner ("bob");
  ASSERT_TRUE (lr.Step ());
  const int64_t victim = lr.Get<BattleLossResult::fighterid> ();
  EXPECT_TRUE (victim == (int64_t) s.bob[1] || victim == (int64_t) s.bob[2])
      << "the victim must be a net-0 teammate, never the shielded worst performer";
}

/* Change C: tournament_max_captures caps how many fighters the single winner may
   capture across ALL losing teams in one tournament; victims past the cap are
   destroyed instead.  With cap 1 and always-capture, the winner takes the
   first-processed losing team's victim (ascending owner order -> "bob") and the
   next losing team's victim ("carl") is destroyed. */
TEST_F (ValidateStateTests, TournamentCaptureCapLimitsWinnerGains)
{
  GameParams (db).SetParam ("tournament_capture_pct", 256);       // always capture (when allowed)
  GameParams (db).SetParam ("tournament_loss_kills_enabled", 1);
  GameParams (db).SetParam ("tournament_max_captures", 1);        // winner may take at most one

  const auto s = SetupThreeTeamAtBrink (WorstAtB3 ());
  const unsigned winnerBefore = tbl3.CountForOwner ("winner");
  UpdateState ("[]");                                             // resolve + permadeath

  auto trn = tbl5.GetById (s.tid, ctx.RoConfig ());
  ASSERT_EQ (trn->GetInstance ().winnerid (), "winner");
  trn.reset ();

  // The winner gained exactly one fighter (the cap), never two.
  EXPECT_EQ (tbl3.CountForOwner ("winner"), winnerBefore + 1);

  // bob's victim (b3, the worst performer) was CAPTURED -> now owned by winner.
  auto captured = tbl3.GetById (s.bob[2], ctx.RoConfig ());
  ASSERT_NE (captured, nullptr) << "captured fighter must still exist";
  EXPECT_EQ (captured->GetOwner (), "winner");
  captured.reset ();

  // carl (processed after the cap was hit) lost exactly one fighter -- DESTROYED.
  int carlAlive = 0;
  for (const uint32_t fid : s.carl)
    if (tbl3.GetById (fid, ctx.RoConfig ()) != nullptr) ++carlAlive;
  EXPECT_EQ (carlAlive, 2) << "carl's over-cap victim must be destroyed, not captured";

  BattleLossesTable losses(db);
  auto bobLoss = losses.QueryForOwner ("bob");
  ASSERT_TRUE (bobLoss.Step ());
  EXPECT_EQ (bobLoss.Get<BattleLossResult::outcome> (), 1);        // captured
  EXPECT_EQ (bobLoss.Get<BattleLossResult::opponent> (), "winner");

  auto carlLoss = losses.QueryForOwner ("carl");
  ASSERT_TRUE (carlLoss.Step ());
  EXPECT_EQ (carlLoss.Get<BattleLossResult::outcome> (), 0);       // destroyed (over cap)
}

/* Change C: tournament_max_captures = 0 disables capture entirely -- even at
   capture_pct 256 every victim across all losing teams is destroyed and the winner
   gains nothing. */
TEST_F (ValidateStateTests, TournamentCaptureCapZeroDestroysAll)
{
  GameParams (db).SetParam ("tournament_capture_pct", 256);       // would always capture
  GameParams (db).SetParam ("tournament_loss_kills_enabled", 1);
  GameParams (db).SetParam ("tournament_max_captures", 0);        // ...but the cap forbids it

  const auto s = SetupThreeTeamAtBrink (WorstAtB3 ());
  const unsigned winnerBefore = tbl3.CountForOwner ("winner");
  UpdateState ("[]");

  auto trn = tbl5.GetById (s.tid, ctx.RoConfig ());
  ASSERT_EQ (trn->GetInstance ().winnerid (), "winner");
  trn.reset ();

  EXPECT_EQ (tbl3.CountForOwner ("winner"), winnerBefore);        // no captures at all
  EXPECT_EQ (tbl3.GetById (s.bob[2], ctx.RoConfig ()), nullptr);  // bob's victim destroyed

  BattleLossesTable losses(db);
  auto bobLoss = losses.QueryForOwner ("bob");
  ASSERT_TRUE (bobLoss.Step ());
  EXPECT_EQ (bobLoss.Get<BattleLossResult::outcome> (), 0);        // destroyed, not captured
}

/* Change C: when the winner's roster is FULL, a rolled capture downgrades to a
   destroy (distinct from the cap path: here it is the 48-slot check that blocks
   the transfer, not tournament_max_captures). */
TEST_F (ValidateStateTests, TournamentCaptureRosterFullDestroys)
{
  GameParams (db).SetParam ("tournament_capture_pct", 256);       // would always capture
  GameParams (db).SetParam ("tournament_loss_kills_enabled", 1);
  GameParams (db).SetParam ("tournament_max_captures", 3);        // cap is NOT the blocker here

  const auto s = SetupThreeTeamAtBrink (WorstAtB3 ());

  /* Make the winner's roster full: pin max_fighter_inventory_amount to its CURRENT
     owned count (which includes the account-bound starters CreateNew grants, not
     just s.winner) so there is no free slot. Save/restore the process-global. */
  auto& cfg = const_cast<proto::ConfigData&> (*ctx.RoConfig ());
  const uint32_t savedMax = cfg.params ().max_fighter_inventory_amount ();
  const unsigned winnerBefore = tbl3.CountForOwner ("winner");
  cfg.mutable_params ()->set_max_fighter_inventory_amount (winnerBefore);

  UpdateState ("[]");                                             // resolve under a full roster
  cfg.mutable_params ()->set_max_fighter_inventory_amount (savedMax);   // restore before asserts

  auto trn = tbl5.GetById (s.tid, ctx.RoConfig ());
  ASSERT_EQ (trn->GetInstance ().winnerid (), "winner");
  trn.reset ();

  // No room -> the rolled capture became a destroy; the winner gained nothing.
  EXPECT_EQ (tbl3.GetById (s.bob[2], ctx.RoConfig ()), nullptr)
      << "roster full -> victim destroyed, not captured";
  EXPECT_EQ (tbl3.CountForOwner ("winner"), winnerBefore) << "no capture -> roster unchanged";

  BattleLossesTable losses(db);
  auto lr = losses.QueryForOwner ("bob");
  ASSERT_TRUE (lr.Step ());
  EXPECT_EQ (lr.Get<BattleLossResult::outcome> (), 0);            // destroyed (roster full)
}

TEST_F (ValidateStateTests, RecepieInstanceFullCycleTest)
{
  proto::ConfigData& cfg = const_cast <proto::ConfigData&>(*ctx.RoConfig());
  
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->AddBalance (100);
   
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  a->GetInventory().SetFungibleCount("Common_Gumdrop", 1);
  a->GetInventory().SetFungibleCount("Common_Icing", 1);
  a.reset();
  
  tbl2.GetById(1)->SetOwner("domob");
  
  auto r0 = tbl2.CreateNew("domob", "5864a19b-c8c0-2d34-eaef-9455af0baf2c", ctx.RoConfig());
  r0.reset();   
   
  Process (R"([
    {"name": "domob", "move": {"ca": {"r": {"rid": 1, "fid": 0}}}}
  ])");  

  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);

  EXPECT_EQ (a->GetOngoingsSize (), 1);
  a.reset ();

  UpdateState ("[]");
  
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->GetOngoingsSize (), 0);
  EXPECT_EQ (a->CollectInventoryFighters(ctx.RoConfig()).size(), 3);

  /* Auto-collect: a finished cook lands as a usable Available fighter -- no
     second "collect" tx -- and every fighter domob owns is Available. */
  for (const auto& f : a->CollectInventoryFighters (ctx.RoConfig ()))
    EXPECT_EQ (f->GetStatus (), pxd::FighterStatus::Available);
  /* The cook credits the append-only auto-collect serial (mirrors reward
     auto-credit) so the client can reveal the new fighter with no claim tx. */
  EXPECT_EQ (a->GetProto ().rewards_serial (), 1u);

  EXPECT_EQ (a->GetBalance (), 85 + cfg.params().starting_crystals());

  auto r = tbl2.GetById(1);
  EXPECT_EQ (r->GetProto().name(), "First Recipe");
  EXPECT_EQ (r->GetOwner(), "");

  EXPECT_EQ (a->GetInventory().GetFungibleCount("Common_Gumdrop"), 0);
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Common_Icing"), 0);

}

TEST_F (ValidateStateTests, DebugRecipeNamesTest)
{
	proto::ConfigData& cfg = const_cast <proto::ConfigData&>(*ctx.RoConfig());
    std::vector<pxd::proto::FighterName> potentialNames;
    const auto& fighterNames = cfg.fighternames();
    
    std::vector<std::pair<std::string, pxd::proto::FighterName>> sortedNamesTypesmap;
    for (auto itr = fighterNames.begin(); itr != fighterNames.end(); ++itr)
        sortedNamesTypesmap.push_back(*itr);

    sort(sortedNamesTypesmap.begin(), sortedNamesTypesmap.end(), [=](std::pair<std::string, pxd::proto::FighterName>& a, std::pair<std::string, pxd::proto::FighterName>& b)
    {
        return a.first < b.first;
    } 
    );        
    
    for (const auto& fighter : sortedNamesTypesmap)
    {
        if((Quality)(int)fighter.second.quality() == pxd::Quality::Rare)
        {
            potentialNames.push_back(fighter.second);
        }
    }
    
    std::vector<pxd::proto::FighterName> position0names;
    std::vector<pxd::proto::FighterName> position1names;
    
    for(auto& name: potentialNames)
    {
        if(name.position() == 0)
        {
            position0names.push_back(name);
        }
        
        if(name.position() == 1)
        {
            position1names.push_back(name);
        }        
    }

    LOG (WARNING) << "Total potential names on position 0 is: " << position0names.size();
	LOG (WARNING) << "Total potential names on position 1 is: " << position1names.size();
	
	std::vector<std::string> namesColleced;
	
	xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->AddBalance (100);	
	for (unsigned i = 0; i < 1000; ++i)
    {
		auto rcpID = pxd::RecipeInstance::Generate(pxd::Quality::Rare, ctx.RoConfig(), rnd, db, "");
		auto r = tbl2.GetById(rcpID);
		
		if (std::find(namesColleced.begin(), namesColleced.end(), r->GetProto().name()) == namesColleced.end()) 
		{
			namesColleced.push_back(r->GetProto().name());
		}
		
		r.reset(); 
	}
	
	LOG (WARNING) << "Total variations collected: " << namesColleced.size();
}

TEST_F (ValidateStateTests, RecepieInstanceGeneratedFullCycleTest)
{
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->AddBalance (100);
  
  auto rcpID = pxd::RecipeInstance::Generate(pxd::Quality::Common, ctx.RoConfig(), rnd, db, "");
  auto r = tbl2.GetById(rcpID);
  
  
  EXPECT_EQ (r->GetProto().duration(), ctx.RoConfig()->params().common_recipe_cook_cost());

  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  r->SetOwner(a->GetName());
  
  for(auto& rC: r->GetProto().requiredcandy())
  {
    a->GetInventory().SetFungibleCount(BaseMoveProcessor::GetCandyKeyNameFromID(rC.candytype(), ctx), rC.amount());
  }
  
  a.reset();
  r.reset();   
   
  std::ostringstream s;
  s << rcpID;
  std::string converted(s.str());    
   
  Process (R"([
    {"name": "domob", "move": {"ca": {"r": {"rid": )"+converted+R"(, "fid": 0}}}}
  ])");  

  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);

  EXPECT_EQ (a->GetOngoingsSize (), 1);
  a.reset ();

  UpdateState ("[]");
  
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);  
  
  EXPECT_EQ (a->GetOngoingsSize (), 1);
  a.reset ();
  
  for (unsigned i = 0; i < ctx.RoConfig()->params().common_recipe_cook_cost(); ++i)
  {
    UpdateState ("[]");
  }    
  
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->GetOngoingsSize (), 0);
  EXPECT_EQ (a->CollectInventoryFighters(ctx.RoConfig()).size(), 3);
  a.reset();

  r = tbl2.GetById(rcpID); 
  EXPECT_EQ (r->GetOwner(), "");
  r.reset();
}   

/*
TEST_F (ValidateStateTests, RecepieInstanceGeneratedDifferentNamesTest)
{
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->AddBalance (100);
  
  auto rcpID = pxd::RecipeInstance::Generate(pxd::Quality::Common, ctx.RoConfig(), rnd, db, "");
  auto r = tbl2.GetById(rcpID);
  
  auto rcpID2 = pxd::RecipeInstance::Generate(pxd::Quality::Common, ctx.RoConfig(), rnd, db, "");
  auto r2 = tbl2.GetById(rcpID2);
  
  EXPECT_NE(r->GetProto().name(), r2->GetProto().name());
  r.reset();
  r2.reset();
}*/ 



TEST_F (ValidateStateTests, RecepieWithApplicableGoodieTest)
{
  proto::ConfigData& cfg = const_cast <proto::ConfigData&>(*ctx.RoConfig());
  
  auto a = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  a->AddBalance (100);
  
  a->GetInventory().SetFungibleCount("Rare_Jawbreaker", 50);
  a->GetInventory().SetFungibleCount("Rare_Toffee Chunk", 30);
  a->GetInventory().SetFungibleCount("Uncommon_Jelly Bean", 50);
  a->GetInventory().SetFungibleCount("Uncommon_Chocolate Nut", 30);
  a->GetInventory().SetFungibleCount("Common_Candy Button", 35);
  
  a->GetInventory().SetFungibleCount("Goodie_PressureCooker_1", 1);
  
  a.reset();
  auto r0 = tbl2.CreateNew("domob", "fda69e34-c1cb-2664-4ba1-d943713218c5", ctx.RoConfig());
  r0.reset();   
   
  tbl2.GetById(2)->SetOwner("domob"); 
   
  Process (R"([
    {"name": "domob", "move": {"ca": {"r": {"rid": 2, "fid": 0}}}}
  ])");  

  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 1);
  a.reset ();

  for (unsigned i = 0; i < 41; ++i)
  {
    UpdateState ("[]");
  }
  
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->GetOngoingsSize (), 0);
  EXPECT_EQ (a->CollectInventoryFighters(ctx.RoConfig()).size(), 3);
  
  EXPECT_EQ (a->GetBalance (), 85 + cfg.params().starting_crystals());
  
  auto r = tbl2.GetById(2); 
  EXPECT_EQ (r->GetProto().name(), "Second Recipe");
  EXPECT_EQ (r->GetOwner(), "");
  
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Goodie_PressureCooker_1"), 0);
}   

TEST_F (ValidateStateTests, RecepieInstanceRevertIfFullRoster)
{
  proto::ConfigData& cfg = const_cast <proto::ConfigData&>(*ctx.RoConfig());
  
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->AddBalance (100);
   
  cfg.mutable_params()->set_max_fighter_inventory_amount(2); 
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  a->GetInventory().SetFungibleCount("Common_Gumdrop", 1);
  a->GetInventory().SetFungibleCount("Common_Icing", 1);
  a.reset();
  
  tbl2.GetById(1)->SetOwner("domob");

  Process (R"([
    {"name": "domob", "move": {"ca": {"r": {"rid": 1, "fid": 0}}}}
  ])");  

  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 1);
  a.reset ();
  
  UpdateState ("[]");
  
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->CollectInventoryFighters(ctx.RoConfig()).size(), 2);
  
  EXPECT_EQ (a->GetBalance (), 100 + cfg.params().starting_crystals());

  /* A full-roster refund still notifies the client via the same serial the
     success path bumps, so the FE always surfaces the outcome ("roster full --
     cook refunded"), never a silent items-came-back diff. */
  EXPECT_EQ (a->GetProto ().rewards_serial (), 1u);

  auto r0 = tbl2.GetById(1);
  EXPECT_EQ (r0->GetProto().name(), "First Recipe");
  EXPECT_EQ (r0->GetOwner(), "domob");

  EXPECT_EQ (a->GetInventory().GetFungibleCount("Common_Gumdrop"), 1);
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Common_Icing"), 1);
}

TEST_F (ValidateStateTests, CollectCookMoveIsInert)
{
  /* ca.cl is deleted (cooks auto-collect at resolve).  A stale client sending it
     must be a harmless no-op: the move parses without effect and no fighter
     changes state. */
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->AddBalance (100);

  int fid;
  {
    auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
    auto owned = a->CollectInventoryFighters (ctx.RoConfig());
    ASSERT_FALSE (owned.empty ());
    fid = owned.front ()->GetId ();
  }

  Process (R"([
    {"name": "domob", "move": {"ca": {"cl": {"fid": )" + std::to_string (fid) + R"(}}}}
  ])");

  /* The fighter is untouched -- still Available, still owned by domob. */
  auto ft = tbl3.GetById (fid, ctx.RoConfig());
  ASSERT_TRUE (ft != nullptr);
  EXPECT_EQ (ft->GetStatus (), pxd::FighterStatus::Available);
  EXPECT_EQ (ft->GetOwner (), "domob");
}

TEST_F (ValidateStateTests, UnitTestExpeditionFailsOnMainNet)
{  
  ctx.SetChain (xaya::Chain::MAIN);
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  ft.reset();
  xp.reset();
  
  EXPECT_EQ (tbl2.CountForOwner(""), 2);
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "00000000-0000-0000-zzzz-zzzzzzzzzzzz", "fid": 4}}}}
  ])");  
  
  
  EXPECT_EQ (tbl2.CountForOwner(""), 2);
  
  for (unsigned i = 0; i < 1; ++i)
  {
    UpdateState ("[]");
  }  
  
  EXPECT_NE (tbl2.CountForOwner(""), 17);    
}


TEST_F (ValidateStateTests, GeneratedRecipeMakeSureItWorks)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  ft.reset();
  xp.reset();
  
  EXPECT_EQ (tbl2.CountForOwner(""), 2);
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "00000000-0000-0000-zzzz-zzzzzzzzzzzz", "fid": 4}}}}
  ])");  
  
  for (unsigned i = 0; i < 1; ++i)
  {
    UpdateState ("[]");
  }  
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  a.reset ();
   
  ft = tbl3.GetById(2, ctx.RoConfig());
  ft.reset();
  
  /* The generated recipe is auto-credited to the player at resolve (no claim),
     so the unowned-template count is unchanged and it appears among domob's. */
  EXPECT_EQ (tbl2.CountForOwner(""), 2);

  bool foundGenerated = false;
  auto res = tbl2.QueryForOwner("domob");
  while (res.Step ())
  {
    auto r = tbl2.GetFromResult (res);
    if (r->GetProto().authoredid() == "generated")
    {
      foundGenerated = true;
      EXPECT_EQ (r->GetProto().moves_size(), 3);
      EXPECT_EQ (r->GetProto().requiredcandy_size(), 3);
    }
  }
  EXPECT_TRUE (foundGenerated);
}

TEST_F (ValidateStateTests, RecepieInstanceFailWithMissingIngridients)
{
   proto::ConfigData& cfg = const_cast <proto::ConfigData&>(*ctx.RoConfig());
   
   xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->AddBalance (100);
   auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());

   a->GetInventory().SetFungibleCount("Common_Icing", 0);
   a.reset();
   
   tbl2.GetById(1)->SetOwner("domob");
   
   auto r0 = tbl2.CreateNew("domob", "5864a19b-c8c0-2d34-eaef-9455af0baf2c", ctx.RoConfig());
   r0.reset();      
   
   Process (R"([
    {"name": "domob", "move": {"ca": {"r": {"rid": 1, "fid": 0}}}}
   ])");  

  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 0);
  a.reset ();

  UpdateState ("[]");
  
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());

  
  EXPECT_EQ (a->GetBalance (), 100 + cfg.params().starting_crystals());
  
  auto r = tbl2.GetById(1); 
  EXPECT_EQ (r->GetProto().name(), "First Recipe");
  EXPECT_EQ (r->GetOwner(), "domob");
  
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Common_Gumdrop"), 0);
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Common_Icing"), 0);
  
}  

TEST_F (ValidateStateTests, SweetenerRandomRewardConsistency)
{
  /* Predates the Epic-4x change; pin the divisor to 1 so this keeps validating
     the legacy sweetener reward-roll RNG fingerprint (divisor 1 = byte-identical
     weighted pick). */
  GameParams (db).SetParam ("rarest_recipe_drop_divisor", 1);

  /* This test validates deterministic sweetener reward RNG.  Rewards now
     auto-credit at resolve; raise the recipe cap so recipe rewards always credit
     (never overflow into the held queue), keeping the credited-reward count a
     clean determinism fingerprint that the append-only rewards_serial tracks. */
  const_cast<proto::ConfigData&>(*ctx.RoConfig()).mutable_params()->set_max_recipe_inventory_amount(10000000);

  auto pl = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  pl->AddBalance(100);
  pl->GetInventory().SetFungibleCount("Sweetener_R2", 1);
  
  pl->GetInventory().SetFungibleCount("Common_Icing", 10);
  pl->GetInventory().SetFungibleCount("Common_Fruit Slice", 10);

  
  
  auto ft = tbl3.GetById(4, ctx.RoConfig());
  ASSERT_TRUE (ft != nullptr); 
  ft->MutableProto().set_rating(1210); 
  ft->MutableProto().set_sweetness((int)pxd::Sweetness::Bittersweet);  
  ft.reset();
  
  tbl2.GetById(1)->SetOwner("domob");	
  
  for (unsigned i = 0; i < 520000; ++i)
  {
	db.GetNextId ();
  }
  
  pl.reset();
  
  for (unsigned i = 0; i < 1000; ++i)
  {
	  pl = xayaplayers.GetByName ("domob", ctx.RoConfig());
	  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
	  int fID = ft->GetId();
	  EXPECT_EQ (ft->GetStatus(), FighterStatus::Available);
	  ft.reset();	  
	  PXLogic::ResolveSweetener(pl, "a5d19aba-ba28-01d4-e8a7-77ba3481288e", fID, 0,  db, ctx, rnd); 
	  pl.reset();
      UpdateState ("[]");	  	 
  }
  
  
  /* Deterministic credited-reward total across 1000 sweetener rolls.  The RNG
     stream is unchanged by the auto-credit rewrite (crediting adds no draw and
     the recipe Generate draw still fires), so every reward that used to become an
     unclaimed row now bumps the serial 1:1 -> the fingerprint carries over as
     2046.  RE-PIN from the build if this diverges. */
  auto plFinal = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (plFinal != nullptr);
  EXPECT_EQ (plFinal->GetProto().rewards_serial(), 2046u);
  EXPECT_EQ (tbl4.CountForOwner("domob"), 0);   // nothing overflowed
  plFinal.reset();

  /* Restore the production default: RoConfig is a process-global mutable
     singleton shared across tests. */
  const_cast<proto::ConfigData&>(*ctx.RoConfig()).mutable_params()->set_max_recipe_inventory_amount(48);
}

TEST_F (ValidateStateTests, RecipeOverflowQueueIsBounded)
{
  /* The held recipe-overflow queue is the only remaining user of the rewards
     table; max_unclaimed_reward_amount bounds it so it can never grow without
     bound (the DB-bloat guard).  With recipe slots forced full, every recipe
     reward overflows; past the queue cap the recipe is dropped, not stored. */
  proto::ConfigData& cfg = const_cast<proto::ConfigData&>(*ctx.RoConfig());
  cfg.mutable_params()->set_max_recipe_inventory_amount(0);   // every recipe reward overflows
  cfg.mutable_params()->set_max_unclaimed_reward_amount(3);   // overflow-queue cap

  auto pl = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);

  pxd::proto::AuthoredActivityReward rec;
  rec.set_type ((uint32_t) pxd::RewardType::GeneratedRecipe);
  rec.set_generatedrecipequality (1);

  for (unsigned i = 0; i < 20; ++i)
  {
    PXLogic::CreditActivityReward (pl, 0, "someexp", 0, rec, ctx, db, rnd, 0, "sometable", "");
    EXPECT_LE (tbl4.CountForOwner("domob"), 3u);   // clamps at the cap, never grows past it
  }

  EXPECT_EQ (tbl4.CountForOwner("domob"), 3u);
  /* Held recipes are not credited, so the serial stayed at 0 the whole time. */
  EXPECT_EQ (pl->GetProto().rewards_serial(), 0u);
  pl.reset();

  /* RoConfig is a process-global mutable singleton shared across tests; restore
     the production defaults so these small caps do not leak into later tests. */
  cfg.mutable_params()->set_max_recipe_inventory_amount(48);
  cfg.mutable_params()->set_max_unclaimed_reward_amount(100);
}

TEST_F (ValidateStateTests, SweetenerCookAndProperRewardsClaimed)
{
  auto pl = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  pl->AddBalance(100);
  pl->GetInventory().SetFungibleCount("Sweetener_R2", 1);
  
  pl->GetInventory().SetFungibleCount("Common_Icing", 10);
  pl->GetInventory().SetFungibleCount("Common_Fruit Slice", 10);

  pl.reset();
  
  auto ft = tbl3.GetById(4, ctx.RoConfig());
  ASSERT_TRUE (ft != nullptr); 
  ft->MutableProto().set_rating(1210); 
  ft->MutableProto().set_sweetness((int)pxd::Sweetness::Bittersweet);  
  ft.reset();
  
  tbl2.GetById(1)->SetOwner("domob");
  
  Process (R"([
    {"name": "domob", "move": {"ca": {"s": {"sid": "d596403b-b76f-52c4-6956-4bfd55231de0", "fid": 4, "rid": 1}}}}
  ])");  
  
  pl = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (pl != nullptr);
  EXPECT_EQ (pl->GetOngoingsSize (), 1);
  pl.reset();

  EXPECT_EQ (tbl4.CountForOwner("domob"), 0);

  ft = tbl3.GetById(4, ctx.RoConfig());
  ASSERT_TRUE (ft != nullptr);
  EXPECT_EQ(ft->GetProto().moves_size(), 2);   // 2 base moves before the cook resolves
  ft.reset();

  for (unsigned i = 0; i < 22; ++i)
  {
    UpdateState ("[]");
  }

  pl = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (pl != nullptr);
  EXPECT_EQ (pl->GetOngoingsSize (), 0);
  /* Auto-credit at resolve: the sweetener's move reward is applied to the fighter
     the moment the cook resolves -- no claim step, no reward row. */
  EXPECT_EQ (pl->GetProto().rewards_serial(), 1u);
  pl.reset();

  EXPECT_EQ (tbl4.CountForOwner("domob"), 0);

  ft = tbl3.GetById(4, ctx.RoConfig());
  ASSERT_TRUE (ft != nullptr);
  EXPECT_EQ(ft->GetProto().moves_size(), 3);    // sweetener move granted at resolve
  ft.reset();
}

TEST_F (ValidateStateTests, ExpeditionInstanceSolveTwiceTest)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Available);
  ft.reset();
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 3);
  xp.reset();
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "93ad71bb-cd8f-dc24-7885-2c3fd0013245", "fid": 4}}}}
  ])");  
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 1);
  a.reset ();
   
  ft = tbl3.GetById(4, ctx.RoConfig());
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Expedition);
  ft.reset();
  
  for (unsigned i = 0; i < 17; ++i)
  {
    UpdateState ("[]");
  }
  
  ft = tbl3.GetById(4, ctx.RoConfig());
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Available);
  ft.reset();  
  
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->GetOngoingsSize (), 0);
  a.reset ();
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "c064e7f7-acbf-4f74-fab8-cccd7b2d4004", "fid": 4}}}}
  ])");  

  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->GetOngoingsSize (), 1);  
 
  ft = tbl3.GetById(4, ctx.RoConfig());
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Expedition);
  ft.reset(); 
}

TEST_F (ValidateStateTests, ClaimRewardsAfterExpedition)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Available);
  ft.reset();
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 3);
  xp.reset();
  
  EXPECT_EQ (tbl4.CountForOwner("domob"), 0);
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "c064e7f7-acbf-4f74-fab8-cccd7b2d4004", "fid": 4}}}}
  ])");  
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 1);
  a.reset ();
   
  ft = tbl3.GetById(4, ctx.RoConfig());
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Expedition);
  ft.reset();
  
  for (unsigned i = 0; i < 1; ++i)
  {
    UpdateState ("[]");
  }
  
  ft = tbl3.GetById(4, ctx.RoConfig());
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Available);
  ft.reset();  
  
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->GetOngoingsSize (), 0);
  /* Auto-credit at resolve: c064e7f7 rolls a List of 1 CraftedRecipe + 2 Candy.
     All three credit straight into the player at resolve (recipe slot free), so
     the rewards table holds nothing and the serial advanced by 3. */
  EXPECT_EQ (a->GetProto().rewards_serial(), 3u);
  a.reset ();

  EXPECT_EQ (tbl4.CountForOwner("domob"), 0);
}

TEST_F (ValidateStateTests, ClaimRewardsTestAllRewardTypesBeingAwardedProperly)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  ft.reset();
  xp.reset();
  
  EXPECT_EQ (tbl2.CountForOwner(""), 2);
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "00000000-0000-0000-zzzz-zzzzzzzzzzzz", "fid": 4}}}}
  ])");

  UpdateState ("[]");   // resolves in 1 block -> 1 GeneratedRecipe, auto-credited to domob (slot free)

  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetProto().rewards_serial(), 1u);   // one recipe reward credited
  a.reset ();

  /* The generated recipe is owned by the player directly (no claim step): the
     unowned-template count is unchanged and nothing sits in the rewards table. */
  EXPECT_EQ (tbl2.CountForOwner(""), 2);
  EXPECT_EQ (tbl4.CountForOwner("domob"), 0);

  bool foundGenerated = false;
  {
    auto res = tbl2.QueryForOwner("domob");
    while (res.Step ())
    {
      auto r = tbl2.GetFromResult (res);
      if (r->GetProto().authoredid() == "generated") foundGenerated = true;
    }
  }
  EXPECT_TRUE (foundGenerated);

  ft = tbl3.GetById(4, ctx.RoConfig());
  EXPECT_EQ (ft->GetProto().animationid(), "05633498-ace9-de14-c939-9435a6343d0f");
}

TEST_F (ValidateStateTests, CreditRewardWithMissingFighterDoesNotHalt)
{
  /* HALT-01 regression. A Move/Armor/Animation reward is credited at resolve
     against a fighter id.  A tournament reward carries fighterID 0, and a fighter
     can already be gone by the time its reward credits.  CreditActivityReward
     must DROP such a reward instead of dereferencing a null fighter handle --
     otherwise every node SIGSEGVs at the same height = a permanent chain HALT. */
  auto pl = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);

  pxd::proto::AuthoredActivityReward mv;
  mv.set_type ((uint32_t) pxd::RewardType::Move);
  mv.set_moveid ("some-move");
  // fighter id 999999 does not exist -> must be dropped, not dereferenced.
  EXPECT_NO_FATAL_FAILURE (
    PXLogic::CreditActivityReward (pl, 999999, "", 0, mv, ctx, db, rnd, 0, "", ""));

  pxd::proto::AuthoredActivityReward arm;
  arm.set_type ((uint32_t) pxd::RewardType::Armor);
  arm.set_armortype (1);
  // tournament-reward path: fighterID 0 (no fighter) -> must be dropped.
  EXPECT_NO_FATAL_FAILURE (
    PXLogic::CreditActivityReward (pl, 0, "", 5, arm, ctx, db, rnd, 0, "", ""));

  // Nothing credited (no valid fighter): the serial did not advance, no crash.
  EXPECT_EQ (pl->GetProto().rewards_serial(), 0u);
  pl.reset ();
}

TEST_F (ValidateStateTests, DeconstructionTest)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  int fID = ft->GetId();
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Available);
  ft.reset();
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 3);
  xp.reset();

  /* H4: the fighter's source recipe must be deleted once it is deconstructed and
     the reward claimed, instead of being orphaned (owner='') forever. */
  ft = tbl3.GetById (fID, ctx.RoConfig());
  const uint32_t srcRecipe = ft->GetProto().recipeid();
  /* First Recipe fighters are account-bound (recover nothing on deconstruct);
     clear the flag here so this test exercises the candy-recovery credit path. */
  ft->MutableProto().set_isaccountbound(false);
  ft.reset();
  EXPECT_GT (srcRecipe, 0u);
  EXPECT_TRUE (tbl2.GetById(srcRecipe) != nullptr);

  EXPECT_EQ (tbl4.CountForOwner("domob"), 0);

  /* First Recipe = Gumdrop x1 + Icing x1 (total 2); 50% return -> exactly 1
     candy recovered, of one of the two types.  Capture the baseline. */
  auto a0 = xayaplayers.GetByName ("domob", ctx.RoConfig());
  const int64_t candyBefore = a0->GetInventory().GetFungibleCount("Common_Gumdrop")
                            + a0->GetInventory().GetFungibleCount("Common_Icing");
  a0.reset();

  std::ostringstream s;
  s << fID;
  std::string converted(s.str());

  Process (R"([
    {"name": "domob", "move": {"f": {"d": {"fid": )"+converted+R"(}}}}
  ])");

  for (unsigned i = 0; i < 15; ++i)
  {
    UpdateState ("[]");
  }

  /* Auto-credit at resolve: the recovered candy lands in inventory and the
     fighter + its source recipe are deleted immediately -- no claim step, and a
     deconstruction never creates a reward row.  H4: the source recipe is gone,
     not leaked owner=''. */
  EXPECT_EQ (tbl4.CountForOwner("domob"), 0);
  EXPECT_TRUE (tbl3.GetById(fID, ctx.RoConfig()) == nullptr);
  EXPECT_TRUE (tbl2.GetById(srcRecipe) == nullptr);

  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetProto().rewards_serial(), 1u);   // recovered candy credited once
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Common_Gumdrop")
             + a->GetInventory().GetFungibleCount("Common_Icing"), candyBefore + 1);
  a.reset();
}

TEST_F (ValidateStateTests, DeconstructionAccountBoundRecoversNothing)
{
  /* Account-bound fighters recover NO candy on deconstruct (the payout is
     suppressed), but the fighter + its recipe are still deleted at resolve. */
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);   // First Recipe = account-bound
  int fID = ft->GetId();
  ASSERT_TRUE (ft->GetProto().isaccountbound());
  ft.reset();
  xp.reset();

  ft = tbl3.GetById (fID, ctx.RoConfig());
  const uint32_t srcRecipe = ft->GetProto().recipeid();
  ft.reset();

  std::ostringstream s;
  s << fID;
  std::string converted(s.str());

  Process (R"([
    {"name": "domob", "move": {"f": {"d": {"fid": )"+converted+R"(}}}}
  ])");

  for (unsigned i = 0; i < 15; ++i)
  {
    UpdateState ("[]");
  }

  EXPECT_TRUE (tbl3.GetById(fID, ctx.RoConfig()) == nullptr);   // fighter deleted
  EXPECT_TRUE (tbl2.GetById(srcRecipe) == nullptr);             // recipe deleted
  EXPECT_EQ (tbl4.CountForOwner("domob"), 0);

  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetProto().rewards_serial(), 0u);   // no candy recovered -> no credit
  a.reset();
}

TEST_F (ValidateStateTests, RecipeRewardOverflowHoldsAndDrains)
{
  /* Recipe is the only capped reward.  At full recipe slots a recipe reward is
     not lost or credited immediately -- it is HELD as a bounded overflow row and
     auto-drains once a slot frees.  Candy in the same roll always credits. */
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Available);
  ft.reset();
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 3);
  xp.reset();

  proto::ConfigData& cfg = const_cast <proto::ConfigData&>(*ctx.RoConfig());
  cfg.mutable_params()->set_max_recipe_inventory_amount(0);   // every recipe reward overflows

  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "c064e7f7-acbf-4f74-fab8-cccd7b2d4004", "fid": 4}}}}
  ])");

  UpdateState ("[]");   // resolves: c064e7f7 -> 2 Candy credited, 1 CraftedRecipe held (slots full)

  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 0);
  EXPECT_EQ (a->GetProto().rewards_serial(), 2u);   // 2 candy credited; held recipe not yet
  a.reset ();

  ft = tbl3.GetById(4, ctx.RoConfig());
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Available);
  ft.reset();

  /* The recipe reward is HELD as a bounded overflow row (not lost, not credited). */
  EXPECT_EQ (tbl4.CountForOwner("domob"), 1);

  /* Free the recipe slots and drain: the held recipe now lands in ownership. */
  cfg.mutable_params()->set_max_recipe_inventory_amount(48);
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  PXLogic::DrainPendingRecipeRewards (a, ctx, db);
  EXPECT_EQ (a->GetProto().rewards_serial(), 3u);   // held recipe drained -> serial advances
  a.reset ();

  EXPECT_EQ (tbl4.CountForOwner("domob"), 0);        // overflow queue emptied
}

TEST_F (ValidateStateTests, ExpeditionInstanceBusyFighterNotSending)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Available);
  ft.reset();
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 3);
  xp.reset();
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "93ad71bb-cd8f-dc24-7885-2c3fd0013245", "fid": 4}}}}
  ])");  
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 1);
  a.reset ();
   
  ft = tbl3.GetById(4, ctx.RoConfig());
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Expedition);
  ft.reset();
  
  for (unsigned i = 0; i < 1; ++i)
  {
    UpdateState ("[]");
  }
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "93ad71bb-cd8f-dc24-7885-2c3fd0013245", "fid": 4}}}}
  ])");  

  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->GetOngoingsSize (), 1);  
}

TEST_F (ValidateStateTests, ExpeditionTestRewards)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Available);
  ft.reset();
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 3);
  xp.reset();
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "c064e7f7-acbf-4f74-fab8-cccd7b2d4004", "fid": 4}}}}
  ])");  
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 1);
  a.reset ();
   
  ft = tbl3.GetById(4, ctx.RoConfig());
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Expedition);
  ft.reset();
  
  for (unsigned i = 0; i < 1; ++i)
  {
    UpdateState ("[]");
  }

  /* Auto-credit at resolve: c064e7f7's 1 CraftedRecipe + 2 Candy land directly
     (recipe slot free), so nothing sits unclaimed and the serial advanced by 3. */
  EXPECT_EQ (tbl4.CountForOwner("domob"), 0);
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->GetProto().rewards_serial(), 3u);
  a.reset();
}

TEST_F (ValidateStateTests, ExpeditionWithApplicableGoodieTest)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  xp->GetInventory().SetFungibleCount("Goodie_Espresso_1", 1);

  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Available);
  ft.reset();
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 3);
  xp.reset();
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "93ad71bb-cd8f-dc24-7885-2c3fd0013245", "fid": 4}}}}
  ])");  
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 1);
  a.reset ();
   
  ft = tbl3.GetById(4, ctx.RoConfig());
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Expedition);
  ft.reset();
  
  for (unsigned i = 0; i < 14; ++i)
  {
    UpdateState ("[]");
  }
  
  ft = tbl3.GetById(4, ctx.RoConfig());
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Available);
  ft.reset();  
  
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Goodie_Espresso_1"), 0);

}

TEST_F (ValidateStateTests, ExpeditionWithWrongTyprApplicableGoodieTest)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  xp->GetInventory().SetFungibleCount("Goodie_PressureCooker_1", 1);

  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Available);
  ft.reset();
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 3);
  xp.reset();
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "93ad71bb-cd8f-dc24-7885-2c3fd0013245", "fid": 4}}}}
  ])");  
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 1);
  a.reset ();
   
  ft = tbl3.GetById(4, ctx.RoConfig());
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Expedition);
  ft.reset();
  
  
  for (unsigned i = 0; i < 14; ++i)
  {
    UpdateState ("[]");
  }
  
  ft = tbl3.GetById(4, ctx.RoConfig());
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Expedition);
  ft.reset();  
  
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Goodie_PressureCooker_1"), 1);

}

TEST_F (ValidateStateTests, TournamentInstanceSheduleTest)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  int ftA1id = ft->GetId();
  ft.reset();
  
  auto ft2 = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  int ftA2id = ft2->GetId();
  ft2.reset();
  
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 4);
  xp.reset();
  
  UpdateState ("[]");
  
  auto tutorialTrmn = tbl5.GetByAuthIdName("cbd2e78a-37ce-b864-793d-8dd27788a774", ctx.RoConfig());
  ASSERT_TRUE (tutorialTrmn != nullptr);
  uint32_t TID = tutorialTrmn->GetId();
  tutorialTrmn.reset();
  
  std::ostringstream s;
  s << TID;
  std::string converted(s.str());  
  
  std::ostringstream s1;
  s1 << ftA1id;
  std::string converted1(s1.str()); 

  std::ostringstream s2;
  s2 << ftA2id;
  std::string converted2(s2.str());   
  
  Process (R"([
    {"name": "domob", "move": {"tm": {"e": {"tid": )" + converted + R"(, "fc": [)"+converted1+R"(,)"+converted2+R"(]}}}}
  ])");   
  
  auto res = tbl5.QueryAll ();
  ASSERT_TRUE (res.Step ());
  
  ft = tbl3.GetById(ftA1id, ctx.RoConfig());
  EXPECT_EQ (ft->GetProto().tournamentinstanceid(), TID);  
  ft.reset();
  
  //Leave test can be here
  
  Process (R"([
    {"name": "domob", "move": {"tm": {"l": {"tid": )" + converted + R"(}}}}
  ])");     
  
  ft = tbl3.GetById(ftA1id, ctx.RoConfig());
  EXPECT_EQ (ft->GetProto().tournamentinstanceid(), 0);
  ft.reset();

  tutorialTrmn = tbl5.GetByAuthIdName("cbd2e78a-37ce-b864-793d-8dd27788a774", ctx.RoConfig());
  ASSERT_TRUE (tutorialTrmn != nullptr);
  
  EXPECT_EQ (tutorialTrmn->GetInstance().fighters_size(), 0);
  tutorialTrmn.reset(); 
}

TEST_F (ValidateStateTests, TournamentInstanceVeryHighDemandExtraInstanceAreCreatedTest)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  int ftA1id = ft->GetId();
  ft.reset();
  
  auto ft2 = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  int ftA2id = ft2->GetId();
  ft2.reset();
  
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 4);
  xp.reset();
  
  
  auto xp__A = xayaplayers.CreateNew ("domob__A", ctx.RoConfig(), rnd);
  auto ft__A = tbl3.CreateNew ("domob__A", 1, ctx.RoConfig(), rnd);
  int ftA1id__A = ft__A->GetId();
  ft__A.reset();
  
  auto ft2__A = tbl3.CreateNew ("domob__A", 1, ctx.RoConfig(), rnd);
  int ftA2id__A = ft2__A->GetId();
  ft2__A.reset();
  
  EXPECT_EQ (xp__A->CollectInventoryFighters(ctx.RoConfig()).size(), 4);
  xp__A.reset();


  auto xp__B = xayaplayers.CreateNew ("domob__B", ctx.RoConfig(), rnd);
  auto ft__B = tbl3.CreateNew ("domob__B", 1, ctx.RoConfig(), rnd);
  int ftA1id__B = ft__B->GetId();
  ft__B.reset();
  
  auto ft2__B = tbl3.CreateNew ("domob__B", 1, ctx.RoConfig(), rnd);
  int ftA2id__B = ft2__B->GetId();
  ft2__B.reset();
  
  EXPECT_EQ (xp__B->CollectInventoryFighters(ctx.RoConfig()).size(), 4);
  xp__B.reset();  
  
  auto xp__C = xayaplayers.CreateNew ("domob__C", ctx.RoConfig(), rnd);
  auto ft__C = tbl3.CreateNew ("domob__C", 1, ctx.RoConfig(), rnd);
  int ftA1id__C = ft__C->GetId();
  ft__C.reset();
  
  auto ft2__C = tbl3.CreateNew ("domob__C", 1, ctx.RoConfig(), rnd);
  int ftA2id__C = ft2__C->GetId();
  ft2__C.reset();
  
  EXPECT_EQ (xp__C->CollectInventoryFighters(ctx.RoConfig()).size(), 4);
  xp__C.reset();

  auto xp__D = xayaplayers.CreateNew ("domob__D", ctx.RoConfig(), rnd);
  auto ft__D = tbl3.CreateNew ("domob__D", 1, ctx.RoConfig(), rnd);
  int ftA1id__D = ft__D->GetId();
  ft__D.reset();
  
  auto ft2__D = tbl3.CreateNew ("domob__D", 1, ctx.RoConfig(), rnd);
  int ftA2id__D = ft2__D->GetId();
  ft2__D.reset();
  
  EXPECT_EQ (xp__D->CollectInventoryFighters(ctx.RoConfig()).size(), 4);
  xp__D.reset();  
  
  UpdateState ("[]");
  
  auto tutorialTrmn = tbl5.GetByAuthIdName("cbd2e78a-37ce-b864-793d-8dd27788a774", ctx.RoConfig());
  ASSERT_TRUE (tutorialTrmn != nullptr);
  uint32_t TID = tutorialTrmn->GetId();
  tutorialTrmn.reset();
  
  std::ostringstream s;
  s << TID;
  std::string converted(s.str());  
  
  std::ostringstream s1;
  s1 << ftA1id;
  std::string converted1(s1.str()); 

  std::ostringstream s2;
  s2 << ftA2id;
  std::string converted2(s2.str());   
  
  Process (R"([
    {"name": "domob", "move": {"tm": {"e": {"tid": )" + converted + R"(, "fc": [)"+converted1+R"(,)"+converted2+R"(]}}}}
  ])");   
  
  auto res = tbl5.QueryAll ();
  ASSERT_TRUE (res.Step ());
  
  ft = tbl3.GetById(ftA1id, ctx.RoConfig());
  EXPECT_EQ (ft->GetProto().tournamentinstanceid(), TID);  
  ft.reset();
  
 std::ostringstream s__A;
  s__A << TID;
  std::string converted__A(s__A.str());  
  
  std::ostringstream s1__A;
  s1__A << ftA1id__A;
  std::string converted1__A(s1__A.str()); 

  std::ostringstream s2__A;
  s2__A << ftA2id__A;
  std::string converted2__A(s2__A.str());   
  
  std::string serializedDataString = gd.GetQueueData();
  EXPECT_EQ (serializedDataString, "");  
  
  Process (R"([
  {"name": "domob__A", "move": {"tm": {"e": {"tid": )" + converted__A + R"(, "fc": [)"+converted1__A+R"(,)"+converted2__A+R"(]}}}}
  ])");   

  std::ostringstream s__B;
  s__B << TID;
  std::string converted__B(s__B.str());  
  
  std::ostringstream s1__B;
  s1__B << ftA1id__B;
  std::string converted1__B(s1__B.str()); 

  std::ostringstream s2__B;
  s2__B << ftA2id__B;
  std::string converted2__B(s2__B.str());   
  
    Process (R"([
    {"name": "domob__B", "move": {"tm": {"e": {"tid": )" + converted__B + R"(, "fc": [)"+converted1__B+R"(,)"+converted2__B+R"(]}}}}
  ])");     
  
  
  // Now this figter should not be able to join, but should be marked in global data as being in demand
  
  res = tbl5.QueryAll ();
  ASSERT_TRUE (res.Step ());
  
  ft__B = tbl3.GetById(ftA1id__B, ctx.RoConfig());
  EXPECT_NE (ft__B->GetProto().tournamentinstanceid(), TID);  
  ft__B.reset(); 
  
  serializedDataString = gd.GetQueueData();
 
  Json::Value root;
  Json::Reader reader;
  reader.parse(serializedDataString, root);

  std::map<std::string, int32_t> tournamentDemand;

  for (int i = 0; i < root.size(); i++) 
  {		
	  std::string kName = root[i]["tournamentauth"].asString();
		
	  if (tournamentDemand.find(kName) == tournamentDemand.end())
	  {
		  tournamentDemand.insert(std::pair<std::string, int32_t>(kName, 0));
	  }

	  tournamentDemand[kName] += 1;
  } 

  EXPECT_EQ (tournamentDemand["cbd2e78a-37ce-b864-793d-8dd27788a774"], 1);  
  
  std::ostringstream s__C;
  s__C << TID;
  std::string converted__C(s__C.str());  
  
  std::ostringstream s1__C;
  s1__C << ftA1id__C;
  std::string converted1__C(s1__C.str()); 

  std::ostringstream s2__C;
  s2__C << ftA2id__C;
  std::string converted2__C(s2__C.str());   
  
    Process (R"([
    {"name": "domob__C", "move": {"tm": {"e": {"tid": )" + converted__C + R"(, "fc": [)"+converted1__C+R"(,)"+converted2__C+R"(]}}}}
  ])");  

  UpdateState ("[]");

  // Ok then, now because we had demand of 2, realistically we should have additional instance of this tournament being created
  
  int totalListed = 0;
  int totalRunning = 0;
  res = tbl5.QueryAll ();
  bool tryAndStep = res.Step();
  while (tryAndStep)
  {
      auto tnm = tbl5.GetFromResult (res, ctx.RoConfig ()); 
	  
	  if((pxd::TournamentState)(int)tnm->GetInstance().state() == pxd::TournamentState::Listed)
	  {
		if(tnm->GetProto().authoredid() == "cbd2e78a-37ce-b864-793d-8dd27788a774")
		{
			totalListed++;
		}
	  }
	  
	  if((pxd::TournamentState)(int)tnm->GetInstance().state() == pxd::TournamentState::Running)
	  {
		if(tnm->GetProto().authoredid() == "cbd2e78a-37ce-b864-793d-8dd27788a774")
		{
			totalRunning++;
		}
	  }	  
	  
	  tnm.reset();
	  tryAndStep = res.Step();
  }
  
  EXPECT_EQ (totalListed, 2); 
  EXPECT_EQ (totalRunning, 1);

  serializedDataString = gd.GetQueueData();
  EXPECT_EQ (serializedDataString, "");   
}

TEST_F (ValidateStateTests, TournamentResolvedTest)
{
  /* This test predates the balance change; pin the two knobs to their no-op
     values so it keeps asserting pure reward auto-credit (divisor 1 = legacy
     reward outcomes; kills disabled = no permadeath serial bump / fighter loss). */
  GameParams (db).SetParam ("rarest_recipe_drop_divisor", 1);
  GameParams (db).SetParam ("tournament_loss_kills_enabled", 0);

  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  int ftA1idx = ft->GetId();
  ft.reset();
  
  auto ft2 = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  int ftA2idx = ft2->GetId();
  ft2.reset();
  
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 4);
  xp.reset();
  
  UpdateState ("[]");
  
  auto tutorialTrmn = tbl5.GetByAuthIdName("cbd2e78a-37ce-b864-793d-8dd27788a774", ctx.RoConfig());
  ASSERT_TRUE (tutorialTrmn != nullptr);
  uint32_t TID = tutorialTrmn->GetId();
  tutorialTrmn.reset();
  
  std::ostringstream s;
  s << TID;
  std::string converted(s.str());  
  
  std::ostringstream s1x;
  s1x << ftA1idx;
  std::string converted1x(s1x.str()); 

  std::ostringstream s2x;
  s2x << ftA2idx;
  std::string converted2x(s2x.str());  
  
  Process (R"([
    {"name": "domob", "move": {"tm": {"e": {"tid": )" + converted + R"(, "fc": [)"+converted1x+R"(,)"+converted2x+R"(]}}}}
  ])");     
  

  auto xp2 = xayaplayers.CreateNew ("andy", ctx.RoConfig(), rnd);
  auto ftA = tbl3.CreateNew ("andy", 1, ctx.RoConfig(), rnd);
  uint32_t ftA1id = ftA->GetId();
  ftA.reset();
  
  auto ftA2 = tbl3.CreateNew ("andy", 1, ctx.RoConfig(), rnd);
  uint32_t ftA2id = ftA2->GetId();
  ftA2.reset();
  
  EXPECT_EQ (xp2->CollectInventoryFighters(ctx.RoConfig()).size(), 4);
  xp2->CalculatePrestige(ctx.RoConfig());
  xp2.reset();
  
  UpdateState ("[]");
  
  EXPECT_EQ (tbl4.CountForOwner("domob"), 0);
  
  std::ostringstream s1;
  s1 << ftA1id;
  std::string converted1(s1.str()); 

  std::ostringstream s2;
  s2 << ftA2id;
  std::string converted2(s2.str()); 
  
  Process (R"([
    {"name": "andy", "move": {"tm": {"e": {"tid": )" + converted + R"(, "fc": [)" + converted1 + R"(,)" + converted2 + R"(]}}}}
  ])");    

  for (unsigned i = 0; i < 3; ++i)
  {
    UpdateState ("[]");
  }
  
  /* Auto-credit at resolve: the tutorial tournament rolls only candy + a crafted
     recipe (no fighter-bound move/armor/anim, which a fighterID-0 tournament
     reward would drop), so every reward lands directly and nothing sits
     unclaimed.  Loser domob gets 7, winner andy gets 10. */
  EXPECT_EQ (tbl4.CountForOwner("domob"), 0);
  EXPECT_EQ (tbl4.CountForOwner("andy"), 0);

  auto pd = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (pd->GetProto().rewards_serial(), 7u);
  pd.reset();
  auto pa = xayaplayers.GetByName ("andy", ctx.RoConfig());
  EXPECT_EQ (pa->GetProto().rewards_serial(), 10u);
  pa.reset();

  tutorialTrmn = tbl5.GetById(TID, ctx.RoConfig());
  EXPECT_EQ (tutorialTrmn->GetInstance().results_size(), 4);
  tutorialTrmn.reset();
}

TEST_F (ValidateStateTests, FighterSacrifice)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  xp.reset();

  auto r0f = tbl2.CreateNew("domob", "5864a19b-c8c0-2d34-eaef-9455af0baf2c", ctx.RoConfig());
  int r0fid = r0f->GetId();
  r0f.reset();  

  auto r0 = tbl2.CreateNew("domob", "c1e9a4e1-cf55-6084-8bf2-778678387353", ctx.RoConfig());
  int rA1idx = r0->GetId();
  r0.reset();  
  
  auto ft = tbl3.CreateNew ("domob", r0fid, ctx.RoConfig(), rnd);
  int ftA1idx = ft->GetId();
  ft.reset();  
  
  xp = xayaplayers.GetByName ("domob", ctx.RoConfig());
  xp->GetInventory().SetFungibleCount("Epic_Gold Candy Ribbon", 150);
  xp->GetInventory().SetFungibleCount("Epic_Silver Icing", 130);
  xp->GetInventory().SetFungibleCount("Epic_Bronze Jawbreaker", 125);
  xp->GetInventory().SetFungibleCount("Rare_Hard Candy", 130);
  xp->GetInventory().SetFungibleCount("Common_Candy Button", 175);
  xp->GetInventory().SetFungibleCount("Uncommon_Cotton Candy", 150);
  xp->GetInventory().SetFungibleCount("Uncommon_Peppermint", 125);
  xp->GetInventory().SetFungibleCount("Common_Ring Pop", 150);
  
  xp->GetInventory().SetFungibleCount("Rare_Toffee Chunk", 175);
  xp->GetInventory().SetFungibleCount("Rare_Candy Loops", 150);
  xp->GetInventory().SetFungibleCount("Uncommon_Banana Chew", 125);
  xp->GetInventory().SetFungibleCount("Uncommon_Jelly Bean", 150);
  
  xp.reset();

  std::ostringstream s1;
  s1 << rA1idx;
  std::string converted1(s1.str()); 

  std::ostringstream s2;
  s2 << ftA1idx;
  std::string converted2(s2.str());       

  Process (R"([
    {"name": "domob", "move": {"ca": {"r": {"rid": )" +converted1+ R"(, "fid": )"+converted2+R"(}}}}
  ])"); 
  
  xp = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (xp->GetOngoingsSize (), 0);
  xp.reset();
    
}

TEST_F (ValidateStateTests, RatingSweetnessUpgrades)
{
  /* Predates permadeath; this test drives many tournaments to climb rating ->
     sweetness, so disable permadeath here to keep the tracked fighters alive
     across rounds (the mechanic is covered by the TournamentPermadeath* tests). */
  GameParams (db).SetParam ("tournament_loss_kills_enabled", 0);

  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  xp->AddBalance(100);
  auto ft = tbl3.CreateNew ("domob", 2, ctx.RoConfig(), rnd);
  int ftA1idx = ft->GetId();
  ft.reset();
  
  auto ft2 = tbl3.CreateNew ("domob", 2, ctx.RoConfig(), rnd);
  int ftA2idx = ft2->GetId();
  ft2.reset();
  
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 4);
  xp.reset();
  
  UpdateState ("[]");
  
  auto xp2 = xayaplayers.CreateNew ("andy", ctx.RoConfig(), rnd);
  auto ftA = tbl3.CreateNew ("andy", 1, ctx.RoConfig(), rnd);
  uint32_t ftA1id = ftA->GetId();
  EXPECT_EQ (ftA->GetProto().rating(), 1000);
  EXPECT_EQ (ftA->GetProto().sweetness(), (int)pxd::Sweetness::Not_Too_Sweet);
  ftA.reset();
  
  auto ftA2 = tbl3.CreateNew ("andy", 1, ctx.RoConfig(), rnd);
  uint32_t ftA2id = ftA2->GetId();
  ftA2.reset();
  
  EXPECT_EQ (xp2->CollectInventoryFighters(ctx.RoConfig()).size(), 4);
  xp2->CalculatePrestige(ctx.RoConfig());
  xp2.reset();
  
  UpdateState ("[]");
  
  EXPECT_EQ (tbl4.CountForOwner("domob"), 0);
        
  std::ostringstream s1x;
  s1x << ftA1idx;
  std::string converted1x(s1x.str()); 

  std::ostringstream s2x;
  s2x << ftA2idx;
  std::string converted2x(s2x.str());        
    
  std::ostringstream s1;
  s1 << ftA1id;
  std::string converted1(s1.str()); 

  std::ostringstream s2;
  s2 << ftA2id;
  std::string converted2(s2.str());       
    
  for (unsigned r = 0; r < 10; ++r)
  {   
    auto tutorialTrmn = tbl5.GetByAuthIdName("cbd2e78a-37ce-b864-793d-8dd27788a774", ctx.RoConfig());
    ASSERT_TRUE (tutorialTrmn != nullptr);
    uint32_t TID = tutorialTrmn->GetId();
    tutorialTrmn.reset();  

    std::ostringstream s;
    s << TID;
    std::string converted(s.str()); 

    Process (R"([
      {"name": "domob", "move": {"tm": {"e": {"tid": )" + converted + R"(, "fc": [)"+converted1x+R"(,)"+converted2x+R"(]}}}}
    ])");     

    Process (R"([
      {"name": "andy", "move": {"tm": {"e": {"tid": )" + converted + R"(, "fc": [)" + converted1 + R"(,)" + converted2 + R"(]}}}}
    ])");    

    for (unsigned i = 0; i < 3; ++i)
    {
      UpdateState ("[]");
    }

    /* Rewards auto-credit at resolve (no claim); rating/sweetness come from the
       combat resolution, so the accumulated upgrades are unaffected. */
    tbl5.DeleteById(TID);

    UpdateState ("[]");
  }
  
  ftA = tbl3.GetById(ftA1id, ctx.RoConfig());
  EXPECT_EQ (ftA->GetProto().rating(), 1127);
  EXPECT_EQ (ftA->GetProto().sweetness(), (int)pxd::Sweetness::Bittersweet);
  
  ftA->MutableProto().set_rating(2000);
  ftA->UpdateSweetness();
  EXPECT_EQ (ftA->GetProto().sweetness(), (int)pxd::Sweetness::Super_Sweet);
  
  ftA.reset();  
}
/*
TEST_F (ValidateStateTests, ExpeditionRewardBalance)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  int ftA1idx = ft->GetId();
  ft.reset();
  xp.reset();  
	
  UpdateState ("[]");	

  std::vector<std::string> expeditionAuthNames;
  

	expeditionAuthNames.push_back("a2512eaa-028a-1f84-6879-eb240ac80a3e");
	expeditionAuthNames.push_back("93ad71bb-cd8f-dc24-7885-2c3fd0013245");
	expeditionAuthNames.push_back("35052272-b4bc-75c4-0ad0-fe122f54b508");
	expeditionAuthNames.push_back("a1387c26-3e1c-4554-e94e-e857643e58ed");
	expeditionAuthNames.push_back("fbb0edc6-92a9-dc04-5ad7-a19c36843074");
	expeditionAuthNames.push_back("6d7e16b9-1a8f-b134-59a6-fb9865615001");
	expeditionAuthNames.push_back("768a4c69-c928-4a34-1bf2-044d3e5bac33");
	expeditionAuthNames.push_back("5edaf5d1-ec6d-7094-a8dd-a344d0650e5c");
	expeditionAuthNames.push_back("0d9af5ec-b3fd-3de4-2a38-0a954c9f9cba");
	expeditionAuthNames.push_back("19bbdaff-2ba7-12f4-3a13-989a490aaf86");
	expeditionAuthNames.push_back("dc498167-f8b7-3164-da5b-776b731a1a6c");
	expeditionAuthNames.push_back("f0e675df-3f16-e4b4-f9e2-6b5291a92b0e");
	expeditionAuthNames.push_back("16457184-f0f1-73e4-183d-e7f8f2d412c2");
	expeditionAuthNames.push_back("c06ccaea-47e5-ec14-bb62-d824095a1bae");
	expeditionAuthNames.push_back("09b9f6ba-cc74-6ad4-dbe9-86759590d0fc");
	expeditionAuthNames.push_back("c088ddf8-2b2e-b9c4-6bb2-52aa1af2584f");
	expeditionAuthNames.push_back("8928eb04-3ee7-f834-9b5b-8b6f48d04ecf");
	expeditionAuthNames.push_back("5c5c9f0e-4eda-bbe4-ba92-ec3c400d081e");
	expeditionAuthNames.push_back("aece8a9f-30c0-3aa4-7a9b-5c33fc2e534a");
	expeditionAuthNames.push_back("15840d95-57de-86a4-da5e-9ab9dfb89299");
	
  std::vector<fpm::fixed_24_8> candiesTT;
  std::vector<fpm::fixed_24_8> recepiesTT;
  
				


  std::vector<fpm::fixed_24_8> targetCandies;		
  std::vector<fpm::fixed_24_8> targetRecepies;	  
  
  targetCandies.push_back(fpm::fixed_24_8(120 / 5));
  targetCandies.push_back(fpm::fixed_24_8(120 / 5));
  targetCandies.push_back(fpm::fixed_24_8(168 / 5));
  targetCandies.push_back(fpm::fixed_24_8(156 / 5));
  targetCandies.push_back(fpm::fixed_24_8(288 / 5));
  targetCandies.push_back(fpm::fixed_24_8(108 / 5));
  targetCandies.push_back(fpm::fixed_24_8(216 / 5));
  targetCandies.push_back(fpm::fixed_24_8(198 / 5));
  targetCandies.push_back(fpm::fixed_24_8(480 / 5));
  targetCandies.push_back(fpm::fixed_24_8(390 / 5));
  targetCandies.push_back(fpm::fixed_24_8(560 / 5));
  targetCandies.push_back(fpm::fixed_24_8(450 / 5));
  targetCandies.push_back(fpm::fixed_24_8(320 / 5));
  targetCandies.push_back(fpm::fixed_24_8(320 / 5));
  targetCandies.push_back(fpm::fixed_24_8(837 / 5));
  targetCandies.push_back(fpm::fixed_24_8(670 / 5));
  targetCandies.push_back(fpm::fixed_24_8(586 / 5));
  targetCandies.push_back(fpm::fixed_24_8(586 / 5));
  targetCandies.push_back(fpm::fixed_24_8(1005 / 5));
  targetCandies.push_back(fpm::fixed_24_8(1005 / 5));
  
  targetRecepies.push_back(fpm::fixed_24_8(0.3));
  targetRecepies.push_back(fpm::fixed_24_8(0.3));
  targetRecepies.push_back(fpm::fixed_24_8(0.4));
  targetRecepies.push_back(fpm::fixed_24_8(0.4));
  targetRecepies.push_back(fpm::fixed_24_8(0.5));
  targetRecepies.push_back(fpm::fixed_24_8(0.2));
  targetRecepies.push_back(fpm::fixed_24_8(0.2));
  targetRecepies.push_back(fpm::fixed_24_8(0.3));
  targetRecepies.push_back(fpm::fixed_24_8(0.5));
  targetRecepies.push_back(fpm::fixed_24_8(0.2));
  targetRecepies.push_back(fpm::fixed_24_8(0.55));
  targetRecepies.push_back(fpm::fixed_24_8(0.412));
  targetRecepies.push_back(fpm::fixed_24_8(0.3));
  targetRecepies.push_back(fpm::fixed_24_8(0.3));
  targetRecepies.push_back(fpm::fixed_24_8(0.5));
  targetRecepies.push_back(fpm::fixed_24_8(0.4));
  targetRecepies.push_back(fpm::fixed_24_8(0.35));
  targetRecepies.push_back(fpm::fixed_24_8(0.35));
  targetRecepies.push_back(fpm::fixed_24_8(0.6));
  targetRecepies.push_back(fpm::fixed_24_8(0.6));	
  
  const auto& expeditionList = ctx.RoConfig()->expeditionblueprints();
  
  for(int p =0; p < 1000; p++)
  {
	  std::vector<fpm::fixed_24_8> candiesT;
      std::vector<fpm::fixed_24_8> recepiesT;
	  
	  for(auto& blueprintAuthID: expeditionAuthNames)
	  {
		  fpm::fixed_24_8 CT = fpm::fixed_24_8(0);
		  fpm::fixed_24_8 RT = fpm::fixed_24_8(0);
		  
            uint32_t rollCount  = 0;
			bool blueprintSolved = false;
			std::string basedRewardsTableAuthId = "";
			
			for(const auto& expedition: expeditionList)
			{
				if(expedition.second.authoredid() == blueprintAuthID)
				{
					rollCount = expedition.second.baserollcount();
					basedRewardsTableAuthId = expedition.second.baserewardstableid();
					blueprintSolved = true;
					break;
				}
			}   
			
			if(blueprintSolved == false)
			{
				LOG (WARNING) << "Could not resolve expedition in logic blueprint with authID: " << blueprintAuthID;
				return;              
			}
			
			pxd::proto::ActivityReward rewardTableDb;
			
			const auto& rewardsList = ctx.RoConfig()->activityrewards();
			bool rewardsSolved = false;
			
			for(const auto& rewardsTable: rewardsList)
			{
				if(rewardsTable.second.authoredid() == basedRewardsTableAuthId)
				{
					rewardTableDb = rewardsTable.second;
					rewardsSolved = true;
					break;
				}
			}     

			if(rewardsSolved == false)
			{
				LOG (WARNING) << "Could not resolve expedition rewards in logic  with authID: " << blueprintAuthID;
				return;             
			}
								 
		  fpm::fixed_24_8 totalWeight = fpm::fixed_24_8(0);
		  for(auto& rw: rewardTableDb.rewards())
		  {
			 totalWeight = totalWeight + fpm::fixed_24_8(rw.weight());
		  }

		  std::vector<uint32_t> totalRewardIds;

		  for(uint32_t roll = 0; roll < rollCount; ++roll)
		  {
			  int rolCurNum = 0;
			  
			  if(totalWeight != fpm::fixed_24_8(0) && (int)totalWeight != 0)
			  {
				rolCurNum = rnd.NextInt((int)totalWeight);
			  }
			  
			  fpm::fixed_24_8 accumulatedWeight = fpm::fixed_24_8(0);
			  int posInTableList = 0;
			  for(auto& rw: rewardTableDb.rewards())
			  {
				  accumulatedWeight = accumulatedWeight + fpm::fixed_24_8(rw.weight());
				  fpm::fixed_24_8  rolCurNumC = fpm::fixed_24_8(rolCurNum);
				  
				  if(rolCurNumC <= accumulatedWeight)
				  {
					  if((pxd::RewardType)(int)rw.type() == pxd::RewardType::Candy)
					  {		   
						   CT = CT + fpm::fixed_24_8(rw.quantity());
					  }
					  else if((pxd::RewardType)(int)rw.type() == pxd::RewardType::CraftedRecipe || (pxd::RewardType)(int)rw.type() == pxd::RewardType::GeneratedRecipe)
					  {
						   RT = RT + fpm::fixed_24_8(1);
					  }
					  else
					  {
						  RT = RT + fpm::fixed_24_8(1);
					  }
			  
					 
					 break;
				  }
				  
				   posInTableList++;
			  }
		  }      

		  candiesT.push_back(CT);
		  recepiesT.push_back(RT);
	  }
	  
	  if(candiesTT.size() == 0)
	  {
	    for(int r2 =0; r2 < candiesT.size(); r2++)
        {	
	       candiesTT.push_back(candiesT[r2]);
		   recepiesTT.push_back(recepiesT[r2]);
		}	
	  }
	  else
	  {
	    for(int r2 =0; r2 < candiesT.size(); r2++)
        {	
	       candiesTT[r2] = (candiesTT[r2] + candiesT[r2]) / fpm::fixed_24_8(2);
		   recepiesTT[r2] = (recepiesTT[r2] + recepiesT[r2]) / fpm::fixed_24_8(2);
		}			  
	  }
  }
  
  for(int r =0; r < expeditionAuthNames.size(); r++)
  {
	 
	 fpm::fixed_24_8 div = targetCandies[r];
	 if(div == fpm::fixed_24_8(0)) div = fpm::fixed_24_8(1);
	 
	 //LOG (WARNING) << "{\"" << expeditionAuthNames[r] << "\", fpm::fixed_24_8(" << (double)(candiesTT[r]/ div) << ")},";
	 
	 //LOG (WARNING) << (double)(candiesTT[r]/ div) << "," <<  (double)(recepiesTT[r] / targetRecepies[r]);
   
     //LOG (WARNING) << "{\"" << expeditionAuthNames[r] << "\", fpm::fixed_24_8(" << ((int)(1000.0 * ((double)((double)recepiesTT[r] / (double)targetRecepies[r])))) << ")},";
   
	 LOG (WARNING) << expeditionAuthNames[r] << " produced CT:" << (double)candiesTT[r] <<"|" << (double)targetCandies[r] << " and RT:" << (double)recepiesTT[r] <<"|"<< (double)targetRecepies[r];  
  }  
	
}*/

/*
TEST_F (ValidateStateTests, TournamentRewardBalance)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  int ftA1idx = ft->GetId();
  ft.reset();
  xp.reset();  
	
  UpdateState ("[]");	

  std::vector<std::string> tournamentAuthNames;
  
  tournamentAuthNames.push_back("cbd2e78a-37ce-b864-793d-8dd27788a774");
  tournamentAuthNames.push_back("e694d5f8-e454-7774-ca76-fc2637a9407f");
  tournamentAuthNames.push_back("5569ff18-4504-0a54-8b54-518ad7501db8");
  tournamentAuthNames.push_back("1dbea2e9-1abf-8524-5bb9-4c27d7d4f631");
  tournamentAuthNames.push_back("0c7385d1-d807-1634-4ae1-9eb4e9991b17");
  tournamentAuthNames.push_back("eedb6522-2311-3ef4-c999-d3ec275ea496");
  tournamentAuthNames.push_back("f6cbb7e0-a2f3-3e14-2be5-477eeefe8963");
  tournamentAuthNames.push_back("99258908-ce4f-50e4-2880-99f0027b8d2b");
  tournamentAuthNames.push_back("e17e19da-139b-c484-2bc2-6eec8d407c8a");
  tournamentAuthNames.push_back("34c5d1a4-0245-3104-6a35-e765865124b1");
  tournamentAuthNames.push_back("06cb83c5-def7-cbc4-4956-b53f755c075a");
  tournamentAuthNames.push_back("1af67bae-6ab2-29e4-9b62-805c73901881");
  tournamentAuthNames.push_back("b714d4e1-b463-dd14-c943-b3d8a3677a0e");
  tournamentAuthNames.push_back("ec723560-ccfa-c984-89a6-f578b5387ce9");
  tournamentAuthNames.push_back("fbc32cb0-c5f4-4884-1a53-f38f1c57e357");
  tournamentAuthNames.push_back("9ab085a2-2247-adc4-1857-68c3a31b20c3");
  tournamentAuthNames.push_back("77a516a8-4824-fbf4-ca26-42d18e610a7a");
  tournamentAuthNames.push_back("8dc57404-ec23-94d4-3919-5ed9e5d4e37f");
  tournamentAuthNames.push_back("06819c56-b599-d864-dbed-b1df4513ca11");
  tournamentAuthNames.push_back("e1cb93b2-3ba6-9494-6bd6-9e81994480ee");
  tournamentAuthNames.push_back("dd89aedb-06c1-1964-d90b-b5977ee61c4c");
	
  std::vector<fpm::fixed_24_8> candiesTT;
  std::vector<fpm::fixed_24_8> recepiesTT;
  
  std::vector<fpm::fixed_24_8> targetCandies;		
  std::vector<fpm::fixed_24_8> targetRecepies;	  
  
  targetCandies.push_back(fpm::fixed_24_8(0));
  targetCandies.push_back(fpm::fixed_24_8(120));
  targetCandies.push_back(fpm::fixed_24_8(120));
  targetCandies.push_back(fpm::fixed_24_8(168));
  targetCandies.push_back(fpm::fixed_24_8(156));
  targetCandies.push_back(fpm::fixed_24_8(288));
  targetCandies.push_back(fpm::fixed_24_8(108));
  targetCandies.push_back(fpm::fixed_24_8(216));
  targetCandies.push_back(fpm::fixed_24_8(198));
  targetCandies.push_back(fpm::fixed_24_8(480));
  targetCandies.push_back(fpm::fixed_24_8(390));
  targetCandies.push_back(fpm::fixed_24_8(560));
  targetCandies.push_back(fpm::fixed_24_8(450));
  targetCandies.push_back(fpm::fixed_24_8(320));
  targetCandies.push_back(fpm::fixed_24_8(320));
  targetCandies.push_back(fpm::fixed_24_8(837));
  targetCandies.push_back(fpm::fixed_24_8(670));
  targetCandies.push_back(fpm::fixed_24_8(586));
  targetCandies.push_back(fpm::fixed_24_8(586));
  targetCandies.push_back(fpm::fixed_24_8(1005));
  targetCandies.push_back(fpm::fixed_24_8(1005));
  
  targetRecepies.push_back(fpm::fixed_24_8(4));
  targetRecepies.push_back(fpm::fixed_24_8(1.5));
  targetRecepies.push_back(fpm::fixed_24_8(1.5));
  targetRecepies.push_back(fpm::fixed_24_8(2.1));
  targetRecepies.push_back(fpm::fixed_24_8(1.95));
  targetRecepies.push_back(fpm::fixed_24_8(2.40));
  targetRecepies.push_back(fpm::fixed_24_8(0.9));
  targetRecepies.push_back(fpm::fixed_24_8(1.80));
  targetRecepies.push_back(fpm::fixed_24_8(1.65));
  targetRecepies.push_back(fpm::fixed_24_8(2.40));
  targetRecepies.push_back(fpm::fixed_24_8(1.95));
  targetRecepies.push_back(fpm::fixed_24_8(2.80));
  targetRecepies.push_back(fpm::fixed_24_8(2.25));
  targetRecepies.push_back(fpm::fixed_24_8(1.60));
  targetRecepies.push_back(fpm::fixed_24_8(1.60));
  targetRecepies.push_back(fpm::fixed_24_8(2.50));
  targetRecepies.push_back(fpm::fixed_24_8(2.00));
  targetRecepies.push_back(fpm::fixed_24_8(1.75));
  targetRecepies.push_back(fpm::fixed_24_8(1.75));
  targetRecepies.push_back(fpm::fixed_24_8(3.00));
  targetRecepies.push_back(fpm::fixed_24_8(3.00));

  
  for(int p =0; p < 1000; p++)
  {
	  std::vector<fpm::fixed_24_8> candiesT;
      std::vector<fpm::fixed_24_8> recepiesT;
	  
	  for(auto& tAuthID: tournamentAuthNames)
	  {
		  fpm::fixed_24_8 CT = fpm::fixed_24_8(0);
		  fpm::fixed_24_8 RT = fpm::fixed_24_8(0);
		  
		  xp = xayaplayers.GetByName ("domob", ctx.RoConfig());
		  auto tnm = tbl5.GetByAuthIdName(tAuthID, ctx.RoConfig()); //The Chocolate Chip Pee Wee Cup
		  ASSERT_TRUE (tnm != nullptr);
		  std::string rewardTableId = tnm->GetProto().baserewardstableid();
		  uint32_t rollCount = tnm->GetProto().baserollcount();
		  bool winner = true;
		  
		  if(winner)
		  {
			 rewardTableId = tnm->GetProto().winnerrewardstableid();  
			 rollCount = tnm->GetProto().winnerrollcount();
		  }
		  
		  rollCount = rollCount * tnm->GetProto().teamsize();
		  
		  pxd::proto::ActivityReward rewardTableDb;
			  
		  const auto& rewardsList = ctx.RoConfig()->activityrewards();
		  bool rewardsSolved = false;
			  
		  for(const auto& rewardsTable: rewardsList)
		  {
			if(rewardsTable.second.authoredid() == rewardTableId)
			{
				rewardTableDb = rewardsTable.second;
				rewardsSolved = true;
				break;
			}
		  }     

		  if(rewardsSolved == false)
		  {
			  LOG (WARNING) << "Could not resolve expedition rewards in logic  with authID: " << rewardTableId;
			  tnm.reset();
			  return;             
		  }
								 
		  fpm::fixed_24_8 totalWeight = fpm::fixed_24_8(0);
		  for(auto& rw: rewardTableDb.rewards())
		  {
			 totalWeight = totalWeight + fpm::fixed_24_8(rw.weight());
		  }

		  std::vector<uint32_t> totalRewardIds;

		  for(uint32_t roll = 0; roll < rollCount; ++roll)
		  {
			  int rolCurNum = 0;
			  
			  if(totalWeight != fpm::fixed_24_8(0))
			  {
				rolCurNum = rnd.NextInt((int)totalWeight);
			  }
			  
			  fpm::fixed_24_8 accumulatedWeight = fpm::fixed_24_8(0);
			  int posInTableList = 0;
			  for(auto& rw: rewardTableDb.rewards())
			  {
				  accumulatedWeight = accumulatedWeight + fpm::fixed_24_8(rw.weight());
				  fpm::fixed_24_8  rolCurNumC = fpm::fixed_24_8(rolCurNum);
				  
				  if(rolCurNumC <= accumulatedWeight)
				  {

					  if((pxd::RewardType)(int)rw.type() == pxd::RewardType::Candy)
					  {		   
						   CT = CT + fpm::fixed_24_8(rw.quantity());
					  }
					  else if((pxd::RewardType)(int)rw.type() == pxd::RewardType::CraftedRecipe || (pxd::RewardType)(int)rw.type() == pxd::RewardType::GeneratedRecipe)
					  {
						   RT = RT + fpm::fixed_24_8(1);
					  }
					  else
					  {
						  RT = RT + fpm::fixed_24_8(1);
					  }
			  
					 
					 break;
				  }
				  
				   posInTableList++;
			  }
		  }      


		  tnm.reset();  
		  xp.reset();
		  
		  candiesT.push_back(CT);
		  recepiesT.push_back(RT);
	  }
	  
	  if(candiesTT.size() == 0)
	  {
	    for(int r2 =0; r2 < candiesT.size(); r2++)
        {	
	       candiesTT.push_back(candiesT[r2]);
		   recepiesTT.push_back(recepiesT[r2]);
		}	
	  }
	  else
	  {
	    for(int r2 =0; r2 < candiesT.size(); r2++)
        {	
	       candiesTT[r2] = (candiesTT[r2] + candiesT[r2]) / fpm::fixed_24_8(2);
		   recepiesTT[r2] = (recepiesTT[r2] + recepiesT[r2]) / fpm::fixed_24_8(2);
		}			  
	  }
  }
  
  for(int r =0; r < tournamentAuthNames.size(); r++)
  {
	 
	 fpm::fixed_24_8 div = targetCandies[r];
	 if(div == fpm::fixed_24_8(0)) div = fpm::fixed_24_8(1);
	 
	 //LOG (WARNING) << "{\"" << tournamentAuthNames[r] << "\", fpm::fixed_24_8(" << (double)(candiesTT[r]/ div) << ")},";
	 
	 //LOG (WARNING) << (double)(candiesTT[r]/ div) << "," <<  (double)(recepiesTT[r] / targetRecepies[r]);
   
     //LOG (WARNING) << "{\"" << tournamentAuthNames[r] << "\", fpm::fixed_24_8(" << ((int)(1000.0 * ((double)((double)recepiesTT[r] / (double)targetRecepies[r])))) << ")},";
   
	 LOG (WARNING) << tournamentAuthNames[r] << " produced CT:" << (double)candiesTT[r] <<"|" << (double)targetCandies[r] << " and RT:" << (double)recepiesTT[r] <<"|"<< (double)targetRecepies[r];  
  }
}*/

TEST_F (ValidateStateTests, SweetnessRatingStaysCapped)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  int ftA1idx = ft->GetId();
  ft.reset();
  
  auto r0 = tbl2.CreateNew("domob", "7b7d8898-7f58-0334-0bad-825dc87a6638", ctx.RoConfig());
  uint32_t strongRecepieID = r0->GetId();
  r0.reset();  
  
  auto ft2VeryStrongFighter = tbl3.CreateNew ("domob", strongRecepieID, ctx.RoConfig(), rnd);
  int ftA2idx = ft2VeryStrongFighter->GetId();
  ft2VeryStrongFighter.reset();
  
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 4);
  xp->CalculatePrestige(ctx.RoConfig());
  xp.reset();
  
  UpdateState ("[]");
  
  auto tutorialTrmn = tbl5.GetByAuthIdName("cbd2e78a-37ce-b864-793d-8dd27788a774", ctx.RoConfig());
  ASSERT_TRUE (tutorialTrmn != nullptr);
  uint32_t TID = tutorialTrmn->GetId();
  tutorialTrmn.reset();
  
  std::ostringstream s;
  s << TID;
  std::string converted(s.str());  
  
  std::ostringstream s1x;
  s1x << ftA1idx;
  std::string converted1x(s1x.str()); 

  std::ostringstream s2x;
  s2x << ftA2idx;
  std::string converted2x(s2x.str());   
  
  Process (R"([
    {"name": "domob", "move": {"tm": {"e": {"tid": )" + converted + R"(, "fc": [)" + converted1x + R"(,)" + converted2x + R"(]}}}}
  ])");   
  
  auto xp2 = xayaplayers.CreateNew ("andy", ctx.RoConfig(), rnd);
  auto ftA = tbl3.CreateNew ("andy", 1, ctx.RoConfig(), rnd);
  uint32_t ftA1id = ftA->GetId();
  ftA.reset();
  
  auto ftA2 = tbl3.CreateNew ("andy", 1, ctx.RoConfig(), rnd);
  uint32_t ftA2id = ftA2->GetId();
  ftA2.reset();
  
  EXPECT_EQ (xp2->CollectInventoryFighters(ctx.RoConfig()).size(), 4);
  xp2->CalculatePrestige(ctx.RoConfig());
  xp2.reset();
  
  UpdateState ("[]");
  
  EXPECT_EQ (tbl4.CountForOwner("domob"), 0);
  
  std::ostringstream s1;
  s1 << ftA1id;
  std::string converted1(s1.str()); 

  std::ostringstream s2;
  s2 << ftA2id;
  std::string converted2(s2.str()); 
  
  Process (R"([
    {"name": "andy", "move": {"tm": {"e": {"tid": )" + converted + R"(, "fc": [)" + converted1 + R"(,)" + converted2 + R"(]}}}}
  ])");    

  for (unsigned i = 0; i < 3; ++i)
  {
    UpdateState ("[]");
  }
  
  /* Auto-credit at resolve: candy + crafted recipe land directly, nothing sits
     unclaimed; winner domob credited 10, loser andy 7. */
  EXPECT_EQ (tbl4.CountForOwner("domob"), 0);
  EXPECT_EQ (tbl4.CountForOwner("andy"), 0);
  {
    auto pdom = xayaplayers.GetByName ("domob", ctx.RoConfig());
    EXPECT_EQ (pdom->GetProto().rewards_serial(), 10u);
    auto pand = xayaplayers.GetByName ("andy", ctx.RoConfig());
    EXPECT_EQ (pand->GetProto().rewards_serial(), 7u);
  }

  tutorialTrmn = tbl5.GetById(TID, ctx.RoConfig());

  for(const auto& result: tutorialTrmn->GetInstance().results())
  {
     EXPECT_LT(result.ratingdelta(), 100000);
     EXPECT_GE(result.ratingdelta(), -100000);
  }   
}

TEST_F (ValidateStateTests, PrestigeValueTest)
{
  // Firstly we need to make sure, that maximum prestige values can
  // be larger then 14000, which is minimum for tier 7 grade
  
  // For this, lets 1) award player all epic treats
  //                2) set large wins count
  //                3) fighters set maximum rating
  //                4) roll for rare names
  
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);

  for (unsigned i = 0; i < 48; ++i)
  {
	  auto r0 = tbl2.CreateNew("domob", "c481aeee-d1a1-01c4-7aca-92d0edcddf18", ctx.RoConfig());
	  uint32_t strongRecepieID = r0->GetId();
	  r0.reset();  
	  
	  auto ft2VeryStrongFighter = tbl3.CreateNew ("domob", strongRecepieID, ctx.RoConfig(), rnd);
	  ft2VeryStrongFighter.reset(); 
  }
  
  xp->CalculatePrestige(ctx.RoConfig());
  EXPECT_EQ (xp->GetPresitge(), 7886);
  
  xp->MutableProto().set_tournamentscompleted(500);
  xp->MutableProto().set_tournamentswon(500);
  
  xp->CalculatePrestige(ctx.RoConfig());
 
  EXPECT_EQ (xp->GetPresitge(), 8436);
  
  auto res3 = tbl3.QueryForOwner ("domob");
  bool tryAndStep3 = res3.Step();

  while (tryAndStep3)
  {
	auto fghtr = tbl3.GetFromResult (res3, ctx.RoConfig ()); 
	fghtr->MutableProto().set_rating(2000);
	fghtr.reset();
	tryAndStep3 = res3.Step();   
  }  
  
  xp->CalculatePrestige(ctx.RoConfig());
  EXPECT_EQ (xp->GetPresitge(), 10569);
  
  xp->MutableProto().set_tournamentscompleted(1000);
  xp->MutableProto().set_tournamentswon(1000);
  
  xp->CalculatePrestige(ctx.RoConfig());
 
  EXPECT_EQ (xp->GetPresitge(), 11119); 
  
  xp->MutableProto().set_tournamentscompleted(1000);
  xp->MutableProto().set_tournamentswon(500);
  
  xp->CalculatePrestige(ctx.RoConfig());
 
  EXPECT_EQ (xp->GetPresitge(), 10619);   
  
  res3 = tbl3.QueryForOwner ("domob");
  tryAndStep3 = res3.Step();

  while (tryAndStep3)
  {  
    auto fghtr = tbl3.GetFromResult (res3, ctx.RoConfig ()); 
	fghtr->RerollName(5376000000, ctx.RoConfig (), rnd, pxd::Quality::Epic);
	fghtr.reset();
	tryAndStep3 = res3.Step();   
  }
  
  xp->CalculatePrestige(ctx.RoConfig());
  EXPECT_EQ (xp->GetPresitge(), 16219);  
  
  res3 = tbl3.QueryForOwner ("domob");
  tryAndStep3 = res3.Step();

  while (tryAndStep3)
  {    
      auto fghtr = tbl3.GetFromResult (res3, ctx.RoConfig ()); 
	  const auto& fighterMoveBlueprintList = ctx.RoConfig ()->fightermoveblueprints();
	  std::map<pxd::ArmorType, std::string> slotsUsed;

	  for(auto& armorPiece : fghtr->GetProto().armorpieces())
	  {
		slotsUsed.insert(std::pair<pxd::ArmorType, std::string>((pxd::ArmorType)armorPiece.armortype(), armorPiece.candy()));		
	  }

	  std::vector<std::pair<std::string, pxd::proto::FighterMoveBlueprint>> sortedMoveBlueprintTypesmap;
	  for (auto itr = fighterMoveBlueprintList.begin(); itr != fighterMoveBlueprintList.end(); ++itr)
		  sortedMoveBlueprintTypesmap.push_back(*itr);

	  sort(sortedMoveBlueprintTypesmap.begin(), sortedMoveBlueprintTypesmap.end(), [=](std::pair<std::string, pxd::proto::FighterMoveBlueprint>& a, std::pair<std::string, pxd::proto::FighterMoveBlueprint>& b)
	  {
		  return a.first < b.first;
	  } 
	  );  

	  int32_t totalMoveSize = sortedMoveBlueprintTypesmap.size();
	  auto moveBlueprint = sortedMoveBlueprintTypesmap[rnd.NextInt(totalMoveSize)];

	  std::vector<pxd::ArmorType> aType;
      std::vector<pxd::ArmorType> pieceList;

	  switch((pxd::MoveType)moveBlueprint.second.movetype()) 
	  {
		 case pxd::MoveType::Heavy:
			pieceList.push_back(pxd::ArmorType::Head);
			pieceList.push_back(pxd::ArmorType::RightShoulder);
			pieceList.push_back(pxd::ArmorType::LeftShoulder);
			break;
		 case pxd::MoveType::Speedy:
			pieceList.push_back(pxd::ArmorType::UpperRightArm);
			pieceList.push_back(pxd::ArmorType::LowerRightArm);
			pieceList.push_back(pxd::ArmorType::UpperLeftArm);
			pieceList.push_back(pxd::ArmorType::LowerLeftArm);
			break;
		 case pxd::MoveType::Tricky:
			pieceList.push_back(pxd::ArmorType::RightHand);
			pieceList.push_back(pxd::ArmorType::Torso);
			pieceList.push_back(pxd::ArmorType::LeftHand);
			break;
		 case pxd::MoveType::Distance:
			pieceList.push_back(pxd::ArmorType::Waist);
			pieceList.push_back(pxd::ArmorType::UpperRightLeg);
			pieceList.push_back(pxd::ArmorType::UpperLeftLeg);
			break;
		 case pxd::MoveType::Blocking:
			pieceList.push_back(pxd::ArmorType::LowerRightLeg);
			pieceList.push_back(pxd::ArmorType::RightFoot);
			pieceList.push_back(pxd::ArmorType::LeftFoot);
			break;        
	  }	  
	  
	  aType = pieceList;
	  
	  pxd::ArmorType randomElement = aType[rnd.NextInt(aType.size())];
	 
	  if(slotsUsed.find(randomElement) == slotsUsed.end())
	  {
		slotsUsed.insert(std::pair<pxd::ArmorType, std::string>(randomElement, moveBlueprint.second.candytype()));	   
		proto::ArmorPiece* newArmorPiece =  fghtr->MutableProto().add_armorpieces();
		newArmorPiece->set_armortype((uint32_t)randomElement);
		newArmorPiece->set_candy(moveBlueprint.second.candytype());
		newArmorPiece->set_rewardsource(0);
		newArmorPiece->set_rewardsourceid("");
	  }  
	  
	  fghtr.reset();
	  tryAndStep3 = res3.Step();  
  }
 
  xp->CalculatePrestige(ctx.RoConfig());
  EXPECT_EQ (xp->GetPresitge(), 18210);   
  
  xp.reset();
}

TEST_F (ValidateStateTests, TournamentStrongerFighterWins)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  int ftA1idx = ft->GetId();
  ft.reset();
  
  auto r0 = tbl2.CreateNew("domob", "7b7d8898-7f58-0334-0bad-825dc87a6638", ctx.RoConfig());
  uint32_t strongRecepieID = r0->GetId();
  r0.reset();  
  
  auto ft2VeryStrongFighter = tbl3.CreateNew ("domob", strongRecepieID, ctx.RoConfig(), rnd);
  ft2VeryStrongFighter->MutableProto().set_rating(1999);
  int ftA2idx = ft2VeryStrongFighter->GetId();
  ft2VeryStrongFighter.reset();
  
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 4);
  xp->CalculatePrestige(ctx.RoConfig());
  xp.reset();
  
  UpdateState ("[]");
  
  auto tutorialTrmn = tbl5.GetByAuthIdName("cbd2e78a-37ce-b864-793d-8dd27788a774", ctx.RoConfig());
  ASSERT_TRUE (tutorialTrmn != nullptr);
  uint32_t TID = tutorialTrmn->GetId();
  tutorialTrmn.reset();
  
  std::ostringstream s;
  s << TID;
  std::string converted(s.str());  
  
  std::ostringstream s1x;
  s1x << ftA1idx;
  std::string converted1x(s1x.str()); 

  std::ostringstream s2x;
  s2x << ftA2idx;
  std::string converted2x(s2x.str());   
  
  Process (R"([
    {"name": "domob", "move": {"tm": {"e": {"tid": )" + converted + R"(, "fc": [)" + converted1x + R"(,)" + converted2x + R"(]}}}}
  ])");   
  
  auto xp2 = xayaplayers.CreateNew ("andy", ctx.RoConfig(), rnd);
  auto ftA = tbl3.CreateNew ("andy", 1, ctx.RoConfig(), rnd);
  uint32_t ftA1id = ftA->GetId();
  ftA.reset();
  
  auto ftA2 = tbl3.CreateNew ("andy", 1, ctx.RoConfig(), rnd);
  uint32_t ftA2id = ftA2->GetId();
  ftA2.reset();
  
  EXPECT_EQ (xp2->CollectInventoryFighters(ctx.RoConfig()).size(), 4);
  xp2->CalculatePrestige(ctx.RoConfig());
  xp2.reset();
  
  UpdateState ("[]");
  
  EXPECT_EQ (tbl4.CountForOwner("domob"), 0);
  
  std::ostringstream s1;
  s1 << ftA1id;
  std::string converted1(s1.str()); 

  std::ostringstream s2;
  s2 << ftA2id;
  std::string converted2(s2.str()); 
  
  Process (R"([
    {"name": "andy", "move": {"tm": {"e": {"tid": )" + converted + R"(, "fc": [)" + converted1 + R"(,)" + converted2 + R"(]}}}}
  ])");    

  for (unsigned i = 0; i < 3; ++i)
  {
    UpdateState ("[]");
  }
  
  /* Auto-credit at resolve: nothing sits unclaimed; winner domob 10, loser andy 7. */
  EXPECT_EQ (tbl4.CountForOwner("domob"), 0);
  EXPECT_EQ (tbl4.CountForOwner("andy"), 0);
  {
    auto pdom = xayaplayers.GetByName ("domob", ctx.RoConfig());
    EXPECT_EQ (pdom->GetProto().rewards_serial(), 10u);
    auto pand = xayaplayers.GetByName ("andy", ctx.RoConfig());
    EXPECT_EQ (pand->GetProto().rewards_serial(), 7u);
  }

  auto superFighter = tbl3.GetById(ftA2idx, ctx.RoConfig());
  EXPECT_EQ (superFighter->GetProto().sweetness(), 10);
  superFighter.reset();

}


    
}

} // namespace pxd

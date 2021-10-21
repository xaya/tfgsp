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
#include "params.hpp"
#include "testutils.hpp"

#include "database/dbtest.hpp"
#include "database/xayaplayer.hpp"
#include "database/reward.hpp"

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
  
  PXLogicTests () : xayaplayers(db), tbl2(db), tbl3(db), tbl4(db), tbl5(db)
  {
    SetHeight (42);
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

};

namespace
{
    
/* ************************************************************************** */

using ValidateStateTests = PXLogicTests;

TEST_F (ValidateStateTests, AncientAccountFaction)
{
  EXPECT_DEATH (xayaplayers.CreateNew ("domob", ctx.RoConfig())->SetRole (PlayerRole::INVALID), "to NULL role");
}    

TEST_F (ValidateStateTests, RecepieInstanceFullCycleTest)
{
  xayaplayers.CreateNew ("domob", ctx.RoConfig())->AddBalance (100);
   
  auto r0 = tbl2.CreateNew("domob", "5864a19b-c8c0-2d34-eaef-9455af0baf2c", ctx.RoConfig());
  r0.reset();   
   
  Process (R"([
    {"name": "domob", "move": {"ca": {"r": {"rid": 1, "fid": 0}}}}
  ])");  

  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 1);
  a.reset ();

  UpdateState ("[]");
  
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->GetOngoingsSize (), 0);
  EXPECT_EQ (a->CollectInventoryFighters(ctx.RoConfig()).size(), 1);
  
  EXPECT_EQ (a->GetBalance (), 75);
  
  auto r = tbl2.GetById(1); 
  EXPECT_EQ (r->GetProto().name(), "First Recipe");
  EXPECT_EQ (r->GetOwner(), "");
  
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Common_Gumdrop"), 0);
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Common_Icing"), 0);
  
}   

TEST_F (ValidateStateTests, RecepieWithApplicableGoodieTest)
{
  auto a = xayaplayers.CreateNew ("domob", ctx.RoConfig());
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
  EXPECT_EQ (a->CollectInventoryFighters(ctx.RoConfig()).size(), 1);
  
  EXPECT_EQ (a->GetBalance (), 55);
  
  auto r = tbl2.GetById(2); 
  EXPECT_EQ (r->GetProto().name(), "Captain Scott Lemonpie");
  EXPECT_EQ (r->GetOwner(), "");
  
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Goodie_PressureCooker_1"), 0);
}   

TEST_F (ValidateStateTests, RecepieInstanceRevertIfFullRoster)
{
  xayaplayers.CreateNew ("domob", ctx.RoConfig())->AddBalance (100);
   
  proto::ConfigData& cfg = const_cast <proto::ConfigData&>(*ctx.RoConfig());
  cfg.mutable_params()->set_max_fighter_inventory_amount(0); 

  Process (R"([
    {"name": "domob", "move": {"ca": {"r": {"rid": 1, "fid": 0}}}}
  ])");  

  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 1);
  a.reset ();
  
  UpdateState ("[]");
  
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->CollectInventoryFighters(ctx.RoConfig()).size(), 0);
  
  EXPECT_EQ (a->GetBalance (), 100);
  
  auto r0 = tbl2.GetById(1); 
  EXPECT_EQ (r0->GetProto().name(), "First Recipe");
  EXPECT_EQ (r0->GetOwner(), "domob");
  
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Common_Gumdrop"), 1);
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Common_Icing"), 1);
} 

TEST_F (ValidateStateTests, UnitTestExpeditionFailsOnMainNet)
{  
  ctx.SetChain (xaya::Chain::MAIN);
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig());
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  ft.reset();
  xp.reset();
  
  EXPECT_EQ (tbl2.CountForOwner(""), 0);
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "00000000-0000-0000-zzzz-zzzzzzzzzzzz", "fid": 2}}}}
  ])");  
  
  for (unsigned i = 0; i < 1; ++i)
  {
    UpdateState ("[]");
  }  
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  a.reset ();
   
  ft = tbl3.GetById(2, ctx.RoConfig());
  ft.reset();
  
  EXPECT_EQ (tbl2.CountForOwner(""), 0);    
}

TEST_F (ValidateStateTests, GeneratedRecipeMakeSureItWorks)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig());
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  ft.reset();
  xp.reset();
  
  EXPECT_EQ (tbl2.CountForOwner(""), 0);
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "00000000-0000-0000-zzzz-zzzzzzzzzzzz", "fid": 2}}}}
  ])");  
  
  for (unsigned i = 0; i < 1; ++i)
  {
    UpdateState ("[]");
  }  
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  a.reset ();
   
  ft = tbl3.GetById(2, ctx.RoConfig());
  ft.reset();
  
  EXPECT_EQ (tbl2.CountForOwner(""), 2);
  auto res = tbl2.QueryForOwner("");
  ASSERT_TRUE (res.Step ());
  auto r = tbl2.GetFromResult (res);
  
  EXPECT_EQ (r->GetProto().name(), "generated");
} 

TEST_F (ValidateStateTests, RecepieInstanceFailWithMissingIngridients)
{
   xayaplayers.CreateNew ("domob", ctx.RoConfig())->AddBalance (100);
   auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());

   a->GetInventory().SetFungibleCount("Common_Icing", 0);
   a.reset();
   
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

  
  EXPECT_EQ (a->GetBalance (), 100);
  
  auto r = tbl2.GetById(1); 
  EXPECT_EQ (r->GetProto().name(), "First Recipe");
  EXPECT_EQ (r->GetOwner(), "domob");
  
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Common_Gumdrop"), 1);
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Common_Icing"), 0);
  
}  

TEST_F (ValidateStateTests, ExpeditionInstanceSolveTwiceTest)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig());
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Available);
  ft.reset();
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 1);
  xp.reset();
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "93ad71bb-cd8f-dc24-7885-2c3fd0013245", "fid": 2}}}}
  ])");  
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 1);
  a.reset ();
   
  ft = tbl3.GetById(2, ctx.RoConfig());
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Expedition);
  ft.reset();
  
  for (unsigned i = 0; i < 17; ++i)
  {
    UpdateState ("[]");
  }
  
  ft = tbl3.GetById(2, ctx.RoConfig());
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Available);
  ft.reset();  
  
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  a->SetFTUEState(FTUEState::FirstExpedition);
  EXPECT_EQ (a->GetOngoingsSize (), 0);
  a.reset ();
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "c064e7f7-acbf-4f74-fab8-cccd7b2d4004", "fid": 2}}}}
  ])");  

  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->GetOngoingsSize (), 1);  
 
  ft = tbl3.GetById(2, ctx.RoConfig());
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Expedition);
  ft.reset(); 
}

TEST_F (ValidateStateTests, ClaimRewardsAfterExpedition)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig());
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Available);
  ft.reset();
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 1);
  xp.reset();
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "c064e7f7-acbf-4f74-fab8-cccd7b2d4004", "fid": 2}}}}
  ])");  
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 1);
  a.reset ();
   
  ft = tbl3.GetById(2, ctx.RoConfig());
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Expedition);
  ft.reset();
  
  for (unsigned i = 0; i < 1; ++i)
  {
    UpdateState ("[]");
  }
  
  ft = tbl3.GetById(2, ctx.RoConfig());
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Available);
  ft.reset();  
  
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  a->SetFTUEState(FTUEState::FirstExpedition);
  EXPECT_EQ (a->GetOngoingsSize (), 0);
  a.reset ();
  
  EXPECT_EQ (tbl4.CountForOwner("domob"), 4);
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"c": {"eid": "c064e7f7-acbf-4f74-fab8-cccd7b2d4004"}}}}
  ])");  

  EXPECT_EQ (tbl4.CountForOwner("domob"), 0);
}

TEST_F (ValidateStateTests, ClaimRewardsTestAllRewardTypesBeingAwardedProperly)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig());
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  ft.reset();
  xp.reset();
  
  EXPECT_EQ (tbl2.CountForOwner(""), 0);
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "00000000-0000-0000-zzzz-zzzzzzzzzzzz", "fid": 2}}}}
  ])");  
  
  for (unsigned i = 0; i < 1; ++i)
  {
    UpdateState ("[]");
  }  
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  a.reset ();
   
  ft = tbl3.GetById(2, ctx.RoConfig());
  ft.reset();
  
  EXPECT_EQ (tbl2.CountForOwner(""), 2);
  auto res = tbl2.QueryForOwner("");
  ASSERT_TRUE (res.Step ());
  auto r = tbl2.GetFromResult (res);
  
  EXPECT_EQ (r->GetProto().name(), "generated"); 
  r.reset();  

  Process (R"([
    {"name": "domob", "move": {"exp": {"c": {"eid": "00000000-0000-0000-zzzz-zzzzzzzzzzzz"}}}}
  ])");  

  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Common_Candy Button"), 5);
  a.reset();
  
  ft = tbl3.GetById(2, ctx.RoConfig());
  EXPECT_EQ (ft->GetProto().animationid(), "27448f34-ca08-99b4-1b63-fd4b227e6af4");
  EXPECT_EQ (ft->GetProto().armorpieces(2).armortype(), 18);    
}

TEST_F (ValidateStateTests, ClaimRewardsInvalidParams)
{
auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig());
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  ft.reset();
  xp.reset();
  
  EXPECT_EQ (tbl2.CountForOwner(""), 0);
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "00000000-0000-0000-zzzz-zzzzzzzzzzzz", "fid": 2}}}}
  ])");  
  
  for (unsigned i = 0; i < 1; ++i)
  {
    UpdateState ("[]");
  }  
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  a.reset ();
   
  ft = tbl3.GetById(2, ctx.RoConfig());
  ft.reset();
  
  EXPECT_EQ (tbl2.CountForOwner(""), 2);
  auto res = tbl2.QueryForOwner("");
  ASSERT_TRUE (res.Step ());
  auto r = tbl2.GetFromResult (res);
  
  EXPECT_EQ (r->GetProto().name(), "generated"); 
  r.reset();  

  Process (R"([
    {"name": "domob", "move": {"exp": {"c": {"eid": "50379c2b-2422-8104-ea69-bb2882e9cac0"}}}}
  ])");  

  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Common_Candy Button"), 0);
  a.reset();    
}

TEST_F (ValidateStateTests, ClaimRewardsWhenFullSlotsEmptySomeAndFinishClaiming)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig());
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Available);
  ft.reset();
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 1);
  xp.reset();
  
  proto::ConfigData& cfg = const_cast <proto::ConfigData&>(*ctx.RoConfig());
  cfg.mutable_params()->set_max_recipe_inventory_amount(0); 
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "c064e7f7-acbf-4f74-fab8-cccd7b2d4004", "fid": 2}}}}
  ])");  
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 1);
  a.reset ();
   
  ft = tbl3.GetById(2, ctx.RoConfig());
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Expedition);
  ft.reset();
  
  for (unsigned i = 0; i < 1; ++i)
  {
    UpdateState ("[]");
  }
  
  ft = tbl3.GetById(2, ctx.RoConfig());
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Available);
  ft.reset();  
  
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  a->SetFTUEState(FTUEState::FirstExpedition);
  EXPECT_EQ (a->GetOngoingsSize (), 0);
  a.reset ();
  
  EXPECT_EQ (tbl4.CountForOwner("domob"), 4);
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"c": {"eid": "c064e7f7-acbf-4f74-fab8-cccd7b2d4004"}}}}
  ])");  

  EXPECT_EQ (tbl4.CountForOwner("domob"), 1);
  
  cfg.mutable_params()->set_max_recipe_inventory_amount(10);  

  Process (R"([
    {"name": "domob", "move": {"exp": {"c": {"eid": "c064e7f7-acbf-4f74-fab8-cccd7b2d4004"}}}}
  ])");    
  
  EXPECT_EQ (tbl4.CountForOwner("domob"), 0);
}

TEST_F (ValidateStateTests, ExpeditionInstanceBusyFighterNotSending)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig());
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Available);
  ft.reset();
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 1);
  xp.reset();
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "93ad71bb-cd8f-dc24-7885-2c3fd0013245", "fid": 2}}}}
  ])");  
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 1);
  a.reset ();
   
  ft = tbl3.GetById(2, ctx.RoConfig());
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Expedition);
  ft.reset();
  
  for (unsigned i = 0; i < 1; ++i)
  {
    UpdateState ("[]");
  }
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "93ad71bb-cd8f-dc24-7885-2c3fd0013245", "fid": 2}}}}
  ])");  

  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->GetOngoingsSize (), 1);  
}

TEST_F (ValidateStateTests, ExpeditionTestRewards)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig());
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Available);
  ft.reset();
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 1);
  xp.reset();
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "c064e7f7-acbf-4f74-fab8-cccd7b2d4004", "fid": 2}}}}
  ])");  
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 1);
  a.reset ();
   
  ft = tbl3.GetById(2, ctx.RoConfig());
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Expedition);
  ft.reset();
  
  for (unsigned i = 0; i < 1; ++i)
  {
    UpdateState ("[]");
  }
  
  EXPECT_EQ (tbl4.CountForOwner("domob"), 4);

}

TEST_F (ValidateStateTests, ExpeditionWithApplicableGoodieTest)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig());
  xp->GetInventory().SetFungibleCount("Goodie_Espresso_1", 1);

  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Available);
  ft.reset();
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 1);
  xp.reset();
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "93ad71bb-cd8f-dc24-7885-2c3fd0013245", "fid": 2}}}}
  ])");  
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 1);
  a.reset ();
   
  ft = tbl3.GetById(2, ctx.RoConfig());
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Expedition);
  ft.reset();
  
  for (unsigned i = 0; i < 14; ++i)
  {
    UpdateState ("[]");
  }
  
  ft = tbl3.GetById(2, ctx.RoConfig());
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Available);
  ft.reset();  
  
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Goodie_Espresso_1"), 0);

}

TEST_F (ValidateStateTests, ExpeditionWithWrongTyprApplicableGoodieTest)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig());
  xp->GetInventory().SetFungibleCount("Goodie_PressureCooker_1", 1);

  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Available);
  ft.reset();
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 1);
  xp.reset();
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "93ad71bb-cd8f-dc24-7885-2c3fd0013245", "fid": 2}}}}
  ])");  
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 1);
  a.reset ();
   
  ft = tbl3.GetById(2, ctx.RoConfig());
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Expedition);
  ft.reset();
  
  for (unsigned i = 0; i < 14; ++i)
  {
    UpdateState ("[]");
  }
  
  ft = tbl3.GetById(2, ctx.RoConfig());
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Expedition);
  ft.reset();  
  
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Goodie_PressureCooker_1"), 1);

}

TEST_F (ValidateStateTests, ExpeditionInstanceTutorialTwiceFail)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig());
  xp->SetFTUEState(FTUEState::FirstExpedition);
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  ft.reset();
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 1);
  xp.reset();
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "c064e7f7-acbf-4f74-fab8-cccd7b2d4004", "fid": 2}}}}
  ])");  
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 1);
  a.reset ();
   
  UpdateState ("[]");
  
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->GetOngoingsSize (), 0);
  a.reset ();
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "c064e7f7-acbf-4f74-fab8-cccd7b2d4004", "fid": 2}}}}
  ])");  

  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->GetOngoingsSize (), 0);  
}

TEST_F (ValidateStateTests, TournamentInstanceSheduleTest)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig());
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  ft.reset();
  
  auto ft2 = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  ft2.reset();
  
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 2);
  xp.reset();
  
  UpdateState ("[]");
  
  auto tutorialTrmn = tbl5.GetByAuthIdName("cbd2e78a-37ce-b864-793d-8dd27788a774", ctx.RoConfig());
  ASSERT_TRUE (tutorialTrmn != nullptr);
  uint32_t TID = tutorialTrmn->GetId();
  tutorialTrmn.reset();
  
  std::ostringstream s;
  s << TID;
  std::string converted(s.str());  
  
  Process (R"([
    {"name": "domob", "move": {"tm": {"e": {"tid": )" + converted + R"(, "fc": [2,3]}}}}
  ])");   
  
  auto res = tbl5.QueryAll ();
  ASSERT_TRUE (res.Step ());
  
  ft = tbl3.GetById(3, ctx.RoConfig());
  EXPECT_EQ (ft->GetProto().tournamentinstanceid(), TID);  
  ft.reset();
}

TEST_F (ValidateStateTests, TournamentResolvedTest)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig());
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  ft.reset();
  
  auto ft2 = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  ft2.reset();
  
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 2);
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
  
  Process (R"([
    {"name": "domob", "move": {"tm": {"e": {"tid": )" + converted + R"(, "fc": [2,3]}}}}
  ])");   
  

  auto xp2 = xayaplayers.CreateNew ("andy", ctx.RoConfig());
  auto ftA = tbl3.CreateNew ("andy", 1, ctx.RoConfig(), rnd);
  uint32_t ftA1id = ftA->GetId();
  ftA.reset();
  
  auto ftA2 = tbl3.CreateNew ("andy", 1, ctx.RoConfig(), rnd);
  uint32_t ftA2id = ftA2->GetId();
  ftA2.reset();
  
  EXPECT_EQ (xp2->CollectInventoryFighters(ctx.RoConfig()).size(), 2);
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
  
  EXPECT_EQ (tbl4.CountForOwner("domob"), 8);
  EXPECT_EQ (tbl4.CountForOwner("andy"), 11);
  
  Process (R"([
    {"name": "andy", "move": {"tm": {"c": {"tid": )" + converted + R"(}}}}
  ])"); 
  
  UpdateState ("[]");
  
  EXPECT_EQ (tbl4.CountForOwner("domob"), 8);
  EXPECT_EQ (tbl4.CountForOwner("andy"), 0);  
}

TEST_F (ValidateStateTests, TournamentStrongerFighterWins)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig());
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  ft.reset();
  
  auto r0 = tbl2.CreateNew("domob", "7b7d8898-7f58-0334-0bad-825dc87a6638", ctx.RoConfig());
  uint32_t strongRecepieID = r0->GetId();
  r0.reset();  
  
  auto ft2VeryStrongFighter = tbl3.CreateNew ("domob", strongRecepieID, ctx.RoConfig(), rnd);
  ft2VeryStrongFighter.reset();
  
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 2);
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
  
  Process (R"([
    {"name": "domob", "move": {"tm": {"e": {"tid": )" + converted + R"(, "fc": [2,4]}}}}
  ])");   
  

  auto xp2 = xayaplayers.CreateNew ("andy", ctx.RoConfig());
  auto ftA = tbl3.CreateNew ("andy", 1, ctx.RoConfig(), rnd);
  uint32_t ftA1id = ftA->GetId();
  ftA.reset();
  
  auto ftA2 = tbl3.CreateNew ("andy", 1, ctx.RoConfig(), rnd);
  uint32_t ftA2id = ftA2->GetId();
  ftA2.reset();
  
  EXPECT_EQ (xp2->CollectInventoryFighters(ctx.RoConfig()).size(), 2);
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
  
  EXPECT_EQ (tbl4.CountForOwner("domob"), 11);
  EXPECT_EQ (tbl4.CountForOwner("andy"), 8);
}
    
}

} // namespace pxd

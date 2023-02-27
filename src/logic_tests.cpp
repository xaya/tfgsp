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
#include "database/specialtournament.hpp"

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
  SpecialTournamentTable tbl6;
  
  /** GameStateJson instance used in testing.  */
  GameStateJson converter;  
  
  PXLogicTests () : xayaplayers(db), tbl2(db), tbl3(db), tbl4(db), tbl5(db), tbl6(db), converter(db, ctx)
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
  
  EXPECT_EQ (a->GetBalance (), 85 + cfg.params().starting_crystals());
  
  auto r = tbl2.GetById(1); 
  EXPECT_EQ (r->GetProto().name(), "First Recipe");
  EXPECT_EQ (r->GetOwner(), "");
  
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Common_Gumdrop"), 0);
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Common_Icing"), 0);
  
}   

TEST_F (ValidateStateTests, RecepieInstanceGeneratedFullCycleTest)
{
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->AddBalance (100);
  
  auto rcpID = pxd::RecipeInstance::Generate(pxd::Quality::Common, ctx.RoConfig(), rnd, db, "");
  auto r = tbl2.GetById(rcpID);
  
  
  EXPECT_EQ (r->GetProto().duration(), 15); //todo, read 15 just from config of common file

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
  
  for (unsigned i = 0; i < 15; ++i) //todo same as 15 up above
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
}   

TEST_F (ValidateStateTests, DefaultSpecialTournamentsArePlottedTest)
{
    UpdateState ("[]");
    
    auto xp1 = xayaplayers.GetByName("xayatf1", ctx.RoConfig());
    auto xp2 = xayaplayers.GetByName("xayatf2", ctx.RoConfig());
    auto xp3 = xayaplayers.GetByName("xayatf3", ctx.RoConfig());
    auto xp4 = xayaplayers.GetByName("xayatf4", ctx.RoConfig());
    auto xp5 = xayaplayers.GetByName("xayatf5", ctx.RoConfig());
    auto xp6 = xayaplayers.GetByName("xayatf6", ctx.RoConfig());
    auto xp7 = xayaplayers.GetByName("xayatf7", ctx.RoConfig());
    
    EXPECT_EQ (xp1->CollectInventoryFightersFromSpecialTournament(ctx.RoConfig(), 1).size(), 6);
    EXPECT_EQ (xp2->CollectInventoryFightersFromSpecialTournament(ctx.RoConfig(), 2).size(), 6);
    EXPECT_EQ (xp3->CollectInventoryFightersFromSpecialTournament(ctx.RoConfig(), 3).size(), 6);
    EXPECT_EQ (xp4->CollectInventoryFightersFromSpecialTournament(ctx.RoConfig(), 4).size(), 6);
    EXPECT_EQ (xp5->CollectInventoryFightersFromSpecialTournament(ctx.RoConfig(), 5).size(), 6);
    EXPECT_EQ (xp6->CollectInventoryFightersFromSpecialTournament(ctx.RoConfig(), 6).size(), 6);
    EXPECT_EQ (xp7->CollectInventoryFightersFromSpecialTournament(ctx.RoConfig(), 7).size(), 6);
    
    EXPECT_EQ (xp7->CollectInventoryFightersFromSpecialTournament(ctx.RoConfig(), 1).size(), 0);
}

TEST_F (ValidateStateTests, EnterLeaveSpecialCompetitionTest)
{
  auto a = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  a->AddBalance (100); 
  a.reset();

  UpdateState ("[]");
  UpdateState ("[]");

  a = xayaplayers.GetByName("domob", ctx.RoConfig());
  EXPECT_EQ (a->CollectInventoryFightersFromSpecialTournament(ctx.RoConfig(), 1).size(), 0);
  a.reset();

  auto ft1 = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  int32_t ft1ID = ft1->GetId();
  EXPECT_EQ (ft1->GetStatus(), FighterStatus::Available);
  ft1.reset();   
  
  auto ft2 = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  int32_t ft2ID = ft2->GetId();
  EXPECT_EQ (ft2->GetStatus(), FighterStatus::Available);
  ft2.reset(); 

  auto ft3 = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  int32_t ft3ID = ft3->GetId();
  EXPECT_EQ (ft3->GetStatus(), FighterStatus::Available);
  ft3.reset(); 

  auto ft4 = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  int32_t ft4ID = ft4->GetId();
  EXPECT_EQ (ft4->GetStatus(), FighterStatus::Available);
  ft4.reset(); 

  auto ft5 = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  int32_t ft5ID = ft5->GetId();
  EXPECT_EQ (ft5->GetStatus(), FighterStatus::Available);
  ft5.reset(); 

  auto ft6 = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  int32_t ft6ID = ft6->GetId();
  EXPECT_EQ (ft6->GetStatus(), FighterStatus::Available);
  ft6.reset();   
  
  auto specialTournamentTier1 = tbl6.GetByTier(1, ctx.RoConfig());
  ASSERT_TRUE (specialTournamentTier1 != nullptr);
  uint32_t TID = specialTournamentTier1->GetId();
  specialTournamentTier1.reset();    
  
  std::ostringstream s;
  s << TID;
  std::string converted(s.str());  
  
  std::ostringstream s1x;
  s1x << ft1ID;
  std::string converted1x(s1x.str()); 

  std::ostringstream s2x;
  s2x << ft2ID;
  std::string converted2x(s2x.str());  
  
  std::ostringstream s3x;
  s3x << ft3ID;
  std::string converted3x(s3x.str()); 

  std::ostringstream s4x;
  s4x << ft4ID;
  std::string converted4x(s4x.str()); 

  std::ostringstream s5x;
  s5x << ft5ID;
  std::string converted5x(s5x.str()); 

  std::ostringstream s6x;
  s6x << ft6ID;
  std::string converted6x(s6x.str());   
  
  Process (R"([
    {"name": "domob", "move": {"tms": {"e": {"tid": )" + converted + R"(, "fc": [)"+converted1x+R"(,)"+converted2x + R"(,)" + converted3x + R"(,)" + converted4x + R"(,)" + converted5x + R"(,)" + converted6x + R"(]}}}}
  ])");    

  UpdateState ("[]");

  a = xayaplayers.GetByName("domob", ctx.RoConfig());
  EXPECT_EQ (a->CollectInventoryFightersFromSpecialTournament(ctx.RoConfig(), 1).size(), 6);
  a.reset();
  
  Process (R"([
    {"name": "domob", "move": {"tms": {"l": {"tid": )" + converted + R"(}}}}
  ])");   

  UpdateState ("[]");
   
  a = xayaplayers.GetByName("domob", ctx.RoConfig());
  EXPECT_EQ (a->CollectInventoryFightersFromSpecialTournament(ctx.RoConfig(), 1).size(), 0);
  a.reset();   
}

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
  
  auto r0 = tbl2.GetById(1); 
  EXPECT_EQ (r0->GetProto().name(), "First Recipe");
  EXPECT_EQ (r0->GetOwner(), "domob");
  
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Common_Gumdrop"), 1);
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Common_Icing"), 1);
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

TEST_F (ValidateStateTests, TestSpecialTournamentPrebuild)
{  
  ctx.SetChain (xaya::Chain::MAIN);
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  ft.reset();
  xp.reset();
  
  EXPECT_EQ (tbl2.CountForOwner(""), 2);
  
  for (unsigned i = 0; i < 1; ++i)
  {
    UpdateState ("[]");
  }  
  
  EXPECT_EQ (tbl2.CountForAll(), 66);
  EXPECT_NE (tbl2.CountForOwner("xayatf1"), 6); 
  EXPECT_NE (tbl2.CountForOwner("xayatf2"), 6);
  EXPECT_NE (tbl2.CountForOwner("xayatf3"), 6);
  EXPECT_NE (tbl2.CountForOwner("xayatf4"), 6);
  EXPECT_NE (tbl2.CountForOwner("xayatf5"), 6);
  EXPECT_NE (tbl2.CountForOwner("xayatf6"), 6);  
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
  
  EXPECT_EQ (tbl2.CountForOwner(""), 17);
  auto res = tbl2.QueryForOwner("");
  ASSERT_TRUE (res.Step ());
  ASSERT_TRUE (res.Step ());
  ASSERT_TRUE (res.Step ());
  auto r = tbl2.GetFromResult (res);
  
  EXPECT_EQ (r->GetProto().authoredid(), "generated");
  
  EXPECT_EQ (r->GetProto().moves_size(), 3);
  EXPECT_EQ (r->GetProto().requiredcandy_size(), 2);
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

  for (unsigned i = 0; i < 22; ++i)
  {
    UpdateState ("[]");
  }
  
  pl = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (pl != nullptr);
  EXPECT_EQ (pl->GetOngoingsSize (), 0);
  pl.reset();  
    
  EXPECT_EQ (tbl4.CountForOwner("domob"), 1);

  ft = tbl3.GetById(4, ctx.RoConfig());
  ASSERT_TRUE (ft != nullptr); 
  EXPECT_EQ(ft->GetProto().moves_size(), 2);
  ft.reset();

  Process (R"([
    {"name": "domob", "move": {"ca": {"sc": {"fid": 4}}}}
  ])"); 

  EXPECT_EQ (tbl4.CountForOwner("domob"), 0);  
  
  ft = tbl3.GetById(4, ctx.RoConfig());
  ASSERT_TRUE (ft != nullptr); 
  EXPECT_EQ(ft->GetProto().moves_size(), 3);
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
  a.reset ();
  
  EXPECT_EQ (tbl4.CountForOwner("domob"), 3);
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"c": {"eid": "c064e7f7-acbf-4f74-fab8-cccd7b2d4004"}}}}
  ])");  

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
  
  for (unsigned i = 0; i < 1; ++i)
  {
    UpdateState ("[]");
  }  
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  a.reset ();
   
  ft = tbl3.GetById(4, ctx.RoConfig());
  ft.reset();
  
  EXPECT_EQ (tbl2.CountForOwner(""), 17);
  auto res = tbl2.QueryForOwner("");
  ASSERT_TRUE (res.Step ());
  ASSERT_TRUE (res.Step ());
  ASSERT_TRUE (res.Step ());
  auto r = tbl2.GetFromResult (res);
  
  EXPECT_EQ (r->GetProto().authoredid(), "generated"); 
  r.reset();  

  Process (R"([
    {"name": "domob", "move": {"exp": {"c": {"eid": "00000000-0000-0000-zzzz-zzzzzzzzzzzz"}}}}
  ])");  

  ft = tbl3.GetById(4, ctx.RoConfig());
  EXPECT_EQ (ft->GetProto().animationid(), "05633498-ace9-de14-c939-9435a6343d0f");  
}

TEST_F (ValidateStateTests, ClaimRewardsInvalidParams)
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
   
  ft = tbl3.GetById(4, ctx.RoConfig());
  ft.reset();
  
  EXPECT_EQ (tbl2.CountForOwner(""), 17);
  auto res = tbl2.QueryForOwner("");
  ASSERT_TRUE (res.Step ());
  ASSERT_TRUE (res.Step ());
  ASSERT_TRUE (res.Step ());
  auto r = tbl2.GetFromResult (res);
  
  EXPECT_EQ (r->GetProto().authoredid(), "generated"); 
  r.reset();  

  Process (R"([
    {"name": "domob", "move": {"exp": {"c": {"eid": "50379c2b-2422-8104-ea69-bb2882e9cac0"}}}}
  ])");  

  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Common_Candy Button"), 0);
  a.reset();    
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

  EXPECT_EQ (tbl4.CountForOwner("domob"), 0);
  
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

  EXPECT_EQ (tbl4.CountForOwner("domob"), 1);   

  Process (R"([
    {"name": "domob", "move": {"f": {"c": {"fid": )"+converted+R"(}}}}
  ])");    
  
  UpdateState ("[]");
  
  EXPECT_EQ (tbl4.CountForOwner("domob"), 0);
}

TEST_F (ValidateStateTests, ClaimRewardsWhenFullSlotsEmptySomeAndFinishClaiming)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Available);
  ft.reset();
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 3);
  xp.reset();
  
  proto::ConfigData& cfg = const_cast <proto::ConfigData&>(*ctx.RoConfig());
  cfg.mutable_params()->set_max_recipe_inventory_amount(0); 
  
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
  a.reset ();
  
  EXPECT_EQ (tbl4.CountForOwner("domob"), 3);
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"c": {"eid": "c064e7f7-acbf-4f74-fab8-cccd7b2d4004"}}}}
  ])");  

  EXPECT_EQ (tbl4.CountForOwner("domob"), 0);
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
  
  EXPECT_EQ (tbl4.CountForOwner("domob"), 3);

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

TEST_F (ValidateStateTests, TournamentResolvedTest)
{
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
  
  EXPECT_EQ (tbl4.CountForOwner("domob"), 7);
  EXPECT_EQ (tbl4.CountForOwner("andy"), 10);
  
  Process (R"([
    {"name": "andy", "move": {"tm": {"c": {"tid": )" + converted + R"(}}}}
  ])"); 
  
  UpdateState ("[]");
  
  EXPECT_EQ (tbl4.CountForOwner("domob"), 7);
  EXPECT_EQ (tbl4.CountForOwner("andy"), 0);  
  
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
    
    Process (R"([
      {"name": "andy", "move": {"tm": {"c": {"tid": )" + converted + R"(}}}}
    ])"); 
    
    Process (R"([
      {"name": "domob", "move": {"tm": {"c": {"tid": )" + converted + R"(}}}}
    ])");     
    
    tbl5.DeleteById(TID);
    
    UpdateState ("[]");         
  }
  
  ftA = tbl3.GetById(ftA1id, ctx.RoConfig());
  EXPECT_EQ (ftA->GetProto().rating(), 1237);
  EXPECT_EQ (ftA->GetProto().sweetness(), (int)pxd::Sweetness::Semi_Sweet);
  ftA.reset();  
}

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
  
  EXPECT_EQ (tbl4.CountForOwner("domob"), 10);
  EXPECT_EQ (tbl4.CountForOwner("andy"), 7);

  tutorialTrmn = tbl5.GetById(TID, ctx.RoConfig());
  
  for(const auto& result: tutorialTrmn->GetInstance().results())
  {
     EXPECT_LT(result.ratingdelta(), 100000);
     EXPECT_GE(result.ratingdelta(), -100000);
  }   
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
  
  EXPECT_EQ (tbl4.CountForOwner("domob"), 10);
  EXPECT_EQ (tbl4.CountForOwner("andy"), 7);

  auto superFighter = tbl3.GetById(ftA2idx, ctx.RoConfig());
  EXPECT_EQ (superFighter->GetProto().sweetness(), 10);
  superFighter.reset();

}


    
}

} // namespace pxd

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
  
  GlobalData gd;
  
  
  PXLogicTests () : xayaplayers(db), tbl2(db), tbl3(db), tbl4(db), tbl5(db), tbl6(db), converter(db, ctx), gd(db)
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
  
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true)->AddBalance (100);
   
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
	
	xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true)->AddBalance (100);	
	for (unsigned i = 0; i < 1000; ++i)
    {
		auto rcpID = pxd::RecipeInstance::Generate(pxd::Quality::Rare, ctx.RoConfig(), rnd, db, "", true);
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
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true)->AddBalance (100);
  
  auto rcpID = pxd::RecipeInstance::Generate(pxd::Quality::Common, ctx.RoConfig(), rnd, db, "", true);
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

/*
TEST_F (ValidateStateTests, RecepieInstanceGeneratedDifferentNamesTest)
{
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->AddBalance (100);
  
  auto rcpID = pxd::RecipeInstance::Generate(pxd::Quality::Common, ctx.RoConfig(), rnd, db, "", true);
  auto r = tbl2.GetById(rcpID);
  
  auto rcpID2 = pxd::RecipeInstance::Generate(pxd::Quality::Common, ctx.RoConfig(), rnd, db, "", true);
  auto r2 = tbl2.GetById(rcpID2);
  
  EXPECT_NE(r->GetProto().name(), r2->GetProto().name());
  r.reset();
  r2.reset();
}*/ 

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
  auto a = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true);
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
  
  auto a = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true);
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
  
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true)->AddBalance (100);
   
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
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true);
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
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true);
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
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true);
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
  EXPECT_EQ (r->GetProto().requiredcandy_size(), 3);
} 

TEST_F (ValidateStateTests, RecepieInstanceFailWithMissingIngridients)
{
   proto::ConfigData& cfg = const_cast <proto::ConfigData&>(*ctx.RoConfig());
   
   xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true)->AddBalance (100);
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
  auto pl = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true);
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
  
  
  EXPECT_EQ (tbl4.CountForOwner("domob"), 2097);
  


}

TEST_F (ValidateStateTests, SweetenerCookAndProperRewardsClaimed)
{
  auto pl = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true);
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
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true);
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
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true);
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
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true);
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
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true);
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
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true);
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
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true);
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
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true);
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
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true);
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
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true);
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
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true);
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
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true);
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
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true);
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  int ftA1id = ft->GetId();
  ft.reset();
  
  auto ft2 = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  int ftA2id = ft2->GetId();
  ft2.reset();
  
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 4);
  xp.reset();
  
  
  auto xp__A = xayaplayers.CreateNew ("domob__A", ctx.RoConfig(), rnd, true);
  auto ft__A = tbl3.CreateNew ("domob__A", 1, ctx.RoConfig(), rnd);
  int ftA1id__A = ft__A->GetId();
  ft__A.reset();
  
  auto ft2__A = tbl3.CreateNew ("domob__A", 1, ctx.RoConfig(), rnd);
  int ftA2id__A = ft2__A->GetId();
  ft2__A.reset();
  
  EXPECT_EQ (xp__A->CollectInventoryFighters(ctx.RoConfig()).size(), 4);
  xp__A.reset();


  auto xp__B = xayaplayers.CreateNew ("domob__B", ctx.RoConfig(), rnd, true);
  auto ft__B = tbl3.CreateNew ("domob__B", 1, ctx.RoConfig(), rnd);
  int ftA1id__B = ft__B->GetId();
  ft__B.reset();
  
  auto ft2__B = tbl3.CreateNew ("domob__B", 1, ctx.RoConfig(), rnd);
  int ftA2id__B = ft2__B->GetId();
  ft2__B.reset();
  
  EXPECT_EQ (xp__B->CollectInventoryFighters(ctx.RoConfig()).size(), 4);
  xp__B.reset();  
  
  auto xp__C = xayaplayers.CreateNew ("domob__C", ctx.RoConfig(), rnd, true);
  auto ft__C = tbl3.CreateNew ("domob__C", 1, ctx.RoConfig(), rnd);
  int ftA1id__C = ft__C->GetId();
  ft__C.reset();
  
  auto ft2__C = tbl3.CreateNew ("domob__C", 1, ctx.RoConfig(), rnd);
  int ftA2id__C = ft2__C->GetId();
  ft2__C.reset();
  
  EXPECT_EQ (xp__C->CollectInventoryFighters(ctx.RoConfig()).size(), 4);
  xp__C.reset();

  auto xp__D = xayaplayers.CreateNew ("domob__D", ctx.RoConfig(), rnd, true);
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
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true);
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
  

  auto xp2 = xayaplayers.CreateNew ("andy", ctx.RoConfig(), rnd, true);
  auto ftA = tbl3.CreateNew ("andy", 1, ctx.RoConfig(), rnd);
  uint32_t ftA1id = ftA->GetId();
  ftA.reset();
  
  auto ftA2 = tbl3.CreateNew ("andy", 1, ctx.RoConfig(), rnd);
  uint32_t ftA2id = ftA2->GetId();
  ftA2.reset();
  
  EXPECT_EQ (xp2->CollectInventoryFighters(ctx.RoConfig()).size(), 4);
  xp2->CalculatePrestige(ctx.RoConfig(), true);
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
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true);
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
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true);
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
  
  auto xp2 = xayaplayers.CreateNew ("andy", ctx.RoConfig(), rnd, true);
  auto ftA = tbl3.CreateNew ("andy", 1, ctx.RoConfig(), rnd);
  uint32_t ftA1id = ftA->GetId();
  EXPECT_EQ (ftA->GetProto().rating(), 1000);
  EXPECT_EQ (ftA->GetProto().sweetness(), (int)pxd::Sweetness::Not_Too_Sweet);
  ftA.reset();
  
  auto ftA2 = tbl3.CreateNew ("andy", 1, ctx.RoConfig(), rnd);
  uint32_t ftA2id = ftA2->GetId();
  ftA2.reset();
  
  EXPECT_EQ (xp2->CollectInventoryFighters(ctx.RoConfig()).size(), 4);
  xp2->CalculatePrestige(ctx.RoConfig(), true);
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
  EXPECT_EQ (ftA->GetProto().rating(), 1172);
  EXPECT_EQ (ftA->GetProto().sweetness(), (int)pxd::Sweetness::Bittersweet);
  
  ftA->MutableProto().set_rating(2000);
  ftA->UpdateSweetness(true);
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
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true);
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
  xp->CalculatePrestige(ctx.RoConfig(), true);
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
  
  auto xp2 = xayaplayers.CreateNew ("andy", ctx.RoConfig(), rnd, true);
  auto ftA = tbl3.CreateNew ("andy", 1, ctx.RoConfig(), rnd);
  uint32_t ftA1id = ftA->GetId();
  ftA.reset();
  
  auto ftA2 = tbl3.CreateNew ("andy", 1, ctx.RoConfig(), rnd);
  uint32_t ftA2id = ftA2->GetId();
  ftA2.reset();
  
  EXPECT_EQ (xp2->CollectInventoryFighters(ctx.RoConfig()).size(), 4);
  xp2->CalculatePrestige(ctx.RoConfig(), true);
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

TEST_F (ValidateStateTests, PrestigeValueTest)
{
  // Firstly we need to make sure, that maximum prestige values can
  // be larger then 14000, which is minimum for tier 7 grade
  
  // For this, lets 1) award player all epic treats
  //                2) set large wins count
  //                3) fighters set maximum rating
  //                4) roll for rare names
  
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true);

  for (unsigned i = 0; i < 48; ++i)
  {
	  auto r0 = tbl2.CreateNew("domob", "c481aeee-d1a1-01c4-7aca-92d0edcddf18", ctx.RoConfig());
	  uint32_t strongRecepieID = r0->GetId();
	  r0.reset();  
	  
	  auto ft2VeryStrongFighter = tbl3.CreateNew ("domob", strongRecepieID, ctx.RoConfig(), rnd);
	  ft2VeryStrongFighter.reset(); 
  }
  
  xp->CalculatePrestige(ctx.RoConfig(), true);
  EXPECT_EQ (xp->GetPresitge(), 7886);
  
  xp->MutableProto().set_tournamentscompleted(500);
  xp->MutableProto().set_tournamentswon(500);
  
  xp->CalculatePrestige(ctx.RoConfig(), true);
 
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
  
  xp->CalculatePrestige(ctx.RoConfig(), true);
  EXPECT_EQ (xp->GetPresitge(), 10569);
  
  xp->MutableProto().set_tournamentscompleted(1000);
  xp->MutableProto().set_tournamentswon(1000);
  
  xp->CalculatePrestige(ctx.RoConfig(), true);
 
  EXPECT_EQ (xp->GetPresitge(), 11119); 
  
  xp->MutableProto().set_tournamentscompleted(1000);
  xp->MutableProto().set_tournamentswon(500);
  
  xp->CalculatePrestige(ctx.RoConfig(), true);
 
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
  
  xp->CalculatePrestige(ctx.RoConfig(), true);
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
 
  xp->CalculatePrestige(ctx.RoConfig(), true);
  EXPECT_EQ (xp->GetPresitge(), 18210);   
  
  xp.reset();
}

TEST_F (ValidateStateTests, TournamentStrongerFighterWins)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd, true);
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
  xp->CalculatePrestige(ctx.RoConfig(), true);
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
  
  auto xp2 = xayaplayers.CreateNew ("andy", ctx.RoConfig(), rnd, true);
  auto ftA = tbl3.CreateNew ("andy", 1, ctx.RoConfig(), rnd);
  uint32_t ftA1id = ftA->GetId();
  ftA.reset();
  
  auto ftA2 = tbl3.CreateNew ("andy", 1, ctx.RoConfig(), rnd);
  uint32_t ftA2id = ftA2->GetId();
  ftA2.reset();
  
  EXPECT_EQ (xp2->CollectInventoryFighters(ctx.RoConfig()).size(), 4);
  xp2->CalculatePrestige(ctx.RoConfig(), true);
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

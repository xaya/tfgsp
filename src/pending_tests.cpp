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

#include "pending.hpp"

#include "logic.hpp"
#include "jsonutils.hpp"
#include "testutils.hpp"
#include "forks.hpp"

#include "database/dbtest.hpp"
#include "database/recipe.hpp"
#include "database/specialtournament.hpp"

#include <xayautil/jsonutils.hpp>

#include <gflags/gflags.h>
#include <gtest/gtest.h>

#include <json/json.h>

#include <string>

namespace pxd
{
namespace
{

/* ************************************************************************** */

class PendingStateTests : public DBTestWithSchema
{

protected:

  PendingState state;
  TestRandom rnd;

  ContextForTesting ctx;

  XayaPlayersTable xayaplayers;
  RecipeInstanceTable tbl2;
  FighterTable tbl3;
  TournamentTable tbl5;
  SpecialTournamentTable tbl6;

  PendingStateTests () : xayaplayers(db), tbl2(db), tbl3(db), tbl5(db), tbl6(db)
  {}
  
  /**
   * Expects that the current state matches the given one, after parsing
   * the expected state's string as JSON.  The comparison is done in the
   * "partial" sense.
   */
  void
  ExpectStateJson (const std::string& expectedStr)
  {
    const Json::Value actual = state.ToJson ();
    
    //LOG (WARNING) << "Actual JSON for the pending state:\n" << actual;//uncomment when adding new stuff to better feedback
    
    ASSERT_TRUE (PartialJsonEqual (actual, ParseJson (expectedStr)));
  }

};

TEST_F (PendingStateTests, Empty)
{
  ExpectStateJson (R"(
    {
      "xayaplayers": []
    }
  )");
}

TEST_F (PendingStateTests, Clear)
{
  auto a = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  a->SetRole (PlayerRole::PLAYER);
  CoinTransferBurn coinOp;
  coinOp.minted = 5;
  coinOp.burnt = 10;
  coinOp.transfers["andy"] = 20;
  state.AddCoinTransferBurn (*a, coinOp);
  a.reset ();

  ExpectStateJson (R"(
    {
      "xayaplayers": [{}]
    }
  )");

  state.Clear ();
  ExpectStateJson (R"(
    {
      "xayaplayers": []
    }
  )");
}

}



/* ************************************************************************** */

class PendingStateUpdaterTests : public PendingStateTests
{

protected:

  TestRandom rnd;
  ContextForTesting ctx;

  PendingStateUpdaterTests ()
  {}

  /**
   * Processes a move for the given name and with the given move data, parsed
   * from JSON string.  If paidToDev is non-zero, then add an "out" entry
   * paying the given amount to the dev address.  If burntChi is non-zero,
   * then also a CHI burn is added to the JSON data.
   */
  void
  ProcessWithDevPaymentAndBurn (const std::string& name,
                                const Amount paidToDev, const Amount burntChi,
                                const std::string& mvStr)
  {
    Json::Value moveObj(Json::objectValue);
    moveObj["name"] = name;
    moveObj["move"] = ParseJson (mvStr);

    if (paidToDev != 0)
    {   
      moveObj["out"]["CPxvCsP9wr8ow4x5r6D1gYpxAFBg6ACzc6"] = xaya::ChiAmountToJson ((paidToDev / 28) * 1);
      moveObj["out"]["CHPVEUVFKy1YugLhVFQmqE8iaPch3MxGsd"] = xaya::ChiAmountToJson ((paidToDev / 28) * 2);
      moveObj["out"]["Cdwan1eAmsvA2sE6XNUB4ZWNDMHwoyhRYr"] = xaya::ChiAmountToJson ((paidToDev / 28) * 3);
      moveObj["out"]["CcX1ksjf4c9qJ2ftd51T2iJbNkRm5SRc94"] = xaya::ChiAmountToJson ((paidToDev / 28) * 4);
      moveObj["out"]["CGr5MT1C5PXUpYhaDQkKoLxP11qJtJxzu8"] = xaya::ChiAmountToJson ((paidToDev / 28) * 5);
      moveObj["out"]["CeJt7YpW8P9jMeVrVm58nUaoM4fJ4KXMUS"] = xaya::ChiAmountToJson ((paidToDev / 28) * 6);
      moveObj["out"]["CZhfYfqbMdzeS5ADRR2su12cWD3TQaeBFc"] = xaya::ChiAmountToJson ((paidToDev / 28) * 7);    
    }    
          
    if (burntChi != 0)
      moveObj["burnt"] = xaya::ChiAmountToJson (burntChi);

    PendingStateUpdater updater(db, state, ctx);
    updater.ProcessMove (moveObj);
  }
  
  /**
   * Processes a move for the given name and with the given move data, parsed
   * from JSON string.  If paidToDev is non-zero, then add an "out" entry
   * paying the given amount to the dev address.  If burntChi is non-zero,
   * then also a CHI burn is added to the JSON data.
   */
  void
  ProcessWithDevPaymentAndBurnAsBatch (const std::string& name,
                                const Amount paidToDev, const Amount burntChi,
                                const std::string& mvStr)
  {
    Json::Value moveObj(Json::objectValue);
    moveObj["name"] = name;
    moveObj["move"] = ParseJson ("["+mvStr+"]");

    if (paidToDev != 0)
    {   
      moveObj["out"]["CPxvCsP9wr8ow4x5r6D1gYpxAFBg6ACzc6"] = xaya::ChiAmountToJson ((paidToDev / 28) * 1);
      moveObj["out"]["CHPVEUVFKy1YugLhVFQmqE8iaPch3MxGsd"] = xaya::ChiAmountToJson ((paidToDev / 28) * 2);
      moveObj["out"]["Cdwan1eAmsvA2sE6XNUB4ZWNDMHwoyhRYr"] = xaya::ChiAmountToJson ((paidToDev / 28) * 3);
      moveObj["out"]["CcX1ksjf4c9qJ2ftd51T2iJbNkRm5SRc94"] = xaya::ChiAmountToJson ((paidToDev / 28) * 4);
      moveObj["out"]["CGr5MT1C5PXUpYhaDQkKoLxP11qJtJxzu8"] = xaya::ChiAmountToJson ((paidToDev / 28) * 5);
      moveObj["out"]["CeJt7YpW8P9jMeVrVm58nUaoM4fJ4KXMUS"] = xaya::ChiAmountToJson ((paidToDev / 28) * 6);
      moveObj["out"]["CZhfYfqbMdzeS5ADRR2su12cWD3TQaeBFc"] = xaya::ChiAmountToJson ((paidToDev / 28) * 7);    
    }    
          
    if (burntChi != 0)
      moveObj["burnt"] = xaya::ChiAmountToJson (burntChi);

    PendingStateUpdater updater(db, state, ctx);
    updater.ProcessMove (moveObj);
  }  

  /**
   * Processes a move for the given name, data and dev payment, without burn.
   */
  void
  ProcessWithDevPayment (const std::string& name, const Amount paidToDev,
                         const std::string& mvStr)
  {
    ProcessWithDevPaymentAndBurn (name, paidToDev, 0, mvStr);
  }

  /**
   * Processes a move for the given name, data and burn.
   */
  void
  ProcessWithBurn (const std::string& name, const Amount burntChi,
                   const std::string& mvStr)
  {
    ProcessWithDevPaymentAndBurn (name, 0, burntChi, mvStr);
  }

  /**
   * Processes a move for the given name and with the given move data
   * as JSON string, without developer payment or burn.
   */
  void
  Process (const std::string& name, const std::string& mvStr)
  {
      
    LOG (WARNING) << "Processing: " << mvStr;
    
    ProcessWithDevPaymentAndBurn (name, 0, 0, mvStr);
  }
};

TEST_F (PendingStateUpdaterTests, UninitialisedAndNonExistantAccount)
{
  /* This verifies that the pending processor is fine with (i.e. does not
     crash or something like that) non-existant and uninitialised xayaplayers.
     The pending moves (even though they might be valid in some of these cases)
     are not tracked.  */

  Process ("domob", R"({
    "vc": {"x": 5}
  })");
  Process ("domob", R"({
    "a": {"init": {"role": "p"}}
  })");

  ASSERT_EQ (xayaplayers.GetByName ("domob",  ctx.RoConfig()), nullptr);
}

TEST_F (PendingStateUpdaterTests, Minting)
{
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);

  ProcessWithBurn ("domob", COIN, R"({
    "vc": {"m": {}}
  })");
  ProcessWithBurn ("domob", 2 * COIN, R"({
    "vc": {"m": {}}
  })");

  ExpectStateJson (R"(
    {
      "xayaplayers":
        [
          {
            "name": "domob",
            "coinops":
              {
                "minted": 30000
              }
          }
        ]
    }
  )");
}

TEST_F (PendingStateUpdaterTests, PurchaseCrystals)
{
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->AddBalance (100);
  
  ProcessWithDevPayment ("domob", 1 * COIN, R"({
    "pc": "T1"
  })");  
  
  ExpectStateJson (R"(
    {
      "xayaplayers":
        [
          {
            "name": "domob",
            "crystalbundles": ["T1"],
            "balance": 200
          }
        ]
    }
  )");
}

TEST_F (PendingStateUpdaterTests, CrystalBalancePendingTestings)
{
  auto a =  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  a->AddBalance (100);
  a.reset();
  
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  ft.reset();
  
  Process ("domob", R"({"ps":  "36adece2-8ed9-3114-db6e-24fa8c494fa5"})");  
  
  ExpectStateJson (R"(
    {
        "xayaplayers" :
        [
                {
                        "balance" : 50,
                        "name" : "domob",
                        "purchasing" :
                        [
                                "36adece2-8ed9-3114-db6e-24fa8c494fa5"
                        ]
                }
        ]

    }
  )");
}

TEST_F (PendingStateUpdaterTests, TestSweetenerCollect)
{
    auto a = xayaplayers.CreateNew ("testy2", ctx.RoConfig(), rnd);
    state.AddClaimingSweetenerReward (*a, "authyauth", 14);
    a.reset();
    
    ExpectStateJson (R"(
    {
      "xayaplayers":
        [
          {
            "name": "testy2",
			"sweetclmauth" : 
			[
              "authyauth"
			],
			"sweetclmfghtr" : 
			[
              14
			]            
          }
        ]
    }
  )");    
}

/*
TEST_F (PendingStateUpdaterTests, BatchSubmitPendingTests)
{
  auto a = xayaplayers.CreateNew ("testy2", ctx.RoConfig(), rnd);
  a->AddBalance(100);
  
  a->GetInventory().SetFungibleCount("Common_Gumdrop", 1);
  a->GetInventory().SetFungibleCount("Common_Icing", 1);  
  
  a.reset();
  
  auto r0 = tbl2.CreateNew("testy2", "5864a19b-c8c0-2d34-eaef-9455af0baf2c", ctx.RoConfig());
  r0.reset();     
  
  tbl2.GetById(1)->SetOwner("testy2");
  
  ProcessWithDevPaymentAndBurnAsBatch ("testy2", 0, 0, R"({"ca": {"d": {"rid": [1]}}})");  
  
  ExpectStateJson (R"(
    {
      "xayaplayers":
        [
          {
            "name": "testy2",
			"recipedestroy" : 
			[
              1
			]
          }
        ]
    }
  )");
}*/


TEST_F (PendingStateUpdaterTests, SubmitRecepieDestroy)
{
  auto a = xayaplayers.CreateNew ("testy2", ctx.RoConfig(), rnd);
  a->AddBalance(100);
  
  a->GetInventory().SetFungibleCount("Common_Gumdrop", 1);
  a->GetInventory().SetFungibleCount("Common_Icing", 1);  
  
  a.reset();
  
  auto r0 = tbl2.CreateNew("testy2", "5864a19b-c8c0-2d34-eaef-9455af0baf2c", ctx.RoConfig());
  r0.reset();     
  
  tbl2.GetById(1)->SetOwner("testy2");
  
  Process ("testy2", R"({"ca": {"d": {"rid": [1]}}})");  
  
  ExpectStateJson (R"(
    {
      "xayaplayers":
        [
          {
            "name": "testy2",
			"recipedestroy" : 
			[
              1
			]
          }
        ]
    }
  )");
}

TEST_F (PendingStateUpdaterTests, SubmitRecepieInstance)
{
  auto a = xayaplayers.CreateNew ("testy2", ctx.RoConfig(), rnd);
  a->AddBalance(100);
  
  a->GetInventory().SetFungibleCount("Common_Gumdrop", 1);
  a->GetInventory().SetFungibleCount("Common_Icing", 1);  
  
  a.reset();
  
  auto r0 = tbl2.CreateNew("testy2", "5864a19b-c8c0-2d34-eaef-9455af0baf2c", ctx.RoConfig());
  r0.reset();     
  
  tbl2.GetById(1)->SetOwner("testy2");
  
  Process ("testy2", R"({"ca": {"r": {"rid": 1, "fid": 0}}})");  
  
  ExpectStateJson (R"(
    {
      "xayaplayers":
        [
          {
            "name": "testy2",
			"ongoings" : 
			[
				{
					"ival" : 1,
					"sval" : "",
					"type" : 1
				}
			]
          }
        ]
    }
  )");
}

TEST_F (PendingStateUpdaterTests, SubmitExpedition)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  EXPECT_EQ (ft->GetStatus(), FighterStatus::Available);
  ft.reset();
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 3);
  xp.reset();
        
  Process ("domob", R"({"exp": {"f": {"eid": "c064e7f7-acbf-4f74-fab8-cccd7b2d4004", "fid": 4}}})");  
  
  ExpectStateJson (R"(
    {
      "xayaplayers":
        [
          {
            "name": "domob",
			"ongoings" : 
			[
				{
					"ival" : 4,
					"sval" : "c064e7f7-acbf-4f74-fab8-cccd7b2d4004",
					"type" : 2
				}
			]
          }
        ]
    }
  )");
}

TEST_F (PendingStateUpdaterTests, SubmitTournamentEntry)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  int ft1id = ft->GetId();
  ft.reset();
  
  auto ft2 = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  int ft2id = ft2->GetId();
  ft2.reset();
  xp.reset();
  
  PXLogic::ReopenMissingTournaments (db, ctx);
  
  auto tutorialTrmn = tbl5.GetByAuthIdName("cbd2e78a-37ce-b864-793d-8dd27788a774", ctx.RoConfig());
  ASSERT_TRUE (tutorialTrmn != nullptr);
  uint32_t TID = tutorialTrmn->GetId();
  tutorialTrmn.reset();  
        
  std::ostringstream s;
  s << TID;
  std::string converted(s.str());  
  
  std::ostringstream s1;
  s1 << ft1id;
  std::string converted1(s1.str());  

  std::ostringstream s2;
  s2 << ft2id;
  std::string converted2(s2.str());    
  
  Process ("domob", R"({"tm": {"e": {"tid": )" + converted + R"(, "fc": [)"+converted1+R"(,)"+converted2+R"(]}}})"); 
  
  ExpectStateJson (R"(
  {
        "xayaplayers" :
        [
                {
                        "name" : "domob",
                        "tournamententries" :
                        [
                                {
                                        "fids" :
                                        [
                                                )"+converted1+R"(,
                                                )"+converted2+R"(
                                        ],
                                        "id" : )"+converted+R"(
                                }
                        ]
                }
        ]

  }
  )");
}

TEST_F (PendingStateUpdaterTests, SubmitSpecialTournamentEntry)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  xp->AddBalance(100);
  xp.reset();
  
  ctx.SetTimestamp(100);
  
  PXLogic::ProcessSpecialTournaments (db, ctx, rnd);

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
  specialTournamentTier1->MutableProto().set_state((int)pxd::SpecialTournamentState::Listed); 
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

  Process ("domob", R"({"tms": {"e": {"tid": )" + converted + R"(, "fc": [)"+converted1x+R"(,)"+converted2x + R"(,)" + converted3x + R"(,)" + converted4x + R"(,)" + converted5x + R"(,)" + converted6x + R"(]}}})"); 
  
  ExpectStateJson (R"(
    {
        "xayaplayers" :
        [
                {
                        "name" : "domob",
                        "specialtournamententries" :
                        [
                                {
                                        "fids" :
                                        [
                                                )"+converted1x+R"(,
                                                )"+converted2x+R"(,
                                                )"+converted3x+R"(,
                                                )"+converted4x+R"(,
                                                )"+converted5x+R"(,
                                                )"+converted6x+R"(
                                        ],
                                        "id" : )"+converted+R"(
                                }
                        ]
                }
        ]

    }
  )");
}

TEST_F (PendingStateUpdaterTests, ExpeditionGetRewards)
{
  auto a = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  
  std::vector<std::string> expeditionIDArray;
  expeditionIDArray.push_back("tst");
  std::vector<uint32_t> rewardDatabaseIds;
  rewardDatabaseIds.push_back(1);
  rewardDatabaseIds.push_back(4);
   
  state.AddRewardIDs (*a, expeditionIDArray, rewardDatabaseIds);
  a.reset ();
   
  ExpectStateJson (R"(
    {
    "xayaplayers" : 
	[
		{
			"claimingrewards" : 
			[
				{
					"ids" : 
					[
						1,
						4
					],
					"type" : "tst"
				}
			],
			"name" : "domob"
		}
	]
    }
  )"); 
  
}

TEST_F (PendingStateUpdaterTests, SubmitRecepieInstanceMultiple)
{
  auto a = xayaplayers.CreateNew ("testy2", ctx.RoConfig(), rnd);
  a->AddBalance(100);
  
  auto r0 = tbl2.CreateNew("testy2", "2729a029-a53e-7b34-38c7-2c6ebe932c94", ctx.RoConfig());
  r0.reset();   
  
  a->GetInventory().SetFungibleCount("Common_Gumdrop", 1);
  a->GetInventory().SetFungibleCount("Common_Icing", 1);    
  a->GetInventory().SetFungibleCount("Rare_Giant Chocolate Chip", 50);
  a->GetInventory().SetFungibleCount("Rare_Peanut Butter Cup", 30);
  a->GetInventory().SetFungibleCount("Uncommon_Chocolate Nut", 50);
  a->GetInventory().SetFungibleCount("Uncommon_Peppermint", 30);
  a->GetInventory().SetFungibleCount("Common_Candy Button", 40);
  
  
  a.reset();
  
  tbl2.GetById(1)->SetOwner("testy2");
  tbl2.GetById(2)->SetOwner("testy2");
  
  Process ("testy2", R"({"ca": {"r": {"rid": 1, "fid": 0}}})"); 
  Process ("testy2", R"({"ca": {"r": {"rid": 2, "fid": 0}}})");    
  
  ExpectStateJson (R"(
    {
      "xayaplayers":
        [
          {
            "name": "testy2",
			"ongoings" : 
			[
				{
					"ival" : 1,
					"sval" : "",
					"type" : 1
				},
				{
					"ival" : 2,
					"sval" : "",
					"type" : 1
				}                
			]
          }
        ]
    }
  )");
}

TEST_F (PendingStateUpdaterTests, SubmitRecepieNotExistingInPlayerInventory)
{
  auto a = xayaplayers.CreateNew ("testy2", ctx.RoConfig(), rnd);
  a->AddBalance(100);
  a.reset();
  
  Process ("testy2", R"({"ca": {"r": {"rid": 3, "fid": 0}}})");  
  
  ExpectStateJson (R"(
    {
        "xayaplayers" :
        [
                {
                        "name" : "testy2"
                }
        ]

    }
  )");
}

} // namespace pxd

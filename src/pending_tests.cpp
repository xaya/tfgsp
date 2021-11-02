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

  PendingStateTests () : xayaplayers(db), tbl2(db), tbl3(db), tbl5(db)
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
    LOG (WARNING) << "Actual JSON for the pending state:\n" << actual;
    LOG (WARNING) << "Expected JSON for the pending state:\n" << expectedStr;
    
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

TEST_F (PendingStateTests, CoinTransferBurn)
{
  auto a = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);

  CoinTransferBurn coinOp;
  coinOp.minted = 32;
  coinOp.burnt = 5;
  coinOp.transfers["andy"] = 20;
  state.AddCoinTransferBurn (*a, coinOp);

  coinOp.minted = 10;
  coinOp.burnt = 2;
  coinOp.transfers["andy"] = 1;
  coinOp.transfers["daniel"] = 10;
  state.AddCoinTransferBurn (*a, coinOp);

  a.reset ();

  ExpectStateJson (R"(
{
	"xayaplayers" : 
	[
		{
			"coinops" : 
			{
				"burnt" : 7,
				"minted" : 42,
				"transfers" : 
				{
					"andy" : 21,
					"daniel" : 10
				}
			},
			"name" : "domob"
		}
	]
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
      moveObj["out"][ctx.RoConfig ()->params ().dev_addr ()]
          = xaya::ChiAmountToJson (paidToDev);
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

TEST_F (PendingStateUpdaterTests, CoinTransferBurn)
{
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->AddBalance (100);
  xayaplayers.CreateNew ("andy", ctx.RoConfig(), rnd)->AddBalance (100);

  Process ("domob", R"({
    "abc": "foo",
    "vc": {"x": 5, "b": 10}
  })");
  Process ("andy", R"({
    "vc": {"b": -10, "t": {"domob": 5, "invalid": 2}}
  })");
  Process ("invalid", R"({
    "vc": {"b": 1}
  })");
  Process ("domob", R"({
    "abc": "foo",
    "vc": {"b": 2, "t": {"domob": 1, "andy": 5}}
  })");
  Process ("andy", R"({
    "vc": {"b": 101, "t": {"domob": 101}}
  })");

  ExpectStateJson (R"(
    {
      "xayaplayers" : 
      [
          {
              "coinops" : 
              {
                  "burnt" : 101,
                  "minted" : 0,
                  "transfers" : 
                  {
                      "domob" : 5,
                      "invalid" : 2
                  }
              },
              "name" : "andy"
          },
          {
              "coinops" : 
              {
                  "burnt" : 12,
                  "minted" : 0,
                  "transfers" : 
                  {
                      "andy" : 5
                  }
              },
              "name" : "domob"
          }
      ]
    }
  )");
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
            "crystalbundles": ["T1"]
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
  
  Process ("testy2", R"({"ca": {"r": {"rid": 1, "fid": 0}}})");  
  
  ExpectStateJson (R"(
    {
      "xayaplayers":
        [
          {
            "name": "testy2",
            "ongoings": [1]
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
            "ongoings": [2]
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

TEST_F (PendingStateUpdaterTests, ExpeditionGetRewards)
{
  auto a = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  
  std::vector<uint32_t> rewardDatabaseIds;
  rewardDatabaseIds.push_back(1);
  rewardDatabaseIds.push_back(4);
   
  state.AddRewardIDs (*a, rewardDatabaseIds);
  a.reset ();
   
  ExpectStateJson (R"(
    {
      "xayaplayers":
        [
          {
            "name": "domob",
            "claimingrewards": [1, 4]
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
  
  Process ("testy2", R"({"ca": {"r": {"rid": 1, "fid": 0}}})"); 
  Process ("testy2", R"({"ca": {"r": {"rid": 2, "fid": 0}}})");    
  
  ExpectStateJson (R"(
    {
      "xayaplayers":
        [
          {
            "name": "testy2",
            "ongoings": [1, 1]
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

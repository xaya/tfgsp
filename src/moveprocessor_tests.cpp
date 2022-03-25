/*
    GSP for the tf blockchain game
    Copyright (C) 2019-2020  Autonomous Worlds Ltd

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

#include "moveprocessor.hpp"

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

class MoveProcessorTests : public DBTestWithSchema
{

protected:

  TestRandom rnd;
  ContextForTesting ctx;
  XayaPlayersTable xayaplayers;
  RecipeInstanceTable tbl2;
  FighterTable tbl3;
  TournamentTable tbl4;

  explicit MoveProcessorTests ()
    : xayaplayers(db), tbl2(db), tbl3(db), tbl4(db)
  {}

  /**
   * Processes an array of admin commands given as JSON string.
   */
  void
  ProcessAdmin (const std::string& str)
  {
    MoveProcessor mvProc(db, rnd, ctx);
    mvProc.ProcessAdmin (ParseJson (str));
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
   * Processes the given data as string, adding the given amount as payment
   * to the dev address for each entry.  This is a utility method to avoid the
   * need of pasting in the long "out" and dev-address parts.
   */
  void
  ProcessWithDevPayment (const std::string& str, const Amount amount)
  {
    Json::Value val = ParseJson (str);
    for (auto& entry : val)
      entry["out"][ctx.RoConfig()->params ().dev_addr ()]
          = xaya::ChiAmountToJson (amount);

    MoveProcessor mvProc(db, rnd, ctx);
    mvProc.ProcessAll (val);
  }

};

/* ************************************************************************** */

TEST_F (MoveProcessorTests, InvalidDataFromXaya)
{
  EXPECT_DEATH (Process ("{}"), "isArray");

  EXPECT_DEATH (Process (R"(
    [{"name": "domob"}]
  )"), "isMember.*move");

  EXPECT_DEATH (Process (R"(
    [{"move": {}}]
  )"), "nameVal.isString");
  EXPECT_DEATH (Process (R"(
    [{"name": 5, "move": {}}]
  )"), "nameVal.isString");

  EXPECT_DEATH (Process (R"([{
    "name": "domob", "move": {},
    "out": {")" + ctx.RoConfig()->params ().dev_addr () + R"(": false}
  }])"), "JSON value for amount is not double");
}

TEST_F (MoveProcessorTests, InvalidAdminFromXaya)
{
  EXPECT_DEATH (ProcessAdmin ("42"), "isArray");
  EXPECT_DEATH (ProcessAdmin ("false"), "isArray");
  EXPECT_DEATH (ProcessAdmin ("null"), "isArray");
  EXPECT_DEATH (ProcessAdmin ("{}"), "isArray");
  EXPECT_DEATH (ProcessAdmin ("[5]"), "isObject");
}

TEST_F (MoveProcessorTests, AllMoveDataAccepted)
{
  for (const auto& mvStr : {"5", "false", "\"foo\"", "{}"})
    {
      LOG (INFO) << "Testing move data (in valid array): " << mvStr;

      std::ostringstream fullMoves;
      fullMoves
          << R"([{"name": "test", "move": )"
          << mvStr
          << "}]";

      Process (fullMoves.str ());
    }
}

TEST_F (MoveProcessorTests, AllAdminDataAccepted)
{
  for (const auto& admStr : {"42", "[5]", "\"foo\"", "null"})
    {
      LOG (INFO) << "Testing admin data (in valid array): " << admStr;

      std::ostringstream fullAdm;
      fullAdm << R"([{"cmd": )" << admStr << "}]";

      ProcessAdmin (fullAdm.str ());
    }
}

/* ************************************************************************** */

using XayaPlayersUpdateTests = MoveProcessorTests;

TEST_F (XayaPlayersUpdateTests, Initialisation)
{
  Process (R"([
    {"name": "domob", "move": {"a": {"x": 42, "init": {"role": "p"}}}}
  ])");

  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetRole (), PlayerRole::PLAYER);
}

TEST_F (XayaPlayersUpdateTests, InvalidInitialisation)
{
  EXPECT_DEATH (Process (R"([
    {"name": "domob", "move": {"a": {"init": {"x": 1, "role": "b"}}}},
    {"name": "domob", "move": {"a": {"init": {"role": "x"}}}},
    {"name": "domob", "move": {"a": {"init": {"role": "a"}}}},
    {"name": "domob", "move": {"a": {"init": {"y": 5}}}},
    {"name": "domob", "move": {"a": {"init": false}}},
    {"name": "domob", "move": {"a": 42}}
  ])"), "Invalid role value from database");
}

TEST_F (XayaPlayersUpdateTests, RecepieInstanceSheduleTest)
{
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
}

TEST_F (XayaPlayersUpdateTests, RecepieInstanceWrongValues)
{
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->AddBalance (100);
  
  Process (R"([
    {"name": "domob", "move": {"ca": {"r": {"rid": 4, "fid": 0}}}}
  ])");  
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 0);
}

TEST_F (XayaPlayersUpdateTests, RecepieInstanceInsufficientResources)
{
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->AddBalance (100);

  tbl2.GetById(1)->SetOwner("domob");
  tbl2.GetById(2)->SetOwner("domob");
  
  auto r0 = tbl2.CreateNew("domob", "5864a19b-c8c0-2d34-eaef-9455af0baf2c", ctx.RoConfig());
  r0.reset();  

  Process (R"([
    {"name": "domob", "move": {"ca": {"r": {"rid": 1, "fid": 0}}}}
  ])");  
  
  Process (R"([
    {"name": "domob", "move": {"ca": {"r": {"rid": 2, "fid": 0}}}}
  ])");    
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 1);
}

TEST_F (XayaPlayersUpdateTests, SweetenerSubmitForCooking)
{
  auto pl = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  pl->GetInventory().SetFungibleCount("Sweetener_R2", 1);
  
  pl->GetInventory().SetFungibleCount("Common_Icing", 10);
  pl->GetInventory().SetFungibleCount("Common_Fruit Slice", 10);

  pl.reset();
  
  auto ft = tbl3.GetById(4, ctx.RoConfig());
  ASSERT_TRUE (ft != nullptr); 
  ft->MutableProto().set_rating(1210); 
  ft->MutableProto().set_sweetness((int)pxd::Sweetness::Bittersweet);  
  ft.reset();
  
  Process (R"([
    {"name": "domob", "move": {"ca": {"s": {"sid": "d596403b-b76f-52c4-6956-4bfd55231de0", "fid": 4, "rid": 1}}}}
  ])");  
  
  pl = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (pl != nullptr);
  EXPECT_EQ (pl->GetOngoingsSize (), 1);
  pl.reset();
}

TEST_F (XayaPlayersUpdateTests, ExpeditionInstanceSheduleTest)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  xp->GetInventory().SetFungibleCount("Common_Gumdrop", 1);
  xp->GetInventory().SetFungibleCount("Common_Icing", 1);    
  
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  ft.reset();
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 3);
  xp.reset();
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "c064e7f7-acbf-4f74-fab8-cccd7b2d4004", "fid": 4}}}}
  ])");  
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 1);
}

TEST_F (XayaPlayersUpdateTests, ExpeditionMultipleInstanceSheduleTest)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  xp->GetInventory().SetFungibleCount("Common_Gumdrop", 1);
  xp->GetInventory().SetFungibleCount("Common_Icing", 1);    
  
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  ft.reset();
  
  ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  int myID = ft->GetId();
  ft.reset();  
  
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 4);
  xp.reset();
  
  std::ostringstream s;
  s << myID;
  std::string converted(s.str());  
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "c064e7f7-acbf-4f74-fab8-cccd7b2d4004", "fid": [4,)" + converted + R"(]}}}}
  ])");  
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 2);
}

TEST_F (XayaPlayersUpdateTests, ExpeditionInstanceSheduleTestWrongData1)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  ft.reset();
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 3);
  xp.reset();
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "c064e7f7-acbf-4f74-fab8-cccd7b2d4004", "fid": 1}}}}
  ])");  
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 0);
}

TEST_F (XayaPlayersUpdateTests, ExpeditionInstanceSheduleTestWrongData2)
{
  auto xp = xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd);
  auto ft = tbl3.CreateNew ("domob", 1, ctx.RoConfig(), rnd);
  ft.reset();
  EXPECT_EQ (xp->CollectInventoryFighters(ctx.RoConfig()).size(), 3);
  xp.reset();
  
  Process (R"([
    {"name": "domob", "move": {"exp": {"f": {"eid": "c064e7f7-acbf-0000-fab8-cccd7b2d4004", "fid": 2}}}}
  ])");  
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 0);
}

TEST_F (XayaPlayersUpdateTests, InitialisationOfExistingAccount)
{
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->SetRole(PlayerRole::PLAYER);

  Process (R"([
    {"name": "domob", "move": {"a": {"init": {"role": "f"}}}}
  ])");

  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetRole (), PlayerRole::PLAYER);
}

/* ************************************************************************** */

class CoinOperationTests : public MoveProcessorTests
{

protected:

  /**
   * Expects that the balances of the given accounts are as stated.
   */
  void
  ExpectBalances (const std::map<std::string, Amount>& expected)
  {
    for (const auto& entry : expected)
      {
        auto a = xayaplayers.GetByName (entry.first, ctx.RoConfig());
        if (a == nullptr)
          ASSERT_EQ (entry.second, 0);
        else
          ASSERT_EQ (a->GetBalance (), entry.second);
      }
  }

};

TEST_F (CoinOperationTests, Invalid)
{
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->AddBalance (100);

  Process (R"([
    {"name": "domob", "move": {"vc": 42}},
    {"name": "domob", "move": {"vc": []}},
    {"name": "domob", "move": {"vc": {}}},
    {"name": "domob", "move": {"vc": {"x": 10}}},
    {"name": "domob", "move": {"vc": {"b": -1}}},
    {"name": "domob", "move": {"vc": {"b": 1000000001}}},
    {"name": "domob", "move": {"vc": {"b": 999999999999999999}}},
    {"name": "domob", "move": {"vc": {"t": 42}}},
    {"name": "domob", "move": {"vc": {"t": "other"}}},
    {"name": "domob", "move": {"vc": {"t": {"other": -1}}}},
    {"name": "domob", "move": {"vc": {"t": {"other": 1000000001}}}},
    {"name": "domob", "move": {"vc": {"t": {"other": 1.999999}}}},
    {"name": "domob", "move": {"vc": {"t": {"other": 101}}}},
    {"name": "domob", "move": {"vc": {"m": {"x": "foo"}}}, "burnt": 1.0},
    {"name": "domob", "move": {"vc": {"m": null}}, "burnt": 1.0},
    {"name": "domob", "move": {"vc": {"m": []}}, "burnt": 1.0},
    {"name": "domob", "move": {"vc": {}}, "burnt": 1.0}
  ])");

  ExpectBalances ({{"domob", 249}, {"other", 351}});
}

TEST_F (CoinOperationTests, ExtraFieldsAreFine)
{
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->AddBalance (100);
  Process (R"([
    {"name": "domob", "move": {"vc": {"b": 10, "x": "foo"}}}
  ])");
  ExpectBalances ({{"domob", 340}});
}

TEST_F (CoinOperationTests, BurnAndTransfer)
{
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->AddBalance (100);

  /* Some of the burns and transfers are invalid.  They should just be ignored,
     without affecting the rest of the operation (valid parts).  */
  Process (R"([
    {"name": "domob", "move": {"vc":
      {
        "b": 10,
        "t": {"a": "invalid", "b": -5, "c": 1000, "second": 5, "third": 3}
      }}
    },
    {"name": "domob", "move": {"vc":
      {
        "b": 1000,
        "t": {"third": 2}
      }}
    },
    {"name": "second", "move": {"vc": {"b": 1}}}
  ])");

  ExpectBalances ({
    {"domob", 330},
    {"second", 254},
    {"third", 255},
  });
}

TEST_F (CoinOperationTests, BurnAll)
{
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->AddBalance (100);
  Process (R"([
    {"name": "domob", "move": {"vc": {"b": 100}}}
  ])");
  ExpectBalances ({{"domob", 250}});
}

TEST_F (CoinOperationTests, TransferAll)
{
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->AddBalance (100);
  Process (R"([
    {"name": "domob", "move": {"vc": {"t": {"other": 100}}}}
  ])");
  ExpectBalances ({{"domob", 250}, {"other", 350}});
}

TEST_F (CoinOperationTests, SelfTransfer)
{
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->AddBalance (100);
  Process (R"([
    {"name": "domob", "move": {"vc": {"t": {"domob": 90, "other": 20}}}}
  ])");
  ExpectBalances ({{"domob", 330}, {"other", 270}});
}

TEST_F (CoinOperationTests, Minting)
{
  xayaplayers.CreateNew ("andy", ctx.RoConfig(), rnd)->SetRole(PlayerRole::PLAYER);

  Process (R"([
    {"name": "domob", "move": {"vc": {"m": {}}}, "burnt": 1000000},
    {"name": "andy", "move": {"vc": {"m": {}}}, "burnt": 2.00019999}
  ])");

  ExpectBalances ({{"domob", 10'000'000'250}});

  MoneySupply ms(db);
  EXPECT_EQ (ms.Get ("burnsale"), 10'000'010'000);
}

TEST_F (CoinOperationTests, BurnsaleBalance)
{
  Process (R"([
    {"name": "domob", "move": {"vc": {"m": {}}}, "burnt": 0.1},
    {"name": "domob", "move": {"vc": {"b": 10}}, "burnt": 1}
  ])");

  ExpectBalances ({{"domob", 1240}});
  EXPECT_EQ (xayaplayers.GetByName ("domob", ctx.RoConfig())->GetProto ().burnsale_balance (),
             1'000);
}

TEST_F (CoinOperationTests, MintBeforeBurnBeforeTransfer)
{
  Process (R"([
    {"name": "domob", "burnt": 0.01, "move": {"vc":
      {
        "m": {},
        "b": 90,
        "t": {"other": 20}
      }}
    }
  ])");

  ExpectBalances ({
    {"domob", 240},
    {"other", 270},
  });

  Process (R"([
    {"name": "domob", "burnt": 0.01, "move": {"vc":
      {
        "m": {},
        "t": {"other": 20}
      }}
    }
  ])");

  ExpectBalances ({
    {"domob", 320},
    {"other", 290},
  });

  MoneySupply ms(db);
  EXPECT_EQ (ms.Get ("burnsale"), 200);
}

TEST_F (CoinOperationTests, TransferOrder)
{
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->AddBalance (100);

  Process (R"([
    {"name": "domob", "move": {"vc":
      {
        "t": {"z": 10, "a": 101, "middle": 99}
      }}
    }
  ])");

  ExpectBalances ({
    {"domob", 140},
    {"a", 351},
    {"middle", 349},
    {"z", 260},
  });
}

TEST_F (CoinOperationTests, PutFighterForSaleAndThenBuy)
{
  Process (R"([
    {"name": "domob", "move": {"a": {"x": 42, "init": {"role": "p"}}}}
  ])");
  
  auto ft = tbl3.GetById(4, ctx.RoConfig());
  ASSERT_TRUE (ft != nullptr);
  ft->MutableProto().set_isaccountbound(false);
  ft.reset();  
  
  Process (R"([
    {"name": "domob", "move": {"f": {"s": {"fid": 4, "d": 3, "p": 500}}}}
  ])");
  
  int secondsInOneDay = 24 * 60 * 60;
  int blocksInOneDay = secondsInOneDay / 30;  
  
  ft = tbl3.GetById(4, ctx.RoConfig());
  ASSERT_TRUE (ft != nullptr);

  EXPECT_EQ (ft->GetStatus(), pxd::FighterStatus::Exchange);
  EXPECT_EQ (ft->GetProto().exchangeexpire(), ctx.Height () + (blocksInOneDay * 3));
  EXPECT_EQ (ft->GetProto().exchangeprice(), 500);
  ft.reset();
  
  ExpectBalances ({{"domob", 240}});
  
  Process (R"([
    {"name": "andy", "move": {"a": {"x": 42, "init": {"role": "p"}}}}
  ])");  
  
  Process (R"([
    {"name": "andy", "move": {"f": {"b": {"fid": 4}}}}
  ])");  

  ft = tbl3.GetById(4, ctx.RoConfig());
  ASSERT_TRUE (ft != nullptr);  
  EXPECT_EQ (ft->GetOwner(), "domob");
  ft.reset();

  auto a = xayaplayers.GetByName ("andy", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  a->AddBalance(600);
  a.reset();

  Process (R"([
    {"name": "andy", "move": {"f": {"b": {"fid": 4}}}}
  ])"); 
  
  ft = tbl3.GetById(4, ctx.RoConfig());
  ASSERT_TRUE (ft != nullptr);  
  EXPECT_EQ (ft->GetOwner(), "andy");  
  EXPECT_EQ (ft->GetStatus(), pxd::FighterStatus::Available); 
  ft.reset();
}

TEST_F (CoinOperationTests, PutFighterForSaleAndThenRemove)
{

   Process (R"([
    {"name": "domob", "move": {"a": {"x": 42, "init": {"role": "p"}}}}
  ])");
  
  auto ft = tbl3.GetById(4, ctx.RoConfig());
  ASSERT_TRUE (ft != nullptr);
  ft->MutableProto().set_isaccountbound(false);
  ft.reset();  
  
  Process (R"([
    {"name": "domob", "move": {"f": {"s": {"fid": 4, "d": 3, "p": 500}}}}
  ])");
  
  int secondsInOneDay = 24 * 60 * 60;
  int blocksInOneDay = secondsInOneDay / 30;  
  ft = tbl3.GetById(4, ctx.RoConfig());
  ASSERT_TRUE (ft != nullptr);

  EXPECT_EQ (ft->GetStatus(), pxd::FighterStatus::Exchange);
  EXPECT_EQ (ft->GetProto().exchangeexpire(), ctx.Height () + (blocksInOneDay * 3));
  EXPECT_EQ (ft->GetProto().exchangeprice(), 500);
  ft.reset();
  
  ExpectBalances ({{"domob", 240}});

  Process (R"([
    {"name": "domob", "move": {"f": {"r": {"fid": 4}}}}
  ])");  
  
  ft = tbl3.GetById(4, ctx.RoConfig());
  ASSERT_TRUE (ft != nullptr);

  EXPECT_EQ (ft->GetStatus(), pxd::FighterStatus::Available);
  ft.reset();  
}

TEST_F (CoinOperationTests, PutFighterForSaleCrazyPrices)
{
  Process (R"([
    {"name": "domob", "move": {"a": {"x": 42, "init": {"role": "p"}}}}
  ])");
  
  Process (R"([
    {"name": "domob", "move": {"f": {"s": {"fid": 4, "d": 3, "p": -10}}}}
  ])");
  
  auto ft = tbl3.GetById(4, ctx.RoConfig());
  ASSERT_TRUE (ft != nullptr);
  EXPECT_EQ (ft->GetStatus(), pxd::FighterStatus::Available);
  ft.reset();
  
  Process (R"([
    {"name": "domob", "move": {"f": {"s": {"fid": 4, "d": 3, "p": 5000000000000000}}}}
  ])");  
  
  ft = tbl3.GetById(4, ctx.RoConfig());
  ASSERT_TRUE (ft != nullptr);
  EXPECT_EQ (ft->GetStatus(), pxd::FighterStatus::Available);
  ft.reset();  
  
  ExpectBalances ({{"domob", 250}});
}

TEST_F (CoinOperationTests, PurchaseStuff)
{
  Process (R"([
    {"name": "domob", "move": {"a": {"x": 42, "init": {"role": "p"}}}}
  ])");

  ProcessWithDevPayment (R"([{
    "name": "domob",
    "move":
      {
        "pc": "T1"
      }
  }])", 1 * COIN);

  ExpectBalances ({{"domob", 350}});

  Process (R"([
    {"name": "domob", "move": {"pg": "ca3378db-cd54-e514-7ae8-23705781bb9d"}}
  ])");

  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Goodie_PressureCooker_1"), 3);
  a.reset();
  
  ExpectBalances ({{"domob", 250}});
  
  ProcessWithDevPayment (R"([{
    "name": "domob",
    "move":
      {
        "pc": "T1"
      }
  }])", 1 * COIN);

  ProcessWithDevPayment (R"([{
    "name": "domob",
    "move":
      {
        "pc": "T1"
      }
  }])", 1 * COIN);

  ExpectBalances ({{"domob", 450}});
  
    Process (R"([
    {"name": "domob", "move": {"pgb": "645a9411-d8f1-3e24-aa12-f3f79665dca2"}}
  ])");
  
  ExpectBalances ({{"domob", 275}});
  
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Goodie_PressureCooker_1"), 4);
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Goodie_Espresso_1"), 1);
  a.reset();

    Process (R"([
    {"name": "domob", "move": {"ps": "36adece2-8ed9-3114-db6e-24fa8c494fa5"}}
  ])");  
  
  ExpectBalances ({{"domob", 225}});
  
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Goodie_PressureCooker_1"), 4);
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Goodie_Espresso_1"), 1);
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Sweetener_R6"), 1);
  a.reset();  
}

TEST_F (CoinOperationTests, PurchaseCrystalsWrongData)
{
  Process (R"([
    {"name": "domob", "move": {"a": {"x": 42, "init": {"role": "p"}}}}
  ])");
  
  ProcessWithDevPayment (R"([{
    "name": "domob",
    "move":
      {
        "pc": "N8"
      }
  }])", 1 * COIN);

  ExpectBalances ({{"domob", 250}});
}

TEST_F (CoinOperationTests, PurchaseCrystalsInsufficientCoin)
{
  Process (R"([
    {"name": "domob", "move": {"a": {"x": 42, "init": {"role": "p"}}}}
  ])");
  
  ProcessWithDevPayment (R"([{
    "name": "domob",
    "move":
      {
        "pc": "N8"
      }
  }])", 0.25 * COIN);

  ExpectBalances ({{"domob", 250}});
}

TEST_F (CoinOperationTests, PurchaseCrystalsTwiceInARow)
{
  Process (R"([
    {"name": "domob", "move": {"a": {"x": 42, "init": {"role": "p"}}}}
  ])");
  
  ProcessWithDevPayment (R"([{
    "name": "domob",
    "move":
      {
        "pc": "T1"
      }
  }])", 1 * COIN);
  
  ProcessWithDevPayment (R"([{
    "name": "domob",
    "move":
      {
        "pc": "T1"
      }
  }])", 1 * COIN);  

  ExpectBalances ({{"domob", 450}});
}

/* ************************************************************************** */

class GameStartTests : public CoinOperationTests
{

protected:

  GameStartTests ()
  {
    FLAGS_fork_height_gamestart = 100;
  }

  ~GameStartTests ()
  {
    FLAGS_fork_height_gamestart = -1;
  }

};

TEST_F (GameStartTests, Before)
{
  ctx.SetHeight (99);
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->AddBalance (100);

  Process (R"([
    {"name": "andy", "move": {"vc": {"m": {}}}, "burnt": 1},
    {"name": "domob", "move": {"vc": {"b": 10}}},
    {"name": "domob", "move": {"vc": {"t": {"daniel": 5}}}},
    {"name": "domob", "move": {"a": {"init": {"role": "p"}}}}
  ])");

  ExpectBalances ({
    {"domob", 335},
    {"daniel", 255},
    {"andy", 10'250},
  });
}

TEST_F (GameStartTests, After)
{
  ctx.SetHeight (100);
  Process (R"([
    {"name": "domob", "move": {"a": {"init": {"role": "p"}}}}
  ])");

  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());

  EXPECT_EQ (a->GetRole(), PlayerRole::PLAYER);
}

/* ************************************************************************** */

} // anonymous namespace
} // namespace pxd

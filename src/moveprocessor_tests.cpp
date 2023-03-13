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
    {      
      entry["out"]["CPxvCsP9wr8ow4x5r6D1gYpxAFBg6ACzc6"] = xaya::ChiAmountToJson ((amount / 35) * 1);
      entry["out"]["CHPVEUVFKy1YugLhVFQmqE8iaPch3MxGsd"] = xaya::ChiAmountToJson ((amount / 35) * 2);
      entry["out"]["Cdwan1eAmsvA2sE6XNUB4ZWNDMHwoyhRYr"] = xaya::ChiAmountToJson ((amount / 35) * 3);
      entry["out"]["CcX1ksjf4c9qJ2ftd51T2iJbNkRm5SRc94"] = xaya::ChiAmountToJson ((amount / 35) * 4);
      entry["out"]["CGr5MT1C5PXUpYhaDQkKoLxP11qJtJxzu8"] = xaya::ChiAmountToJson ((amount / 35) * 5);
      entry["out"]["CeJt7YpW8P9jMeVrVm58nUaoM4fJ4KXMUS"] = xaya::ChiAmountToJson ((amount / 35) * 6);
      entry["out"]["CZhfYfqbMdzeS5ADRR2su12cWD3TQaeBFc"] = xaya::ChiAmountToJson ((amount / 35) * 7);
	  entry["out"]["chi1qmexgupgkmqh6gt23f96hfcpacf0rlzj069cck8pp3g6nacz97qasy87a3m"] = xaya::ChiAmountToJson ((amount / 35) * 7);
    }

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
    {"name": "domob", "move": {"a": {"x": 42, "init": {"address": "CGUpAcjsb6MDktSYg8yRDxDutr7FhWtdWC"}}}}
  ])");
  
  UpdateState ("[]");

  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
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


TEST_F (XayaPlayersUpdateTests, TestFractionPaymentRounding)
{
  Amount smallestPayFraction = 500100;
  smallestPayFraction = smallestPayFraction / 1000;
  smallestPayFraction = smallestPayFraction * 1000;    
  
   EXPECT_EQ (smallestPayFraction, 500000);
}

TEST_F (XayaPlayersUpdateTests, BacthSubmitNormalTest)
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
    {"name": "domob", "move": [{"ca": {"r": {"rid": 1, "fid": 0}}}]}
  ])");  
  
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 1);
}

TEST_F (XayaPlayersUpdateTests, RecepieDestroyTest)
{
  ctx.SetHeight (4265752);
  
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->AddBalance (100);
  
  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  a.reset();
  
  tbl2.GetById(1)->SetOwner("domob");
  
  auto r0 = tbl2.CreateNew("domob", "5864a19b-c8c0-2d34-eaef-9455af0baf2c", ctx.RoConfig());
  r0.reset();    
  
  Process (R"([
    {"name": "domob", "move": {"ca": {"d": {"rid": [1]}}}}
  ])");  
  
  EXPECT_EQ (tbl2.GetById(1)->GetOwner(), "");
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

TEST_F (CoinOperationTests, PutFighterForSaleAndThenBuy)
{
  proto::ConfigData& cfg = const_cast <proto::ConfigData&>(*ctx.RoConfig());
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->AddBalance (250 + cfg.params().starting_crystals());  
	
  Process (R"([
    {"name": "domob", "move": {"a": {"x": 42, "init": {"address": "CGUpAcjsb6MDktSYg8yRDxDutr7FhWtdWC"}}}}
  ])");
 
  ctx.SetTimestamp(1000);
 
  UpdateState ("[]");
  
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
  
  ExpectBalances ({{"domob", 240  + cfg.params().starting_crystals()}});
  
  Process (R"([
    {"name": "andy", "move": {"a": {"x": 42, "init": {"address": "psss"}}}}
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
  proto::ConfigData& cfg = const_cast <proto::ConfigData&>(*ctx.RoConfig());
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->AddBalance (250 + cfg.params().starting_crystals()); 	
	
  Process (R"([
    {"name": "domob", "move": {"a": {"x": 42, "init": {"address": "CGUpAcjsb6MDktSYg8yRDxDutr7FhWtdWC"}}}}
  ])");
  
  UpdateState ("[]");
  
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
  
  ExpectBalances ({{"domob", 240  + cfg.params().starting_crystals()}});

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
  proto::ConfigData& cfg = const_cast <proto::ConfigData&>(*ctx.RoConfig());
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->AddBalance (250 + cfg.params().starting_crystals()); 	
  
  Process (R"([
    {"name": "domob", "move": {"a": {"x": 42, "init": {"address": "CGUpAcjsb6MDktSYg8yRDxDutr7FhWtdWC"}}}}
  ])");
  
  UpdateState ("[]");
  
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
  
  ExpectBalances ({{"domob", 250 + cfg.params().starting_crystals()}});
}

TEST_F (CoinOperationTests, PurchaseStuff)
{
  proto::ConfigData& cfg = const_cast <proto::ConfigData&>(*ctx.RoConfig());
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->AddBalance (250 + cfg.params().starting_crystals()); 		
	
  Process (R"([
    {"name": "domob", "move": {"a": {"x": 42, "init": {"address": "CGUpAcjsb6MDktSYg8yRDxDutr7FhWtdWC"}}}}
  ])");
  
  UpdateState ("[]");
  
  ProcessWithDevPayment (R"([{
    "name": "domob",
    "move":
      {
        "pc": "T1"
      }
  }])", 0.14 * COIN);

  ExpectBalances ({{"domob", 350 + cfg.params().starting_crystals()}});

  Process (R"([
    {"name": "domob", "move": {"pg": "ca3378db-cd54-e514-7ae8-23705781bb9d"}}
  ])");

  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Goodie_PressureCooker_1"), 3);
  a.reset();
  
  ExpectBalances ({{"domob", 250 + cfg.params().starting_crystals()}});
  
  ProcessWithDevPayment (R"([{
    "name": "domob",
    "move":
      {
        "pc": "T1"
      }
  }])", 0.14 * COIN);

  ProcessWithDevPayment (R"([{
    "name": "domob",
    "move":
      {
        "pc": "T1"
      }
  }])", 0.14 * COIN);

  ExpectBalances ({{"domob", 450 + cfg.params().starting_crystals()}});
  
    Process (R"([
    {"name": "domob", "move": {"pgb": "645a9411-d8f1-3e24-aa12-f3f79665dca2"}}
  ])");
  
  ExpectBalances ({{"domob", 275 + cfg.params().starting_crystals()}});
  
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Goodie_PressureCooker_1"), 6);
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Goodie_Espresso_1"), 3);
  a.reset();

    Process (R"([
    {"name": "domob", "move": {"ps": "36adece2-8ed9-3114-db6e-24fa8c494fa5"}}
  ])");  
  
  ExpectBalances ({{"domob", 225 + cfg.params().starting_crystals()}});
  
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Goodie_PressureCooker_1"), 6);
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Goodie_Espresso_1"), 3);
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Sweetener_R6"), 1);
  a.reset();  
  
    Process (R"([
    {"name": "domob", "move": {"ps": "36adece2-8ed9-3114-db6e-24fa8c494fa5,2"}}
  ])"); 

  ExpectBalances ({{"domob", 125 + cfg.params().starting_crystals()}});  
  
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Goodie_PressureCooker_1"), 6);
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Goodie_Espresso_1"), 3);
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Sweetener_R6"), 3);
  a.reset();    
}

TEST_F (CoinOperationTests, RRNameProperly)
{
  Process (R"([
    {"name": "domob", "move": {"a": {"x": 42, "init": {"address": "CGUpAcjsb6MDktSYg8yRDxDutr7FhWtdWC"}}}}
  ])");
 
  ctx.SetTimestamp(1000);
 
  UpdateState ("[]");
  
  auto ft = tbl3.GetById(4, ctx.RoConfig());
  ASSERT_TRUE (ft != nullptr);
  ft->MutableProto().set_isaccountbound(false);
  std::string fighterName = ft->GetProto().name();
  ft.reset();  
  
  ProcessWithDevPayment (R"([{
    "name": "domob",
    "move":
      {
        "nr": 4
      }
  }])", 0.14 * COIN);

  UpdateState ("[]");

  ft = tbl3.GetById(4, ctx.RoConfig());
  EXPECT_NE(fighterName, ft->GetProto().name());
  fighterName = ft->GetProto().name();
  ft.reset(); 
  
  ProcessWithDevPayment (R"([{
    "name": "domob",
    "move":
      {
        "nr": 4
      }
  }])", 0.14 * COIN);

  UpdateState ("[]");  
  
  ft = tbl3.GetById(4, ctx.RoConfig());
  EXPECT_EQ(fighterName, ft->GetProto().name());
  ft.reset();   
}

TEST_F (CoinOperationTests, BuyCrustalAndThenRerollNameProperly)
{
  proto::ConfigData& cfg = const_cast <proto::ConfigData&>(*ctx.RoConfig());
  
  Process (R"([
    {"name": "domob", "move": {"a": {"x": 42, "init": {"address": "CGUpAcjsb6MDktSYg8yRDxDutr7FhWtdWC"}}}}
  ])");
 
  ctx.SetTimestamp(1000);
 
  UpdateState ("[]");
  
  auto ft = tbl3.GetById(4, ctx.RoConfig());
  ASSERT_TRUE (ft != nullptr);
  ft->MutableProto().set_isaccountbound(false);
  std::string fighterName = ft->GetProto().name();
  ft.reset();  
  
 ProcessWithDevPayment (R"([{
    "name": "domob",
    "move":
      {
        "pc": "T1"
      }
  }])", 0.14 * COIN);
  
  ProcessWithDevPayment (R"([{
    "name": "domob",
    "move":
      {
        "pc": "T1"
      }
  }])", 0.14 * COIN);    
  
  ProcessWithDevPayment (R"([{
    "name": "domob",
    "move":
      {
        "nr": 4
      }
  }])", 0.35 * COIN);

  UpdateState ("[]");

  ft = tbl3.GetById(4, ctx.RoConfig());
  EXPECT_NE(fighterName, ft->GetProto().name());
  ft.reset(); 
  
   ExpectBalances ({{"domob", 200 + cfg.params().starting_crystals()}});
}

TEST_F (CoinOperationTests, RerollNameFailsAsNoFunds)
{
  Process (R"([
    {"name": "domob", "move": {"a": {"x": 42, "init": {"address": "CGUpAcjsb6MDktSYg8yRDxDutr7FhWtdWC"}}}}
  ])");
 
  ctx.SetTimestamp(1000);
 
  UpdateState ("[]");
  
  auto ft = tbl3.GetById(4, ctx.RoConfig());
  ASSERT_TRUE (ft != nullptr);
  ft->MutableProto().set_isaccountbound(false);
  std::string fighterName = ft->GetProto().name();
  ft.reset();  
  
  ProcessWithDevPayment (R"([{
    "name": "domob",
    "move":
      {
        "nr": 4
      }
  }])", 0.11 * COIN);

  UpdateState ("[]");

  ft = tbl3.GetById(4, ctx.RoConfig());
  EXPECT_EQ(fighterName, ft->GetProto().name());
  ft.reset(); 
}

TEST_F (CoinOperationTests, PurchaseCrystalsWrongData)
{
  proto::ConfigData& cfg = const_cast <proto::ConfigData&>(*ctx.RoConfig());
	
  Process (R"([
    {"name": "domob", "move": {"a": {"x": 42, "init": {"address": "CGUpAcjsb6MDktSYg8yRDxDutr7FhWtdWC"}}}}
  ])");
  
  UpdateState ("[]");
  
  ProcessWithDevPayment (R"([{
    "name": "domob",
    "move":
      {
        "pc": "N8"
      }
  }])", 0.14 * COIN);

  ExpectBalances ({{"domob", 0 + cfg.params().starting_crystals()}});
}

TEST_F (CoinOperationTests, PurchaseCrystalsInsufficientCoin)
{
  proto::ConfigData& cfg = const_cast <proto::ConfigData&>(*ctx.RoConfig());
  
  Process (R"([
    {"name": "domob", "move": {"a": {"x": 42, "init": {"address": "CGUpAcjsb6MDktSYg8yRDxDutr7FhWtdWC"}}}}
  ])");
  
  UpdateState ("[]");
  
  ProcessWithDevPayment (R"([{
    "name": "domob",
    "move":
      {
        "pc": "N8"
      }
  }])", 0.28 * COIN);

  ExpectBalances ({{"domob", 0 + cfg.params().starting_crystals()}});
}

TEST_F (CoinOperationTests, PurchaseCrystalsTwiceInARow)
{
  proto::ConfigData& cfg = const_cast <proto::ConfigData&>(*ctx.RoConfig());
  
  Process (R"([
    {"name": "domob", "move": {"a": {"x": 42, "init": {"address": "CGUpAcjsb6MDktSYg8yRDxDutr7FhWtdWC"}}}}
  ])");
  
  UpdateState ("[]");
  
  ProcessWithDevPayment (R"([{
    "name": "domob",
    "move":
      {
        "pc": "T1"
      }
  }])", 0.14 * COIN);
  
  ProcessWithDevPayment (R"([{
    "name": "domob",
    "move":
      {
        "pc": "T1"
      }
  }])", 0.14 * COIN);  

  ExpectBalances ({{"domob", 200 + cfg.params().starting_crystals()}});
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
/* ************************************************************************** */

} // anonymous namespace
} // namespace pxd

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
#include "moveprocessor.hpp"

#include "jsonutils.hpp"
#include "params.hpp"
#include "testutils.hpp"

#include "database/dbtest.hpp"
#include "database/xayaplayer.hpp"


#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <json/json.h>

#include <string>
#include <vector>

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

private:

  TestRandom rnd;

protected:

  ContextForTesting ctx;

  XayaPlayersTable xayaplayers;
  
  PXLogicTests () : xayaplayers(db)
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
   
   Process (R"([
    {"name": "domob", "move": {"ca": {"r": {"rid": "5864a19b-c8c0-2d34-eaef-9455af0baf2c", "fid": ""}}}}
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
  
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Recipe_Common_FirstRecipe"), 0);
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Common_Gumdrop"), 0);
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Common_Icing"), 0);
  
}   

TEST_F (ValidateStateTests, RecepieInstanceRevertIfFullRoster)
{
  xayaplayers.CreateNew ("domob", ctx.RoConfig())->AddBalance (100);
   
  proto::ConfigData& cfg = const_cast <proto::ConfigData&>(*ctx.RoConfig());
  cfg.mutable_params()->set_max_fighter_inventory_amount(0); 
   
  Process (R"([
    {"name": "domob", "move": {"ca": {"r": {"rid": "5864a19b-c8c0-2d34-eaef-9455af0baf2c", "fid": ""}}}}
  ])");  

  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 1);
  a.reset ();

  UpdateState ("[]");
  
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  EXPECT_EQ (a->CollectInventoryFighters(ctx.RoConfig()).size(), 0);
  
  EXPECT_EQ (a->GetBalance (), 100);
  
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Recipe_Common_FirstRecipe"), 1);
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Common_Gumdrop"), 1);
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Common_Icing"), 1);
} 

TEST_F (ValidateStateTests, RecepieInstanceFailWithMissingIngridients)
{
   xayaplayers.CreateNew ("domob", ctx.RoConfig())->AddBalance (100);
   auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());

   a->GetInventory().SetFungibleCount("Common_Icing", 0);
   a.reset();
   
   Process (R"([
    {"name": "domob", "move": {"ca": {"r": {"rid": "5864a19b-c8c0-2d34-eaef-9455af0baf2c", "fid": ""}}}}
   ])");  

  a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetOngoingsSize (), 0);
  a.reset ();

  UpdateState ("[]");
  
  a = xayaplayers.GetByName ("domob", ctx.RoConfig());

  
  EXPECT_EQ (a->GetBalance (), 100);
  
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Recipe_Common_FirstRecipe"), 1);
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Common_Gumdrop"), 1);
  EXPECT_EQ (a->GetInventory().GetFungibleCount("Common_Icing"), 0);
  
}  
    
}

} // namespace pxd

/*
    GSP for the TF blockchain game
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

#include "xayaplayer.hpp"

#include "dbtest.hpp"
#include "proto/roconfig.hpp"

#include <gtest/gtest.h>

namespace pxd
{
namespace
{

class XayaPlayerTests : public DBTestWithSchema
{

protected:

  XayaPlayersTable tbl;
  std::unique_ptr<pxd::RoConfig> cfg;

  XayaPlayerTests ()
    : tbl(db)
  {
      cfg = std::make_unique<pxd::RoConfig> (xaya::Chain::REGTEST);
  }

};

TEST_F (XayaPlayerTests, DefaultData)
{
  auto a = tbl.CreateNew ("foobar", *cfg);
  EXPECT_EQ (a->GetName (), "foobar");
  EXPECT_FALSE (a->IsInitialised ());
  EXPECT_EQ (a->GetRole (), PlayerRole::INVALID);
  EXPECT_EQ (a->GetBalance (), 0);
}

TEST_F (XayaPlayerTests, UpdateFields)
{
  auto a = tbl.CreateNew ("foobar", *cfg);
  a->SetRole (PlayerRole::PLAYER);
  a.reset ();

  a = tbl.GetByName ("foobar", *cfg);
  EXPECT_TRUE (a->IsInitialised ());
  EXPECT_EQ (a->GetRole (), PlayerRole::PLAYER);
}

TEST_F (XayaPlayerTests, UpdateProto)
{
  auto a = tbl.CreateNew ("foobar", *cfg);
  a.reset ();
  a = tbl.GetByName ("foobar", *cfg);
  EXPECT_EQ (a->GetName (), "foobar");
}

TEST_F (XayaPlayerTests, Balance)
{
  auto a = tbl.CreateNew ("foobar", *cfg);
  a->AddBalance (10);
  a->AddBalance (20);
  a.reset ();

  a = tbl.GetByName ("foobar", *cfg);
  EXPECT_EQ (a->GetBalance (), 30);
  a->AddBalance (-30);
  EXPECT_EQ (a->GetBalance (), 0);
  a.reset ();
}

using XayaPlayersTableTests = XayaPlayerTests;

TEST_F (XayaPlayersTableTests, CreateAlreadyExisting)
{
  tbl.CreateNew ("domob", *cfg);
  EXPECT_DEATH (tbl.CreateNew ("domob", *cfg), "exists already");
}

TEST_F (XayaPlayersTableTests, QueryAll)
{
  tbl.CreateNew ("uninit", *cfg);
  tbl.CreateNew ("foo", *cfg)->SetRole (PlayerRole::PLAYER);

  auto res = tbl.QueryAll ();

  ASSERT_TRUE (res.Step ());
  auto a = tbl.GetFromResult (res, *cfg);
  EXPECT_EQ (a->GetName (), "foo");
  EXPECT_EQ (a->GetRole (), PlayerRole::PLAYER);

  ASSERT_TRUE (res.Step ());
  a = tbl.GetFromResult (res, *cfg);
  EXPECT_EQ (a->GetName (), "uninit");
  EXPECT_EQ (a->GetRole (), PlayerRole::INVALID);
  
  EXPECT_EQ (a->GetFTUEState (), FTUEState::Intro);
  
  

  ASSERT_FALSE (res.Step ());
}

TEST_F (XayaPlayersTableTests, QueryInitialised)
{
  tbl.CreateNew ("foo", *cfg)->SetRole (PlayerRole::PLAYER);
  tbl.CreateNew ("uninit", *cfg);
  auto a = tbl.CreateNew ("bar", *cfg);
  a->SetRole (PlayerRole::ROLEADMIN);
  a->SetFTUEState (FTUEState::SecondRecipe);
  a.reset ();

  auto res = tbl.QueryInitialised ();

  ASSERT_TRUE (res.Step ());
  a = tbl.GetFromResult (res, *cfg);
  EXPECT_EQ (a->GetName (), "bar");
  EXPECT_EQ (a->GetRole (), PlayerRole::ROLEADMIN);
  EXPECT_EQ (a->GetFTUEState (), FTUEState::SecondRecipe);

  ASSERT_TRUE (res.Step ());
  a = tbl.GetFromResult (res, *cfg);
  EXPECT_EQ (a->GetName (), "foo");
  EXPECT_EQ (a->GetRole (), PlayerRole::PLAYER);

  ASSERT_FALSE (res.Step ());
}

TEST_F (XayaPlayersTableTests, GetByName)
{
  tbl.CreateNew ("foo", *cfg);

  auto h = tbl.GetByName ("foo", *cfg);
  ASSERT_TRUE (h != nullptr);
  EXPECT_EQ (h->GetName (), "foo");
  EXPECT_FALSE (h->IsInitialised ());

  EXPECT_TRUE (tbl.GetByName ("bar", *cfg) == nullptr);
}

TEST_F (XayaPlayersTableTests, Inventory)
{
  auto h = tbl.CreateNew ("domob", *cfg);
  h->SetRole( PlayerRole::PLAYER);
  h->GetInventory ().SetFungibleCount ("candy", 10);
  h.reset ();

  h = tbl.GetByName ("domob", *cfg);
  EXPECT_EQ (h->GetInventory ().GetFungibleCount ("candy"), 10);
  h->GetInventory ().SetFungibleCount ("candy", 0);
  
  h->GetInventory ().SetFungibleCount ("Recipe_Common_FirstRecipe", 0);
  h->GetInventory ().SetFungibleCount ("Common_Gumdrop", 0);
  h->GetInventory ().SetFungibleCount ("Common_Icing", 0);
  h.reset ();

  h = tbl.GetByName ("domob", *cfg);
  EXPECT_TRUE (h->GetInventory ().IsEmpty ());
  h.reset ();
}

TEST_F (XayaPlayersTableTests, StartingItems)
{
  const RoConfig& cfg2 = *cfg;
  auto h = tbl.CreateNew ("domob", *cfg);
  EXPECT_EQ (h->GetInventory ().GetFungibleCount ("Recipe_Common_FirstRecipe"), 1);
  EXPECT_EQ (h->GetInventory ().GetFungibleCount ("Common_Gumdrop"), 1);
  EXPECT_EQ (h->GetInventory ().GetFungibleCount ("Common_Icing"), 1);  
  EXPECT_EQ (h->GetBalance (), cfg2->params().starting_crystals());
  h.reset ();

  h = tbl.GetByName ("domob", *cfg);
  EXPECT_EQ (h->GetInventory ().GetFungibleCount ("Recipe_Common_FirstRecipe"), 1);
  EXPECT_EQ (h->GetInventory ().GetFungibleCount ("Common_Gumdrop"), 1);
  EXPECT_EQ (h->GetInventory ().GetFungibleCount ("Common_Icing"), 1);
  EXPECT_EQ (h->GetBalance (), cfg2->params().starting_crystals());
}

TEST_F (XayaPlayersTableTests, Prestige)
{
  const RoConfig& cfg2 = *cfg;
  auto h = tbl.CreateNew ("domob", *cfg);
  h.reset ();

  h = tbl.GetByName ("domob", *cfg);
  EXPECT_EQ (h->GetPresitge(), cfg2->params().base_prestige());
}

} // anonymous namespace
} // namespace pxd

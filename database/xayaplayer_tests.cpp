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
#include "recipe.hpp"

#include <xayautil/random.hpp>
#include <xayautil/hash.hpp>

#include "dbtest.hpp"
#include "proto/roconfig.hpp"

#include <gtest/gtest.h>

namespace pxd
{
 
/**
 * Random instance that seeds itself on construction from a fixed test seed.
 */
class TestRandom : public xaya::Random
{

  public:

  TestRandom ()
  {
    xaya::SHA256 seed;
    seed << "test seed";
    Seed (seed.Finalise ());
  } 

}; 

namespace
{

class XayaPlayerTests : public DBTestWithSchema
{

protected:

  XayaPlayersTable tbl;
  RecipeInstanceTable tbl2;
  TestRandom rnd;
  
  std::unique_ptr<pxd::RoConfig> cfg;

  XayaPlayerTests ()
    : tbl(db), tbl2(db)
  {
      cfg = std::make_unique<pxd::RoConfig> (xaya::Chain::REGTEST);
  }

};

TEST_F (XayaPlayerTests, DefaultData)
{
  auto a = tbl.CreateNew ("foobar", *cfg, rnd);
  EXPECT_EQ (a->GetName (), "foobar");

  EXPECT_EQ (a->GetRole (), PlayerRole::INVALID);
  EXPECT_EQ (a->GetBalance (), 50);
}

TEST_F (XayaPlayerTests, UpdateFields)
{
  auto a = tbl.CreateNew ("foobar", *cfg, rnd);
  a->SetRole (PlayerRole::PLAYER);
  a.reset ();

  a = tbl.GetByName ("foobar", *cfg);

  EXPECT_EQ (a->GetRole (), PlayerRole::PLAYER);
}

TEST_F (XayaPlayerTests, UpdateProto)
{
  auto a = tbl.CreateNew ("foobar", *cfg, rnd);
  a.reset ();
  a = tbl.GetByName ("foobar", *cfg);
  EXPECT_EQ (a->GetName (), "foobar");
}

TEST_F (XayaPlayerTests, Balance)
{
  auto a = tbl.CreateNew ("foobar", *cfg, rnd);
  a->AddBalance (10);
  a->AddBalance (20);
  a.reset ();

  a = tbl.GetByName ("foobar", *cfg);
  EXPECT_EQ (a->GetBalance (), 80);
  a->AddBalance (-30);
  EXPECT_EQ (a->GetBalance (), 50);
  a.reset ();
}

using XayaPlayersTableTests = XayaPlayerTests;

TEST_F (XayaPlayersTableTests, CreateAlreadyExisting)
{
  tbl.CreateNew ("domob", *cfg, rnd);
  EXPECT_DEATH (tbl.CreateNew ("domob", *cfg, rnd), "exists already");
}

TEST_F (XayaPlayersTableTests, QueryAll)
{
  tbl.CreateNew ("uninit", *cfg, rnd);
  tbl.CreateNew ("foo", *cfg, rnd)->SetRole (PlayerRole::PLAYER);

  auto res = tbl.QueryAll ();

  ASSERT_TRUE (res.Step ());
  auto a = tbl.GetFromResult (res, *cfg);
  EXPECT_EQ (a->GetName (), "foo");
  EXPECT_EQ (a->GetRole (), PlayerRole::PLAYER);

  ASSERT_TRUE (res.Step ());
  a = tbl.GetFromResult (res, *cfg);
  EXPECT_EQ (a->GetName (), "uninit");
  EXPECT_EQ (a->GetRole (), PlayerRole::INVALID);

  ASSERT_FALSE (res.Step ());
}

TEST_F (XayaPlayersTableTests, QueryInitialised)
{
  tbl.CreateNew ("foo", *cfg, rnd)->SetRole (PlayerRole::PLAYER);
  tbl.CreateNew ("uninit", *cfg, rnd);
  auto a = tbl.CreateNew ("bar", *cfg, rnd);
  a->SetRole (PlayerRole::ROLEADMIN);
  a.reset ();

  auto res = tbl.QueryInitialised ();

  ASSERT_TRUE (res.Step ());
  a = tbl.GetFromResult (res, *cfg);
  EXPECT_EQ (a->GetName (), "bar");
  EXPECT_EQ (a->GetRole (), PlayerRole::ROLEADMIN);

  ASSERT_TRUE (res.Step ());
  a = tbl.GetFromResult (res, *cfg);
  EXPECT_EQ (a->GetName (), "foo");
  EXPECT_EQ (a->GetRole (), PlayerRole::PLAYER);

  ASSERT_FALSE (res.Step ());
}

TEST_F (XayaPlayersTableTests, GetByName)
{
  tbl.CreateNew ("foo", *cfg, rnd);

  auto h = tbl.GetByName ("foo", *cfg);
  ASSERT_TRUE (h != nullptr);
  EXPECT_EQ (h->GetName (), "foo");

  EXPECT_TRUE (tbl.GetByName ("bar", *cfg) == nullptr);
}

TEST_F (XayaPlayersTableTests, Inventory)
{
  auto h = tbl.CreateNew ("domob", *cfg, rnd);
  h->SetRole( PlayerRole::PLAYER);
  h->GetInventory ().SetFungibleCount ("candy", 10);
  h.reset ();

  h = tbl.GetByName ("domob", *cfg);
  EXPECT_EQ (h->GetInventory ().GetFungibleCount ("candy"), 10);
  h->GetInventory ().SetFungibleCount ("candy", 0);
  
  h->GetInventory ().SetFungibleCount ("Common_Nonpareil", 0);
  h->GetInventory ().SetFungibleCount ("Common_Icing", 0);
  h->GetInventory ().SetFungibleCount ("Common_Fizzy Powder", 0);
  h->GetInventory ().SetFungibleCount ("Common_Chocolate Chip", 0);
  h->GetInventory ().SetFungibleCount ("Common_Candy Cane", 0);
  h.reset ();

  h = tbl.GetByName ("domob", *cfg);
  EXPECT_TRUE (h->GetInventory ().IsEmpty ());
  h.reset ();
}

TEST_F (XayaPlayersTableTests, StartingItems)
{
  const RoConfig& cfg2 = *cfg;
  auto h = tbl.CreateNew ("domob", *cfg, rnd);
  
  EXPECT_EQ (h->GetInventory ().GetFungibleCount ("Common_Nonpareil"), 10);
  EXPECT_EQ (h->GetInventory ().GetFungibleCount ("Common_Icing"), 20);  
  EXPECT_EQ (h->GetInventory ().GetFungibleCount ("Common_Fizzy Powder"), 9);  
  EXPECT_EQ (h->GetInventory ().GetFungibleCount ("Common_Chocolate Chip"), 20);  
  EXPECT_EQ (h->GetInventory ().GetFungibleCount ("Common_Candy Cane"), 9);  
  EXPECT_EQ (h->GetBalance (), cfg2->params().starting_crystals());
  h.reset ();
  
  auto r0 = tbl2.GetById(1);  
  EXPECT_EQ (r0->GetProto().name(), "First Recipe");  
  r0.reset();

  h = tbl.GetByName ("domob", *cfg);  
  r0 = tbl2.GetById(1);  
  EXPECT_EQ (r0->GetProto().name(), "First Recipe");  
  
  EXPECT_EQ (h->GetInventory ().GetFungibleCount ("Common_Nonpareil"), 10);
  EXPECT_EQ (h->GetInventory ().GetFungibleCount ("Common_Icing"), 20);  
  EXPECT_EQ (h->GetInventory ().GetFungibleCount ("Common_Fizzy Powder"), 9);  
  EXPECT_EQ (h->GetInventory ().GetFungibleCount ("Common_Chocolate Chip"), 20);  
  EXPECT_EQ (h->GetInventory ().GetFungibleCount ("Common_Candy Cane"), 9); 
  EXPECT_EQ (h->GetBalance (), cfg2->params().starting_crystals());
}

TEST_F (XayaPlayersTableTests, Prestige)
{
  //const RoConfig& cfg2 = *cfg;
  //auto h = tbl.CreateNew ("domob", *cfg, rnd);
  //h.reset ();

  //h = tbl.GetByName ("domob", *cfg);
  //todo EXPECT_EQ (h->GetPresitge(), cfg2->params().base_prestige());
}

} // anonymous namespace
} // namespace pxd

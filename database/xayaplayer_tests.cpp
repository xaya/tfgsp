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

#include <gtest/gtest.h>

namespace pxd
{
namespace
{

class XayaPlayerTests : public DBTestWithSchema
{

protected:

  XayaPlayersTable tbl;

  XayaPlayerTests ()
    : tbl(db)
  {}

};

TEST_F (XayaPlayerTests, DefaultData)
{
  auto a = tbl.CreateNew ("foobar");
  EXPECT_EQ (a->GetName (), "foobar");
  EXPECT_FALSE (a->IsInitialised ());
  EXPECT_EQ (a->GetRole (), PlayerRole::INVALID);
  EXPECT_EQ (a->GetBalance (), 0);
}

TEST_F (XayaPlayerTests, UpdateFields)
{
  auto a = tbl.CreateNew ("foobar");
  a->SetRole (PlayerRole::PLAYER);
  a.reset ();

  a = tbl.GetByName ("foobar");
  EXPECT_TRUE (a->IsInitialised ());
  EXPECT_EQ (a->GetRole (), PlayerRole::PLAYER);
}

TEST_F (XayaPlayerTests, UpdateProto)
{
  auto a = tbl.CreateNew ("foobar");
  a.reset ();
  a = tbl.GetByName ("foobar");
  EXPECT_EQ (a->GetName (), "foobar");
}

TEST_F (XayaPlayerTests, Balance)
{
  auto a = tbl.CreateNew ("foobar");
  a->AddBalance (10);
  a->AddBalance (20);
  a.reset ();

  a = tbl.GetByName ("foobar");
  EXPECT_EQ (a->GetBalance (), 30);
  a->AddBalance (-30);
  EXPECT_EQ (a->GetBalance (), 0);
  a.reset ();
}

using XayaPlayersTableTests = XayaPlayerTests;

TEST_F (XayaPlayersTableTests, CreateAlreadyExisting)
{
  tbl.CreateNew ("domob");
  EXPECT_DEATH (tbl.CreateNew ("domob"), "exists already");
}

TEST_F (XayaPlayersTableTests, QueryAll)
{
  tbl.CreateNew ("uninit");
  tbl.CreateNew ("foo")->SetRole (PlayerRole::PLAYER);

  auto res = tbl.QueryAll ();

  ASSERT_TRUE (res.Step ());
  auto a = tbl.GetFromResult (res);
  EXPECT_EQ (a->GetName (), "foo");
  EXPECT_EQ (a->GetRole (), PlayerRole::PLAYER);

  ASSERT_TRUE (res.Step ());
  a = tbl.GetFromResult (res);
  EXPECT_EQ (a->GetName (), "uninit");
  EXPECT_EQ (a->GetRole (), PlayerRole::INVALID);
  
  EXPECT_EQ (a->GetFTUEState (), FTUEState::Intro);
  
  

  ASSERT_FALSE (res.Step ());
}

TEST_F (XayaPlayersTableTests, QueryInitialised)
{
  tbl.CreateNew ("foo")->SetRole (PlayerRole::PLAYER);
  tbl.CreateNew ("uninit");
  auto a = tbl.CreateNew ("bar");
  a->SetRole (PlayerRole::ROLEADMIN);
  a->SetFTUEState (FTUEState::SecondRecipe);
  a.reset ();

  auto res = tbl.QueryInitialised ();

  ASSERT_TRUE (res.Step ());
  a = tbl.GetFromResult (res);
  EXPECT_EQ (a->GetName (), "bar");
  EXPECT_EQ (a->GetRole (), PlayerRole::ROLEADMIN);
  EXPECT_EQ (a->GetFTUEState (), FTUEState::SecondRecipe);

  ASSERT_TRUE (res.Step ());
  a = tbl.GetFromResult (res);
  EXPECT_EQ (a->GetName (), "foo");
  EXPECT_EQ (a->GetRole (), PlayerRole::PLAYER);

  ASSERT_FALSE (res.Step ());
}

TEST_F (XayaPlayersTableTests, GetByName)
{
  tbl.CreateNew ("foo");

  auto h = tbl.GetByName ("foo");
  ASSERT_TRUE (h != nullptr);
  EXPECT_EQ (h->GetName (), "foo");
  EXPECT_FALSE (h->IsInitialised ());

  EXPECT_TRUE (tbl.GetByName ("bar") == nullptr);
}

} // anonymous namespace
} // namespace pxd

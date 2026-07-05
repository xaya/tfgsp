/*
    GSP for the tf blockchain game
    Copyright (C) 2026  Autonomous Worlds Ltd

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

#include "battlelosses.hpp"

#include "dbtest.hpp"

#include <gtest/gtest.h>

namespace pxd
{
namespace
{

class BattleLossesTests : public DBTestWithSchema
{
protected:
  BattleLossesTable losses;
  BattleLossesTests () : losses(db) {}
};

TEST_F (BattleLossesTests, AddAndQueryForOwner)
{
  losses.Add ("domob", 42, "Sour Gummi Brawler", 0, "andy", 7);
  losses.Add ("domob", 43, "Choco Punch", 1, "andy", 7);
  losses.Add ("bob", 44, "Nougat Knight", 0, "andy", 7);

  auto res = losses.QueryForOwner ("domob");
  ASSERT_TRUE (res.Step ());
  EXPECT_EQ (res.Get<BattleLossResult::fighterid> (), 42);
  EXPECT_EQ (res.Get<BattleLossResult::name> (), "Sour Gummi Brawler");
  EXPECT_EQ (res.Get<BattleLossResult::outcome> (), 0);
  EXPECT_EQ (res.Get<BattleLossResult::opponent> (), "andy");
  EXPECT_EQ (res.Get<BattleLossResult::tournamentid> (), 7);
  ASSERT_TRUE (res.Step ());
  EXPECT_EQ (res.Get<BattleLossResult::fighterid> (), 43);
  EXPECT_EQ (res.Get<BattleLossResult::outcome> (), 1);
  EXPECT_FALSE (res.Step ());
}

} // anonymous namespace
} // namespace pxd

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

#include "params.hpp"

#include "dbtest.hpp"

#include <gtest/gtest.h>

namespace pxd
{
namespace
{

class GameParamsTests : public DBTestWithSchema
{
protected:
  GameParams gp;
  GameParamsTests () : gp(db) {}
};

TEST_F (GameParamsTests, SeededDefaultsPresent)
{
  EXPECT_EQ (gp.GetParam ("rarest_recipe_drop_divisor"), 4);
  EXPECT_EQ (gp.GetParam ("tournament_loss_kills_enabled"), 1);
  EXPECT_EQ (gp.GetParam ("tournament_capture_pct"), 128);
  EXPECT_EQ (gp.GetParam ("tournament_max_captures"), 3);
}

TEST_F (GameParamsTests, SetUpdatesValue)
{
  gp.SetParam ("rarest_recipe_drop_divisor", 8);
  EXPECT_EQ (gp.GetParam ("rarest_recipe_drop_divisor"), 8);
}

TEST_F (GameParamsTests, HasAndRemove)
{
  EXPECT_TRUE (gp.HasParam ("tournament_capture_pct"));
  gp.RemoveParam ("tournament_capture_pct");
  EXPECT_FALSE (gp.HasParam ("tournament_capture_pct"));
}

TEST_F (GameParamsTests, GetUnsetCheckFails)
{
  EXPECT_DEATH (gp.GetParam ("nonexistent_param"), "unset parameter");
}

} // anonymous namespace
} // namespace pxd

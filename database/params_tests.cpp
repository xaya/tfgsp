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

TEST_F (GameParamsTests, ScaleDurationGeometricCurveAtDefaultPct)
{
  EXPECT_EQ (ScaleDuration (15, 1, 100), 15);        /* sw1 = 1.0x, unchanged */
  EXPECT_EQ (ScaleDuration (350, 5, 100), 1324);     /* sw5: 350*969/256 */
  EXPECT_EQ (ScaleDuration (700, 7, 100), 5157);     /* sw7: 700*1886/256 */
  EXPECT_EQ (ScaleDuration (2800, 10, 100), 56000);  /* sw10 = 20.0x */
}

TEST_F (GameParamsTests, ScaleDurationPctTunesIntensity)
{
  EXPECT_EQ (ScaleDuration (2800, 10, 30), 16800);   /* ~superblock (5s) setting: 20x*0.3 = 6x */
  EXPECT_EQ (ScaleDuration (2800, 10, 200), 112000); /* harsher */
}

TEST_F (GameParamsTests, ScaleDurationClampsSweetnessAndFloors)
{
  EXPECT_EQ (ScaleDuration (100, 0, 100), 100);      /* sweetness<1 -> sw1 (1.0x) */
  EXPECT_EQ (ScaleDuration (100, 99, 100), 2000);    /* sweetness>10 -> sw10 (20x) */
  EXPECT_EQ (ScaleDuration (1, 1, 1), 1);            /* result floored to >= 1 */
  EXPECT_EQ (ScaleDuration (0, 5, 100), 0);          /* non-positive base passes through */
}

TEST_F (GameParamsTests, ScaleDurationNoOverflowAtMax)
{
  EXPECT_EQ (ScaleDuration (2800, 10, 1000), 560000); /* 2800*5120*1000/25600, needs uint64 intermediate */
}

TEST_F (GameParamsTests, CookQualityMapsToTiers)
{
  EXPECT_EQ (CookQualityToSweetness (1), 1u);   /* Common */
  EXPECT_EQ (CookQualityToSweetness (2), 4u);   /* Uncommon */
  EXPECT_EQ (CookQualityToSweetness (3), 7u);   /* Rare */
  EXPECT_EQ (CookQualityToSweetness (4), 10u);  /* Epic */
  EXPECT_EQ (CookQualityToSweetness (0), 1u);   /* None -> floor tier */
}

TEST_F (GameParamsTests, DurationScalePctSeededToDefault)
{
  EXPECT_EQ (gp.GetParam ("duration_scale_pct"), 100);
}

TEST_F (GameParamsTests, ScaledDurationUsesSeededParam)
{
  EXPECT_EQ (gp.ScaledDuration (2800, 10), 56000); /* default pct 100 -> 20x */
  gp.SetParam ("duration_scale_pct", 30);
  EXPECT_EQ (gp.ScaledDuration (2800, 10), 16800); /* live retune -> 6x */
}

} // anonymous namespace
} // namespace pxd

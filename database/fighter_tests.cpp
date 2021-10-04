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

#include "fighter.hpp"
#include "recipe.hpp"
#include <xayautil/hash.hpp>

#include "dbtest.hpp"
#include "proto/roconfig.hpp"

#include <gtest/gtest.h>

namespace pxd
{
namespace
{

/** 
 We have this class in testutils.hpp already, but to solve
 cross-reference issue, I just made copypast in there for now,
 can refactor stuff later on
*/
class TestRandomS : public xaya::Random
{

public:

  TestRandomS ()
  {
    xaya::SHA256 seed;
    seed << "test seed 1";
    Seed (seed.Finalise ());
  }
};


class FighterTests : public DBTestWithSchema
{


protected:

  TestRandomS rnd;
  FighterTable tbl;
  RecipeInstanceTable tbl2;
  std::unique_ptr<pxd::RoConfig> cfg;

  FighterTests ()
    : tbl(db), tbl2(db)
  {
      cfg = std::make_unique<pxd::RoConfig> (xaya::Chain::REGTEST);
  }

};

TEST_F (FighterTests, GetsArmor)
{
    auto r0 = tbl2.CreateNew("orvald", "5864a19b-c8c0-2d34-eaef-9455af0baf2c", *cfg);
    const auto id0 = r0->GetId ();
    r0.reset();
    
    auto f1 = tbl.CreateNew ("orvald", id0, *cfg, rnd);
    bool resComp = f1->GetProto().armorpieces().size() > 0;
    EXPECT_EQ (resComp, true);
}

TEST_F (FighterTests, InsureDataIsRandom)
{
   auto r0 = tbl2.CreateNew("orvald", "5864a19b-c8c0-2d34-eaef-9455af0baf2c", *cfg);
   auto r1 = tbl2.CreateNew("orvald", "5864a19b-c8c0-2d34-eaef-9455af0baf2c", *cfg);
   
   const auto id0 = r0->GetId ();
   r0.reset();
    
   const auto id1 = r1->GetId ();
   r1.reset();
    
   auto f1 = tbl.CreateNew ("orvald", id0, *cfg, rnd);
   auto f2 = tbl.CreateNew ("orvald", id1, *cfg, rnd);
   bool resComp = f1->GetProto().armorpieces()[0].armortype() != f2->GetProto().armorpieces()[0].armortype();
   EXPECT_EQ (resComp, true);
}

TEST_F (FighterTests, FormulaChecks)
{
    auto r0 = tbl2.CreateNew("orvald", "5864a19b-c8c0-2d34-eaef-9455af0baf2c", *cfg);
    
    const auto id0 = r0->GetId ();
    r0.reset();    
    
    auto f1 = tbl.CreateNew ("orvald", id0, *cfg, rnd);
    
    EXPECT_EQ (f1->GetRatingFromQuality(1), 1000);
    EXPECT_EQ (f1->GetRatingFromQuality(2), 1100);
    EXPECT_EQ (f1->GetRatingFromQuality(3), 1200);
    
    EXPECT_EQ (f1->GetProto().quality(), 1);
    EXPECT_EQ (f1->GetProto().sweetness(), 1);
}

} // anonymous namespace
} // namespace pxd

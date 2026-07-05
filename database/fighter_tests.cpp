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

/* The exchange/quality columns are pure projections of the proto, re-bound on every flush; read them
   back with a raw SELECT to prove they cannot drift from the blob. */
struct ExchangeColsResult : public Database::ResultType
{
  RESULT_COLUMN (int64_t, price, 1);
  RESULT_COLUMN (int64_t, expire, 2);
  RESULT_COLUMN (int64_t, quality, 3);
  RESULT_COLUMN (int64_t, sweetness, 4);
};

TEST_F (FighterTests, ExchangeColumnsProjectProto)
{
  auto r0 = tbl2.CreateNew ("orvald", "5864a19b-c8c0-2d34-eaef-9455af0baf2c", *cfg);
  const auto rid = r0->GetId ();
  r0.reset ();

  auto f = tbl.CreateNew ("orvald", rid, *cfg, rnd);
  const auto fid = f->GetId ();
  f->SetStatus (pxd::FighterStatus::Exchange);
  f->MutableProto ().set_exchangeprice (500);
  f->MutableProto ().set_exchangeexpire (12345);
  f->MutableProto ().set_quality (4);
  f->MutableProto ().set_sweetness (7);
  f.reset (); // destructor flushes the row

  auto stmt = db.Prepare (
      "SELECT `price`,`expire`,`quality`,`sweetness` FROM `fighters` WHERE `id` = ?1");
  stmt.Bind (1, fid);
  auto res = stmt.Query<ExchangeColsResult> ();
  ASSERT_TRUE (res.Step ());
  EXPECT_EQ (res.Get<ExchangeColsResult::price> (), 500);
  EXPECT_EQ (res.Get<ExchangeColsResult::expire> (), 12345);
  EXPECT_EQ (res.Get<ExchangeColsResult::quality> (), 4);
  EXPECT_EQ (res.Get<ExchangeColsResult::sweetness> (), 7);

  /* Re-flip to Available (a sale/expiry) and lower price: the column must follow the proto. */
  auto f2 = tbl.GetById (fid, *cfg);
  f2->SetStatus (pxd::FighterStatus::Available);
  f2->MutableProto ().set_exchangeprice (0);
  f2.reset ();

  auto stmt2 = db.Prepare ("SELECT `price` FROM `fighters` WHERE `id` = ?1");
  stmt2.Bind (1, fid);
  auto res2 = stmt2.Query<ExchangeColsResult> ();
  ASSERT_TRUE (res2.Step ());
  EXPECT_EQ (res2.Get<ExchangeColsResult::price> (), 0);
}

} // anonymous namespace
} // namespace pxd

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

/* Lists a freshly-cooked fighter for `owner` at `price`/`expire`/`quality`, returns its id. */
static Database::IdT
MakeListing (FighterTable& tbl, RecipeInstanceTable& tbl2, pxd::RoConfig& cfg,
             xaya::Random& rnd, const std::string& owner,
             int price, int expire, int quality)
{
  auto r0 = tbl2.CreateNew (owner, "5864a19b-c8c0-2d34-eaef-9455af0baf2c", cfg);
  const auto rid = r0->GetId ();
  r0.reset ();
  auto f = tbl.CreateNew (owner, rid, cfg, rnd);
  const auto fid = f->GetId ();
  f->SetStatus (pxd::FighterStatus::Exchange);
  f->MutableProto ().set_exchangeprice (price);
  f->MutableProto ().set_exchangeexpire (expire);
  f->MutableProto ().set_quality (quality);
  f.reset ();
  return fid;
}

TEST_F (FighterTests, QueryExchangeSortsFiltersPagesAndCounts)
{
  const auto a = MakeListing (tbl, tbl2, *cfg, rnd, "alice", 300, 1000, 3);
  const auto b = MakeListing (tbl, tbl2, *cfg, rnd, "alice", 100, 1000, 5);
  const auto c = MakeListing (tbl, tbl2, *cfg, rnd, "bob",   200, 1000, 4);
  // A listed-then-delisted fighter must never appear. (AutoIds are global across recipes+fighters, so
  // never guess ids — use MakeListing's returned fighter id.)
  const auto gone = MakeListing (tbl, tbl2, *cfg, rnd, "bob", 999, 1000, 9);
  { auto x = tbl.GetById (gone, *cfg); x->SetStatus (pxd::FighterStatus::Available); x.reset (); }

  // price ascending: b(100) < c(200) < a(300)
  {
    ExchangeQuery q; q.sort = ExchangeSort::Price; q.ascending = true;
    std::vector<Database::IdT> got;
    auto res = tbl.QueryExchange (q);
    while (res.Step ()) got.push_back (tbl.GetFromResult (res, *cfg)->GetId ());
    EXPECT_EQ (got, (std::vector<Database::IdT>{b, c, a}));
    EXPECT_EQ (tbl.CountExchange (q), 3);
  }

  // quality descending, exclude bob: b(q5) then a(q3); c is bob's.
  {
    ExchangeQuery q; q.sort = ExchangeSort::Quality; q.ascending = false; q.excludeOwner = "bob";
    std::vector<Database::IdT> got;
    auto res = tbl.QueryExchange (q);
    while (res.Step ()) got.push_back (tbl.GetFromResult (res, *cfg)->GetId ());
    EXPECT_EQ (got, (std::vector<Database::IdT>{b, a}));
    EXPECT_EQ (tbl.CountExchange (q), 2);
  }

  // price filter maxPrice=200 → b, c ; total counts filter, not the page.
  {
    ExchangeQuery q; q.sort = ExchangeSort::Price; q.ascending = true; q.maxPrice = 200;
    EXPECT_EQ (tbl.CountExchange (q), 2);
    q.limit = 1; q.offset = 0;
    std::vector<Database::IdT> got;
    auto res = tbl.QueryExchange (q);
    while (res.Step ()) got.push_back (tbl.GetFromResult (res, *cfg)->GetId ());
    EXPECT_EQ (got, (std::vector<Database::IdT>{b})); // page 1 of 2
  }
}

/* The marketplace WHERE is built by two lockstep helpers (append placeholders / bind values); a
   single-filter test never binds past ?1, so this activates three filters at once (int, int, string)
   — any append/bind order divergence changes the result set or mis-binds a type. */
TEST_F (FighterTests, QueryExchangeMultiFilterLockstep)
{
  const auto excluded = MakeListing (tbl, tbl2, *cfg, rnd, "alice", 100, 1000, 5); // owner excluded
  const auto keep     = MakeListing (tbl, tbl2, *cfg, rnd, "bob",   200, 1000, 5); // matches all 3
  const auto tooDear  = MakeListing (tbl, tbl2, *cfg, rnd, "bob",   300, 1000, 5); // price > maxPrice
  const auto tooLowQ  = MakeListing (tbl, tbl2, *cfg, rnd, "bob",   150, 1000, 2); // quality < minQuality
  (void) excluded; (void) tooDear; (void) tooLowQ;

  ExchangeQuery q;
  q.sort = ExchangeSort::Price; q.ascending = true;
  q.minQuality = 4;          // ?1
  q.maxPrice = 250;          // ?2
  q.excludeOwner = "alice";  // ?3 (string — a transposition with an int filter would mis-bind)

  std::vector<Database::IdT> got;
  auto res = tbl.QueryExchange (q);
  while (res.Step ()) got.push_back (tbl.GetFromResult (res, *cfg)->GetId ());
  EXPECT_EQ (got, (std::vector<Database::IdT>{keep}));
  EXPECT_EQ (tbl.CountExchange (q), 1);

  // Offset past the end returns an empty page, no crash (same filters still bound correctly).
  q.limit = 50; q.offset = 50;
  auto res2 = tbl.QueryExchange (q);
  EXPECT_FALSE (res2.Step ());
}

TEST_F (FighterTests, QueryExpiredListingsSeeksOnlyExpired)
{
  const auto soon = MakeListing (tbl, tbl2, *cfg, rnd, "alice", 100, /*expire*/ 50, 3);
  const auto late = MakeListing (tbl, tbl2, *cfg, rnd, "bob",   100, /*expire*/ 500, 3);
  (void) late;

  // At height 100: only `soon` (expire 50 < 100) is returned; strict `<` preserves the flip semantics.
  std::vector<Database::IdT> got;
  auto res = tbl.QueryExpiredListings (100);
  while (res.Step ()) got.push_back (tbl.GetFromResult (res, *cfg)->GetId ());
  EXPECT_EQ (got, (std::vector<Database::IdT>{soon}));

  // At height 50: nothing (50 < 50 is false).
  std::vector<Database::IdT> none;
  auto res2 = tbl.QueryExpiredListings (50);
  while (res2.Step ()) none.push_back (tbl.GetFromResult (res2, *cfg)->GetId ());
  EXPECT_TRUE (none.empty ());
}

} // anonymous namespace
} // namespace pxd

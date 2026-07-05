# Exchange Scale Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the Treatfighter fighter marketplace scale to tens of thousands of live listings — O(page) at the GSP, on the wire, and in the DOM — with server-side sort/filter/paging and event-driven (idle-cheap) per-block listing expiry.

**Architecture:** Denormalise four proto fields (`price`, `expire`, `quality`, `sweetness`) into indexed columns on the `fighters` table, bound from the proto at the single write point so they cannot drift. Rewrite `getexchange` into a paginated/sorted/filtered query returning one page + a total, and rewrite the per-block `CheckFightersForSale` into an indexed `expire < height` seek. On the frontend, drop the per-block full-market poll slice in favour of an on-demand `useExchangeMarket(query, blockTick)` hook plus sort/filter/paging UI.

**Tech Stack:** GSP — C++17, SQLite via libxayagame's `Database`/`Result<T>` wrapper, protobuf, libjson-rpc-cpp (`jsonrpcstub`), GoogleTest, autotools; built only inside the `tf-builder:local` Docker toolchain. Frontend — Next.js 16 (App Router), TypeScript, viem, vitest, Playwright.

## Global Constraints

- **Never rename `recepie`/`recepies`** (misspelling is load-bearing across the wire/DB/tests) — copy verbatim.
- **GSP cannot be built on the native host.** Build + test only in Docker via the `tf-builder:local` image, `--network none`, building in an internal copy; the `make check` gate must pass. See `docs`/memory `tf-build-and-test`.
- **Golden replay:** regenerate `src/goldenreplay.golden.json` only with `GOLDEN_REGEN=1 ./goldenreplay_tests` **after reviewing the diff** — never blind-regen.
- **Consensus/determinism (money-critical):** the new columns are pure projections of the proto, bound at every `INSERT OR REPLACE`; sort columns come from a fixed C++ whitelist (identifiers are **never** interpolated from RPC input — only bound values are); preserve the exact existing expiry semantics (`expire < height`, strict `<`).
- **Frontend:** no new dependencies (Next 16 + viem + three.js only); keep exchange params in lockstep with the roconfig (`src/data/exchange.ts` mirrors `proto/roconfig`); tests are vitest, pure-function-first (the repo has no React test harness).
- **Fork e2e (Task 11):** pin a **non-192.168** Docker subnet for the fork stack; the Alchemy RPC key lives ONLY in the gitignored `forked-evm-testing/.env` and must never be committed or printed; `dev_address` = `0x25763F408461ddc66d8955d22963815DEA6d8722` (public test payout).

**Repo roots:** GSP paths are under `/home/acoloss/treatfighter/tfgsp-polygon` (branch `polygon-rewrite`). Frontend paths are under `/home/acoloss/treatfighter/tf-frontend` (branch `master`).

**Build/test helper (GSP).** All GSP tasks build + test in the toolchain container. The canonical invocation (from `tf-build-and-test`):

```bash
docker run --rm --network none \
  -v /home/acoloss/treatfighter/tfgsp-polygon:/src-ro:ro \
  -v /tmp/tf-gsp-scratch:/scratch \
  --entrypoint bash tf-builder:local -c '
    set -e
    cp -a /src-ro /build && cd /build
    make distclean >/dev/null 2>&1 || true
    ./autogen.sh && ./configure && make -j"$(nproc)"
    # then either: make check
    # or a single gtest: ./src/tests --gtest_filter="Foo.Bar"  /  ./database/tests --gtest_filter="Foo.Bar"
  '
```

Where a step says "run `<gtest filter>`", append the corresponding `./src/tests --gtest_filter=...` or `./database/tests --gtest_filter=...` after `make -j`. Where it says "run `make check`", append `make check`.

---

## GSP (Tasks 1–6) — lands first; the frontend depends on the new RPC

### Task 1: Schema columns + drift-proof bind-at-flush

**Files:**
- Modify: `database/schema.sql` (fighters table + indexes)
- Modify: `database/fighter.cpp:414-427` (the `~FighterInstance` INSERT)
- Test: `database/fighter_tests.cpp`

**Interfaces:**
- Produces: four new `fighters` columns `price`, `expire`, `quality`, `sweetness` (all `INTEGER NOT NULL DEFAULT 0`), each equal to the corresponding proto scalar on every persisted row; composite indexes `fighters_exchange_{price,quality,sweetness,expire}` on `(status, <col>)`.

- [ ] **Step 1: Write the failing test** — append to `database/fighter_tests.cpp` (inside the existing `namespace pxd { namespace {` block, after the `FighterTests` fixture). It creates a fighter, lists it (proto fields set), flushes, and asserts the raw columns match the proto:

```cpp
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
```

- [ ] **Step 2: Run the test to verify it fails** — build + `./database/tests --gtest_filter='FighterTests.ExchangeColumnsProjectProto'`. Expected: FAIL — the build errors on unknown columns `price`/`expire`/... (the columns don't exist yet), or the SELECT throws "no such column".

- [ ] **Step 3: Add the columns + indexes** — in `database/schema.sql`, replace the fighters table + status index block (lines 116-139) so the table ends with the four new columns and the composite indexes are added:

```sql
-- Data for the fighters in the game.
CREATE TABLE IF NOT EXISTS `fighters` (

  -- The fighter ID, which is assigned based on libxayagame's AutoIds.
  `id` INTEGER PRIMARY KEY,

 -- The Xaya name that owns this fighter (and is thus allowed to send
  -- moves for it).
  `owner` TEXT NOT NULL,

  -- Additional data encoded as a Fighter protocol buffer.
  `proto` BLOB NOT NULL,

  -- The status (as integer corresponding to the FighterStatus enum in C++).
  -- Is always Available on start, when fresh fighter is created.
  `status` INTEGER NULL,

  -- Denormalised projections of proto fields, re-bound from the proto on EVERY
  -- INSERT OR REPLACE (database/fighter.cpp ~FighterInstance) so they can never
  -- drift from the blob. They exist purely to make the marketplace query
  -- (getexchange) and the per-block listing-expiry maintenance index-driven
  -- instead of full scans. (SQLite generated columns can't parse a protobuf, so
  -- these are explicit columns bound in C++.)
  `price` INTEGER NOT NULL DEFAULT 0,
  `expire` INTEGER NOT NULL DEFAULT 0,
  `quality` INTEGER NOT NULL DEFAULT 0,
  `sweetness` INTEGER NOT NULL DEFAULT 0
);

-- "Which fighters need a per-block status flip" -- the two per-block maintenance
-- scans (free Transfiguring fighters back to Available; expire for-sale listings)
-- act only on Exchange/Transfiguring rows, so this index lets them seek just those
-- instead of scanning the whole (unbounded) fighters table every block (DEF2).
CREATE INDEX IF NOT EXISTS `fighters_by_status`
  ON `fighters` (`status`);

-- Marketplace query + event-driven listing expiry: each sort/filter column is
-- indexed with a leading `status` so getexchange (WHERE status=Exchange ORDER BY
-- <col>) and CheckFightersForSale (WHERE status=Exchange AND expire<height) are
-- index-driven and O(page)/O(expiring), never O(all listings).
CREATE INDEX IF NOT EXISTS `fighters_exchange_price`
  ON `fighters` (`status`, `price`);
CREATE INDEX IF NOT EXISTS `fighters_exchange_quality`
  ON `fighters` (`status`, `quality`);
CREATE INDEX IF NOT EXISTS `fighters_exchange_sweetness`
  ON `fighters` (`status`, `sweetness`);
CREATE INDEX IF NOT EXISTS `fighters_exchange_expire`
  ON `fighters` (`status`, `expire`);
```

- [ ] **Step 4: Bind the columns at the single flush point** — in `database/fighter.cpp`, replace the `~FighterInstance` INSERT block (lines 414-427) with:

```cpp
      auto stmt = db.Prepare (R"(
        INSERT OR REPLACE INTO `fighters`
          (`id`,`owner`, `proto`, `status`, `price`, `expire`, `quality`, `sweetness`)
          VALUES
          (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8)
      )");

      BindFieldValues (stmt);                 // ?1 id, ?2 owner
      stmt.BindProto (3, data);               // ?3 proto (the single source of truth)
      BindStatusParameter (stmt, 4, status);  // ?4 status
      /* ?5..?8: pure projections of the proto, so the marketplace query + expiry
         maintenance can be index-driven. Re-derived here on every flush → cannot
         drift from the blob. */
      const auto& pb = data.Get ();
      stmt.Bind (5, static_cast<int64_t> (pb.exchangeprice ()));
      stmt.Bind (6, static_cast<int64_t> (pb.exchangeexpire ()));
      stmt.Bind (7, static_cast<int64_t> (pb.quality ()));
      stmt.Bind (8, static_cast<int64_t> (pb.sweetness ()));
      stmt.Execute ();

      return;
```

- [ ] **Step 5: Run the test to verify it passes** — build + `./database/tests --gtest_filter='FighterTests.ExchangeColumnsProjectProto'`. Expected: PASS.

- [ ] **Step 6: Run the database suite to catch regressions** — `./database/tests`. Expected: all PASS (existing fighter/schema tests unaffected — `FighterResult` maps by column name, so the extra columns are ignored by existing reads).

- [ ] **Step 7: Commit**

```bash
cd /home/acoloss/treatfighter/tfgsp-polygon
git add database/schema.sql database/fighter.cpp database/fighter_tests.cpp
git commit -m "feat(gsp): denormalise exchange/quality fighter columns for indexed marketplace queries

Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>"
```

---

### Task 2: `FighterTable::QueryExchange` + `CountExchange` (paginated/sorted/filtered)

**Files:**
- Modify: `database/fighter.hpp` (add `ExchangeSort`, `ExchangeQuery`, two method decls)
- Modify: `database/fighter.cpp` (add whitelist helper + two method defs)
- Test: `database/fighter_tests.cpp`

**Interfaces:**
- Consumes: the `price`/`expire`/`quality`/`sweetness` columns from Task 1.
- Produces:
  - `enum class ExchangeSort { Price, Quality, Sweetness, Expire };`
  - `struct ExchangeQuery { ExchangeSort sort=Price; bool ascending=true; int64_t minQuality=-1,maxQuality=-1,minPrice=-1,maxPrice=-1,maxAffordable=-1; std::string excludeOwner; int64_t limit=50, offset=0; };` (`-1`/empty = unset)
  - `Database::Result<FighterResult> FighterTable::QueryExchange (const ExchangeQuery& q);`
  - `int64_t FighterTable::CountExchange (const ExchangeQuery& q);`

- [ ] **Step 1: Write the failing test** — append to `database/fighter_tests.cpp`. Seeds mixed listings and asserts order, filter, paging, count, and self-exclusion. Add a small helper first, then the test:

```cpp
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
```

*(Delete the unused `ids` lambda when writing — it is shown only to flag that `GetFromResult` needs the real `*cfg`, not a null.)*

- [ ] **Step 2: Run the test to verify it fails** — build + `./database/tests --gtest_filter='FighterTests.QueryExchangeSortsFiltersPagesAndCounts'`. Expected: FAIL — `QueryExchange`/`CountExchange`/`ExchangeQuery` undeclared (compile error).

- [ ] **Step 3: Declare the query types + methods** — in `database/fighter.hpp`, add above `class FighterTable` (near `struct FighterResult`):

```cpp
/** Sort key for the marketplace query; each maps to an indexed `fighters` column. */
enum class ExchangeSort { Price, Quality, Sweetness, Expire };

/** A validated marketplace query. Numeric filters use -1 = "unset"; excludeOwner "" = no exclusion. */
struct ExchangeQuery
{
  ExchangeSort sort = ExchangeSort::Price;
  bool ascending = true;
  int64_t minQuality = -1, maxQuality = -1;
  int64_t minPrice = -1, maxPrice = -1;
  int64_t maxAffordable = -1;   // price <= this (the "affordable only" filter)
  std::string excludeOwner;     // e.g. the caller — hide their own listings from the market
  int64_t limit = 50, offset = 0;
};
```

  and inside `class FighterTable` (after `QueryForStatus`, line ~357):

```cpp
  /**
   * One page of on-exchange listings matching `q`, index-backed via the
   * fighters_exchange_* composite indexes (touches only status=Exchange rows).
   */
  Database::Result<FighterResult> QueryExchange (const ExchangeQuery& q);

  /**
   * Count of on-exchange listings matching `q`'s FILTERS (ignores limit/offset) —
   * for the marketplace page total. Uses the same WHERE as QueryExchange.
   */
  int64_t CountExchange (const ExchangeQuery& q);
```

- [ ] **Step 4: Implement the methods** — in `database/fighter.cpp`, add near `QueryForStatus` (after line 511). The sort column comes from a fixed whitelist (never interpolated input); every value is bound:

```cpp
namespace
{

/** Whitelisted column name for a sort key (identifiers can't be bound, so map — never interpolate
 *  RPC input into SQL). */
const char*
ExchangeSortColumn (ExchangeSort s)
{
  switch (s)
    {
    case ExchangeSort::Price:     return "price";
    case ExchangeSort::Quality:   return "quality";
    case ExchangeSort::Sweetness: return "sweetness";
    case ExchangeSort::Expire:    return "expire";
    }
  LOG (FATAL) << "unknown ExchangeSort";
}

/** Appends "WHERE status=Exchange [AND filters]" to `sql`, numbering placeholders from `nextParam`
 *  upward (advancing it). BindExchangeWhere binds the same values in the same order. */
void
AppendExchangeWhere (const ExchangeQuery& q, std::string& sql, int& nextParam)
{
  sql += " WHERE `status` = "
       + std::to_string (static_cast<int> (pxd::FighterStatus::Exchange));
  if (q.minQuality >= 0)        sql += " AND `quality` >= ?" + std::to_string (nextParam++);
  if (q.maxQuality >= 0)        sql += " AND `quality` <= ?" + std::to_string (nextParam++);
  if (q.minPrice >= 0)          sql += " AND `price` >= ?"   + std::to_string (nextParam++);
  if (q.maxPrice >= 0)          sql += " AND `price` <= ?"   + std::to_string (nextParam++);
  if (q.maxAffordable >= 0)     sql += " AND `price` <= ?"   + std::to_string (nextParam++);
  if (!q.excludeOwner.empty ()) sql += " AND `owner` != ?"   + std::to_string (nextParam++);
}

/** Binds the WHERE params in the SAME order AppendExchangeWhere emits them, starting at ?1. Returns
 *  the next free placeholder index. */
int
BindExchangeWhere (Database::Statement& stmt, const ExchangeQuery& q)
{
  int p = 1;
  if (q.minQuality >= 0)        stmt.Bind (p++, q.minQuality);
  if (q.maxQuality >= 0)        stmt.Bind (p++, q.maxQuality);
  if (q.minPrice >= 0)          stmt.Bind (p++, q.minPrice);
  if (q.maxPrice >= 0)          stmt.Bind (p++, q.maxPrice);
  if (q.maxAffordable >= 0)     stmt.Bind (p++, q.maxAffordable);
  if (!q.excludeOwner.empty ()) stmt.Bind (p++, q.excludeOwner);
  return p;
}

} // anonymous namespace

Database::Result<FighterResult>
FighterTable::QueryExchange (const ExchangeQuery& q)
{
  std::string sql = "SELECT * FROM `fighters`";
  int n = 1;
  AppendExchangeWhere (q, sql, n);
  sql += std::string (" ORDER BY `") + ExchangeSortColumn (q.sort) + "` "
       + (q.ascending ? "ASC" : "DESC") + ", `id` ASC";
  const int limitParam = n++;
  const int offsetParam = n++;
  sql += " LIMIT ?" + std::to_string (limitParam)
       + " OFFSET ?" + std::to_string (offsetParam);

  auto stmt = db.Prepare (sql);
  int p = BindExchangeWhere (stmt, q);
  stmt.Bind (p++, q.limit);
  stmt.Bind (p++, q.offset);
  return stmt.Query<FighterResult> ();
}

int64_t
FighterTable::CountExchange (const ExchangeQuery& q)
{
  std::string sql = "SELECT COUNT(*) AS `cnt` FROM `fighters`";
  int n = 1;
  AppendExchangeWhere (q, sql, n);
  auto stmt = db.Prepare (sql);
  BindExchangeWhere (stmt, q);
  auto res = stmt.Query<CountResult> ();   // CountResult already defined for CountForOwner
  CHECK (res.Step ());
  return res.Get<CountResult::cnt> ();
}
```

Note: `CountResult` is the anonymous-namespace struct already defined in `fighter.cpp` for `CountForOwner` (`RESULT_COLUMN (int64_t, cnt, 1)`). Its definition sits **above** `CountForOwner` (line ~516); move the `CountExchange` definition to **after** that struct so it is in scope (or leave `CountExchange` after `CountForOwner`).

- [ ] **Step 5: Run the test to verify it passes** — build + `./database/tests --gtest_filter='FighterTests.QueryExchangeSortsFiltersPagesAndCounts'`. Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add database/fighter.hpp database/fighter.cpp database/fighter_tests.cpp
git commit -m "feat(gsp): FighterTable::QueryExchange/CountExchange (paged, sorted, filtered marketplace query)

Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>"
```

---

### Task 3: `GameStateJson::Exchange(request)` rewrite

**Files:**
- Modify: `src/gamestatejson.hpp:106` (Exchange decl)
- Modify: `src/gamestatejson.cpp:796-822` (Exchange impl)
- Test: `src/gamestatejson_tests.cpp`

**Interfaces:**
- Consumes: `FighterTable::QueryExchange`/`CountExchange` + `ExchangeQuery`/`ExchangeSort` (Task 2).
- Produces: `Json::Value GameStateJson::Exchange (const Json::Value& request);` returning `{ fighters: [...page...], total: <int> }`. `request` is an object with optional keys `limit, offset, sort, order, minquality, maxquality, minprice, maxprice, affordablefor, excludeowner`.

- [ ] **Step 1: Write the failing test** — append to `src/gamestatejson_tests.cpp` (the `XayaPlayersJsonTests` fixture has `tbl2` = `FighterTable`, `tbl3` = `RecipeInstanceTable`, `converter`, `ctx`, `*cfg`, `rnd`):

```cpp
TEST_F (XayaPlayersJsonTests, ExchangePagesSortsAndExcludesOwner)
{
  auto mk = [&] (const std::string& owner, int price) -> int
  {
    auto r0 = tbl3.CreateNew (owner, "5864a19b-c8c0-2d34-eaef-9455af0baf2c", *cfg);
    const auto rid = r0->GetId (); r0.reset ();
    auto f = tbl2.CreateNew (owner, rid, *cfg, rnd);
    const int fid = f->GetId ();
    f->SetStatus (pxd::FighterStatus::Exchange);
    f->MutableProto ().set_exchangeprice (price);
    f->MutableProto ().set_exchangeexpire (99999);
    f.reset ();
    return fid;
  };
  const int cheap = mk ("alice", 100);
  const int mid   = mk ("bob",   200);
  const int dear  = mk ("alice", 300);

  ctx.SetHeight (10);

  Json::Value req (Json::objectValue);
  req["sort"] = "price"; req["order"] = "asc"; req["limit"] = 2; req["offset"] = 0;
  req["excludeowner"] = "carol"; // excludes nobody here
  const Json::Value page = converter.Exchange (req);

  ASSERT_EQ (page["fighters"].size (), 2u);            // limit honoured
  EXPECT_EQ (page["fighters"][0]["id"].asInt (), cheap); // price asc
  EXPECT_EQ (page["fighters"][1]["id"].asInt (), mid);
  EXPECT_EQ (page["total"].asInt (), 3);              // total counts ALL matches, not the page
  (void) dear;

  // excludeowner=alice → only bob's listing remains.
  req["excludeowner"] = "alice"; req["limit"] = 10;
  const Json::Value page2 = converter.Exchange (req);
  ASSERT_EQ (page2["fighters"].size (), 1u);
  EXPECT_EQ (page2["fighters"][0]["id"].asInt (), mid);
  EXPECT_EQ (page2["total"].asInt (), 1);
}
```

- [ ] **Step 2: Run the test to verify it fails** — build + `./src/tests --gtest_filter='XayaPlayersJsonTests.ExchangePagesSortsAndExcludesOwner'`. Expected: FAIL — `converter.Exchange(req)` — no matching `Exchange(const Json::Value&)` (compile error, current decl takes no args).

- [ ] **Step 3: Update the declaration** — in `src/gamestatejson.hpp`, change line 106:

```cpp
  Json::Value Exchange (const Json::Value& request);
```

- [ ] **Step 4: Rewrite the implementation** — in `src/gamestatejson.cpp`, replace the whole `Exchange()` body (lines 796-822) with:

```cpp
namespace
{

ExchangeSort
ParseExchangeSort (const Json::Value& v)
{
  const std::string s = v.isString () ? v.asString () : "";
  if (s == "quality")   return ExchangeSort::Quality;
  if (s == "sweetness") return ExchangeSort::Sweetness;
  if (s == "expire")    return ExchangeSort::Expire;
  return ExchangeSort::Price;   // default + unknown
}

int64_t
OptInt (const Json::Value& r, const char* key, int64_t dflt)
{
  return (r.isMember (key) && r[key].isInt ()) ? r[key].asInt64 () : dflt;
}

} // anonymous namespace

Json::Value
GameStateJson::Exchange (const Json::Value& request)
{
  const Json::Value r = request.isObject () ? request : Json::Value (Json::objectValue);

  ExchangeQuery q;
  q.sort = ParseExchangeSort (r["sort"]);
  q.ascending = !(r.isMember ("order") && r["order"].isString ()
                  && r["order"].asString () == "desc");
  q.minQuality    = OptInt (r, "minquality", -1);
  q.maxQuality    = OptInt (r, "maxquality", -1);
  q.minPrice      = OptInt (r, "minprice", -1);
  q.maxPrice      = OptInt (r, "maxprice", -1);
  q.maxAffordable = OptInt (r, "affordablefor", -1);
  if (r.isMember ("excludeowner") && r["excludeowner"].isString ())
    q.excludeOwner = r["excludeowner"].asString ();

  int64_t limit = OptInt (r, "limit", 50);
  q.limit = limit < 1 ? 1 : (limit > 100 ? 100 : limit);   // server-side hard cap
  const int64_t offset = OptInt (r, "offset", 0);
  q.offset = offset < 0 ? 0 : offset;

  FighterTable tbl (db);
  Json::Value fghtrarr (Json::arrayValue);
  {
    auto res = tbl.QueryExchange (q);
    while (res.Step ())
      {
        auto h = tbl.GetFromResult (res, ctx.RoConfig ());
        fghtrarr.append (Convert<FighterInstance> (*h));
        h.reset ();
      }
  } // close the page cursor before the COUNT statement

  Json::Value out (Json::objectValue);
  out["fighters"] = fghtrarr;
  out["total"]    = IntToJson (tbl.CountExchange (q));
  return out;
}
```

Note: `Convert<FighterInstance>` is unchanged — its per-fighter `ongoings` subquery is now bounded to one page (≤100 rows), so the spec's "skip the ongoings subquery" micro-optimisation is intentionally **deferred as YAGNI** (it would add a stateful flag to a hot path for a negligible page-scale gain). The `salehistory` clause and `QueryAll()` are gone.

- [ ] **Step 5: Run the test to verify it passes** — build + `./src/tests --gtest_filter='XayaPlayersJsonTests.ExchangePagesSortsAndExcludesOwner'`. Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add src/gamestatejson.hpp src/gamestatejson.cpp src/gamestatejson_tests.cpp
git commit -m "feat(gsp): getexchange serves one paged/sorted/filtered page + total (drops full-table scan + salehistory dump)

Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>"
```

---

### Task 4: RPC wiring (`tf-rpc.json` + `PXRpcServer`)

**Files:**
- Modify: `src/rpc-stubs/tf-rpc.json:36-40` (getexchange params)
- Modify: `src/pxrpcserver.hpp:143` (override signature)
- Modify: `src/pxrpcserver.cpp:138-147` (impl)

**Interfaces:**
- Consumes: `GameStateJson::Exchange(const Json::Value&)` (Task 3).
- Produces: RPC `getexchange` taking one named object param `request`; the generated stub declares `getexchange (const Json::Value& request)`.

- [ ] **Step 1: Change the RPC param spec** — in `src/rpc-stubs/tf-rpc.json`, replace the getexchange entry (lines 36-40):

```json
  {
    "name": "getexchange",
    "params": { "request": {} },
    "returns": {}
  },
```

`jsonrpcstub` turns the single object-valued named param into `getexchange (const Json::Value& request)` (same convention that gives `getfueldata` its `const Json::Value&` params).

- [ ] **Step 2: Update the override declaration** — in `src/pxrpcserver.hpp`, change line 143:

```cpp
  Json::Value getexchange (const Json::Value& request) override;
```

- [ ] **Step 3: Update the implementation** — in `src/pxrpcserver.cpp`, replace `getexchange` (lines 138-147):

```cpp
Json::Value
PXRpcServer::getexchange (const Json::Value& request)
{
  LOG (INFO) << "RPC method called: getexchange";
  return logic.GetCustomStateData (game,
    [request] (GameStateJson& gsj)
      {
        return gsj.Exchange (request);
      });
}
```

- [ ] **Step 4: Build to verify the stub regenerates + the override matches** — run the full build (the `rpc-stubs/pxrpcserverstub.h` rule regenerates from `tf-rpc.json`; a signature mismatch would fail to compile as an unimplemented pure virtual). Command: build with `make -j"$(nproc)"` only. Expected: compiles clean (no "cannot allocate an object of abstract type PXRpcServer").

- [ ] **Step 5: Commit**

```bash
git add src/rpc-stubs/tf-rpc.json src/pxrpcserver.hpp src/pxrpcserver.cpp
git commit -m "feat(gsp): getexchange RPC takes a query object (limit/offset/sort/filters)

Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>"
```

---

### Task 5: Event-driven listing expiry (`CheckFightersForSale`)

**Files:**
- Modify: `database/fighter.hpp` (add `QueryExpiredListings` decl)
- Modify: `database/fighter.cpp` (add def)
- Modify: `src/logic_tournament.cpp:335-360` (`CheckFightersForSale`)
- Test: `database/fighter_tests.cpp`

**Interfaces:**
- Consumes: the `(status, expire)` index from Task 1.
- Produces: `Database::Result<FighterResult> FighterTable::QueryExpiredListings (unsigned height);` — Exchange rows with `expire < height`, ordered by id.

- [ ] **Step 1: Write the failing test** — append to `database/fighter_tests.cpp` (reuse the `MakeListing` helper from Task 2):

```cpp
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
```

- [ ] **Step 2: Run the test to verify it fails** — build + `./database/tests --gtest_filter='FighterTests.QueryExpiredListingsSeeksOnlyExpired'`. Expected: FAIL — `QueryExpiredListings` undeclared.

- [ ] **Step 3: Declare + implement the query** — in `database/fighter.hpp`, inside `class FighterTable` (after `QueryForStatus`):

```cpp
  /**
   * On-exchange listings whose window has closed (`expire` < height), ordered by id. Backed by the
   * `(status, expire)` index so an idle block with nothing expiring is a single empty index probe,
   * not a scan of the live-listing set.
   */
  Database::Result<FighterResult> QueryExpiredListings (unsigned height);
```

  and in `database/fighter.cpp` (after `QueryForStatus`):

```cpp
Database::Result<FighterResult>
FighterTable::QueryExpiredListings (unsigned height)
{
  auto stmt = db.Prepare (
      "SELECT * FROM `fighters` WHERE `status` = "
      + std::to_string (static_cast<int> (FighterStatus::Exchange))
      + " AND `expire` < ?1 ORDER BY `id`");
  stmt.Bind (1, static_cast<int64_t> (height));
  return stmt.Query<FighterResult> ();
}
```

- [ ] **Step 4: Run the test to verify it passes** — build + `./database/tests --gtest_filter='FighterTests.QueryExpiredListingsSeeksOnlyExpired'`. Expected: PASS.

- [ ] **Step 5: Rewrite `CheckFightersForSale` to use the index** — in `src/logic_tournament.cpp`, replace the body (lines 335-360):

```cpp
void PXLogic::CheckFightersForSale(Database& db, const Context& ctx)
{
    FighterTable fighters(db);

    /* Event-driven expiry (exchange-scale): the (status, expire) index returns ONLY the listings whose
       window has closed, so an idle block does ~zero work here — no scan of the live-listing set.
       Collect ids first, then flip -- never mutate `status` while the status-keyed cursor is open. */
    std::vector<Database::IdT> expired;
    {
      auto res = fighters.QueryExpiredListings (ctx.Height ());
      while (res.Step ())
        expired.push_back (fighters.GetFromResult (res, ctx.RoConfig ())->GetId ());
    }

    for (const auto id : expired)
    {
      auto fighterDb = fighters.GetById (id, ctx.RoConfig ());
      fighterDb->SetStatus (pxd::FighterStatus::Available);
    }
}
```

The set of fighters flipped, and the heights at which they flip, are **identical** to the old `QueryForStatus(Exchange)` + `exchangeexpire() < ctx.Height()` scan (same strict `<`), so state transitions are byte-for-byte unchanged.

- [ ] **Step 6: Run the existing exchange/logic tests to confirm no behavioural change** — build + `./src/tests --gtest_filter='*ForSale*:*Exchange*:*Expire*'`. Expected: PASS (existing listing-expiry coverage still green).

- [ ] **Step 7: Commit**

```bash
git add database/fighter.hpp database/fighter.cpp database/fighter_tests.cpp src/logic_tournament.cpp
git commit -m "perf(gsp): event-driven listing expiry via (status,expire) index — idle blocks do ~zero work

Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>"
```

---

### Task 6: Full `make check` + golden replay reconciliation

**Files:**
- Possibly modify: `src/goldenreplay.golden.json` (only if the state hash shifted — review the diff)

**Interfaces:** none new — this is the GSP consensus gate.

- [ ] **Step 1: Run the full gate** — build + `make check` in the container. Expected: `proto2/tests`, `database/tests`, and `src/{tests,goldenreplay_tests,reorg_tests,reorg_game_tests}` all PASS.

- [ ] **Step 2: If `goldenreplay_tests` fails on a state-hash mismatch, inspect why** — the new columns add bytes to every fighter row, so a stored state hash can shift even though the RPC JSON output (`FullState`/`Convert`) is unchanged. Regenerate and **review the diff**:

```bash
# inside the container, in /build/src
GOLDEN_REGEN=1 ./goldenreplay_tests
git -C /src-ro diff --stat   # (or copy the regenerated file out and diff on the host)
```

Confirm the diff is limited to hash/state fields and contains **no** change to fighter JSON field values (owner/quality/price/etc.). If any `Convert` output changed, STOP — that is a real regression, not a re-pin.

- [ ] **Step 3: Copy the regenerated golden back to the repo (host side) if it changed, and re-run** — then `make check` again. Expected: all PASS.

- [ ] **Step 4: Commit (only if the golden changed)**

```bash
git add src/goldenreplay.golden.json
git commit -m "chore(gsp): regenerate golden replay for the fighter exchange/quality columns (state-hash only)

Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>"
```

Note: the roconfig blob is unchanged (schema ≠ roconfig), so `RoConfigTests.LaunchConfigIsPinned` is unaffected and no roconfig re-pin is needed here. The launch-time genesis re-pin (`GetInitialStateBlock` Polygon height/hash) remains a separate, pre-existing launch blocker.

---

## Frontend (Tasks 7–11)

Frontend paths are under `/home/acoloss/treatfighter/tf-frontend`. Tests run with `npx vitest run <path>`; the whole suite is `npm test`.

### Task 7: `GspClient.getExchange(query)` + typed page

**Files:**
- Modify: `src/lib/chain/gsp-client.ts` (add `ExchangePage` + `getExchange`)
- Test: `tests/data/gsp-exchange.test.ts` (create)

**Interfaces:**
- Produces:
  - `export interface ExchangePage { fighters: Fighter[]; total: number; height: number; }`
  - `getExchange(request: Record<string, unknown>): Promise<ExchangePage>` — sends `getexchange` with named params `{ request }`, parses `{ fighters, total, height }`.

- [ ] **Step 1: Write the failing test** — create `tests/data/gsp-exchange.test.ts`:

```ts
// @vitest-environment node
//
// Unit test for GspClient.getExchange — sends the query as the named `request` param and parses the
// { fighters, total, height } page. Fully mocked fetch, no network.

import { describe, it, expect } from 'vitest';
import { GspClient, type FetchLike } from '@/lib/chain/gsp-client';

function jsonFetch(result: unknown): { fetchImpl: FetchLike; lastBody: () => any } {
  let body: any;
  const fetchImpl: FetchLike = async (_url, init) => {
    body = JSON.parse((init as { body: string }).body);
    return { ok: true, status: 200, statusText: 'OK', json: async () => ({ jsonrpc: '2.0', id: 1, result }) };
  };
  return { fetchImpl, lastBody: () => body };
}

describe('GspClient.getExchange', () => {
  it('sends the query under the named `request` param and parses the page', async () => {
    const { fetchImpl, lastBody } = jsonFetch({
      state: 'up-to-date', blockhash: 'h9', height: 42,
      data: { fighters: [{ id: 7 }, { id: 8 }], total: 137 },
    });
    const c = new GspClient('/gsp', fetchImpl);
    const page = await c.getExchange({ limit: 50, offset: 0, sort: 'price', order: 'asc' });

    expect(lastBody().method).toBe('getexchange');
    expect(lastBody().params).toEqual({ request: { limit: 50, offset: 0, sort: 'price', order: 'asc' } });
    expect(page.total).toBe(137);
    expect(page.height).toBe(42);
    expect(page.fighters.map((f) => f.id)).toEqual([7, 8]);
  });

  it('defaults total/fighters when the payload is empty', async () => {
    const { fetchImpl } = jsonFetch({ state: 'up-to-date', blockhash: '', height: 1, data: {} });
    const c = new GspClient('/gsp', fetchImpl);
    const page = await c.getExchange({});
    expect(page.total).toBe(0);
    expect(page.fighters).toEqual([]);
  });
});
```

- [ ] **Step 2: Run the test to verify it fails** — `npx vitest run tests/data/gsp-exchange.test.ts`. Expected: FAIL — `c.getExchange` is not a function.

- [ ] **Step 3: Implement `getExchange`** — in `src/lib/chain/gsp-client.ts`, add the type near the other exported interfaces and the method next to `getExchange`'s old positional stub (replace the existing `getExchange()` at lines 93-96):

```ts
/** One page of the marketplace (server-side paginated/sorted/filtered). */
export interface ExchangePage { fighters: Fighter[]; total: number; height: number; }
```

```ts
  /** The fighter marketplace: ONE page matching `request` (limit/offset/sort/order + optional
   *  minquality/maxquality/minprice/maxprice/affordablefor/excludeowner). Sent as the named `request`
   *  param. */
  async getExchange(request: Record<string, unknown>): Promise<ExchangePage> {
    const r = await this.read('getexchange', { request });
    const fighters = Array.isArray(r.data.fighters) ? (r.data.fighters as Fighter[]) : [];
    return { fighters, total: Number(r.data.total ?? 0) || 0, height: r.height };
  }
```

Add `Fighter` to the existing type import from `@/data/types` at the top of the file (it is not currently imported there — add `import type { Fighter } from '@/data/types';` if absent).

- [ ] **Step 4: Run the test to verify it passes** — `npx vitest run tests/data/gsp-exchange.test.ts`. Expected: PASS.

- [ ] **Step 5: Commit**

```bash
cd /home/acoloss/treatfighter/tf-frontend
git add src/lib/chain/gsp-client.ts tests/data/gsp-exchange.test.ts
git commit -m "feat(fe): GspClient.getExchange(query) returns one typed marketplace page

Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>"
```

---

### Task 8: Marketplace query model + pure helpers

**Files:**
- Create: `src/data/exchange-query.ts`
- Test: `tests/data/exchange-query.test.ts` (create)

**Interfaces:**
- Produces: `ExchangeSort`, `SortOrder`, `EXCHANGE_PAGE_SIZE`, `ExchangeQuery`, `DEFAULT_EXCHANGE_QUERY`, `toExchangeRequest(q, myName, myCrystals, pageSize?)`, `pageCount(total, pageSize?)`, `clampPage(page, total, pageSize?)`.

- [ ] **Step 1: Write the failing test** — create `tests/data/exchange-query.test.ts`:

```ts
// @vitest-environment node
import { describe, it, expect } from 'vitest';
import {
  toExchangeRequest, pageCount, clampPage, DEFAULT_EXCHANGE_QUERY, EXCHANGE_PAGE_SIZE,
} from '@/data/exchange-query';

describe('toExchangeRequest', () => {
  it('encodes window + sort and omits unset filters', () => {
    expect(toExchangeRequest(DEFAULT_EXCHANGE_QUERY, 'alice', 999)).toEqual({
      limit: EXCHANGE_PAGE_SIZE, offset: 0, sort: 'price', order: 'asc', excludeowner: 'alice',
    });
  });
  it('includes filters when set and affordablefor when toggled', () => {
    const req = toExchangeRequest(
      { sort: 'quality', order: 'desc', minQuality: 3, maxPrice: 500, affordableOnly: true, page: 2 },
      'bob', 250,
    );
    expect(req).toEqual({
      limit: EXCHANGE_PAGE_SIZE, offset: 2 * EXCHANGE_PAGE_SIZE, sort: 'quality', order: 'desc',
      minquality: 3, maxprice: 500, affordablefor: 250, excludeowner: 'bob',
    });
  });
  it('omits excludeowner when no player is selected', () => {
    expect(toExchangeRequest(DEFAULT_EXCHANGE_QUERY, null, 0).excludeowner).toBeUndefined();
  });
});

describe('pagination math', () => {
  it('pageCount is at least 1 and ceils', () => {
    expect(pageCount(0)).toBe(1);
    expect(pageCount(50)).toBe(1);
    expect(pageCount(51)).toBe(2);
  });
  it('clampPage keeps the page in range', () => {
    expect(clampPage(5, 51)).toBe(1);   // only 2 pages (0,1)
    expect(clampPage(-3, 200)).toBe(0);
  });
});
```

- [ ] **Step 2: Run the test to verify it fails** — `npx vitest run tests/data/exchange-query.test.ts`. Expected: FAIL — cannot resolve `@/data/exchange-query`.

- [ ] **Step 3: Implement the module** — create `src/data/exchange-query.ts`:

```ts
/**
 * Marketplace query model: the user-controlled view state (sort/filters/page) that ExchangeScreen owns,
 * plus the pure encoder into the getexchange RPC request and the pagination math. Kept pure + separate
 * from React so it is unit-tested directly (the repo has no React test harness).
 */

export type ExchangeSort = 'price' | 'quality' | 'sweetness' | 'expire';
export type SortOrder = 'asc' | 'desc';

/** Server-capped page size (the GSP also hard-caps at 100). */
export const EXCHANGE_PAGE_SIZE = 50;

/** The market view state controlled by the UI. Numeric filters are undefined = unset. */
export interface ExchangeQuery {
  sort: ExchangeSort;
  order: SortOrder;
  minQuality?: number;
  maxQuality?: number;
  minPrice?: number;
  maxPrice?: number;
  affordableOnly: boolean;
  page: number; // 0-based
}

export const DEFAULT_EXCHANGE_QUERY: ExchangeQuery = {
  sort: 'price', order: 'asc', affordableOnly: false, page: 0,
};

/** Build the getexchange request object from the view state + caller context. Unset filters are
 *  omitted so the GSP applies only what the user chose; the caller's own listings are excluded
 *  server-side so paging offsets/counts are correct. */
export function toExchangeRequest(
  q: ExchangeQuery, myName: string | null, myCrystals: number, pageSize = EXCHANGE_PAGE_SIZE,
): Record<string, unknown> {
  const req: Record<string, unknown> = {
    limit: pageSize, offset: q.page * pageSize, sort: q.sort, order: q.order,
  };
  if (q.minQuality != null) req.minquality = q.minQuality;
  if (q.maxQuality != null) req.maxquality = q.maxQuality;
  if (q.minPrice != null) req.minprice = q.minPrice;
  if (q.maxPrice != null) req.maxprice = q.maxPrice;
  if (q.affordableOnly) req.affordablefor = myCrystals;
  if (myName) req.excludeowner = myName;
  return req;
}

/** Total pages for `total` listings at `pageSize` (never below 1). */
export function pageCount(total: number, pageSize = EXCHANGE_PAGE_SIZE): number {
  return Math.max(1, Math.ceil(total / pageSize));
}

/** Clamp a 0-based page index into [0, pageCount-1]. */
export function clampPage(page: number, total: number, pageSize = EXCHANGE_PAGE_SIZE): number {
  return Math.min(Math.max(0, page), pageCount(total, pageSize) - 1);
}
```

- [ ] **Step 4: Run the test to verify it passes** — `npx vitest run tests/data/exchange-query.test.ts`. Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add src/data/exchange-query.ts tests/data/exchange-query.test.ts
git commit -m "feat(fe): marketplace query model + request encoder + pagination math (pure)

Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>"
```

---

### Task 9: `useExchangeMarket` hook + `ExchangeScreen` market UI

**Files:**
- Create: `src/ui/exchange/useExchangeMarket.ts`
- Modify: `src/ui/exchange/ExchangeScreen.tsx` (market section only — List + My-listings unchanged)
- Modify: `app/global.css` (controls bar styles)

**Interfaces:**
- Consumes: `getExchange` (Task 7), `toExchangeRequest`/`ExchangeQuery`/pagination helpers (Task 8).
- Produces: `useExchangeMarket(query, blockTick, myName, myCrystals): { page: ExchangePage | null; loading: boolean; error: string | null }`.

- [ ] **Step 1: Implement the hook** — create `src/ui/exchange/useExchangeMarket.ts`:

```ts
'use client';

/**
 * Fetch ONE page of the marketplace for `query`, refetching whenever the query OR `blockTick` changes.
 * `blockTick` is the USER slice height (advances every block), so the visible page stays live without a
 * dedicated per-block poll of the full market. Uses its own GspClient so aborts are isolated from the
 * block poller's long-poll.
 */

import { useEffect, useRef, useState } from 'react';
import { GspClient, type ExchangePage } from '@/lib/chain/gsp-client';
import { GSP_URL } from '@/config';
import { toExchangeRequest, type ExchangeQuery } from '@/data/exchange-query';

export interface MarketState { page: ExchangePage | null; loading: boolean; error: string | null; }

export function useExchangeMarket(
  query: ExchangeQuery, blockTick: number, myName: string | null, myCrystals: number,
): MarketState {
  const [state, setState] = useState<MarketState>({ page: null, loading: true, error: null });
  const clientRef = useRef<GspClient | null>(null);
  if (!clientRef.current) clientRef.current = new GspClient(GSP_URL);

  // Stable dependency: the request only changes when the query / name / affordability changes.
  const req = JSON.stringify(toExchangeRequest(query, myName, myCrystals));

  useEffect(() => {
    let alive = true;
    setState((s) => ({ ...s, loading: true, error: null }));
    clientRef.current!.getExchange(JSON.parse(req) as Record<string, unknown>)
      .then((page) => { if (alive) setState({ page, loading: false, error: null }); })
      .catch((e: unknown) => {
        if (alive) setState((s) => ({ ...s, loading: false, error: e instanceof Error ? e.message : 'load failed' }));
      });
    return () => { alive = false; clientRef.current!.cancel(); };
  }, [req, blockTick]);

  return state;
}
```

*(No isolated unit test: this is a thin fetch/abort/refetch React wrapper. Its pure inputs are covered by Tasks 7–8 and its live behaviour by the Task 11 Playwright pass — consistent with the repo's pure-function-first test style.)*

- [ ] **Step 2: Rewrite the market section of `ExchangeScreen`** — in `src/ui/exchange/ExchangeScreen.tsx`: remove `const exchange = useSlice('EXCHANGE');`, the `marketHeight`/`market` derivations (lines 31, 44, 55-57), and the `useSlice`/`OngoingType`-only imports that become unused. Add query state + the hook, and replace the Market `<section>` (lines 155-189) with the controls + paged grid. The List form and My-listings section (both from the USER slice) are unchanged. New pieces:

```tsx
// add imports
import { useExchangeMarket } from './useExchangeMarket';
import {
  DEFAULT_EXCHANGE_QUERY, pageCount, clampPage, EXCHANGE_PAGE_SIZE,
  type ExchangeQuery, type ExchangeSort, type SortOrder,
} from '@/data/exchange-query';
```

```tsx
// inside the component, after `const crystals = ...`
const [query, setQuery] = useState<ExchangeQuery>(DEFAULT_EXCHANGE_QUERY);
const blockTick = user?.height ?? 0; // advances every block via the USER poll
const { page, loading, error: marketErr } = useExchangeMarket(query, blockTick, playerName, crystals);
const total = page?.total ?? 0;
const totalPages = pageCount(total);
const market = (page?.fighters ?? []).filter((f) => f.status === FighterStatus.Exchange);
const marketHeight = page?.height ?? 0;

// clamp the page if the result set shrank (e.g. listings sold)
const setPage = (p: number) => setQuery((q) => ({ ...q, page: clampPage(p, total) }));
// any sort/filter change resets to page 0
const patch = (p: Partial<ExchangeQuery>) => setQuery((q) => ({ ...q, ...p, page: 0 }));
```

```tsx
// the Market section JSX
<section className="panel">
  <div className="row spread">
    <h2>Market</h2>
    <span className="muted">{total.toLocaleString()} listing{total === 1 ? '' : 's'}</span>
  </div>

  <div className="row exchange-controls" style={{ gap: 8, flexWrap: 'wrap', marginBottom: 8 }}>
    <label className="muted">Sort
      <select className="mono" value={query.sort}
        onChange={(e) => patch({ sort: e.target.value as ExchangeSort })}>
        <option value="price">Price</option>
        <option value="quality">Quality</option>
        <option value="sweetness">Sweetness</option>
        <option value="expire">Expiring soon</option>
      </select>
    </label>
    <label className="muted">Order
      <select className="mono" value={query.order}
        onChange={(e) => patch({ order: e.target.value as SortOrder })}>
        <option value="asc">Ascending</option>
        <option value="desc">Descending</option>
      </select>
    </label>
    <label className="muted">
      <input type="checkbox" checked={query.affordableOnly}
        onChange={(e) => patch({ affordableOnly: e.target.checked })} /> Affordable only
    </label>
  </div>

  {loading && !page ? (
    <p className="empty">Loading market…</p>
  ) : marketErr ? (
    <p className="err">{marketErr}</p>
  ) : market.length === 0 ? (
    <p className="empty">No fighters for sale right now.</p>
  ) : (
    <>
      <div className="cards">
        {market.map((f) => {
          const buyable = isBuyable(f, marketHeight);
          const affordable = crystals >= f.exchangeprice;
          const room = slots <= MAX_FIGHTER_INVENTORY;
          const canBuy = buyable && affordable && room;
          return (
            <div className="card" key={f.id}>
              <div className="fcard-art exchange-art"><FighterThumb fighter={f} /></div>
              <span className="cname" style={{ color: qualityColor(f.quality) }}>{f.name || `Fighter #${f.id}`}</span>
              <span className="cmeta">{f.exchangeprice.toLocaleString()} crystals · Q{f.quality} · sweet {f.sweetness}</span>
              <span className="cmeta">seller {f.owner} · {isLive(f, marketHeight) ? `${listingBlocksLeft(f, marketHeight)} blocks left` : 'expired'}</span>
              {canBuy ? (
                <button disabled={!!busy} onClick={() => onBuy(f)}>
                  {busy === `Buy #${f.id}` ? '…' : `Buy · ${f.exchangeprice.toLocaleString()}`}
                </button>
              ) : (
                <span className="badge warn">
                  {!buyable ? 'expired' : !affordable ? 'not enough crystals' : 'inventory full'}
                </span>
              )}
            </div>
          );
        })}
      </div>
      <div className="row spread exchange-pager" style={{ marginTop: 10 }}>
        <button className="ghost" disabled={query.page <= 0} onClick={() => setPage(query.page - 1)}>‹ Prev</button>
        <span className="muted">Page {query.page + 1} of {totalPages}</span>
        <button className="ghost" disabled={query.page >= totalPages - 1} onClick={() => setPage(query.page + 1)}>Next ›</button>
      </div>
    </>
  )}
</section>
```

Keep `import { useState } from 'react';` (already present). Remove `useSlice` import if no longer used elsewhere in the file, and drop `OngoingType`/`marketHeight`-from-slice only if now unused (leave `OngoingType` — it is still used for the `cooking`/`slots` calc).

- [ ] **Step 3: Add the controls styles** — in `app/global.css`, add:

```css
.exchange-controls label { display: inline-flex; align-items: center; gap: 6px; }
.exchange-controls select { margin-left: 6px; padding: 6px 8px; border-radius: 8px;
  border: 1px solid var(--line); background: var(--panel-2); color: var(--ink); }
.exchange-pager { align-items: center; }
```

- [ ] **Step 4: Build + typecheck + run the full test suite** — `npm run build` (Next type-checks) and `npm test`. Expected: build succeeds; all existing tests still green EXCEPT the EXCHANGE-slice tests, which are fixed in Task 10 (if `npm run build` fails only because `useSlice('EXCHANGE')` was removed while the slice type still exists, that is fine — the slice is removed next). If any NON-exchange test regresses, fix before continuing.

- [ ] **Step 5: Commit**

```bash
git add src/ui/exchange/useExchangeMarket.ts src/ui/exchange/ExchangeScreen.tsx app/global.css
git commit -m "feat(fe): ExchangeScreen server-paginated market with sort/filter/paging via useExchangeMarket

Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>"
```

---

### Task 10: Remove the per-block EXCHANGE poll slice

**Files:**
- Modify: `src/data/slices.ts` (remove `ExchangeSlice`, `SLICE_RPC.EXCHANGE`, `ALL_SLICES` entry, `SCREEN_EXTRA.exchange`)
- Modify: `src/data/poller.ts:173` (comment mentioning EXCHANGE)
- Modify: `tests/data/slices.test.ts`, `tests/data/poller.test.ts`, `tests/data/poller-live.test.ts`

**Interfaces:**
- Consumes: the market now comes from `useExchangeMarket` (Task 9), so no screen reads the EXCHANGE slice.
- Produces: `Slice` union without `EXCHANGE`; the exchange screen's slice set is just `['USER']`.

- [ ] **Step 1: Update the failing tests first (they now assert the new shape)** — in `tests/data/slices.test.ts`, replace the EXCHANGE assertions (lines 27, 45, 52) so the exchange screen needs only USER and EXCHANGE is gone:

```ts
    expect(slicesForScreen('exchange')).toEqual(['USER']);
```

  and delete the two `SLICE_RPC.EXCHANGE...` expectations (lines 45, 52). In `tests/data/poller.test.ts`, change the exchange-screen assertions (lines 103-108, 158): the exchange screen now batches only `['getuser']` (and with no player, an empty slice set → the poller tracks the tip via `getNullState`, issuing no batch). Update:

```ts
    // switch to the exchange screen: USER only (EXCHANGE is fetched on-demand by the screen, not polled)
    expect(mock.batchCalls[1].map((c) => c.method)).toEqual(['getuser']);
```

  and remove the `cache.get('EXCHANGE')` expectation (line 108) and the line-158 `['getexchange']` expectation (with no player + exchange screen there are no name-independent slices left, so no batch is sent — assert `mock.batchCalls.length` stays 0 instead). In `tests/data/poller-live.test.ts`, delete the EXCHANGE-slice assertions (lines 36, 40-41) and retitle the test to only cover USER seeding on the exchange screen (this is a gated live test; keep it building).

- [ ] **Step 2: Run the tests to verify they fail** — `npx vitest run tests/data/slices.test.ts tests/data/poller.test.ts`. Expected: FAIL — the code still declares EXCHANGE, so `slicesForScreen('exchange')` still returns `['USER','EXCHANGE']`.

- [ ] **Step 3: Remove the slice from the data layer** — in `src/data/slices.ts`: delete the `ExchangeSlice` interface (lines 30-34), the `EXCHANGE: ExchangeSlice;` line in `SliceValue` (line 46), the `'EXCHANGE'` entry in `ALL_SLICES` (line 53), the whole `EXCHANGE: { ... }` block in `SLICE_RPC` (lines 90-95), and change `SCREEN_EXTRA.exchange` (line 119) to `exchange: [],`. In `src/data/poller.ts` line 173, drop "EXCHANGE/" from the comment so it reads "(ALL_USERS) are player-independent".

- [ ] **Step 4: Run the tests to verify they pass** — `npx vitest run tests/data/slices.test.ts tests/data/poller.test.ts`. Expected: PASS.

- [ ] **Step 5: Run the whole suite + build** — `npm test` and `npm run build`. Expected: all green; no dangling `EXCHANGE`/`ExchangeSlice` references (the gsp-batch/proxy tests that merely use the string `'getexchange'` as an example method name still pass — they don't depend on the slice).

- [ ] **Step 6: Commit**

```bash
git add src/data/slices.ts src/data/poller.ts tests/data/slices.test.ts tests/data/poller.test.ts tests/data/poller-live.test.ts
git commit -m "refactor(fe): drop the per-block EXCHANGE poll slice — market is on-demand paged now

Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>"
```

---

### Task 11: Live verification — eyes-on-pixels + fork e2e at scale

**Files:** none (verification only). Uses the fork stack (`forked-evm-testing`) + the gated on-chain suite in `tf-frontend/tests` per memory `fork-harness-e2e`.

**Interfaces:** none — this proves the whole path end-to-end and at scale.

- [ ] **Step 1: Bring up the fork stack + GSP with the new schema** — rebuild the GSP image, start the fork (non-192.168 subnet; Alchemy key only from `forked-evm-testing/.env`), and confirm the GSP syncs. Expected: `getnullstate` → `up-to-date`.

- [ ] **Step 2: Seed many listings** — via the gated on-chain harness, cook + list a few hundred fighters across ≥2 accounts at varied price/quality/sweetness (script the `f.s` moves in a loop; use the fork auto-miner to advance blocks). Expected: `getexchange({"limit":50,"offset":0,"sort":"price","order":"asc"})` returns 50 rows + a `total` in the hundreds, sorted by price.

- [ ] **Step 3: Verify server-side sort/filter/paging/exclusion by RPC** — call `getexchange` with: `sort:"quality",order:"desc"`; `minprice`/`maxprice`; `affordablefor`; `excludeowner:<seller>`; and successive `offset`s. Expected: order, filtering, self-exclusion, and page windows all correct; `total` reflects the filter, not the page.

- [ ] **Step 4: Verify idle-block cost is flat** — advance many empty blocks and confirm the GSP does no per-block work proportional to the listing count (spot-check logs / block-processing time is stable as listings grow; the `(status,expire)` seek returns nothing on idle blocks). Expected: idle-block processing time does not grow with listing count.

- [ ] **Step 5: Eyes-on-pixels** — serve the live frontend (per `fork-harness-e2e`; render harness needs `allowedDevOrigins += localhost/127.0.0.1`) and run `npm run render:shots`. Manually drive the Exchange screen: sort dropdown, affordable toggle, Prev/Next paging, buy flow. Expected: the controls bar renders on-theme, pages of ≤50 cards load with thumbnails, paging + sort + filter update the grid, and a buy still works; the count reads the real `total`.

- [ ] **Step 6: Final full gates** — GSP `make check` (green from Task 6) and frontend `npm test` (green). Record the results. No commit (verification task); if any defect is found, add a follow-up task rather than marking complete.

---

## Self-review notes

- **Spec coverage:** schema columns + bind-at-flush (T1) · indexes (T1) · paged/sorted/filtered `getexchange` + `total` + server-side `excludeowner` + drop QueryAll/salehistory (T2–T4) · event-driven expiry (T5) · golden/gate (T6) · remove poll slice + on-demand hook (T7–T10) · sort/filter/paging UI (T9) · SSE/Redis forward-compat is design-only (no task, per spec §9) · genesis re-pin noted as pre-existing (T6). The one deliberate spec deviation: the `Convert` ongoings-skip micro-opt is **deferred as YAGNI** (T3 note) because `Convert` is now page-bounded.
- **Determinism:** columns are proto projections bound at the one flush point (T1); sort identifiers come from a fixed whitelist, values are bound (T2); expiry preserves strict `<` (T5) → state transitions byte-identical, golden shifts at most in the state hash (T6).
- **Type consistency:** `ExchangeQuery`/`ExchangeSort` (C++ T2) and `ExchangeQuery`/`ExchangeSort` (TS T8) are intentionally parallel but independent; `ExchangePage {fighters,total,height}` is produced in T7 and consumed in T9; `toExchangeRequest` keys (`minquality`,`affordablefor`,`excludeowner`,…) match the C++ `OptInt`/`isMember` keys in T3.

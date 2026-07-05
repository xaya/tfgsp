# Exchange Scale — Design Spec

**Date:** 2026-07-05
**Status:** Approved design, pre-implementation
**Spans:** `tfgsp-polygon` (GSP `getexchange` + `fighters` schema + per-block maintenance) and
`tf-frontend` (`ExchangeScreen` + data layer)

## 1. Goal

Make the fighter marketplace usable at **thousands–tens of thousands of live listings**:

- Bounded GSP cost per `getexchange` call (O(page), not O(all fighters)).
- Bounded wire payload (one page, not the whole market + every ever-sold fighter).
- Bounded frontend DOM + parse cost (one page of cards).
- **~zero exchange work on an idle block** — no per-block O(listed) scan ([[event-driven-gsp]]).
- Real discovery: server-side **sort + filter + paging** (a "full marketplace", not a dump).
- Forward-compatible with an SSE + Redis relay tier (see §9) without rework.

## 2. Current state (the diagnosis)

**GSP `GameStateJson::Exchange()` (`src/gamestatejson.cpp:797`)** is the bottleneck:

- Calls `FighterTable::QueryAll()` — a **full scan of the entire `fighters` table**, which grows with
  every fighter *ever cooked*, forever — not just current listings.
- Emits every row where `status == Exchange` **OR `salehistory_size() > 0`**, so it also returns
  **every fighter ever sold** — a monotonically growing set. **Nothing on the frontend consumes sale
  history** (the client just filters those rows out by `status`), so this is pure wasted payload + CPU.
- For **each** emitted fighter, `Convert<FighterInstance>` runs a *per-fighter*
  `OngoingsTable::QueryForOwner(...)` subquery to derive `blocksleft`. A listed fighter is never
  mid-activity, so this subquery is always wasted work, once per emitted row.

**GSP per-block maintenance `CheckFightersForSale()` (`src/logic_tournament.cpp:335`)** runs on **every**
block and does `QueryForStatus(Exchange)` → walks **every live listing**, deserializes each proto, checks
`exchangeexpire < height`. At 50k live listings that is 50k proto deserializations **every block**, even
when nothing expires. Index scan, but still O(listed) per idle block — violates [[event-driven-gsp]].

**Frontend:**

- The block poller refetches the **full** `getexchange` every block while the Exchange screen is open
  (`src/data/poller.ts`, `EXCHANGE` slice) → a big JSON parse every ~2s at scale.
- `ExchangeScreen` renders every listing as a card. Mitigant already in place: `FighterThumb` uses a
  **single shared offscreen renderer → cached `<img>`** (`src/ui/chrome/FighterThumb.tsx`), *not* a
  canvas-per-card, so there is **no** WebGL-context-limit problem. The real frontend cost at scale is
  DOM node count + the full-set JSON parse — both fixed by server-side paging.

**Confirmed safe foundation:** `FighterInstance::~FighterInstance()` (`database/fighter.cpp:405`) is the
**single** fighter write path — one `INSERT OR REPLACE INTO fighters(...)` per dirty fighter. Denormalized
columns bound there are re-projected from the proto on *every* write, so they cannot drift from the proto
and fork consensus.

## 3. GSP — schema (consensus-critical; requires genesis re-pin)

Add four denormalized columns to `fighters`, each a pure projection of the proto, bound in the single
flush point (`~FighterInstance`) alongside `proto`/`status`:

| column      | source (proto scalar) | purpose                        |
|-------------|-----------------------|--------------------------------|
| `price`     | `proto.exchangeprice()`  | sort / filter by price      |
| `expire`    | `proto.exchangeexpire()` | expiry maintenance + "expiring soon" sort |
| `quality`   | `proto.quality()`     | sort / filter by rarity        |
| `sweetness` | `proto.sweetness()`   | sort / filter by sweetness     |

All four are direct proto scalar fields (already emitted by `Convert` as `quality`/`sweetness`/
`exchangeprice`/`exchangeexpire`), so the flush binds are plain accessor reads.

Composite indexes, each prefixed by `status` so the market query (`WHERE status = Exchange`) is
index-driven and the ORDER BY is satisfied by the index:

```sql
CREATE INDEX fighters_exchange_price     ON fighters (status, price);
CREATE INDEX fighters_exchange_quality   ON fighters (status, quality);
CREATE INDEX fighters_exchange_sweetness ON fighters (status, sweetness);
CREATE INDEX fighters_exchange_expire    ON fighters (status, expire);
```

**Drift-safety:** columns are recomputed from the proto on every `INSERT OR REPLACE`, so they are
structurally guaranteed to match the blob. (SQLite generated columns can't parse a protobuf blob, so
explicit bind-at-flush is the correct pattern — not `GENERATED ALWAYS AS`.) The values written are the
same on every node running the same code, so consensus is preserved; the new column bytes change the state
hash, hence the genesis re-pin.

`BindFieldValues` / the `~FighterInstance` INSERT statement extend from
`(id, owner, proto, status)` to `(id, owner, proto, status, price, expire, quality, sweetness)`.

## 4. GSP — `getexchange` RPC

Change from the currently-ignored empty param to a **named param object** (same convention as
`getfueldata`). Update `src/rpc-stubs/tf-rpc.json`, regenerate the stub, and give
`PXRpcServer::getexchange` / `GameStateJson::Exchange` the query argument.

**Request:**

```
getexchange({
  limit,          // page size; server-capped (default 50, hard cap 100)
  offset,         // page window start (LIMIT/OFFSET paging)
  sort,           // 'price' | 'quality' | 'sweetness' | 'expire'
  order,          // 'asc' | 'desc'
  minquality?, maxquality?,   // optional filters
  minprice?,     maxprice?,
  affordablefor?, // optional: price <= this many crystals (maps to caller balance)
  excludeowner?   // caller's name; self-listings filtered SERVER-side
})
```

**Response:**

```
Exchange() returns just:
{ fighters: [ ...one page, already Convert<FighterInstance> ... ],
  total }    // COUNT(*) of rows matching the filter (before LIMIT/OFFSET) — for "N listings" + page count
```

`height` is NOT a key of this object — it rides on the standard libxayagame read envelope
(`{ blockhash, height, state, data: {fighters, total} }`), so `GspClient.getExchange` reads
`total` from `.data` and `height` from the envelope. All request keys are lowercase
(`affordablefor`, `excludeowner`, `minquality`, …) — the exact keys `GameStateJson::Exchange` reads.

**Query shape:**

```sql
SELECT ... FROM fighters
 WHERE status = <Exchange>
   [AND quality BETWEEN :minq AND :maxq]
   [AND price   BETWEEN :minp AND :maxp]
   [AND price  <= :affordable]
   [AND owner  <> :excludeowner]
 ORDER BY <sortcol> <order>, id
 LIMIT :limit OFFSET :offset;
-- plus a matching COUNT(*) for `total`.
```

Key changes vs today:

- **Replaces `QueryAll()`** with the indexed, filtered, paged query — O(page) walk over the sort index.
- **Drops the `salehistory` clause** — ever-sold rows leave the feed entirely (nothing consumes them).
- **`excludeowner` is server-side now** — mandatory, else offsets/counts would include the caller's own
  listings and paging math breaks. (The old client-side `owner !== playerName` filter is removed.)
- Only the **page** of rows is `Convert`-ed, not the whole market.
- The `Convert` used for exchange rows **skips the per-fighter `ongoings` subquery** (a listed fighter is
  never mid-activity → `blocksleft` is irrelevant; the client uses `exchangeexpire`). Add an exchange
  Convert path / flag that emits `blocksleft = 0` without the subquery.
- `ORDER BY <sortcol>, id` — the `id` tiebreak keeps paging deterministic across equal sort keys.

Deep-`OFFSET` paging is O(offset) index-walk in SQLite; fine to tens of thousands (microseconds/row).
**Keyset pagination** (`WHERE sortcol > :lastseen`) is the noted future upgrade for hundreds of thousands.

## 5. GSP — per-block maintenance (idle-block performance)

Rewrite `CheckFightersForSale()` to seek only the rows that actually expired, using the new
`(status, expire)` index:

```sql
SELECT id FROM fighters WHERE status = <Exchange> AND expire < :height;
```

Add `FighterTable::QueryExpiredListings(unsigned height)` for this. On an idle block (nothing expiring)
this is a single index probe returning zero rows → **~zero work**. This is the direct answer to
"idle blocks must be fast; no per-block scan of the listing set."

**Not changed** (already bounded, confirmed):

- `SetFreeTransfiguringFighters()` drains the Transfiguring status to Available *every* block, so the set
  it scans is only ever "fighters transfigured in the previous block" — bounded by moves-per-block, not
  total fighters. Leave as-is.
- `getexchange` is a client-driven read (`GetCustomStateData`); it never runs inside `UpdateState`, so it
  cannot affect idle-block cost regardless of listing count.

## 6. Frontend — data layer

- **Remove the `EXCHANGE` slice from the poller/slice system** (`src/data/slices.ts` — `ExchangeSlice`,
  `SLICE_RPC.EXCHANGE`, `ALL_SLICES`, `SCREEN_EXTRA.exchange`). It no longer fits the name-keyed,
  poll-every-block slice model; it is now a query-shaped on-demand read. Net DRY cleanup — no screen but
  Exchange ever needed it.
- **New `GspClient.getExchange(query)`** → `{ fighters, total, height }`, sent as a **named param object**
  (mirrors `getFuelData`, not the positional `[{}]` used today).
- **`useExchangeMarket(query, blockTick)` hook**, owned by `ExchangeScreen`:
  - Fetches on **query change** (debounced for filter typing / price inputs) and on **block tick**.
  - `blockTick = user.height` — already advanced every block by the USER poll, so no new subscription
    primitive is needed; when it changes, refetch the **current page only** (~50 rows) → bounded payload
    per block, never the full set.
  - Exposes `{ market, total, loading, error }`. Aborts in-flight fetches on query change / unmount.
- **"My listings" is untouched** — it already derives from the USER slice (still polled), so the seller's
  own listings stay live independent of market paging.

## 7. Frontend — UX (`ExchangeScreen`)

Keep the existing card grid (shared-renderer thumbs are fine at page size). Add a controls bar above it:

- **Sort** dropdown: Price / Quality / Sweetness / Expiring-soon, each asc/desc.
- **Filters**: quality range, price range, "affordable only" toggle (→ `affordableFor` = my crystals).
- **Paging**: Prev / Next, "Page X of Y", "N listings" total. Reset to page 0 on any sort/filter change.
- Loading / empty / error states.

Page size default **50**. Sort set: **Price / Quality / Sweetness / Expiring-soon** (confirmed).

## 8. Testing

**GSP:**

- Fighter DB test: the four columns stay in sync with the proto across list → sell → expire; a sold /
  expired fighter (`status != Exchange`) is excluded from the market query.
- `getexchange` (`src/gamestatejson_tests.cpp`): seed N listings across owners/qualities/prices; assert
  sort order, order direction, each filter, `total`, `excludeowner` self-exclusion, page windows, and
  that expired + sold + salehistory rows never appear.
- `CheckFightersForSale`: only listings with `expire < height` flip to Available; others stay Exchange.
- **Regenerate the golden replay; re-pin genesis** (schema changed — new column bytes change the hash).

**Frontend:**

- `useExchangeMarket`: refetch on query change and on `blockTick` change; page reset on sort/filter
  change; query-param encoding; in-flight abort.
- Pagination math (page count from `total` + `limit`).
- Playwright eyes-on-pixels with many seeded listings (grid, controls, paging).

## 9. Forward compatibility — SSE + Redis (out of scope now)

`getexchange(query)` is a pure function of (committed block, query) with no per-connection state. A future
[[sse-client-scaling]] relay tier (taurionui method) can therefore:

- Broadcast block signals over **SSE**, so many web clients stop each long-polling the one GSP.
- Cache `getexchange` pages in **Redis** keyed by `(blockhash, query)`, serving thousands of clients from
  cache and invalidating on each new block.

Nothing in this design precludes that; it is explicitly a later layer, not built here.

## 10. Migration

Schema change → **genesis re-pin** (already a pending launch blocker; folded in). Pre-mainnet the DB
regenerates from genesis, so there is no live migration to write.

## 11. Out of scope

- The SSE/Redis relay tier itself (§9).
- Keyset pagination (LIMIT/OFFSET is sufficient at target scale; noted future upgrade).
- A sale-history UI (none exists; the salehistory rows are simply dropped from the feed).
- Any change to listing/buying/removing move grammar or economics.

## 12. Implementation order (for the plan)

1. GSP schema: columns + indexes + bind-at-flush + regenerate golden.
2. GSP `getexchange` query (params, indexed query, `total`, exchange Convert without ongoings subquery) +
   tests.
3. GSP `CheckFightersForSale` → indexed expiry query + test.
4. GSP genesis re-pin + golden replay regen + full `make check`.
5. Frontend: remove EXCHANGE slice, add `getExchange(query)` + `useExchangeMarket` + tests.
6. Frontend: `ExchangeScreen` controls (sort/filter/paging) + tests + eyes-on-pixels.
7. Fork e2e at scale (seed many listings; verify paging/sort/filter live).

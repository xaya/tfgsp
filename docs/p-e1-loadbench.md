# P-E1 — the "writes every block / clogs the DB" problem: issue, fix, and benchmarks

## The issue (root cause)

`PXLogic::TickAndResolveOngoings` (`src/logic.cpp`) runs **every block**. The old code did:

```cpp
auto res = xayaplayers.QueryAll ();            // EVERY player, every block
while (res.Step ()) {
  auto a = xayaplayers.GetFromResult (res, ...);
  auto& ongoings = *a->MutableProto().mutable_ongoings();   // <-- always
  ...
}
```

`MutableProto()` calls `LazyProto::Mutable()`, which **unconditionally** flags the row
`MODIFIED` — *before* checking whether the player has any ongoing operation. So the
`~XayaPlayer` destructor re-serialised and `INSERT OR REPLACE`d that player's **entire
proto + inventory BLOB every block, for every player**, even players with nothing
happening. Result:

- **O(players) physical row-writes per block** → WAL / page churn → the "writes every
  block, slow sync" symptom.
- **O(players) full-table scan per block** → CPU/time grows with the player count.

A second, related problem (not fixed by Stage 1, see below): an in-flight build stores a
per-block `BlocksLeft` countdown *inside the player proto*, so an active player's proto
genuinely changes every block and is legitimately re-written every block → its undo
changeset grows every block.

## What I did to fix it

**Stage 1 (commit `28a841d`) — skip-when-empty guard.** Wrap the per-block body so idle
players are never dirtied:

```cpp
if (a->GetOngoingsSize () > 0)      // GetProto() — read-only, does NOT dirty the row
{
  auto& ongoings = *a->MutableProto().mutable_ongoings();
  ... unchanged ...
}
```

`GetOngoingsSize()` reads through `GetProto()` (which leaves the row `UNMODIFIED`), so a
player with no ongoings is never written and the destructor early-returns. Players **with**
ongoings are processed exactly as before. **Rule-preserving:** the golden-replay snapshot is
byte-identical, and `make check` is green.

**Stage 2 (foundation committed `6eb4536`, cutover = 2b, pending).** A height-keyed
`ongoing_operations` table: each build is one row whose `height` column is the *absolute*
resolve block. The tick becomes `SELECT … WHERE height <= now` (no full scan), and an
in-flight build's row is written once at schedule + once at fire (not every block). This is
what removes the remaining O(N) scan time and the active-player undo growth.

## How the benchmark measures it

`src/loadbench_tests.cpp` (`TF_LOADBENCH=1`), run on the **POLYGON** chain (production) at a
production-like height. Steady-state means over 20 measured blocks (`TF_LOADBENCH_WARMUP=10
TF_LOADBENCH_M=30`). Metrics per block:

- **writes/block** — physical `INSERT OR REPLACE` row-ops (`sqlite3_total_changes` delta) =
  WAL/page churn = sync-speed cost.
- **undo-bytes/block** — size of the SQLite session changeset = exactly what `xaya::SQLiteGame`
  appends to the `xayagame_undo` table each block = **on-disk DB growth**. (Identical-content
  rewrites coalesce to 0 here — so idle churn shows as writes, not undo.)
- **main-db KiB** — live SQLite size (`page_count × page_size`) before → after the run.

> NOTE: benchmark on POLYGON, never REGTEST — REGTEST takes a test-only every-block
> `RecalculatePlayerTiers` branch (`logic.cpp:986`) that rewrites all players and inflates the
> numbers. Also: `docker cp` preserves source mtime, so `touch`/`rm` the object after copying
> a changed file into the build container or `make` skips the recompile.

## Benchmarks — ORIGINAL (pre-fix) vs NOW (Stage 1)

### Idle players (no ongoings) — the "writes every block" case

| players | writes/block ORIG → NOW | undo-bytes/block ORIG → NOW | main-db KiB (NOW) |
|--------:|:-----------------------:|:--------------------------:|------------------:|
| 0       | 28      → **21** | 0 → 0 | 152     |
| 100     | 128     → **21** | 0 → 0 | 404     |
| 1 000   | 1 028   → **21** | 0 → 0 | 2 592   |
| 10 000  | 10 028  → **21** | 0 → 0 | 24 560  |
| 50 000  | 50 028  → **21** | 0 → 0 | 124 684 |

Original idle writes/block = exactly **N + 28** (every player rewritten every block). NOW it is
a **flat 21 regardless of N** — at 50k players, **50 028 → 21 writes/block (~2 400× fewer)**.
Idle undo is 0 in both (the idle cost was pure WAL churn, not undo growth).

### Active players (10 000 seeded, K with an in-flight build)

| K active | writes/block ORIG → NOW | undo-bytes/block ORIG → NOW |
|---------:|:-----------------------:|:--------------------------:|
| 10       | 10 028 → **31**   | 809    → 809    |
| 100      | 10 028 → **121**  | 8 009  → 8 009  |
| 1 000    | 10 028 → **1 021**| 80 909 → 80 909 |

Stage 1 also cuts active-case **writes/block** from 10 028 (all 10k players) to `21 + K` (only
the K players with a build), because the 9 000–9 990 idle players are now skipped. BUT
**undo-bytes/block is unchanged** (809 per active build per block) — that is the Stage 2b
target, not fixed yet.

### Storage

- **Main DB** does **not** grow from per-block activity in either version: `INSERT OR REPLACE`
  reuses the row's pages, so main-db KiB before→after a 30-block run is ~flat (it only scales
  with how many players exist, which is expected). The idle "writes every block" was I/O churn
  (slow sync), not permanent disk growth.
- **Undo log** (`xayagame_undo`) is the permanent on-disk bloat, and it grows by
  `undo-bytes/block` every block. With the GSP default `--enable_pruning=-1` (off) it is never
  trimmed → unbounded. Projected at Polygon ~2 s blocks (~43 200 blocks/day):

  | scenario | undo-bytes/block | projected undo/day |
  |----------|-----------------:|-------------------:|
  | idle, any N (NOW)        | 0      | **0**        |
  | 10 active builds         | 809    | ~35 MB/day   |
  | 100 active builds        | 8 009  | ~346 MB/day  |
  | 1 000 active builds      | 80 909 | **~3.5 GB/day** |

  The active-build numbers are the **same before and after Stage 1**; Stage 2b (absolute-height
  table) drops the per-in-flight-block undo to ~0 (a build only writes undo on the block it is
  scheduled and the block it fires). Enabling pruning additionally bounds the retained history.

## Summary

| symptom | ORIGINAL | NOW (Stage 1) | after Stage 2b (planned) |
|---------|----------|---------------|--------------------------|
| idle writes/block (50k) | 50 028 | **21** | 21 |
| idle scan time/block (50k) | 643 ms | 261 ms | ~O(due ops), not O(N) |
| active undo/block (1k builds) | 80 909 B | 80 909 B | ~0 on non-firing blocks |
| undo log growth (1k builds) | ~3.5 GB/day | ~3.5 GB/day | ~0 + pruning |

Stage 1 eliminated the O(N) **write churn** (the "writes every block" complaint) for idle
players, verified rule-preserving. Stage 2b removes the remaining O(N) **scan time** and the
active-build **undo growth** (the "clogs the DB" complaint).

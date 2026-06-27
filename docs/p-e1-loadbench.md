# P-E1 load benchmark — results

Synthetic-load benchmark (`src/loadbench_tests.cpp`, run with `TF_LOADBENCH=1`)
measuring the per-block cost of the ongoing-operations tick as the player count
scales. Run on the **POLYGON** chain (production) at a production-like height —
NOT REGTEST, which takes a test-only `RecalculatePlayerTiers`-every-block branch
(`logic.cpp:986`) that does not exist in production. All numbers are steady-state
means (warm-up blocks discarded via `TF_LOADBENCH_WARMUP`).

Metrics per block:
- **writes/block** — physical `INSERT OR REPLACE` row-ops (`sqlite3_total_changes`
  delta). This is WAL / page churn — the "writes every block / slow sync" symptom.
- **undo-bytes/block** — size of the SQLite session changeset (what `xaya::SQLiteGame`
  stores into `xayagame_undo` each block). This is the on-disk DB-bloat / "clogs the
  DB" symptom. Note: the session coalesces identical-content rewrites to nothing, so
  idle-but-rewritten rows contribute 0 here.
- **ms/block** — wall-time, i.e. the O(N) `QueryAll` scan cost.

## Stage 1 — skip-when-empty guard in `TickAndResolveOngoings`

Guard: only call `MutableProto()` for players that actually have ongoings
(`if (a->GetOngoingsSize () > 0)`), so idle players are never marked dirty / rewritten.
Rule-preserving: the golden-replay snapshot is byte-identical before/after.

### Idle players (no ongoings) — steady state, POLYGON

| players | writes/block BEFORE | writes/block AFTER | undo-bytes/block (both) | ms/block AFTER |
|--------:|--------------------:|-------------------:|------------------------:|---------------:|
| 0       | 28      | 21 | 0 | 0.7   |
| 100     | 128     | 21 | 0 | 1.2   |
| 1 000   | 1 028   | 21 | 0 | 5.1   |
| 10 000  | 10 028  | 21 | 0 | 47    |
| 50 000  | 50 028  | 21 | 0 | 249   |

BEFORE the fix, idle writes/block are exactly **N + 28** — every player's full
proto+inventory BLOB is physically rewritten every block (identical content, so
`undo-bytes` is still 0 — the bloat here is pure WAL/page churn, not undo growth).
AFTER, writes/block is a **flat 21 regardless of N** (fixed special-tournament +
globaldata + tournament bookkeeping). At 50k players that is ~50 028 → 21 writes/block.

## What Stage 1 does NOT fix (→ Stage 2)

1. **ms/block still scales O(N)** (249 ms at 50k). The guard stops the per-block
   *writes* but the tick still `QueryAll`-**scans** every player every block. The
   height-keyed `ongoing_operations` table (Stage 2) replaces the full scan with
   `SELECT ... WHERE height <= now`, so a block with nothing due touches nothing.

2. **Active players grow the undo log O(K)**. A player with an in-flight ongoing
   has its `BlocksLeft` decremented every block → its proto changes → it is written
   every block:

   | active players (of 10 000) | undo-bytes/block | net `xayaplayers` writes/block |
   |---------------------------:|-----------------:|-------------------------------:|
   | 10    | 809    | 10   |
   | 100   | 8 009  | 100  |
   | 1 000 | 80 909 | 1 000 |

   ~809 undo-bytes per active player per block ⇒ ~80 KB/block at 1 000 active builds,
   i.e. on the order of GBs/day of `xayagame_undo`. Stage 2 stores an **absolute
   firing height** instead of a per-block-decremented countdown, so an in-flight
   ongoing's row is written once at schedule and once when it fires — not every block.

## Caveats

- REGTEST inflates idle writes/block to ~N (test-only `RecalculatePlayerTiers` every
  block at `logic.cpp:986`); always benchmark on POLYGON for production numbers.
- `docker cp` preserves source mtime; after copying a changed file into the build
  container, `touch` it (or `rm` its object) or `make` may skip the recompile.

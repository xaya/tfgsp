# Original Treatfighter vs. the Polygon rewrite — performance & issues comparison

Living comparison of the **original** Treatfighter GSP (Xaya/CHI-era, the "writes every
block / clogs the DB" version) against the **rewrite** (`polygon-rewrite` branch, on
Polygon via the XayaX eth bridge). It tracks measured performance differences and every
consensus / correctness / scalability issue found and fixed, so the two can be compared
directly and nothing is lost.

Detailed source docs (this page is the index + the head-to-head numbers):
- `docs/p-e1-loadbench.md` — the P-E1 "writes every block" benchmark (original vs new).
- `docs/security-audit.md` — the move-reachability security/DoS audit (§11 = launch re-verify).
- `docs/quality-audit-findings.md` / `docs/quality-audit-2026-06-29.md` — the 80-finding clean/DRY/determinism sweep.
- `docs/reorg-and-scale-testing-plan.md` — the reorg + scale (20k) test harness design.

## Benchmark tooling (all repeatable; on the POLYGON chain at production height)

| Tool | File | What it measures | How to run (in `tfdev`) |
|---|---|---|---|
| P-E1 load sweep | `src/loadbench_tests.cpp` (`LoadBenchTests`) | per-block undo-bytes / ms / main-db KiB / per-table writes vs player count, IDLE blocks | `TF_LOADBENCH=1 ./loadbench_tests` |
| **Scale / block-growth** | `src/loadbench_tests.cpp` (`ScaleBenchTests.BlockGrowthUnderFullActivity`) | per-block UpdateState ms + DB size + table row counts over a sustained run of REAL mixed activity (tournaments + cooking + market) — the growth CURVE over time | `TF_SCALE=1 TF_SCALE_N=20000 TF_SCALE_M=200 ./loadbench_tests --gtest_filter=*BlockGrowth*` |
| Reorg / scale | `src/reorg_tests.cpp`, `src/reorg_game_tests.cpp` | exact undo round-trip + idle-block cost at scale | `TF_REORG_N=20000 ./reorg_tests` |

Both `loadbench_tests` and the scale bench are `check_PROGRAMS` (compiled by `make check`,
never auto-run) and env-gated, so they don't slow the normal test gate but never bit-rot.

---

## Performance comparison

### 1. P-E1 — per-block write/scan cost on IDLE blocks  (the original "clogs the DB" complaint)

| Metric | Original TF | Rewrite | Why |
|---|---|---|---|
| per-block row-writes (idle) | O(all players) | O(players with ongoings) ≈ 0 | Stage 1 skip-when-empty guard (`28a841d`) |
| per-block scan time (idle, 50k) | ~643 ms | ~261 ms → ~0 after H3 | H3 event-driven ongoings (`aa60798`) moved ongoings out of the per-player proto into a height-keyed table |
| undo-log growth (1k active builds) | ~3.5 GB/day | ~0 + pruning | active build no longer re-writes its whole proto every block |

Full detail + raw numbers: `docs/p-e1-loadbench.md`. **Verdict: the original's idle-block
bloat is fixed** — an idle block now touches ~0 rows.

### 2. DEF3 — per-block cost grows with accumulated TOURNAMENTS  (found by the new scale bench)

The scale bench drove ~20k players through sustained real tournament play and exposed a
second, distinct unbounded cost the rewrite still inherited from the original: **completed
tournament rows are never deleted, and `ProcessTournaments` + `ReopenMissingTournaments`
re-scan the whole `tournaments` table (~25× per block, with proto deserialisation) every
block.** Per-block `UpdateState` time grows LINEARLY in the accumulated tournament-row
count — a permanent ratchet (every tournament ever played adds cost to every future block,
forever).

**BEFORE the fix (current/original behavior) — measured, non-vacuous (0 rejected moves, real completions):**

N=2,000 players, 120 blocks:
| % of run | block | tournament rows | UpdateState ms |
|---|---|---|---|
| 0% | 0 | 83 | 19 |
| 25% | 30 | 1,958 | 550 |
| 50% | 60 | 3,834 | 915 |
| 75% | 90 | 5,710 | 1,105 |
| 100% | 119 | 7,524 | 1,421 |

- mean ms/block: first-10% **32 ms** → last-10% **1,345 ms** = **41× growth** over 120 blocks.
- fit: **ms/block ≈ 280 + 0.151 × tournament_rows** (fighters table flat = control).

N=20,000 players (production scale):
| block | tournament rows | UpdateState ms |
|---|---|---|
| 20 | 12,520 | 13,788 |
| 40 | 25,020 | 30,012 |
| 60 | 37,520 | 44,627 |
| 80 | 50,020 | 59,895 |

- fit: **ms/block ≈ 1.22 × tournament_rows** — ~8× steeper per row than at 2k (the 100 MB+
  in-memory DB spills out of CPU cache, so DEF3 gets **worse** at scale). At block 80 a
  single block already costs **~60 s** of pure `UpdateState`.
- **Operational impact:** at ~2 s Polygon blocks, the GSP would fall behind under real
  tournament load and never recover. Deterministic (not a fork), but an operational
  showstopper at scale, and unbounded. No crashes/CHECK failures across 50k tournaments.

**AFTER the fix — measured, same bench (N=2,000, M=120; golden byte-identical, 98/98 + reorg green):**

The right read is **steady-state ms as the tournament backlog grows** (the headline
first-vs-last growth-factor is dominated by the cold-start ramp — block 0–11 at ~34 ms
before activity spins up — not by row growth):

| block | tournament rows | BEFORE ms | AFTER ms |
|---|---|---|---|
| 30 | 1,958 | 550 | **427** |
| 60 | 3,834 | 915 | **492** |
| 90 | 5,710 | 1,105 | **476** |
| 119 | 7,524 | 1,421 | **553** |

- BEFORE: steady-state climbs 550 → 1,421 ms as rows grow 3.8× (the DEF3 ratchet — superlinear).
- **AFTER: steady-state is FLAT, 427 → 553 ms for the same 3.8× row growth (1.29× — sublinear/bounded).** The per-block cost no longer grows with the accumulated backlog.
- Headline growth-factor 41× → 15×, but the residual 15× is cold-start ramp, not rows.
- Non-vacuity preserved: 7,504/7,504 entries, 6,566 completions, 200 cooks, 50 trades, **0 rejected**.

**Two unbounded per-block scans were fixed (the bench surfaced both):**
1. **DEF3 — tournament scan:** `ProcessTournaments`/`ReopenMissingTournaments` now query only
   active (Listed/Running) instances via a new indexed `state` column. Isolated saving grows
   **linearly with the completed backlog** (23 ms → 158 ms at 1,000 → 6,566 completed; → ~seconds at 50k).
2. **`rewards.CountForOwner` scan:** every reward roll/claim full-scanned the unbounded
   `rewards` table (no owner index). Added `rewards_by_owner` (mirrors `ongoing_operations_by_owner`).
   This was actually the larger term here (~680 ms/block at 6,566 completed). Golden-neutral.

**Disk bounded (GC proof):** completed tournaments are pruned to a retention cap
(`MAX_RETAINED_COMPLETED_TOURNAMENTS = 10000`, keeping recent results for the `UserTournaments`
RPC). Demo at cap=1,500: from block 60 on the prune fires every block — **completed plateaus at
exactly 1,500, the `tournaments` table plateaus at 2,458 rows** (958 active + 1,500 completed)
through the rest of the run even as 32,830 rewards accrue. Bounded.

Fix design: `state` column + `tournaments_by_state` index on `tournaments`;
`ProcessTournaments`/`ReopenMissingTournaments` use `QueryActive()` (golden byte-identical — completed
rows did zero work and drew no RNG); `PruneCompleted(keep)` (cheap COUNT probe, deletes only the
oldest excess) bounds disk while preserving recent results; reward-claims decoupled from the
tournament row (`ParseTournamentRewardData`) so late claims survive pruning; `rewards_by_owner` index.

**Still O(per-block) but CONSTANT in this bench → next scalability item (DEF2):** the residual
steady-state (~500 ms here) is dominated by `ReopenMissingTournaments`' per-block parse of the active
tournaments + reward generation for ~62 completions/block + the per-block full `FighterTable` scans in
`CheckFightersForSale`/`SetFreeTransfiguringFighters` (flat ~4,850 fighters here, but O(total fighters)
in production). DEF2 should get the same filtered-query + index treatment next.

### 3. Sync feasibility — the per-block floor over ~50M blocks  (DEF2 + ReopenMissing — FIXED, `6179a4a`)

Sync time ≈ (average per-block processing time) × (number of blocks). Three years of Polygon at
~1.5–2 s/block ≈ **~50 million blocks**, so a from-genesis sync is only practical if the AVERAGE per-block
time is a tiny fraction of the block interval. The worst block doesn't matter — the average over all ~50M does.

The catch (now fixed): the per-block MAINTENANCE — the fighter scans (DEF2: `CheckFightersForSale` /
`SetFreeTransfiguringFighters`) and `ReopenMissingTournaments` — ran on **every block, even empty ones**, and
its cost **grew with the size of the game**. Measured idle/empty-block floor (loadbench `TickCost`, no moves,
20 blocks), BEFORE vs AFTER the fix:

| idle block, players | BEFORE ms | AFTER ms | speedup |
|---|---|---|---|
| 1,000  | 3.24   | 0.33 | ~10× |
| 10,000 | 25.09  | 0.33 | 75× |
| 50,000 | 135.84 | 0.33 | **412×** |

The floor is now **FLAT at ~0.33 ms across 0–50k players** (was linear/superlinear in fighter count) —
**independent of game size**. From-genesis sync of a mature game drops from months/~1 year to well under a day;
the floor no longer climbs.

**Fix — DONE (`6179a4a`), same indexed pattern as DEF3, golden BYTE-IDENTICAL:**
- **DEF2** — `fighters_by_status` index + `FighterTable::QueryForStatus()`; the two per-block fighter scans now
  touch only Exchange/Transfiguring rows. Collect-then-mutate (ids first, flip after) so the status write never
  disturbs the status-keyed cursor mid-scan (the one real hazard).
- **`ReopenMissingTournaments`** — `tournaments_by_name_state` index + `QueryActiveForBlueprint()`; seeks one
  blueprint's active rows (the `name` column already holds the authoredid) instead of rescanning every active
  tournament once per blueprint.
- No schema column / proto change. Verified: golden byte-identical; 98 unit + 4 reorg + 2 reorg-game + 28 db green.

**Under FULL load it stays flat too** (scale bench, 20k players all entering tournaments + cooking + trading):
per-block time is identical at block 30 and block 60 (11,472 → 11,471 ms — those seconds are pure saturation,
~3,300 tournament resolutions/block, nothing like a gas-limited Polygon block) → **no ratchet** as state grows.
[Full curve + growth-factor + disk plateau + 0-rejected: filled in when the run completes.]

(DEF3 — already fixed — was the worst case: it grew UNBOUNDED, so a deep sync would never have finished at all.)

**Remaining periodic scan (flagged, not yet fixed): the every-100-block anti-fork hash** (`logic.cpp:239`,
`if (height % 100 == 0) { GetStateAsJson() … }`) rebuilds the ENTIRE game state as JSON (QueryAll on every
table) inline in `UpdateState`, every 100 blocks — the largest remaining O(game-size) scan in the sync path
(100× rarer than a per-block scan; transient memory, not disk). libxayagame already provides the right tool:
`xaya::SQLiteHasher` (a `SQLiteProcessor`) — registered via `AddProcessor` + `SetInterval(N)`, it SHA256-hashes
the DB **async on a read-only snapshot off the block thread**, persists `(blockhash, statehash)` into
`xayagame_statehashes` (so it is reorg-rolled-back — fixing audit finding **F4**), and the cadence is a real
arg. Soccerverse (`smcd`) and the xayaships example both wire it to a `--statehash_interval` gflags flag
(default 0 = off). Treatfighter is the outlier with a hardcoded inline `% 100` + a weak hash (row counts +
fighter names only) stored in non-rolled-back statics. **Recommendation:** replace the inline block with the
framework `SQLiteHasher` + a `--statehash_interval` flag (Soccerverse pattern). The hash is diagnostic/RPC-only
(`statehex`), not consensus, and not in golden — so this is consensus-safe. Deferred pending owner sign-off.

**Complementary mitigation:** node operators normally start from a published state **snapshot/checkpoint** at a
recent height rather than syncing from genesis (standard for libxayagame GSPs) — worth setting up too. With the
per-block floor now flat ~0.33 ms, from-genesis sync is also feasible.

---

## Issues found & fixed in the rewrite (consensus / correctness / scalability)

| # | Area | Issue | Severity | Status |
|---|---|---|---|---|
| Security C1–C9 | move pipeline | 6 move-reachable chain-halt crashes + 2 economic exploits (crystal-mint replay, test-content faucet) | chain/economy-fatal | DONE (security-audit.md) |
| OVF-01 | transfigure | candy amount fed unbounded into int32 `fpm::fixed_24_8` → signed-overflow UB at 2²³ | launch-blocker (consensus UB) | DONE (`f72ed26`) |
| Sentinel UB | transfigure | `fpm::fixed_24_8(9999999)` rating sentinels overflow int32 at construction | UB (deterministic) | DONE (`f72ed26`) |
| DEF3 | tournaments | completed rows never GC'd + re-scanned every block → unbounded per-block time | launch-blocker (scalability) | DONE — `QueryActive` filter + indexed `state` column + windowed retention GC (golden byte-identical; steady-state curve flattened, §2) |
| DEF2 | fighters / tournaments | `CheckFightersForSale` + `SetFreeTransfiguringFighters` full-scanned ALL fighters every block; `ReopenMissingTournaments` rescanned all active tournaments per blueprint → per-block floor grew with game size | launch-blocker (sync feasibility) | DONE (`6179a4a`) — `fighters_by_status` + `tournaments_by_name_state` indexes + filtered queries (collect-then-mutate); idle floor 135.8→0.33 ms @50k, now FLAT; golden byte-identical (§3) |
| F4 / statehash | anti-fork hash | inline `% 100` full-state JSON scan in `UpdateState`; weak hash (counts + fighter names) in non-rolled-back process statics | scalability + reorg-correctness (diagnostic, non-consensus) | OPEN — recommend libxayagame `xaya::SQLiteHasher` + `--statehash_interval` flag (§3, Soccerverse pattern) |
| rewards-idx | rewards | `CountForOwner`/`QueryForOwner` full-scanned the unbounded `rewards` table every reward roll/claim (no owner index) | scalability (found by the scale bench) | DONE — added `rewards_by_owner` index (golden-neutral) |
| DEF8–14 | determinism | raw float/double in consensus (probabilities, alms, exchange %, sweetness, prestige) | fork-risk discipline | DONE (Pass D) — now integer/fpm fixed-point; CI lint + `-ffp-contract=off` guardrails (`f72ed26`) |
| Quality audit | repo-wide | 80 findings: dead code, DRY, money-path correctness (reward-roll `<=`, transfigure fuel cost, armor-reward by-value, demand-queue double-append, role-load HALT, …) | mixed | DONE (quality-audit-findings.md) |
| F1/REORG-01 | pending | pending processing must not write the confirmed DB | reorg-safety | DONE (read-only flag + regression test) |
| P-E1 | ongoings | per-block O(players) write/scan (the "clogs the DB" root cause) | scalability | DONE (Stage 1 + H3) |

### Economy / product decisions (chain-safe, see `tf-economy-decisions` memory)
- SB-06 starter giveaway: **kept** (verified not exploitable — starter fighters account-bound, no item transfer, crystals WCHI-gated).
- Tournament JoinCost: **not added** (bots can't extract value or crowd out players; the real threat was DEF3 bloat, now fixed).
- Crystal economy: WCHI-gated, sink-heavy, not inflationary.

### Still open (non-blocking, deterministic)
- **Anti-fork state hash** (`logic.cpp:239` `% 100` inline FullState scan): replace with libxayagame's `xaya::SQLiteHasher` + a `--statehash_interval` flag (Soccerverse pattern) — async on a snapshot off the block thread, configurable cadence, reorg-safe (fixes audit **F4**), stronger SHA256-over-tables hash. Diagnostic/non-consensus, golden-safe. Deferred pending owner sign-off. See §3.
- Launch prep (non-code): confirm the real `dev_address` (still TEMP `0x59F5…`), a live Polygon + XayaX end-to-end test, a CI roconfig-blob hash assertion.

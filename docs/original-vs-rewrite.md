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

---

## Issues found & fixed in the rewrite (consensus / correctness / scalability)

| # | Area | Issue | Severity | Status |
|---|---|---|---|---|
| Security C1–C9 | move pipeline | 6 move-reachable chain-halt crashes + 2 economic exploits (crystal-mint replay, test-content faucet) | chain/economy-fatal | DONE (security-audit.md) |
| OVF-01 | transfigure | candy amount fed unbounded into int32 `fpm::fixed_24_8` → signed-overflow UB at 2²³ | launch-blocker (consensus UB) | DONE (`f72ed26`) |
| Sentinel UB | transfigure | `fpm::fixed_24_8(9999999)` rating sentinels overflow int32 at construction | UB (deterministic) | DONE (`f72ed26`) |
| DEF3 | tournaments | completed rows never GC'd + re-scanned every block → unbounded per-block time | launch-blocker (scalability) | DONE — `QueryActive` filter + indexed `state` column + windowed retention GC (golden byte-identical; steady-state curve flattened, §2) |
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
- DEF2 — per-block full FighterTable scans (`CheckFightersForSale` / `SetFreeTransfiguringFighters`): scalability, fix with the same filtered-query pattern as DEF3.
- Launch prep (non-code): confirm the real `dev_address` (still TEMP `0x59F5…`), a live Polygon + XayaX end-to-end test, a CI roconfig-blob hash assertion.

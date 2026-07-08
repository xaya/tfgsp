# Sweetness-scaled activity durations — design

**Status:** approved (brainstorm 2026-07-08), pending implementation plan.
**Consensus-critical:** YES — changes activity end-heights → golden-replay regen + genesis re-pin.

## Goal

The game currently runs at ~30s-per-block wall-clock assumptions but targets Polygon (~1.5s/block),
so everything resolves ~20× too fast. We do **not** want a flat 20× (that would punish onboarding).
Instead: keep newbie pacing ~unchanged and make the grind progressively harder toward the top, so
reaching the best treats takes real time. Concretely — scale every timed activity's duration by a
**geometric multiplier keyed on its sweetness tier**, from **1× at sw1** to **20× at sw10**, with the
top intensity **live-tunable** (no golden re-pin per tweak; one knob also absorbs a future 5s-superblock
switch).

## 1. The scaling function

A single pure, deterministic helper applied wherever a timed activity's end-height is set:

```
scaledDuration(base, sweetness, pct):
    s   = clamp(sweetness, 1, 10)
    // MULT is the fixed geometric SHAPE, precomputed at design time (no runtime float — respects the
    // float-ban). MULT[s] = round(256 * 20^((s-1)/9)), fixed-point with 256 = 1.0×.
    // 64-bit intermediate: 2800 * 5120 * 1000 overflows uint32.
    scaled = (uint64) base * MULT[s] * pct / (256 * 100)
    return max(1, (uint32) scaled)     // never 0 blocks
```

`MULT[1..10]` (constants, `256 == 1.0×`):

| sw | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|----|----|----|----|----|----|----|----|----|----|----|
| MULT | 256 | 357 | 498 | 695 | 969 | 1352 | 1886 | 2630 | 3669 | 5120 |
| ×   | 1.0 | 1.4 | 1.9 | 2.7 | 3.8 | 5.3 | 7.4 | 10.3 | 14.3 | 20.0 |

`pct` is the admin param **`duration_scale_pct`** (default **100**; range **1..1000**). 100 = the curve
above; **~30 for 5s superblocks** (keeps wall-clock identical); >100 = harsher. Seeded in
`database/params.cpp` beside `rarest_recipe_drop_divisor`; allow-listed + range-guarded in
`moveprocessor.cpp HandleSetParam`.

Representative end-to-end at pct=100 (base blocks → scaled blocks → wall-clock @1.5s):
sw1 15→15 (22s, unchanged) · sw5 350→1324 (33m) · sw7 700→5157 (2.1h) · sw10 2800→56000 (23.3h).

**Home:** a new pure helper (e.g. `ScaleActivityDuration` in `database/params.*` next to `GameParams`,
or a small `src/logic_duration.*`) so every activity start calls the identical code (DRY). Reads
`duration_scale_pct` via `GameParams(db).GetParam`.

## 2. What each activity keys its sweetness off

Applied at the point each activity's `duration` is finalized from its blueprint:

- **Expeditions** — `moveprocessor.cpp ParseExpeditionData`: key on the blueprint's `MinSweetness`.
- **Sweeteners** — the sweetener parse path: key on `RequiredSweetness`.
- **Cooks** — `moveprocessor.cpp ParseCookRecepie`: cooks have no sweetness gate, so map recipe
  **quality → sweetness**: Common(1)→1, Uncommon(2)→4, Rare(3)→7, Epic(4)→10. (Epic cook 120→~2400
  blocks ≈ 1h.) Encoded as a tiny fixed map beside the helper.
- **Tournaments** — `src/logic_tournament.cpp`: key on the tier blueprint's `MinSweetness`, applied when
  a tournament instance's resolve/duration is set. (Exact call site pinned in the plan.)
  **[Errata 2026-07-08: implemented as `MinSweetness`, NOT `MaxSweetness`. Keying on MaxSweetness would
  20×-slow the Min1/Max10 FTUE tutorial tournament (Dur2 → 40 blocks), violating this design's own
  "newbie pacing unchanged" goal; MinSweetness matches the expedition keying and preserves the endgame
  curve (Golden Ribbon Min10: 2800 → 56000 ≈ 23.3h). Verified: all 12 tutorial-tier tests stay green.]**

## 3. When it applies

Computed **once, at activity start**, and stored as the activity's end-height/`blocksleft`. A later live
`duration_scale_pct` change affects only **newly started** activities — in-flight expeditions / cooks /
sweeteners / tournaments keep the height they were assigned. No retroactive recomputation, no reorg or
consensus surprise from a mid-flight param change.

## 4. Separate, same effort: the exchange day-constant

`moveprocessor_exchange.cpp:281` hardcodes `blocksInOneDay = secondsInOneDay / 30` — a 30s-block
assumption, so a "3-day" listing currently expires in ~9 minutes at 1.5s blocks. This is **not**
sweetness-scaled; it's a flat block-time correction: `blocksInOneDay` should reflect 1.5s blocks
(86400 / 1.5 = **57600**, computed as integer `secondsInOneDay * 10 / 15`). Fixed alongside but as its
own change. (Any other `/ 30` block-time literals surfaced during implementation get the same flat
correction; the plan greps for them.)

## 5. Consensus, determinism & testing

- **Deterministic integer/fixed-point only** — MULT constants baked at design time; runtime is pure
  integer mult/div (64-bit intermediate); floored; clamped ≥1. No float (float-ban lint stays green).
- **Golden-replay regen + genesis re-pin** — end-heights change, so `goldenreplay.golden.json` is
  regenerated (after diff review) and the roconfig blob re-pinned.
- **Tests:**
  - Unit: `ScaleActivityDuration` for every sweetness 1..10 at pct=100 / 30 / 200; overflow guard
    (base 2800 × pct 1000); min-1 clamp; sweetness clamp <1 / >10; the rarity→sweetness map.
  - Per-activity: expedition/cook/sweetener/tournament each pick up the scaled height at start and are
    unaffected by a later param change (in-flight invariance).
  - Param: `duration_scale_pct` seeds to 100; `HandleSetParam` accepts in-range, refuses out-of-range.
  - Golden-replay pass; then fork e2e to feel the pacing.

## Out of scope

- Rebalancing the *reward-drop* rates (Epic 4× is already done separately).
- Any actual superblock mechanism (deferred; this design just leaves `duration_scale_pct` ready for it).
- Deep frontend work. In-progress activities are correct for free: the FE reads live `blocksleft` from
  the GSP (already scaled). The ONE gap is **pre-start** display — screens that show a blueprint's
  expected `Duration` before you commit (from `blueprints.ts`) will show the unscaled base. That's a
  display-only, non-consensus follow-up (mirror the same MULT curve + `duration_scale_pct` client-side,
  or have the GSP surface an effective duration); tracked separately, not in this consensus change.

## Files touched (indicative)

- `database/params.cpp` (seed `duration_scale_pct`), `database/params.hpp` (doc), `database/schema.sql`
  (comment).
- New helper + MULT table + rarity→sweetness map (`database/params.*` or `src/logic_duration.*`).
- `src/moveprocessor.cpp` (`ParseExpeditionData`, `ParseCookRecepie`, sweetener parse, `HandleSetParam`).
- `src/logic_tournament.cpp` (tournament duration).
- `src/moveprocessor_exchange.cpp:281` (exchange day-constant) + any sibling `/30` literals.
- Tests: `src/logic_tests.cpp` / `database/params_tests.cpp` (+ golden regen).

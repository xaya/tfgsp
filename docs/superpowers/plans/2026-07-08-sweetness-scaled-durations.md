# Sweetness-scaled activity durations — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Scale every timed activity's duration by a geometric multiplier keyed on its sweetness tier (1× at sw1 → 20× at sw10), live-tunable via one admin param, so newbie pacing is unchanged and the endgame grind gets much longer.

**Architecture:** A pure fixed-point helper `ScaleDuration(base, sweetness, pct)` (geometric shape baked as integer constants — no float) plus a `GameParams::ScaledDuration(base, sweetness)` wrapper that reads the tunable `duration_scale_pct` param. Every activity-start site (expedition / cook / sweetener / tournament) routes its final duration through the wrapper, keyed on that activity's sweetness gate. A separate flat fix corrects the exchange listing-expiry day-constant from a legacy 30s-block basis to 1.5s Polygon.

**Tech Stack:** C++17, libxayagame, SQLite, protobuf, gtest, autotools. Fixed-point via the existing `fpm::fixed_24_8`.

## Global Constraints

- **Consensus-critical:** activity end-heights change → golden-replay regen + genesis re-pin (Task 7). Every logic change must be deterministic.
- **Determinism / no float:** the geometric multiplier is a baked integer table (`DURATION_MULT`); runtime is integer/`uint64_t` math only. The float-ban lint must stay green.
- **Build only in Docker:** the native host cannot build the GSP. Build + test via the `tf-builder:local` image, `--network none`, using `scratchpad/gspcheck.sh` (incremental `make` + full `make check`). Golden regen runs inside the container: `GOLDEN_REGEN=1 ./tests --gtest_filter='GoldenReplay*'`, then copy `src/goldenreplay.golden.json` back into the tree.
- **Never rename** `recepie`/`recepies`/`ParseCookRecepie` (established misspelling).
- **Commit author:** `snailbrainx <17693211+snailbrainx@users.noreply.github.com>`. NEVER add a Claude trailer.
- **Branch:** `polygon-rewrite`.
- **The multiplier table** `DURATION_MULT[1..10]` = `{256, 357, 498, 695, 969, 1352, 1886, 2630, 3669, 5120}` (fixed-point, `256 == 1.0×`; = `round(256 · 20^((sw-1)/9))`). Denominator `256 · 100 = 25600`.
- **Cook rarity→sweetness map** `COOK_Q2S[0..4]` = `{1, 1, 4, 7, 10}` (index by `Quality`: None/Common→1, Uncommon→4, Rare→7, Epic→10).
- **Param:** `duration_scale_pct`, default `100`, range `[1, 1000]`.

---

### Task 1: The scaling core — `ScaleDuration` + `CookQualityToSweetness`

**Files:**
- Modify: `database/params.hpp` (declare two free functions, after the `GameParams` class)
- Modify: `database/params.cpp` (define the table, map, and functions)
- Test: `database/params_tests.cpp`

**Interfaces:**
- Produces: `int32_t pxd::ScaleDuration(int32_t base, uint32_t sweetness, int64_t pct)` and `uint32_t pxd::CookQualityToSweetness(uint32_t quality)` (free functions in the `pxd` namespace, same as `GameParams`).

- [ ] **Step 1: Write the failing tests** — append to `database/params_tests.cpp`, inside the existing `GameParamsTests` fixture block (it already has `GameParams gp; gp(db)`), and ensure the file can see the new free functions (they live in `database/params.hpp`, already included):

```cpp
TEST_F (GameParamsTests, ScaleDurationGeometricCurveAtDefaultPct)
{
  EXPECT_EQ (ScaleDuration (15, 1, 100), 15);        /* sw1 = 1.0x, unchanged */
  EXPECT_EQ (ScaleDuration (350, 5, 100), 1324);     /* sw5: 350*969/256 */
  EXPECT_EQ (ScaleDuration (700, 7, 100), 5157);     /* sw7: 700*1886/256 */
  EXPECT_EQ (ScaleDuration (2800, 10, 100), 56000);  /* sw10 = 20.0x */
}

TEST_F (GameParamsTests, ScaleDurationPctTunesIntensity)
{
  EXPECT_EQ (ScaleDuration (2800, 10, 30), 16800);   /* ~superblock (5s) setting: 20x*0.3 = 6x */
  EXPECT_EQ (ScaleDuration (2800, 10, 200), 112000); /* harsher */
}

TEST_F (GameParamsTests, ScaleDurationClampsSweetnessAndFloors)
{
  EXPECT_EQ (ScaleDuration (100, 0, 100), 100);      /* sweetness<1 -> sw1 (1.0x) */
  EXPECT_EQ (ScaleDuration (100, 99, 100), 2000);    /* sweetness>10 -> sw10 (20x) */
  EXPECT_EQ (ScaleDuration (1, 1, 1), 1);            /* result floored to >= 1 */
  EXPECT_EQ (ScaleDuration (0, 5, 100), 0);          /* non-positive base passes through */
}

TEST_F (GameParamsTests, ScaleDurationNoOverflowAtMax)
{
  EXPECT_EQ (ScaleDuration (2800, 10, 1000), 560000); /* 2800*5120*1000/25600, needs uint64 intermediate */
}

TEST_F (GameParamsTests, CookQualityMapsToTiers)
{
  EXPECT_EQ (CookQualityToSweetness (1), 1u);   /* Common */
  EXPECT_EQ (CookQualityToSweetness (2), 4u);   /* Uncommon */
  EXPECT_EQ (CookQualityToSweetness (3), 7u);   /* Rare */
  EXPECT_EQ (CookQualityToSweetness (4), 10u);  /* Epic */
  EXPECT_EQ (CookQualityToSweetness (0), 1u);   /* None -> floor tier */
}
```

- [ ] **Step 2: Run the tests, verify they FAIL to compile** (functions not declared):

```
docker run --rm --network none -v /home/acoloss/treatfighter/tfgsp-polygon:/src:ro tf-builder:local /src/scratchpad/gspcheck.sh
```
Expected: compile error — `ScaleDuration`/`CookQualityToSweetness` not declared.

- [ ] **Step 3: Declare the functions** in `database/params.hpp`, immediately after the closing `};` of `class GameParams` (still inside `namespace pxd`):

```cpp
/**
 * Geometric per-sweetness duration multiplier. base * (DURATION_MULT[clamp(sweetness,1,10)] * pct)
 * / 25600, in 64-bit, floored, min 1. Deterministic (no float): the geometric shape is a baked
 * integer table (256 == 1.0x, sw10 == 20.0x). `pct` is the duration_scale_pct param (100 = default).
 * A non-positive base passes through unchanged.
 */
int32_t ScaleDuration (int32_t base, uint32_t sweetness, int64_t pct);

/** Cook recipes have no sweetness gate; map recipe Quality (0..4) to a representative sweetness tier. */
uint32_t CookQualityToSweetness (uint32_t quality);
```

- [ ] **Step 4: Define them** in `database/params.cpp`. Add an anonymous-namespace constants block near the top (after includes / existing anon namespace if present), and the two definitions (anywhere in `namespace pxd`):

```cpp
namespace
{
/* Fixed geometric shape, 256 == 1.0x: DURATION_MULT[sw] = round (256 * 20^((sw-1)/9)).
   Index 0 is unused (sweetness is 1..10). */
constexpr int64_t DURATION_MULT[11]
    = {0, 256, 357, 498, 695, 969, 1352, 1886, 2630, 3669, 5120};

/* Cook Quality (None=0,Common=1,Uncommon=2,Rare=3,Epic=4) -> sweetness tier. */
constexpr uint32_t COOK_Q2S[5] = {1, 1, 4, 7, 10};
} // namespace

int32_t
ScaleDuration (int32_t base, uint32_t sweetness, int64_t pct)
{
  if (base <= 0)
    return base;
  uint32_t s = sweetness;
  if (s < 1) s = 1;
  if (s > 10) s = 10;
  const uint64_t scaled = (uint64_t) base * (uint64_t) DURATION_MULT[s]
                          * (uint64_t) pct / 25600ULL;
  return scaled < 1 ? 1 : (int32_t) scaled;
}

uint32_t
CookQualityToSweetness (uint32_t quality)
{
  if (quality > 4)
    quality = 4;
  return COOK_Q2S[quality];
}
```

- [ ] **Step 5: Run the tests, verify PASS:**

```
docker run --rm --network none -v /home/acoloss/treatfighter/tfgsp-polygon:/src:ro tf-builder:local /src/scratchpad/gspcheck.sh
```
Expected: `CHECK_ALL_PASS`, the six new tests green.

- [ ] **Step 6: Commit:**

```bash
git add database/params.hpp database/params.cpp database/params_tests.cpp
git commit --author="snailbrainx <17693211+snailbrainx@users.noreply.github.com>" \
  -m "feat(gsp): geometric ScaleDuration helper + cook rarity->sweetness map"
```

---

### Task 2: The `duration_scale_pct` param + `GameParams::ScaledDuration`

**Files:**
- Modify: `database/params.hpp` (add the `ScaledDuration` method to `GameParams`)
- Modify: `database/params.cpp` (seed the param; define the method)
- Modify: `src/moveprocessor.cpp` (`HandleSetParam` allow-list + range guard, lines 188-259)
- Test: `database/params_tests.cpp`

**Interfaces:**
- Consumes: `ScaleDuration` (Task 1).
- Produces: `int32_t GameParams::ScaledDuration(int32_t base, uint32_t sweetness)` — reads `duration_scale_pct` and applies `ScaleDuration`. Used by every activity-start site (Tasks 3-6).

- [ ] **Step 1: Write the failing tests** — append to `database/params_tests.cpp`:

```cpp
TEST_F (GameParamsTests, DurationScalePctSeededToDefault)
{
  EXPECT_EQ (gp.GetParam ("duration_scale_pct"), 100);
}

TEST_F (GameParamsTests, ScaledDurationUsesSeededParam)
{
  EXPECT_EQ (gp.ScaledDuration (2800, 10), 56000); /* default pct 100 -> 20x */
  gp.SetParam ("duration_scale_pct", 30);
  EXPECT_EQ (gp.ScaledDuration (2800, 10), 16800); /* live retune -> 6x */
}
```

- [ ] **Step 2: Run, verify FAIL** (`ScaledDuration` not a member; param unseeded):

```
docker run --rm --network none -v /home/acoloss/treatfighter/tfgsp-polygon:/src:ro tf-builder:local /src/scratchpad/gspcheck.sh
```
Expected: compile error / `GetParam("duration_scale_pct")` CHECK-fail.

- [ ] **Step 3a: Seed the param** — in `database/params.cpp`, in `GameParams::InitialiseDatabase()` (currently seeds 4 params at lines 87-93), add:

```cpp
  SetParam ("duration_scale_pct", 100);
```

- [ ] **Step 3b: Declare the method** — in `database/params.hpp`, inside `class GameParams` public section (after `RemoveParam`):

```cpp
  /** base duration scaled by the geometric sweetness curve at the live duration_scale_pct. */
  int32_t ScaledDuration (int32_t base, uint32_t sweetness);
```

- [ ] **Step 3c: Define the method** — in `database/params.cpp`:

```cpp
int32_t
GameParams::ScaledDuration (int32_t base, uint32_t sweetness)
{
  return ScaleDuration (base, sweetness, GetParam ("duration_scale_pct"));
}
```

- [ ] **Step 3d: Allow-list + range-guard the param** — in `src/moveprocessor.cpp` `HandleSetParam`: add to the `known` disjunction (after `tournament_max_captures`):

```cpp
                       || name == "duration_scale_pct"
```
and add to the range-guard chain (after the `tournament_max_captures` branch):

```cpp
      else if (name == "duration_scale_pct")
        ok = (v >= 1 && v <= 1000);
```

- [ ] **Step 4: Run, verify PASS** (same command). Expected: `CHECK_ALL_PASS`, both new tests green, `SeededDefaultsPresent` still green.

- [ ] **Step 5: Commit:**

```bash
git add database/params.hpp database/params.cpp database/params_tests.cpp src/moveprocessor.cpp
git commit --author="snailbrainx <17693211+snailbrainx@users.noreply.github.com>" \
  -m "feat(gsp): duration_scale_pct param (default 100) + GameParams::ScaledDuration"
```

---

### Task 3: Apply the scale to expeditions

**Files:**
- Modify: `src/moveprocessor_activity.cpp` (`ParseExpeditionData`, duration assignment at lines 461-467)
- Test: whichever suite starts an expedition and asserts its `blocksleft`/duration (grep — see Step 3).

**Interfaces:** Consumes `GameParams::ScaledDuration` (Task 2). Keyed on `expeditionBlueprint.minsweetness()`.

- [ ] **Step 1: Confirm the include.** Ensure `src/moveprocessor_activity.cpp` includes `database/params.hpp`; if not, add `#include "database/params.hpp"` with the other database includes.

- [ ] **Step 2: Make the code change.** In `ParseExpeditionData`, replace the final duration assignment (currently `duration = (int32_t)effective_duration;` at ~line 467):

```cpp
    duration = GameParams (db).ScaledDuration ((int32_t) effective_duration,
                                               expeditionBlueprint.minsweetness ());
```
(`db` is the `BaseMoveProcessor` member, in scope in this method. The espresso-goody reduction still applies first — the scale multiplies the post-goody `effective_duration`.)

- [ ] **Step 3: Update the affected tests (RED→GREEN via the build).** Grep for expedition-start duration assertions:

```
grep -rnE 'expedition|Expedition' src/moveprocessor_tests.cpp src/logic_tests.cpp | grep -iE 'blocksleft|duration|EXPECT'
```
For each test that asserts a pre-scale expedition duration, replace the expected value with `ScaleDuration(base, minSweetness, 100)`. Compute it as `base * DURATION_MULT[minSweetness] / 256` (floored). Example: a sw9 (`MinSweetness 9`, base `1400`) expedition → `1400*3669/256 = 20064`. A sw1 (base `15`) expedition is unchanged (`15`).

- [ ] **Step 4: Build + test:**

```
docker run --rm --network none -v /home/acoloss/treatfighter/tfgsp-polygon:/src:ro tf-builder:local /src/scratchpad/gspcheck.sh
```
Expected: `CHECK_ALL_PASS` (GoldenReplay may fail here — that is expected and fixed in Task 7; note it and continue).

- [ ] **Step 5: Commit:**

```bash
git add src/moveprocessor_activity.cpp src/moveprocessor_tests.cpp src/logic_tests.cpp
git commit --author="snailbrainx <17693211+snailbrainx@users.noreply.github.com>" \
  -m "feat(gsp): scale expedition duration by MinSweetness"
```

---

### Task 4: Apply the scale to cooks and sweeteners

**Files:**
- Modify: `src/moveprocessor_cooking.cpp` (`ParseCookRecepie` duration at ~line 448; `ParseSweetener` duration at ~line 268)
- Test: cook/sweetener suites (grep — Step 4).

**Interfaces:** Consumes `GameParams::ScaledDuration` and `CookQualityToSweetness`. Cook keyed on `CookQualityToSweetness(itemInventoryRecipe->GetProto().quality())`; sweetener keyed on `sweetenerBlueprint.requiredsweetness()`.

- [ ] **Step 1: Confirm the include** in `src/moveprocessor_cooking.cpp` (`#include "database/params.hpp"`).

- [ ] **Step 2: Cook change.** In `ParseCookRecepie`, replace `duration = (int32_t)effective_duration;` (~line 448):

```cpp
    duration = GameParams (db).ScaledDuration (
        (int32_t) effective_duration,
        CookQualityToSweetness (itemInventoryRecipe->GetProto ().quality ()));
```

- [ ] **Step 3: Sweetener change.** In `ParseSweetener`, replace `duration = sweetenerBlueprint.duration();` (~line 268):

```cpp
     duration = GameParams (db).ScaledDuration (
         sweetenerBlueprint.duration (), sweetenerBlueprint.requiredsweetness ());
```

- [ ] **Step 4: Update affected tests.** Grep:

```
grep -rnE 'cook|Cook|sweeten|Sweeten' src/moveprocessor_tests.cpp src/logic_tests.cpp | grep -iE 'blocksleft|duration|EXPECT'
```
Update each pre-scale expected duration to the scaled value. Cook: `base * DURATION_MULT[COOK_Q2S[quality]] / 256`. Sweetener: `base * DURATION_MULT[requiredsweetness] / 256`. (e.g. a sweetener with `RequiredSweetness 5`, base `50` → `50*969/256 = 189`.)

- [ ] **Step 5: Build + test** (same docker command). Expected `CHECK_ALL_PASS` (GoldenReplay still expected-fail until Task 7).

- [ ] **Step 6: Commit:**

```bash
git add src/moveprocessor_cooking.cpp src/moveprocessor_tests.cpp src/logic_tests.cpp
git commit --author="snailbrainx <17693211+snailbrainx@users.noreply.github.com>" \
  -m "feat(gsp): scale cook (by rarity) and sweetener (by RequiredSweetness) durations"
```

---

### Task 5: Apply the scale to tournaments

**Files:**
- Modify: `src/logic_tournament.cpp` (the `Listed`→`Running` transition, line 134)
- Test: tournament suite in `src/logic_tests.cpp` (grep — Step 3).

**Interfaces:** Consumes `GameParams::ScaledDuration`. Keyed on `tnm->GetProto().maxsweetness()`.

- [ ] **Step 1: Confirm the include** in `src/logic_tournament.cpp` (`#include "database/params.hpp"`; the file already uses `GameParams` for capture params, so it is present — verify).

- [ ] **Step 2: Make the change.** At line 134, replace:

```cpp
          tnm->MutableInstance().set_blocksleft(tnm->GetProto().duration());
```
with:

```cpp
          tnm->MutableInstance ().set_blocksleft (
              GameParams (db).ScaledDuration (tnm->GetProto ().duration (),
                                              tnm->GetProto ().maxsweetness ()));
```
(`db` is in scope in this tournament-processing function — the surrounding code already reads `GameParams(db)` for capture params; reuse the same `db`.)

- [ ] **Step 3: Update affected tests.** Grep:

```
grep -rnE 'ournament' src/logic_tests.cpp | grep -iE 'blocksleft|duration|EXPECT'
```
Update each tournament-start `blocksleft` expectation to `base * DURATION_MULT[maxSweetness] / 256`. (e.g. a tier with `Duration 15`, `MaxSweetness 3` → `15*498/256 = 29`.) The FTUE/low tiers with small MaxSweetness barely change; high tiers scale up.

- [ ] **Step 4: Build + test** (same docker command). Expected `CHECK_ALL_PASS` (GoldenReplay expected-fail until Task 7).

- [ ] **Step 5: Commit:**

```bash
git add src/logic_tournament.cpp src/logic_tests.cpp
git commit --author="snailbrainx <17693211+snailbrainx@users.noreply.github.com>" \
  -m "feat(gsp): scale tournament duration by tier MaxSweetness"
```

---

### Task 6: Fix the exchange day-constant (flat 1.5s-block correction)

**Files:**
- Modify: `src/moveprocessor_exchange.cpp:281`
- Modify: `src/moveprocessor_tests.cpp:708` and `:776` (the two mirror computations)

**Interfaces:** none (independent flat fix).

- [ ] **Step 1: Fix production code.** In `src/moveprocessor_exchange.cpp`, replace line 281:

```cpp
    int32_t blocksInOneDay = secondsInOneDay / 30;
```
with:

```cpp
    int32_t blocksInOneDay = secondsInOneDay * 2 / 3;   /* ~1.5s Polygon blocks (was /30, a legacy 30s-block basis) */
```
(`86400 * 2 / 3 = 57600` blocks/day.)

- [ ] **Step 2: Fix the two test mirrors.** In `src/moveprocessor_tests.cpp` at lines ~708 and ~776, replace each `int blocksInOneDay = secondsInOneDay / 30;` with `int blocksInOneDay = secondsInOneDay * 2 / 3;`. The tests assert `exchangeexpire == Height + blocksInOneDay * 3`, so they self-update to the new constant.

- [ ] **Step 3: Build + test** (same docker command). Expected: the two exchange tests green with the new constant.

- [ ] **Step 4: Commit:**

```bash
git add src/moveprocessor_exchange.cpp src/moveprocessor_tests.cpp
git commit --author="snailbrainx <17693211+snailbrainx@users.noreply.github.com>" \
  -m "fix(gsp): exchange listing expiry uses 1.5s-block day (57600), not legacy 30s"
```

---

### Task 7: Regenerate the golden replay + full green

**Files:**
- Modify: `src/goldenreplay.golden.json` (regenerated)

**Interfaces:** none.

- [ ] **Step 1: Review the golden diff intent.** The golden replay drives activities whose `blocksleft` now changes (scaled): e.g. the sw9/sw10 expeditions at `1400`/`2800` become `20064`/`56000`, sw1/sw2 at `15`/`30` stay near-unchanged. Confirm these are the ONLY observable changes expected (blocksleft/exchangeexpire), not reward RNG or ownership.

- [ ] **Step 2: Regenerate inside the container** on a WRITABLE mount (drop `:ro`). First read `scratchpad/gspcheck.sh` to get the exact build dir and `tests` binary path it uses; then build once and run the golden target with `GOLDEN_REGEN=1` on the `GoldenReplay*` filter:

```
docker run --rm --network none -v /home/acoloss/treatfighter/tfgsp-polygon:/src tf-builder:local \
  bash -lc 'set -e; <build-as-in-gspcheck.sh>; GOLDEN_REGEN=1 <path/to/tests-binary> --gtest_filter="GoldenReplay*"'
```
Then confirm `src/goldenreplay.golden.json` was rewritten (it is written into the source tree by the regen). If `gspcheck.sh` builds into an internal copy, ensure the regenerated golden is copied back to the mounted `/src/src/goldenreplay.golden.json`.

- [ ] **Step 3: Diff-review the regenerated golden.** `git diff src/goldenreplay.golden.json` — verify ONLY `blocksleft` (and any `exchangeexpire`) values changed, and each changed value equals `ScaleDuration(oldValue, sweetness, 100)` for its activity (spot-check 3-4). No reward/ownership/serial drift.

- [ ] **Step 4: Full make check green:**

```
docker run --rm --network none -v /home/acoloss/treatfighter/tfgsp-polygon:/src:ro tf-builder:local /src/scratchpad/gspcheck.sh
```
Expected: `CHECK_ALL_PASS` including `GoldenReplay*`, unit, reorg, reorg_game.

- [ ] **Step 5: Commit:**

```bash
git add src/goldenreplay.golden.json
git commit --author="snailbrainx <17693211+snailbrainx@users.noreply.github.com>" \
  -m "chore(gsp): regenerate golden replay for sweetness-scaled durations"
```

- [ ] **Step 6 (out-of-band, NOT a code commit): genesis re-pin.** Flag to the human that the roconfig blob / genesis must be re-pinned before mainnet (per the launch punch-list). This plan does not itself re-pin; it is a launch-gating follow-up.

---

## Post-plan (separate, tracked elsewhere)

- **Frontend pre-start duration display** (`tf-frontend/src/data/blueprints.ts` consumers): shows unscaled base durations before an activity starts. Display-only, non-consensus; mirror the curve client-side or surface an effective duration from the GSP. Not in this plan.
- **Fork e2e** to feel the new pacing, then deploy (rebuild `tfd-polygon:latest`, recreate the fork GSP).

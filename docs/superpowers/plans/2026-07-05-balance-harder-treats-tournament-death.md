# Balance: harder rarest treats + tournament permadeath — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make Epic treats ~4× rarer, add tournament permadeath with a 50/50 destroy-or-capture coin-flip, put both behind live admin-tunable game-state parameters, and give the loser a required client reveal when a treat is lost.

**Architecture:** Three GSP changes plus a client reveal. (0) A new `parameters` KV table in game state (Soccerverse pattern) read at the point of use and set live via a `g/tf` admin `param` move. (B) A single shared weighted-pick helper scales every non-Epic reward weight by the divisor, quartering Epic's relative odds across all five reward-roll sites. (A) The tournament-resolution loop, after ELO, makes each losing team lose one RNG-picked entered fighter — starters immune, else a `capture_pct/256` coin-flip transfers it to the winner (roster-cap permitting) or destroys it. (C) Every loss writes a client-visible `battle_losses` row and bumps the loser's `rewards_serial`; the frontend reveals it via a new `FighterLossModal`. GSP lands first; the frontend depends on the new `battlelosses` wire field.

**Tech Stack:** C++17, libxayagame, SQLite, protobuf, Autotools, gtest (GSP `tfgsp-polygon`, branch `polygon-rewrite`). Next 16, viem, vitest (frontend `tf-frontend`, branch `master`).

## Global Constraints

- **Seeded param defaults (genesis):** `rarest_recipe_drop_divisor = 4`, `tournament_loss_kills_enabled = 1`, `tournament_capture_pct = 128` (128/256 = 50%).
- **Admin `param` move shape:** `{"cmd":{"param":[{"n":"<name>","v":<int>}]}}` on the `g/tf` name (delivered as `blockData["admin"]`). Auth is chain-level only (whoever owns `g/tf`); NOT god-gated; NO in-code address gate.
- **Admin range guards (soft-fail, `LOG(WARNING)` + skip, never `CHECK`/crash):** divisor ∈ `[1, 100000]`; `tournament_capture_pct` ∈ `[0, 256]`; `tournament_loss_kills_enabled` ∈ `{0, 1}`. Unknown param names rejected. Removal (`v:null`) of any of the three required knobs is **refused** (GetParam CHECK-fails on unset → removing one would brick consensus).
- **Epic predicate (the only entry spared from divisor scaling):** `(RewardType)(int32_t)rw.type() == RewardType::GeneratedRecipe && (pxd::Quality)(int32_t)rw.generatedrecipequality() == pxd::Quality::Epic` (i.e. `type()==1 && generatedrecipequality()==4`). Rare (q3) IS scaled.
- **Permadeath rules:** only when `tournament_loss_kills_enabled==1` AND the tournament has a non-empty `winnerid`. Process non-winning teams in ascending owner-name order (iterate the `std::map<std::string,std::vector<uint32_t>> teams`). One victim draw per losing team (`rnd.NextInt(roster.size())`), always taken. Account-bound (starter) victim → nothing happens, and NO capture draw is taken. Otherwise draw `roll = rnd.NextInt(256)`; capture iff `roll < capture_pct` AND `fighters.CountForOwner(winnerName) < max_fighter_inventory_amount()`; else destroy. Draw/no-winner → no deaths.
- **Loss record + notification (required):** every destroy/capture writes a `battle_losses` row `{owner, fighterid, name, outcome (0=destroyed,1=captured), opponent, tournamentid}` and bumps that loser's `rewards_serial` by 1. No fighter leaves a roster without a recorded, revealable loss.
- **Determinism:** single per-block `xaya::Random& rnd` stream. All new draws sit in a fixed order (reward rolls first, then permadeath: teams ascending, victim draw then capture draw). Serial bumps and DB writes consume NO rnd. No `float`/`double` anywhere in `src/` or `database/` (the `check-local` linter `contrib/check-consensus-determinism.py` fails the build otherwise); the only math here is integer.
- **No RoConfig proto/blob change.** Params live in game-state, not RoConfig. Therefore `proto2/roconfig_tests.cpp::LaunchConfigIsPinned` (`PINNED_HASH`) is UNCHANGED and must not be touched. What DOES change is the genesis STATE (new tables + seeded rows + RNG outcomes) → **golden replay must be regenerated** and the mainnet genesis re-pin folds into the existing pre-launch blocker (no live migration; fresh genesis).
- **Build/test:** GSP compiles ONLY in Docker `tf-builder:local` with `--network none`, building an internal copy, `make check` as the gate. The native host has no compiler. Golden regen ONLY after diff review: `cd src && GOLDEN_REGEN=1 ./goldenreplay_tests`. See the "Building & testing the GSP" section below for the exact command.
- **Never rename** `recepie`/`recepies`/`Recepie` (intentional legacy spelling).

---

## Building & testing the GSP (read before any GSP task)

The native host cannot compile. Every GSP task's "run the tests" step means running `make check` inside the prebuilt `tf-builder:local` image, offline, against an internal copy of the repo (keeps the host tree pristine, no root-owned artifacts). Canonical command (run from anywhere):

```bash
docker run --rm --network none \
  -v /home/acoloss/treatfighter/tfgsp-polygon:/src-ro:ro \
  -v /tmp/tf-ccache:/ccache \
  --entrypoint bash tf-builder:local -c '
    set -e
    export CCACHE_DIR=/ccache CC="ccache gcc" CXX="ccache g++"
    cp -a /src-ro /build && cd /build
    make distclean 2>/dev/null || true
    ./autogen.sh && ./configure && make -j"$(nproc)"
    make check || (cat src/test-suite.log proto2/test-suite.log database/test-suite.log 2>/dev/null; exit 1)
  '
```

- The `-v /tmp/tf-ccache:/ccache` mount + `ccache` wrappers make incremental rebuilds fast after the first full build (seconds vs minutes) — this is how we "speed it up". `mkdir -p /tmp/tf-ccache` once on the host.
- A `make check` failure cats the three `test-suite.log` files. `database/` tests, `src/` tests (`tests`, `goldenreplay_tests`, `reorg_tests`, `reorg_game_tests`), and `proto2/` tests all run, then `check-local` runs the float/double linter.
- **Golden regen** (only after reviewing the mismatch diff): add a second step inside the container, `cd src && GOLDEN_REGEN=1 ./goldenreplay_tests`, then `cp /build/src/goldenreplay.golden.json` back out to the host repo. Because the container's `/build` is a copy, the practical flow is: run the regen inside a container that bind-mounts a writable scratch dir, or run the build with the host repo bind-mounted read-write for the regen pass only. Simplest: after review, run the same container with `-v /home/acoloss/treatfighter/tfgsp-polygon:/build-rw` and regen there. (The controller handles the golden-regen mechanics in Task 7.)

For **frontend** tasks, build/test natively in `tf-frontend`: `npm test` (vitest) and `npm run build`.

---

## File Structure

**GSP (`tfgsp-polygon`):**

- `database/schema.sql` — MODIFY: add `parameters` table (Task 1) and `battle_losses` table + index (Task 4).
- `database/params.hpp` / `database/params.cpp` — CREATE: `GameParams` accessor (Task 1).
- `database/params_tests.cpp` — CREATE: GameParams unit tests (Task 1).
- `database/battlelosses.hpp` / `database/battlelosses.cpp` — CREATE: `BattleLossesTable` (Task 4).
- `database/battlelosses_tests.cpp` — CREATE: BattleLossesTable unit tests (Task 4).
- `database/Makefile.am` — MODIFY: register the four new source/header/test files (Tasks 1, 4).
- `database/dbtest.cpp` — MODIFY: seed `GameParams` in the test-DB schema setup (Task 1).
- `src/logic.cpp` — MODIFY: seed `GameParams` in `InitialiseState` (Task 1).
- `src/moveprocessor.hpp` / `src/moveprocessor.cpp` — MODIFY: `HandleSetParam` admin verb (Task 2).
- `src/moveprocessor_tests.cpp` — MODIFY: admin `param` move tests (Task 2).
- `src/logic.hpp` — MODIFY: declare `ScaledRewardWeights` + `PickWeightedRewardIndex` statics (Task 3).
- `src/logic_resolve.cpp` — MODIFY: define the helpers; replace the 4 pick loops (Task 3).
- `src/logic_tournament.cpp` — MODIFY: use the helper for the tournament reward roll (Task 3); add permadeath/capture + loss record + serial bump (Task 5).
- `src/gamestatejson.cpp` — MODIFY: surface `battlelosses` in `getuser` (Task 4).
- `src/logic_tests.cpp` — MODIFY: divisor unit test (Task 3), permadeath integration tests (Task 5).
- `src/reorg_tests.cpp` — MODIFY: a captured fighter reverts ownership on reorg (Task 7).
- `src/goldenreplay.golden.json` — REGENERATE (Task 7).

**Frontend (`tf-frontend`):**

- `src/data/types.ts` — MODIFY: `BattleLoss` type, `UserSlice.battlelosses`, parse it (Task 6).
- `src/data/reward-reveal.ts` — MODIFY: `InvSnapshot.losses`, `RewardWon.lostFighters`, `empty` (Task 6).
- `src/ui/chrome/RevealSwitch.tsx` — MODIFY: loss-first branch (Task 6).
- `src/ui/chrome/FighterLossModal.tsx` — CREATE: the "you lost a treat" reveal (Task 6).
- `app/page.tsx` — MODIFY: add `lostFighters: []` to the empty-state literal (Task 6).
- `tests/data/reward-reveal.test.ts` — MODIFY: `lostFighters` test (Task 6).

---

## Task 1: `parameters` table + `GameParams` accessor + genesis/test seeding

**Files:**
- Modify: `database/schema.sql` (add the `parameters` table)
- Create: `database/params.hpp`, `database/params.cpp`, `database/params_tests.cpp`
- Modify: `database/Makefile.am` (register the three files)
- Modify: `src/logic.cpp` (seed at genesis, ~line 199)
- Modify: `database/dbtest.cpp` (seed in test-DB setup, ~line 53)

**Interfaces:**
- Produces: `class pxd::GameParams` with `int64_t GetParam(const std::string&)` (CHECK-fails on unset), `bool HasParam(const std::string&)`, `void SetParam(const std::string&, int64_t)`, `void RemoveParam(const std::string&)`, `void InitialiseDatabase()`. Constructed as `GameParams(Database&)` — including as a temporary `GameParams(db).GetParam("...")`. Seeded defaults: `rarest_recipe_drop_divisor=4`, `tournament_loss_kills_enabled=1`, `tournament_capture_pct=128`.

- [ ] **Step 1: Write the failing test** — create `database/params_tests.cpp`:

```cpp
/*
    GSP for the tf blockchain game
    Copyright (C) 2026  Autonomous Worlds Ltd

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

#include "params.hpp"

#include "dbtest.hpp"

#include <gtest/gtest.h>

namespace pxd
{
namespace
{

class GameParamsTests : public DBTestWithSchema
{
protected:
  GameParams gp;
  GameParamsTests () : gp(db) {}
};

TEST_F (GameParamsTests, SeededDefaultsPresent)
{
  EXPECT_EQ (gp.GetParam ("rarest_recipe_drop_divisor"), 4);
  EXPECT_EQ (gp.GetParam ("tournament_loss_kills_enabled"), 1);
  EXPECT_EQ (gp.GetParam ("tournament_capture_pct"), 128);
}

TEST_F (GameParamsTests, SetUpdatesValue)
{
  gp.SetParam ("rarest_recipe_drop_divisor", 8);
  EXPECT_EQ (gp.GetParam ("rarest_recipe_drop_divisor"), 8);
}

TEST_F (GameParamsTests, HasAndRemove)
{
  EXPECT_TRUE (gp.HasParam ("tournament_capture_pct"));
  gp.RemoveParam ("tournament_capture_pct");
  EXPECT_FALSE (gp.HasParam ("tournament_capture_pct"));
}

TEST_F (GameParamsTests, GetUnsetCheckFails)
{
  EXPECT_DEATH (gp.GetParam ("nonexistent_param"), "unset parameter");
}

} // anonymous namespace
} // namespace pxd
```

- [ ] **Step 2: Add the schema** — in `database/schema.sql`, after the `globaldata` block (ends ~line 72), add:

```sql
-- Runtime-tunable game parameters (Soccerverse `parameters` pattern): a small
-- KV table set live via a g/tf admin `param` move, so the balance knobs can be
-- retuned without a rebuild/re-pin.  Seeded at genesis (GameParams::
-- InitialiseDatabase); GameParams::GetParam CHECK-fails on an unset name so the
-- value is always the on-chain seeded/admin value (never a drifting C++ literal
-- / silent hard fork).  Added the additive `IF NOT EXISTS` way, same as
-- ongoing_operations, so it perturbs no existing table's bytes.
CREATE TABLE IF NOT EXISTS `parameters` (
  `name` TEXT PRIMARY KEY,
  `value` INTEGER NOT NULL
);
```

- [ ] **Step 3: Create `database/params.hpp`:**

```cpp
/*
    GSP for the tf blockchain game
    Copyright (C) 2026  Autonomous Worlds Ltd
    <GPL header — copy verbatim from database/globaldata.hpp lines 1-17>
*/

#ifndef DATABASE_PARAMS_HPP
#define DATABASE_PARAMS_HPP

#include "database.hpp"

#include <cstdint>
#include <string>

namespace pxd
{

/**
 * Runtime-tunable game parameters, held in the `parameters` KV table
 * (name TEXT PRIMARY KEY -> value INTEGER).  Mirrors Soccerverse's params table
 * so the balance knobs (rarest_recipe_drop_divisor / tournament_loss_kills_enabled
 * / tournament_capture_pct) can be retuned live via a g/tf admin `param` move
 * with no rebuild.  Every knob is seeded at genesis (InitialiseDatabase), so
 * GetParam CHECK-fails on an unset name -- the authoritative value is always the
 * on-chain seeded/admin value, never a drifting C++ literal (no silent hard fork).
 */
class GameParams
{

private:

  /** The underlying database handle.  */
  Database& db;

public:

  explicit GameParams (Database& d) : db(d) {}

  GameParams () = delete;
  GameParams (const GameParams&) = delete;
  void operator= (const GameParams&) = delete;

  /** Returns the value for `name`; CHECK-fails if the name was never set.  */
  int64_t GetParam (const std::string& name);

  /** Returns true iff `name` has a row.  */
  bool HasParam (const std::string& name);

  /** Inserts or updates `name` -> `value`.  */
  void SetParam (const std::string& name, int64_t value);

  /** Deletes `name` (no-op if absent).  */
  void RemoveParam (const std::string& name);

  /** Seeds the launch defaults; called once at genesis and in test-DB setup.  */
  void InitialiseDatabase ();

};

} // namespace pxd

#endif // DATABASE_PARAMS_HPP
```

- [ ] **Step 4: Create `database/params.cpp`:**

```cpp
/*
    <GPL header — copy verbatim from database/globaldata.cpp lines 1-17>
*/

#include "params.hpp"

#include <glog/logging.h>

namespace pxd
{

namespace
{

struct ParamResult : public Database::ResultType
{
  RESULT_COLUMN (std::string, name, 1);
  RESULT_COLUMN (int64_t, value, 2);
};

} // anonymous namespace

int64_t
GameParams::GetParam (const std::string& name)
{
  auto stmt = db.Prepare (R"(
    SELECT `name`, `value` FROM `parameters` WHERE `name` = ?1
  )");
  stmt.Bind (1, name);

  auto res = stmt.Query<ParamResult> ();
  CHECK (res.Step ()) << "GameParams::GetParam: unset parameter '" << name << "'";
  const int64_t value = res.Get<ParamResult::value> ();
  CHECK (!res.Step ());

  return value;
}

bool
GameParams::HasParam (const std::string& name)
{
  auto stmt = db.Prepare (R"(
    SELECT `name`, `value` FROM `parameters` WHERE `name` = ?1
  )");
  stmt.Bind (1, name);

  auto res = stmt.Query<ParamResult> ();
  return res.Step ();
}

void
GameParams::SetParam (const std::string& name, const int64_t value)
{
  auto stmt = db.Prepare (R"(
    INSERT OR REPLACE INTO `parameters` (`name`, `value`) VALUES (?1, ?2)
  )");
  stmt.Bind (1, name);
  stmt.Bind (2, value);
  stmt.Execute ();
}

void
GameParams::RemoveParam (const std::string& name)
{
  auto stmt = db.Prepare (R"(
    DELETE FROM `parameters` WHERE `name` = ?1
  )");
  stmt.Bind (1, name);
  stmt.Execute ();
}

void
GameParams::InitialiseDatabase ()
{
  SetParam ("rarest_recipe_drop_divisor", 4);
  SetParam ("tournament_loss_kills_enabled", 1);
  SetParam ("tournament_capture_pct", 128);
}

} // namespace pxd
```

- [ ] **Step 5: Seed at genesis** — in `src/logic.cpp`, add `#include "database/params.hpp"` beside the existing `#include "database/globaldata.hpp"` (line 27), and in `PXLogic::InitialiseState` (after `gd.InitialiseDatabase ();`, ~line 200) add:

```cpp
  GameParams gp(dbObj);
  gp.InitialiseDatabase ();
```

- [ ] **Step 6: Seed in the test DB** — in `database/dbtest.cpp`, add `#include "params.hpp"` near the top, and in the `DBTestWithSchema` constructor (after `gd.InitialiseDatabase ();`, ~line 53) add:

```cpp
  GameParams gp(db);
  gp.InitialiseDatabase ();
```

(Every C++ test that reads a param on a fresh `DBTestWithSchema` DB relies on this seeding — omitting it makes `GetParam` CHECK-fail.)

- [ ] **Step 7: Register the build files** — in `database/Makefile.am`: add `params.cpp` to `libdatabase_la_SOURCES` (after `globaldata.cpp`); add `params.hpp` to `noinst_HEADERS` (after `globaldata.hpp`); add `params_tests.cpp` to `tests_SOURCES` (after `inventory_tests.cpp`).

- [ ] **Step 8: Run the tests** — run the Docker `make check` command from "Building & testing the GSP" above.
Expected: `database/tests` compiles and `GameParamsTests.*` pass (4 tests). Everything else still green.

- [ ] **Step 9: Commit**

```bash
git add database/schema.sql database/params.hpp database/params.cpp \
        database/params_tests.cpp database/Makefile.am src/logic.cpp database/dbtest.cpp
git commit -m "feat(gsp): parameters KV table + GameParams accessor, seeded at genesis"
```

---

## Task 2: `HandleSetParam` admin verb (live-tune via g/tf)

**Files:**
- Modify: `src/moveprocessor.hpp` (declare `HandleSetParam`, ~line 305 near `ProcessOneAdmin`)
- Modify: `src/moveprocessor.cpp` (call it from `ProcessOneAdmin`; implement it; `#include "database/params.hpp"`)
- Modify: `src/moveprocessor_tests.cpp` (admin `param` move tests; `#include "database/params.hpp"`)

**Interfaces:**
- Consumes: `GameParams` (Task 1).
- Produces: an admin verb that parses `cmd["param"]` (an array of `{n:string, v:int|null}`), applies range-guarded `SetParam`, refuses unknown names and removal of the three required knobs. NOT god-gated.

- [ ] **Step 1: Write the failing tests** — in `src/moveprocessor_tests.cpp`, add `#include "database/params.hpp"` and, in the `MoveProcessorTests` fixture section (which already has the `ProcessAdmin(const std::string&)` helper at ~line 115), add:

```cpp
TEST_F (MoveProcessorTests, SetParamAdminUpdatesValue)
{
  ProcessAdmin (R"([{"cmd": {"param": [{"n": "rarest_recipe_drop_divisor", "v": 8}]}}])");
  EXPECT_EQ (GameParams (db).GetParam ("rarest_recipe_drop_divisor"), 8);
}

TEST_F (MoveProcessorTests, SetParamRejectsUnknownName)
{
  ProcessAdmin (R"([{"cmd": {"param": [{"n": "bogus_param", "v": 5}]}}])");
  EXPECT_FALSE (GameParams (db).HasParam ("bogus_param"));
}

TEST_F (MoveProcessorTests, SetParamRejectsOutOfRange)
{
  ProcessAdmin (R"([{"cmd": {"param": [{"n": "tournament_capture_pct", "v": 300}]}}])");
  EXPECT_EQ (GameParams (db).GetParam ("tournament_capture_pct"), 128);   // unchanged
  ProcessAdmin (R"([{"cmd": {"param": [{"n": "rarest_recipe_drop_divisor", "v": 0}]}}])");
  EXPECT_EQ (GameParams (db).GetParam ("rarest_recipe_drop_divisor"), 4); // unchanged
  ProcessAdmin (R"([{"cmd": {"param": [{"n": "tournament_loss_kills_enabled", "v": 2}]}}])");
  EXPECT_EQ (GameParams (db).GetParam ("tournament_loss_kills_enabled"), 1); // unchanged
}

TEST_F (MoveProcessorTests, SetParamRefusesRemovalOfRequired)
{
  ProcessAdmin (R"([{"cmd": {"param": [{"n": "tournament_capture_pct", "v": null}]}}])");
  EXPECT_TRUE (GameParams (db).HasParam ("tournament_capture_pct"));   // still present
}

TEST_F (MoveProcessorTests, SetParamAtBoundaries)
{
  ProcessAdmin (R"([{"cmd": {"param": [{"n": "tournament_capture_pct", "v": 256}]}}])");
  EXPECT_EQ (GameParams (db).GetParam ("tournament_capture_pct"), 256);   // at max -> accepted
  ProcessAdmin (R"([{"cmd": {"param": [{"n": "tournament_capture_pct", "v": 0}]}}])");
  EXPECT_EQ (GameParams (db).GetParam ("tournament_capture_pct"), 0);     // at min -> accepted
}
```

(The `MoveProcessorTests` fixture uses a `DBTestWithSchema`-derived in-memory DB, so the seeded defaults from Task 1 are present. Confirm the fixture seeds the schema before relying on `128`/`4`.)

- [ ] **Step 2: Declare the verb** — in `src/moveprocessor.hpp`, next to `void HandleGodMode (const Json::Value& cmd);` (~line 307), add:

```cpp
  /**
   * Handles a runtime-parameter admin command (the `param` verb).  Unlike
   * god-mode this is NOT gated on REGTEST -- it is live on POLYGON, authorised
   * solely by ownership of the g/tf admin name (libxayagame only delivers admin
   * moves from that name).  Parses an array of {n:string, v:int|null} ops and
   * applies range-guarded SetParam; unknown names and out-of-range values are
   * ignored (soft-fail, never CHECK), and removal of the required knobs is
   * refused (GetParam CHECK-fails on unset).
   */
  void HandleSetParam (const Json::Value& cmd);
```

- [ ] **Step 3: Implement + wire it** — in `src/moveprocessor.cpp`, add `#include "database/params.hpp"` with the other database includes. In `ProcessOneAdmin` (~line 161) add the new verb call after `HandleGodMode`:

```cpp
void
MoveProcessor::ProcessOneAdmin (const Json::Value& cmd)
{
  if (!cmd.isObject ())
    return;

  HandleGodMode (cmd["god"]);
  HandleSetParam (cmd["param"]);
}
```

Then add the implementation (place it after `HandleGodMode`, ~line 185):

```cpp
void
MoveProcessor::HandleSetParam (const Json::Value& cmd)
{
  if (!cmd.isArray ())
    return;

  GameParams gp(db);

  for (const auto& op : cmd)
    {
      if (!op.isObject ())
        continue;

      const auto& nameVal = op["n"];
      if (!nameVal.isString ())
        continue;
      const std::string name = nameVal.asString ();

      /* Only these three knobs are settable.  An unknown name never creates a
         phantom consensus row (a fat-finger/future-typo can't add state). */
      const bool known = (name == "rarest_recipe_drop_divisor"
                       || name == "tournament_loss_kills_enabled"
                       || name == "tournament_capture_pct");
      if (!known)
        {
          LOG (WARNING) << "Ignoring setparam for unknown name: " << name;
          continue;
        }

      const auto& vVal = op["v"];

      /* Removal is refused: GetParam CHECK-fails on unset, so removing a seeded
         knob would crash every node on the next reward/tournament read.  The
         Soccerverse v:null=remove semantics are kept in the shape but these
         three are load-bearing, so removal is a no-op here. */
      if (vVal.isNull ())
        {
          LOG (WARNING) << "Refusing to remove required param: " << name;
          continue;
        }
      if (!vVal.isInt ())
        {
          LOG (WARNING) << "Ignoring non-int setparam value for " << name;
          continue;
        }
      const int64_t v = vVal.asInt64 ();

      /* Range guards -- mirrors MaybeSetNewCostMultiplier.  divisor feeds
         weight*divisor in ScaledRewardWeights (keep well under uint32 overflow);
         capture_pct is compared against rnd.NextInt(256); kills_enabled is a
         boolean flag. */
      bool ok = false;
      if (name == "rarest_recipe_drop_divisor")
        ok = (v >= 1 && v <= 100000);
      else if (name == "tournament_capture_pct")
        ok = (v >= 0 && v <= 256);
      else if (name == "tournament_loss_kills_enabled")
        ok = (v == 0 || v == 1);

      if (!ok)
        {
          LOG (WARNING) << "Ignoring out-of-range setparam " << name << " = " << v;
          continue;
        }

      gp.SetParam (name, v);
    }
}
```

- [ ] **Step 4: Run the tests** — Docker `make check`.
Expected: the six new `MoveProcessorTests.SetParam*` pass; all else green.

- [ ] **Step 5: Commit**

```bash
git add src/moveprocessor.hpp src/moveprocessor.cpp src/moveprocessor_tests.cpp
git commit -m "feat(gsp): g/tf admin 'param' verb to live-tune balance knobs (range-guarded)"
```

---

## Task 3: Epic 4× — shared weighted-pick helper + divisor at all 5 roll sites

**Files:**
- Modify: `src/logic.hpp` (declare the two statics near `GenerateActivityReward`)
- Modify: `src/logic_resolve.cpp` (define both helpers; replace the 4 pick loops at ~349-380, ~408-433, ~469-500, ~650-681; `#include "database/params.hpp"`)
- Modify: `src/logic_tournament.cpp` (replace the tournament reward roll at ~217-248; `#include "database/params.hpp"`)
- Modify: `src/logic_tests.cpp` (unit test for `ScaledRewardWeights`)

**Interfaces:**
- Consumes: `GameParams` (Task 1); `RewardType` (`database/reward.hpp`, `GeneratedRecipe==1`), `pxd::Quality` (`database/recipe.hpp`, `Epic==4`); `pxd::proto::ActivityReward` / `AuthoredActivityReward` (`type()`, `generatedrecipequality()`, `weight()`, `rewards()`, `rewards(i)`).
- Produces: `static std::vector<uint32_t> PXLogic::ScaledRewardWeights(const pxd::proto::ActivityReward& table, uint32_t divisor);` and `static int32_t PXLogic::PickWeightedRewardIndex(const pxd::proto::ActivityReward& table, uint32_t divisor, xaya::Random& rnd);` (returns the chosen index into `table.rewards()`, or `-1` when the scaled total is 0).

Behaviour note: with `divisor==1`, `ScaledRewardWeights` returns each entry's raw weight, so `PickWeightedRewardIndex` reproduces the current four/five loops **byte-for-byte** (same `NextInt(totalWeight)` draw, same cumulative walk). `NextInt` is rejection sampling but consumes exactly one 8-byte draw per call for realistic weights, so growing `totalWeight` does not desync the RNG stream — the golden diff localizes to which reward each roll picks (and any RNG that a differently-typed chosen reward then consumes downstream).

- [ ] **Step 1: Write the failing test** — in `src/logic_tests.cpp`, add (a) includes if missing: `#include "database/reward.hpp"`, `#include "database/recipe.hpp"`, `#include "proto/activity_rewards.pb.h"`; (b) a test in the `ValidateStateTests` group:

```cpp
TEST_F (ValidateStateTests, EpicDivisorSparesOnlyEpicRecipe)
{
  pxd::proto::ActivityReward table;

  auto* candy = table.add_rewards ();
  candy->set_type ((uint32_t) pxd::RewardType::Candy);
  candy->set_weight (10);

  auto* epic = table.add_rewards ();
  epic->set_type ((uint32_t) pxd::RewardType::GeneratedRecipe);
  epic->set_generatedrecipequality ((uint32_t) pxd::Quality::Epic);
  epic->set_weight (2);

  auto* rare = table.add_rewards ();
  rare->set_type ((uint32_t) pxd::RewardType::GeneratedRecipe);
  rare->set_generatedrecipequality ((uint32_t) pxd::Quality::Rare);
  rare->set_weight (5);

  // divisor 1 -> unchanged (byte-identical to legacy behaviour).
  EXPECT_EQ (PXLogic::ScaledRewardWeights (table, 1),
             (std::vector<uint32_t> {10, 2, 5}));

  // divisor 4 -> every NON-Epic entry x4 (candy, and Rare q3), Epic q4 spared.
  // Epic share 2/17 (11.8%) -> 2/62 (3.2%) ~ quartered.
  EXPECT_EQ (PXLogic::ScaledRewardWeights (table, 4),
             (std::vector<uint32_t> {40, 2, 20}));
}
```

- [ ] **Step 2: Run it to confirm it fails** — Docker `make check`.
Expected: compile error (`ScaledRewardWeights` undeclared).

- [ ] **Step 3: Declare the statics** — in `src/logic.hpp`, next to the existing `GenerateActivityReward` declaration, add:

```cpp
  /**
   * Change B (Epic 4x): scales every reward entry's weight by `divisor`, EXCEPT
   * an Epic generated recipe (type==GeneratedRecipe && quality==Epic) which keeps
   * its authored weight -- so Epic's relative odds become ~1/divisor.  Pure, so
   * it is unit-testable directly.  With divisor==1 it returns the raw weights.
   */
  static std::vector<uint32_t> ScaledRewardWeights (
      const pxd::proto::ActivityReward& table, uint32_t divisor);

  /**
   * One weighted draw over the scaled distribution.  Returns the chosen index
   * into table.rewards(), or -1 if the scaled total is 0 (no reward -- matches
   * the legacy totalWeight==0 no-draw path).  Consumes exactly one rnd.NextInt
   * when the scaled total is > 0.
   */
  static int32_t PickWeightedRewardIndex (
      const pxd::proto::ActivityReward& table, uint32_t divisor,
      xaya::Random& rnd);
```

Ensure `logic.hpp` sees `RewardType`/`pxd::Quality` (add `#include "database/reward.hpp"` / `#include "database/recipe.hpp"` if the definitions are only needed in the .cpp — they are, so the includes belong in `logic_resolve.cpp`; the header only needs `pxd::proto::ActivityReward`, already available via the `GenerateActivityReward` decl's include).

- [ ] **Step 4: Define the helpers** — in `src/logic_resolve.cpp`, add `#include "database/params.hpp"` and (ensure `#include "database/reward.hpp"`, `#include "database/recipe.hpp"` are present), then add near the top of the file (after the anonymous-namespace helpers, at namespace scope):

```cpp
std::vector<uint32_t>
PXLogic::ScaledRewardWeights (const pxd::proto::ActivityReward& table,
                              const uint32_t divisor)
{
  std::vector<uint32_t> weights;
  weights.reserve (table.rewards_size ());
  for (const auto& rw : table.rewards ())
    {
      const bool isEpicRecipe =
          (RewardType) (int32_t) rw.type () == RewardType::GeneratedRecipe
          && (pxd::Quality) (int32_t) rw.generatedrecipequality () == pxd::Quality::Epic;
      weights.push_back (isEpicRecipe ? (uint32_t) rw.weight ()
                                      : (uint32_t) rw.weight () * divisor);
    }
  return weights;
}

int32_t
PXLogic::PickWeightedRewardIndex (const pxd::proto::ActivityReward& table,
                                  const uint32_t divisor, xaya::Random& rnd)
{
  const std::vector<uint32_t> weights = ScaledRewardWeights (table, divisor);

  uint32_t totalWeight = 0;
  for (const uint32_t w : weights)
    totalWeight += w;

  if (totalWeight == 0)
    return -1;

  const uint32_t rolCurNum = rnd.NextInt (totalWeight);

  uint32_t accumulatedWeight = 0;
  for (int32_t i = 0; i < (int32_t) weights.size (); ++i)
    {
      accumulatedWeight += weights[i];
      if (rolCurNum < accumulatedWeight)
        return i;
    }
  return -1;  /* unreachable when totalWeight == sum(weights) > 0 */
}
```

- [ ] **Step 5: Replace the four `logic_resolve.cpp` loops.** In `PXLogic::ResolveSweetener`, read the divisor ONCE near the top of the function (after `Database& db` is in scope), e.g.:

```cpp
  const uint32_t divisor
      = (uint32_t) GameParams (db).GetParam ("rarest_recipe_drop_divisor");
```

Then replace loop 1 (lines ~349-380) with:

```cpp
    for (int32_t roll = 0;
         roll < (int32_t) sweetenerBlueprint.rewardchoices (rewardID).baserollcount ();
         ++roll)
      {
        const int32_t idx = PickWeightedRewardIndex (rewardTableDb, divisor, rnd);
        if (idx >= 0)
          GenerateActivityReward (fighterID, "", 0, rewardTableDb.rewards (idx),
                                  ctx, db, a, rnd, (uint32_t) idx,
                                  sweetenerBlueprint.rewardchoices (rewardID).rewardstableid (),
                                  sweetenerAuthID);
      }
```

Replace loop 2 (lines ~408-433) identically but with `moverollcount()` and `moverewardstableid()`. Replace loop 3 (lines ~469-500) identically but with `armorrollcount()` and `armorrewardstableid()`. (Delete each old loop's `totalWeight` pre-sum, the `rolCurNum`/`accumulatedWeight`/`posInTableList` inner walk — the helper subsumes them. Note `rewardTableDb` is rebound before each of the three loops in the current code; keep those rebinds — only the pick machinery changes.)

In `PXLogic::ResolveExpedition`, read the divisor once near the top the same way, then replace loop 4 (lines ~650-681) with:

```cpp
    for (int32_t roll = 0; roll < rollCount; ++roll)
      {
        const int32_t idx = PickWeightedRewardIndex (rewardTableDb, divisor, rnd);
        if (idx >= 0)
          GenerateActivityReward (fighterID, blueprintAuthID, 0,
                                  rewardTableDb.rewards (idx), ctx, db, a, rnd,
                                  (uint32_t) idx, basedRewardsTableAuthId, "");
      }
```

- [ ] **Step 6: Replace the tournament reward roll.** In `src/logic_tournament.cpp`, add `#include "database/params.hpp"`. In `ProcessTournaments`, inside the `blocksleft == 0` branch, read the divisor once BEFORE the per-participant reward loop (place it right after `XayaPlayersTable xayaplayers(db);`, ~line 177):

```cpp
              const uint32_t divisor
                  = (uint32_t) GameParams (db).GetParam ("rarest_recipe_drop_divisor");
```

Then replace the inner reward roll (lines ~217-248: the `uint32_t totalWeight` sum + the `for(uint32_t roll...)` cumulative walk) with:

```cpp
                  for (uint32_t roll = 0; roll < rollCount; ++roll)
                    {
                      const int32_t idx
                          = PickWeightedRewardIndex (rewardTableDb, divisor, rnd);
                      if (idx >= 0)
                        GenerateActivityReward (0, "", tnm->GetId (),
                                                rewardTableDb.rewards (idx), ctx, db,
                                                a, rnd, (uint32_t) idx, rewardTableId, "");
                    }
```

(Keep the surrounding `rewardTableDb`/`rewardTableId`/`rollCount` setup and the `rewardsSolved` guard unchanged — only the roll machinery changes.)

- [ ] **Step 7: Run the tests** — Docker `make check`.
Expected: `EpicDivisorSparesOnlyEpicRecipe` passes and everything compiles. **The golden replay test WILL now fail** (reward outcomes shifted under divisor 4) — that is expected; Task 7 reviews and regenerates it. To keep this task green in isolation, temporarily run with the golden regen so the suite passes, OR accept the single expected `GoldenReplay.FullScenario` mismatch and note it for Task 7. Do NOT commit a regenerated golden here — the golden is regenerated once, after all rule changes, in Task 7.

- [ ] **Step 8: Commit** (excluding the golden)

```bash
git add src/logic.hpp src/logic_resolve.cpp src/logic_tournament.cpp src/logic_tests.cpp
git commit -m "feat(gsp): Epic 4x rarer via shared divisor-scaled reward pick (all 5 roll sites)"
```

---

## Task 4: `battle_losses` table + `BattleLossesTable` + `getuser` surfacing

**Files:**
- Modify: `database/schema.sql` (add `battle_losses` table + index)
- Create: `database/battlelosses.hpp`, `database/battlelosses.cpp`, `database/battlelosses_tests.cpp`
- Modify: `database/Makefile.am` (register the three files)
- Modify: `src/gamestatejson.cpp` (emit `battlelosses` in `User`; `#include "database/battlelosses.hpp"`)

**Interfaces:**
- Produces: `class pxd::BattleLossesTable` with `void Add(const std::string& owner, uint32_t fighterId, const std::string& name, uint32_t outcome, const std::string& opponent, uint32_t tournamentId)` and `Database::Result<BattleLossResult> QueryForOwner(const std::string& owner)`; `struct BattleLossResult` columns `id, owner, fighterid, name, outcome, opponent, tournamentid`. `getuser(name)` gains a `battlelosses` array of `{id, fighterid, name, outcome, opponent, tournamentid}`.

- [ ] **Step 1: Write the failing test** — create `database/battlelosses_tests.cpp`:

```cpp
/*
    <GPL header — copy verbatim from database/globaldata.cpp lines 1-17>
*/

#include "battlelosses.hpp"

#include "dbtest.hpp"

#include <gtest/gtest.h>

namespace pxd
{
namespace
{

class BattleLossesTests : public DBTestWithSchema
{
protected:
  BattleLossesTable losses;
  BattleLossesTests () : losses(db) {}
};

TEST_F (BattleLossesTests, AddAndQueryForOwner)
{
  losses.Add ("domob", 42, "Sour Gummi Brawler", 0, "andy", 7);
  losses.Add ("domob", 43, "Choco Punch", 1, "andy", 7);
  losses.Add ("bob", 44, "Nougat Knight", 0, "andy", 7);

  auto res = losses.QueryForOwner ("domob");
  ASSERT_TRUE (res.Step ());
  EXPECT_EQ (res.Get<BattleLossResult::fighterid> (), 42);
  EXPECT_EQ (res.Get<BattleLossResult::name> (), "Sour Gummi Brawler");
  EXPECT_EQ (res.Get<BattleLossResult::outcome> (), 0);
  EXPECT_EQ (res.Get<BattleLossResult::opponent> (), "andy");
  EXPECT_EQ (res.Get<BattleLossResult::tournamentid> (), 7);
  ASSERT_TRUE (res.Step ());
  EXPECT_EQ (res.Get<BattleLossResult::fighterid> (), 43);
  EXPECT_EQ (res.Get<BattleLossResult::outcome> (), 1);
  EXPECT_FALSE (res.Step ());
}

} // anonymous namespace
} // namespace pxd
```

- [ ] **Step 2: Add the schema** — in `database/schema.sql`, after the `rewards` table + `rewards_by_owner` index block (~line 114), add:

```sql
-- Change C: a client-visible record of every fighter lost in a tournament
-- (destroyed or captured), so the loser gets a "you lost a treat" reveal.  One
-- row per loss; surfaced in getuser; the loser's rewards_serial is bumped
-- alongside so the existing serial-driven reveal fires.  Flat columns (no proto)
-- since the shape is small and fixed.  Added the additive IF NOT EXISTS way.
CREATE TABLE IF NOT EXISTS `battle_losses` (
  `id` INTEGER PRIMARY KEY,
  `owner` TEXT NOT NULL,
  `fighterid` INTEGER NOT NULL,
  `name` TEXT NOT NULL,
  `outcome` INTEGER NOT NULL,   -- 0 = destroyed, 1 = captured
  `opponent` TEXT NOT NULL,     -- the tournament winner's Xaya name
  `tournamentid` INTEGER NOT NULL
);

CREATE INDEX IF NOT EXISTS `battle_losses_by_owner`
  ON `battle_losses` (`owner`);
```

- [ ] **Step 3: Create `database/battlelosses.hpp`:**

```cpp
/*
    <GPL header — copy verbatim from database/globaldata.hpp lines 1-17>
*/

#ifndef DATABASE_BATTLELOSSES_HPP
#define DATABASE_BATTLELOSSES_HPP

#include "database.hpp"

#include <cstdint>
#include <string>

namespace pxd
{

/** A row of the battle_losses table.  */
struct BattleLossResult : public Database::ResultType
{
  RESULT_COLUMN (int64_t, id, 1);
  RESULT_COLUMN (std::string, owner, 2);
  RESULT_COLUMN (int64_t, fighterid, 3);
  RESULT_COLUMN (std::string, name, 4);
  RESULT_COLUMN (int64_t, outcome, 5);
  RESULT_COLUMN (std::string, opponent, 6);
  RESULT_COLUMN (int64_t, tournamentid, 7);
};

/**
 * Access to the battle_losses table (Change C): append a client-visible loss
 * record when a fighter is destroyed/captured in a tournament, and read a
 * player's losses for getuser.
 */
class BattleLossesTable
{

private:

  Database& db;

public:

  explicit BattleLossesTable (Database& d) : db(d) {}

  /** Appends one loss row (id auto-assigned).  outcome: 0=destroyed, 1=captured.  */
  void Add (const std::string& owner, uint32_t fighterId, const std::string& name,
            uint32_t outcome, const std::string& opponent, uint32_t tournamentId);

  /** All losses for `owner`, ordered by id (oldest first).  */
  Database::Result<BattleLossResult> QueryForOwner (const std::string& owner);

};

} // namespace pxd

#endif // DATABASE_BATTLELOSSES_HPP
```

- [ ] **Step 4: Create `database/battlelosses.cpp`:**

```cpp
/*
    <GPL header — copy verbatim from database/globaldata.cpp lines 1-17>
*/

#include "battlelosses.hpp"

#include <glog/logging.h>

namespace pxd
{

void
BattleLossesTable::Add (const std::string& owner, const uint32_t fighterId,
                        const std::string& name, const uint32_t outcome,
                        const std::string& opponent, const uint32_t tournamentId)
{
  auto stmt = db.Prepare (R"(
    INSERT INTO `battle_losses`
      (`id`, `owner`, `fighterid`, `name`, `outcome`, `opponent`, `tournamentid`)
      VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7)
  )");
  stmt.Bind (1, db.GetNextId ());
  stmt.Bind (2, owner);
  stmt.Bind (3, (int64_t) fighterId);
  stmt.Bind (4, name);
  stmt.Bind (5, (int64_t) outcome);
  stmt.Bind (6, opponent);
  stmt.Bind (7, (int64_t) tournamentId);
  stmt.Execute ();
}

Database::Result<BattleLossResult>
BattleLossesTable::QueryForOwner (const std::string& owner)
{
  auto stmt = db.Prepare (R"(
    SELECT * FROM `battle_losses` WHERE `owner` = ?1 ORDER BY `id`
  )");
  stmt.Bind (1, owner);
  return stmt.Query<BattleLossResult> ();
}

} // namespace pxd
```

- [ ] **Step 5: Register the build files** — in `database/Makefile.am`: add `battlelosses.cpp` to `libdatabase_la_SOURCES`; add `battlelosses.hpp` to `noinst_HEADERS`; add `battlelosses_tests.cpp` to `tests_SOURCES`.

- [ ] **Step 6: Surface it in `getuser`** — in `src/gamestatejson.cpp`, add `#include "database/battlelosses.hpp"`, and in `GameStateJson::User` just before `return res;` (~line 633, after the `res["multiplier"]` block) add:

```cpp
  BattleLossesTable lossesTbl(db);
  auto lossRes = lossesTbl.QueryForOwner (userName);
  Json::Value lossesArr(Json::arrayValue);
  while (lossRes.Step ())
    {
      Json::Value o(Json::objectValue);
      o["id"] = IntToJson (lossRes.Get<BattleLossResult::id> ());
      o["fighterid"] = IntToJson (lossRes.Get<BattleLossResult::fighterid> ());
      o["name"] = lossRes.Get<BattleLossResult::name> ();
      o["outcome"] = IntToJson (lossRes.Get<BattleLossResult::outcome> ());
      o["opponent"] = lossRes.Get<BattleLossResult::opponent> ();
      o["tournamentid"] = IntToJson (lossRes.Get<BattleLossResult::tournamentid> ());
      lossesArr.append (o);
    }
  res["battlelosses"] = lossesArr;
```

- [ ] **Step 7: Run the tests** — Docker `make check`.
Expected: `BattleLossesTests.AddAndQueryForOwner` passes; `gamestatejson` still compiles (empty `battlelosses` array for users with no losses does not perturb existing gamestatejson tests — confirm they still pass).

- [ ] **Step 8: Commit**

```bash
git add database/schema.sql database/battlelosses.hpp database/battlelosses.cpp \
        database/battlelosses_tests.cpp database/Makefile.am src/gamestatejson.cpp
git commit -m "feat(gsp): battle_losses table + getuser surfacing (Change C storage)"
```

---

## Task 5: Tournament permadeath + 50/50 capture + loss record + serial bump

**Files:**
- Modify: `src/logic_tournament.cpp` (insert the permadeath pass between the reward loop and the survivor reset loop; `#include "database/battlelosses.hpp"` — `database/params.hpp` already added in Task 3)
- Modify: `src/logic_tests.cpp` (permadeath integration tests)

**Interfaces:**
- Consumes: `GameParams` (Task 1), `BattleLossesTable` (Task 4). Uses in-scope: `teams` (`std::map<std::string,std::vector<uint32_t>>`), `winnerName` (`std::string`), `fighters` (`FighterTable&`), `xayaplayers` (`XayaPlayersTable&`, declared ~line 177), `rnd`, `ctx`, `db`, `tnm`. Also `fighters.DeleteById(id)`, `fighters.CountForOwner(owner)`, `fighter->GetProto().isaccountbound()`, `fighter->GetProto().name()`, `fighter->SetOwner(name)`, `ctx.RoConfig()->params().max_fighter_inventory_amount()`, `XayaPlayer::CalculatePrestige(cfg)`, `a->MutableProto().set_rewards_serial(...)`.

- [ ] **Step 1: Write the failing tests** — in `src/logic_tests.cpp`, add `#include "database/params.hpp"` if not present, and add these tests. They mirror `TournamentResolvedTest` (join move `{"tm":{"e":{"tid":<id>,"fc":[<f1>,<f2>]}}}`, resolution via repeated `UpdateState("[]")`). They avoid needing to control the RNG-picked winner by (a) forcing the capture/destroy branch via the `tournament_capture_pct` knob, and (b) reading the actual `winnerid` after resolution to identify the loser. The two entered fighters per player must be made NON-account-bound so permadeath applies (starters are protected).

```cpp
namespace
{

/* Helper: run the tutorial 2v2 tournament with two players each entering two
   non-account-bound fighters, then return the resolved winner name and the
   two players' ids so the caller can inspect the loser.  Mirrors the setup of
   TournamentResolvedTest. */
struct TwoPlayerTournament
{
  std::string winner;
  std::string loser;
  std::vector<uint32_t> loserEntered;   // the loser's two entered fighter ids
  uint32_t tid;
};

} // anonymous namespace

TEST_F (ValidateStateTests, TournamentPermadeathAlwaysCaptures)
{
  GameParams (db).SetParam ("tournament_capture_pct", 256);   // always capture
  GameParams (db).SetParam ("tournament_loss_kills_enabled", 1);

  const TwoPlayerTournament t = RunTwoPlayerTournament ();    // see Step 3 helper

  // Exactly one of the loser's entered fighters transferred to the winner.
  auto lp = xayaplayers.GetByName (t.loser, ctx.RoConfig ());
  auto wp = xayaplayers.GetByName (t.winner, ctx.RoConfig ());
  const auto loserRoster = lp->CollectInventoryFighters (ctx.RoConfig ());
  // The loser lost exactly one fighter; the captured one now belongs to winner.
  int transferred = 0;
  for (const uint32_t fid : t.loserEntered)
    {
      auto f = tbl3.GetById (fid, ctx.RoConfig ());
      if (f != nullptr && f->GetOwner () == t.winner) transferred++;
    }
  EXPECT_EQ (transferred, 1);

  // The loser has a battle_losses row with outcome=captured, opponent=winner.
  BattleLossesTable losses(db);
  auto lr = losses.QueryForOwner (t.loser);
  ASSERT_TRUE (lr.Step ());
  EXPECT_EQ (lr.Get<BattleLossResult::outcome> (), 1);
  EXPECT_EQ (lr.Get<BattleLossResult::opponent> (), t.winner);
}

TEST_F (ValidateStateTests, TournamentPermadeathAlwaysDestroys)
{
  GameParams (db).SetParam ("tournament_capture_pct", 0);     // always destroy
  GameParams (db).SetParam ("tournament_loss_kills_enabled", 1);

  const TwoPlayerTournament t = RunTwoPlayerTournament ();

  int alive = 0;
  for (const uint32_t fid : t.loserEntered)
    if (tbl3.GetById (fid, ctx.RoConfig ()) != nullptr) alive++;
  EXPECT_EQ (alive, 1);   // exactly one of the loser's two entered fighters destroyed

  BattleLossesTable losses(db);
  auto lr = losses.QueryForOwner (t.loser);
  ASSERT_TRUE (lr.Step ());
  EXPECT_EQ (lr.Get<BattleLossResult::outcome> (), 0);
}

TEST_F (ValidateStateTests, TournamentPermadeathDisabledKeepsEveryone)
{
  GameParams (db).SetParam ("tournament_loss_kills_enabled", 0);   // disabled

  const TwoPlayerTournament t = RunTwoPlayerTournament ();

  for (const uint32_t fid : t.loserEntered)
    EXPECT_NE (tbl3.GetById (fid, ctx.RoConfig ()), nullptr);   // all survive
  BattleLossesTable losses(db);
  EXPECT_FALSE (losses.QueryForOwner (t.loser).Step ());        // no loss recorded
}

TEST_F (ValidateStateTests, TournamentPermadeathProtectsStarters)
{
  GameParams (db).SetParam ("tournament_capture_pct", 256);   // would always capture
  GameParams (db).SetParam ("tournament_loss_kills_enabled", 1);

  // RunTwoPlayerTournament sets both entered fighters account-bound when
  // `accountBound` is true.
  const TwoPlayerTournament t = RunTwoPlayerTournament (/*accountBound=*/true);

  for (const uint32_t fid : t.loserEntered)
    {
      auto f = tbl3.GetById (fid, ctx.RoConfig ());
      ASSERT_NE (f, nullptr);                    // not destroyed
      EXPECT_EQ (f->GetOwner (), t.loser);       // not captured
    }
  BattleLossesTable losses(db);
  EXPECT_FALSE (losses.QueryForOwner (t.loser).Step ());   // starter loss not recorded
}
```

- [ ] **Step 2: Add the `RunTwoPlayerTournament` fixture helper** — in `src/logic_tests.cpp`'s `PXLogicTests`/`ValidateStateTests` fixture, add a helper modeled exactly on `TournamentResolvedTest` (quoted in the research digest). It: creates players `"domob"` and `"andy"`; for each, creates two fighters via `tbl3.CreateNew(name, 1, ctx.RoConfig(), rnd)`, sets `ft->MutableProto().set_isaccountbound(accountBound)` on each and `ft->SetStatus(...)`/`.reset()`; enters both via the `{"tm":{"e":{"tid":..,"fc":[..]}}}` move for each player against the tutorial tournament auth-id `"cbd2e78a-37ce-b864-793d-8dd27788a774"` (looked up with `tbl5.GetByAuthIdName(...)`); ticks `UpdateState("[]")` enough times to resolve; reads `tbl5.GetById(TID,...)->GetInstance().winnerid()` to fill `.winner`, sets `.loser` to the other name, and fills `.loserEntered` with the loser's two entered fighter ids. Return the struct. (This mirrors `TournamentResolvedTest` lines already in the file — reuse its exact join/resolve sequence; add the `set_isaccountbound` line and the winner/loser bookkeeping.)

- [ ] **Step 3: Run it to confirm it fails** — Docker `make check`. Expected: the four permadeath tests FAIL (no permadeath logic yet; fighters all survive, no loss rows).

- [ ] **Step 4: Implement the permadeath pass** — in `src/logic_tournament.cpp`, add `#include "database/battlelosses.hpp"`. Locate the reward loop's closing brace (`}` at ~line 259, right after `a.reset();`) and the survivor reset loop (`for(auto participantFighter : tnm->GetInstance().fighters())` at ~line 261). Insert BETWEEN them:

```cpp
              /* Change A + C: tournament permadeath + 50/50 capture.  Runs only
                 when enabled and there is a winner.  Each NON-winning team (in
                 ascending owner-name order via the std::map) loses exactly ONE
                 randomly-picked entered fighter: an account-bound starter is
                 immune (and takes no capture draw); otherwise a capture_pct/256
                 coin-flip either transfers it to the winner (if the winner has a
                 free roster slot) or destroys it.  A battle_losses row is written
                 and the loser's rewards_serial bumped so the client reveals it.
                 The survivor-reset loop below then flips the surviving (and any
                 captured) fighters back to Available; destroyed ids simply return
                 null there and are skipped. */
              {
                GameParams gp(db);
                if (gp.GetParam ("tournament_loss_kills_enabled") == 1
                    && winnerName != "")
                  {
                    const uint32_t capturePct
                        = (uint32_t) gp.GetParam ("tournament_capture_pct");
                    const uint32_t maxInv
                        = ctx.RoConfig ()->params ().max_fighter_inventory_amount ();
                    BattleLossesTable battleLosses(db);

                    for (const auto& team : teams)
                      {
                        if (team.first == winnerName)
                          continue;   /* the winner's team loses no one */
                        const auto& roster = team.second;
                        if (roster.empty ())
                          continue;

                        /* One victim draw per losing team (always taken). */
                        const uint32_t victimId
                            = roster[rnd.NextInt (roster.size ())];
                        auto victim = fighters.GetById (victimId, ctx.RoConfig ());
                        if (victim == nullptr)
                          continue;

                        /* Starter protection: nothing happens, and NO capture
                           draw is taken (keeps the RNG stream deterministic). */
                        if (victim->GetProto ().isaccountbound ())
                          {
                            victim.reset ();
                            continue;
                          }

                        const uint32_t roll = rnd.NextInt (256);
                        const bool capture
                            = (roll < capturePct)
                           && (fighters.CountForOwner (winnerName) < maxInv);

                        const std::string victimName = victim->GetProto ().name ();
                        battleLosses.Add (team.first, victimId, victimName,
                                          capture ? 1 : 0, winnerName, tnm->GetId ());

                        if (capture)
                          {
                            victim->SetOwner (winnerName);   /* keeps all stats/armor */
                            victim.reset ();                 /* flush owner change */
                            auto wp = xayaplayers.GetByName (winnerName,
                                                             ctx.RoConfig ());
                            if (wp != nullptr)
                              {
                                wp->CalculatePrestige (ctx.RoConfig ());
                                wp.reset ();
                              }
                          }
                        else
                          {
                            victim.reset ();                 /* release before delete */
                            fighters.DeleteById (victimId);
                          }

                        auto lp = xayaplayers.GetByName (team.first, ctx.RoConfig ());
                        if (lp != nullptr)
                          {
                            lp->MutableProto ().set_rewards_serial (
                                lp->GetProto ().rewards_serial () + 1);
                            lp->CalculatePrestige (ctx.RoConfig ());
                            lp.reset ();
                          }
                      }
                  }
              }

```

Determinism: `teams` is `std::map<std::string,...>` (deterministic ascending owner order); `roster` is a `std::vector<uint32_t>` in insertion order; the victim draw precedes the capture draw; the capture draw is skipped only for account-bound victims (a consensus-state predicate). `CountForOwner(winnerName)` is re-read per capture (each capture flushes via `victim.reset()` before the next iteration), so the 48-cap is respected as slots fill.

- [ ] **Step 5: Run the tests** — Docker `make check`.
Expected: the four permadeath tests pass. `GoldenReplay.FullScenario` remains expected-to-fail until Task 7 (the tutorial-resolve scenario may now record a loss / different reward outcomes) — note it, do not regen here.

- [ ] **Step 6: Commit** (excluding the golden)

```bash
git add src/logic_tournament.cpp src/logic_tests.cpp
git commit -m "feat(gsp): tournament permadeath + 50/50 capture + loss record + serial bump"
```

---

## Task 6: Frontend loss reveal (Change C client side)

**Files:**
- Modify: `src/data/types.ts` (`BattleLoss` type; `UserSlice.battlelosses`; parse it)
- Modify: `src/data/reward-reveal.ts` (`InvSnapshot.losses`; `RewardWon.lostFighters`; `empty`)
- Modify: `src/ui/chrome/RevealSwitch.tsx` (loss-first branch)
- Create: `src/ui/chrome/FighterLossModal.tsx`
- Modify: `app/page.tsx` (empty-state literal gains `lostFighters: []`)
- Modify: `tests/data/reward-reveal.test.ts` (`lostFighters` test)

**Interfaces:**
- Consumes: the GSP `getuser` `battlelosses` array `{id, fighterid, name, outcome, opponent, tournamentid}` (Task 4). The loss reveal is driven by these authoritative records (NOT a fighter-id diff), so it never over-fires on ordinary sale/deconstruction/sacrifice and carries the fighter's name/outcome for display even though the fighter is gone.
- Produces: `interface BattleLoss`; `UserSlice.battlelosses: BattleLoss[]`; `InvSnapshot.losses: BattleLoss[]`; `RewardWon.lostFighters: BattleLoss[]`; `<FighterLossModal losses={...} onClose={...}/>`.

Build/test natively: `cd tf-frontend`.

- [ ] **Step 1: Write the failing test** — in `tests/data/reward-reveal.test.ts`, add (the `slice` helper already builds a `UserSlice`; add `battlelosses` to it and a test):

```ts
it('detects a lost fighter from the battlelosses record (destroyed)', () => {
  const before = snapshotInventory(slice({ battlelosses: [] }), 'me');
  const after = snapshotInventory(
    slice({ battlelosses: [{ id: 1, fighterid: 9, name: 'Gummi', outcome: 0, opponent: 'andy', tournamentid: 3 }] }),
    'me');
  const won = rewardDelta(before, after);
  expect(won.lostFighters.map((l) => l.fighterid)).toEqual([9]);
  expect(won.empty).toBe(false);
});

it('does not re-report an already-seen loss', () => {
  const rec = { id: 1, fighterid: 9, name: 'Gummi', outcome: 1, opponent: 'andy', tournamentid: 3 };
  const before = snapshotInventory(slice({ battlelosses: [rec] }), 'me');
  const after = snapshotInventory(slice({ battlelosses: [rec] }), 'me');
  expect(rewardDelta(before, after).lostFighters).toEqual([]);
});
```

Update the `slice` helper's default object to include `battlelosses: []` (so existing tests keep compiling).

- [ ] **Step 2: Add the type + parse** — in `src/data/types.ts`:

```ts
/** A fighter lost in a tournament (Change C). outcome: 0 = destroyed, 1 = captured. */
export interface BattleLoss {
  id: number;
  fighterid: number;
  name: string;
  outcome: number;
  opponent: string;
  tournamentid: number;
}
```

Add `battlelosses: BattleLoss[];` to the `UserSlice` interface, and in `parseUserSlice` add to the returned object:

```ts
    battlelosses: asArray<BattleLoss>(data.battlelosses),
```

- [ ] **Step 3: Extend the diff** — in `src/data/reward-reveal.ts`:
  - Add `import { OngoingType, type UserSlice, type BattleLoss } from './types';` (extend the existing import).
  - Add `losses: BattleLoss[];` to `InvSnapshot`.
  - In `snapshotInventory`, add to the returned object: `losses: [...(u?.battlelosses ?? [])],`.
  - Add `lostFighters: BattleLoss[];` to `RewardWon`.
  - In `rewardDelta`, compute it (defensive against old baselines lacking the field):

```ts
  const beforeLoss = new Set((before.losses ?? []).map((l) => l.id));
  const lostFighters = (after.losses ?? []).filter((l) => !beforeLoss.has(l.id));
```

  - Update `empty` to include losses, and return `lostFighters`:

```ts
  const empty = fungible.length === 0 && newRecipes === 0 && fighterUpgrades.length === 0
    && newFighters.length === 0 && lostFighters.length === 0 && !cookRefunded;
  return { fungible, newRecipes, fighterUpgrades, newFighters, lostFighters, cookRefunded, empty };
```

- [ ] **Step 4: Route the reveal** — in `src/ui/chrome/RevealSwitch.tsx`, import and branch loss-FIRST (a lost treat is a required notification and must never be swallowed):

```tsx
import { FighterLossModal } from './FighterLossModal';
// ...
export function RevealSwitch({ won, onClose }: { won: RewardWon; onClose: () => void }): React.JSX.Element {
  if (won.lostFighters.length > 0) return <FighterLossModal losses={won.lostFighters} onClose={onClose} />;
  if (won.newFighters.length > 0) return <FighterRevealModal fighterIds={won.newFighters} onClose={onClose} />;
  return <RewardRevealModal won={won} onClose={onClose} />;
}
```

- [ ] **Step 5: Create `src/ui/chrome/FighterLossModal.tsx`** — renders from the `BattleLoss` records (NOT a live `user.fighters` lookup — the fighter is gone), mirroring `RewardRevealModal`'s portal shell:

```tsx
'use client';

/**
 * Fighter loss — the required "you lost a treat" reveal (Change C). A fighter you
 * lost in a tournament is GONE from your roster, so unlike the win reveals this
 * renders entirely from the battle_losses record the GSP surfaced (name +
 * destroyed/captured + who took it), never a live user.fighters lookup. Portaled
 * to <body>, mirrors RewardRevealModal's shell.
 */
import { useEffect } from 'react';
import { createPortal } from 'react-dom';
import { audio } from '@/audio/audioManager';
import type { BattleLoss } from '@/data/types';

export function FighterLossModal({ losses, onClose }: { losses: BattleLoss[]; onClose: () => void }): React.JSX.Element {
  useEffect(() => { audio.sfx('lose'); }, []);   // somber sting; fall back to any existing loss/defeat sfx key
  const line = (l: BattleLoss) =>
    l.outcome === 1
      ? `${l.name || `Fighter #${l.fighterid}`} was captured by ${l.opponent}`
      : `${l.name || `Fighter #${l.fighterid}`} was destroyed in battle`;

  return createPortal(
    <div className="modal-overlay" onClick={onClose} role="presentation">
      <div className="modal reward-modal fighter-loss-modal" onClick={(e) => e.stopPropagation()}>
        <div className="reward-hero">💀</div>
        <h2 className="reward-title">{losses.length > 1 ? 'You lost treats!' : 'You lost a treat'}</h2>
        <div className="reward-items">
          {losses.map((l) => (
            <span className="reward-item" key={l.id}>{line(l)}</span>
          ))}
        </div>
        <button onClick={onClose}>Close</button>
      </div>
    </div>,
    document.body,
  );
}
```

(Use whatever loss/defeat SFX key exists in `audioManager`; if none, drop the `audio.sfx` line. Style class `fighter-loss-modal` can reuse the reward-modal CSS; add a small somber accent only if trivial.)

- [ ] **Step 6: Fix the empty-state literal** — in `app/page.tsx`, the `onOpenRewards` fallback object literal (~line 222) constructs a `RewardWon`; add `lostFighters: []` to it so it still type-checks:

```ts
    setRevealWon(won ?? { fungible: [], newRecipes: 0, fighterUpgrades: [], newFighters: [], lostFighters: [], cookRefunded: false, empty: true });
```

(The live auto-pop `if (won && !won.empty)` at ~line 214 now fires for a loss-only serial advance, because Step 3 made `empty` account for `lostFighters`.)

- [ ] **Step 7: Run the tests + typecheck** — `cd tf-frontend && npm test && npm run build`.
Expected: the two new reward-reveal tests pass; all existing tests pass; the build type-checks (every `RewardWon` construction site now carries `lostFighters`).

- [ ] **Step 8: Commit**

```bash
git add src/data/types.ts src/data/reward-reveal.ts src/ui/chrome/RevealSwitch.tsx \
        src/ui/chrome/FighterLossModal.tsx app/page.tsx tests/data/reward-reveal.test.ts
git commit -m "feat(fe): FighterLossModal — required 'you lost a treat' reveal (Change C)"
```

---

## Task 7: Reorg coverage, golden regen, full-suite green, genesis note

**Files:**
- Modify: `src/reorg_tests.cpp` (a captured fighter reverts ownership on reorg)
- Regenerate: `src/goldenreplay.golden.json`

**Interfaces:**
- Consumes: everything above. This is the integration/consensus gate.

- [ ] **Step 1: Add the capture reorg test** — in `src/reorg_tests.cpp`, add a test modeled on the existing `TradeReorgRestoresStateExactly` (the ownership-transfer-reverts template): build a block that resolves a tournament with `tournament_capture_pct=256` and `tournament_loss_kills_enabled=1` so a fighter is captured (owner transfers to the winner), snapshot the whole-DB hashes before and after, then `ApplyInverse` the changeset and assert the captured fighter's ownership (and the `battle_losses` row) revert exactly — and that re-applying the identical block reproduces the same state. Use the fixture's `SetupActors`/`BuildMoves`/`RunBlockCapture`/`WholeDbHashes`/`ApplyInverse`/`RestoreNextId` primitives and `HashForHeight` (POLYGON chain, fixed per-height hash for the RNG reseed), exactly as `TradeReorgRestoresStateExactly` and `BuildResolveAndDeepReorgRestoresState` do. Set the two params via `GameParams(db).SetParam(...)` in the block's setup. Assert: after `ApplyInverse`, the captured fighter's `GetOwner()` is back to the loser and `BattleLossesTable(db).QueryForOwner(loser).Step()` is false; after re-apply, the whole-DB hash matches the post-capture snapshot.

- [ ] **Step 2: Run the suite to see the expected golden mismatch** — Docker `make check`.
Expected: everything passes EXCEPT `GoldenReplay.FullScenario`, which mismatches (reward outcomes shifted under divisor 4, and/or the scenario now records a tournament loss). The reorg test and all unit tests pass.

- [ ] **Step 3: Review the golden diff** — compare `src/goldenreplay.actual.json` against `src/goldenreplay.golden.json` (the test always writes `.actual.json`). Confirm the diff is bounded to: (a) reward-roll OUTCOMES changing (different rewards chosen under the scaled distribution, and any downstream RNG from a differently-typed chosen reward); (b) the new `parameters` rows in state; (c) if the scenario resolves a tournament with a loss, tournament fighter ownership/deletion + a `battle_losses` row + a bumped `rewards_serial`. There must be NO unrelated state change, crash, or structural corruption. Record the review conclusion.

- [ ] **Step 4: Regenerate the golden** — after review, regenerate and copy it back to the repo:

```bash
docker run --rm --network none \
  -v /home/acoloss/treatfighter/tfgsp-polygon:/build-rw \
  -v /tmp/tf-ccache:/ccache \
  --entrypoint bash tf-builder:local -c '
    set -e
    export CCACHE_DIR=/ccache CC="ccache gcc" CXX="ccache g++"
    cd /build-rw
    ./autogen.sh && ./configure && make -j"$(nproc)"
    cd src && GOLDEN_REGEN=1 ./goldenreplay_tests'
```

(This bind-mounts the host repo read-write so the regenerated `src/goldenreplay.golden.json` lands directly in the tree. Alternatively regen in a copy and `cp` it out.)

- [ ] **Step 5: Full green** — re-run the standard read-only Docker `make check` (internal copy). Expected: ALL suites pass, including `GoldenReplay.FullScenario` against the regenerated golden, `reorg_tests`, `reorg_game_tests`, and `check-local` (float/double linter). Confirm `RoConfigTests.LaunchConfigIsPinned` still passes UNCHANGED (we did not touch RoConfig).

- [ ] **Step 6: Commit**

```bash
git add src/reorg_tests.cpp src/goldenreplay.golden.json
git commit -m "test(gsp): capture reorg-reverts test; regenerate golden for divisor+permadeath"
```

- [ ] **Step 7: Genesis re-pin note (deferred launch action).** Record in the launch punch-list that the genesis STATE hash changed (new `parameters`/`battle_losses` tables + seeded param rows + RNG-outcome shift) → the mainnet genesis re-pin folds into the existing pre-mainnet re-pin blocker. No code action now (pre-launch, fresh genesis, no migration). The RoConfig `PINNED_HASH` is unchanged.

---

## Self-Review (writing-plans checklist)

**1. Spec coverage:**
- Change 0 (runtime params): Task 1 (table + accessor + genesis/test seeding) + Task 2 (admin verb) ✓
- Change B (Epic 4×): Task 3 (shared helper + all 5 roll sites incl. the tournament reward roll) ✓
- Change A (permadeath + 50/50 capture): Task 5 ✓ (starter protection, cap→destroy fallback, draw→no deaths, kills_enabled gate, multi-team via per-team loop, owner-ascending, victim-then-capture draw order) ✓
- Change C (loss notification): Task 4 (battle_losses table + getuser) + Task 5 (write row + bump serial) + Task 6 (FighterLossModal reveal) ✓
- Consensus/rollout: Task 7 (reorg test, golden regen after diff review, genesis-pin note; RoConfig pin untouched) ✓
- Testing spec items (params seed/set/remove/unknown/range; permadeath destroy/capture/fallback/starter/draw/disabled/multi-team; loss record + serial; divisor A/B; golden regen; reorg capture revert) — all covered ✓

**2. Placeholder scan:** The only intentional `<...>` are the GPL-header copy directives (copy verbatim from the named existing file) and the `RunTwoPlayerTournament` helper (Task 5 Step 2), which is specified by exact reference to the quoted `TournamentResolvedTest` sequence plus the two concrete additions (set `isaccountbound`, record winner/loser). No "TBD"/"add error handling"/"similar to Task N".

**3. Type consistency:** `GameParams` (Get/Set/Remove/Has/InitialiseDatabase) is used identically in Tasks 1/2/3/5. `PickWeightedRewardIndex(table, divisor, rnd)->int32_t` and `ScaledRewardWeights(table, divisor)->vector<uint32_t>` match between logic.hpp (Task 3 decl) and all call sites. `BattleLossesTable::Add(owner, fighterId, name, outcome, opponent, tournamentId)` + `BattleLossResult` columns match between Task 4 (def), Task 5 (write), Task 7 (reorg assert), and the getuser emission. FE `BattleLoss{id,fighterid,name,outcome,opponent,tournamentid}` matches the GSP `battlelosses` JSON keys exactly. `RewardWon.lostFighters: BattleLoss[]` used consistently in reward-reveal.ts, RevealSwitch.tsx, FighterLossModal.tsx, page.tsx.

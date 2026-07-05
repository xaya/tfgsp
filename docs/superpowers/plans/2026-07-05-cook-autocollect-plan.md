# Cook Auto-Collect Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking. GSP tasks (1–2) land FIRST — the frontend depends on the GSP `rewards_serial` bump to detect a finished cook.

**Goal:** Remove the manual "collect a finished cook" transaction (`ca.cl`). A finished cook auto-lands as a usable `Available` fighter at resolve; the client reveals it dramatically (no second tx). Mirrors the shipped reward auto-credit pattern.

**Architecture:** GSP: in `ResolveCookingRecepie`, set the new fighter `Available` (was `Cooked`) and bump the player's append-only `rewards_serial`; bump the serial on the full-roster refund branch too so the client always learns the outcome. Delete the entire `ca.cl` verb + its `Cooked` status gate. FE: extend the reward-reveal inventory diff to detect a newly-appeared fighter id (⇒ dramatic `FighterRevealModal`) and a full-roster refund (a cook ongoing vanished with no new fighter ⇒ "roster full — refunded" notice), reusing the existing `rewardsserial`-driven auto-pop; remove the collect button + all `Cooked` wording; add a proactive roster-cap hint.

**Tech Stack:** GSP = C++17, SQLite (libxayagame), protobuf, GoogleTest, autotools — builds ONLY in the `tf-builder:local` Docker image, `--network none`. FE = Next.js 16, TypeScript, viem, three.js, vitest.

## Global Constraints

Every task's requirements implicitly include this section.

- **Never rename `recepie`/`recepies`/`Recepie`** — the misspelling is the on-chain move grammar and DB/proto identifier. Copy it verbatim wherever it appears.
- **GSP builds ONLY in Docker**, `tf-builder:local`, `--network none`, in an internal `/build` copy of a read-only `/src-ro` mount (never build in the host tree). Exact recipe in every GSP task.
- **Determinism is money-critical.** Do NOT add, remove, or reorder any `rnd` draw. `fighters.CreateNew(..., rnd)` stays at its exact sequence point. A status value and a `rewards_serial` bump consume no entropy.
- **Golden regen ONLY after diff review.** Task 1 legitimately changes observable state; regenerate the golden and confirm the diff is EXACTLY `{one fighter status 7→0, domob rewardsserial 0→1}` — nothing else. Task 2 must NOT change the golden.
- **Fresh-genesis basis:** the DB is wiped at launch and there are no in-flight `ca.cl` moves to honor — delete the verb outright (no migration, no no-op stub).
- **Code quality:** clean, compact, DRY, well-commented in the surrounding house style. This is a *removal* — leave no dead symbol, dead comment, or orphaned wiring behind.
- **`dev_address` = `0x25763F408461ddc66d8955d22963815DEA6d8722`** (unchanged; not touched by this work).

## File Structure

**GSP (`tfgsp-polygon`, branch `polygon-rewrite`):**
- `src/logic_resolve.cpp` — `ResolveCookingRecepie`: the behavior change (Available + serial bumps).
- `src/logic_tests.cpp` — extend `RecepieInstanceFullCycleTest` + `RecepieInstanceRevertIfFullRoster`; add `CollectCookMoveIsInert`.
- `src/goldenreplay.golden.json` — regenerated (status + serial drift).
- `src/moveprocessor.cpp` / `.hpp`, `src/moveprocessor_cooking.cpp`, `src/pending.cpp` / `.hpp` — delete the `ca.cl` verb + parser + handler + pending plumbing.
- `database/fighter.hpp` — remove the `Cooked = 7` enum value.

**FE (`tf-frontend`, branch `master`):**
- `src/data/reward-reveal.ts` — extend `InvSnapshot` + `RewardWon` + `snapshotInventory` + `rewardDelta` (new-fighter + refund detection).
- `tests/data/reward-reveal.test.ts` — invert the "ignores new fighters" test; add refund tests.
- `src/lib/chain/move-builder.ts` — delete `collectCook`.
- `src/ui/fighters/FighterDetail.tsx`, `src/data/types.ts`, `src/data/fighter-display.ts`, `src/ui/fighters/FightersScreen.tsx` — remove the collect button + all `Cooked` wording/enum.
- `tests/data/fighter-status.test.ts`, `tests/data/fighter-display.test.ts` — drop the `Cooked` assertions.
- `src/ui/chrome/FighterRevealModal.tsx` (NEW) + `src/ui/chrome/RevealSwitch.tsx` (NEW) — the dramatic reveal + the reward/fighter/refund modal selector.
- `src/ui/chrome/RewardRevealModal.tsx` — add the `cookRefunded` framing branch.
- `app/page.tsx` — route reveals through `RevealSwitch`.
- `src/ui/cooking/CookingScreen.tsx` — proactive roster-cap hint.
- `src/data/tutorial/fake-state.ts`, `src/data/tutorial/script.ts`, `src/data/tutorial/tutorial-store.ts` — the sandbox cook step raises the dramatic reveal.

---

## GSP BUILD & TEST RECIPE (used verbatim by every GSP task)

Run from a scratch dir. `REPO=/home/acoloss/treatfighter/tfgsp-polygon`, `SCRATCH=<your scratch dir>`.

```bash
# Full build + check (compiles proto/database/src, builds all check_PROGRAMS, runs make check).
docker run --rm --network none \
  -v "$REPO":/src-ro:ro -v "$SCRATCH":/scratch --entrypoint bash \
  tf-builder:local -c '
    set -e
    cp -a /src-ro /build && cd /build
    make distclean 2>/dev/null || true
    ./autogen.sh && ./configure
    make -j"$(nproc)"
    make -C database tests            # builds libdbtest.la + database/tests
    make -C src tests goldenreplay_tests   # src unit tests + the golden binary
    make check                        # proto2 + database/tests + src/{tests,goldenreplay_tests,reorg_tests,reorg_game_tests} + lint
  '
```

To run a single suite fast during TDD (skip full `make check`):
```bash
# ... same build prefix through `make -C src tests goldenreplay_tests` ...
cd /build/src && ./tests --gtest_filter="ValidateStateTests.RecepieInstance*"
```

**Golden regen (Task 1 only, after diff review):** build as above, then inside the SAME container run and copy the regenerated golden OUT via the `/scratch` mount, because the build happens in the throwaway `/build` copy:
```bash
docker run --rm --network none \
  -v "$REPO":/src-ro:ro -v "$SCRATCH":/scratch --entrypoint bash \
  tf-builder:local -c '
    set -e
    cp -a /src-ro /build && cd /build
    ./autogen.sh && ./configure && make -j"$(nproc)"
    make -C database tests && make -C src goldenreplay_tests
    cd /build/src && GOLDEN_REGEN=1 ./goldenreplay_tests --gtest_filter="GoldenReplayTests.*"
    cp /build/src/goldenreplay.golden.json /scratch/goldenreplay.golden.json
  '
# On the host: review, then install.
diff -u "$REPO/src/goldenreplay.golden.json" "$SCRATCH/goldenreplay.golden.json"   # MUST be only status 7→0 + one rewardsserial 0→1
cp "$SCRATCH/goldenreplay.golden.json" "$REPO/src/goldenreplay.golden.json"
```

---

## Task 1: GSP — cook resolves to Available + bump rewards_serial (both outcomes)

**Files:**
- Modify: `src/logic_resolve.cpp` — `ResolveCookingRecepie` (currently lines 684–748).
- Test: `src/logic_tests.cpp` — extend `RecepieInstanceFullCycleTest` (196–237) and `RecepieInstanceRevertIfFullRoster` (425–462).
- Regenerate: `src/goldenreplay.golden.json`.

**Interfaces:**
- Consumes: `XayaPlayer::MutableProto().set_rewards_serial(...)` / `GetProto().rewards_serial()` (proto field 18, already present); `FighterInstance::SetStatus`, `FighterStatus::Available`; `FighterTable::CreateNew(owner, recipeID, cfg, rnd)`.
- Produces: after this task a resolved cook yields an `Available` fighter and `rewards_serial += 1` (success), and a full-roster refund also does `rewards_serial += 1`. Task 2 (verb deletion) and the whole FE side depend on this serial bump.

- [ ] **Step 1: Extend the success-path test to pin Available + serial.** In `RecepieInstanceFullCycleTest`, after the existing line `EXPECT_EQ (a->CollectInventoryFighters(ctx.RoConfig()).size(), 3);` (line 226), add:

```cpp
  /* Auto-collect: a finished cook lands as a usable Available fighter -- no
     second "collect" tx -- and every fighter domob owns is Available. */
  for (const auto& f : a->CollectInventoryFighters (ctx.RoConfig ()))
    EXPECT_EQ (f->GetStatus (), pxd::FighterStatus::Available);
  /* The cook credits the append-only auto-collect serial (mirrors reward
     auto-credit) so the client can reveal the new fighter with no claim tx. */
  EXPECT_EQ (a->GetProto ().rewards_serial (), 1u);
```

- [ ] **Step 2: Extend the refund-path test to pin the serial bump.** In `RecepieInstanceRevertIfFullRoster`, after the existing line `EXPECT_EQ (a->GetBalance (), 100 + cfg.params().starting_crystals());` (line 454), add:

```cpp
  /* A full-roster refund still notifies the client via the same serial the
     success path bumps, so the FE always surfaces the outcome ("roster full --
     cook refunded"), never a silent items-came-back diff. */
  EXPECT_EQ (a->GetProto ().rewards_serial (), 1u);
```

- [ ] **Step 3: Run both tests — expect FAIL.** Use the single-suite recipe with `--gtest_filter="ValidateStateTests.RecepieInstanceFullCycleTest:ValidateStateTests.RecepieInstanceRevertIfFullRoster"`. Expected: both FAIL — `FullCycleTest` sees a `Cooked (7)` fighter and `rewards_serial 0`; `RevertIfFullRoster` sees `rewards_serial 0`.

- [ ] **Step 4: Implement the resolve change.** In `ResolveCookingRecepie`, add a serial-bump lambda right after the tables are constructed (after `RecipeInstanceTable recipeTbl(db);`, line 690):

```cpp
    /* Bump the player's append-only rewards serial so the client's existing
       serial-driven auto-pop reveals this cook's outcome with NO claim tx
       (mirrors CreditActivityReward's bumpSerial).  Pure bookkeeping -- it
       consumes no rnd, so the RNG stream stays byte-identical. */
    const auto bumpSerial = [&a] ()
    {
      a->MutableProto ().set_rewards_serial (a->GetProto ().rewards_serial () + 1);
    };
```

In the refund branch, after `recepie->SetOwner(a->GetName());` (line 731), add:

```cpp
        bumpSerial ();
```

In the success branch, replace the `SetStatus(FighterStatus::Cooked)` + `LOG` block (lines 740–742):

```cpp
        newFighter->SetStatus(FighterStatus::Cooked);

        LOG (WARNING) << "Cooked new fighter with id: " << newFighter->GetId();
```

with:

```cpp
        /* Auto-collect: the finished cook lands directly usable -- no second
           "collect" tx.  The client reveals it from the bumpSerial() below. */
        newFighter->SetStatus(FighterStatus::Available);
        bumpSerial ();

        LOG (INFO) << "Auto-collected cooked fighter with id: " << newFighter->GetId();
```

Leave `CreateNew(..., rnd)`, `CalculatePrestige`, and the "recipe row intentionally kept" comment exactly as they are.

- [ ] **Step 5: Run both tests — expect PASS.** Same filter as Step 3. Both green.

- [ ] **Step 6: Full `make check` — expect ONE failure: GoldenReplay.** Run the full recipe. Every suite passes EXCEPT `GoldenReplayTests.FullScenario`, which now diverges (the golden still records the pre-change cooked fighter). This is expected and is fixed by the deliberate regen next.

- [ ] **Step 7: Regenerate the golden and review the diff.** Use the golden-regen recipe. The host-side `diff -u` MUST show ONLY: exactly one fighter `"status" : 7` → `"status" : 0`, and exactly one `"rewardsserial" : 0` → `"rewardsserial" : 1` (domob). If any other field drifts, STOP — a `rnd` draw moved; do not install the golden. Install the reviewed golden into `src/`.

- [ ] **Step 8: Full `make check` — expect ALL GREEN.** Re-run the full recipe; GoldenReplay now matches.

- [ ] **Step 9: Commit.**

```bash
git add src/logic_resolve.cpp src/logic_tests.cpp src/goldenreplay.golden.json
git commit -m "feat(gsp): cook auto-collects to Available + bumps rewards_serial (both outcomes)

A finished cook lands as a usable Available fighter (was Cooked) and bumps the
player's append-only rewards_serial so the client reveals it with no claim tx;
the full-roster refund bumps the serial too so the outcome is never silent. No
rnd draw moves -- golden diff is exactly {status 7->0, domob rewardsserial 0->1}.

Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>"
```

---

## Task 2: GSP — delete the `ca.cl` verb + `Cooked` status entirely

**Files:**
- Modify: `src/moveprocessor.cpp` (388–393), `src/moveprocessor.hpp` (195–199, 378–381), `src/moveprocessor_cooking.cpp` (318–360 parser, 612–632 handler), `src/pending.cpp` (116–121, 386–395, 595–598), `src/pending.hpp` (84–87, 189–191), `database/fighter.hpp` (73).
- Test: `src/logic_tests.cpp` — add `CollectCookMoveIsInert`.

**Interfaces:**
- Consumes: nothing new. This is a pure deletion enabled by Task 1 (nothing sets `Cooked` anymore).
- Produces: the `ca.cl` move is unknown/ignored; `FighterStatus::Cooked` no longer exists. Observable game state is UNCHANGED (the golden scenario never exercised `ca.cl` or held a `Cooked` fighter after Task 1), so the golden must stay byte-identical.

- [ ] **Step 1: Add the inert-move regression test.** In `src/logic_tests.cpp`, add after `RecepieInstanceRevertIfFullRoster` (after line 462):

```cpp
TEST_F (ValidateStateTests, CollectCookMoveIsInert)
{
  /* ca.cl is deleted (cooks auto-collect at resolve).  A stale client sending it
     must be a harmless no-op: the move parses without effect and no fighter
     changes state. */
  xayaplayers.CreateNew ("domob", ctx.RoConfig(), rnd)->AddBalance (100);

  auto a = xayaplayers.GetByName ("domob", ctx.RoConfig());
  const auto owned = a->CollectInventoryFighters (ctx.RoConfig());
  ASSERT_FALSE (owned.empty ());
  const int fid = owned.front ()->GetId ();
  a.reset ();

  Process (R"([
    {"name": "domob", "move": {"ca": {"cl": {"fid": )" + std::to_string (fid) + R"(}}}}
  ])");

  /* The fighter is untouched -- still Available, still owned by domob. */
  auto ft = tbl3.GetById (fid, ctx.RoConfig());
  ASSERT_TRUE (ft != nullptr);
  EXPECT_EQ (ft->GetStatus (), pxd::FighterStatus::Available);
  EXPECT_EQ (ft->GetOwner (), "domob");
}
```

- [ ] **Step 2: Run the new test against current code — expect PASS (it is already inert on a non-Cooked fighter, but this pins it for the deletion).** Filter `ValidateStateTests.CollectCookMoveIsInert`. It passes now (the handler rejects a non-`Cooked` fighter); the point is it must STILL pass after the deletion. Keep going.

- [ ] **Step 3: Delete the dispatch branch.** In `src/moveprocessor.cpp`, remove lines 388–393 (the `/*Trying to collect cooked recepie*/` block):

```cpp
    /*Trying to collect cooked recepie*/
	const auto& ref3 = upd["cl"];
    if (!ref3.isNull ())
	{
		MaybeCollectCookedRecepie (name, upd["cl"]); 
	}	
    
```

- [ ] **Step 4: Delete the handler + parser.** In `src/moveprocessor_cooking.cpp`, delete `MoveProcessor::MaybeCollectCookedRecepie` (612–632) and `BaseMoveProcessor::ParseCollectCookRecepie` (318–360, including the `!= FighterStatus::Cooked` gate — the last consumer of `Cooked`). In `src/moveprocessor.hpp`, delete the `MaybeCollectCookedRecepie` declaration + its comment (378–381) and the `ParseCollectCookRecepie` declaration + its comment (195–199).

- [ ] **Step 5: Delete the pending plumbing.** In `src/pending.cpp`: delete `AddCookedRecepieCollectInstance` (116–121); delete the `cookedFightersToCollect` JSON-emit block (386–395); delete the `ParseCollectCookRecepie`/`AddCookedRecepieCollectInstance` call block (595–598). In `src/pending.hpp`: delete the `AddCookedRecepieCollectInstance` declaration + comment (84–87) and the `cookedFightersToCollect` member + comment (189–191). Verify no other reference remains: `grep -rn "cookedFightersToCollect\|CollectCookedRecepie\|CollectCookRecepie\|AddCookedRecepieCollect\|MaybeCollectCooked" src/` must return nothing.

- [ ] **Step 6: Remove the `Cooked` enum value.** In `database/fighter.hpp`, delete the line `  Cooked = 7,` (73). Leave the other values and the `// 6 retired` comment as-is (do NOT renumber). Confirm `grep -rn "Cooked" src/ database/` returns nothing.

- [ ] **Step 7: Full `make check` — expect ALL GREEN, golden byte-identical.** Run the full recipe. Everything compiles (the last `Cooked` reference died with the parser); `CollectCookMoveIsInert` passes; `GoldenReplayTests.FullScenario` PASSES with the Task-1 golden unchanged (this task changed no observable state). If the golden diverges, something is wrong — investigate, do NOT regen.

- [ ] **Step 8: Commit.**

```bash
git add src/moveprocessor.cpp src/moveprocessor.hpp src/moveprocessor_cooking.cpp \
        src/pending.cpp src/pending.hpp database/fighter.hpp src/logic_tests.cpp
git commit -m "refactor(gsp): delete the ca.cl collect verb + the Cooked status

Cooks auto-collect at resolve (prior commit), so the ca.cl verb, its parser, its
Cooked-status gate, and all pending-state plumbing are dead -- removed wholesale,
including the FighterStatus::Cooked enum value (nothing produces it now). A stale
ca.cl move is a harmless no-op (CollectCookMoveIsInert). Golden byte-identical.

Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>"
```

---

## Task 3: FE — extend the reward-reveal diff to detect a new fighter + a cook refund

**Files:**
- Modify: `src/data/reward-reveal.ts`.
- Test: `tests/data/reward-reveal.test.ts`.

**Interfaces:**
- Consumes: `UserSlice`, `OngoingType` (`@/data/types`; `OngoingType.CookRecipe === 1`).
- Produces: `RewardWon` gains `newFighters: number[]` and `cookRefunded: boolean`; `InvSnapshot` gains `fighterIds: number[]` and `cookOngoings: number`. Tasks 5 & 6 consume these to choose the reveal.

- [ ] **Step 1: Write the failing tests.** In `tests/data/reward-reveal.test.ts`, add `import { OngoingType } from '@/data/types';` to the type import line, then REPLACE the `it('is empty when nothing changed, and ignores crystals/new-fighters ...')` test (lines 50–56) with:

```ts
  it('detects a newly appeared fighter (a finished cook auto-collected it)', () => {
    const before = snapshotInventory(slice({ fighters: [] }), 'me');
    const after = snapshotInventory(slice({ fighters: [fighter({ id: 9 })] }), 'me');
    const won = rewardDelta(before, after);
    expect(won.newFighters).toEqual([9]);
    expect(won.empty).toBe(false);
  });

  it('still ignores a rising crystal balance (claims never mint crystals)', () => {
    const acct = slice({}).account!;
    const before = snapshotInventory(slice({ account: { ...acct, balance: { available: 100 } } }), 'me');
    const after = snapshotInventory(slice({ account: { ...acct, balance: { available: 500 } } }), 'me');
    expect(rewardDelta(before, after).empty).toBe(true);
  });

  it('flags a full-roster cook refund (a cook ongoing vanished with no new fighter)', () => {
    const acct = slice({}).account!;
    const before = snapshotInventory(
      slice({ account: { ...acct, ongoings: [{ type: OngoingType.CookRecipe, blocksleft: 0, recipeid: 1 }] } }), 'me');
    const after = snapshotInventory(slice({ account: { ...acct, ongoings: [] } }), 'me');
    const won = rewardDelta(before, after);
    expect(won.cookRefunded).toBe(true);
    expect(won.newFighters).toEqual([]);
    expect(won.empty).toBe(false);
  });

  it('a successful cook is NOT a refund (ongoing vanished but a fighter appeared)', () => {
    const acct = slice({}).account!;
    const before = snapshotInventory(
      slice({ account: { ...acct, ongoings: [{ type: OngoingType.CookRecipe, blocksleft: 0, recipeid: 1 }] }, fighters: [] }), 'me');
    const after = snapshotInventory(slice({ account: { ...acct, ongoings: [] }, fighters: [fighter({ id: 5 })] }), 'me');
    const won = rewardDelta(before, after);
    expect(won.newFighters).toEqual([5]);
    expect(won.cookRefunded).toBe(false);
  });
```

- [ ] **Step 2: Run — expect FAIL** (`newFighters`/`cookRefunded` undefined). `npx vitest run tests/data/reward-reveal.test.ts`.

- [ ] **Step 3: Implement.** In `src/data/reward-reveal.ts`:

Change the import to pull in `OngoingType`:
```ts
import { OngoingType, type UserSlice } from './types';
```

Extend `InvSnapshot` (after the `fighters` field):
```ts
  /** every owned fighter id — a new id in `after` ⇒ a finished cook auto-collected a fighter. */
  fighterIds: number[];
  /** active COOK_RECIPE ongoings — a drop with no new fighter ⇒ a full-roster cook refund. */
  cookOngoings: number;
```

Extend `snapshotInventory`'s return (add the two fields):
```ts
  return {
    fungible: { ...(u?.account?.inventory.fungible ?? {}) },
    ownedRecipeDids: (u?.recepies ?? []).filter((r) => r.owner === playerName).map((r) => r.did),
    fighters,
    fighterIds: (u?.fighters ?? []).map((f) => f.id),
    cookOngoings: (u?.account?.ongoings ?? []).filter((o) => o.type === OngoingType.CookRecipe).length,
  };
```

Extend `RewardWon` (after `fighterUpgrades`):
```ts
  /** Fighters that newly appeared since the baseline — a finished cook auto-collected them. */
  newFighters: number[];
  /** A cook resolved into a full-roster refund: a cook ongoing vanished with no new fighter. */
  cookRefunded: boolean;
```

In `rewardDelta`, before the `empty` line, add:
```ts
  const beforeF = new Set(before.fighterIds);
  const newFighters = after.fighterIds.filter((id) => !beforeF.has(id));

  // A cook ongoing only disappears when it resolves; more resolved than fighters gained ⇒ ≥1 refunded.
  const cookRefunded = (before.cookOngoings - after.cookOngoings) > newFighters.length;
```

Update `empty` and the return:
```ts
  const empty = fungible.length === 0 && newRecipes === 0 && fighterUpgrades.length === 0
    && newFighters.length === 0 && !cookRefunded;
  return { fungible, newRecipes, fighterUpgrades, newFighters, cookRefunded, empty };
```

Also update the file's top doc-comment: the diff now DOES surface auto-collected fighters and full-roster cook refunds (it previously said claims never mint fighters — that predates cook auto-collect).

- [ ] **Step 4: Run — expect PASS** (all reward-reveal tests, including the pre-existing candy/recipe/upgrade ones). `npx vitest run tests/data/reward-reveal.test.ts`.

- [ ] **Step 5: Commit.**

```bash
git add src/data/reward-reveal.ts tests/data/reward-reveal.test.ts
git commit -m "feat(fe): reward-reveal diff detects auto-collected fighters + cook refunds

snapshotInventory now records owned fighter ids + active cook-ongoing count;
rewardDelta surfaces newFighters (a finished cook) and cookRefunded (a cook
ongoing vanished with no new fighter — a full-roster refund).

Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>"
```

---

## Task 4: FE — remove the collect button + all `Cooked` wording

**Files:**
- Modify: `src/lib/chain/move-builder.ts` (31–32), `src/ui/fighters/FighterDetail.tsx` (15, 42–44, 55–57, 119–122), `src/data/types.ts` (63 comment, 109 enum, 121 label), `src/data/fighter-display.ts` (81), `src/ui/fighters/FightersScreen.tsx` (35).
- Test: `tests/data/fighter-status.test.ts` (13, 22), `tests/data/fighter-display.test.ts` (69).

**Interfaces:**
- Consumes: nothing new.
- Produces: no `collectCook` builder, no `FighterStatus.Cooked`, no collect UI. Independent of Tasks 3/5/6.

- [ ] **Step 1: Delete `collectCook`.** In `src/lib/chain/move-builder.ts`, remove lines 31–32:
```ts
/** Collect a finished cook for fighter `fid`. */
export const collectCook = (fid: number): TfMove => ({ ca: { cl: { fid } } });
```

- [ ] **Step 2: Remove the collect button from `FighterDetail.tsx`.**
  - Line 15: `import { deconstructFighter, collectCook } from ...` → `import { deconstructFighter } from '@/lib/chain/move-builder';`
  - Line 44: delete `const canCollect = fighter.status === FighterStatus.Cooked;`
  - Lines 55–57: delete the `onCollect` handler.
  - Lines 119–122: delete the `{canCollect && (<button ...>Collect fighter</button>)}` block, leaving the `fdactions` div with the sweeten CTA + deconstruct.

- [ ] **Step 3: Strip `Cooked` from `types.ts`.**
  - Line 63 comment: remove the `7 Cooked (ready to collect),` clause from the status-doc list.
  - Line 109: remove `Cooked: 7,` from the `FighterStatus` object (leave the other members; do not renumber).
  - Lines 121: delete the `case FighterStatus.Cooked: return 'Cooked (ready to collect)';` arm from `fighterStatusLabel` (the `default` already covers any stray value).

- [ ] **Step 4: Strip `Cooked` from `fighter-display.ts`.** Line 81: delete `case FighterStatus.Cooked: return 'COOKED';` from `rosterStatusLabel`.

- [ ] **Step 5: Strip `Cooked` from `FightersScreen.tsx`.** Line 35: delete the `[FighterStatus.Cooked]: 'recipe',` entry from `STATUS_ICON`.

- [ ] **Step 6: Update the two tests.**
  - `tests/data/fighter-status.test.ts`: line 13, remove `Cooked: 7,` from the mirrored enum literal; line 22, delete the `expect(fighterStatusLabel(FighterStatus.Cooked)).toMatch(/collect/i);` assertion (and any now-dangling reference to `FighterStatus.Cooked`).
  - `tests/data/fighter-display.test.ts`: line 69, delete the `expect(rosterStatusLabel(FighterStatus.Cooked, 0)).toBe('COOKED');` assertion.

- [ ] **Step 7: Verify no `Cooked`/`collectCook` remain.** `grep -rn "Cooked\|COOKED\|collectCook\|canCollect\|onCollect" src app tests` → nothing. `npx tsc --noEmit` clean.

- [ ] **Step 8: Build + full test run — expect GREEN.** `npm run build` and `npx vitest run`.

- [ ] **Step 9: Commit.**

```bash
git add src/lib/chain/move-builder.ts src/ui/fighters/FighterDetail.tsx src/data/types.ts \
        src/data/fighter-display.ts src/ui/fighters/FightersScreen.tsx \
        tests/data/fighter-status.test.ts tests/data/fighter-display.test.ts
git commit -m "refactor(fe): remove the cook-collect button + all Cooked wording

Cooks auto-collect on-chain, so the collectCook builder, the FighterDetail
Collect button, and the FighterStatus.Cooked enum value + its labels/icons are
dead — removed.

Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>"
```

---

## Task 5: FE — dramatic FighterRevealModal + refund notice + proactive cap hint

**Files:**
- Create: `src/ui/chrome/FighterRevealModal.tsx`, `src/ui/chrome/RevealSwitch.tsx`.
- Modify: `src/ui/chrome/RewardRevealModal.tsx`, `app/page.tsx` (346–347), `src/ui/cooking/CookingScreen.tsx`.
- Modify (styles): `app/global.css` (new reveal classes).

**Interfaces:**
- Consumes: `RewardWon` (with `newFighters`, `cookRefunded` from Task 3); `useGame().user.fighters`; `MAX_FIGHTER_INVENTORY` (`@/data/exchange`); the `FighterCanvas` dynamic import + `fighter-display` helpers (as `FighterDetail` uses them); `useUiStore().setTab`.
- Produces: `RevealSwitch` — one selector used by both the live and tutorial reveal sites. `FighterRevealModal({ fighterIds, onClose })`.

- [ ] **Step 1: Add the `cookRefunded` branch to `RewardRevealModal.tsx`.** When `won.cookRefunded` (and no new fighter — that case never reaches this modal, see `RevealSwitch`), the modal is a refund notice, not a "You won!": override the hero/title/subline and still list what came back (`won.fungible` + `won.newRecipes`). Concretely, compute:

```tsx
  const refund = won.cookRefunded;
```
then use it for the title and an explanatory line:
```tsx
        <h2 className="reward-title">{refund ? 'Roster full — cook refunded' : won.empty ? 'Rewards' : 'You won!'}</h2>
        {refund && (
          <p className="reward-refund-note">
            Your roster is full ({/* count */}), so the finished cook was refunded — its recipe and
            candy are back in your pantry. Free a slot, then cook again.
          </p>
        )}
```
For the count, read `useGame().user?.fighters.length` (already have `user`). Keep the existing items list (`won.fungible` / `won.newRecipes`) so the player sees exactly what returned. Suppress the `'treatfighter'` win-sting on a refund (`useEffect(() => { if (!won.empty && !won.cookRefunded) audio.sfx('treatfighter'); }, [won.empty, won.cookRefunded]);`). Change the close button label to `Close` on a refund.

- [ ] **Step 2: Create `FighterRevealModal.tsx`.** A dramatic dedicated reveal for a freshly cooked fighter. Model the shell on `RewardRevealModal` (portal to `document.body`, plays the `'treatfighter'` sting on mount) and the 3D portrait on `FighterDetail` (the `next/dynamic` `FighterCanvas`, `ssr:false`). Props `{ fighterIds: number[]; onClose: () => void }`. Look the fighter up in `useGame().user.fighters` (reveal the first; if `fighterIds.length > 1`, add a "+N more in your roster" line). Render: rarity-colored name (`qualityColor`), the `FighterCanvas` portrait in a `fighter-reveal-portrait` container with a CSS entrance animation, a stat roll-up (Rating / Sweetness — a `useEffect` counting 0→value over ~600ms, guarded by `prefers-reduced-motion`), a rarity flare behind the portrait, and two buttons: **View in roster** (`setTab('fighters'); onClose();`) and **Nice!** (`onClose`). If the fighter id isn't found in `user.fighters` yet (race), fall back to a minimal "Your new fighter is ready!" card with just the buttons. Use `createPortal`, `'use client'`, and the same class-naming style as the existing modals.

- [ ] **Step 3: Create `RevealSwitch.tsx`** — the single selector both reveal sites use (DRY):

```tsx
'use client';

import type { RewardWon } from '@/data/reward-reveal';
import { RewardRevealModal } from './RewardRevealModal';
import { FighterRevealModal } from './FighterRevealModal';

/** One reveal, routed by its payload: a freshly cooked fighter gets the dramatic 3D reveal; everything
 *  else (rewards, or a full-roster cook refund) goes through the reward modal (which self-frames a
 *  refund). Used by both the live serial-driven pop and the sandbox tutorial. */
export function RevealSwitch({ won, onClose }: { won: RewardWon; onClose: () => void }): React.JSX.Element {
  if (won.newFighters.length > 0) return <FighterRevealModal fighterIds={won.newFighters} onClose={onClose} />;
  return <RewardRevealModal won={won} onClose={onClose} />;
}
```

- [ ] **Step 4: Route both reveal sites through `RevealSwitch` in `app/page.tsx`.** Replace the two render lines (346–347):
```tsx
          {revealWon && !tut.active && <RewardRevealModal won={revealWon} onClose={() => setRevealWon(null)} />}
          {tut.pendingReveal && <RewardRevealModal won={tut.pendingReveal} onClose={() => tutorialStore.notifyRevealClosed()} />}
```
with:
```tsx
          {revealWon && !tut.active && <RevealSwitch won={revealWon} onClose={() => setRevealWon(null)} />}
          {tut.pendingReveal && <RevealSwitch won={tut.pendingReveal} onClose={() => tutorialStore.notifyRevealClosed()} />}
```
Update the import: drop the direct `RewardRevealModal` import if now unused here, add `import { RevealSwitch } from '@/ui/chrome/RevealSwitch';`. (`RewardRevealModal` stays imported inside `RevealSwitch`.)

- [ ] **Step 5: Proactive roster-cap hint in `CookingScreen.tsx`.** Import `MAX_FIGHTER_INVENTORY` from `@/data/exchange`. Compute `const rosterSize = user?.fighters.length ?? 0;` and `const nearCap = rosterSize >= MAX_FIGHTER_INVENTORY - 2;`. Render a prominent warning line directly under the `potbar` when `nearCap`:
```tsx
      {nearCap && (
        <p className={rosterSize >= MAX_FIGHTER_INVENTORY ? 'err' : 'note'} style={{ marginTop: 8 }}>
          ⚠ Roster {rosterSize >= MAX_FIGHTER_INVENTORY ? 'full' : 'almost full'} ({rosterSize}/{MAX_FIGHTER_INVENTORY}) —
          a cook that finishes with no free slot is refunded (recipe + candy returned). Free a slot to keep your Treat.
        </p>
      )}
```
Also fix the stale line-6 doc comment ("appears in the roster as Cooked, collectable there") → the cook now auto-lands Available and reveals with no collect step.

- [ ] **Step 6: Add reveal CSS to `app/global.css`.** Add classes for `.fighter-reveal-modal`, `.fighter-reveal-portrait` (entrance keyframes), `.fighter-reveal-flare` (rarity glow), `.fighter-reveal-stats`, `.reward-refund-note`. Keep the palette/rounding consistent with the existing `.reward-modal`. Gate the entrance/roll-up animations behind `@media (prefers-reduced-motion: no-preference)`.

- [ ] **Step 7: Build + typecheck + full test run — expect GREEN.** `npm run build`, `npx tsc --noEmit`, `npx vitest run`. (The modals are thin view components; their logic — the `RevealSwitch` routing — is covered by Task 3's `newFighters`/`cookRefunded` tests. Deep visual verification via `render:shots` is deferred to the live-verify step with the user.)

- [ ] **Step 8: Commit.**

```bash
git add src/ui/chrome/FighterRevealModal.tsx src/ui/chrome/RevealSwitch.tsx \
        src/ui/chrome/RewardRevealModal.tsx app/page.tsx src/ui/cooking/CookingScreen.tsx app/global.css
git commit -m "feat(fe): dramatic FighterRevealModal + cook-refund notice + roster-cap hint

A finished cook now auto-pops a dedicated 3D fighter reveal (serial-driven, no
tx); a full-roster refund self-frames as 'roster full — cook refunded' listing
what returned; the Pantry shows a proactive near-cap warning so a refund is
never a surprise. RevealSwitch routes reward vs fighter vs refund from one place.

Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>"
```

---

## Task 6: FE — the sandbox tutorial cook step raises the dramatic reveal

**Files:**
- Modify: `src/data/tutorial/fake-state.ts` (add a `cookReveal` builder), `src/data/tutorial/script.ts` (the `cook-wait` step), `src/data/tutorial/tutorial-store.ts` (the `completeActivity` cook branch).

**Interfaces:**
- Consumes: `RewardWon` (with `newFighters`); the existing sandbox `resolveCook` (which already mints an `Available` fighter at `s.nextFighterId`); the `RevealSwitch` wiring from Task 5 (`tut.pendingReveal` already routes through it).
- Produces: the FTUE cook step models the live auto-collect — cook resolves → dramatic `FighterRevealModal` → dismiss advances the script.

- [ ] **Step 1: Add a `cookReveal` builder to `fake-state.ts`.** Next to `expeditionReveal`/`tournamentReveal` (247–), add a builder that produces a `RewardWon` carrying the just-minted fighter id:

```ts
/** The dramatic reveal for a finished sandbox cook — carries the fighter id resolveCook just minted
 *  (its id is the pre-increment nextFighterId), so RevealSwitch routes it to the FighterRevealModal. */
export function cookReveal(fighterId: number): RewardWon {
  return { fungible: [], newRecipes: 0, fighterUpgrades: [], newFighters: [fighterId], cookRefunded: false, empty: false };
}
```
(Confirm `RewardWon` is already imported in this file — it is, for the other reveal builders.)

- [ ] **Step 2: Raise the reveal in `tutorial-store.ts`.** In `completeActivity`, change the `cook` branch (214–218) so it mints the fighter, raises the reveal, and does NOT auto-advance (the script now waits on the reveal, mirroring expedition/tournament):

```ts
    if (kind === 'cook') {
      const o = this.state.ongoings.find((x) => x.type === OngoingType.CookRecipe);
      const newId = this.state.nextFighterId; // resolveCook mints at this id, then increments
      if (o?.recipeid != null) resolveCook(this.state, o.recipeid);
      this.pendingReveal = cookReveal(newId);
      this.inject(); this.emit();
    } else if (kind === 'expedition') {
```
Add `cookReveal` to the `fake-state` import list (28–30).

- [ ] **Step 3: Make the `cook-wait` step advance on the reveal.** In `script.ts`, change the `cook-wait` step's `advanceOn` (line 88) from `{ kind: 'activity' }` to `{ kind: 'reveal' }`. (The `first-treat` step that follows is unchanged — after dismissing the dramatic reveal, it navigates to the roster and teaches Sweetening, reinforcing the new fighter.)

- [ ] **Step 4: Prune the now-unreachable `activity` advance path if it is fully dead.** Grep `advanceOn: { kind: 'activity' }` across `script.ts` — if `cook-wait` was its only user, also remove the `'activity'` handling in `completeActivity` (the `if (step?.advanceOn.kind === 'activity') this.advance();` is gone with Step 2's rewrite) and, if truly unused, the `{ kind: 'activity' }` variant from the `AdvanceOn` union in `script.ts` (24) and any `notify`/tick reference to it. If any other step still uses `activity`, leave the type in place. Do NOT remove `activityKind`/tick machinery that expeditions/tournaments still use.

- [ ] **Step 5: Manual reasoning check (no unit test — sandbox flow is integration-tested live).** Confirm by reading: cook move → `startCook` (ongoing appears) → ticks to 0 → `completeActivity` cook branch mints fighter + sets `pendingReveal` (newFighters) + emits → `RevealSwitch` shows `FighterRevealModal` for the sandbox fighter (it IS in `this.state.fighters` after `resolveCook`) → dismiss → `notifyRevealClosed` advances (`cook-wait.advanceOn.kind === 'reveal'`) → `first-treat`. No off-by-one on `nextFighterId` (captured before `resolveCook` increments it).

- [ ] **Step 6: Build + typecheck + full test run — expect GREEN.** `npm run build`, `npx tsc --noEmit`, `npx vitest run`.

- [ ] **Step 7: Commit.**

```bash
git add src/data/tutorial/fake-state.ts src/data/tutorial/script.ts src/data/tutorial/tutorial-store.ts
git commit -m "feat(fe): FTUE cook step raises the dramatic fighter reveal

The sandbox cook now models the live auto-collect: on resolve it mints the
fighter and raises the FighterRevealModal (via cookReveal → RevealSwitch);
dismissing it advances to the roster step. Mirrors the expedition/tournament
reveal-wait pattern.

Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>"
```

---

## Deferred (do WITH the user, pairs with Exchange Task 11)

- **Live fork e2e + eyes-on-pixels:** rebuild the fork GSP from `polygon-rewrite`, cook a fighter end-to-end on the fork, confirm it auto-lands `Available` + the `FighterRevealModal` pops with no `ca.cl` tx, force a full-roster refund and confirm the notice, and `npm run render:shots` the reveal for AAA polish. Needs the disruptive fork rebuild/restart — same reason Exchange Task 11 is deferred. Not run overnight.

## Self-Review notes (author)

- Spec coverage: every locked decision in `docs/cook-autocollect-spec.md` maps to a task — dramatic reveal (T5/T6), remove `Cooked` fully (T2/T4), keep refund + clearly notify (T1 serial + T5 notice + T5 proactive hint), bump serial on both outcomes (T1), reuse the reveal pipeline (T3).
- Determinism: only T1 touches the RNG-adjacent path; it adds no `rnd` draw and the golden diff is bounded to `{status 7→0, serial 0→1}` (T1 Step 7).
- Type consistency: `RewardWon.newFighters`/`cookRefunded` defined in T3 and consumed identically in T5 (`RevealSwitch`) and T6 (`cookReveal`); `InvSnapshot` new fields are internal to T3.

# Cook Auto-Collect — Design Spec

**Goal:** Remove the manual "collect a finished cook" transaction (`ca.cl`). When a recipe cook
finishes, the fighter auto-lands as `Available` at resolve, and the client shows a dramatic
"new fighter ready!" reveal — no second transaction. Mirrors the already-shipped reward-autocredit
pattern (`docs/reward-autocredit-spec.md`, commit `17cfdae`).

**Why:** On-chain txs cost gas + UX friction. The cook fighter is *already fully rolled at resolve*
(`ResolveCookingRecepie` → `fighters.CreateNew(..., rnd)` with all stats/quality); the `ca.cl` tx does
nothing but flip `Cooked → Available`. It carries no user intent and no new entropy — a pure tax, the
same shape reward-autocredit eliminated for the four reward-claim verbs. Cook-collect is the last one.

## Decisions (locked)

1. **Reveal UX = dedicated dramatic fighter reveal** — a bespoke component (animated portrait entrance,
   stat roll-up, rarity flare), not the plain reward modal. A cooked fighter is a big moment.
2. **`Cooked` status = remove fully** — delete the enum value + its now-dead expedition/tournament gate
   checks + all "collect" wording. Nothing produces `Cooked` after this change (strip-Taurion-baggage).
3. **Roster full at resolve = keep the existing refund, but the player MUST be told.** Full refund of
   candy + crystals + recipe, no fighter created, no new hold/drain machinery. **The refund is
   surfaced as an explicit "Roster full — your cook was refunded" reveal** (not a silent items-came-back
   diff) so the player always understands the outcome of a cook. Plus a proactive roster-cap indicator
   so a player near 48 knows pending cooks may refund. (This is why cook, unlike recipe rewards, needs no
   auto-drain pen: overflow is solved at resolve — but "solved" must be *visible*.)
4. **Bump `rewards_serial` on every cook resolve outcome** (technical default) — both the successful
   auto-collect AND the full-roster refund — so the FE's existing serial-driven auto-pop always fires
   when a cook finishes. Extends the serial's meaning from "rewards" to "notable auto-credited arrivals."
5. **Detection reuses the shared reveal pipeline** (technical default) — extend
   `snapshotInventory`/`rewardDelta` to diff the *set of fighter ids* (a new id ⇒ new-fighter payload),
   feeding the dramatic reveal. Reuses the reward-autocredit serial auto-pop; one detection path.

## Current flow (the tax)

| Step | Tx | Effect | Code |
|---|---|---|---|
| Start cook | `ca.r` | consume candy+crystals, clear recipe owner, create `COOK_RECIPE` ongoing @ absolute resolve height; roster-cap gate at parse | `moveprocessor_cooking.cpp:437` |
| Resolve | — (auto @ height) | **normal:** `fighters.CreateNew(owner, recipeID, roconfig, rnd)` (RNG roll) → `CalculatePrestige` → `SetStatus(Cooked)`; recipe row kept. **full roster:** refund candy+crystals+recipe, no fighter. Ongoing deleted. | `logic_resolve.cpp:684-748` |
| Cooked wait | — | fighter row exists, owned, counts to 48-cap + prestige, but status `Cooked` (=7) → **unusable** in expeditions/tournaments (they require `Available`) | `fighter.hpp:64-75` |
| Collect | `ca.cl` | **only** effect: flip `Cooked → Available`. No creation, no balance, no prestige, no recipe delete. | `moveprocessor_cooking.cpp:612-632` |

`Cooked` is a manual reveal/acknowledge gate, **not** an inventory holding pen — verified: the fighter
already occupies its slot and prestige the instant it's created; overflow is handled separately by the
resolve-time refund branch.

## GSP changes (`tfgsp-polygon`, branch `polygon-rewrite`)

1. **Resolve creates `Available`, bumps serial.** In `ResolveCookingRecepie` (`logic_resolve.cpp:740`):
   `SetStatus(FighterStatus::Cooked)` → `SetStatus(FighterStatus::Available)`, and bump the player's
   `rewards_serial` (mirror `logic_resolve.cpp:115`'s `bumpSerial`). **The serial bump is pure
   bookkeeping and must consume no `rnd`.** The `CreateNew(..., rnd)` draw is unchanged and stays at the
   exact same sequence point → RNG stream byte-identical.
2. **Overflow refund branch: keep the refund logic, add a `rewards_serial` bump** so the FE auto-pop
   fires and can show the "roster full — cook refunded" notice (`logic_resolve.cpp:694-732`). The bump
   is the only change; the refund itself is unchanged and consumes no `rnd`.
3. **Recipe row kept** (unchanged) — the created fighter references its recipe id.
4. **Delete the `ca.cl` verb + all plumbing** (fresh-genesis → delete outright, not no-op):
   - dispatch branch `moveprocessor.cpp:389-393` (`upd["cl"]`)
   - handler `MaybeCollectCookedRecepie` (`moveprocessor_cooking.cpp:612-632`)
   - parser `ParseCollectCookRecepie` (`moveprocessor_cooking.cpp:318-360`) + decl `moveprocessor.hpp:199`
   - pending mirror: `AddCookedRecepieCollectInstance` (`pending.cpp:116-121`, `pending.hpp:87`), the
     `cookedFightersToCollect` vector (`pending.hpp:191`) + JSON emit (`pending.cpp:386-394`) + parse
     call (`pending.cpp:595-597`) — removed as a unit.
5. **Remove the `Cooked` enum value** (`database/fighter.hpp:64-75`) and every now-dead consumer that
   checks `== Cooked` (the expedition/tournament activity gates that special-cased it — audit
   `moveprocessor_activity.cpp` and any status-label mapping). Do NOT renumber the other enum values.
6. **Regenerate goldens** (deliberately, after diff review). State legitimately changes: the cooked
   fighter is now `Available` not `Cooked`, and `rewards_serial` +1 per cook. Unlike the exchange work,
   this IS an observable-state change (`FullState` serializes fighter status + rewardsserial), so the
   golden WILL differ — confirm the diff is exactly {status 7→available, serial bumps}, nothing else.

## FE changes (`tf-frontend`, branch `master`)

**Remove:**
- "Collect fighter" button + `onCollect`/`canCollect` (`FighterDetail.tsx:44,55-57,120-122`), the
  `collectCook` import (`:15`), and the dead `collectCook` builder (`move-builder.ts:32`).
- The `Cooked` "ready to collect" UI: roster scrim (`FightersScreen.tsx:35`), `rosterStatusLabel`
  COOKED case (`fighter-display.ts`), the `'Cooked (ready to collect)'` label (`types.ts:121`), and the
  `Cooked` enum value if the FE mirrors it. Strip stale "collect" copy (`CookingScreen.tsx:6`,
  `FightersScreen.tsx:8`).

**Add — new-fighter detection + dramatic reveal:**
- Extend `snapshotInventory`/`rewardDelta` (`reward-reveal.ts`) to diff the set of fighter ids; a newly
  appeared id ⇒ a new-fighter reveal payload (portrait + rolled stats). Today the diff explicitly skips
  new fighters (`reward-reveal.ts:5-6,26-36`) — this is the one real gap.
- A dedicated **`FighterRevealModal`** (dramatic): animated portrait entrance, stat roll-up (0→value),
  rarity flare scaled to quality, and a "View in roster" deep-link. Triggered by the existing
  serial-driven auto-pop (`page.tsx:202,209-215`) when the delta carries a new-fighter payload.
- **Refund case (player MUST know):** on a full-roster refund the GSP **bumps `rewards_serial` too**, so
  the FE auto-pop fires. The FE distinguishes the two outcomes of a resolving cook by whether a new
  fighter id appeared: new fighter ⇒ dramatic `FighterRevealModal`; NO new fighter but a `COOK_RECIPE`
  ongoing vanished + candy/crystals/recipe returned ⇒ an explicit **"Roster full — cook refunded"**
  notice listing what came back. Never a silent items-came-back diff. (Detecting "an ongoing vanished"
  needs the reveal snapshot to also track the `CookRecipe` ongoing set — a small extension of
  `snapshotInventory`.)
- **Proactive roster-cap indicator:** when the roster is at/near 48, show a clear "roster full — pending
  cooks will refund on finish" hint on the cooking/roster screens, so the refund is never a surprise.
- **Tutorial:** the sandbox FTUE models cook resolving without a reveal (`tutorial-store.ts:208-209`);
  update the cook step to expect the reveal now.

## Consensus / determinism

- **RNG invariant:** do not add/remove/reorder any `rnd` draw. `CreateNew(..., rnd)` stays put; the
  status value and serial bump consume no entropy. Verify with the golden replay.
- **`TickAndResolveOngoings` ordering** (`logic_resolve.cpp:756-773`, `ORDER BY id`) unchanged.
- **Golden regen is expected here** (observable-state change) — review the diff, confirm it's only
  status `7→Available` on cooked fighters + `rewardsserial` increments, no other field drift.
- Fresh-genesis basis: no in-flight `ca.cl` moves to honor, DB wiped at launch — deleting the verb is
  safe (same basis reward-autocredit used).

## Testing

- **GSP:** a cook resolve test asserting the new fighter lands `Available` (not `Cooked`) and
  `rewards_serial` incremented; a full-roster refund test asserting candy+crystals+recipe returned, no
  fighter, AND `rewards_serial` incremented (so the FE can notify); a test that the `ca.cl` move is now
  unknown/rejected (or its handler gone). Full `make check` + deliberate golden regen.
- **FE (vitest, pure-first):** `rewardDelta` new-fighter detection (a fighter id appearing ⇒ payload;
  no false positive on status-only changes); the reveal trigger logic. The modal itself is a thin
  component (live-verified via `render:shots`, consistent with the repo's pure-function-first style).

## Out of scope

- Sweetener collect (`ca.sc`) — already removed by reward-autocredit.
- Any change to cook *start* (`ca.r`), cook cost/duration, or the RNG roll itself.
- The recipe-reward hold/auto-drain (unchanged; cook needs none).

## Implementation order (GSP-first; FE depends on the serial bump for detection)

1. GSP: resolve → `Available` + serial bump (+ test).
2. GSP: delete `ca.cl` verb + parser + pending plumbing (+ test move is gone).
3. GSP: remove `Cooked` enum + dead consumers.
4. GSP: full `make check` + deliberate golden regen (diff review).
5. FE: `rewardDelta` new-fighter detection (+ pure test).
6. FE: `FighterRevealModal` + wire to serial auto-pop; remove collect button/wording; tutorial update.
7. FE: build + `npm test` + eyes-on-pixels (`render:shots`).
8. Live fork e2e (with the Exchange Task 11) — cook a fighter, confirm it auto-lands + reveals, no tx.

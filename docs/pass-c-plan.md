# Pass C — remaining consensus work (verified scoping plans)

> Built 2026-06-28 from a parallel deep-read workflow (`wxtelgz0u`, 5 Explore agents) +
> first-hand verification. Source of truth for the remaining golden-regen / architectural items
> after A2b, C9b, SB-06 (all golden-safe, committed: `ffd6f1e`, `a18a0eb`, SB-06, `0851510` docs).
> Companion to `docs/security-audit.md` (findings) and `docs/p-e1-loadbench.md` (Stage 1/2 plan).

## STATUS (updated 2026-06-28)

- **F2/F3 — DONE.** All three raw-double consensus sites fixed + verified:
  - Site B sweetness `/100.0`→`/100` (`e425598`, golden byte-identical).
  - Site A prestige percentage → fixed-point, dropped the 4 double wrappers (`b97182e`,
    golden byte-identical — test rosters never hit a ratio where round-from-double ≠ fixed div).
  - Site C crystal cost → proto `CHICost`(float)→`CHICostSats`(int64), pure-int64 gate (`c8dcc98`,
    golden byte-identical; blob verified to carry the six int64 sat varints, old float32 patterns gone).
  - Swept the rest of `src/`+`database/`: remaining FP is `fpm` fixed-point (deterministic),
    gamestatejson display-only, and `std::round` over int64 quotients (deterministic). Clean.
- **A2c — DONE (newly discovered).** Stripped the remaining Taurion *height*-fork gates that A2b
  (isFork2 blocks) didn't cover. 6 golden-safe strips + 3 orphaned `chain` decls (`36bf135`) and
  R1 the special-tournament tier de-gate/unify on `fpm::round` (`7f44da5`). All golden byte-identical
  + 98/98. Detail: each gate was dead on both REGTEST(h~46) and Polygon(genesis 89.2M), OR always-true,
  OR (RNG reseed) a magic height replaced by the equivalent `Chain()!=REGTEST`. **Open follow-ups:**
  (a) ~9 remaining `chain==/!=REGTEST` gates are REGTEST test-harness accommodations (payout splits,
  fixed dev addresses) — a separate, delicate pass, NOT touched. (b) the two now-identical
  `RecalculatePlayerTiers` bodies could be DRYed into one free function. (c) `xaya_player.proto`/protos
  may still hold magic Taurion heights elsewhere.
- **H5 — DONE** (`c48e46d`). Per-player cap on UNCLAIMED rewards: new RoConfig param
  `Params.max_unclaimed_reward_amount` (field 35) = 100, enforced at the passive-reward creation site
  (`GenerateActivityReward`); at the cap the reward + its generated recipe are not created (reject-new).
  Deconstruction rewards exempt (1:1 with the 48-fighter cap; rejecting would strand a fighter). Golden
  byte-identical + new `UnclaimedRewardCapRejectsPassiveRewards` test (99/99).
- **H4 — DONE** (`8365ed9`). Source-level recipe deletes (NOT the rejected GC-sweep — refs live in proto
  BLOBs so a column-based GC would chain-halt at the no-null-check claim derefs). Delete each recipe where
  its 1:1 reference dies: deconstruct-claim (delete fighter's source recipe), cook-onto-existing-fighter
  (delete replaced fighter's recipe), destroy-recipe (DeleteById not SetOwner('')), discarded reward
  (delete its owner='' generated recipe). Golden byte-identical + RecepieDestroyTest/DeconstructionTest
  now assert deletion (99/99). Mapping + adversarial GC/cap verdicts: workflow `wh3a8qjdi`.
  **KEY by-product finding: the `ongoing_operations` TABLE IS DEAD** (no live writer; ongoings live in the
  player proto BLOB) — directly relevant to H3 below (H3 is what makes that table live).
- **H3 / Stage 2b — DONE** (`aa60798`). Event-driven ongoings: migrated out of the player proto BLOB into the
  height-keyed `ongoing_operations` table (was dead since Stage 2a). New resolver = `QueryForHeight(now)`
  (WHERE height<=now ORDER BY id), materialize-then-resolve-then-DeleteById. 4 add-sites + 4 guards + JSON +
  GetOngoingsSize()->CountForOwner + proto field 7 reserved. Unit harness now bumps height per block (required:
  resolution is height-based; timing preserved — old & new both resolve at the D-th block after create).
  Golden REGEN (deterministic 2x, integrity-verified). 99/99 + golden + loadbench 8/8. Full plan +
  fork analysis: `docs/h3-cutover-plan.md`. Reorg test = scoped follow-up there (undo safety structurally
  guaranteed by the table-agnostic whole-DB session changeset).
- **F1 — pending read-only — DONE.** Exhaustive 46-agent audit of all 24 pending-reachable `Parse*`/
  `ExtractMoveBasics` methods (adversarial refute-pass on every "clean" verdict) found exactly **3** confirmed-DB
  write sites, all write-only side-effects on `return false` paths: `ExtractMoveBasics:178`
  (`set_address` on a crown-holder with a missing address) and `ParseTournamentEntryData:2542,2573`
  (`globalData.SetQueueData`, demand-queue fill on a full roster). Fix: added `const bool readOnly` to
  `BaseMoveProcessor` (`MoveProcessor` passes `false`, `PendingStateUpdater` passes `true`) and inline-guarded
  each of the 3 writes `if(!readOnly) ...`. **GOLDEN-SAFE by construction** (confirmed path always takes the
  branch → byte-identical golden, no regen). New regression test `PendingTournamentEntryDoesNotWriteQueueData`
  proven non-vacuous (fails with the guard removed). 100/100 unit + golden byte-identical.
- **Pass C COMPLETE.** Optional tail remaining: the H3 reorg test, OVF-01 candy clamp, the deferred REGTEST
  chain-gate hygiene pass, and DRY-ing the two identical `RecalculatePlayerTiers` bodies.

## Golden-regen workflow (verified)

`goldenreplay_tests.cpp` regen mode (lines 38-39, 354-373):
1. Make change → build in `tfdev` container (`docker cp` → `touch` → `make`).
2. `cd /usr/src/tfgsp/src && GOLDEN_REGEN=1 ./goldenreplay_tests` → writes fresh `goldenreplay.golden.json` in cwd, SUCCEEDs.
3. `docker cp tfdev:/usr/src/tfgsp/src/goldenreplay.golden.json src/goldenreplay.golden.json` (back to host worktree).
4. Re-run `./goldenreplay_tests` (no env) → must PASS against new golden.
5. **Run regen twice, diff the two outputs → must be byte-identical** (proves determinism; a non-deterministic change would differ between runs).
6. **Independently sanity-check the golden diff** (regen only proves golden==itself; you must confirm the NEW behavior is correct — the 98 unit tests assert specific outcomes and must still pass).
7. Commit the new `goldenreplay.golden.json` with the code.

## Chosen order (rationale)

1. **F2/F3 determinism** — lock fixed-point math before structural regens.
2. **H4/H5 row GC** — DB-bloat root cause (needs a cap-policy decision — ask user).
3. **H3 Stage 2b event-driven** — capstone; its migration updates H4/H5's ongoing-reference predicate.
4. **F1 pending read-only** — lowest; on Polygon the GSP runs `--pending_moves=false`, so `PendingStateUpdater::ProcessMove` is likely never invoked (hygiene fix, not launch-blocking). Scope first; the scoping agent failed on it.

---

## F2/F3 — kill raw double/float in consensus (sites verified first-hand)

**Site A — prestige percentage (REGEN, the risky one).** `database/xayaplayer.cpp:247-258`
`GetFighterPercentageFromQuality` returns `double` (`totalFighters / fighters.size()`); call sites
`xayaplayer.cpp:274-277` wrap it in `fpm::fixed_24_8(...)`. Fix: return `fpm::fixed_24_8`, compute
`fpm::fixed_24_8(count) / fpm::fixed_24_8(fighters.size())`, drop the wrapper at the 4 call sites.
Risk HIGH: prestige feeds `specialtournamentprestigetier` → tournament participant sets → cascade.
double→fpm vs direct fpm division can differ in the last 1/256 bit at boundaries. **REGEN.**

**Site B — sweetness (GOLDEN-SAFE).** `database/fighter.cpp:371`
`(pxd::Sweetness)(((GetProto().rating() - 1000) / 100.0) + 1)`. In this branch `rating ∈ [1000,2000)`
(guarded by the `>=2000` early-return and `rating >= srate >= 1000`), so `(int)(x/100.0)+1 == x/100+1`
for x≥0 (verified: 100.0 divides exact multiples exactly, truncation matches integer floor otherwise).
Fix: `/ 100.0)` → `/ 100)`. **Should stay golden byte-identical — do as a separate golden-safe commit
to confirm the analysis; if golden shifts, it's actually regen.**

**Site C — crystal bundle cost (REGEN, consensus payment gate).** `src/moveprocessor.cpp:560-585`
`float chiPRICE = vals.chicost();` (proto `crystal_bundle.proto:33` `optional float CHICost`), then
`cost = chiPRICE * COIN * (multiplier / 1000);` with `COIN=1e8`. A 24-bit float mantissa cannot hold 1e8
exactly → precision loss; `cost` (int64) is the gate at `if (paidToDev < cost)`. `multiplier/1000` is
already int64 integer division. Fix (Option B, no proto migration): `int64_t chiPRICE_sats =
(int64_t)(vals.chicost() * COIN);` once, then `cost = chiPRICE_sats * (multiplier / 1000);`. (Option A:
change proto to `int64 CHICostSats` — cleaner but proto migration + regtest config values.) **REGEN.**

CI hardening (optional): ban `float`/`double`/`std::pow`/`std::log10` in `src/`+`database/`; build with
`-ffp-contract=off`. est ~85 LOC. Open Q: prestige stays int64 cast at xayaplayer.cpp:363 (keep).

---

## H4/H5 — row GC (DB-bloat root cause). REGEN, ~350 LOC, risk MEDIUM.

**H4 orphan recepies (owner='').** Created as rewards `logic.cpp:168` (CraftedRecipe) / `179`
(GeneratedRecipe) owner=''; set to '' via `SetOwner('')` at `moveprocessor.cpp:4865` (destroy) / `4928`
(cook); only deleted at `moveprocessor.cpp:3726` (DestroyUsedElements, transfigure fuel). Per-owner cap
`moveprocessor.cpp:3405-3406` bypassed (`recipe.cpp:544-549` `CountForOwner` ignores owner=''). →
unbounded growth.

**H5 uncapped rewards.** Created `logic.cpp:140-184` no cap; deleted only on claim
(`moveprocessor.cpp:3536/3543`) + deconstruction (`3666`). The per-eid "claim previous first" check is an
O(rewards-for-owner) scan → hoarder makes own moves heavier.

**Plan.** GC pass (end of UpdateState) deleting owner='' recipes NOT referenced by any (a) `fighter.recipeid`,
(b) ongoing `recipeid`, (c) `reward.generatedrecipeid`. **Riskiest thing:** the GC predicate MUST be exact —
deleting a recipe a live fighter/ongoing/reward points at = move-reachable null-deref chain-halt. Cap unclaimed
rewards per player at creation time. **NOTE the H3 interdependency:** the "referenced by an ongoing" check
depends on whether ongoings live in the player proto (today) or the OngoingsTable (after H3) — if H4/H5 lands
before H3, its predicate updates as part of H3.

**Genuine decision for the user (ask when reached):** cap values (e.g. GLOBAL_RECIPE_CAP, MAX_UNCLAIMED_
REWARDS_PER_PLAYER) and overflow policy on reward cap — (A) reject the new reward, (B) delete oldest unclaimed,
or (C) silently skip. Also SB-06 already done (`4979-4983` removed).

Golden: deleting currently-persisted (but JSON-invisible) orphan rows changes the state row count/hash → REGEN.

---

## H3 / Stage 2b — event-driven GSP (the hard requirement). REGEN, ~450 LOC, risk MEDIUM.

**Today:** `logic.cpp` UpdateState (~2087-2129) runs 6 unconditional full-table scans every block
(TickAndResolveOngoings, CheckFightersForSale, SetFreeTransfiguringFighters, ProcessTournaments,
ReopenMissingTournaments, ProcessSpecialTournaments) → O(players + 3·fighters + tournaments)/block.
Ongoings live in the player proto (`xaya_player.proto:50 repeated ongoings = 7`) with a `blocksleft`
countdown; `TickAndResolveOngoings` (`logic.cpp:660-743`) scans all players.

**Infra ready (Stage 2a):** `OngoingsTable` (`database/ongoings.{hpp,cpp}`), `ongoing_operations` table
(`schema.sql:149-174`, indexed on height + owner), `OngoingOperation` proto has height/owner/type/refs.
**VERIFY the exact OngoingsTable API before coding** (CreateNew signature, QueryForHeight, QueryForOwner,
SetHeight, GetFromResult — these names came from a haiku agent and MUST be checked).

**Cutover plan:**
1. `TickAndResolveOngoings` → `QueryForHeight(ctx.Height())` (WHERE height<=h ORDER BY id), dispatch by
   `type` to the existing Resolve* fns, `DeleteById` after. Drop the blocksleft countdown.
2. 4 add-sites — `MaybeDeconstructFighter`, `MaybeStartExpedition` (per-fighter), `MaybeApplySweetener`,
   `MaybeSubmitCookRecepie` (moveprocessor.cpp ~4332/4707/4792/4875 area) — replace `add_ongoings()`+
   `set_blocksleft` with `OngoingsTable.CreateNew` + `SetHeight(height+duration)` + owner + type + refs.
3. 4 dup-check sites (moveprocessor.cpp ~1503/1864/1925/2828) — replace `a.GetProto().ongoings()` loop
   with `QueryForOwner` + type filter.
4. gamestatejson.cpp (~191-199, 424-469, 672/680) — derive `blocksleft = op.height - ctx.Height()`; build
   one owner→ongoings map (avoid N queries).
5. pending.cpp (~171/200/246/476-503) — pending is non-consensus preview; keep blocksleft for now.
6. proto: `xaya_player.proto` drop `repeated ongoings = 7` → `reserved 7;`. Keep `ongoing.proto BlocksLeft`.
7. Remove `xayaplayer.hpp:230 GetOngoingsSize` after cutover.
8. **Add a reorg test** (apply N → ProcessBackwards → re-apply → byte-identical).

**Riskiest thing (FORK):** any non-deterministic resolution order. `QueryForHeight`/`QueryForOwner` MUST
`ORDER BY id`; if resolution order differs across nodes, RNG draws diverge → different rewards → permanent
fork. Verify ordering is preserved everywhere.

Golden: player proto shrinks (ongoings move out) + resolution timing changes → REGEN (final state should be
equivalent; verify recipe/fighter/reward outcomes match by reasoning the diff).

---

## F1 — pending read-only — DONE. GOLDEN-SAFE (byte-identical, no regen).

`PendingStateUpdater::ProcessMove` (`src/pending.cpp`) shares `BaseMoveProcessor`'s `Parse*`/`ExtractMoveBasics`
methods with the confirmed `MoveProcessor`. A few of those "parse" methods embed genuine consensus side-effects
that write the confirmed DB — and on the pending path those `UPDATE`s land outside any block transaction
(pending processing is never rolled back), i.e. latent confirmed-state corruption. Dormant on Polygon only
because the GSP runs `--pending_moves=false` (XayaX has no pending feed).

**Exhaustive audit (workflow `wj31wkggy`, 46 agents):** all 24 pending-reachable methods read independently,
with an adversarial refute-pass on every method first reported clean. Grep-by-idiom had missed a site twice, so
this per-method read was necessary. Exactly **3** write sites, each a write-only side-effect immediately
followed by `return false`:
- `ExtractMoveBasics:178` — `player->MutableProto().set_address(...)` (flushed at `player.reset()`,
  `UPDATE xayaplayers`); fires only when a special-tournament crown-holder has an empty address.
- `ParseTournamentEntryData:2542` & `:2573` — `globalData.SetQueueData(...)` (`UPDATE globaldata.queuedata`);
  fires only when a tournament roster is full (demand-queue fill for next block).

**Fix:** `const bool readOnly` member on `BaseMoveProcessor` (ctor 3rd param; `MoveProcessor` → `false`,
`PendingStateUpdater` → `true`), with each of the 3 writes inline-guarded `if(!readOnly) ...`. Confirmed path
always takes the branch ⇒ state transitions byte-identical ⇒ golden unchanged (no regen). All 22 other methods
verified strictly read-only. Regression test `pending_tests.cpp::PendingTournamentEntryDoesNotWriteQueueData`
forces a full roster and asserts `GlobalData::GetQueueData()` stays `""` across a pending entry; proven
non-vacuous (fails when the guard is stripped). 100/100 unit + golden byte-identical.

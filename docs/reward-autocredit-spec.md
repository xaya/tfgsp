# Reward auto-credit — remove the claim tx (spec / build plan)

Status: **DRAFT for review** · Author: session 2026-07-04 · Scope: `tfgsp-polygon` (consensus) + `tf-frontend`

## 1. Goal (from the user)

Today, getting an activity reward costs the player an **on-chain claim tx** (`exp.c` / `tm.c` /
`ca.sc` / `f.c`). That tx is pure bookkeeping — the reward is already rolled and stored at resolve; the
claim only moves it into inventory. Goals:

1. **No tx to receive rewards.** Rewards land automatically when the activity resolves, even if the
   player never "collects" and even while offline.
2. **Keep the reward buzz.** The "You won!" popup + "TREAT FIGHTER!" shout stays — but as a **client-side
   reveal**, not a tx. A **"Claim all"** button still exists; it only *reveals* what you already got.
3. **Must not break at a limit.** The current caps (unclaimed-reward backpressure, recipe-slot cap) must
   degrade gracefully — no lost rewards, no unbounded state growth (the DB-bloat the rewrite fixed).

## 2. TL;DR — recommended design

- **GSP:** move the crediting logic from the claim handlers into the four `Resolve*` paths, so
  candy/recipe/move/armor/animation land in inventory/fighter state **the moment the activity resolves**
  (still event-driven — no new per-block work). The `rewards` table becomes an **overflow-only holding
  area** (see §5.3), not the normal path. Claim move handlers become **no-ops** (kept for one version).
- **Caps:** candy + move/armor/animation are unbounded-safe → credit immediately. **Recipe** is the only
  capped reward → credit if a slot is free, otherwise **hold as a bounded pending row and auto-drain**
  when a slot frees (event-driven). Nothing is ever silently lost.
- **Reveal:** stays entirely **client-side**. Reuse the existing `snapshotInventory`/`rewardDelta`
  inventory-diff (it already powers the popup). The client keeps a **per-player persisted baseline**
  (last-seen height + inventory snapshot) and a small **unrevealed buffer**; "Claim all" flushes the
  buffer into the existing `RewardRevealModal`. A tiny optional GSP counter makes offline detection
  bullet-proof (§6.2).
- **Correction from research:** the sweetener carve-out is **not needed**. The candy-set *choice* is made
  at **cook** time (`ca.s rid`), stored on the ongoing op, and rolled deterministically at resolve;
  `ca.sc` is a choice-less collect. All four sources auto-credit uniformly.

## 3. How it works today (evidence-backed)

Per-block driver order (`src/logic.cpp:102-145 UpdateState`): player moves (incl. claims) →
`TickAndResolveOngoings` → `ProcessTournaments`. So a reward rolled at resolve is only claimable next block.

**Resolution is event-driven** and does ~zero idle work:
- Expedition / deconstruction / sweetener-cook / recipe-cook resolve via the height-keyed
  `ongoing_operations` table — `TickAndResolveOngoings` runs one indexed `WHERE height<=now` query and
  returns early when nothing is due (`src/logic_resolve.cpp:618,635,643`).
- Tournaments resolve via a bounded per-block scan of **active** instances only
  (`src/logic_tournament.cpp:55,60,139,142`), `blocksleft==0` → resolve.

**Rolling vs crediting are split:**
- At resolve, `GenerateActivityReward` (`logic_resolve.cpp:49-155`) is the single reward-row creation
  site for expedition/tournament/sweetener. It **rolls** (consumes the per-block `xaya::Random`) and
  writes a `rewards` row tagged with source ids (`expeditionid`/`tournamentid`/`fighterid`/`sweetenerid`/
  `rewardid`/`positionintable`). For recipe rewards it also creates the recipe row with **owner=""** and
  stores `generatedrecipeid`. **No inventory/fighter mutation happens here.**
- At claim, `ClaimRewardsInnerLogic` (`src/moveprocessor_activity.cpp:660-849`) does the actual crediting
  per `RewardType` (Candy `0` → `AddFungibleCount`; GeneratedRecipe/CraftedRecipe → `SetOwner`;
  Move `3` → `add_moves`; Armor `4` → armorpieces; Animation `5` → `set_animationid`), then
  `rewards.DeleteById`. Discards (unsolved / stale index / recipe-cap-full / dangling fighter) go to
  `markedToRemove` with H4 orphan-recipe cleanup.
- **Deconstruction is separate** (`ResolveDeconstruction` `logic_resolve.cpp:386-450` +
  `MaybeClaimDeconstructionReward` `src/moveprocessor_fighter.cpp:410-477`): resolve rolls the recovered
  candy into the row; claim credits candy (**skipped if `isaccountbound`**), deletes the reward rows,
  **deletes the fighter + its recipe**, and recomputes prestige.

**Caps (roconfig values authoritative):**
- `max_unclaimed_reward_amount = 100` (`params.pb.text:41`) — enforced at **resolve** in
  `GenerateActivityReward` (`logic_resolve.cpp:112`): at/over the cap the reward row is **not created
  (silently dropped)**. This is the anti-bloat backpressure bounding the `rewards` table. **Deconstruction
  bypasses it** (`logic_resolve.cpp:433`, no `CountForOwner`).
- `max_recipe_inventory_amount = 48` (`params.pb.text:37`) — enforced only at **claim**
  (`moveprocessor_activity.cpp:721`): recipe reward at full slots is **discarded** (row + owner=""
  recipe deleted). Note owner="" recipe rows don't count toward this cap (`recipe.cpp CountForOwner`
  keys on exact owner).

**RNG order is consensus-critical** (`logic.cpp:106-117` seed from block hash; ordered materialise-then-
resolve `logic_resolve.cpp:622-628`). Any change must **not add or reorder `rnd` draws**.

**The reveal reads reward rows today only indirectly** — rows carry *no contents*
(`tf-frontend/src/data/types.ts:126-137`); the popup is already computed by an **inventory diff**
(`reward-reveal.ts:26-71`, `RewardRevealModal.tsx`). The modal is already client-side and fires the
`treatfighter` shout on mount.

## 4. Frontend gates that currently depend on unclaimed reward rows

These break/soften under auto-credit and must be handled (§7):
- **StartExpedition backpressure** (`moveprocessor_activity.cpp:610-624`): blocks re-running an expedition
  while its reward is unclaimed. Becomes moot (nothing unclaimed). *Acceptable — it only existed because
  rewards piled up.*
- **Sweetener re-cook gate**: GSP blocks a new sweetener cook while the fighter has an unclaimed sweetener
  reward (`sweeteners.ts:559-563`, `SweetenerScreen.tsx:56-61`). Moot under auto-credit — remove the gate.
- **`f.c` over-deletion footgun**: deconstruction claim deletes **all** of a fighter's reward rows
  (no source scoping), so the client sequences sweetener-before-deconstruct (`rewards.ts:28-32,57-61`).
  Moot under auto-credit; but the underlying "one fighter resolves a sweetener AND a deconstruction the
  same block" case must be handled server-side (it is — separate resolve paths, §5).
- **Unclaimed-reward cap warnings** (`sweeteners.ts:531-537`, `SweetenerScreen.tsx`): the "near the 100
  cap, rolls forfeited" warning goes away for candy/upgrades; only recipe-overflow (§5.3) remains.

## 5. Proposed GSP design

### 5.1 Core change — credit at resolve
Factor the per-type crediting out of `ClaimRewardsInnerLogic` into a shared helper
`CreditActivityReward(player, fighter?, rewardTableEntry, ctx, db)` and call it **inside**
`GenerateActivityReward` at the point the row is created today — i.e. credit instead of (or in addition to,
see §5.3) writing a persisted row. Because `GenerateActivityReward` already runs at resolve inside the
event-driven tick / tournament scan, **no new per-block work is introduced**.

Critical invariants:
- **Do not add or reorder any `rnd` draw.** Crediting is pure state mutation after the existing draw;
  insert it at the same sequence point the row `CreateNew` happens today. (Golden-replay will change
  because *state* changes, but the RNG stream must be identical — regen goldens deliberately.)
- **Keep HALT-01 null-guards.** Tournament rewards pass `fighterID=0`; move/armor/animation with no valid
  fighter are dropped exactly as today. Expedition/sweetener target the just-resolved fighter (exists at
  resolve), which *closes* most of the dangling-fighter window but the guard stays.
- **Freeze the armor `rewardsource` mislabel** (hard-coded `Expedition`, `activity.cpp:811,820`) OR fix it
  now — either way it's a deliberate, golden-affecting decision (§6.4).

Per source at resolve:
- **Expedition** (`ResolveExpedition`): already sets fighter Available, then rolls → credit each roll.
- **Tournament** (`ProcessTournaments` resolve): already writes results/ratings/prestige/state=Completed;
  add crediting to the per-participant reward roll (`fighterID=0` → move/armor/anim dropped as today;
  confirm tournament tables only roll candy/recipe).
- **Sweetener** (`ResolveSweetener`): already sets tier + Available; credit the rolled candy/move/armor.
  Preserve the "already-applied → no reward" early return (`logic_resolve.cpp:178-184`).
- **Deconstruction** (`ResolveDeconstruction`): credit candy **unless `isaccountbound`**, then perform the
  fighter+recipe deletion and `CalculatePrestige` that currently live in the claim
  (`fighter.cpp:447-473`). Net UX win: the roster slot frees at resolve instead of at claim.

### 5.2 Retire the claim moves
**No backward-compat needed — fresh genesis (see §5.4).** Just **delete** the `exp.c` / `tm.c` / `ca.sc` /
`f.c` claim handlers and their parsers outright; the frontend stops emitting them (§7). No no-op
transition, no in-flight-tx concern (nobody is playing / DB is wiped at launch).

### 5.3 Caps — "doesn't break at the limit"
- **Candy / Move / Armor / Animation:** unbounded-safe (fungible counter with only a per-quantity supply
  ceiling; fighter-proto edits are bounded). **Credit unconditionally at resolve.** The
  `max_unclaimed_reward_amount` backpressure is no longer needed for these (they never sat as rows).
- **Recipe (the only capped reward):** re-check `max_recipe_inventory_amount` **at resolve**:
  - slot free → `SetOwner(player)` on the already-created recipe row (credit immediately). ✅
  - **full → HOLD**: keep the `rewards` row (owner="" recipe attached) as a **bounded pending overflow**,
    capped by `max_unclaimed_reward_amount` (which now *only* ever holds recipe-overflow → naturally
    tiny). **Auto-drain**: when the player frees a recipe slot, drain pending recipe rewards oldest-first
    into ownership. No tx, no loss, still bounded. ✅
  - **DECIDED (D1): hold + auto-drain + a near-full warning.** Confirmed this does **not** clog the GSP:
    (a) holding is *strictly less* state than today — the `rewards` table currently holds *every*
    unclaimed reward for every player; under auto-credit it holds only the rare recipe-overflow, and is
    otherwise empty. (b) The drain is **event-driven, never a per-block scan**: it fires only when *that*
    player frees a recipe slot (recipe cook/destroy/deconstruct resolve — moments the GSP is already
    doing that player's work), as a single owner-scoped indexed lookup (O(1)). Zero idle-block cost →
    honours [[event-driven-gsp]].
  - **Near-full warning:** surface a "recipe inventory almost full (X/48) — free a slot so rewards land
    immediately" nudge in the pantry/rewards UI (mirrors the existing sweetener near-cap warning). New
    frontend copy only; no GSP change.
- **Result:** the only surviving use of the `rewards` table is the rare recipe-overflow queue. Everything
  else credits straight through.

### 5.4 Consensus / rollout safety
- **Fresh genesis, not a fork-height cutover.** Nobody is playing, so the DB is **wiped and restarted from
  a new genesis** at launch. This removes all backward-compat: no no-op claim transition, no migration, no
  fork gate. Folds into the already-pending pre-mainnet **genesis re-pin** (see launch-punchlist).
- Still requires **golden-replay regen** + full test pass (goldens are the determinism tripwire, not a
  migration) — crediting mutates state earlier, so goldens change; regen deliberately.
- Preserve all proto shapes and the `recepie/recepies` spellings (do not rename).
- Deterministic + idempotent: crediting happens inside `UpdateState` in fixed id order; **no new/reordered
  `rnd` draw**; reorg replay re-runs identically.

## 6. Reveal design (client-side, mostly no GSP change)

### 6.1 Reuse what exists
`snapshotInventory` + `rewardDelta` already compute a `RewardWon` purely from an inventory before/after,
and `RewardRevealModal` is already presentational + fires the shout. Reuse verbatim.

### 6.2 Detecting new rewards without on-chain rows
The client sees **every block's** `USER` slice (event-driven poller, height-stamped). Design:
- Persist a **per-player baseline** in `localStorage` (`tf-reward-seen-<player>`: `{serial, height,
  invSnapshot}`), mirroring the existing `AudioController` try/catch pattern. Per-player keyed.
- **GSP signal (D2, decided):** append-only `uint64 rewards_serial` on the player, +1 per credited reward.
  It is the source of truth for "how many new rewards since I last looked" — immune to spend masking.
- **FIRST RUN / fresh browser / new device (no baseline) — CRITICAL, must not spam:** when there is no
  stored baseline for this player, **silently seed** `baseline = {serial: current, height: current,
  invSnapshot: current}` and **reveal NOTHING**. The player's rewards are already in their inventory/roster
  (auto-credited on-chain) — they see them normally; we never reconstruct or replay historical rewards.
  This is what prevents "a million claim buttons / popups" on a returning fresh browser.
- **Live path (precise):** with a baseline present, on each new slice do a single-block diff vs the
  previous slice; accumulate positive reward-attributable deltas (candy up, new owned recipe, fighter
  move/armor/anim added) into the **unrevealed buffer**. Advance the stored serial/snapshot.
- **Return-after-offline (baseline present):** if `rewards_serial` jumped while away, show **at most ONE
  aggregated** "while you were away you earned…" reveal (never one-per-reward). Best-effort contents via
  the snapshot diff; the serial guarantees the button appears reliably.
- **Hard rule: the reveal is bounded to a single aggregated popup — never a backlog, never per-reward
  buttons.** There is nothing to "claim"; rewards are already owned.

### 6.3 Reveal UX — "Rewards", never "Claim", never per-reward buttons
"Claim" implies an action/tx; nothing is claimed anymore. **There are NO claim buttons anywhere** — not
per reward, not "claim all". The existing HUD **rewards bell** *is* the surface:
- Rename any "Claim"/"Claimable" copy to **"Rewards"**. The bell shows a **count badge** when
  `rewards_serial` has advanced past the local baseline (i.e. unrevealed rewards exist).
- Tapping the bell opens the reveal: **one aggregated** `RewardRevealModal` ("You won …"), plays the
  "TREAT FIGHTER!" shout, then advances the baseline (badge clears). Modal button stays **"Collect"** (it
  just closes the popup — no tx).
- The old **Rewards overlay** (previously a list of Claim buttons) becomes a passive **recent-rewards
  view** (what you earned lately) or is folded into the reveal — no actionable buttons.
- While actively playing, a live single-reward win can pop the reveal immediately (nice moment); it's the
  same one-popup path, never stacked.
- Unify reveal coverage (today it's uneven: Rewards reveals all, Expeditions only expeditions,
  Tournaments/Sweetener/Deconstruction show none). One buffer → one reveal surface.
- If a recipe reward is **held** (inventory full, §5.3), surface a non-blocking nudge ("1 recipe waiting —
  free a recipe slot to receive it"), not a reveal.

## 7. Frontend change list
- Remove claim-move emission (`rewards.ts toClaimable`/`claimableRewards`, the four `move-builder`
  claim verbs) → replace with reveal-only flush. Drop the `blockedBy` sweetener-ordering workaround.
- New per-player reward baseline + unrevealed buffer module (localStorage), wired to the poller.
- "Claim all" button + unified reveal; keep `RewardRevealModal` as-is.
- Remove the sweetener "unclaimed reward blocks re-cook" gate and the unclaimed-cap warnings (candy path).
- Deconstruction: fighter/recipe now vanish at resolve — update FighterDetail to reveal the candy result.

## 8. Decisions — RESOLVED (2026-07-04)
- **D1 — Recipe overflow → HOLD + auto-drain + near-full warning.** Confirmed no GSP clog (§5.3): holding
  is strictly less state than today; draining is event-driven (fires only when that player frees a slot).
- **D2 — Reveal signal → ADD `rewards_serial`** (tiny append-only per-player counter) for reliable
  offline detection; contents still via inventory diff.
- **D3 — Cross-device reveal → ACCEPT** per-device baseline (reveal is cosmetic).
- **D4 — Drop StartExpedition + sweetener unclaimed gates → YES** (moot once nothing sits unclaimed).
- **D5 — Armor `rewardsource` → FIX to the true source** (Expedition/Tournament/Sweetener) — known at
  resolve, more correct, and we're regenning goldens anyway. (Flag if any consumer depends on the old
  always-Expedition value; frontend armor panel only displays the piece, doesn't branch on source.)
- **D6 — Scope → ALL FOUR sources in one fresh-genesis change.**
- **Button rename (new):** the reveal trigger must NOT say "Claim" (no tx) → rewards bell + badge; modal
  button stays "Collect" (§6.3).
- **Fresh genesis (new):** DB wiped at launch → no backward-compat / no fork cutover (§5.4).

## 9. Phased build plan
1. **GSP §5.1–5.3** behind a fork height: factor `CreditActivityReward`, credit at resolve for all four
   sources, recipe hold+drain, claim handlers → no-ops. Preserve RNG order.
2. **Golden regen + tests**: unit (each resolve path credits correctly, recipe overflow holds+drains,
   account-bound suppression, HALT-01, tournament fighterid=0), reorg/replay determinism, golden replay.
3. **Frontend**: baseline/buffer module, "Claim all" reveal, remove claim moves + dead gates.
4. **Fork e2e** on the forked-evm testing stack: resolve an expedition/tournament/sweetener/deconstruction
   and assert inventory credited with **no claim tx**, reveal fires, recipe-overflow holds then drains.

## 10. Risks
- Consensus fork — must regen goldens and pass the full replay/reorg suite; RNG stream must be untouched.
- Recipe overflow queue is the one place bounded state remains — keep it capped + auto-draining.
- Reveal accuracy across offline gaps is best-effort without `rewards_serial` (D2).
- Tournament move/armor/anim with `fighterid=0` are dropped today — confirm intended (likely tournaments
  only roll candy/recipe) so nothing silently changes.

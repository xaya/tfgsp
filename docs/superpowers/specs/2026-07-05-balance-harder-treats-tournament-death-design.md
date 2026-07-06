# Balance: harder rarest treats + tournament permadeath — design spec

**Date:** 2026-07-05
**Repo:** `tfgsp-polygon` (branch `polygon-rewrite`)
**Status:** design — awaiting user review before planning

## Goal

Two balance changes to make Treatfighter harder and raise the stakes of PvP, plus the
runtime-tuning infrastructure to adjust them live:

1. **Rarest treats ~4× harder.** The Epic (quality-4) *generated* recipe — the only path to an
   Epic fighter — drops ~4× less often from the sweetness-8 expedition/tournament reward tables
   (~0.6% → ~0.15% per sw8 expedition send).
2. **Tournament permadeath with 50/50 capture.** When a team loses a tournament, one of its
   entered fighters is lost — a coin-flip between being **destroyed** and being **captured**
   (ownership transferred to the tournament winner).
3. **Runtime-tunable params** (Soccerverse pattern): a `parameters` KV table in game state, set
   live via a `g/tf` admin move, so the difficulty divisor and the permadeath knobs can be retuned
   without a rebuild or genesis re-pin.

These are consensus-critical (they change the deterministic RNG output and game state). The game is
**pre-launch on a fresh genesis**, so any golden-replay change is acceptable — regenerate the golden
and re-pin the genesis; there is no live state to migrate.

---

## Change 0: Runtime-tunable params (infrastructure) — matches Soccerverse

Treatfighter today keeps all tunables in the **compile-time RoConfig protobuf blob**
(`ctx.RoConfig()->params()`), so changing one needs a rebuild + genesis re-pin. Soccerverse
(github.com/soccerverse/soccerverse) instead keeps live-tunable params in **game state** and changes
them via admin moves. We adopt that pattern for the three balance knobs below.

**Soccerverse reference (verbatim mechanism):**
- Storage: a `parameters` table `(name TEXT PRIMARY KEY, value INTEGER)` in the SQLite game state
  (`database/schema.sql:1028`); accessors `GetParam`/`SetParam`/`RemoveParam` with a per-block cache
  (`database/dbhandle.cpp:351-405`). `GetParam` CHECK-fails if the name was never set → **every param
  must be seeded at genesis** (`database/params.cpp`).
- Admin move: sent on the **`g/<gameid>` name** (libxayagame's admin channel — only that name's owner
  can send; delivered to the GSP as `blockData["admin"]`). JSON:
  `{"cmd":{"param":[{"n":"<name>","v":<int|null>}]}}` — an array of `{n,v}` ops; `v:null` removes,
  `v:<int>` sets (`smcd/moveparser.cpp:1542-1576`, `smcd/moveprocessor.cpp:386-400`).
- An `OnParameterChange(name, value)` hook runs one-off side effects when a param flips (optional; we
  do not need it initially).

**Treatfighter today (verified):**
- `ProcessAdmin(blockData["admin"])` **is** already wired on every chain incl. POLYGON
  (`src/logic.cpp:126` → `src/moveprocessor.cpp:148-158`). The current `ProcessOneAdmin` only calls
  `HandleGodMode(cmd["god"])`, which is gated by `params().god_mode()` (false on POLYGON → unreachable).
- Game id `"tf"` (`src/main.cpp:247`) → admin name `g/tf`.
- No params KV table exists; there is a single-row-ish `globaldata` table (`database/schema.sql:56`)
  used for other global state — we do **not** overload it; we add a dedicated `parameters` table to
  mirror Soccerverse faithfully.

**TF implementation** (model the new table + accessor on TF's existing **`GlobalData`** class,
`database/globaldata.{hpp,cpp}` — the closest existing analog: a genesis-seeded, admin-updatable
game-state config table):
- **Schema:** add `CREATE TABLE IF NOT EXISTS parameters (name TEXT PRIMARY KEY, value INTEGER NOT NULL);`
  to `database/schema.sql` (keep the Soccerverse name `parameters`). A brand-new table is required — do
  NOT add columns to `globaldata`: `CREATE TABLE IF NOT EXISTS` is a no-op on an already-created DB, so
  adding a column would silently diverge a fresh-sync node from an already-synced one. Precedent: the
  `ongoing_operations` table was added this way (byte-identical golden) without touching the genesis
  pin.
- **Accessor:** new `database/params.{hpp,cpp}` — a `GameParams(Database& db)` class mirroring
  `GlobalData` in style (uses `Database::Statement`/`Result`/`RESULT_COLUMN` like every TF table class):
  `GetParam(name) -> int64`, `SetParam(name, value)`, `RemoveParam(name)`, `HasParam(name)`.
- **Defaults + read semantics — seed at genesis, `GetParam` CHECK-fails on unset (Soccerverse rigor).**
  Seed the three params in `PXLogic::InitialiseState` right beside the existing
  `GlobalData::InitialiseDatabase()` call (`src/logic.cpp:199-200`). Because TF is **pre-launch and we
  re-pin genesis for this change anyway**, every param is seeded at genesis, so `GetParam` can (and
  should) CHECK-fail on an unset name — this is strictly safer than a hardcoded C++ fallback default:
  a fallback constant that some later binary changes while on-chain rows are unset would be a *silent
  consensus hard fork* (old nodes read 4, new nodes read 5 for identical state). CHECK-fail forces the
  authoritative value to always be the on-chain seeded/admin value, never a drifting code literal.
  Seeded values:
  - `rarest_recipe_drop_divisor = 4`
  - `tournament_loss_kills_enabled = 1`
  - `tournament_capture_pct = 128`   (fixed_8: 128/256 = 0.5 = the 50/50 split)
  - `tournament_max_captures = 3`     (max fighters the single winner may capture per tournament)
- **Admin verb:** extend `ProcessOneAdmin` (`moveprocessor.cpp:161`) with a **non-god-gated**
  `HandleSetParam(cmd["param"])`, parsing exactly like Soccerverse's `TryAdminSetParam`: an array of
  `{n:string, v:int|null}`; `v:null` → `RemoveParam`, int → `SetParam`. Keep `HandleGodMode(cmd["god"])`
  as-is (still god-gated) for the existing `cost`/`version`/`vanillaurl` verbs. **Authorization is
  chain-level only, matching Soccerverse:** libxayagame delivers admin moves solely from the `g/tf`
  name owner, so whoever controls `g/tf` is the admin — no in-code address gate. (`dev_address` is a
  payment-recipient field only; it is NOT wired to admin auth, and we don't add it.) **Guard rails:**
  reject unknown param names and out-of-range values (divisor `>= 1`; `tournament_capture_pct` in
  `[0,256]`; `tournament_loss_kills_enabled` in `{0,1}`; `tournament_max_captures` in `[0,1000]`) so a
  fat-finger can't brick consensus (mirrors
  the existing `MaybeSetNewCostMultiplier` range guard, `moveprocessor.cpp:194-204`).
- **Reads:** `GameParams gp(db); gp.GetParam("...")` at the point of use each block. `db` is already in
  scope at every relevant call site (tournament resolution and `RecipeInstance::Generate`, which already
  threads `Database& db`).

**Consensus:** the `parameters` table is deterministic game state; a `setparam` is an on-chain move
applied identically on every node at the block it lands; reads are deterministic. Setting a
consensus-affecting param takes effect from the block the admin move is processed — all nodes agree.

---

## Change A: Tournament permadeath + 50/50 capture

**Where:** `PXLogic::ProcessTournaments` / the tournament-resolution loop
(`src/logic_tournament.cpp`, the block that already iterates participants and calls `UpdateSweetness`
at ~line 261-269 after computing `winnerid` and applying per-pair ELO).

**Rule (evaluated once, at resolution, only if `tournament_loss_kills_enabled == 1`):**
For each **non-winning team** (every participating owner whose team is not `winnerid`), processed in
**ascending owner-name order** (a fixed, deterministic sequence — not the RNG or map-iteration order),
in a single fixed RNG sequence:

1. **Build the loseable set:** the team's entered fighter ids (the `fc` roster, `TeamSize` long) that
   are **not account-bound** (`isaccountbound`/`IsAccountBound`) starters, walked in deterministic
   roster order. Starters are immune and excluded up front; **a team of only starters loses no one.**
2. **Shield the strongest, then take the worst performer:** identify the single **highest-`rating`**
   loseable fighter (ties broken by **lowest fighter id** → consensus-stable) and shield it — the
   `candidates` are the loseable set **minus the shielded fighter** (*unless* there is only one loseable
   fighter, in which case it is the victim). Among the `candidates`, the victim is the **worst performer
   this tournament**: the fewest **net wins** (`wins - losses`, read from the per-fighter
   `TournamentResult` rows just populated by `ProcessFighterPair`). Ties for worst are broken by **one**
   `rnd.NextInt(worst.size())` draw (a constant-count draw → deterministic RNG stream).
   **You always lose exactly one fighter when you have anything loseable, but never your strongest, and
   the one you lose is the one that did worst.** (The `rating` read here is post-combat:
   `ProcessFighterPair` updates ratings earlier in the same resolution, before this block.)
3. **Destroy-or-capture (capped):** draw `roll = rnd.NextInt(256)`. **Capture** — reassign the fighter's
   owner to `winnerName` (keep all stats/rating/sweetness/moves/armor unchanged; clear any transient
   status back to `Available`; recompute affected value/prestige aggregates for both the loser and the
   winner) — iff **all three** hold: `roll < tournament_capture_pct`, the winner's roster has a free slot
   (`fighters.CountForOwner(winnerName) < max_fighter_inventory_amount`), **and** the winner has captured
   fewer than `tournament_max_captures` this tournament. Otherwise **destroy**: delete the fighter
   (`fighters.DeleteById`), and free any references (it is not on the exchange — it was in a tournament —
   and not mid-cook). Each successful capture increments the winner's per-tournament capture counter.

**Edge cases (explicit):**
- **Shield is exactly one (never zero, never two):** with ≥2 loseable fighters the single highest-rated
  is shielded and the victim is the **worst performer** among the rest; with exactly one loseable
  fighter nothing is shielded and that one is the victim (you still lose one). Ties at the top rating
  shield only the lowest-id fighter — the other tied fighters remain eligible victims.
- **Draw / no winner:** if the tournament has no `winnerid`, no deaths occur (nobody "lost").
- **Winner roster full (48):** capture is impossible → falls back to destroy (still a loss).
- **Capture cap (`tournament_max_captures`):** across all losing teams in one tournament, the single
  winner captures at most `tournament_max_captures` fighters; every victim past the cap is destroyed
  instead. Because losing teams are processed in ascending owner-name order, the cap is filled by the
  alphabetically-first teams and later teams' victims are destroyed.
- **Multi-team tournaments (TeamCount > 2):** every non-winning team loses one fighter as above; the
  single winner may capture several — bounded by both the 48-slot roster cap (re-checked as each capture
  fills a slot) and `tournament_max_captures`.
- **Account-bound fighters are never captured** (they are non-transferable by definition — consistent
  with starter protection; no special case needed beyond step 2).
- **Value/prestige & sweetness aggregates:** on capture/destroy, recompute the owner-level derived
  fields the tournament path already maintains, so state stays consistent.
- **The victim fighter's own status:** it was `Tournament`; on destroy it's deleted, on capture it
  becomes `Available` under the new owner. (Surviving fighters flip back to `Available` as today.)

**RNG determinism:** the added draws (`NextInt(worst.size())` for the worst-performer tiebreak, then
`NextInt(256)` for the capture roll) extend the existing tournament-resolution RNG stream in a fixed
order. The loseable set, the shield (max `rating`, lowest-id tiebreak) and the worst-net set are all
pure functions of consensus state, so every node picks the same `worst` set, the same victim, and the
same capture outcome; the capture-cap check and the `battle_losses`/`rewards_serial` writes consume no
RNG. Because this is a fresh-genesis change, the golden replay is regenerated wholesale if the scenario
exercises a permadeath; the requirement is only that the order is fully specified and stable
(losing-team order fixed; loseable/shield/worst-net deterministic; tiebreak draw before capture draw).

---

## Change B: Rarest (Epic) treats ~4× harder

**Where:** the activity-reward weighted pick that turns a reward table into one rolled reward — the
`totalWeight` / `accumulatedWeight` cumulative-walk loops in `src/logic_resolve.cpp` (the four existing
pick loops at ~`349-380`, `408-433`, `469-492`, `650-673`). Prefer factoring these into ONE shared
weighted-pick helper and applying the scaling there (DRY), rather than editing four copies. `db` is in
scope at these call sites (and in `RecipeInstance::Generate`, `database/recipe.cpp:179`). The same 6
tables feed both expeditions and tournaments: `RewardTable_Expedition_T4Epic{1,2,3}` and
`RewardsTable_Tournament_T4Epic{1,2,3}`.

**Rule:** when building the weighted distribution for a roll, scale the weight of every entry that is
**NOT** an Epic generated recipe (`Type == GeneratedRecipe && GeneratedRecipeQuality == 4`) by
`divisor = GetParam(db, "rarest_recipe_drop_divisor")`, and keep Epic entries at their configured
weight. Then draw against the scaled total. Effect: Epic's relative probability becomes ~`1/divisor`
of current (weight 2 / 1340 → ~2 / (4·1338+2) ≈ ¼), while the *relative* mix of all other rewards is
unchanged (they all scale together).

- Only quality-4 (Epic) is scaled; Rare (q3) is untouched (per "just the rarest").
- `divisor >= 1` guaranteed by the admin guard (divisor 1 = no change; the seeded default is 4).
- Applies uniformly wherever a reward table is rolled, so both the expedition and tournament Epic
  paths get 4× rarer.

**RNG note to verify in implementation:** whether `xaya::Random::NextInt(n)` consumes a *fixed* amount
of entropy regardless of `n` (mask-and-mod) or uses rejection sampling (variable). Either way the
result is deterministic; if fixed-entropy, downstream draws stay stream-aligned and the golden diff is
localized to reward outcomes; if rejection-sampling, changing the total range can shift subsequent
draws (a wider golden diff). Both are acceptable pre-launch (regen + re-pin); document which it is.

---

## Change C: Loss notification (REQUIRED)

A player **must be notified when they lose a treat in a battle** — you can't silently delete/transfer
someone's fighter. This reuses the existing serial-driven reward-reveal pipeline
(`rewards_serial` + `snapshotInventory`/`rewardDelta` on the client), extended to the negative case.

- **GSP:** on a tournament death/capture, bump the losing player's `rewards_serial` (so the client
  re-reads and knows something changed for them) AND record a small client-visible loss entry so the
  client can say *what* happened, not just "a fighter vanished." Record shape (a new `battle_losses`
  row per loss, surfaced in `getuser`, or an extension of the existing `rewards` table):
  `{ fighterid, name, outcome: "destroyed"|"captured", opponent: <winner name>, tournamentid }`.
  It clears the same way rewards do (serial-baseline advance), so it shows exactly once.
- **Frontend:** a dramatic "you lost a treat" reveal (mirror of the win reveal): extend
  `reward-reveal.ts` `rewardDelta` to also compute `lostFighters` (ids in the before-snapshot missing
  from after), and a `RevealSwitch` branch → a `FighterLossModal` ("**{name} was destroyed**" /
  "**{name} was captured by {opponent}**"). No asset can leave a roster without this reveal firing.
- The **winner** side (captured a fighter) rides the existing new-fighter reveal (a fighter appeared in
  their roster) — already handled by the `newFighters` path; optionally label it "captured from {loser}".

## Testing

New/extended C++ unit + golden tests (`src/*_tests.cpp`, `database/*_tests.cpp`):
- **Params:** seed defaults present after genesis; `setparam` admin move updates a value; `v:null`
  removes; unknown-name and out-of-range values rejected (state unchanged); a non-`g/tf` sender cannot
  set params (covered by libxayagame's admin delivery — assert the parser rejects malformed shapes).
- **Permadeath:** a losing team's fighter is destroyed when `roll >= capture_pct`; captured (owner
  transfers, stats intact) when `roll < capture_pct` and winner has room; capture→destroy fallback when
  winner roster is full; **account-bound starter never dies/transfers**; draw → no deaths;
  `tournament_loss_kills_enabled = 0` disables the whole mechanic; multi-team: each loser loses one.
- **Shield the strongest:** the highest-`rating` loseable fighter survives while a weaker one dies —
  with the strongest deliberately given the **higher id** so the test proves the shield keys on rating,
  not id order (`TournamentPermadeathShieldsStrongest`).
- **Worst performer goes:** in a 3v3v3 with single move types so combat is deterministic, the losing
  team's **unique fewest-net-wins** fighter is the victim — armed as the **highest-id / last candidate**
  so an id/roster-order rule would wrongly pick the first candidate; the shielded top-rated (middle-id)
  and a better-performing teammate survive (`TournamentPermadeathTakesWorstPerformer`). Every non-winning
  team loses exactly one.
- **Shield overrides worst:** when the shielded top-rated fighter is *also* the unique worst performer,
  it still survives (a net-0 teammate dies) — removing the shield would kill it, so the test
  deterministically proves the shield exists (`TournamentPermadeathShieldOverridesWorst`).
- **Capture cap:** with `tournament_max_captures = 1` and always-capture, the first losing team's victim
  is captured and the next losing team's victim is destroyed — the winner gains exactly one
  (`TournamentCaptureCapLimitsWinnerGains`); with `= 0` every victim is destroyed and the winner gains
  nothing (`TournamentCaptureCapZeroDestroysAll`).
- **Roster-full fallback:** with the winner's roster full, a rolled capture downgrades to a destroy via
  the 48-slot check (distinct from the cap path) (`TournamentCaptureRosterFullDestroys`).
- **Loss notification (Change C):** a death/capture writes a `battle_losses` record
  (`{fighterid,name,outcome,opponent,tournamentid}`) for the loser and bumps their `rewards_serial`;
  `getuser` surfaces it; FE `rewardDelta` computes `lostFighters` and the loss modal fires exactly once
  (serial-baseline clears it). No fighter leaves a roster without a recorded, revealable loss.
- **4× divisor:** with `divisor = 1`, Epic roll rate matches the pre-change baseline; with `divisor = 4`,
  Epic's expected rate is ~¼ (statistical test over a fixed seed, or exact bucket-boundary assertion);
  Rare (q3) rate unchanged.
- **Golden replay:** regenerate; review the diff is bounded to (a) reward-roll outcomes shifting and
  (b) tournament-loss fighter ownership/deletion + the new `parameters` rows. **Reorg tests** must still
  pass (permadeath + capture must undo cleanly on a reorg — they are ordinary state writes, so the
  existing undo machinery covers them; add a reorg test that a captured fighter reverts ownership).

## Rollout / determinism checklist

- Build only via `tf-builder:local` Docker (`--network none`), `make check` gate.
- **Golden regen** after diff review (`GOLDEN_REGEN=1 ./goldenreplay_tests`).
- **Genesis re-pin**: the schema (+`parameters` table +seeded rows) and the RNG changes shift the
  state hash → fold into the existing pre-mainnet genesis re-pin blocker.
- No RoConfig blob change is strictly required for the divisor (it lives in game state now), but the
  seeded defaults are part of genesis state.
- Frontend: none required for the GSP rules; optionally surface "captured/lost a fighter" in the
  tournament result UI and the reveal pipeline (follow-up, not in this spec).

## Files (anticipated)

GSP (`tfgsp-polygon`):
- `database/schema.sql` — add `parameters` table (+ `battle_losses` table for Change C).
- `database/params.{hpp,cpp}` (new) — `GameParams` (`GetParam`/`SetParam`/`RemoveParam`).
- `src/logic.cpp` — seed default params in `InitialiseState` (beside `GlobalData`, ~line 199-200).
- `src/moveprocessor.{hpp,cpp}` — non-god-gated `HandleSetParam(cmd["param"])` with range guards.
- `src/logic_tournament.cpp` — permadeath + 50/50 capture + record the loss + bump the loser's serial.
- `src/logic_resolve.cpp` (+ a shared reward-pick helper) — divisor scaling in the weighted pick.
- `src/gamestatejson.cpp` — surface the loss record in `getuser`.
- `src/*_tests.cpp`, `database/*_tests.cpp`, `src/reorg_tests.cpp` — coverage above.
- Golden fixtures — regenerated.

Frontend (`tf-frontend`, Change C reveal):
- `src/data/reward-reveal.ts` — add `lostFighters` to the diff.
- `src/ui/chrome/RevealSwitch.tsx` + new `FighterLossModal.tsx` — the "you lost a treat" reveal.

## Out of scope / related future work (captured, NOT in this spec)

**Duels + game channels + real-time combat** — a future epic the user raised: a **1v1 duel mode** where
the winner **picks** one of the opponent's treats (player-chosen capture, vs this spec's random
tournament capture), built on **Xaya game channels** (off-chain signed real-time play; see the
`xaya-game-skill` / XayaShips reference), with **interactive combat** (Street-Fighter- or
Final-Fantasy/Axie-Infinity-style) replacing the current deterministic auto-resolve — likely as a
separate tournament/battle type. This is a large, separate design (its own spec → plan cycle) and is
explicitly deferred. This spec's permadeath/capture rules are the on-chain-tournament analog and should
compose with, not block, the future duel mechanic.

## Open items for the plan (not blockers — code facts to confirm during implementation)

- Confirm/introduce the single shared reward-pick helper to scale (vs the four current copies).
- Confirm `xaya::Random::NextInt` entropy consumption (fixed-entropy vs rejection-sampling) and note it
  — either is deterministic and acceptable pre-launch; it only affects how wide the golden diff is.

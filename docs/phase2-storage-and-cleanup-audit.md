# Phase 2 — State storage & Taurion-cleanup audit + Stage 2b plan

**Status:** living document. Storage audit ✅ complete. Taurion-cleanup audit ✅ complete.
Implementation: not started (awaiting user sign-off on order + the gating decisions in §7).

This is the canonical record of *what we did, what we found, and what we will do* for the Phase 2
efficiency + cleanup pass. It complements `docs/p-e1-loadbench.md` (the before/after benchmarks).

---

## 0. Method (how these findings were produced)

Two independent multi-agent audits, each **adversarially verified** — every "we don't need this" claim
was handed to a separate agent whose only job was to *refute* it by grepping the whole tree for live
consumers. Default verdict on uncertainty = `must_keep` (a wrong delete in a consensus GSP forks the
chain). Plus a dedicated Explore pass that produced the exact Stage 2b edit list ("cutover map", §5).

- **Storage audit** (`wutybm3a0`, 39 agents): 7 surfaces — player proto, fighter proto,
  rewards/activities, tournaments, recipes, undo/write-amp, globaldata/inventory/money — each finder
  then per-claim verifiers.
- **Taurion-cleanup audit** (`w34zjavuf`): fork gates, dead Taurion concepts, dead code, DRY
  violations, dead files/build-cruft, dead protos/config. *(results in §6 when complete)*

Every change below is gated by the golden-replay harness (`src/goldenreplay_tests.cpp` +
`goldenreplay.golden.json`) — keep it byte-identical, or regenerate deliberately at the same
fresh-relaunch genesis.

---

## 1. Headline

The DB-size problem is **row accumulation that is never pruned after the data is consumed — NOT fat
fields and NOT (mainly) the undo log.** Player count drives a one-time ~125 MB of state; the unbounded
growth comes from four append-only leaks (§3). Dead proto fields the finders flagged are proto2
optionals that are never set → **0 bytes on disk today** — removing them is hygiene, not a size win.

Two different problems, two different fixes:
- **Per-block churn / undo growth** (the "writes every block / clogs the DB" complaint): Stage 1 (done)
  + Stage 2b (ongoings → height table) + `--enable_pruning=1000` + the write-amp fixes (§4).
- **Permanent on-disk growth** (the real long-term size): the row-lifecycle redesigns (§3) — consensus
  changes, each in its own carefully-replayed commit.

---

## 2. Pruning decision

`--enable_pruning=1000` (production). Rationale:
- Caps the undo log (`xayagame_undo`) **retention** to the last 1000 blocks. Worst case measured (1000
  concurrent in-flight builds = ~80.9 KB undo/block) → steady-state ceiling **~81 MB**, flat, instead
  of unbounded (~3.5 GB/day → ~100 GB/month).
- 1000 Polygon blocks ≈ **33 min** of rollback depth. Polygon finalizes in 1–2 blocks (Bor/Heimdall);
  real reorgs are 1–2 deep. So 1000 is safe and generous (256 would also be safe).
- Pruning caps *retention*; **Stage 2b caps *generation*** — with the absolute-height table an in-flight
  build writes undo only on its schedule block and its fire block, not every block, so undo/block on
  ordinary blocks → ~0. Do both.

---

## 3. Permanent growth to fix (the real long-term size; each = its own consensus commit, NOT Stage 2b)

| Leak | Lifecycle gap | Size @ 50k | Fix | Verdict |
|---|---|---|---|---|
| **Leaked `recepies` rows** (BIGGEST) | On cook, owner blanked (`moveprocessor.cpp:5012`) but row never deleted — delete is commented out at `logic.cpp:654-663`. Every fighter ever cooked (+2 tutorial recipes/player) leaks a row. | **~1–4 GB** | Redesign: copy `requiredcandy` onto the Fighter proto at cook; rewrite `ResolveDeconstruction` to read fighter-side; fix sweetener-cauldron display; accept state-hash semantic change. Do at genesis, replay-tested. | `must_keep` as a naive delete — 3 live consumers (deconstruction refund `logic.cpp:432`, anti-fork state-hash count `logic.cpp:2260`, sweetener JSON `logic.cpp:661`) |
| **Unclaimed `rewards` rows** | Created on expedition (1–6 each)/sweetener/tournament/deconstruction; deleted ONLY on explicit claim (`moveprocessor.cpp:3493/3500/3620`). No TTL, no cap → abandoned accounts leak forever. | ~50 MB+ growing | Deterministic lifecycle bound: TTL/auto-resolve after N blocks, or per-owner pending cap. The rule itself is consensus-critical → identical across nodes. | high-confidence problem |
| **Orphan `recepies` from unclaimed recipe rewards** (compounds rewards) | `GeneratedRecipe`/`CraftedRecipe` rewards pre-create an `owner=""` recipe row (`logic.cpp:168/179`) adopted only on claim; unclaimed → permanent orphan. | (part of above) | Any reward-expiry fix must jointly delete the linked recipe row (via `reward.generatedrecipeid`). | medium |
| **Completed `tournaments` never deleted** | `DeleteById` exists (`tournament.cpp:180`) but has **no caller**; completion only sets state (`logic.cpp:1936`). ~21 blueprints continuously reopen → steady stream of permanent rows; also grows per-block O(rows) scans. | ~30–60 MB/yr | **Conditional** prune: delete when `state==Completed(3)` AND no `rewards.tournamentid` references it. (Unconditional drop of Completed rows = consensus break via the claim guard at `moveprocessor.cpp:1397`.) | `reduce_only`/low |

---

## 4. Per-block write amplification (cheap, byte-identical — fold into Stage 2b)

All verified `reduce_only`, low risk. These add per-block undo/WAL churn or O(N) scans with no state
benefit. Validate each via goldenreplay (must stay byte-identical).

| Fix | File:line | What |
|---|---|---|
| `MutableProto()` → `GetProto()` | `logic.cpp:1049`, `:1156`, `:1415` | Calls `MutableProto()` only to **read** `specialtournamentinstanceid()` → flips LazyProto to MODIFIED → full fighter-proto rewrite for every special-tournament fighter. |
| Tier write value-compare guard | `logic.cpp:1322`, `:1343`, `moveprocessor.cpp:4478` | `set_specialtournamentprestigetier()` fires for ALL ~50k players every ~25 h even when unchanged → full player-BLOB rewrite burst. Only set when `pTier != current`. Apply to BOTH `RecalculatePlayerTiers` impls. |
| `teamsjoined` value-compare guard | `logic.cpp:1732/1774` (write; read only `gamestatejson.cpp:345`) | Rewritten every block for ~21 idle Listed tournaments. Guard with value-compare. |
| Whole-row blueprint rewrite | `tournament.cpp:88-114` | `INSERT OR REPLACE` re-binds the static blueprint blob on every instance-only change; use `UPDATE ... SET instance=?` when only the instance is dirty. (Mostly disappears once `teamsjoined` per-block write is gone.) |

**Event-driven (defer; 0 undo bytes but violate "~zero work per idle block"):** make
`CheckFightersForSale` (`logic.cpp:1996`), `SetFreeTransfiguringFighters` (`logic.cpp:2019`),
special-tournament count (`logic.cpp:909`), and nested `ReopenMissingTournaments` (`logic.cpp:2057`)
index-by-expiry / incremental. **Determinism trap:** the special-tournament count gates first-time tier
setup (`logic.cpp:938`) and resolution sizing (`:1011`) — preserve exact semantics.

**Tournament `blocksleft` → endHeight: do NOT do in isolation** (`must_keep`). It is the consensus
completion trigger (`logic.cpp:1793`); converting yields zero write savings while `teamsjoined` rewrites
the blob anyway, and risks shifting completion timing. Revisit only after `teamsjoined` is fixed.

---

## 5. Stage 2b cutover map (ongoings: player-proto `repeated ongoings=7` → height-keyed table)

`OngoingsTable`/`OngoingOperation` (`database/ongoings.{hpp,cpp}`) already exist (Stage 2a), unwired.
Table API: `CreateNew(startHeight)`, `GetFromResult[+cfg]`, `GetById`, `QueryAll`,
`QueryForHeight(h)`=`WHERE height<=h`, `QueryForOwner(owner)`, `CountForOwner(owner)`,
`DeleteForHeight(h)`, `DeleteForOwner`, `DeleteById`.

**27 sites across 4 files** (full detail in scratchpad `stage2b-cutover-map.md`):

1. **4 add-ongoing sites** — `moveprocessor.cpp:4332` (DECONSTRUCTION), `:4799` (EXPEDITION),
   `:4886` (COOK_SWEETENER), `:5006` (COOK_RECIPE). New: `row.height = currentHeight + duration`,
   `row.owner = name`, proto keeps type + refs, **drop BlocksLeft** (now derived).
2. **`TickAndResolveOngoings`** (`logic.cpp:667-750`) — rewrite: `QueryForHeight(currentHeight)` →
   dispatch by `proto.type()` to the 4 `Resolve*` (load owner player by `row.GetOwner()`) →
   `DeleteForHeight`/`DeleteById`. **No per-block decrement** (kills 809 undo-bytes/block/active).
3. **4 dup-check sites** — `moveprocessor.cpp:1479/1840/1901/2786` — replace
   `a.GetProto().ongoings()` loops with `QueryForOwner(name)` (1840 needs a type-filtered COOK_RECIPE
   count vs slot limit).
4. **6 gamestatejson emission reads** — `gamestatejson.cpp:193,424,432,444,672,680`. `blocksleft =
   row.height - currentHeight`. Build owner→ongoings map once via `QueryAll` (don't do N `QueryForOwner`).
5. **`pending.cpp:476-503`** — pending-preview emits `blocksleft`; migrate to derived height.
6. **`xayaplayer.hpp:232` `GetOngoingsSize()`** — remove after cutover (was the Stage-1 guard).
7. **`proto/xaya_player.proto`** — `reserved 7;` (drop `repeated ongoings`). `proto/ongoing.proto`
   `BlocksLeft=1` now derived (drop unless pending needs it); `StartHeight=9` already added.

⚠️ **DETERMINISM TRAP (most important Stage 2b correctness item):** resolution currently consumes `rnd`
in *QueryAll player order, then proto order*. The new path must pin an explicit deterministic
`ORDER BY` (e.g. `id`) so cooked-fighter/expedition-reward RNG outputs are identical across nodes.
Fresh relaunch means it need not match the OLD order byte-for-byte (no migration), but it MUST be
deterministic; the golden re-gen then locks the new behavior.

**Also required in Stage 2b (per user):** add a **reorg test** (apply N blocks → `ProcessBackwards` →
assert state byte-identical), and set `--enable_pruning=1000`.

---

## 6. Taurion-cleanup audit results ✅ (audit `w34zjavuf`, 54 agents)

The heavy Taurion world-sim (regions, movement, hex-grid, combat, vehicles) was already stripped in
earlier phases. What remains is **CHI-fork/version baggage + orphan files + stale vocabulary** — not
deep rot. On the Polygon genesis (height 89,246,000) every `>=5097362` gate is permanently true and
every `<5097362` permanently false, so collapsing them is byte-identical (golden stays green). Total
reachable reduction: **~1,300–1,800 LOC C++/proto + ~1.38M lines / ~21 MB of dead config data.**
(2 of 54 verifier agents hit the StructuredOutput retry cap → 2 claims unverified, conservatively
excluded from the "delete now" set.)

### 6a. Quick wins — pure deletion, `safe_to_delete`, golden byte-identical
- **Charon client** `src/charon.{cpp,hpp}` (~629 LOC, in no Makefile, won't compile) + `CHARON_PREFIX`
  (`configure.ac:62-66`, `gametest/Makefile.am:2`) + the `PKG_CHECK`/`CHARON_CFLAGS/LIBS` probe.
- **ForkHandler subsystem** `src/forks.{cpp,hpp}`, `forks_tests.cpp`, `Context::forks`/`Forks()`,
  gflag, `GameStartTests`, the `#include "forks.hpp"` in `context.hpp:22`/`pending_tests.cpp:24`/
  `moveprocessor_tests.cpp:24` + `moveprocessor.cpp:22` + `Makefile.am:22,35,116`.
- **Orphan files:** `jsonrpclib/` dangling gitlink; `proto/roconfig_blob.s` (byte-dup of the wired
  `proto2/` one); `data/letsencrypt.pem` + `data/Makefile.am` (+ drop `data` from SUBDIRS & configure);
  `src/fpm/ios.hpp` (+ `Makefile.am:32`); committed autotools/log cruft (`config.h`, `config.status`,
  `config.log`, `config.h.in~`, `configure~`, `help.log`, `whelp.log`) + `.gitignore` them.
- **Dead code:** `IsDirtyCombatData()` ×4 (`fighter/recipe/reward/tournament.hpp`); Taurion RPC error
  codes (`pxrpcserver.hpp:41,47-55`); one-shot fork-migration trigger (`logic.cpp:2116-2121`); dead
  `injection==true` branches (`logic.cpp:1420,1447-1460`).
- **License/comment rewording:** 20 files (incl. 6 `gametest/*.py`); `faction`→`role` prose; Taurion
  world terms in doc comments. **Prose only** — `PlayerRoleToString` codes / enum ints / SQL `role`
  column are golden-locked.

### 6b. Dead protos / config — `safe_to_delete`, low/none risk
- 🟢 **THE BIG ONE — dead `blocks` config (~21 MB / ~1.38M lines).** `Blocks` message +
  `ConfigData.blocks` (field 15) + `proto/roconfig/blocks.pb.text` + `blocks.proto` + Makefile entries.
  Read by exactly one unreachable branch. **Must delete together with that reader (`logic.cpp:2091-2104`)
  in one commit** — a text-proto parse fails on an unknown field otherwise. `reserved 15`. Golden
  unaffected (REGTEST never reads it). Single biggest footprint win.
- `XayaPlayer.notused`(1), `role`(2, the *proto* field — live role is the SQL column/`PlayerRole` enum,
  NOT this) → delete + `reserved`.
- `Params.*_cook_duration`(14-17) + their `params.pb.text` lines (live duration is per-recipe
  `duration()`); `Params.fungible_items`(26) (live is `Inventory.fungible`) — delete proto+text together.
- Unused imports of `fighter_move_blueprint.proto` in `armor_piece.proto:22` + `fighter.proto:22`.

### 6c. Fork-gate collapses — byte-identical on fresh relaunch, `low` risk
- The **13× `isFork2 = (chain==REGTEST || Height>=5097362)`** sites (`logic.cpp:398,643,933,1834`;
  `moveprocessor.cpp:1232,1293,3121,3625,4300,4410,4533,4566,4948,4995,5032`) → factor to one
  `IsPostFork(ctx)` helper returning `true`, drop the boolean.
- 3 negative gates (`logic.cpp:1620-1627`; `moveprocessor.cpp:1551-1561` delete whole if;
  `:4651-4677` run else unconditionally); pre-launch move-drop (`moveprocessor.cpp:1322-1331`);
  manual block-hash reseed (`logic.cpp:2091-2104`, with the blocks delete); `UpdateSweetness` isFork
  branch (`fighter.cpp:356-371`); `RecipeInstance::Generate(bool fork)` param drop.
- ⛔ **Do NOT bulk-collapse two same-era look-alikes:** `moveprocessor.cpp:4596` uses **`&&`** (collapses
  to *false*, live on REGTEST) and `logic.cpp:1310` tier-formula gate (genuine divergence) — see §6e.

### 6d. DRY refactors (dovetail with Stage 2b ongoings cutover)
- **Ongoing resolve dispatch** (`logic.cpp:704-742`, 4 if-blocks) → `switch(type)` with common
  erase/restart tail factored; keep per-type field plucking. **Fold into Stage 2b.**
- **Ongoing duration clamp** (`moveprocessor.cpp:4814,4897,5022`) → one `std::max(1,duration)` helper
  (clamp only; don't touch the reject-semantics site `1723` or the field-sets). **Fold into Stage 2b.**
- `RequirePlayer(name)` helper for ~16 `GetByName`+null-log-return blocks in `moveprocessor.cpp`.
- gamestatejson cooking-recipeid loop helper (`424-438`≈`672-686`) — **preserve COOK_RECIPE-then-
  SWEETENER grouping** (merging reorders the golden-locked `recepies` array).
- `TryXxxAction` null-check dispatch inline (`moveprocessor.cpp:5081-5189`); CRTP base for the
  `(id,owner,proto)` table trio (reward/activity/recipe) **only**.
- **Keep separate (adversary overturned):** `pending.cpp` ongoing mirror (reduced projection, not a
  copy; sharing would drag the consensus `isFork2` gate into non-consensus pending); the
  specialtournament/tournament/ongoings tables (different schemas — unifying = consensus break).

### 6e. ⚠️ Needs a human eye — do NOT touch blindly
1. **`logic.cpp:1310-1346` tier-formula gate** — no `chain==REGTEST` escape, so golden (REGTEST) locks
   the *integer* branch while Polygon runs the *fpm* branch (can round differently). Golden does NOT
   cover the production tier path. Collapsing to fpm is correct but **requires a deliberate
   `GOLDEN_REGEN=1` re-bless.** Highest-risk single item.
2. **`Chain::MAIN` guards silently dead on Polygon** (`logic.cpp:481`, `moveprocessor.cpp:1618,2653,
   2758`) — intended to disable tutorial/test-blueprint shortcuts on production, but `chain==MAIN`
   never fires on Polygon → **these restrictions are OFF right now.** Possible latent production bug;
   a *rules* decision (switch to `!=REGTEST`/`==POLYGON` or confirm obsolete), not dead-code removal.
3. **CalculatePrestige "old" branch** (`xayaplayer.cpp:264-318`) — dead on Polygon but still executed
   on REGTEST via the `&&` bug at `moveprocessor.cpp:4596` (`{"injection":{}}` →
   `gametest/specialtournament.py:320`). Keep until 4596 is fixed + gametest re-baselined.
4. **`moveprocessor.cpp:4596` `&&` copy-paste bug** — fix to `||` (then retire the prestige old branch)
   or delete `MaybeSQLTestInjection` as debug cruft. Either re-baselines `specialtournament.py`.
5. `logic.cpp:2105` reseed gate → `if(chain != REGTEST)` is cleaner/identical but controls RNG —
   re-verify golden; never make reseed unconditional.
6. `database/ongoings.*` + `ongoing_operations` are the **intended Stage 2b** table (not dead) — wire
   it, don't delete.
7. `Params.prestige_total_treats_mod`(32) dead-but-populated (=100) — confirm prestige is meant to
   ignore it before deleting.
8. `RecalculatePlayerTiers` duplicated (`logic.cpp:1266` vs `moveprocessor.cpp:4426`) + REGTEST
   per-block recalc crutch (`logic.cpp:985-990`) + REGTEST injection handlers
   (`moveprocessor.cpp:4521-4626`) — test-coupled, not consensus-risky; confirm before consolidating.

---

## 7. Unified sequenced plan (each step golden-test-guarded)

Ordering principle: **safest + biggest wins first** (pure deletions, golden byte-identical), then the
consensus-touching efficiency work (Stage 2b), then gate collapses, then DRY, then the decision-gated
redesigns. Recommended order:

**Commit 1 — Taurion/Charon dead-file & dead-proto purge** *(§6a + §6b except blocks; zero consensus
surface, golden byte-identical).* Charon, ForkHandler, orphan files, autotools cruft, dead stubs/RPC
codes/injection branches, dead proto fields (`notused`/`role`/`cook_duration`/`fungible_items`/imports),
comment+license rewording. ~1,000–1,300 LOC removed.

**Commit 2 — Drop the dead `blocks` config** *(§6b big-one; coordinated single edit).* Delete reader
`logic.cpp:2091-2104` + `blocks.proto`/`config.proto` field 15+import + `blocks.pb.text` + Makefile
entries; collapse `else if(>5155298)`→`if`. **~21 MB / ~1.38M lines gone.** Golden unaffected.

**Commit 3 — Stage 2b: ongoings cutover + safe storage drops + write-amp** *(the consensus efficiency
core; §5 + §3-safe + §4 + §6d-ongoings).*
1. Wire `OngoingsTable`; migrate the 27 sites (§5); **pin resolution `ORDER BY` (determinism trap).**
2. Drop `repeated ongoings=7` (`reserved`); fold the ongoing-dispatch `switch` + duration-clamp DRY (§6d).
3. Delete the dead `activities` table + proto + wrappers (remove `logic.cpp:2256` in lockstep).
4. `reserved` `salehistory`?/`TotalMatches/Won/Lost` (fighter), `FighterAverage`, `RewardAutoTableId`,
   `tournaments.name`, recipe `Armor/AnimationId/Name` — the verified safe storage drops.
5. Write-amp (§4): 3× `MutableProto→GetProto`, tier value-compare, `teamsjoined` value-compare.
6. Add the **reorg test** (apply N → `ProcessBackwards` → byte-identical); set `--enable_pruning=1000`.
7. Regenerate golden (structural verify), re-run loadbench, `make check`.

**Commit 4 — Collapse CHI fork gates** *(§6c; byte-identical).* The 13 `||` sites → `IsPostFork()`
helper + the 3 negative gates + launch guard + UpdateSweetness + Generate param. **Skip `4596` &
`1310`.** If golden moves, stop — you hit a divergence.

**Commit 5 — DRY cleanup** *(§6d remainder).* `RequirePlayer`, gamestatejson cooking-loop helper,
`TryXxxAction` inline, `(id,owner,proto)` CRTP base.

**Deferred — own PRs, decision-gated, golden regen expected:**
- Permanent-growth redesigns (§3): recipe-row redesign (biggest size win), rewards/orphan lifecycle
  bound, conditional Completed-tournament prune, denormalization→RoConfig refs.
- The §6e traps: tier-formula collapse (golden regen), `4596`+CalculatePrestige, **`Chain::MAIN` guard
  restoration (potential live bug)**, prestige knob, `RecalculatePlayerTiers` dedup.

**Open decisions for the user (gating):**
- **Order:** purge-first (recommended) vs Stage 2b-first.
- **`Fighter.salehistory`** — drop it (unbounded growth, consensus-safe) and move the Exchange
  sales-history view off-chain, or keep it on-chain?
- **`Chain::MAIN` guards (§6e.2)** — are production tutorial/test shortcuts supposed to be off? (They're
  currently on.) Restore now or leave for a dedicated rules pass?
- JSON fields recomputed-at-emit (`tournaments.name`, recipe `Armor/AnimationId/Name`, `FighterAverage`)
  — confirm no browser frontend reads the stored form.

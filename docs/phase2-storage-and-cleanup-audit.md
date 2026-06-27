# Phase 2 — State storage & Taurion-cleanup audit + Stage 2b plan

**Status:** living document. Storage audit ✅ complete. Taurion-cleanup audit ⏳ pending (results
appended below when ready). Stage 2b implementation: not started.

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

## 6. Taurion-cleanup audit results ⏳ PENDING

Audit `w34zjavuf` running (fork gates, dead Taurion concepts, dead code, DRY, dead files, dead protos).
Results + verified delete/refactor/collapse list to be inserted here on completion, then merged into
the sequenced plan (§7). Known dovetail: the 4 ongoing types are copy-pasted (add-sites + dup-checks +
resolve dispatch) → factor into one shape during the Stage 2b cutover.

---

## 7. Sequenced plan

**Commit A — Stage 2b (ongoings cutover) + safe drops + write-amp** *(this is the next commit)*
1. Wire `OngoingsTable`; migrate the 27 sites (§5); pin resolution order (determinism trap).
2. Drop `repeated ongoings=7` (`reserved`); factor the 4 ongoing types DRY.
3. Delete the dead `activities` table + proto + wrappers (remove `logic.cpp:2256` in lockstep).
4. `reserved` the dead proto fields (`notused/role/xayaname/ftuestate/RewardAutoTableId`).
5. Drop fighter `TotalMatches/Won/Lost` (field + increment lines `logic.cpp:1659-1677`).
6. Write-amp fixes (§4): 3× `MutableProto→GetProto`, tier value-compare, `teamsjoined` value-compare.
7. Add the reorg test; set `--enable_pruning=1000`.
8. Regenerate golden (structural verify), re-run loadbench, `make check`.

**Commit B — Taurion dead-code sweep** *(after §6 lands)* — fork-gate collapses, dead files/concepts,
dead protos, remaining DRY refactors.

**Commit C+ — permanent-growth redesigns** *(separate, carefully replayed each)* —
recipe-row redesign (§3, biggest size win), rewards/orphan-recipe lifecycle bound, conditional
Completed-tournament prune, denormalization→RoConfig references.

**Open product decisions (need user):**
- **`Fighter.salehistory`** — consensus-safe to drop (unbounded growth) but removes the Exchange
  sales-history view unless that ledger moves to an off-chain indexer.
- Whether `tournaments.name` / recipe `Armor/AnimationId/Name` / `FighterAverage` JSON fields can be
  recomputed at emit (yes, per audit) — confirm no frontend depends on stored form.

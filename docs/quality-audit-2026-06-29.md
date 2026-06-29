# Quality audit — clean / DRY / deterministic / not-overengineered (2026-06-29)

Standing directive (user): *"make sure all code clean, dry, works, deterministic, not
overengineered."* This is a maintainability + correctness sweep of the GSP **consensus** code on
top of the already-completed security / money-path / storage / reorg work. Money + determinism
paths are the priority (*"people could lose money if there are bugs"*).

## Method

Multi-agent audit (workflow `wgm9hkdph`, 90 agents): 8 finders (6 per-file-group comprehensive +
a determinism specialist + a money-path specialist) → dedup → an **adversarial skeptic
independently re-read and verified every finding**, correcting its true golden-impact and fix
safety. Result: **87 raw → 82 unique → 80 confirmed (71 fix-now, 9 deferred, 2 rejected)**.

The 2 rejected were refuted in-code (REJ1 sweetener cost is already capped upstream; REJ2
GetOngoingsSize/CollectTournaments *are* called).

## Baseline (captured BEFORE any edit — for before/after comparison)

- Branch `polygon-rewrite` @ `e2f4183` (clean), built green in `tfdev`.
- **Golden** (`src/goldenreplay.golden.json`) FullState entity counts:
  `xayaplayers 5 · fighters 15 · recepies 16 · rewards 3 · tournaments 21 · crystalbundles 6 ·
  activities 0` ; `multiplier 1000 · version 1.1.5 · vanillaurl xaya.io`.
- **Suite:** 100 unit + golden + 4 reorg + 2 reorg-game + database, all green.
- **Original pre-conversion benchmark preserved** in `docs/p-e1-loadbench.md` (the old GSP's
  O(players)/block full-table-scan + per-block proto rewrite = the "writes every block / clogs the
  DB" root cause) and the post-fix numbers (event-driven: flat **21 rows/block, 0 undo bytes** from
  0→10k players / 0→1k active ongoings; ms/block grows only with raw DB size). Live attach latency
  ~41 ms/block (`forked-evm-testing/fulltest.py`). **These numbers are the conversion baseline and
  must not regress** — re-run `loadbench` after structural changes (Pass-deferred DEF2/DEF3).

## Plan — gated passes (each: build in `tfdev` → golden check → 100 unit + reorg green → local commit)

Neutral and regen changes are kept in separate commits so the neutral ones can assert a
**byte-identical** golden (the strongest correctness net) while regen ones are diff-verified
(entity counts + no dangling refs + 2× determinism).

| Pass | What | Golden | Findings |
|------|------|--------|----------|
| **A1** | Real correctness bugs that DON'T change replay | neutral (byte-identical) | FN2, FN10, FN13/69, FN40, FN46, FN54, FN55, FN56, FN70, FN71 |
| **A2** | Real correctness bugs that DO change game outcomes | **regen** | FN1, FN14/28, FN25, FN26 |
| **B**  | Dead-code / fork-baggage removal + safe DRY | neutral | FN3–9, FN11–12, FN15–24, FN27, FN29–33, FN35–39, FN41–53, FN57–61 |
| **C**  | Dead proto / config fields | **regen** | FN34, FN62, FN63, FN64, FN65, FN66, FN67 |
| **D**  | Determinism discipline: raw float → int in consensus RNG | neutral | DEF8, DEF9 |

**A2 changes observable game behavior** (reward distribution, transfigure cost, demand-queue
counts, armor-reward effect) — fixes of genuine bugs, acceptable on the fresh pre-launch chain, but
worth a config balance glance. Flagged to user.

### Deferred (report only — design-level / architectural, NOT done in this sweep)

| # | What | Why deferred |
|---|------|--------------|
| DEF1 | Reward-table lookup + weighted-roll + GenerateActivityReward block copy-pasted 5× | Large DRY refactor across 5 sites with differing args; do deliberately with golden regen |
| DEF2 | Per-block full FighterTable scans (CheckFightersForSale + SetFreeTransfiguringFighters) | **Event-driven violation** — needs a height/flag-keyed index like H3; own focused effort + reorg test. Re-run loadbench after. |
| DEF3 | Per-block tournament scans (ReopenMissingTournaments O(blueprints×tournaments), ProcessTournaments QueryAll over completed) | Same as DEF2 — architectural, event-driven follow-up |
| DEF4 | Load-fighter+owner+status validation copy-pasted across ~10 Parse* helpers | DRY refactor of consensus validation; safe but broad, do as its own pass |
| DEF5 | Move-key dispatch table duplicated between pending and Try*Action | Touches the pending read-only path; do carefully |
| DEF6 | Convert<FighterInstance> N+1 OngoingsTable/RewardsTable scans | Display-path perf; not consensus-critical |
| DEF7 | Per-entity table boilerplate copy-pasted across 6 DB classes | Large cross-cutting templatization; high churn |
| FN50 | QuantityProduct (GMP bignum) dead outside tests; drops whole GMP dep | Build-dependency change — verify GMP isn't needed elsewhere before removing |

## RESUME STATE (2026-06-29, SWEEP COMPLETE — A1/A2/B/C/D/DEF10-14/Taurion/B2/F all done)

**Pass F DONE** — `d374718` (logic.cpp 1685→335 + logic_resolve/_combat/_tournament) and `f4b89e4`
(moveprocessor.cpp 4154→531 + moveprocessor_internal.hpp shared helpers + _purchase/_cooking/_exchange/
_fighter/_activity). Pure code movement: all 66 moveprocessor + 20 logic top-level defs brace-matched and
verified to land in exactly one TU; no duplicate/undefined symbols; golden byte-identical. **The entire
quality-audit sweep is complete. Final full suite green: golden + 98 unit + 4 reorg + 2 reorg-game + 4
roconfig + 32 database.**

---
_(historical resume state below)_
## RESUME STATE (2026-06-29, after Pass D + B2 + Taurion removal COMPLETE; only Pass F remains)

**Done + committed** (branch `polygon-rewrite`, on top of `e2f4183`): Pass A1 `0c02348`, A2 `f6ffde4`,
B `1947cd0`, **FN1+FN2+FN72 `bf5271b`**, **Pass C** = `183f13a` + `419dd57` + `84c56f1`,
**Pass D DEF8/DEF9** = `11ee008` (FighterName/FighterType/MoveProbability `Probability`
float→uint32 + recipe.cpp `(int32_t)` casts; golden byte-identical, full suite green, independently
verified value-exact by sweep `wqfneq70y`). Per-finding status in `quality-audit-findings.md`.

**Pass D surfaced + fixed 4 NEW float-in-consensus findings (DEF10-14) the 80-finding audit missed** (same
class as DEF8/DEF9), committed `e2b6e3a`. User directive: *"whatever we need to make it
deterministic and suitable, like fixed point math."*
- **DEF10** `deconstruction_return_percent` (50, integer) → `uint32` (no code change; reader already uint32).
- **DEF11** `prestige_tournament_performance_mod` (0.5, DEAD) → removed (`reserved 34`).
- **DEF12/13/14** `alms` 0.15 / `exchange_sale_percentage` 0.96 (**money**) / `ReductionPercent` 0.8/0.65/0.5
  → proto field changed to `int32` storing the fpm `fixed_24_8` **raw** value (38 / 246 / 205,166,128 —
  computed empirically in-container), read via `fpm::from_raw_value()` (or directly as int64 for the
  exchange `.raw_value()` consumer). **Zero float in the consensus path; golden byte-identical** (the old
  float ctor already quantized to these exact raws). Config carries `# raw/256 = <decimal>` comments.
  Basis-points alternative rejected (96/100 ≠ 246/256 → would shift money).

**ALL of Pass D done. Suite green (golden byte-identical + 98 unit + 4 reorg + 2 reorg-game + roconfig +
database).**

**Taurion removal DONE** `55e355f` (user: "no relation to taurion") — every Taurion mention stripped
repo-wide (8 copyright headers → "tf", 2 comments reworded, audit-doc "Taurion-baggage" category →
"fork-baggage"); comment-only, golden byte-identical.

**Pass B2 DONE** — `f0940de` (FN11 combat lmt/rmt honest rename + drop "0____0" comment; FN33 delete dead
`Params` class incl. context member/accessor + Makefile + autoreconf; FN50 drop `QuantityProduct` and the
whole GMP dependency — configure.ac + database/Makefile.am; dead `pending.hpp` onChainPlayerFighterData
member) and `52181f0` (FN61 merge the two RecipeInstance create-ctors via a `PopulateRecipeCommon`
helper — both sources are `CraftedRecipe`). All golden byte-identical, full suite green.

**NEXT = Pass F (the only remaining pass)** — split the 4154-line moveprocessor.cpp + 1682-line logic.cpp
into cohesive TUs by domain (logic: core/state · resolve · combat · tournament; moveprocessor: core ·
purchase · cooking · expedition · tournament · exchange · fighter), each verified golden byte-identical.
Pure code movement: distribute class member-method definitions across new .cpp files all including the same
header; relocate the 4 shared moveprocessor free helpers (AsciiToLower/DedupInPlace/SortPairsByKey/
FindApplicableGoody) to a small internal header. Each new .cpp → src/Makefile.am + autoreconf. No proto
change, so fast incremental `make -C src` builds (not the slow full rebuild). User chose "B2 first, then F".
**Heed the BUILD GOTCHA in the progress log before any proto/proto2/*.pb.text edit. Pass C's activities
teardown needed autoreconf -i + ./config.status (Makefile.am changes) -- D/F may too.**

**User decisions (this session):**
1. **[DONE `bf5271b`] FN1 reward-roll** = *"Fix + you re-tune for me"* — apply strict `<` at all 5 roll sites + FN2
   (fpm→int), then **adjust the weight tables in `proto/roconfig/activityrewardstable.pb.text` so
   observed drop rates stay ~the same as today**, and **get user sign-off on the target rates** before
   committing. Then regen golden + recalibrate the 4 RNG `ValidateStateTests`
   (GeneratedRecipeMakeSureItWorks, SweetenerRandomRewardConsistency,
   ClaimRewardsTestAllRewardTypesBeingAwardedProperly, ClaimRewardsInvalidParams).
   FN1 analysis so far: reward table file has 1086 `Rewards:` entries; MANY tables end with a
   `Weight: 1` last entry (currently UNREACHABLE under `<=`) — these need a sign-off decision (make
   reachable vs keep starved). Effect of `<=`→`<`: first bucket loses its +1 over-weight, last bucket
   gains its missing share. To preserve observed rates exactly per table: new_w[first]=old+1,
   new_w[last]=old−1 (but that perpetuates the weight-1-unreachable pathology → flag those tables).
2. **Remaining passes** = *"Continue all now"* → after FN1: **Pass C** (dead proto/config fields:
   FN34/62/63/64/65/66/67/68, incl. removing the dead Activity message+table), **Pass D** (determinism:
   DEF8/DEF9 float→int proto probabilities), **Pass F** (split the 4157-line moveprocessor.cpp +
   logic.cpp into cohesive TUs, each golden byte-identical), **Pass B2** (FN11 rename, FN33 delete dead
   Params class [context.* + Makefile + autoreconf], FN50 drop QuantityProduct/GMP dep, FN61 merge
   recipe ctors, pending.hpp dead member, repo-wide legacy copyright headers ×7).

**Build/test loop (resume):** `tfdev` container holds the source at `/usr/src/tfgsp` (plain copy). Edit
host → `docker cp src/F tfdev:/usr/src/tfgsp/src/F` → `docker exec tfdev touch …` → `make -C database`
and/or `make -C src goldenreplay_tests tests reorg_tests reorg_game_tests` → run from
`/usr/src/tfgsp/src`. Golden = `./goldenreplay_tests` (must PASS byte-identical for neutral changes;
`GOLDEN_REGEN=1 ./goldenreplay_tests` to regen, then `docker cp` the new golden back, run TWICE for
determinism, diff entity counts + no dangling refs). Proto/Makefile/schema changes need
`autoreconf -i && ./config.status` (+ delete generated schema.cpp / roconfig.pb so they regen). Full
detail of every finding is in the workflow output `wgm9hkdph` (scratchpad `audit-full.md`).

## Progress log
<!-- Pass B (1947cd0): 43 dead-code/DRY findings, golden byte-identical, 98 unit + 4 reorg + 2
     reorg-game. Orchestrated via 4 disjoint-file subagents + direct edits. The moveprocessor agent
     died mid-run (API overload) leaving 2 incomplete cross-file edits (CalculateFuelPower header decl;
     pending.cpp Parse* callers for the dropped name param) — both completed by hand; build+golden gate
     caught and validated everything. B2 deferred: FN11, FN33, FN50, FN61 + pending.hpp dead member +
     repo-wide legacy copyright headers (7 files). -->


- 2026-06-29: audit run; tracking doc created; baseline captured; `gametest/` stale `__pycache__`
  removed (untracked).
- 2026-06-29: **Pass A1 DONE** (`0c02348`): FN10,13/69,40,46,54,55,56,70,71 — golden-neutral
  correctness & money-path hardening. Golden byte-identical, 98 unit + 4 reorg + 2 reorg-game green.
  FN2 reassigned to A2 (shares the reward-roll lines with FN1).
- User directive added mid-pass: *"dry, compact, elegant, well-structured; avoid gigantic monolithic
  files."* → added **Pass F (structural)**: after the dead-code cull (Pass B shrinks them), split the
  monoliths `moveprocessor.cpp` (4274 LOC) and `logic.cpp` (1717 LOC) into cohesive translation units
  by domain (cooking / expedition / tournament / exchange / transfigure / crystal), each split verified
  golden byte-identical. Done last because consensus-file splits are the highest-churn change.
- 2026-06-29: **Pass A2 DONE** (`f6ffde4`): FN14/25/26 — transfigure fuel per-rarity cost, armor reward
  by-value→reference, demand-queue double-append. Golden byte-identical (golden scenario doesn't
  traverse these paths), 98 unit + 4 reorg + 2 reorg-game green.
  **FN1+FN2 DEFERRED — needs a user balance decision.** [resolved below]
- 2026-06-29: **FN1+FN2 DONE** (`bf5271b`), user signed off on *config-as-authored* (no weight re-tune).
  Strict `<` at all 5 roll sites + fpm->int at the 2 expedition/tournament sites. Measured impact: the
  fix only moves the first (-1/total) and last (+1/total) bucket of each table; **max change to any
  reward is 1.54%**, and it un-strands exactly one Rare-armor recipe (0% -> 0.93%). **Golden stayed
  byte-identical** (the replay scenario's rolls land on the same buckets), so FN1 was golden-NEUTRAL, not
  regen. Re-pinned `SweetenerRandomRewardConsistency` 2063 -> 2046.
  - **Surfaced a 2nd real bug, FN72 (config-loader self-merge).** The all-zero `UnitTest_Reward` regtest
    fixture relied on the old `<=` to select bucket 0; giving it `Weight: 1` should have fixed the 3
    expedition tests but the weight kept reading 0 at runtime. Root cause: `RoConfig` merged the overlay
    with `pb.MergeFrom(pb.regtest_merge())` — **source aliasing destination**, undefined in protobuf,
    silently dropping merged scalar Weights. Invisible for years only because every overlay value was 0.
    Fixed by copying the sub-message out first. No-op on mainnet/Polygon (no overlay merged there).
- 2026-06-29: **Pass C DONE** — `183f13a` (FN62 dead Params.prestige_total_treats_mod, FN64 dead
  OngoingOperation.ItemDatabaseID, FN67 widen FighterSaleEntry.Price uint32->uint64, FN68 legacy
  comments; all golden byte-identical), `419dd57` (FN65 MaxRewardQuality removed from both blueprint
  protos + tournament.cpp + 43 config entries + roconfig validation, FN66 Deconstruction.DeconstructionId;
  golden byte-identical -- the JSON dump never surfaced these), `84c56f1` (FN34+FN63 full removal of the
  dead `activities` table/proto/ActivityTable/gamestatejson emitter; needed autoreconf for 3 Makefile.am
  edits + schema regen; golden regenerated, diff is exactly one line `- "activities" : []`, deterministic).
  **All 71 fix-now findings + FN72 now DONE.**
  - **BUILD GOTCHA (cost ~30 min — READ BEFORE TOUCHING proto/ or proto2/):** `src/libtf.la` *bundles*
    proto2's `roconfig.o` (libtool convenience-lib), and the dependency is **not tracked** — editing
    `proto/roconfig.cpp` or a `*.pb.text` and running incremental `make` (even `make clean` in `src` or
    `proto2` alone, even `rm libtf.la`) keeps linking a **stale `roconfig.o`** into `tests`. Symptom:
    runtime config != the freshly-built `roconfig.pb`/blob. The ONLY reliable fix is a full clean rebuild
    in dependency order: `make clean` (top) -> `make -C proto` -> `make -C proto2` -> `make -C database &&
    make -C database libdbtest.la` -> `make -C src <targets>`. Verify with
    `strings src/tests | grep -c <a-unique-string-from-your-edit>`.

- 2026-06-29: **Pass D DEF8/DEF9 DONE** (`11ee008`): `FighterName.Probability` (field 5),
  `FighterType.Probability` (field 3), `FighterTypeMoveProbability.Probability` (field 2) changed
  `float`→`uint32`; `fightertype.pb.text` `Probability: N.0`→`N`; recipe.cpp's six `*1000` sites given
  explicit `(int32_t)` casts (pure signed-int math). All config probability values are exact integers
  (FighterName 1..1000, FighterType 20, moves 15/20/30), so every `NextInt` bound/comparison is identical
  → **golden byte-identical**, 98 unit + 4 reorg + 2 reorg-game + roconfig + database all green. Required
  the full clean rebuild (proto change; heed BUILD GOTCHA). No `set_probability` writer exists anywhere
  (config-read-only), and roconfig_tests only check `has_probability()` → uint32-safe.
  - **Ran a 4-lens read-only determinism completeness sweep** (workflow `wqfneq70y`) alongside: it
    independently re-verified DEF8/DEF9 value-exactness AND found **4 more float-in-consensus proto fields
    the audit missed** — DEF10 `deconstruction_return_percent` (int 50, fix-now), DEF11
    `prestige_tournament_performance_mod` (dead, remove), DEF12 `alms` 0.15, DEF13
    `exchange_sale_percentage` 0.96 (money), DEF14 `ReductionPercent` 0.8/0.65/0.5. The fractional three
    are deterministic in practice (power-of-2 fpm scaling) but flagged for a representation decision — see
    findings doc "Pass-D determinism sweep". DEF10/11 are golden-neutral; will batch with the DEF12-14
    decision into one rebuild.
- 2026-06-29: **Pass D DEF10-14 DONE** (`e2b6e3a`): user said *"whatever we need to make it
  deterministic and suitable, like fixed point math."* → removed ALL remaining float/double from the
  consensus path. `deconstruction_return_percent` float→`uint32` (50, exact); dead
  `prestige_tournament_performance_mod` removed (`reserved 34`); `alms`/`exchange_sale_percentage`/goody
  `ReductionPercent` float/double→`int32` storing the fpm `fixed_24_8` **raw** value, rebuilt via
  `fpm::from_raw_value()` (exchange reads the raw int64 directly, since its consumer already took
  `.raw_value()`). Raw integers (38, 246, 205/166/128) computed empirically in-container with the actual
  fpm header — the old float ctor already quantized to exactly these, so **golden byte-identical** and
  every payout/rating/duration is unchanged. Full clean rebuild (proto change); inline `# raw/256 =`
  comments in the config parse fine. No other readers/writers/overlays touch these fields. Suite green.

<!-- STATUS: per-finding status tracked in the table at docs/quality-audit-findings.md -->


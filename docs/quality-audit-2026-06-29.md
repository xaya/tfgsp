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
| **B**  | Dead-code / Taurion-baggage removal + safe DRY | neutral | FN3–9, FN11–12, FN15–24, FN27, FN29–33, FN35–39, FN41–53, FN57–61 |
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

## Progress log
<!-- Pass B (1947cd0): 43 dead-code/DRY findings, golden byte-identical, 98 unit + 4 reorg + 2
     reorg-game. Orchestrated via 4 disjoint-file subagents + direct edits. The moveprocessor agent
     died mid-run (API overload) leaving 2 incomplete cross-file edits (CalculateFuelPower header decl;
     pending.cpp Parse* callers for the dropped name param) — both completed by hand; build+golden gate
     caught and validated everything. B2 deferred: FN11, FN33, FN50, FN61 + pending.hpp dead member +
     repo-wide Taurion copyright headers (7 files). -->


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
  **FN1+FN2 DEFERRED — needs a user balance decision.** The weighted reward roll uses `<=` (should be
  `<`): the first bucket is over-weighted by 1 and the last bucket is unreachable when its weight==1.
  The fix is correct, but it shifts reward distribution across expeditions/sweeteners/tournaments and
  the configured weight tables in `proto/roconfig/*.pb.text` may have been authored against the current
  (biased) behavior. It also requires recalibrating 4 RNG `ValidateStateTests` (one structurally — the
  corrected roll no longer yields a generated recipe for the test's fixed seed). **Decision needed:** fix
  the roll + re-tune the weight tables to preserve intended drop rates, or leave as-is. Until then, the
  `<=` behavior is unchanged.

<!-- STATUS: per-finding status tracked in the table at docs/quality-audit-findings.md -->


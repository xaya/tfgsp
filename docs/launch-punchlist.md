# Treatfighter — Launch punch-list

Single source of truth for what remains before mainnet launch on Polygon. Built from the
2026-06-30 doc+code survey. Tiers by severity; **who** = needs the owner (external input /
launch-time) vs implementable now. Companion docs: `original-vs-rewrite.md` (perf/issues),
`security-audit.md` (move-reachability audit; §11 = launch re-verify).

## 🔴 Launch-blockers (must happen before mainnet)

1. **Re-pin POLYGON genesis height** — `src/logic.cpp:160`. Still the placeholder tip
   `height = 89'246'000`, `hashHex = ""` (chosen ~2026-06-27). **Consensus-critical:** set to
   the real Polygon tip at actual launch time; never change after. Owner gives tip → 1-line
   edit + golden regen. (For the live-Polygon *test* gameid, bumping to the current tip is
   optional — only ~3 days / ~130k blocks of backfill from the current value.)

2. **dev_address** — `proto/roconfig/params.pb.text:5`. Routes every WCHI bundle / dev
   crystal payment (`moveprocessor.cpp:111`).
   - **DECIDED (owner 2026-06-30): use `0x25763F408461ddc66d8955d22963815DEA6d8722`** for the
     live-Polygon TEST gameid. This is **NOT the final foundation address** — a separate real
     gameid + real address will be set later for the production launch.
   - ✅ **DONE (2026-07-01):** replaced TEMP `0x59F5…BF95` → `0x2576…8722`. Rebuilt the
     roconfig blob; **golden byte-identical** (dev_address does not reach the golden replay
     state), full `make check` green. All tests read `dev_address` dynamically, so no test/
     fixture edits were needed. The **production** address remains TODO for the real gameid.

3. **Live Polygon + XayaX end-to-end test** — `polygon-test/integration.py` only runs vs a
   FORKED (Anvil) Polygon, manually. Need a real-chain sync + move round-trip on live Polygon
   before sign-off. Owner: "we will make it live, just test it on live polygon anyway."

## 🟡 Recommended hardening (implementable now)

4. ✅ **DONE (2026-07-01) — roconfig-blob pin test.** Added `RoConfigTests.LaunchConfigIsPinned`
   (`proto2/roconfig_tests.cpp`): deterministic-serialises `RoConfig(MAIN)` + SHA256, pins the
   hash (`dd48e3fa…78ba1`), and asserts `god_mode==false`, no testnet/regtest merge, and no
   `zzzz`/`xxxx`/`UnitTest` leakage. Runs under `make check`. Closes the C9 sign-off.
   **This test immediately caught a latent consensus bug** — see the blob-race fix below.

5. ✅ **DONE (already fixed) — hardcoded name-gift removed in commit `3a3c46c`** ("SB-06: remove
   hardcoded early-access free-recipe gift", ancestor of HEAD). `MaybeInitXayaPlayer` no longer
   gifts recipes to `DonRinkula`/`PandaBoss`/`"1 John"`; grep-verified zero references in code/
   tests. Golden byte-identical (the three names aren't in the golden fixture). Distinct from
   SB-06 starter-mint (kept).

6. ✅ **DONE (2026-07-01) — roconfig blob parallel-build race (consensus-critical).** Surfaced by
   #4 under `make -j`: the multi-target rule `roconfig.pb roconfig.pb.text:` made GNU make run
   `roconfig_gen` **twice concurrently** (once per requested target), two processes writing the
   same 4.5 MB `roconfig.pb` → a truncated/corrupt blob embedded via `.incbin`. At runtime this
   aborts the daemon (`CHECK(ParseFromArray)` — the "lucky" case) or could embed a valid-but-
   different config → **silent fork**. Fixed in `proto2/Makefile.am` (single-output rule +
   `roconfig.pb.text` alias + explicit `roconfig_blob.lo: roconfig.pb` `.incbin` edge) and made
   the blob **deterministic/byte-reproducible** (`roconfig_gen` now uses
   `SetSerializationDeterministic`). Validated: 15× clean parallel rebuilds all parse, blob md5
   constant. See `original-vs-rewrite.md` §"Issues found & fixed".

7. ✅ **DONE (2026-07-01) — tournament roster `fc` DoS cap (DOS-FC).** The sign-off re-verification
   audit found `fc` (`ParseTournamentEntryData`, `moveprocessor_activity.cpp`) was the **lone**
   user-supplied id-array with no `MAX_MOVE_ARRAY` guard: the O(n²) `HasDuplicates` dedup + the
   per-entry `fighters.GetById` loop ran **before** the `size()!=teamsize()` reject and before the
   joincost payment gate, so one cheap move carrying a huge `fc` forces super-linear per-block work
   on every honest node (deterministic near-halt — **not** a fork). Fixed (`0bdab5e`) by adding the
   cap right after the `isArray()` check, mirroring the expedition `fid` sibling; extended the
   Pass-B regression test (`OversizedMoveArraysAreCappedNotFatal`) with a huge-`fc` entry move.
   Golden byte-identical; full `make check` + determinism lint green.

## 🟢 Done — re-verify at sign-off (launch-day confirmation, not code work)

**Code-verifiable portions RE-VERIFIED 2026-07-01** (adversarial 6-dimension audit, each finding
confirmed/refuted by 3 independent lenses — determinism / exploitability / fork-divergence):
- **F2/F3 — CLEAN.** Consensus state is integer / `fpm::fixed_24_8` fixed-point only; the
  determinism lint (`contrib/check-consensus-determinism.py`) runs under `make check` and its
  exclusion list hides no hashed-float path; `-ffp-contract=off` is applied globally via
  `configure.ac` `CXXFLAGS` (inherited by every consensus TU and any downstream `./configure`
  builder). Low-risk note (not a bug): a handful of `fpm::fixed_24_8(0.1|0.5|0.6|1.25|100.0)`
  literals sit in consensus code — invisible to the token-based lint, but provably deterministic
  (×256 is exact for the exact-representable ones; `0.1`/`0.6` land at `25.6`/`153.6`, far from a
  `.5` round boundary, so `round()` collapses any literal-rounding ambiguity) and frozen in the
  pinned golden. Left as readable literals rather than churned to `from_raw_value()`.
- **C7 — CLEAN.** The crownholder mechanic is fully removed (`crown-removal.md`); reroll name
  selection is deterministic (proto map copied to a vector and sorted by unique key — no
  iteration-order dependence, all randomness via seeded `xaya::Random`), and the empty/edge state
  early-returns safely. The former chain-halt is fixed by the clamp at **`database/fighter.cpp:299`**
  (`increasedProbability>1000 → 1000`, guaranteeing `NextInt(≥1)`). *(Doc-drift fix: earlier notes
  cited line 284; 284 is `probabilityIndex`, the clamp is 299.)*
- **C9 — CLEAN.** No reachable unbounded/unsinked mint: crystal mint is WCHI-gated (CRYS-1/C8),
  all candy/reward credit funnels through the H5 `max_unclaimed_reward_amount` cap, and the
  UnitTest faucet lives only in `regtest_merge{}` which MAIN strips — the `LaunchConfigIsPinned`
  pin test asserts no `zzzz/xxxx/UnitTest` survive in the assembled MAIN blob.
- **F1/REORG-01 — CLEAN.** `pending.cpp` performs **zero** consensus-state DB writes (every `Parse*`
  reachable from pending is read-only; all mutators live only on the confirmed `ProcessOne` path);
  undo is automatic via libxayagame's SQLite session (no manual backward override); no
  non-reproducible clock/random/height on the consensus path (per-block RNG reseeded from block hash).
- **Integer overflow (OVF-01 siblings) — CLEAN.** `isInt()` bounds every `asInt()` to int32; OVF-01
  (`MAX_TRANSFIGURE_CANDY`) and EXCH-1 (`CHECK_LE(pctRaw,256)`) closed; unsigned-balance deductions
  all pre-checked. One low-risk defensive note: `fuelPower *100/*1.25` narrowing is bounded by the
  48-inventory cap (defined, node-identical wrap — no fork).

**Still needs a live/launch-day confirmation (cannot be done from code alone):**

- All of the above are now **code-confirmed clean** (2026-07-01) — no remaining code sign-off work.
  The only launch-day confirmations left are the operational ones already tracked as 🔴 blockers
  above: a live Polygon + XayaX end-to-end sync / move round-trip (which also exercises a runtime
  reorg on the live chain, complementing `reorg_game_tests`), run against the re-pinned genesis height.

## ⚪ Economy / product sign-offs (deterministic, owner's call — not chain bugs)

- **SB-06** free starter mint — DECIDED: keep (account-bound, not exploitable).
- **Hardcoded name-gift** (#5) — ✅ already removed (`3a3c46c`).
- **Non-zero tournament JoinCost** — needs audit before enabling (all blueprints 0 today).
- **Reward-cap UX polish** — parked: entry-time "at cap" guard + a
  `max_unclaimed_reward_amount != 0` assertion (cap itself is DB-safe/deterministic/bounded).

## 🧹 Code hygiene / DRY (non-blocking tech-debt — clean/compact/DRY directive)

✅ **DONE (2026-07-01)** — the safe, non-consensus trims (comment/test/dead-code only; golden
byte-identical, full `make check` green):
- `src/logic_tests.cpp:309,344` — test magic-`15` now reads `params().common_recipe_cook_cost()`
  (the real config field; same value, so no behaviour change).
- `src/logic_combat.cpp:171` — dropped the stale "For now set to 15 for everyone" comment (the
  K-factor already reads `params().elok_factor()`; it's ELO, not combat duration).
- `src/logic_resolve.cpp:603` — trimmed the resolved dead-code `DeleteById` block to a one-line
  note (recipe kept because the cooked fighter references its id).
- `src/moveprocessor_activity.cpp:717` — trimmed the trailing `//TODO` comment (live
  `set_animationid` statement untouched).
- `database/xayaplayer_tests.cpp:228` — removed the fully-commented-out empty `Prestige` test.
- `src/pending.cpp:625` — removed the parked commented transfigure-diff block (dead path:
  `pending_moves=false` on Polygon).

⏭️ **Skipped (recon found these mischaracterised / unsafe):**
- `database/fighter_tests.cpp:33` `TestRandomS` — **NOT a clean dedup**: it seeds `"test seed 1"`
  while the shared `testutils` `TestRandom` seeds `"test seed"`; switching would change the RNG
  seed and shift fighter-stat expectations. Also kept separate to avoid a `database/`→`src/`
  link dependency. Left as-is.
- `src/moveprocessor.hpp:278` `authID` — **nothing removable**: it's a used parameter of
  `GetCandyKeyNameFromID` (consumed in `moveprocessor_cooking.cpp`), not a redundant member.

## ✅ Confirmed clean (no action)

- Taurion fully stripped (only doc mentions remain).
- Alchemy RPC key NOT committed (lives in a gitignored `.env` outside the repo; `.gitignore`
  now also blocks `.env`/keys — `d08d573`).
- regtest `xxxx/zzzz` ids are intentional unit-test fixtures.

## Build gotcha

Any roconfig VALUE change (#1, #2, #5) ⇒ full clean rebuild + golden regen (acceptable
pre-launch). Batch them into one regen where possible. Always run the full suite afterward
(golden byte-identical + unit + reorg + reorg-game + database).

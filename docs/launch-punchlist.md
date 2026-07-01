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

## 🟢 Done — re-verify at sign-off (launch-day confirmation, not code work)

- **C7** — confirm a valid-address crownholder exists at fresh-launch heights (else reroll
  returns early; clamp at `database/fighter.cpp:284`).
- **C9** — run the UnitTest expedition farm to confirm the faucet is closed (depends on #4).
- **F1/REORG-01** — human/runtime reorg + addressless-crownholder test (regression test
  covers the queue-data write path; confirm no other Parse\* path writes during pending).
- **F2/F3** — confirm the determinism lint exclusion list hides no hashed float path + that
  third-party builders inherit `-ffp-contract=off`.
- **security-audit §11** move-reachability re-verify.

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

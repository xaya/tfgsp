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
   - Action: replace TEMP `0x59F5f46dC284e3414F67EDa31eE90126bC48BF95` → `0x2576…`. Config
     value change ⇒ rebuild roconfig blob + golden regen (full clean rebuild).

3. **Live Polygon + XayaX end-to-end test** — `polygon-test/integration.py` only runs vs a
   FORKED (Anvil) Polygon, manually. Need a real-chain sync + move round-trip on live Polygon
   before sign-off. Owner: "we will make it live, just test it on live polygon anyway."

## 🟡 Recommended hardening (implementable now)

4. **CI roconfig-blob hash assertion** — no CI pipeline exists (only `make check` at docker
   build). Add a test that hashes the assembled roconfig blob and pins it; assert
   `god_mode=false` + no `zzzz/xxxx`/UnitTest ids. Closes the C9 sign-off.

5. **Remove the hardcoded name-gift** — `moveprocessor.cpp:5068` gifts fighters/recipes to
   `DonRinkula`/`PandaBoss`/`"1 John"`. On a fresh relaunch a name-squatter who registers
   those `p/` names claims it. Distinct from SB-06 (kept). Economy change + golden regen.

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
- **Hardcoded name-gift** (#5) — recommend remove.
- **Non-zero tournament JoinCost** — needs audit before enabling (all blueprints 0 today).
- **Reward-cap UX polish** — parked: entry-time "at cap" guard + a
  `max_unclaimed_reward_amount != 0` assertion (cap itself is DB-safe/deterministic/bounded).

## 🧹 Code hygiene / DRY (non-blocking tech-debt — clean/compact/DRY directive)

- `database/fighter_tests.cpp:33` — copy-pasted `TestRandomS`; dedup vs `testutils.hpp`.
- `src/moveprocessor.hpp:278` — remove redundant `authID`.
- `src/logic_combat.cpp:171` (+ `logic_tests.cpp:309,344`) — combat duration hardcoded `15`;
  read from config.
- `src/logic_resolve.cpp:603` — trim the stale dead-code `DeleteById` comment to the
  resolved decision.
- `src/moveprocessor_activity.cpp:717` — `animationid` as a repeated list (TODO).
- `database/xayaplayer_tests.cpp:235` — commented-out prestige assertion.
- `src/pending.cpp:625` — transfigure-diff refactor; parked (`pending_moves=false` on Polygon
  ⇒ `PendingStateUpdater::ProcessMove` is a dead path).

## ✅ Confirmed clean (no action)

- Taurion fully stripped (only doc mentions remain).
- Alchemy RPC key NOT committed (lives in a gitignored `.env` outside the repo; `.gitignore`
  now also blocks `.env`/keys — `d08d573`).
- regtest `xxxx/zzzz` ids are intentional unit-test fixtures.

## Build gotcha

Any roconfig VALUE change (#1, #2, #5) ⇒ full clean rebuild + golden regen (acceptable
pre-launch). Batch them into one regen where possible. Always run the full suite afterward
(golden byte-identical + unit + reorg + reorg-game + database).

# Treatfighter GSP — Security / DoS audit (wvwh9cxkd, 38 agents, 2026-06-28)

> Adversarially-verified move-reachability audit of the consensus pipeline. CRITICAL: chain not launch-safe.
> Source of truth for the security hardening passes. Each finding traced to an untrusted move on Polygon.

# Treatfighter GSP (Polygon) — Security Synthesis Report

**Scope:** consensus move pipeline (`src/moveprocessor.cpp`, `src/logic.cpp`, `src/pending.cpp`, `database/*`). **Deployment:** fresh relaunch as `Chain::POLYGON`, permissionless near-free moves. **Method:** synthesis of six audit dimensions plus adversarial reachability verdicts.

---

## 1. Bottom line

**The chain is NOT safe to launch.** There are **8 confirmed, move-reachable, chain-fatal or economy-fatal vectors** on Polygon, each triggerable by a single near-free permissionless move from any registered Xaya name:

- **6 confirmed chain-halt crashes** — one of them (`MPV-1`, a top-level array move like `[0]`) fires *before every gameplay/height gate*, so it halts the chain at any height with no game state; three more are unguarded `asInt()`/`asString()` calls on move array elements; one is an out-of-bounds `stoi` on a sweetener string; one aborts via `Random::NextInt(0)`.
- **2 confirmed economic exploits** — unlimited crystal minting for one payment (`ECON-01`), and a dead `chain==MAIN` guard that leaves a free repeatable recipe/fighter faucet (`UnitTest_Expedition`) live on Polygon (`SB-BD-01`).

On top of that there are several confirmed **DoS/bloat** vectors (O(N²) dedup loops with no array cap, per-block full-table scans, never-deleted rows), one confirmed **reorg/fork** defect (pending processing writes to the confirmed DB), and latent **non-determinism** (raw `double` in live prestige/sweetness math). The headline "backdoor" leads (`MaybeSQLTestInjection`, god-mode, the `log10` prestige path, the `&&`-vs-`||` bug at 4596) are all **confirmed regtest/height/config-gated and NOT reachable on Polygon** — but should still be deleted.

Confidence on the crash and economic findings is **high** (most empirically reproduced against the installed jsoncpp 1.9.5). Two items (`CRASH-01` reroll preconditions, `REORG-01` end-to-end) need a runtime check, noted below.

---

## 2. CRITICAL — fix before launch

All rows below are `confirmed_exploitable`, reachable on Polygon, single permissionless move. Ordered by ease/severity (cheapest + most certain first).

| # | Vector | Location | Trigger move | Impact | Fix |
|---|--------|----------|--------------|--------|-----|
| C1 | **Top-level array sub-move with scalar element** (`mrl["a"]` on a non-object throws `Json::LogicError`) | `moveprocessor.cpp:1278-1288`, throw at `1319`; pending mirror `pending.cpp:856-862` | `{"g":{"tf":[0]}}` (also `["x"]`, `[[]]`, `[true]`) | **chain-halt**, *at any height* (runs before launch gate at 1325) | In `ProcessOne` and `PendingStateUpdater::ProcessMove`, `if(!mrl.isObject()) continue;` at top of per-sub-move loop; reject non-object array elements in `ExtractMoveBasics`. |
| C2 | **`exp.f.fid` array element → `asInt()`** no per-element `isInt()` | `moveprocessor.cpp:2694-2696`; pending `pending.cpp:936` | `{"exp":{"f":{"eid":"x","fid":["a"]}}}` (or `[{}]`,`[[]]`,`[3000000000]`) | **chain-halt** | `for(auto ft: expedition["fid"]) { if(!ft.isInt()) return false; ... }` |
| C3 | **`ca.d.rid` array element → `asInt()`** no per-element `isInt()` | `moveprocessor.cpp:3318-3322`; pending `pending.cpp:908` | `{"ca":{"d":{"rid":["a"]}}}` | **chain-halt** | `if(!ft.isInt()) return false;` first in the `rid` loop. |
| C4 | **`exp.c.eid` array element → `asString()`** no per-element `isString()` | `moveprocessor.cpp:1448-1450`; pending `pending.cpp:943` | `{"exp":{"c":{"eid":[{}]}}}` or `eid:[[1]]` (must be object/array element) | **chain-halt** | `if(!eID.isString()) return false;` in the `eid` loop. |
| C5 | **Sweetener `ps` trailing comma → OOB `result[1]` → `std::stoi`** (UB; SEGV not caught by `catch(...)`) | `moveprocessor.cpp:312-324`; pending `pending.cpp:1051` | `{"ps":","}` or `{"ps":"x,"}` | **chain-halt** (or fork: divergent `total` from garbage heap read) | Check `result.size() >= 2` before indexing; validate count is a positive int. |
| C6 | **Pending `ParseCoinTransferBurn` `CHECK(isObject())` on scalar array element** → `LOG(FATAL)`/`abort()` crash-loop | `moveprocessor.cpp:1033-1037`, via `pending.cpp:882` | `{"g":{"tf":[0]}}` in mempool (account must already exist) | **chain-halt** (pending DoS / crash-loop; same move halts consensus via C1) | Replace `CHECK` with `if(!moveObj.isObject()) return false;` + fix C1 root cause. |
| C7 | **Name-reroll with large CHI → `increasedProbability==1001` → `Random::NextInt(0)` `CHECK_GT(n,0)` abort** (and infinite loop for larger payments) | `database/fighter.cpp:284-298`; `xayautil/random.cpp:138`; cost path `moveprocessor.cpp:511-519,694-718` | `{"nr":<ownedCommonFighterId>}` with `out` paying crownholders ~1738.48–1739.48 CHI total | **chain-halt** | Clamp `increasedProbability` to `[0,1000]` and/or cap reroll `cost` to `lookUpPrices.back()` before `NextInt(1001-increasedProbability)`. |
| C8 | **Crystal bundle replay** — `paidToCrownHolders` never decremented (`fraction=paidToDev/35` computed *before* the sum = 0; `cost%35==0` for every bundle), and `move` array re-runs `TryCrystalPurchase` N times sharing one budget | `moveprocessor.cpp:816-912` (bug 820-821, 896-909), array dispatch `1276-1288`, call `1333` | `move:[{"pc":"T1"},{"pc":"T1"},…×N]` + one `out` payment for ONE bundle | **economic-exploit** (mint N×crystals for ~0.14 CHI; core currency, irreversible inflation) | Track one mutable remaining-budget and subtract `cost` per success; move/fix the `fraction` computation to after the sum; cap/dedupe repeated paid commands per move. |
| C9 | **`chain==xaya::Chain::MAIN` dead-guard exposes test content on Polygon** → `UnitTest_Expedition` (Duration 1, all reward weights 0 → always GeneratedRecipe q1) is a free, repeatable recipe/fighter faucet | `logic.cpp:481`; `moveprocessor.cpp:2757,1617,2652`; config in base mainnet roconfig (`proto/Makefile.am:85-86`) | `{"exp":{"f":{"eid":"00000000-0000-0000-zzzz-zzzzzzzzzzzz","fid":<ownedFighterId>}}}`, then `{"exp":{"c":{"eid":[…]}}}`, repeat | **economic-exploit** + permanent `recepies` bloat | Change every `chain == MAIN` test guard to also cover POLYGON (e.g. `!= REGTEST`), **and** strip `UnitTest_*`/`*zzzz*`/tutorial blueprints+reward-tables out of the base mainnet config into the regtest merge. |

**Notes / residual checks for CRITICAL:**
- C2–C4 are *one instance each of the same pervasive class*: unguarded `as*()` on attacker-controlled move array/object elements. **Sweep every `Parse*` in `moveprocessor.cpp` and `pending.cpp`** for `asInt()/asUInt()/asInt64()/asString()` on a move-derived `Json::Value` without an `is*()` guard before fixing only the four named sites. The tournament-roster dedup parsers (`2364`, `2558`, `2701`) *do* guard element type (1978/2002/2026-2042) — confirm each as you sweep.
- **Belt-and-braces:** wrap `MoveProcessor::ProcessOne` (and `PendingStateUpdater::ProcessMove`) in a `try/catch(std::exception)` that **deterministically skips** the offending move. This neutralizes the entire jsoncpp-throw class at once (any missed `as*()` site no longer halts the chain). The catch must behave identically on every node (skip the whole sub-move) to avoid converting a halt into a fork.
- **C7 needs a runtime check:** the crash requires a special-tournament crownholder with a valid address (so `smallestPayFraction != 0`) and a Common fighter with position0/position1 names. The seeded crownholder `ryumaster`/`xayatf*` makes this true in steady state, but confirm the path isn't short-circuited at the very first blocks of a fresh launch. Cost to attacker is real (~1738 CHI WCHI), but impact is permanent.
- **C9 confirmed by complete static trace; not yet run end-to-end.** Before signing off, run the UnitTest expedition farm on a regtest/staging Polygon config to confirm (a) all `UnitTest_Reward` weights are 0 so reward index 0 is always selected, (b) the cooked fighter is exchange-sellable.

---

## 3. HIGH — DoS / slowdown / bloat

All `reachable_needs_fix`, deterministic (no crash/fork) but degrade or stall block processing / bloat the DB. None are gated on Polygon.

| # | Vector | Location | Spam move | Effect | Cap / fix |
|---|--------|----------|-----------|--------|-----------|
| H1 | **O(N²) dedup of move-supplied arrays** in `ParseTransfigureData` (runs before any ownership check) | `moveprocessor.cpp:1986-1991, 2010-2015, 2047-2052`; pending `1025` | `{"f":{"t":{"fid":0,"o":1,"if":[0,1,…,N-1],"ic":[],"ir":[]}}}` | Gas-bounded N (~150k–250k in a block-filling tx) → ~2.6s+ per move at `-O2` (benchmarked), >Polygon ~2s block time; pending-broadcast amplifies. Linear gas vs quadratic GSP work → sustained slowdown DoS. | Hard size cap on `if`/`ic`/`ir` (match real game limits) **before** building vectors; replace `std::remove`-loop with `sort+unique`/`unordered_set` (O(N log N)). Mirror in `pending.cpp`. |
| H2 | **Same uncapped O(N²) dedup** in `ParseTournamentEntryData` & `ParseExpeditionData` (size check happens *after* dedup) | `moveprocessor.cpp:2558-2567` (check at 2644), `2701-2710` (only `eid.isString` gate); pending `958`,`936` | `{"tm":{"e":{"tid":<any Listed id>,"fc":[0..N-1]}}}` / `{"exp":{"f":{"eid":"x","fid":[0..N-1]}}}` | Same algorithmic blow-up; `ReopenMissingTournaments` always keeps a Listed tid available, expedition needs only a string `eid`. | Validate roster/`fid` size at top of parser (reject before populate+dedup); cap to teamsize/party size; sort+unique. |
| H3 | **Per-block unconditional full-table scans** — 3× `fighters.QueryAll` + 1× `xayaplayers.QueryAll` + 2× `tournaments.QueryAll`, deserializing every proto every block (violates the hard event-driven rule) | `logic.cpp` UpdateState `2087-2129` → `CheckFightersForSale(1996)`, `SetFreeTransfiguringFighters(2019)`, `ProcessSpecialTournaments(909)`, `TickAndResolveOngoings(672)`, `ReopenMissingTournaments(2057)`, `ProcessTournaments(1694)` | Any first move from a fresh name auto-mints player+2 fighters+3 recipes for free (`xayaplayer.cpp:86-118`, trigger `moveprocessor.cpp:1300-1303`) | Block cost grows O(players + 3·fighters + tournaments) **permanently and irreversibly**; attacker inflates loop bounds by mass-registering names; also degrades organically. | Index/filter queries (`WHERE status IN(Exchange,Transfiguring,SpecialTournament)`, `ongoings_size>0`, tournament `state`); complete the P-E1 migration to drive ticks off `OngoingsTable.QueryForHeight()`; query tournaments by `authoredid+state`; charge for / gate the free fighter auto-mint. |
| H4 | **Never-deleted orphan `recepies` rows** (owner='') — no global cap | `moveprocessor.cpp:5011` (cook), `4943` (destroy), `logic.cpp:168,179` (reward-gen at resolution); per-owner cap bypassed (`recipe.cpp:544-549` `WHERE owner=?`) | `{"exp":{"f":…}}` / `{"ca":{"r":…}}` — esp. the C9 free expedition faucet | Monotonic unbounded DB growth at gas-only cost → slow sync/replay for every node. | `DeleteById` when owner is reset to '' (or GC owner='' rows not referenced by a live fighter — fighters reference `recipeid`, so GC must exclude those); add a hard global cap. |
| H5 | **`rewards` rows per-player uncapped, deleted only on claim** | `logic.cpp:150`; only deleted at `moveprocessor.cpp:3492,3499,3619` | Run each distinct expedition/tournament/sweetener once, never claim | Reward rows accumulate; the per-eid "collect previous reward first" gate is itself an O(rewards-for-owner) scan, so a hoarder makes their own moves heavier. | Cap total unclaimed rewards per player; index/scope the previous-reward check. |
| H6 | **Uncapped sub-move array** — one tx's `move` may be an unbounded array, each element runs all ~12 handlers | `moveprocessor.cpp:1276-1288`, loop `1313-1368` | `move:[{…},{…},…]` | Multiplies per-handler cost; amplifies H1/H2 within one tx. | Cap sub-moves per move (e.g. ≤16) before the loop. |
| H7 | **`ParseRewardData` O(N×R)** — uncapped non-deduped `eid` array, full `rewards.QueryForOwner` scan per element | `moveprocessor.cpp:1448-1509`; pending `943` | `{"exp":{"c":{"eid":["<authid>",…repeated N×]}}}` (after growing R via H5) | O(N×R) per-move CPU. | Dedupe + size-cap `eid`; fetch rewards once into a map keyed by expedition id. |

Confidence high on existence/reachability of all H-items. Magnitude of H1/H2 is **argued from gas math + a local `-O2` benchmark, not a live Polygon run** — single-digit seconds per block-filling move, not the "multi-minute" some sub-reports claimed; a `-O0`/debug build would be 10–30× worse. The catastrophic endpoint of H3 requires registering a large number of names (real one-time gas per name). Treat H1–H3 as must-fix; the rest as same-pass cleanup.

---

## 4. Consensus divergence (fork) risks

| # | Vector | Why nodes diverge | Deterministic fix | Status |
|---|--------|-------------------|-------------------|--------|
| F1 | **Pending processing writes to the live confirmed DB** outside undo/session tracking (`ExtractMoveBasics` crownholder address-fixup `set_address`) | A crownholder with empty `proto.address` (a real player who won a special tournament without ever sending an `a`/init move) triggers `set_address(candidate)` during *pending* processing; the write **autocommits to the confirmed DB and is absent from any block changeset**. Nodes that saw the mempool move now hold a different address than those that didn't → next block's `ExtractMoveBasics` either processes or skips (`return false`) the move differently → **confirmed-state divergence**; reorg cannot revert the out-of-band write → stale state. | Make pending strictly read-only: process pending against a private snapshot/copy, **not** `game.database->GetDatabase()`; move the address-fixup out of shared `ExtractMoveBasics` into a confirmed-only `MoveProcessor` path; assert no writes during pending. | `reachable_needs_fix`, impact **chain-fork**. Mechanism fully proven; the precondition (forcing an addressless account to win a 25h special tournament) is **not** exhaustively demonstrated → **medium confidence, needs human/runtime check.** Pending tracking is on by default. |
| F2 | **Raw `double` in LIVE prestige & sweetness consensus state** (`isFork2=true` path still uses it) | `GetFighterPercentageFromQuality` returns `count/fighters.size()` as a `double`, wrapped into `fpm::fixed_24_8` (`xayaplayer.cpp:247-258,330-333`); `UpdateSweetness` computes `(rating-1000)/100.0+1` (`fighter.cpp:384`). `prestige` is a stored column feeding `specialtournamentprestigetier` → in the state hash. Bit-identical on homogeneous IEEE-754 builds, but an independent node operator building with `-ffast-math`/`-mfpmath=387`/aggressive `-ffp-contract` gets a 1-ULP-different value that truncates to a different tier → permanent fork. | Compute the percentage in `fpm` fixed-point (`fixed_24_8(count)/fixed_24_8(size)`); compute sweetness with integer math. Add CI: ban `float`/`double`/`log10`/`pow` in `src/` and `database/` consensus files; compile with `-ffp-contract=off`. | `reachable_needs_fix`, impact **chain-fork** *only under non-uniform FP builds* — NOT move-forceable, won't fork a single-image deployment. Medium confidence. |
| F3 | **`float CHICost` in crystal-bundle cost gate** (`cost = chiPRICE * COIN * (multiplier/1000)`) | Same FP-ABI concern as F2: the `paidToDev < cost` gate is a consensus state transition computed through `float`. | Store bundle cost as an integer base-unit `int64` field; drop float arithmetic from the gate. Replace `0.14 * COIN` with `14000000`. | `reachable_needs_fix`, **real impact none** on realistic 64-bit IEEE-754 nodes (the 6 fixed costs are byte-identical under `-O2` and `-ffast-math`; multiplier is god-mode-locked at 1000). Defense-in-depth only. |
| F4 | **Anti-fork state hash in process-local statics** not rolled back on reorg; `GetStateAsJson` Context built with `std::time(nullptr)` | After a reorg below the last `height%100` checkpoint, `latestKnownStateHash/Block` retain an orphan-branch value until the next checkpoint → RPC `statehex/stateblock` report a hash for an off-chain block, misleading external fork monitoring. Does **not** corrupt consensus DB. | Derive the reported hash from the committed DB at the queried height (pure function), or recompute every block; build the Context with a deterministic timestamp. | **RESOLVED (`23d3c50`)** — replaced the inline `%100` hash + process statics with libxayagame `xaya::SQLiteHasher` (persisted in `xayagame_statehashes`, reorg-rolled-back via undo changesets) + `--statehash_interval` flag + `getstatehash`/`hashcurrentstate` RPCs. Was `low`/diagnostic-only. |

---

## 5. Economic exploits

- **C8 — Unlimited crystal minting (CRITICAL, confirmed).** Pay one bundle (~0.14 CHI), receive N bundles via a repeated-`pc` array move. Root cause is the always-zero `fraction = paidToDev/35` computed before the summation, plus `cost % 35 == 0` for every configured bundle, plus the shared `paidToCrownHolders` across array sub-moves. **The same `fraction`-before-sum bug also exists in `TryNameRerollPurchase` (700-795)** and could enable analogous array replay — fix both. See §2 C8.
- **C9 — Free recipe/fighter faucet (HIGH, confirmed).** `chain==MAIN` dead-guard + `UnitTest_Expedition` in the mainnet config. See §2 C9.
- **SB-06 — Hardcoded free-recipe gift to 3 names (LOW, confirmed).** `MaybeInitXayaPlayer` (`moveprocessor.cpp:5068-5072`) mints a free recipe to exactly `DonRinkula`, `PandaBoss`, `"1 John"` on init. On a fresh relaunch these `p/` names are likely unregistered → an attacker registers them and claims. One-time, one recipe each. **Fix:** remove the hardcoded name gift from the consensus path. Confidence medium. **✅ RESOLVED (`3a3c46c`):** the gift is removed from `MaybeInitXayaPlayer`; grep-verified zero references to the three names / the recipe uuid in code+tests; golden byte-identical (the names aren't in the golden fixture).
- **ECON-03 — Tournament-leave double-deduct (LATENT chain-halt, currently guarded).** `MaybeLeaveTournament` (`moveprocessor.cpp:3564`) charges `joincost` *again* on leave with no balance check; `AddBalance`'s `CHECK_GE(balance,0)` aborts on underflow. **Safe today only because all 42 shipped tournament blueprints have `JoinCost: 0`** (grep-verified). The instant any blueprint ships a non-zero `joincost`, a player who joins at minimal balance then leaves halts the chain. **Fix now** (make leave a refund `+joincost` or a no-op, and guard balance before any `AddBalance(-x)`) — this is a config-fragile guard, not a real gate.
- **OVF-01 — `fpm::fixed_24_8` overflow on large transfigure candy (LOW, uncertain).** `candy["a"].asInt()` (up to INT32_MAX) → `val*256` signed-overflow UB in the `int32` base (`moveprocessor.cpp:3953`, `fpm/fixed.hpp:40`) for candy magnitude >8,388,607. Reachable (inventory `Quantity` is `int64`, MAX 2^50). **But:** build is `-O2` without `-ftrapv` so it wraps (no halt); all nodes wrap identically (no practical fork); the corrupted `fuel20` is capped at 25% of `fuel80` and only lowers the attacker's own rating (no gain). `reachable_needs_fix` / impact uncertain. **Fix:** clamp `candy["a"]` to ≤8,388,607 (and during `fuel20` accumulation) before the conversion.

---

## 6. Guarded / not reachable (do not spend launch effort — but remove for hygiene)

These were prior "CRITICAL leads" and are **confirmed refuted on Polygon**. They are dead code or config-gated; the residual risk is purely "a future edit re-arms them," so delete/compile-out before mainnet.

| Lead | Why it's safe on Polygon | Hygiene action |
|------|--------------------------|----------------|
| **`MaybeSQLTestInjection` / `MaybeSQLTestInjection2`** (spawn 150 + 10 players/fighters) | Only call sites are inside `if(chain == REGTEST)` at `moveprocessor.cpp:1347`; `POLYGON(4) != REGTEST(3)`, distinct enum, not move-spoofable. No pending dispatch. | **Delete both functions, their decls (`moveprocessor.hpp:404-405`), and the 1347-1352 dispatch block.** |
| **The `&&`-vs-`||` fork-gate at `moveprocessor.cpp:4595/4596`** | Lives *inside* the regtest-only `MaybeSQLTestInjection`; only skews `isFork2` in regtest. Zero Polygon consensus effect. | Removed when the function is deleted; otherwise fix `&&`→`||` for consistency. |
| **The OLD `double`/`log10` prestige path (`ND-3`)** (`xayaplayer.cpp:262-318`) | Selected only when `isFork2==false`; Polygon genesis height 89,246,000 ≫ the 5,097,362 fork gate, so `isFork2==true` everywhere. The only non-REGTEST selector is the 4595 bug, itself regtest-gated. | **Delete the dead pre-fork branch** so no future call can resurrect it. The planned genesis-height reset (`logic.cpp:2154` TODO) only raises height further — does not arm it. |
| **`HandleGodMode`** (cost/version/url overrides) | Gated by `params().god_mode()`, which is `false` in the mainnet/Polygon RoConfig (`roconfig.cpp:76-82` routes POLYGON→mainnet, no regtest merge; `roconfig_tests.cpp:53` asserts false). Also fed by `blockData["admin"]`, not player moves. | Keep `god_mode=false`; **add a CI assertion** that the shipped blob has `god_mode=false` and no `*zzzz*`/`*xxxx*`/UnitTest expedition/reward ids. |
| **`burnt` `CHECK` abort (`TF-CRASH-05`)** | `moveObj["burnt"]` is a bridge-synthesized sibling field (not in `moveObj["move"]`), always canonical CHI-decimal; the only in-range failure (>80M CHI) needs burning >80% of WCHI supply in one tx. | Defense-in-depth: replace the hard `CHECK` at `moveprocessor.cpp:291` with a graceful `return false`/`burnt=0`. |

---

## 7. Recommended security hardening plan

Ordered by when to do it. **"Golden-safe"** = the fix changes behavior *only on inputs that currently crash/are invalid*, so existing golden state/test vectors are unaffected. **"Needs golden regen"** = changes a valid state transition, so golden fixtures must be regenerated and re-reviewed.

### Pass A — dedicated security commit, BEFORE launch (blockers)

1. **Crash-class kill (C1–C6 + the full `as*()` sweep).** Add per-element `is*()` guards in every `Parse*`; add `if(!mrl.isObject()) continue;` in both `ProcessOne` and `ProcessMove`; fix the `ps` `size()>=2` guard; replace the `ParseCoinTransferBurn` `CHECK` with `return false`. **Wrap `ProcessOne`/`ProcessMove` in a deterministic `try/catch` that skips the offending sub-move** as the backstop. → **Golden-safe** (only currently-crashing moves change behavior; the catch must skip identically on all nodes).
2. **C7 reroll clamp** — clamp `increasedProbability∈[0,1000]` / cap `cost`. → **Golden-safe** (only out-of-range payments differ).
3. **C8 crystal accounting + the twin `TryNameRerollPurchase` bug** — single mutable remaining-budget, decrement per purchase; cap repeated paid commands per move. → **Needs golden regen** (changes crystal-purchase economic transitions).
4. **C9 + config** — change `chain==MAIN` test guards to cover POLYGON **and** strip `UnitTest_*`/tutorial blueprints+reward-tables out of the base mainnet roconfig into the regtest merge. → **Needs golden regen** + **roconfig blob rebuild**; add the CI blob assertion from §6.
5. **ECON-03** — make tournament-leave a refund/no-op + balance guard. → **Golden-safe today** (joincost=0 ⇒ no behavioral change), but mandatory before any non-zero joincost ships.
6. **Delete the regtest backdoors** (`MaybeSQLTestInjection/2`, dispatch, decls) and the dead `log10` prestige branch; compile-out god-mode setters from the production binary. → **Golden-safe** (dead on Polygon).

### Pass B — current cleanup pass / alongside Stage 2b (DoS + bloat)

7. **H1/H2** — array size caps + replace O(N²) `std::remove` dedup with `sort+unique`. → **Golden-safe if the resulting deduped set is identical** (it is); the caps only reject oversized arrays.
8. **H6/H7** — cap sub-moves per move; dedupe + cap `eid`, single rewards fetch. → mostly **golden-safe** (caps reject oversized/duplicate input).
9. **H4/H5 + SB-06** — delete orphan `recepies` rows on consume (GC excluding live-fighter-referenced ids) and add global caps; cap unclaimed rewards; remove the hardcoded name gift. → **Needs golden regen** (changes DB row lifecycle/state).
10. **OVF-01** — clamp candy magnitude before `fixed_24_8`. → **Golden-safe** (only >8.39M inputs differ; corrupt today anyway).

### Pass C — Stage 2b (event-driven) + determinism hardening

11. **H3 / SB-DOS-03** — finish the P-E1 migration: indexed/`WHERE`-filtered queries and a height-keyed ongoing queue so idle blocks touch ~0 rows; gate/charge the free fighter auto-mint. This is the core "event-driven GSP" requirement. → **Needs golden regen** (changes per-block work but should preserve state if filters are exact — regenerate and diff carefully).
12. **F2/F3** — eliminate raw `double`/`float` from consensus (fpm fixed-point for the percentage/sweetness; integer bundle cost) + CI lint banning `float/double/log10/pow` in consensus dirs and `-ffp-contract=off`. → **Needs golden regen** (boundary prestige/sweetness values may shift) — do this once and freeze.
13. **F1 (REORG-01)** — make pending strictly read-only (snapshot/copy or write-guard); move the crownholder address-fixup into a confirmed-only path. → **Golden-safe for block state** (pending is non-consensus), but changes pending RPC output; **run a human/runtime reorg + addressless-crownholder test** to close the open precondition.
14. **F4** — derive the diagnostic state hash from committed DB; deterministic Context timestamp. → diagnostic-only.

### Items still needing a human / runtime check before sign-off
- **C7** — confirm a valid-address crownholder exists at fresh-launch heights (else the reroll path returns early and does not crash).
- **C9** — run the UnitTest expedition farm to confirm reward-weight-0 selection and exchange-sellability of the resulting fighter.
- **F1 (REORG-01)** — demonstrate (or refute) that an addressless account can be driven to win a special tournament, to upgrade from "mechanism proven, medium confidence" to confirmed.
- **H1–H3 magnitude** — measure actual per-block processing time on the production `-O2` binary under sustained spam at gas-bounded N and at realistic player/fighter counts, to set the array caps and confirm the slowdown threshold.

---

## 8. Pass A1 — crash-class hardening (DONE, golden-safe)

Implemented in one commit; **golden replay byte-identical, 92/92 unit tests green** on the `-O2` container build. An exhaustive adversarial sweep (82-agent workflow: 250 candidate sites → 53 unguarded move-derived → 15 confirmed reachable crash sites) drove this, beyond the originally-named C1–C7. Every fix is golden-safe (changes behavior only on inputs that currently crash).

**Dispatch / backstop:**
- **C1** — `if(!mrl.isObject()) continue;` at the top of the per-sub-move loop in `MoveProcessor::ProcessOne` and `PendingStateUpdater::ProcessMove`. Kills the `[0]`/`[1]`/`["x"]` top-level-array-element crash (`mrl["a"]` → `Json::LogicError`) and every sibling `mrl[...]`.
- **Backstop** — per-sub-move `try/catch(const std::exception&){ log+continue; }` in both dispatchers. Verified deterministic (skip is identical on all nodes; logs are not hashed). Catches any *thrown* jsoncpp type-error missed by the per-element guards. NOTE: it does **not** catch `CHECK`/abort/UB — those are fixed at source below.

**Per-element / size guards (validate-before-mutate, the primary fix):**
- **C5** — `result.size() >= 2` before `std::stoi(result[1])` in `ParseSweetenerPurchase` (OOB `vector::operator[]` UB; the existing `catch(...)` never covered it).
- **C6** — `ParseCoinTransferBurn`: `CHECK(isObject())` → `if(!isObject()) return false;` (subsumes the old array guard, DRY).
- **C2** — `ParseExpeditionData`: `if(!ft.isInt()) return false;` per `fid` element.
- **C3** — `ParseDestroyRecepie`: `if(!ft.isInt()) return false;` per `rid` element.
- **C4** — `ParseRewardData`: `if(!eID.isString()) return false;` per `eid` element (also closes the jsoncpp version-dependent `asString()`-on-number fork hazard).

**Abort/UB fixed at source (backstop cannot catch these):**
- **C7** — `RerollName`: clamp `increasedProbability` to `<= 1000` (kills both `NextInt(0)` `CHECK_GT` abort and the negative-wrap infinite reroll loop at ~1738 CHI overpay).
- **NEW — fighter-deletion root cause (`MaybeCookRecepie`).** `ParseCookRecepie` only validates `fid` ownership/status when `requiredfighterquality()>0`, but `MaybeCookRecepie` deleted `fid` for any `fid>0` with **no owner/status check** → (a) griefing: delete *any* player's fighter via a quality-0 recipe; (b) chain-halt: deleting a tournament participant / mid-cook fighter left a dangling reference. Fixed: delete only when `fighter` exists **and** `GetOwner()==name` **and** status `Available`. This single root-cause fix removes the dangling reference that caused the four null-derefs below.
- **NEW — `logic.cpp` null-deref defense-in-depth** (in case any other delete path appears): `ResolveSweetener` (null-check `fighterDb`), `ProcessTournaments` `if(fighter==nullptr) continue;` at all three `fighters.GetById(participant)` sites (Running/Listed/Completed).
- **NEW — `xayaplayer` `CreateNew` non-idempotent abort.** `ProcessSpecialTournaments` first-run init `CreateNew("xayatf<N>")` would `CHECK`-abort if an attacker pre-registered that name and triggered account creation. Fixed: get-or-create guard at `logic.cpp:955`.

**Economic latent halt:**
- **ECON-03** — `MaybeLeaveTournament`: removed the erroneous second `AddBalance(-joincost)` (double-charge → `CHECK_GE` underflow abort once a non-zero joincost ships). A `+refund` was rejected as a faucet (the deduction ran *outside* the ownership-validated loop).

**Trusted (swept, not a bug):** `CalculateFuelPower`'s `fighterToSactifice["ap"]/["c"].asString()` (~3917) reads `wholeFightersData`, which is reconstructed server-side from the DB fighter proto (`moveprocessor.cpp:4095-4149`), not the move — no attacker control.

## 9. Pass A — C8 (crystal-mint replay) + C9a (production guards) — DONE, golden-safe

Both landed with **golden replay byte-identical, 92/92 green** — and both turned out **golden-safe**, not golden-regen as originally assumed.

- **C8 — unlimited crystal mint (and twin name-reroll) replay.** Root insight: `paidToCrownHolders` is **non-persisted** local bookkeeping (it only validates `out` payment ≥ bundle cost and is logged at end-of-move); the `fraction = paidToDev/35`-before-sum bug is therefore cosmetic. The *only* real exploit is the **replay**: an array move `[{"pc":"T1"},…×N]` re-sums the unconsumed budget every sub-move and mints N-for-1. Fix: **consume the budget immediately after the mint/reroll** (`for(auto& e: paidToCrownHolders) e.second = 0;`) in `TryCrystalPurchase` (after `AddBalance(crystalAmount)`) and `TryNameRerollPurchase` (after `RerollName`), placed *before* the crownholder-loop early-returns so it can't be bypassed. A second `pc`/`nr` then re-sums to `paidToDev=0 < cost` → parser returns false. Golden-safe: a single legitimate purchase mints identically and the zeroed map only changes the (non-state) end-of-move log.
- **C9a — production guards restored** (`chain == MAIN` → `chain != REGTEST`): `logic.cpp:486` (UnitTest `zzzz` expedition reward refused → **faucet closed**), and `moveprocessor.cpp` ×3 (tutorial/FTUE tournament leave+rejoin, tutorial expedition resolve-twice). Golden-safe: REGTEST evaluated false under `==MAIN` and still evaluates false under `!=REGTEST`; only POLYGON/MAIN behavior is armed.

## 10. Session progress + remaining (2026-06-28, overnight)

**DONE this session (8 commits, each golden replay byte-identical):**
- `23ec97a` A1 crash-class (C1–C7 + the cook-delete root cause + null-deref defenses + CreateNew idempotency + ECON-03).
- `bf9cad4` C8 mint-replay + C9a production guards.
- `0fd1c97` A2a delete the `MaybeSQLTestInjection`/`2` backdoors (+ gametest update).
- `2ebd7da` security regression tests (`src/security_tests.cpp`).
- `a48e4c3` Pass B DoS caps (H1/H2 transfigure + expedition arrays via `MAX_MOVE_ARRAY=1000`; H6 sub-moves via `MAX_SUBMOVES=100`).
- (this commit) delete the dead `log10`/`double` OLD-prestige branch in `CalculatePrestige` (only the deterministic `fpm` fixed-point path remains; removes the F2-leaning raw-`double` path); H7 reward-`eid` array cap.

Test count: **98 unit tests** (was 92) + golden replay, all green on the `-O2` container build.

**DONE (continuation, 2026-06-28 — each golden replay byte-identical, 98/98 unit tests pass):**
- `ffd6f1e` **A2b** — drop the dead `isFork` param from the `XayaPlayer` ctor / `CalculatePrestige` / `CreateNew` / `UpdateSweetness` (+ ~100 call sites incl. tests); collapse `UpdateSweetness` to the deterministic `fpm::fixed_24_8` branch only; delete all 12 always-true `isFork2=(chain==REGTEST||height>=5097362)` fork-gate blocks (4 logic.cpp, 8 moveprocessor.cpp). `RecipeInstance::Generate`'s `fork` param kept (genuinely bivalued: prod=false, tests=true). Net −91 lines.
- `a18a0eb` **C9b** — move the `UnitTest_Expedition` (`zzzz`) blueprint + `UnitTest_Reward` (`xxxx`) table out of the base config into a new `proto/roconfig/regtest_unittest.pb.text` registered in `ROCONFIG_TEXT_PROTOS_REGTEST` (regtest_merge only). Both fields are protobuf maps consumed strictly by-key (`logic.cpp:114` sorts-by-key, proving order is never relied on), so `MergeFrom` keeps REGTEST's keyset identical → golden-safe; production base loses the keys (verified via blob dump: `zzzz`/`xxxx` now only under `regtest_merge`). Tutorial (`c064e7f7`/`cbd2e78a`) left on-chain (C9a-guarded) per owner guidance — frontend-track candidate.

**Remaining (being scoped via a parallel deep-read workflow, then implemented sequentially):**
- **H3 / Pass C "Stage 2b" — event-driven GSP (hard requirement):** drive ticks off `OngoingsTable.QueryForHeight()` so idle blocks do ~zero per-player work; eliminate the per-block full-table scans (`logic.cpp` UpdateState ~2087-2129).
- **H4/H5 (row GC, golden-regen):** delete orphan `recepies` rows on consume (excluding live-fighter-referenced recipeids); cap unclaimed rewards per player.
- **F2/F3 (determinism):** replace any raw `double`/float reaching persisted consensus state with `fpm::fixed_24_8` (e.g. sweetness `((rating-1000)/100.0)+1`).
- **F1 (reorg):** make `PendingStateUpdater::ProcessMove` strictly read-only (no writes to the confirmed DB).
- **SB-06:** remove the hardcoded name-gift. **OVF-01 (low):** clamp candy magnitude before `fixed_24_8`.
- **Pass C (event-driven + determinism):** H3 = Stage 2b height-keyed ongoing table + indexed per-block queries; F2/F3 = remove raw `double`/`float` from consensus (sweetness/percentage/bundle cost) + CI lint; F1 = make pending strictly read-only.

## 11. Launch-readiness re-verification (2026-06-30) — code-state audit of every "remaining" item

The §10 "Remaining" list predated the cleanup/quality sweep and was reconciled
against the **actual current source** (post Pass-F split, so old line numbers are
stale) by a 10-verifier + completeness-critic workflow (`wnugk5ru7`). Outcome:
most items had in fact already landed; the headline 80-/security-audit had also
**missed one real launch-blocker** (OVF-01 on the candy/fuel path). Findings:

**Re-confirmed DONE in current code (first-hand evidence):**
- **F2/F3 determinism** — no raw runtime `float`/`double` reaches hashed state.
  Sweetness bucket is integer (`fighter.cpp` `((rating-1000)/100)+1`, all `uint32`);
  `GetFighterPercentageFromQuality` returns `fpm::fixed_24_8` (no `count/size`
  double); prestige uses `fpm::log10` on `fixed_24_8`; the dead `log10`/`double`
  OLD-prestige branch is gone.
- **F1 (REORG-01)** — pending is read-only: `BaseMoveProcessor(readOnly=true)`; the
  only Parse\*-reachable confirmed-DB write (`globalData.SetQueueData` in the
  tournament-entry validator) is `if(!readOnly)`-guarded; regression test
  `PendingTournamentEntryDoesNotWriteQueueData` locks it.
- **Pass-A C1/C8/C9** present and correct; **ECON-03** leave path is a no-op.
- **Tournament-entry affordability** — NOT a gap (critic false positive):
  `ParseTournamentEntryData` already rejects `GetBalance() < joincost` before
  `MaybeEnterTournament`'s `AddBalance(-joincost)`.

**Fixed this session — security/determinism pass (golden byte-identical, full suite green):**
- **OVF-01 (was the one real launch-blocker)** — attacker-controlled candy amount
  `{"ic":[{"a":N}]}` was fed straight into `fpm::fixed_24_8(N)`, whose int32 store
  overflows (UB) at `N == 2^23` (raw `N*256 == INT_MIN`); inventory cap `2^50`
  makes it reachable, and it's the byte-identical mechanism already fixed for the
  EXCH-1 exchange path. Fix: reject `N > MAX_TRANSFIGURE_CANDY` (1'000'000, ~8x
  under the overflow point) in `ParseTransfigureData` + a defensive clamp in
  `CalculateFuelPower` for the non-consensus RPC fuel estimator. Regression test
  pinned at `8'388'608` added to `TransfigureWrongValues`.
- **Sentinel UB (same function)** — `fpm::fixed_24_8(9999999)/(-9999999)` rating
  min/max sentinels overflow int32 at construction; replaced with
  `from_raw_value(-1734967552)/(1734967552)` (the exact wrapped raws, captured
  empirically in-container) → UB removed, result provably byte-identical. The
  min/max diversity branch is consequently inert (collapses to `rDiffAverage`);
  making it actually track min/max is a separate, sign-off-gated balance change.
- **Determinism guardrails (F2/F3 insurance)** — `-ffp-contract=off` added to
  `configure.ac` (portable `AX_CHECK_COMPILE_FLAG`); a `make check` gate
  (`contrib/check-consensus-determinism.py`, wired via top-level `check-local`)
  now fails the build on any raw `float`/`double` in `src/`+`database/` consensus
  code (excludes `fpm/`, display TUs, tests). Verified: passes clean on the tree,
  catches a planted `double`, ignores comments/strings.

**Still OPEN, but all deterministic ⇒ NOT chain-safety blockers (economic / scoped):**
- **SB-06** — every new account auto-mints 2 free fighters + item stacks + 3 recipes
  (crystals already 0). Deterministic; economics/product sign-off, not a chain bug.
- **Tournament `JoinCost`** — entry IS guarded today; but enabling any non-zero
  `JoinCost` still needs an audit (all 21 blueprints are 0 now).

**RESOLVED since this audit (scalability, deterministic — golden BYTE-IDENTICAL):**
- **DEF3** (`0ceb591`) + **DEF2** (`6179a4a`) — the per-block full-table scans of
  fighters (`CheckFightersForSale`, `SetFreeTransfiguringFighters`) and tournaments
  (`ProcessTournaments`, `ReopenMissingTournaments`) are gone: replaced with indexed
  filtered queries (`fighters_by_status`, `tournaments_by_name_state`,
  `tournaments_by_state`) plus a completed-tournament prune. Idle/maintenance floor is
  now FLAT ~0.33 ms across 0–50k players (was 135.8 ms @50k), independent of game size;
  validated at N=20k full load (0 rejected, no per-block ratchet). See
  `docs/original-vs-rewrite.md` §3.

**Bottom line:** the one move-reachable consensus launch-blocker (OVF-01) is closed
and regression-tested. No deterministic chain-halt/fork/economy-fatal vector remains
open in code, and the scalability ratchet (DEF2/DEF3) is fixed. Remaining items are
economic (SB-06 starter-mint kept; name-gift removed `3a3c46c`) and non-code launch prep:
**re-pin the POLYGON genesis height** (`src/logic.cpp` — still the placeholder tip
`89'246'000`; consensus-critical, set it to the real launch-time tip) and a **live Polygon +
XayaX bridge end-to-end test**. Since this audit (2026-07-01): **`dev_address` set** to the
live-Polygon TEST address `0x2576…8722` (real foundation address TODO for production) and the
**roconfig-blob pin test added** (`RoConfigTests.LaunchConfigIsPinned`) — which surfaced and led
to fixing a latent **blob parallel-build race** (consensus-critical; see `original-vs-rewrite.md`
BLOB-RACE: two concurrent `roconfig_gen` writers under `make -j` could embed a corrupt config
blob → daemon abort or silent fork).

## 12. Sign-off re-verification (2026-07-01) — adversarial 6-dimension code audit

A final launch sign-off re-verification (6 parallel auditors — FP determinism, move reachability,
crownholder/C7, faucet/C9, reorg/F1, integer overflow — each finding confirmed or refuted by 3
independent lenses: determinism / exploitability / fork-divergence). Result: **all dimensions
clean except one move-reachable DoS**, now fixed.

- **DOS-FC (found + fixed, `0bdab5e`)** — the tournament-entry roster array `fc`
  (`ParseTournamentEntryData`, `moveprocessor_activity.cpp`) was the **lone** user-supplied id-array
  with no `MAX_MOVE_ARRAY` cap (the H1/H2/H7 §3 hardening covered expedition `fid`, reward `eid`,
  transfigure `if/ic/ir`, but missed this sibling). The O(n²) `HasDuplicates` dedup + the per-entry
  `GetById` loop ran before the `size()!=teamsize()` reject and before the joincost gate, so one
  cheap move with a large `fc` forces super-linear per-block work on every honest node — a
  deterministic per-block near-halt (**not** a fork). Fix mirrors the expedition cap after the
  `isArray()` check; `OversizedMoveArraysAreCappedNotFatal` extended with a huge-`fc` entry move.
- **All other sign-off items re-confirmed CLEAN first-hand:** F2/F3 (integer/fixed-point only,
  lint + `-ffp-contract=off` global; the `fixed_24_8(0.x)` literals are provably deterministic and
  golden-pinned), C7 (crownholder mechanic removed; deterministic reroll; empty-state safe; clamp
  at `fighter.cpp:299`), C9 (no unbounded/unsinked mint; MAIN strips the test faucet, pin-test
  asserted), F1/REORG-01 (zero pending-time state writes; SQLite-session undo), integer overflow
  (OVF-01/EXCH-1/CRYS-1 closed; `isInt()` bounds all `asInt()`).

**Bottom line (2026-07-01):** no move-reachable chain-halt / fork / economy-fatal / DoS vector
remains open in code. The only remaining launch work is operational: re-pin the POLYGON genesis
height and a live Polygon + XayaX end-to-end test.
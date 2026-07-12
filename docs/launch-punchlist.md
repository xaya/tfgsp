# Treatfighter — Launch punch-list

Single source of truth for what remains before mainnet launch on Polygon. Built from the
2026-06-30 doc+code survey. Tiers by severity; **who** = needs the owner (external input /
launch-time) vs implementable now. Companion docs: `original-vs-rewrite.md` (perf/issues),
`security-audit.md` (move-reachability audit; §11 = launch re-verify).

## 🔴 Launch-blockers (must happen before mainnet)

1. **Re-pin POLYGON genesis height** — `src/logic.cpp:160`. Still the placeholder tip
   `height = 89'246'000`, `hashHex = ""` (chosen ~2026-06-27). **Consensus-critical:** set to
   the real Polygon tip at actual launch time; never change after. **Tooling ready
   (2026-07-09): `contrib/repin-genesis.sh` — one command fetches tip+hash, edits, regenerates
   the golden, runs full make check, commits; E2E-tested against live Polygon.** See
   `docs/launch-runbook.md` step 4. (For the live-Polygon *test* gameid, bumping to the current tip is
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
     **Tooling ready (2026-07-09): `contrib/set-dev-address.sh 0xADDR` — checksum-validates,
     edits BOTH repos in lockstep, regenerates + re-pins the roconfig blob hash, runs full GSP
     make check + the frontend `dev-address-lockstep` vitest; E2E-tested.** Runbook step 3.

3. **Live Polygon + XayaX end-to-end test** — `polygon-test/integration.py` only runs vs a
   FORKED (Anvil) Polygon, manually. Need a real-chain sync + move round-trip on live Polygon
   before sign-off. Owner: "we will make it live, just test it on live polygon anyway."
   **Procedure written: `docs/launch-runbook.md` (claim g/tf FIRST → dev_address → genesis pin
   → deploy → sync-verify → real-WCHI smoke test). Contract pins independently verified against
   live Polygon 2026-07-09 (`tf-frontend/scripts/verify-contracts.mjs`: WCHI symbol/decimals ok,
   XayaAccounts.wchiToken()==WCHI, + official xaya/wchi README match).**

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

8. ✅ **DONE (2026-07-01) — UPD-1 latent-UB guard + deep determinism audit.** A second adversarial
   pass (6 subtle-fork dimensions: SQLite row-order, in-memory/proto-map iteration, JSON-key order,
   integer div/mod/shift, uninit/proto-default, locale/string) came back **0 confirmed issues** — the
   consensus code is defensively written (every state query `ORDER BY` unique PK; every proto-`map<>`
   iteration find-by-key-or-sort-first; no `std::unordered_*`; ASCII-only case-fold). It surfaced one
   latent defense-in-depth gap, now hardened: **UPD-1 (`1cc8862`)** re-guards the
   `sweetenerBlueprint.rewardchoices(rewardID)` repeated-field index in `ResolveSweetener` (out-of-
   range access would be UB → silent fork in an `NDEBUG` build; currently unreachable via the
   cook-time bound). Golden byte-identical. See `security-audit.md` §13. (LOC-5 stringstream map-keys
   noted as an optional non-fork DRY tidy.)

9. ✅ **DONE (2026-07-01) — HALT-01 (critical chain halt) + CC-1 (latent halt).** A third/final
   safety sweep (time/RNG, replay/idempotency, container-bounds crash-safety, completeness critic)
   caught a **CRITICAL move-reachable chain HALT: HALT-01 (`acc1cfd`)** — claiming a
   Move/Armor/Animation reward whose fighter had been deleted (transfigure/cook-replace) null-derefed
   `fighters.GetById(...)` → SIGSEGV on every node = permanent halt. Fixed with a null-check +
   deterministic discard in all three reward branches + regression test
   `ClaimRewardAfterFighterDeletedDoesNotHalt`. Also hardened **CC-1 (`f1a8b41`)** — bounded the
   `RewardType::List` recursion (`MAX_REWARD_LIST_DEPTH`) against a cyclic/deep reward-table config.
   Time/RNG + replay/idempotency dimensions came back clean. Golden byte-identical; full suite +
   determinism lint green. See `security-audit.md` §14. **This sweep earning its keep by catching
   HALT-01 is why the extra passes were run.**

10. ✅ **DONE (2026-07-01) — comprehensive code-quality + security review** (`docs/code-review-2026-07-01.md`).
    6-dimension review (Taurion, dead-code, DRY, naming, scalability, security), consensus-risk-triaged.
    Confirmed **Taurion is gone from code**; reworded the residual Taurion-flavoured comments/log strings
    + fixed log typos + removed dead decls (`94ec3ee`). Found + fixed **6 more latent halt paths
    ROB-1..6** (`e4a7115`, `NextInt(0)`/null-deref guards, all golden byte-identical). Applied the one
    provably-safe perf win PERF-01 (const-ref, `c63a5da`). The higher-value structural refactors
    (PERF-02..06 scalability, DRY-01..06, MAGIC-01/02, `outputDebug`/dead-method removals, the
    still-referenced `Faction` enum) are documented as **owner-gated follow-ups** — they touch consensus
    RNG/state paths (golden-identity necessary-but-not-sufficient) so they want human sign-off. **Do NOT
    rename `recepie(s)`** — it is the load-bearing proto/config/JSON identifier.

11. ✅ **DONE (2026-07-01) — Faction/PlayerRole removal (owner-approved Taurion strip).** The item-10
    "still-referenced `Faction` enum" follow-up was a mischaracterisation: there was no `Faction` enum —
    the Taurion `Faction` had already been refactored into an **inert** `PlayerRole` enum (nothing sets
    a role, nothing gates on it; god-mode uses the roconfig `god_mode()` bool; the lone assignment was a
    hardcoded `name=="tftr"` block the author flagged as "leftover from original source"). It *was*
    consensus state, though: a `role` DB column (always NULL) + a `"role"` account-JSON field (always
    `"i"`). Owner approved full removal. Removed the enum, `ResultWithRole`, the `role` column
    (`schema.sql`), the JSON field (`gamestatejson.cpp`), all Bind/ToString/FromString/GetNullable +
    Set/GetRole + `QueryInitialised`, the now-empty `xayaplayer.tpp`, and every role test ref; reworded
    the 6 residual "faction" comments. Consensus change ⇒ golden **regenerated**; full `make check` +
    determinism lint green.

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

## 2026-07-02 — full-codebase audit (13-dim find→adversarial-verify), commit `ec9aea6`

A fresh 13-dimension audit over the whole repo + the web frontend surfaced findings the
earlier crash-safety passes had missed (they focused on the confirmed move path; these
live on the RPC/reward/serialization edges). All fixed, `make check` green (golden
byte-identical):

- **getfueldata RPC daemon crash (high).** A malformed RPC array element (e.g.
  `candiesSubmited:[5]`, `fightersSubmited:["x"]`) made jsoncpp throw `Json::LogicError`
  out through the libmicrohttpd C callback → `std::terminate`, killing the whole daemon
  (reachable unauthenticated through the web proxy whitelist). `getfueldata` now wraps
  `EvaluateFuelList` in try/catch → invalid-params. Non-consensus (no fork), recoverable
  by restart — but a trivial remote DoS. **This is why the "no chain-fatal vector open"
  posture needed this line: it was true for consensus, not for daemon availability.**
- **Tournament claim `tid<=0` reward destruction (med).** `ParseTournamentRewardData`
  matched reward rows purely on `tournamentid==tid`; a `tid:0` claim collected the
  tournamentid-0 rows (expedition/sweetener/**deconstruction**) and silently deleted the
  deconstruction rewards without paying their candy. Now rejects `tid<=0`.
- **Two latent OOB indexings (low, config-revision-triggered UB):** `ResolveSweetener`
  guarded only the first of three `rewardchoices(rewardID)` accesses; `ClaimRewardsInner`
  indexed a DB-persisted `positionintable()` with no bound. Both now bounds-checked.
- **RerollName money-brick (high, `nr` path).** Set `isnamererolled` before the empty-pool
  early-returns (a no-op reroll permanently bricked the fighter) and hard-coded the pool
  cap at Common, so a paid `nr` on any uncommon/rare/epic fighter consumed the WCHI and
  changed nothing. Now marks rerolled only on success and caps at the fighter's own quality.
- **Transfigure self-sacrifice (low)** and an **unguarded tournament `GetById` on the
  serialization path (low)** — both guarded.
- **Two hot-path indexes** added (`fighters_by_owner`, `recepies_by_owner`) — owner-filtered
  `CountForOwner/QueryForOwner` ran unindexed on nearly every move (same class DEF2/DEF3 fixed
  for rewards/ongoings).
- **Dead code / DRY:** removed empty `BaseMapInstances`, unused `CollectTournaments`, the unused
  `ongoing.proto` import; unified `ArmorTypesForMoveType` + `RecipeCookCostForQuality` and dropped
  a redundant transfigure candy re-validation.

⏭️ **Intentionally deferred (documented):**
- The **5-way reward-table-roll** duplication (sweetener×3 / expedition / tournament) and the
  **fighter-fetch / authoredid-scan** boilerplate — real duplication, but all copies compute
  identical results today and **tournament resolution has no golden scenario**, so a bit-identical
  extraction can't be fully regression-verified. Add tournament/purchase golden coverage FIRST
  (see next), then extract.
- **Golden coverage gap:** ✅ **CLOSED (2026-07-08, `665d2f2`).** `goldenreplay_tests` now also drives
  a tournament join+resolve with permadeath in BOTH branches (capture via `capture_pct=256` on the
  tutorial `cbd2e78a` tier + destroy via `capture_pct=0` on Chocolate Chip `e694d5f8`), plus `pc`
  (WCHI crystal buy), `nr` (WCHI name reroll), and `transfigure` (fighter/candy/recipe sacrifice).
  Every added move is EXPECT-guarded so a silent rejection can't pin false coverage; golden regenerated
  (deterministic byte-for-byte compare + reorg/reorg_game green). Adversarially reviewed clean. Remaining
  golden-uncovered (non-blocking, unit-tested): transfigure options 0/1/2 + the o:3 success roll, and
  the capture-cap (outcome 4) / roster-full (outcome 3) permadeath sub-branches. Reward-roll DRY can
  now proceed on this pinned base.
- The fighter-inventory cap `>` at COOK parse is **intentional** (speculative cook reverts with
  a full refund at resolve — `RecepieInstanceRevertIfFullRoster`), NOT an off-by-one. That rationale
  does NOT extend to exchange buys: transfer is immediate with no resolve-time recheck, so the buy
  parse uses `>=` (fixed 2026-07-10, audit P2-A — `>` allowed buying fighter cap+1 at exactly 48).
- **Frontend serving scale — SSE block-signal fanout (web client, not GSP).** ✅ **DONE
  (2026-07-10, tf-frontend `f633b09`/`089810e`).** One server-side reader holds the single GSP
  `waitforchange` loop and fans block signals out to all browsers over `GET /events` (SSE) — the
  per-tab GSP long-polls are gone, so upstream GSP cost is constant regardless of audience size.
  Load-tested flat to 5000 concurrent streams (~9 KB/client). Ops notes live in
  `docs/launch-runbook.md` step 5.5: **`TRUST_PROXY=1` is required behind a TLS proxy** (else the
  per-IP cap of 8 acts as a global cap), plus the `SSE_MAX_CLIENTS`/`SSE_MAX_PER_IP` knobs.

## 2026-07-10 audit follow-ups (external audit `CODEBASE_AUDIT_2026-07-10.md`, adversarially verified)

Fixed this session (one working-tree batch, committed together after `make check`):

- **P1-01 — `getfueldata` anonymous DoS.** The RPC is reachable unauthenticated through the web
  proxy whitelist and did O(candidates × submitted-items) work with per-candidate deep copies.
  Now bounded: `FuelRequestCapError` rejects oversized (well-formed) inputs with invalid-params
  before any evaluation work.
- **P1-03 — deleted fighters leaked their retained `owner=""` source-recipe row.** All fighter
  deletion paths (deconstruct-resolve, permadeath destroy incl. roster-full/capture-cap overflow)
  now go through `FighterTable::DeleteWithSourceRecipe`, which drops the recipe row with the
  fighter; a capture keeps it (the transferred fighter still references it).
- **P2-A — exchange buy at exactly the 48-fighter cap.** Buy parse now uses `>=` (see the
  cook-vs-buy rationale above; fixed alongside a read-side market filter that hides listings whose
  buy is already guaranteed to fail at the current height — no more wasted gas on
  `expire == height` listings).
- **P1-05 — the emergency control plane was mis-documented.** `docs/launch-runbook.md` named a
  nonexistent `tournament_kills_enabled` (the accepted key is `tournament_loss_kills_enabled`;
  unknown names are silently ignored) — the documented permadeath kill switch was a no-op. Both
  occurrences fixed, ranges documented, and runbook step 6.5 now sends EVERY lever once (no-op
  defaults) and verifies the log + the stored `parameters` rows.
- **P0-01 residue — runbook step 2 rewritten.** `g/tf` is ALREADY registered on live Polygon, so
  the step is now verify `ownerOf(tokenIdForName("g","tf"))` == custody wallet (+ ERC-721 transfer
  off any hot wallet), not a fresh registration.
- **P0-02 — responsive-but-stalled GSP had no watchdog.** The live GSP sat frozen `catching-up`
  ~716k blocks behind a healthy XayaX for ~12 days; the only known watchdog pattern checked RPC
  liveness and was blind to it. Shipped `contrib/gsp-progress-watchdog.sh` (host cron: RPC answers
  + height progress + bounded lag vs XayaX `getblockchaininfo`, then `docker restart`), a
  `getnullstate` compose healthcheck on `tfd` (visibility only — Docker never restarts unhealthy
  containers; + `curl` in the runtime image for it), and runbook step 5.3 now mandates the shipped
  watchdog. NOTE: the P0-02 verdict "do not launch on the current stalled stack" still stands —
  the launch stack is rebuilt from the re-pinned genesis (🔴 #1) regardless.
- **P2-20 — this punch-list reconciled** (SSE fanout marked DONE, the exchange-cap paragraph
  corrected from "intentional" to fixed, this section added as the audit-fix ledger).

Still open from the audit (tracked):

- **Crystal-bundle price parity gate (P2-11).** `tf-frontend/src/data/crystal-bundles.ts`
  hand-mirrors `proto/roconfig/crystalbundle.pb.text` (T1–T6 `CHICostSats`/`Quantity`) and its
  header points at this punch-list, but no tracked item existed until now. Values manually
  verified to match 2026-07-10 — that is a point-in-time check, not a gate. TODO: generate the
  frontend table from the roconfig (or add a cross-repo parity test in the mold of
  `dev-address-lockstep`) and run it as a mandatory launch/CI gate; a silent drift here misprices
  real-WCHI purchases (overpay is unrefunded, underpay burns the WCHI and mints nothing).

- **Base-image digest pin (P1-07 / phase2-storage-audit §"libxayagame pin").** ✅ **DONE
  (2026-07-12).** `docker/Dockerfile` used untagged `FROM xaya/libxayagame` (= `:latest`), so a
  build silently took whatever the local Docker cache held. Our cache held a **stale Mar-2026
  pre-amend snapshot that ships the SQLITE_BUSY snapshot-open crash WITHOUT the autocheckpoint
  guard** (Docker Hub's `:latest` is still that stale image). Pinned to
  `xaya/libxayagame@sha256:56f0b3b2…` (published tag `6a6f0a2`, 2026-07-05) which carries
  `sqlite3_wal_autocheckpoint(db,0)` + the SnapshotGate locking-gap fix + the `getnullstate`
  unblock. `make check` (golden replay) is the gate on every bump — **golden byte-identical** ⇒
  no consensus change. Two follow-ups NOT covered by this pin: (a) `xaya/xayax:latest` in
  `docker-compose.yml` is still an unpinned **consensus input** (XayaX decodes Polygon logs into
  moves; not touched by `make check`) — pin it by digest too before mainnet; (b) libxayagame
  master's post-2026-07-05 `RollbackTransaction` savepoint/data-loss fix (#149) is NOT in any
  published image — needs a source build of the lib, a separate pre-mainnet decision.

## ✅ Confirmed clean (no action)

- Taurion fully stripped (only doc mentions remain).
- Alchemy RPC key NOT committed (lives in a gitignored `.env` outside the repo; `.gitignore`
  now also blocks `.env`/keys — `d08d573`).
- regtest `xxxx/zzzz` ids are intentional unit-test fixtures.

## Build gotcha

Any roconfig VALUE change (#1, #2, #5) ⇒ full clean rebuild + golden regen (acceptable
pre-launch). Batch them into one regen where possible. Always run the full suite afterward
(golden byte-identical + unit + reorg + reorg-game + database).

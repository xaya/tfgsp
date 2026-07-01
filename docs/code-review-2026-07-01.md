# Treatfighter GSP — code-quality & security review (2026-07-01)

Comprehensive once-over review requested by the owner ("massive once over … DRY, clean, elegant,
scalable, secure … make sure no Taurion stuff left"). Run as a 6-dimension adversarial review
(Taurion remnants, dead code, DRY, naming/elegance, scalability, general security), each finding
triaged for **consensus-risk** so that only provably golden-neutral fixes were auto-applied. This
followed three determinism/safety sweeps (16 dimensions; see `security-audit.md` §11–§14).

## Verdict

- **Taurion: gone from code.** Zero `taurion` references in any `.cpp/.hpp/.proto/.sql`. The only
  remnants were Taurion-*flavoured* comments/log strings (Character/Region/building/account/waypoint)
  — all reworded this session. One live-referenced `Faction` enum remains (see below).
- **Security: no fork/halt/economy/DoS vector open in code.** The review found **6 additional
  latent halt paths (ROB-1…6)**, all fixed this session, plus the earlier DOS-FC/HALT-01/UPD-1/CC-1.
- **Clean/DRY/scalable:** cosmetic cleanup applied; the higher-value structural refactors are listed
  below as owner-gated follow-ups (they touch consensus RNG/state paths where golden-identity is
  necessary-but-not-sufficient proof, so they want human sign-off — not an unsupervised auto-apply).

## Fixed this session (all golden byte-identical; full suite + determinism lint green)

| Item | Commit | What |
|---|---|---|
| ROB-1/2 | `e4a7115` | recipe move-gen `NextInt(0)` + `while(true)` non-convergence → guarded |
| ROB-3/4 | `e4a7115` | armor / animation empty-vector `NextInt(0)` → guarded |
| ROB-5 | `e4a7115` | deconstruction-reward `fighterDb` null-deref → captured behind null guard |
| ROB-6 | `e4a7115` | reward-claim `rewards.GetById`/`recipeTbl.GetById` null-deref (HALT-01 sibling) → guarded |
| Taurion comments (TB-02…12) | `94ec3ee` | reworded Character/Region/building/account/waypoint → tf entities |
| Typos (TYPO-01…07) | `94ec3ee` | `erorr/figher/maxomus/raonge/succesfully/leaved/purchace` in LOG strings; `Sactifice`→`Sacrifice` local |
| Dead code (TB-01, DC-11) | `94ec3ee` | removed dead `MaybeUpdateFTUEState` decl + parked `C:/log/` comment |
| PERF-01 | `c63a5da` | `GenerateActivityReward` by-value proto+3 strings → const-ref (copy elision) |

## Recommended follow-ups (owner-gated — NOT auto-applied)

### Scalability (verify golden-replay byte-equality after each)
- **PERF-02** `logic_resolve.cpp:73-105` — the `List`-reward branch copies the **whole**
  `activityrewards` map and `std::sort`s it on every call/recursion, then only does a linear
  `authoredid == listtableid` match whose result is order-independent. Replace with a direct linear
  scan (as `ResolveExpedition`/`ResolveSweetener` already do); also drop the `[=]` copy-capture. The
  inner `rewards()` loop order (consensus-visible, RNG) comes from the matched table's repeated
  field, **not** the outer sort, so removing the sort is byte-neutral — but it is on an RNG/state
  path, so confirm golden.
- **PERF-03** `logic_tournament.cpp:103-160` — `ProcessTournaments` builds the O(roster²)
  `fighterPairs` vector every Running block but consumes it only at `blocksleft()==0`. Move the
  pair-population into the `blocksleft()==0` branch (keep `collectTeams()`/`teamsjoined` each block).
- **PERF-04** `logic_combat.cpp:49-73` — `ExecuteOneMoveAgainstAnother` linearly scans the whole
  `fightermoveblueprints` map **twice per move-pair per round** (O(pairs·rounds·2·|map|)). Precompute
  an `authoredid→movetype` map once per block and pass it in. (Params `lmv`/`rmv` also copy-by-value.)
- **PERF-05** `logic_resolve.cpp` (×4) + `logic_tournament.cpp:193` + `moveprocessor_activity.cpp:655`
  — reward-table-by-authoredid resolution is a duplicated O(activityrewards) scan in ~7 hot sites.
  NB: the map **key is not the authoredid** (it's a name like `RewardsTable_1stExpedition`), so a
  direct `map[]` lookup is impossible without a precomputed `authoredid→ActivityReward*` index.
- **PERF-06** `moveprocessor_internal.hpp:74-88` — `SortPairsByKey` deep-copies every proto map value
  before sorting; callers only read in sorted key order. Sort pointers/keys instead (byte-identical).

### DRY (golden-neutral in effect, but cross-file / RNG-order-sensitive → verify)
- **DRY-01** — three hand-rolled map-copy+sort blocks (`logic_resolve.cpp:75`, `logic_tournament.cpp:388`,
  `gamestatejson.cpp:499`) duplicate the existing `SortPairsByKey`; unify (needs the helper promoted to
  a shared header).
- **DRY-03** — the "find proto-map entry by `authoredid`, capture `.second`, break" loop is duplicated
  ~19×; add a `FindByAuthoredId(map, id)` template. (Do NOT touch the `authoredid` field name.)
- **DRY-04** — four near-identical weighted-roll reward-table resolution blocks in `logic_resolve.cpp`
  (base/moves/armor/expedition); extract `RollRewardTable(...)`. **RNG call sequence is
  consensus-critical** — only golden-safe if every `NextInt` and every `GenerateActivityReward` arg is
  preserved exactly.
- **DRY-05** — the fighter `GetById→null→owner→status==Available` validation preamble repeats ~10×;
  extract `AcquireOwnedAvailableFighter(...)`. Careful: sites differ in accepted status
  (`Available` vs `Deconstructing`) and `return false` vs `return`.
- **DRY-06** — `(pxd::FighterStatus)(int32_t)…GetStatus()` (×14) and `(pxd::RewardType)(int32_t)…type()`
  (×8) verbose double-casts; add tiny inline `StatusOf()/RewardTypeOf()` helpers (golden-neutral).

### Naming / magic values (behavioral → owner's call)
- **MAGIC-01** `moveprocessor_activity.cpp:216/450/552` — the tutorial/FTUE tournament (`cbd2e78a-…`)
  and expedition (`c064e7f7-…`) GUIDs are hardcoded magic strings scattered across leave/join/resolve,
  each gated on `chain != REGTEST`. Hoist into roconfig params (or named constants). Needs golden regen.
- **MAGIC-02** `logic_resolve.cpp:450` — the unit-test blueprint sentinel GUID
  (`00000000-…-zzzz-…`) is an inline magic string; define it once as a shared named constant.
- **READ-01** `logic_resolve.cpp:567-582` (dup `moveprocessor_cooking.cpp:472-487`) — raw quality
  literals `1..4` where `pxd::Quality` (Common..Epic) exists; compare against the enum (byte-identical).

### Do NOT change (load-bearing / intentional)
- **UNSAFE-01 — `recepie`/`recepies`.** Pervasive misspelling, but it is the **proto map field name**
  and the **client-facing state-JSON key** (asserted in `gamestatejson_tests.cpp`). Renaming would
  break the pinned roconfig, the golden replay, and clients. **Leave as-is.**
- **UNSAFE-02 — test-name spellings** (`Shedule`, `Uninitialised`). British-spelling / API-mirroring
  identifiers in test names; renaming is churn with no consensus benefit. Leave.
- **`Faction` enum** — still referenced in `database/xayaplayer.cpp:315/339/367` (bind/validate). It is
  a possible Taurion remnant but is live-referenced, so it was **not** touched. Owner decision: confirm
  whether `Faction` is a real tf concept or dead scaffolding to excise (would need golden regen).

### Deferred safe cleanups (low value; skipped to avoid unsupervised build-bisect risk)
- **DC-01** — `CalculateFuelPower`'s `outputDebug` param is always `false`; the param + its ~9
  `if(outputDebug)` LOG blocks + the 5 `, false` call sites are dead. Safe to remove (all callers pass
  `false`), just mechanical surgery across a signature — do when convenient.
- **DC-02/03/04** — never-called `OngoingsTable::DeleteForHeight`/`DeleteForOwner` and
  `RecipeInstanceTable::CountForAll` (decl+def, zero callers). Safe to delete.
- **DC-05/07/08/09/10** — unused `<thread>/<fstream>/<cmath>/<sstream>/<iostream>` includes in various
  consensus TUs. Legit removals, but the review's include analysis was **unreliable** (it wrongly
  flagged `<chrono>` in `moveprocessor.cpp`, which IS used), so re-verify each per-file before removing.
- **DC-12** — `XayaPlayersTable::QueryInitialised` has only a test caller; keep or drop per intent.

## Notes
- Every fix in this session was verified with a full Docker `make check` (proto2 + database + src +
  goldenreplay + reorg + reorg-game) and the consensus-determinism lint, with the **golden replay
  byte-identical** in every case.
- The remaining launch blockers are unchanged and operational only: re-pin the POLYGON genesis height,
  live Polygon + XayaX e2e, production `dev_address`/gameid.

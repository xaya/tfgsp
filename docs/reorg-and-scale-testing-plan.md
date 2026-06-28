# Reorg + scale ("10000s of stuff") testing plan — VERIFIED APIs + design

## STATUS (2026-06-28)

- **Tier 1 — DONE.** `src/reorg_tests.cpp` written, wired into `src/Makefile.am`
  (`check_PROGRAMS` + `TESTS`, with `-DSQLITE_ENABLE_SESSION=1` + `-lsqlite3`).
  Three tests, all green in `make check`:
  `IdleBlockReorgIsExactAndCheapAtScale`, `BuildResolveAndDeepReorgRestoresState`,
  `MultiBlockReorgUnwindsExactly`.  Default scale 1000 players (~2 s);
  `TF_REORG_N=20000 ./reorg_tests` exercises the user's "10000s" (verified at
  20 000 — undo exact, idle block still nets 21 redundant rewrites + ZERO undo
  bytes regardless of N, i.e. the event-driven guarantee survives reorg).
- **CONSENSUS BUG found + fixed (the test's first catch).** The re-apply
  determinism check exposed that `LazyProto::GetSerialised()`
  (`database/lazyproto.tpp`) used plain `SerializeToString`, which serialises
  protobuf `map<>` fields (e.g. the player Inventory's fungible items) in a
  per-message RANDOMISED order.  Same logical proto → different bytes per node /
  per replay → silent chain fork (xaya::SQLiteGame hashes the stored bytes).
  Fix: `CodedOutputStream::SetSerializationDeterministic(true)` at that single
  chokepoint (covers every proto in every table).  Golden byte-identical, 100/100
  unit tests pass.  Fresh relaunch ⇒ no legacy-format compatibility concern.
- Also fixed an UNRELATED pre-existing gate breakage: `proto2/roconfig_tests.cpp`
  referenced the renamed `CrystalBundle.chicost` (now `chicostsats`).
- **Tier 2 (GameTestWithBlockchain production path) and Tier 3 (gametest/reorg.py
  end-to-end) remain TODO.**

---

# Reorg + scale ("10000s of stuff") testing plan — VERIFIED APIs + design

> User directive (2026-06-28): "do the reorg tests … should also have some fake data to put in, like
> 10000s of stuff to see how it all works, with building stuff, and reorging. we need super good testing
> for this. very important." Pass C is complete (F1 committed `5d60f18`); this is the next work.
>
> All file:line below were VERIFIED first-hand (grep) on 2026-06-28, not just agent-reported.
> libxayagame source is on the host at `/home/acoloss/libxayagame/`. Build/test loop unchanged
> (docker cp → touch → make in `tfdev`; run from `/usr/src/tfgsp/src`).

## Two reorg mechanisms available (both REAL undo, not mocks)

### A. Production reorg path — `GameTestWithBlockchain` (libxayagame `xayagame/testutils.hpp`)
- `class GameTestWithBlockchain : public GameTestFixture`  — testutils.hpp:396
- `void SetStartingBlock (unsigned height, const uint256& hash);`  — :426
- `void AttachBlock (Game& g, const uint256& hash, const Json::Value& moves);`  — :433 (forward; auto-increments height; calls `Game::BlockAttach` → `ProcessForwardInternal` which RECORDS the per-block sqlite session changeset as UndoData)
- `void DetachBlock (Game& g);`  — :438 (REORG; calls `Game::BlockDetach` → `ProcessBackwardsInternal` which inverts+applies the stored changeset)
- Canonical example: libxayagame `xayagame/sqlitegame_tests.cpp:557-588` `MovingTests.ForwardAndBackward`
  (AttachBlock ×2 → DetachBlock → state back to prev → DetachBlock → back to genesis).
- Undo internals: `sqlitegame.cpp` ProcessForwardInternal ~576-596 (`sqlite3session_changeset` → UndoData),
  ProcessBackwardsInternal ~664-685 (`sqlite3changeset_invert` + `sqlite3changeset_apply`).
- NOTE: tfgsp's current unit fixture `PXLogicTests` (logic_tests.cpp:52-190) calls the STATIC
  `PXLogic::UpdateState(...)` directly — it BYPASSES the Game framework and records NO undo data. To use
  AttachBlock/DetachBlock I must wire a real `xaya::Game` + `PXLogic` with storage (the work in Tier 2).

### B. Undo PRIMITIVE directly — sqlite3 session (already built in loadbench)
- `loadbench_tests.cpp:211` `sqlite3* RawHandle()` → `(*db).AccessDatabase([&](sqlite3* raw){...})`.
- `loadbench_tests.cpp:284-371` `InstrumentedAdvance(m)` ALREADY wraps each `UpdateState` in
  `sqlite3session_create` / `sqlite3session_attach(nullptr=all)` / `sqlite3session_changeset`. To UNDO a
  block: keep that changeset, `sqlite3changeset_invert` + `sqlite3changeset_apply` (trivial OMIT conflict
  handler) → DB rolled back. This is the EXACT primitive `xaya::SQLiteGame` uses — testing it at scale
  directly validates the h3 "structural guarantee" (ongoing_operations undone like the 8 proven tables).

### C. End-to-end integration reorg — Python `gametest/` (real regtest daemon)
- Base `gametest/pxtest.py` (`PXTest(XayaGameTest)`, gameid `tf`, binary `../src/tfd`).
- Real reorg via Xaya Core RPC: `self.rpc.xaya.invalidateblock(blk)` then `reconsiderblock(blk)`.
- Template: `/home/acoloss/libxayagame/mover/gametest/reorg.py` (88 lines) — build fork, invalidate,
  assert state reverts, reconsider, assert re-applied. Needs `abandontransaction` on orphaned coinbase txs.
- Wired into `make check` via `gametest/Makefile.am` `REGTESTS=...`; `run-test.sh` exits 77 (SKIP) if
  `xayad`/`xayagametest` absent (so CI passes without the daemon). No reorg test exists in tfgsp yet.

## Scale seeding (reuse from loadbench_tests.cpp)
- Fixture `LoadBenchTests : DBTestWithSchema, WithParamInterface<BenchConfig>` (loadbench_tests.cpp:152).
- `SeedPlayers(int n)` (:243): N players, each via `xayaplayers.CreateNew(name,cfg,rnd)` (also makes tutorial
  recipes+fighters in the XayaPlayer ctor). Logs every 10k. Config sweep includes `active-10k-1k` = 10000
  players / 1000 active (:121-130).
- `SeedActiveOngoings(int k)` (:261): K rows in `ongoing_operations` via
  `OngoingsTable(db).CreateNew(ctx.Height()); op->SetOwner("p"+i); op->SetHeight(now+1e9); set_type(DECONSTRUCTION)`.
- Table CreateNew signatures: XayaPlayersTable `CreateNew(name,cfg,rnd)` (xayaplayer.hpp:313);
  FighterTable `CreateNew(owner,recipe,cfg,rnd)` (fighter.hpp:336);
  RecipeInstanceTable `CreateNew(owner,cr_str|cr_proto,cfg)` (recipe.hpp:212/218);
  OngoingsTable `CreateNew(startHeight)` (ongoings.hpp:187);
  TournamentTable `CreateNew(blueprintGuid,cfg)` (tournament.hpp:219);
  SpecialTournamentTable `CreateNew(tier,cfg)` (specialtournament.hpp:188);
  RewardsTable `CreateNew(owner)` (reward.hpp:226); GlobalData getters/setters only (globaldata.hpp:45).

## Move JSON formats for "building stuff" (verified from existing tests)
- Cook recipe:   `{"ca":{"r":{"rid":1,"fid":0}}}`            (moveprocessor_tests.cpp:245, logic_tests.cpp:1684)
- Expedition:    `{"exp":{"f":{"eid":"<uuid>","fid":4}}}`     (moveprocessor_tests.cpp:384; multi: `"fid":[4,5]`)
- Deconstruct:   `{"f":{"d":{"fid":4}}}`                       (logic_tests.cpp:1063)
- Cook sweetener:`{"ca":{"s":{"sid":"<uuid>","fid":4,"rid":1}}}` (moveprocessor_tests.cpp:363)
- Destroy recipe:`{"ca":{"d":{"rid":[1]}}}`                    (moveprocessor_tests.cpp:301)
- Enter tournament: `{"tm":{"e":{"tid":3,"fc":[4,5]}}}`        (logic_tests.cpp:1309); leave `{"tm":{"l":{"tid":3}}}`
- Special tourn:    `{"tms":{"e":{"tid":1,"fc":[4,5,6,7,8,9]}}}` (logic_tests.cpp:478)
- Wrapper: `{"name":"<player>","move":<above or [array]>}`. Array form = batched sub-moves.

## PLAN — three tiers (implement in order, build+run each)

### Tier 1 (PRIMARY — the user's "10000s + building + reorg"): new `src/reorg_tests.cpp`
Loadbench-style fixture (DBTestWithSchema + RawHandle + SeedPlayers/SeedActiveOngoings). Per scenario:
1. Seed at scale (e.g. 10k players; give a subset real buildable fighters/recipes; create some tournaments).
2. Compute a deterministic WHOLE-STATE hash at B0 — reuse the gamestatejson full-state builder that
   goldenreplay already hashes (find it: goldenreplay_tests.cpp / gamestatejson `GetFullState`), OR dump every
   table `SELECT * ORDER BY id` and hash. (Decide in impl; full-state JSON is the cleaner, proven path.)
3. Open a sqlite3 session (RawHandle), run a block whose moves BUILD ongoings (cook/expedition/deconstruct)
   AND the tick that resolves due ones; capture the changeset.
4. `sqlite3changeset_invert` + `sqlite3changeset_apply` (OMIT conflict handler) → assert whole-state hash ==
   B0 hash (undo correctness AT SCALE, incl. ongoing_operations + fighter/recipe/reward churn).
5. Determinism: from the B0 snapshot, re-run the SAME block → assert identical forward result.
6. Multi-block reorg: stack N changesets, invert+apply in REVERSE order, assert back to B0.
Scenarios: idle-10k (tick touches nothing — proves event-driven win survives reorg), build-and-resolve
(cook→…→resolve across the reorg boundary), full-roster tournament churn, deconstruct→reward→claim.

### Tier 2 (production-path fidelity): `GameTestWithBlockchain<PXLogic>` C++ test
Wire a real `xaya::Game` + `PXLogic` + storage. AttachBlock several blocks of real moves that build/resolve
ongoings; DetachBlock to reorg; assert state reverts; Attach an ALTERNATE block to prove fork handling.
Smaller scale (correctness of the real BlockAttach/BlockDetach path, not the raw primitive).

### Tier 3 (end-to-end): `gametest/reorg.py` (model on mover/reorg.py)
Real regtest daemon: register players, send real cook/expedition moves, build a fork, `invalidateblock`,
assert game state reverts, `reconsiderblock`, assert re-applied. Add to `gametest/Makefile.am` REGTESTS.

## Open design choices to settle in impl
- Whole-state hash mechanism for Tier 1 equality (full-state JSON vs per-table dump). Prefer reusing the
  goldenreplay full-state hash for rigor + consistency with existing determinism proof.
- Tier 1 seeding realism: tutorial fighters/recipes from `XayaPlayersTable::CreateNew` may be enough to drive
  cook/deconstruct; confirm a seeded player can actually submit a valid cook/expedition move (needs balance,
  a cookable recipe id, available fighter). May need to grant balance + known recipe ids per player.
- Whether `<xayagame/testutils.hpp>` (GameTestWithBlockchain) is in the container build include path + a
  Makefile.am target for the new test binaries (add `reorg_tests` to src/Makefile.am like `tests`/`loadbench`).

# Treat Duels Prototype (Phase 1) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** A deterministic C++ duel engine (native + zero-import WASM) in `tfgsp-polygon/duel/` plus a playable vs-AI/hotseat 3v3 duel overlay in `tf-frontend`, per `docs/duels-prototype-spec.md` (commit `df6f378`).

**Architecture:** Dependency-free C++17 core (`duel/src/`) operating on little-endian byte buffers (config → state → per-round orders), compiled twice: a native CLI test binary (emsdk clang++) and a wasi-sdk-24 reactor-mode WASM blob consumed by the browser. Stats are hand-authored JSON code-generated into a C++ table and TS types. The FE adds an isolated overlay (own Zustand store, no submitMove/poller coupling) that drives the WASM engine locally.

**Tech Stack:** C++17 (no exceptions/RTTI/libc-beyond-memcpy in the blob), wasi-sdk 24 (`~/arcade-spike/toolchains/wasi-sdk-24.0-x86_64-linux`), emsdk clang++ (`~/emsdk/upstream/bin/clang++`) for native, node 22 + vitest, Next 16/React/Zustand/three.js (existing FE stack).

## Global Constraints

- Commit author must be `snailbrainx <17693211+snailbrainx@users.noreply.github.com>`; NEVER add Co-Authored-By/Claude trailers (both repos).
- tfgsp-polygon work goes on branch `polygon-rewrite` ONLY. tf-frontend on `master`. No pushes (owner pushes).
- Engine determinism: integer math only, all division by power-of-two shifts, `/256` fixed-point scale (spec §5's intent; fpm not vendored — zero-dep). No floats, no time, no ambient RNG. Reject-not-crash: any malformed input returns an error code; no assert/abort reachable from input bytes.
- WASM blob: zero imports (`-mexec-model=reactor`, undefined symbol = link error). No libc calls that pull WASI imports (no stdio/errno/env); memcpy/memset builtins OK.
- The GSP autotools build (`make check`) must NOT compile `duel/` in phase 1 — `duel/` is self-contained with its own build.sh.
- Prototype is economy-inert: FE writes no moves, touches no gameCache slices, leaves FTUE alone.
- Existing test suites must stay green: tfgsp `make check` untouched by construction; tf-frontend `npx vitest run` (180+ tests) must pass after every FE task.
- three.js never enters the server module graph (`next/dynamic` `ssr:false` for anything importing it).
- Wire formats below are FROZEN contracts across tasks — change requires updating this plan.

## Wire formats (contract for Tasks 1–7)

All integers little-endian. Dense move index = position of the move's AuthoredId in the ASCII-sorted list of all 39 GUIDs from `proto/roconfig/fightermoveblueprint.pb.text`. Canonical encoding: every `reserved`/`pad` field below MUST be 0 and decoders MUST reject non-zero values (these buffers get hashed/signed in game channels — exactly one byte encoding per logical state, no malleability).

**Config (input to `duel_init`)** — `CFG_LEN = 4 + 2*team_size*(3+loadout_size)` (46 for 3v3/4):
```
u8 version=1 | u8 team_size (1..5) | u8 loadout_size (1..4) | u8 reserved=0 (reject non-zero)
per side s in 0,1: per slot t in 0..team_size-1:
  u8 quality (1..4) | u8 sweetness (1..10) | u8 move_count (1..loadout_size)
  u8 moves[loadout_size]   // dense indices, strictly: entries >= move_count MUST be 0xFF,
                           // entries < move_count MUST be < 39 and unique within the treat
```

**State (output of `duel_init`/`duel_apply`)** — `STATE_LEN = 8 + 2*team_size*(8+2*loadout_size)` (104 for 3v3/4):
```
u8 version=1 | u8 team_size | u8 loadout_size | u8 phase (0 active,1 side0 won,2 side1 won,3 draw)
u16 round (starts 0) | u16 reserved=0 (reject non-zero)
per side, per slot:
  u16 hp | u16 max_hp | u8 quality | u8 sweetness | u8 move_count | u8 pad=0 (reject non-zero)
  u8 moves[loadout_size] | u8 cooldowns[loadout_size]  // rounds until usable, 0=ready;
                           // filler slots (>= move_count) MUST be 0 (reject non-zero)
```

**Round orders (input to `duel_apply`)** — `ORDERS_LEN = 1 + 2*team_size*2` (13 for 3v3):
```
u8 version=1
per side, per slot:
  u8 action  // loadout index (< move_count, cooldown must be 0) or 0xFE = Recover
  u8 target  // enemy slot 0..team_size-1 for a move; MUST be 0xFF for Recover
             // KO'd treat (hp==0): action MUST be 0xFE and target 0xFF (canonical encoding)
```

**Resolve semantics (exact):**
1. Validate everything (any violation → reject, state unchanged).
2. Block stances: set of living treats whose action is a real move with MoveType Blocking. Active the whole resolve phase; ends only if that treat is KO'd mid-round.
3. Sort all 2×team_size actions by (speed DESC, side ASC, slot ASC). Recover speed = 0.
4. For each action in order: skip if actor hp==0 now, or action==Recover. Otherwise: if target hp==0 → retarget to living enemy with lowest hp (tie: lowest slot); none living → skip. Clash type of target = its picked move's type if its action is a real move, else NEUTRAL. `mult_256` = 384 advantage / 170 disadvantage / 256 neutral-or-same (pentagon from src/logic_combat.cpp: Heavy>Speedy,Blocking; Speedy>Tricky,Distance; Tricky>Heavy,Blocking; Distance>Heavy,Tricky; Blocking>Speedy,Distance). `dmg = (power * mult_256) >> 8`; if target block-stance: `dmg = (dmg * (256 - block_pct_256)) >> 8`. `hp = hp > dmg ? hp - dmg : 0`. Append log entry.
5. End of round, for every living treat: for each loadout slot j: if j was the action used this round → `cooldowns[j] = stat.cooldown` else `cooldowns[j] = max(0, cooldowns[j]-1)`. (KO'd treats' cooldowns untouched — they're out.)
6. `round += 1`. Winner: enemy side all hp==0 → phase 1/2 (sequential resolution makes mutual-annihilation impossible). Else if `round >= round_cap`: compare Σhp — higher total wins, equal → phase 3 (draw).

**C ABI (native lib + WASM exports, Task 1/5):**
```c
int32_t duel_init(const uint8_t* cfg, uint32_t len);           // 0 ok, -1 reject; out buffer = state
int32_t duel_apply(const uint8_t* st, uint32_t stLen,
                   const uint8_t* ord, uint32_t ordLen);       // 0 ok; out = new state, log buffer = round log JSON
int32_t duel_view(const uint8_t* st, uint32_t stLen);          // RESERVED stub (0 ok/-1 reject; out = state JSON) — see note
const uint8_t* duel_out_ptr(void);  uint32_t duel_out_len(void);
const uint8_t* duel_log_ptr(void);  uint32_t duel_log_len(void);
uint8_t* duel_alloc(uint32_t n);    void duel_free(uint8_t* p); // wasm-side scratch for inputs
```
`duel_view` is **deferred to the channel phase** — the phase-1 web client decodes state client-side (tf-frontend `src/duel/wire.ts` `decodeState`), so the ABI slot is frozen (signature fixed) but its body is an unimplemented stub that hard-rejects.
Log JSON (one round): `{"round":N,"actions":[{"side":0,"slot":1,"kind":"hit|recover|skip","move":12,"target":2,"retargeted":false,"clash":"adv|dis|neu","dmg":37,"blocked":true,"targetHp":63,"ko":false},...],"phase":0}`. View JSON: `{"phase":0,"round":3,"teams":[[{"hp":120,"maxHp":150,"cooldowns":[0,2,0,0]},...],[...]]}` (numbers only — names/art are FE-side).

---

### Task 1: Engine scaffold — wire structs, dual build, test runner

**Files:**
- Create: `duel/src/wire.h` (format constants, offsets, `struct DuelStats`extern decl), `duel/src/engine.h` (C ABI above), `duel/src/engine.cpp` (stub bodies returning -1), `duel/src/jsonout.h` (tiny fixed-buffer JSON appender: `struct JsonBuf { uint8_t* p; uint32_t cap, len; }` + `jb_raw/jb_str/jb_u32/jb_i32` helpers, no libc I/O)
- Create: `duel/test/test_main.cpp` (tiny assert-based runner: `RUN(test_fn)` macro, returns nonzero on failure, prints to stdout — native only, libc fine here)
- Create: `duel/build.sh` (below), `duel/README.md` (one paragraph: what/how-to-build/spec pointer)
- Test: `duel/test/test_main.cpp` with one smoke test

**Interfaces:** Produces the frozen C ABI + `duel/build.sh {native|wasm|all|test}`. Later tasks only fill bodies and add tests.

- [ ] **Step 1:** Write `duel/build.sh`:
```bash
#!/usr/bin/env bash
# Dual build: native test binary (emsdk clang) + zero-import wasm reactor (wasi-sdk 24,
# the proven bomberman blob recipe — no -Wl flags: undefined symbol = link error = zero-import).
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
NAT="${NATIVE_CXX:-$HOME/emsdk/upstream/bin/clang++}"
WASI="${WASI_SDK:-$HOME/arcade-spike/toolchains/wasi-sdk-24.0-x86_64-linux}"
SRC="$here/src/engine.cpp"
mode="${1:-all}"
if [[ "$mode" == native || "$mode" == all || "$mode" == test ]]; then
  "$NAT" -O2 -std=c++17 -fno-exceptions -fno-rtti -Wall -Wextra -Werror \
    -I"$here/src" "$SRC" "$here/test/test_main.cpp" -o "$here/test/duel_tests"
fi
if [[ "$mode" == wasm || "$mode" == all ]]; then
  "$WASI/bin/clang++" -O2 -std=c++17 -fno-exceptions -fno-rtti -mexec-model=reactor \
    -I"$here/src" "$SRC" -o "$here/engine.wasm"
  sha256sum "$here/engine.wasm" | awk '{print $1}' > "$here/engine.wasm.sha256"
fi
if [[ "$mode" == test || "$mode" == all ]]; then "$here/test/duel_tests"; fi
```
Every exported ABI function in engine.cpp gets `__attribute__((export_name("duel_...")))` (and `extern "C"`).
- [ ] **Step 2:** Write the failing smoke test in test_main.cpp: `duel_init(nullptr,0)` returns -1; a valid-length zeroed config also returns -1 (stub).
- [ ] **Step 3:** `bash duel/build.sh all` — both compile, test runs green (stubs reject), `duel/engine.wasm` exists. Verify zero imports: `node -e "WebAssembly.compile(require('fs').readFileSync('duel/engine.wasm')).then(m=>console.log(WebAssembly.Module.imports(m).length))"` prints `0`.
- [ ] **Step 4:** Commit: `feat(duel): engine scaffold — wire contract, dual native/wasm build, test runner`

### Task 2: Stats pipeline — duel-stats.json + codegen

**Files:**
- Create: `duel/data/duel-stats.json`, `duel/gen-stats.mjs`, generated `duel/src/stats_gen.h`, generated `tf-frontend/src/data/duel-stats.ts`
- Test: `duel/test/` add `test_stats.cpp`-style checks in test_main.cpp; FE side checked in Task 6's vitest.

**Interfaces:** Produces `stats_gen.h`: `struct DuelMoveStat { uint16_t power; uint8_t speed, cooldown, type; }` (`type`: 0 Heavy,1 Speedy,2 Tricky,3 Distance,4 Blocking — matches proto enum), `constexpr DuelMoveStat kDuelMoves[39]`, `constexpr DuelTunables kTun = {hp_base,hp_per_quality,hp_per_sweetness,adv_mult_256,dis_mult_256,block_pct_256,round_cap,...}`; and `duel-stats.ts`: `DUEL_MOVE_INDEX: Record<guid, denseIdx>`, `DUEL_MOVES: {guid,name,type,quality,power,speed,cooldown,block}[]` (dense order), `DUEL_TUNABLES`.

- [ ] **Step 1:** `gen-stats.mjs`: parse `proto/roconfig/fightermoveblueprint.pb.text` (regex over `AuthoredId/Name/CandyType/Quality/MoveType` blocks — MoveType may be absent for enum 0 in text-proto), ASCII-sort GUIDs → dense index; merge `duel/data/duel-stats.json` (keyed by GUID: `{power,speed,cooldown,effects:[{kind:"damage"}|{kind:"block"}]}` + top-level `tunables`); fail loudly if JSON keys ≠ blueprint GUIDs exactly (both directions). Emit both files with a `// GENERATED by duel/gen-stats.mjs — edit duel/data/duel-stats.json` header.
- [ ] **Step 2:** Author `duel-stats.json` v1 (mechanical baseline, then hand-jitter ±10% for identity): power by quality `[25,32,40,50]`, Blocking moves ×0.5 power + `{"kind":"block"}`; speed base by type Heavy 30/Blocking 45/Tricky 60/Distance 75/Speedy 90; cooldown = quality-1 (clamp 0..3), Blocking −1 (min 0). Tunables: `hp_base 100, hp_per_quality 30, hp_per_sweetness 10, adv_mult_256 384, dis_mult_256 170, block_pct_256 102, round_cap 30, team_size 3, loadout_size 4`.
- [ ] **Step 3:** Tests (native runner): 39 entries; every power ∈ [10,60], speed ∈ [1,120], cooldown ≤3; ≥5 blocking moves have block flag; dense order matches independent sort of the GUIDs (hardcode first + last GUID and their indices in the test).
- [ ] **Step 4:** Run `node duel/gen-stats.mjs && bash duel/build.sh test` → green. Commit both repos' files: tfgsp `feat(duel): move stats data + codegen (C++ table / TS types)`, tf-frontend `feat(duel): generated duel-stats.ts`.

### Task 3: `duel_init` + state codec + validation

**Files:** Modify `duel/src/engine.cpp` (+ small `duel/src/state.h` structs if wanted); Test: extend `duel/test/test_main.cpp`.

**Interfaces:** Consumes wire formats + `kDuelMoves/kTun`. Produces working `duel_init` and internal `decode_state/encode_state` reused by Task 4. `max_hp = hp_base + (quality-1)*hp_per_quality + sweetness*hp_per_sweetness`.

- [ ] **Step 1 (failing tests):** valid 3v3 config (six treats: q1sw1 with 3 common moves … q4sw10 with 4) → returns 0, out len == 104, phase 0, round 0, every hp==max_hp (assert exact values, e.g. q1sw1 → 110; q4sw10 → 290), cooldowns all 0, moves echoed. Reject cases (each -1, out len 0): bad version, team_size 0/6, loadout 0/5, len mismatch, quality 0/5, sweetness 0/11, move_count 0/>loadout, move idx ≥39, non-0xFF filler, duplicate move within a treat, reserved/pad != 0 (canonical encoding).
- [ ] **Step 2:** Implement; run `bash duel/build.sh test` green. Commit: `feat(duel): duel_init — config validation + initial state`.

### Task 4: `duel_apply` — full round resolve + log (the game)

**Files:** Modify `duel/src/engine.cpp`; Create `duel/test/vectors/*.json` mirrored as C++ test cases (handwritten in test_main.cpp) + `duel/test/selfplay.cpp` (folded into test_main).

**Interfaces:** Implements Resolve semantics 1–6 verbatim. Produces the log JSON (schema above) in the log buffer.

- [ ] **Step 1 (failing tests — the behavioral gate, exact expected numbers computed by hand in comments):**
  1. *Clash advantage:* Heavy(power 40) vs target that picked Speedy → dmg (40*384)>>8 = 60.
  2. *Disadvantage:* Heavy vs target that picked Tricky → (40*170)>>8 = 26.
  3. *Block stance:* target picked Blocking → attacker Heavy gets adv ×384 then block: ((40*384)>>8 *154)>>8 = 36; and stance holds even when the blocker's action fires LAST.
  4. *Speed order + KO skip:* fast Speedy KOs a slow treat before it acts → victim's action logged `skip`.
  5. *Retarget:* two attackers aim at same 1-hp treat; second retargets to lowest-hp living enemy (assert `retargeted:true`).
  6. *Recover:* treat with all moves cooling → action 0xFE legal, logged `recover`; picking a cooling move → reject; KO'd treat encoded non-canonically → reject.
  7. *Cooldown cycle:* move with cd 2 → unusable next 2 rounds, usable on the 3rd (drive 3 applies).
  8. *Win:* KO all 3 → phase 1, later applies reject (phase != 0).
  9. *Round cap:* drive 30 recover-only rounds → phase decided by Σhp (unequal teams) / 3 on equal.
  10. *Determinism:* same state+orders applied twice → byte-identical outputs.
- [ ] **Step 2:** Implement resolve + log writer. Green.
- [ ] **Step 3:** Self-play soak in the runner: xorshift32 PRNG (test-side only) picks random LEGAL orders both sides, 500 games: no reject on legal input, every game terminates ≤ round_cap+1 applies, state len constant, phase ∈{1,2,3} at end. Fuzz: 10k random byte-flips/truncations of valid state/orders → only 0 or -1 returns, never a crash (run once under `-fsanitize=address,undefined` native build variant — add `sanitize` mode to build.sh).
- [ ] **Step 4:** Commit: `feat(duel): round resolver — clash/block/speed/retarget/cooldown/win + log`.

### Task 5: WASM parity gate + artifact publish

**Files:** Create `duel/test/parity.mjs`, `duel/test/golden.json` (generated), `duel/publish-fe.sh`; Modify `duel/build.sh` (add `golden` + `parity` modes), `duel/test/test_main.cpp` (add `--dump-golden` mode: runs every behavioral vector, prints JSON lines `{name, cfgHex|stateHex, ordersHex, outHex, logJson}`).

**Interfaces:** Produces `tf-frontend/public/duel/engine.wasm` + `engine.wasm.sha256` and the parity guarantee.

- [ ] **Step 1:** `parity.mjs`: instantiate `duel/engine.wasm` (assert imports length 0), replay every golden line via the ABI (duel_alloc → write → call → read out/log), byte-compare to native `outHex`/`logJson`. Any mismatch = exit 1 with the vector name.
- [ ] **Step 2:** `native --dump-golden > test/golden.json`, then `node duel/test/parity.mjs` → `PARITY OK (N vectors)`.
- [ ] **Step 3:** `publish-fe.sh`: copy engine.wasm + .sha256 to `../tf-frontend/public/duel/` (path relative to repo layout: `$here/../../tf-frontend`, overridable `FE_DIR` env). Run it.
- [ ] **Step 4:** Commit tfgsp: `feat(duel): native==wasm parity gate + FE artifact publish`; tf-frontend: `feat(duel): engine.wasm artifact (sha <short>)`.

### Task 6: FE wire codec + WASM bridge

**Files:**
- Create: `tf-frontend/src/duel/wire.ts`, `tf-frontend/src/duel/engine.ts`
- Test: `tf-frontend/tests/duel/wire.test.ts`, `tests/duel/engine.test.ts`

**Interfaces:** Produces
```ts
// wire.ts — TS mirror of the frozen formats
export interface DuelTreatCfg { quality: number; sweetness: number; moves: number[] } // dense idx
export interface DuelConfig { teamSize: number; loadoutSize: number; sides: [DuelTreatCfg[], DuelTreatCfg[]] }
export interface DuelOrder { action: number | 'recover'; target: number | null }
export function encodeConfig(c: DuelConfig): Uint8Array
export function encodeOrders(sides: [DuelOrder[], DuelOrder[]], teamSize: number): Uint8Array
export function decodeState(b: Uint8Array): DuelStateView // {phase, round, teams: [{hp,maxHp,quality,sweetness,moves,cooldowns}[],[...]]}
// engine.ts — loads /duel/engine.wasm (fetch in browser, fs in vitest), sha256-checks vs engine.wasm.sha256
export async function loadDuelEngine(): Promise<DuelEngine>
export interface DuelEngine {
  init(cfg: DuelConfig): Uint8Array;                     // state bytes (throws DuelReject on -1)
  apply(state: Uint8Array, orders: [DuelOrder[], DuelOrder[]]): { state: Uint8Array; log: DuelRoundLog };
  view(state: Uint8Array): DuelStateView;
}
```
`DuelRoundLog` matches the engine log JSON schema verbatim.

- [ ] **Step 1 (failing vitest):** wire roundtrip — `encodeConfig` of the Task 3 golden config byte-equals its `cfgHex`; `decodeState` of golden `outHex` gives expected hp/cooldowns. engine.test.ts (node): loadDuelEngine loads the real `public/duel/engine.wasm`, plays golden vector #1 and #7, asserts state + log equal golden.
- [ ] **Step 2:** Implement (engine.ts follows repo module style; wasm instantiation via `WebAssembly.instantiate`, no imports object needed). `npx vitest run tests/duel` green, full suite green.
- [ ] **Step 3:** Commit: `feat(duel): TS wire codec + wasm engine bridge (golden-locked to the C++ engine)`.

### Task 7: Duel store + AI

**Files:** Create `tf-frontend/src/duel/store.ts`, `src/duel/ai.ts`; Test: `tests/duel/store.test.ts`, `tests/duel/ai.test.ts`.

**Interfaces:** Produces a Zustand store (repo pattern = `src/stores/ui-store.ts`):
```ts
type DuelPhase = 'team'|'loadout'|'picking'|'playback'|'result'
interface DuelStore {
  phase: DuelPhase; mode: 'ai'|'hotseat'; pickingSide: 0|1;      // hotseat pass-device
  team: [SelectedTreat[], SelectedTreat[]];                       // SelectedTreat = {fighterId, name, flavor, animationId, quality, sweetness, armorpieces, loadout: number[] /*dense*/ }
  state: Uint8Array | null; view: DuelStateView | null; log: DuelRoundLog | null;
  orders: (DuelOrder|null)[];                                     // current picker's picks
  start(mode, playerTeam: SelectedTreat[]): Promise<void>;        // ai mode: AI builds its team (mirror of player q/sw for fairness v1) 
  pick(slot: number, order: DuelOrder): void; commitPicks(): void; // ai: commit → aiOrders → engine.apply → playback
  finishPlayback(): void; reset(): void;
}
```
`ai.ts`: `aiOrders(view: DuelStateView, aiLoadouts: number[][], lastPlayerTypes: number[]): DuelOrder[]` — deterministic given inputs: prefer type countering the player's most-frequent last-round type, else highest power off cooldown; target = lowest-hp living enemy.

- [ ] **Step 1 (failing tests):** store drives a full scripted 3v3 vs-AI game to `result` using the real engine (reuse Task 6 loader); hotseat mode requires both sides' commits before apply; illegal pick (cooling move) rejected by store guard before reaching engine. ai.test: given a view where player spammed Heavy, AI picks Tricky/Distance when available; deterministic (same input → same orders).
- [ ] **Step 2:** Implement; suites green. Commit: `feat(duel): duel store (team/loadout/pick/playback/result) + heuristic AI`.

### Task 8: DuelStage — 6-fighter 3D scene + playback performance

**Files:** Create `tf-frontend/src/render/DuelStage.tsx` (client-only), `src/duel/playback.ts`; Test: `tests/duel/playback.test.ts` (pure timeline logic; the 3D component is verified by render:shots in Task 10).

**Interfaces:** Consumes `loadFighter`/`createRenderContext`/`attachArmorSet` (src/render). Produces `<DuelStage team0 team1 playing={log|null} onDone />` and `buildTimeline(log: DuelRoundLog): TimelineStep[]` where `TimelineStep = {at: ms, kind: 'attack'|'flinch'|'damage-number'|'ko'|'end', side, slot, dmg?, clash?}` (~1s per action, skippable).

- [ ] **Step 1 (failing test):** buildTimeline on golden log #4: actions ordered by log order, each attack step followed by its flinch+damage-number at +400ms, KO appends 'ko', final 'end'; skip flag compresses to end.
- [ ] **Step 2:** Implement playback.ts; then DuelStage: ONE createRenderContext; 6 × loadFighter (`stripRootMotion:false`), left side x=-2.2/-3.2/-4.2 yaw +π/2, right mirrored; armor attach; frameToBox over union; per-step: attacker `mixer.clipAction(its own AnimationID clip)` once then back to Idle01, target flinch = 150ms code shake + red flash, damage number = DOM overlay div floating up (portal), KO = fall-back rotate + fade; `isLiteDevice()` respected; StrictMode-safe mount/teardown copied from FighterCanvas.tsx:39-93.
- [ ] **Step 3:** vitest green; `npx next build` green (three.js stays client-only). Commit: `feat(duel): DuelStage 3D scene + resolve-log playback timeline`.

### Task 9: Screens + wiring — overlay, team/loadout, pick UI, result, entry point

**Files:**
- Create: `src/duel/DuelOverlay.tsx`, `src/duel/TeamSelect.tsx`, `src/duel/LoadoutEditor.tsx`, `src/duel/PickBar.tsx`, `src/duel/ResultModal.tsx`
- Modify: `src/stores/ui-store.ts` (add `'duels'` to the Overlay union), `app/page.tsx` (render DuelOverlay when overlay==='duels'), `src/ui/tournaments/TournamentsScreen.tsx` (a "Duels" button → `openOverlay('duels')`), FTUE tab-gate check (duels button hidden while tutorial active)
- Test: `tests/duel/overlay.test.tsx` (RTL-style if repo pattern allows, else store-level phase-routing test)

**Interfaces:** Consumes DuelStore + DuelStage + FighterPickCard + FighterRevealModal skeleton + DUEL_MOVES labels. All modals/overlays portal to document.body.

- [ ] **Step 1:** TeamSelect: reuse `FighterPickCard` over the USER slice roster (pick 3, real fighters only). LoadoutEditor: per treat, its owned moves (GUID→dense via DUEL_MOVE_INDEX; unknown GUIDs filtered) as cards showing name/type/power/speed/cooldown; pick ≤4 (default = 4 highest-power). PickBar (picking phase): per living treat a move-card row w/ cooldown badges + tap-enemy targeting (highlight on DuelStage), Recover auto-offered when all cooling; commit button; hotseat interstitial ("Pass to Player 2") between sides. ResultModal on the FighterRevealModal skeleton (victory flare / defeat, Σ stats, Rematch + Done). Mobile ≤640px: cards stack, stage keeps 16:9 letterbox, 360px playable.
- [ ] **Step 2:** Phase-routing test green + full vitest suite green + `npx next build` green.
- [ ] **Step 3:** Commit: `feat(duel): duel overlay — team select, loadout, pick/target UI, result; Tournaments entry`.

### Task 10: Integration proof + docs

**Files:** Modify `tf-frontend/BUILD_PLAN.md` (Phase 8: Duels prototype — status), `tfgsp-polygon/docs/duels-prototype-spec.md` (append "Implementation notes" §11: /256 integer math decision, own-AnimationID playback decision); optional `scripts/render-shots` duel scene.
- Test: everything, eyes-on.

- [ ] **Step 1:** Full gates: `bash duel/build.sh all && node duel/test/parity.mjs` green; tf-frontend `npx vitest run` all green; `npx next build` green; tfgsp `make check` untouched (verify `git status` shows no src/ changes).
- [ ] **Step 2:** Eyes-on-pixels: run the dev server, drive a full vs-AI duel via playwright (team → loadout → 3 rounds → result), screenshot pick phase + mid-playback + result at desktop and 390px; visually verify HP bars, damage numbers, clash badge, cooldown badges. Fix what looks broken (this is the AAA bar).
- [ ] **Step 3:** Commit docs; final commit message `feat(duel): phase-1 prototype complete — playable 3v3 vs AI`.

---

## Self-review notes
- Spec coverage: §4 rules → Task 4 vectors 1–10; §5 engine/builds/tests → Tasks 1,3,4,5; §6 stats → Task 2; §7 UI flow items 1–5 → Tasks 7–9; §8 seams: CommandBatch shape (orders blob), ≤2KB state (104B), reject-not-crash (T3/T4 fuzz), purity (config-only), economy-inert (no Modify of gameCache/actions) — all structural; §9 tests → per-task; §10 acceptance → Task 10.
- Deviations from spec recorded in Task 10 doc step: /256 integer math instead of fpm; own-AnimationID clip instead of a move→clip map; both are implementation-level, intent preserved.
- Type consistency: dense index u8 everywhere; DuelOrder/action encoding single-sourced in wire.ts §contract; log schema quoted once and referenced.

# Duel Combat Depth — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Spec:** `docs/superpowers/specs/2026-07-14-duel-combat-depth-design.md` — **READ IT FIRST.**

**Goal:** Make duels skilful. Today focus-firing one enemy is not a strategy players found — it is
the only line the rules permit — and rarity only adds HP, so all six treats are identical guns.

**Architecture:** ONE wire-format bump up front (v1→v2) that adds shield + limited-use counters +
reserved bytes, *while the format is still free to change*. Then Wave 1 removes the rules that
subsidise the race, Wave 1b makes the enemy's option-set legible (turning a blind coin-flip into
inference), and Wave 2 adds the verbs — which cost **zero wire bytes**, because the move table is a
compile-time constant baked into the WASM.

**Tech Stack:** C++17 (integer-only, no libc, freestanding WASM), Node codegen (`gen-stats.mjs`),
TypeScript/React frontend, vitest, Playwright.

## Global Constraints

- **RARITY BUYS VERBS, NOT NUMBERS.** Damage **never** scales with fighter quality or sweetness.
  This is the anti-pay-to-win property; duels will stake treats. Do not add such scaling.
- **DETERMINISTIC. NO RNG. NO FLOAT.** Integer-only, `/256` fixed-point. Native and WASM must be
  **byte-identical** (`./build.sh parity`). Crits/misses/doubles are *earned*, never rolled.
- **CANONICAL ENCODINGS ENFORCED.** Exactly ONE byte-encoding per logical state (these buffers get
  hashed and signed in the channel phase). Reserved bytes MUST be zero and a non-zero reserved byte
  MUST make `decode_state` return false.
- **KO IS PERMANENT.** It rests entirely on the `hp == 0` guards. A heal on a dead ally is an
  explicit **no-op** — an unguarded heal is an accidental revive.
- **The wire bump happens ONCE** (Task 1). Everything that will *ever* need state is reserved there.
  After game channels ship, the layout is frozen forever.
- Engine repo: `tfgsp-polygon` branch **`polygon-rewrite`** (NEVER `main`). Frontend: `tf-frontend`
  branch **`master`**. Commit as `snailbrainx`; **never** add Claude/Co-Authored-By trailers.
- Gates after any engine change: `./build.sh test` → `./build.sh golden` → `./build.sh parity` →
  `./publish-fe.sh`. `test/golden.json` is a **gitignored build artifact** — regenerating it is one
  command, not a consensus break.

---

## File Structure

| File | Responsibility |
|---|---|
| `duel/src/wire.h` | Byte layout, versions, sentinels, contract pins. **Task 1 only.** |
| `duel/src/state.h` + its codec | `TreatState`/`DuelState`, `encode_state`/`decode_state`. **Task 1 only.** |
| `duel/src/jsonout.h` | `JsonBuf`. Gains an **overflow flag** (Task 2). |
| `duel/src/engine.cpp` | Validation + the resolve pipeline. Tasks 2–6. |
| `duel/src/stats_gen.h` | **GENERATED.** Move table + tunables. Never hand-edit. |
| `duel/data/duel-stats.json` | **The single source of truth** for move power/speed/cooldown/effect/uses/description + tunables. |
| `duel/gen-stats.mjs` | Validates the JSON and emits `stats_gen.h` **and** `tf-frontend/src/data/duel-stats.ts`. |
| `tf-frontend/src/duel/wire.ts` | The TS mirror of the byte layout. Must not drift. |
| `tf-frontend/src/duel/{store,ai,playback}.ts` | Game flow, the heuristic AI, the pure performance timeline. |
| `tf-frontend/src/duel/move-info.ts` | **NEW, pure:** what a move does / beats, and a treat's *derived* weakness. |
| `tf-frontend/src/render/DuelStage.tsx` | 3D. Gains heal numbers, shield bubbles, guard beats, and **walk-to-target staging**. |

---

## Task 1: Wire v2 — shield, limited-use counters, reserved bytes

**This is the only task that may change the byte layout. Everything the game will *ever* need is
reserved here.** No behaviour change: every damage number stays identical.

**Files:**
- Modify: `duel/src/wire.h`, `duel/src/state.h` + its codec `.cpp`, `duel/src/engine.cpp` (encode/decode)
- Modify: `tf-frontend/src/duel/wire.ts` (the mirror + its contract pins)
- Test: `duel/test/test_main.cpp`, `tf-frontend/tests/duel/wire.test.ts`

**Interfaces — Produces:**
```cpp
constexpr uint8_t  kVersion = 2;
constexpr uint32_t kStateTreatOffShield   = 7;   // was kStateTreatOffPad — DELETE the old name
constexpr uint32_t kStateTreatOffReserved = 8;   // 4 bytes: future stun/dot/atk_mod/spd_mod
constexpr uint32_t kStateTreatReservedLen = 4;
constexpr uint32_t kStateTreatOffMoves    = 12;
constexpr uint32_t kStateTreatBaseLen     = 12;  // was 8
// per-treat then: moves[L], cooldowns[L], uses_left[L]
constexpr uint32_t StateTreatLen(uint32_t L) { return kStateTreatBaseLen + 3 * L; }  // was 2*L
// StateLen(3,4) == 8 + 2*3*(12+12) == 152     (was 104)
constexpr uint32_t kStateOffEnergyReserved = 6;  // the header's existing reserved u16 — NAME it now,
                                                 // it is the future team energy pool (action points)
constexpr uint8_t  kTargetAll = 0xFE;            // AoE sentinel (target byte)
```
```cpp
struct TreatState {
  uint16_t hp; uint16_t max_hp;
  uint8_t quality; uint8_t sweetness; uint8_t move_count;
  uint8_t shield;                                    // v2: absorb pool, 0..255
  uint8_t reserved[wire::kStateTreatReservedLen];    // v2: MUST be 0
  uint8_t moves[wire::kMaxLoadoutSize];
  uint8_t cooldowns[wire::kMaxLoadoutSize];
  uint8_t uses_left[wire::kMaxLoadoutSize];          // v2: limited-use ultimates; 255 = unlimited
};
```
- `CfgLen(3,4) == 46` and `OrdersLen(3) == 13` are **UNCHANGED**.
- `decode_state` contract: `shield` and `uses_left` are free `u8`s (all values round-trip); **a
  non-zero `reserved` byte MUST return false**.
- TS mirror: `WIRE_VERSION = 2`, `DuelTreatView` gains `shield: number` and `usesLeft: number[]`,
  `stateLen(3,4) === 152`.

- [ ] **Step 1: RED — pin the v2 shape in the C++ tests**
  In `test/test_main.cpp`: add `constexpr uint32_t kBaselineStateLen = duel::wire::StateLen(3,4);`
  and replace **every** hardcoded `104`/`103`/`105`/`56` with it (`duel_out_len() == 152`,
  `kExpectedStateLen`). Replace the literal version bytes in the cfg builders with
  `duel::wire::kVersion`, and make the two "bad version" mutations relative
  (`static_cast<uint8_t>(duel::wire::kVersion + 1)`) so a future bump never silently ACCEPTS.
  Replace the pad-must-be-zero assertion with: **reserved[0..3] must be zero (4 reject cases)**, and
  add positive cases proving `shield = 42` and `uses_left = 2` round-trip.
- [ ] **Step 2: Run — expect FAIL** (`./build.sh test`; length + missing-symbol errors).
- [ ] **Step 3: GREEN — wire.h, state.h, the codec.** Bump `kVersion`, add the offsets/formulas
      above, update the three `static_assert` contract pins, add the fields to `TreatState`, and
      write/read them in `encode_state`/`decode_state` (reject non-zero reserved).
      `duel_init` seeds `shield = 0` and `uses_left[i] = kDuelMoves[m].max_uses` (255 = unlimited).
- [ ] **Step 4: Run — expect PASS.** `./build.sh test` → `29 ran, 0 failed`.
- [ ] **Step 5: Mirror it in TS.** `wire.ts`: `WIRE_VERSION = 2`, the formulas, the 3 contract pins,
      the `pad !== 0` reject becomes a `reserved !== 0` reject, decode `shield`/`usesLeft`.
      Update `tests/duel/wire.test.ts` pins.
- [ ] **Step 6: Regenerate + republish.** `./build.sh golden && ./build.sh parity` (expect
      `PARITY OK`), then `./publish-fe.sh`. Copy `test/golden.json` → `tf-frontend/tests/duel/golden.json`.
      `cd tf-frontend && npx vitest run tests/duel` → green.
- [ ] **Step 7: Commit** — `feat(duel): wire v2 — shield, limited-use counters, reserved status bytes`

---

## Task 2: Stop subsidising the race — fizzle, fair turn order, sudden death, hard log cap

**Files:** Modify `duel/src/jsonout.h`, `duel/src/engine.cpp`; Test `duel/test/test_main.cpp`

**Interfaces — Consumes:** Task 1's `kVersion = 2`.
**Produces:** log kinds `"skip"` (a whiffed swing) and `"chip"` (sudden death); `duel_apply` returns
`-1` on log overflow.

- [ ] **Step 1: RED — four tests.**
  (a) *fizzle*: A and B both aim at enemy slot 0; A (faster) kills it; **B's blow must WHIFF** — a
  `"skip"` entry with `dmg: 0`, and enemy slot 1 must be at **full HP** (today it is not: B is
  auto-retargeted onto it).
  (b) *fair turn order*: two treats with **equal speed** on opposite sides — side 0 must act first on
  an even round and **side 1 first on an odd round**.
  (c) *sudden death*: run past `sudden_start`; every living treat takes escalating chip damage at
  end of round, it can KO, and the duel terminates.
  (d) *log cap*: a round whose log would exceed the cap returns **-1**, never truncated JSON.
- [ ] **Step 2: Run — expect FAIL** (`./build.sh test`).
- [ ] **Step 3: GREEN — fizzle-on-death.** **Delete** the auto-retarget block (the one that walks
      the enemy team for the lowest-HP living treat). Replace with: if the chosen target is dead, log
      `"skip"` (dmg 0, `targetHp` 0) and `continue` — the move still burns its cooldown/use.
      *Why:* over-committing is now a **bet you can lose**, and every enemy defensive action becomes a
      **bait**. This single rule is what makes target choice a trade-off instead of a lookup.
- [ ] **Step 4: GREEN — fair turn order.** In the insertion sort, the side tie-break becomes
      round-alternating. The comparator's "previous comes before current" test changes from
      `ps < cs` to *"`ps` is this round's first side"*:
      ```cpp
      const uint32_t firstSide = s.round & 1u;  // alternates every round -- see spec
      // ... inside the sort, on equal speed and differing sides:
      const bool prevSideFirst = (ps == firstSide);
      const bool prevBefore = (psp > csp) ||
                              (psp == csp && ((ps != cs) ? prevSideFirst : (pl < cl)));
      ```
      Keep it a **total, deterministic** order: speed DESC → this round's first side → slot ASC.
- [ ] **Step 5: GREEN — sudden death.** New resolve stage **after** the cooldown tick and **before**
      the win check. From `kTun.sudden_start`, every **living** treat takes
      `kTun.sudden_step * (playedRound - kTun.sudden_start + 1)` chip damage. It **ignores** clash,
      block, shield and guard (it is the arena collapsing, not an attack). It can KO. Emit one
      `"chip"` log entry per victim. The win check that follows must see the new HP.
      *Why:* it guarantees termination and kills turtling — the round-cap tiebreak is by **HP-sum**,
      which becomes a win-on-time button the moment heals exist.
- [ ] **Step 6: GREEN — hard log cap.** `kLogCap` 8192 → 32768. Add `bool overflow` to
      `duel::JsonBuf`; every `jb_*` writer sets it when a write is dropped. At the end of
      `duel_apply`, `if (log.overflow) return -1;` — **never** emit truncated JSON.
      *Why:* `jsonout.h` drops past-cap writes **silently by design**; AoE + chip multiply the
      entries, and truncated JSON makes the client's `JSON.parse(readLog())` throw.
- [ ] **Step 7: Run — expect PASS**, then `./build.sh golden && ./build.sh parity`.
- [ ] **Step 8: Commit** — `feat(duel): fizzle-on-death, round-alternating turn order, sudden death, hard log cap`

---

## Task 3: Move-table effects, limited uses, descriptions (codegen)

**Files:** Modify `duel/gen-stats.mjs`, `duel/data/duel-stats.json`; Generated: `duel/src/stats_gen.h`,
`tf-frontend/src/data/duel-stats.ts`

**Interfaces — Produces (compile-time table; ZERO wire cost):**
```cpp
enum : uint8_t { kEffDamage = 0, kEffBlock = 1, kEffGuard = 2, kEffAoe = 3,
                 kEffHeal = 4, kEffSiphon = 5, kEffShield = 6, kEffCounter = 7 };
struct DuelMoveStat {
  uint16_t power; uint8_t speed; uint8_t cooldown; uint8_t type;
  uint8_t effect;    // kEff*
  uint8_t hits;      // 1 = normal; 2 = a double-strike (a move PROPERTY, never a proc)
  uint8_t max_uses;  // 255 = unlimited; 1-2 = a signature ultimate
};
```
New tunables: `aoe_pct_256`, `guard_pct_256`, `siphon_pct_256`, `counter_pct_256`, `block_pct_256`
(raised — block must be worth taking), `sudden_start`, `sudden_step`, `adv_mult_256 = 320`,
`dis_mult_256 = 205` (narrowed: ±50% reads as luck and drowns out every other decision).
TS mirror gains `effect`, `hits`, `maxUses`, `description` per move.

- [ ] **Step 1: Extend the JSON schema** — each move gains `effect` (one of the 8 kinds), optional
      `hits` (default 1), optional `max_uses` (default unlimited), and a human `description`.
- [ ] **Step 2: Validate in `gen-stats.mjs`** — fail the build on: a missing/unknown effect; a
      missing description; `guard`/`shield`/`counter`/`block` on a **non-Blocking** move; `aoe`/`heal`/
      `siphon` on a **Blocking** move; `max_uses == 0`. Keep the existing block↔MoveType-4 agreement
      check. *(A codegen that cannot express an illegal move is worth more than a runtime check.)*
- [ ] **Step 3: Emit** the new fields into **both** `stats_gen.h` and `duel-stats.ts`.
- [ ] **Step 4: In THIS task every move keeps its current behaviour** — all damage moves get
      `kEffDamage`, all Blocking get `kEffBlock`. Authoring the real verbs is Task 7. Add the
      descriptions now.
- [ ] **Step 5:** `node gen-stats.mjs && ./build.sh test && ./build.sh golden && ./build.sh parity` — golden must be **unchanged** (no behaviour change).
- [ ] **Step 6: Commit** — `feat(duel): move effects, limited uses, hits and descriptions in the stat table`

---

## Task 4: The Blocking corner comes alive — Guard, Shield, Counter

Owner: *"what's the point doing block, unless blocks all i guess."* Four q4 Blocking moves are
currently **strictly worse than q1 ones**. Fix the corner.

**Files:** Modify `duel/src/engine.cpp`; Test `duel/test/test_main.cpp`

**Interfaces — Produces:** log kinds `"guard"`, `"shield"`, `"counter"`, `"intercept"`, `"reflect"`;
log fields `absorbed`, `via` (the guardian's slot).

- [ ] **Step 1: RED — tests.** Guard intercepts a blow aimed at the ward (the **guardian** takes it,
      at `guard_pct`); guarding a **dead** ally or **yourself** is rejected (`-1`); shield absorbs
      before HP and excess carries through; shield **persists between rounds** until consumed;
      counter reflects a fraction back at the attacker; **AoE and sudden-death chip ignore Guard**;
      a counter's reflect cannot itself be countered.
- [ ] **Step 2: Run — expect FAIL.**
- [ ] **Step 3: GREEN — orders validation.** A **Guard** move's `target` names an **ALLY slot** on
      the same side: `target < team_size`, the ally must be **alive**, and `target != own slot`
      (self-guard is meaningless — reject, do not silently allow, or two encodings mean one thing).
- [ ] **Step 4: GREEN — the guard map.** In the stance stage (which already runs before any action
      resolves, so a same-round intercept is fully computable), build `guardedBy[side][slot]` →
      guardian slot. **Lowest guardian slot wins ties.**
- [ ] **Step 5: GREEN — resolve.** Fixed interaction order, and state it in a comment:
      **1. guard redirect** (single-target damage only) → **2. shield absorb** (on whoever finally
      receives it) → **3. HP**. Then **counter** fires off the damage *actually dealt*.
      Bounds: `hp` is `u16`, `shield` is `u8`, all `/256` — fuse multiplies (`power * clash * pct >> 16`),
      never chain two `>> 8` shifts (double truncation drifts, and native/WASM must agree bit-for-bit).
- [ ] **Step 6: GREEN — raise `block_pct_256`** so a plain block is worth the turn.
- [ ] **Step 7: Run — PASS**, then golden + parity.
- [ ] **Step 8: Commit** — `feat(duel): Guard, Shield and Counter — the Blocking corner comes alive`

---

## Task 5: AoE

**Files:** Modify `duel/src/engine.cpp`; Test `duel/test/test_main.cpp`

- [ ] **Step 1: RED.** An AoE hits **every living enemy**; the clash multiplier is computed **PER
      TARGET** against *that victim's own picked move*; block and shield apply per victim; it
      **ignores Guard**; each victim gets its **own log entry**; if every enemy is dead it whiffs
      (one `"skip"`). Test: 3 living enemies with mixed clashes, one blocking, one shielded, an AoE
      that kills two at once.
- [ ] **Step 2: Run — expect FAIL.**
- [ ] **Step 3: GREEN — validation.** An AoE move **requires** `target == kTargetAll (0xFE)`; any
      other target is rejected. A non-AoE move must **not** use `kTargetAll`. (Canonical: one
      encoding per logical order.)
- [ ] **Step 4: GREEN — resolve.** Loop the victims at `kTun.aoe_pct_256` of power (sublinear:
      throughput-positive but spread). The per-victim clash data is already in hand (`action[enemy][slot]`).
      *Why it matters:* it removes the proof that "spreading is never better", it is the designated
      **counter to Guard and group-heal**, and with fizzle-on-death it is the one move that can cash
      **two kills**.
- [ ] **Step 5: Verify the log arithmetic** — state it in a comment: 5v5 worst case is 10 actors × 5
      victims = 50 entries. Confirm it fits 32768 and that overflow returns `-1`.
- [ ] **Step 6: Run — PASS**, golden + parity. **Commit** — `feat(duel): AoE — hits every living enemy, clash per target`

---

## Task 6: Heal, Siphon, limited-use ultimates, double-strikes

**Files:** Modify `duel/src/engine.cpp`; Test `duel/test/test_main.cpp`

- [ ] **Step 1: RED.** Heal a wounded ally; heal at full HP (wasted, no error); **heal a DEAD ally →
      explicit NO-OP, no revive**; self-heal is legal; siphon heals the attacker for
      `siphon_pct` of the damage **actually dealt** (so a blocked hit siphons less and a **fizzled
      hit siphons nothing**); siphon cannot overheal; a `max_uses` move is rejected once spent;
      `hits: 2` strikes twice (each strike clashes and can KO independently).
- [ ] **Step 2: Run — expect FAIL.**
- [ ] **Step 3: GREEN — heal.** Ally-targeted (`target < team_size`, same side).
      **`if (ally.hp == 0) → log a no-op and change nothing.`** No clash multiplier applies to a heal
      and the target's block is irrelevant. `hp = min(max_hp, hp + power)`; overheal is wasted.
- [ ] **Step 4: GREEN — siphon.** Damage exactly like a normal move (clash, block, shield, guard all
      apply), then heal the attacker by `siphon_pct` of the damage **dealt**, clamped to `max_hp`.
- [ ] **Step 5: GREEN — limited uses.** Validation rejects an action whose `uses_left == 0`. On use,
      decrement (255 = unlimited, never decremented). *This is the owner's replacement for action
      points: the same "can I afford this now?" tension, at a fraction of the cost.*
- [ ] **Step 6: GREEN — `hits`.** A `hits: 2` move resolves its damage branch twice. **A move
      property, never a proc** — no RNG anywhere.
- [ ] **Step 7: Run — PASS**, golden + parity. **Commit** — `feat(duel): heal, life siphon, limited-use ultimates, double-strikes`

---

## Task 7: Author the 39 moves — the verbs land

**Files:** Rewrite `duel/data/duel-stats.json`; regenerate `stats_gen.h` + `duel-stats.ts`

- [ ] **Step 1: Apply the spec's name-driven map** (see the spec's "Move → ability mapping"):
      **q1 = plain damage only** (the honest, reliable baseline a newbie wins with);
      **q2 = one simple rider**; **q3 = real utility**; **q4 = signature/exotic + `max_uses` 1–2.**
      `Gilded`/`Toffee Tripper` → stun-flavoured trip · `Chewy Absorption`/`Pucker Sucker` → **life
      siphon** · `Golden Shower of Chips`/`Conductive Coat`/`Gold Rush` → **AoE** · `Gilded Bonds` →
      **Guard** · `Berry Bounce`/`Bouncing Barrage` → **Counter** · `Sugar Shield`/`Candied Shell` →
      **Shield**.
- [ ] **Step 2: RETUNE so a rare move is NOT strictly stronger.** A verb-carrying move **trades raw
      damage for its utility** (AoE at ~60% power; heal/guard/shield deal **no** damage). State the
      rule in the JSON header comment. **A q4 must be more *versatile*, never simply bigger** — that
      is the entire fairness property.
- [ ] **Step 3: Fairness levers.** `adv_mult_256 = 320`, `dis_mult_256 = 205`. **Reduce
      `hp_per_quality`** (currently 30 → an Epic is 2.6× tankier) so that within a sweetness bracket
      an Epic's *entire* advantage is its toolkit, not its stat line. Recommend **10** and justify it
      in the commit.
- [ ] **Step 4: Write a description for every one of the 39 moves** (owner's ask — the game must say
      what a move does, in plain words).
- [ ] **Step 5:** `node gen-stats.mjs && ./build.sh test && ./build.sh golden && ./build.sh parity && ./publish-fe.sh`
- [ ] **Step 6: Commit** — `feat(duel): author all 39 moves — AoE, heals, siphon, guard, shields, counters`

---

## Task 8: The frontend learns the verbs (wire, store, AI)

**Files:** Modify `tf-frontend/src/duel/{wire.ts,engine.ts,store.ts,ai.ts}`; Test `tests/duel/*`

- [ ] **Step 1: RED — AI tests.** The heuristic AI must **heal a hurt ally**, **guard the focused
      ally**, **AoE when ≥2 enemies are hurt**, **siphon when low**, and **spend an ultimate**. An AI
      that ignores the new verbs is trivially exploitable and would make the whole rework feel dead.
- [ ] **Step 2: `store.ts`** — `DuelOrder`'s target must be able to name an **ally slot** (heal/guard)
      or `kTargetAll` (AoE). Extend it **minimally**; keep `pick()`/`clickTarget()` total (stray taps
      never throw). The picking UI must offer ALLY targets for a heal/guard, and no target for an AoE.
- [ ] **Step 3: `ai.ts`** — teach it the verbs (deterministic; **no `Math.random` in `aiOrders`**).
- [ ] **Step 4:** `npx vitest run tests/duel` → green. **Commit**

---

## Task 9: Wave 1b — information is depth (the picking HUD)

Owner: *"when choose the attacks, we see a lot more info… what it does, what it's best against. also
the treats weaknesses."* **This is not cosmetic:** public loadouts + public cooldowns give poker's
structure — you know their **range**, not their **hand** — which turns a blind coin-flip into
**inference**.

**Files:** Create `tf-frontend/src/duel/move-info.ts` (**pure**) + `tests/duel/move-info.test.ts`;
Modify `PickBar.tsx`

- [ ] **Step 1: RED — the pure helper.** `describeMove(dense)` → description + what it **beats** and
      **loses to**, in plain words. `derivedThreat(treat)` → a treat has **NO intrinsic element**; its
      weakness is read off its **still-available (non-cooling)** moves: mostly-Heavy ⇒ *"vulnerable to
      Tricky & Distance"*. Unit-test both.
- [ ] **Step 2: GREEN — the helper.**
- [ ] **Step 3: GREEN — the HUD.** For the move you're picking: what it does, what it beats/loses to
      **in words** (the owner has asked **twice** what the colours mean — the game must say it),
      power/speed/cooldown/effect/uses-left. For each **enemy**: its move pool, **which moves are on
      cooldown right now** (the load-bearing bit — it narrows their range), and its derived
      weakness. Same for your own team. **Legible to a casual player:** plain words, progressive
      disclosure (the button stays clean; tap/hover expands).
- [ ] **Step 4:** vitest + `tsc` green. **Commit**

---

## Task 10: The stage performs the new verbs (playback, 3D, audio)

**Files:** Modify `src/duel/{playback.ts,audio.ts,DuelOverlay.tsx}`, `src/render/DuelStage.tsx`;
Modify `scripts/gen-duel-audio.mjs`

- [ ] **Step 1: RED — `playback.ts`.** New `TimelineStep` kinds: `heal-number`, `shield-up`,
      `guard-up`, `intercept`, `counter-flash`, `chip`. An AoE is **one swing** whose victims are
      staggered ~90 ms apart so the eye can count them. `rewindHp` must undo heals and chip too.
- [ ] **Step 2: GREEN — `playback.ts`** (keep it **pure**; all timing lives here).
- [ ] **Step 3: GREEN — `DuelStage.tsx`.** **Green `+N` heal numbers** rising off the *ally*; a
      **shield bubble** that persists between rounds and pops when the last point is eaten; a
      **guard beat** where the guardian steps in front and takes the hit; an AoE that strikes
      everyone. Keep `DuelStage` **sound-free** (it emits `onBeat`; `DuelOverlay` routes audio).
- [ ] **Step 4: GREEN — staging (owner's ask).** *"the treat should walk over to the one it's
      attacking and do the move there… if healing, turn to face whomever is getting healed."*
      Melee walks to its target, strikes **there**, returns. A healer/guardian **turns to face** its
      ally. **Distance moves stay put** — the projectile is their identity. Drive it off the existing
      `TimelineStep` seam; the resolve order already gives one actor at a time, so nobody overlaps.
- [ ] **Step 5: GREEN — audio.** Reuse the existing bank (per-type impacts, `blocked`, `crit`, `ko`,
      `launch`, …). **Five genuinely new clips** are needed — nothing in the bank reads as *healing*
      or *dread*: `heal`, `shield-up`, `shield-break`, `guard-clang`, `sudden-death`. Add them to
      `scripts/gen-duel-audio.mjs` and generate with `ELEVENLABS_API_KEY` (idempotent; the key is
      **env-only, never committed**). The code is safe to land first — a missing clip is silently
      skipped, never a crash.
- [ ] **Step 6: Eyes-on.** Play a real duel and confirm **every verb READS on screen** — *a verb the
      player cannot see is a verb that does not exist*: the AoE hits all three; a heal is a green
      number on the **ally**; healing a **KO'd** ally does **nothing** (KO permanence, visible); a
      shield bubble survives the round and pops; the **guardian** takes the blow, not the ward;
      sudden death actually ends the duel. **Commit**

---

## Task 11: Fairness — sweetness brackets for staked duels

- [ ] **Step 1:** Gate duel matchmaking by **sweetness bracket**, mirroring the tournament brackets
      (`minSweetness`/`maxSweetness`) — the concept players already understand. Owner: *"i dont think
      we should allow someone to play super expensive treats vs a noob, and lose those treats."*
- [ ] **Step 2:** Unit-test the bracket rule; surface it in the duel's team-select UI.
- [ ] **Step 3: Commit**

---

## Task 12: Duel battle music (owner-supplied, 2026-07-14)

Owner dropped **6 battle tracks** in `treatfighter/` (repo-root, outside both repos):
`01_treat_fighter_anthem` · `02_candydoom_knockout` · `03_sweetest_survivor` · `04_sugar_rush_riot` ·
`05_final_bell_fever` · `06_three_on_three` (*"original favourite"*). Owner: *"can be faded in/out and
played in loop for duels/combat. when start combat probably that is when should play."*

**Files:** Move/encode into `tf-frontend/public/audio/duel/music/`; modify `src/duel/audio.ts`,
`src/audio/audioManager.ts`, `src/duel/DuelOverlay.tsx`; Test `tests/duel/audio.test.ts`

- [ ] **Step 1: Encode.** They are **320 kbps, ~4.8 MB each (~29 MB total)** — far too heavy to ship
      as-is. Re-encode to ~112–128 kbps (`ffmpeg -i in.mp3 -b:a 128k -ac 2 out.mp3`), targeting
      **≤2 MB each**. A/B one track by ear before committing to the bitrate.
- [ ] **Step 2: Never preload.** Music is fetched **on duel start only** (the existing battle SFX
      bank stays preloaded; 29 MB — or even 12 MB — of music must never sit in the initial payload).
- [ ] **Step 3: Deterministic track choice.** Pick by `fighterIds`/round-0 state, **not**
      `Math.random()` — the same duel must always sound the same (same rule as `variantFor`).
      Track 6 (`three_on_three`) is the owner's favourite → make it the **3v3 default**.
- [ ] **Step 4: Fade in on combat start, loop, fade out on result.** Route through the existing
      `AudioManager` so the HUD mute + music slider govern it like every other track, and so it
      **ducks or replaces the zone music** rather than stacking on top of it.
- [ ] **Step 5:** Test the deterministic pick + the manifest-on-disk rule (mirror
      `tests/duel/audio.test.ts`). **Commit.**

---

## Pre-flight resolutions (controller, 2026-07-14 — owner asleep)

Four conflicts found scanning the plan before execution. All are **task-ordering mechanics with one
obvious correct answer**, not design changes — resolved as below and flagged for owner review.

**R1 — every engine task is a TWO-REPO task.** The plan only says so in Task 1, but
`tf-frontend/tests/duel/engine.test.ts` replays **every** golden vector through the **real
`engine.wasm`** and byte-compares, and pins the vector count (`toHaveLength(27)`). So *any* engine
change breaks the FE suite unless the artifacts move with it. **Mandatory tail for EVERY engine
task (1,2,4,5,6,7):**
```
bash duel/build.sh test && bash duel/build.sh golden && bash duel/build.sh parity   # expect PARITY OK
bash duel/publish-fe.sh                                                             # engine.wasm + .sha256 -> FE
cp duel/test/golden.json ../tf-frontend/tests/duel/golden.json
# update tests/duel/engine.test.ts: the toHaveLength(N) pin + the provenance comment (new sha/msg)
cd ../tf-frontend && npx vitest run tests/duel                                      # green
```
Commit **both** repos (tfgsp `polygon-rewrite`, FE `master`). `gen-stats.mjs` likewise writes
`tf-frontend/src/data/duel-stats.ts` directly, so Tasks 2/3/7 touch the FE too.

**R2 — `sudden_start`/`sudden_step` move to Task 2.** Task 2 Step 5 consumes them, but Task 3 was
the task adding new tunables. **Task 2 adds those two itself** (`duel-stats.json` + `gen-stats.mjs`
`TUNABLE_KEYS` + the `DuelTunables` struct); Task 3 adds the rest (`aoe_pct_256`, `guard_pct_256`,
`siphon_pct_256`, `counter_pct_256`). Suggested values: **`sudden_start = 12`, `sudden_step = 6`**
(chip `6,12,18,…` from round 12 sums past any real max_hp by ~round 20, so the duel always ends
well inside `round_cap = 30` — the cap becomes unreachable, which is the point).

**R3 — `uses_left` seeding is staged.** Task 1 Step 3 seeds from `kDuelMoves[m].max_uses`, but
`max_uses` does not exist until Task 3. **Task 1 seeds `uses_left[j] = 255` (unlimited) for
`j < move_count`;** Task 3 repoints the real-slot seed at `kDuelMoves[m].max_uses` (which it
defaults to 255 for every move, so **golden stays byte-identical** — Task 3's no-behaviour-change
gate still holds).

**R4 — canonical `uses_left` for filler slots = 0.** The plan does not say. Filler slots
(`j >= move_count`) hold no move, so **`uses_left` MUST be 0 there and `decode_state` MUST reject a
non-zero byte** — the identical malleability rule already enforced for filler *cooldowns*. Without
it, one logical state has many byte encodings and the channel-phase signature is malleable.

**R5 — `retargeted` dies with auto-retarget (Task 2).** Fizzle-on-death deletes the only code that
could ever set it, so the log field becomes permanently `false` — dead weight in a format we are
about to freeze. **Task 2 removes it** from the log schema *and* from
`tf-frontend/src/duel/engine.ts`'s `DuelAction` (it is declared there but never read), plus the
`playback`/`overlay` test fixtures. The three `retarget*` golden vectors are **replaced by
fizzle vectors**, and `engine.test.ts`'s `golden.find(g => g.name === 'retarget')` case must be
repointed.

---

## Self-review

**Spec coverage:** Wave 1 → Tasks 2, 4. Wave 1b → Task 9. Wave 2 → Tasks 5, 6, 7. Limited-use
ultimates → Tasks 1, 6, 7. Crits/misses/doubles earned-not-rolled → Task 6 (`hits`) + Task 7 (trip/
evade) + Task 10 (selling the crit). Block-must-be-worth-taking → Task 4. Turn-order fairness →
Task 2. 3D staging → Task 10. Sweetness brackets → Task 11. Reserved status bytes + energy byte →
Task 1. Landmines: log cap → Task 2; HP-sum tiebreak → Task 2 (sudden death); KO permanence →
Task 6; AI → Task 8.

**Wave 3 (stun/poison/slow)** is deliberately NOT implemented — its bytes are *reserved* in Task 1
so it never costs a second breaking change.

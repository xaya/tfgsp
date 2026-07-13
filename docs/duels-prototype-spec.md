# Treat Duels — prototype spec (phase 1: local fun test)

Owner-approved design, 2026-07-13 (brainstormed section-by-section).  Scope of THIS
spec = the local prototype: a deterministic C++ duel engine (compiled native and to
WASM) plus a playable vs-AI / hotseat duel screen in tf-frontend.  No chain moves,
no channels, no asset writes.  Channels are phase 3 and get their own design cycle
after the fun verdict; this spec only pins the seams the prototype must respect so
phase 3 is a wrap, not a rework.

## 1. Goal & phasing

1v1 player duels: two teams of treats fight with their special moves, Axie/FF-style
simultaneous rounds.  End state (owner decision): duels live INSIDE the tf GSP on
Xaya game channels (`ChannelGame` is `public SQLiteGame` — drop-in base swap), so a
later "winner picks one of the loser's treats" settles natively through the existing
tournament capture/permadeath tail.

- **Phase 1 (this spec):** engine + local prototype UI.  Test: is it fun?
- **Phase 2 (optional):** relay PvP over server.mjs WebSocket, trust-based, no
  consensus — real-human pacing test.  Decided after phase 1.
- **Phase 3:** channels in the tf GSP (base swap, commit-reveal rounds, session
  keys, disputes, stakes).  Own spec.

## 2. Identity — deliberately not Axie

- **The clash read is the core.**  Damage multiplier depends on what the TARGET
  chose THIS round, not a static type chart.  Every round is a poker/fighting-game
  read layered over targeting.  (Axie: cards + energy + auto-target + static
  class-vs-body advantage.)
- **Cooldown rotation, not energy/cards.**  No deck, no draw RNG.
- **TF-native hooks:** sweetness/quality feed HP; v1.5 candidate = armor pieces
  finally matter (a worn piece soaks damage from its `ArmorTypesForMoveType`
  move type — tournament-earned cosmetics become duel gear).
- **Real stakes later:** winner-picks-a-treat permadeath/capture economy.
- **Variant shelf** (only if playtests feel flat): "sugar rush" momentum meter from
  won clashes → comeback ultimate; energy system; commit-salt-derived crit RNG.

## 3. Owner decisions (locked)

| Decision | Choice |
|---|---|
| Turn structure | Simultaneous rounds (commit-reveal in channels; instant in prototype) |
| Team size | 3v3 now; engine parameter for later 5v5 format |
| Abilities | Data-driven effect lists; damage+block v1, heal/shield/buff later |
| Build path | Local-first: engine + vs-AI/hotseat prototype before any channel work |
| End-state home | Inside the tf GSP (ChannelGame base swap), NOT a separate game-id / arcade blob |
| Engine | Written once in C++ (fixed-point, counted RNG), compiled native + WASM from day 1 |
| Uniqueness | Must not be an Axie copy (see §2) |

## 4. Combat rules v1

**Setup.**  Each side: 3 treats (engine param `team_size`).  Pre-duel each treat
gets a **loadout of up to 4 of its owned moves** (duplicates collapse; bounds
channel state; team-building depth).  HP derived from existing identity:
`HP = hp_base + (quality-1)*hp_per_quality + sweetness*hp_per_sweetness`
(tunables in duel-stats.json; target ≈ 120 fresh Common, ≈ 260 sweet Epic).

**Round loop:**
1. **Pick** — both players secretly choose one move per living treat + a chosen
   enemy target per move (FF-style, not auto-target).  A treat with no off-cooldown
   move gets the free `Recover` fallback action (no damage, no cooldown; exists so
   a treat is never stuck).
2. **Reveal** — prototype: instant (AI/hotseat).  Channels later: hash commit-reveal.
3. **Resolve in speed order** — each action carries its move's `speed`; all 6
   actions fire fastest-first across both teams.  Deterministic tie-break:
   higher speed → lower side index (challenger first) → lower slot index.
   A treat KO'd before its action fires loses it.  Dead treats' pending picks are
   simply skipped; targets that died are re-targeted to the lowest-HP living enemy
   (deterministic), so late actions are never wasted on corpses.

**The clash.**  `damage = power(move) × clash(attackerType, targetPickedType)`,
fixed_24_8 math, where the pentagon is the existing one (logic_combat.cpp):
Heavy>Speedy+Blocking · Speedy>Tricky+Distance · Tricky>Heavy+Blocking ·
Distance>Heavy+Tricky · Blocking>Speedy+Distance.
Advantage ×1.5, disadvantage ×0.66, neutral/same ×1.0 (tunables).
The comparison is against the move the TARGET picked this round (a KO'd or
Recover-ing target counts as neutral).  **Blocking moves additionally reduce all
damage the user takes that round** by `block_pct` (tunable, ~40%) — that is their
identity; their own power is low.  Block is a STANCE: it is active from the start
of the resolve phase (picking it = raising your guard), independent of when the
blocking action itself fires in speed order, and ends only if the treat is KO'd.

**Cooldowns.**  Per-move `cooldown` 0–3 rounds; strong moves cool down longer,
forcing rotation (the depth lever replacing Axie's energy).

**Win.**  All enemy treats KO'd.  Round cap (`round_cap`, ~30): higher total
remaining HP wins; equal = draw.  **v1 is RNG-free** — no crit/miss; fair variance
can come later from hashing both commit salts (never from one side's input).

**Economy-inert:** prototype writes nothing — no Rating, no Sweetness, no assets.

## 5. Engine architecture

**Home:** `tfgsp-polygon/duel/` — dependency-free C++17 (no libxayagame, protobuf,
glog, exceptions, RTTI).  It becomes consensus code in phase 3 (board rules wrap
it), but compiles standalone today.

**API (pure functions over byte buffers, C ABI):**
- `duel_init(config_bytes) → state_bytes | reject` — config carries EVERYTHING:
  both teams' per-treat {quality, sweetness, loadout move ids}; engine never reads
  the TF database.  (Phase 3: config = channel metadata blob.)
- `duel_apply(state_bytes, round_orders_bytes) → state_bytes | reject` —
  `round_orders` = BOTH sides' CommandBatches for one round (prototype calls it
  once per round; channels later feed the same bytes after commit-reveal).
- `duel_view(state_bytes) → JSON` — render feed: HP, cooldowns, per-action resolve
  log for playback (attacker, target, clash verdict, damage, KO).
- WASM build is a zero-import reactor (`-mexec-model=reactor`, wasi-sdk 24, the
  proven bomberman recipe: export_name attributes, no -Wl flags, undefined symbol
  = link error = zero-import discipline) + `duel_alloc`/`duel_free` for buffers.

**Determinism (mandatory later, cheap now):** vendored fpm fixed_24_8 like the rest
of TF; float ban; no time / ambient RNG (round seed is an explicit input; v1 passes
zero and consumes zero draws); every input bounds-checked, reject-not-crash (the
IsValid-on-attacker-bytes discipline starts here).

**Move ids on the wire:** the 39 roconfig GUIDs map to a dense generated u8 index
(codegen, §6) — state stays compact; GUID strings never enter engine state.

**Builds (all host-local, no Docker):** native test binary via emsdk clang++;
WASM via wasi-sdk 24 (`~/arcade-spike/toolchains/wasi-sdk-24.0-x86_64-linux`).
`duel/build.sh` builds both, runs tests, then copies + sha256-stamps the wasm to
`tf-frontend/public/duel/engine.wasm`.

**Tests (day 1):** golden round-resolution vectors (JSON in → state/log out,
committed); **native==WASM byte-parity gate** over the vectors (node runner);
malformed-input fuzz (truncated/oversized/out-of-range bytes must reject, never
trap); self-play soak (random-legal AI vs itself N games, no trap, bounded state
size, games terminate).

## 6. Stats data

`duel/data/duel-stats.json` — hand-authored, one source of truth:
- per-move (all 39 GUIDs): `{power, speed, cooldown, effects[]}` — effect schema
  `{kind: damage|block|heal|shield|buff|..., magnitude, target}`; v1 uses
  damage+block only, the schema is the owner's extensibility ask;
- global tunables: hp_base/per_quality/per_sweetness, clash multipliers, block_pct,
  round_cap, team_size, loadout_size.

Authoring rule of thumb v1: power scales with move Quality (Common≈25 → Epic≈50),
speed inverse to power within a type, Blocking low power + block effect,
Distance fast/light, Heavy slow/hard.  Balance iterates here — that IS the
prototype's purpose.

`duel/gen-stats.mjs` codegens `duel/src/stats_gen.h` (C++ table) and
`tf-frontend/src/data/duel-stats.ts` (TS types/labels) — engine and UI can't drift.
NOT in the consensus roconfig blob: the engine is consensus code in phase 3, so its
embedded table is consensus data with no blob re-pin.

## 7. Prototype UI (tf-frontend)

**Entry:** "Duels" button on the Tournaments screen → full-screen **overlay** (like
Leaderboard).  No 6th tab yet — 5-tab bar is deliberate Unity parity; promotion is
a later art decision.

**Flow:**
1. **Team select** — reuse `FighterPickCard`; pick 3 from the REAL roster (USER
   slice: real quality/sweetness/moves/armor).  Loadout editor: 4 moves per treat
   showing power/speed/cooldown/type from generated stats.
2. **Stage — `DuelCanvas`:** ONE shared scene/canvas (never per-fighter
   FighterCanvas mounts), 6 × `loadFighter` left-vs-right (yaw ±90°), armor
   attached, `frameToBox` on the union, mobile lite tier respected, portal to
   document.body (the `.frame` backdrop-filter trap).
3. **Pick phase** — per living treat: 4 move cards with cooldown badges +
   tap-an-enemy targeting; commit locks picks.
4. **Playback** — engine resolves instantly; UI performs the resolve log in speed
   order: attacker plays its attack clip (`stripRootMotion:false` for lunges),
   target gets code-side flinch/flash/knockback + floating damage numbers + HP
   drain; KO = code-side collapse/fade (no death clips exist — faked, fine for the
   fun test).  ~1 s per action, tap to skip.
5. **Result** — victory/defeat modal on the `FighterRevealModal` skeleton.  No
   rewards, nothing written.

**Opponent:** primary = **vs AI** (heuristic: counter-pick bias vs the player's
last-round types + focus-fire lowest-HP; good enough to test feel, solo-playable).
Hotseat behind a toggle (pass-the-device between pick phases).

**Isolation:** pure client state (its own Zustand store); no submitMove, no
gameCache/poller coupling, FTUE untouched; three.js stays out of the server module
graph (`next/dynamic ssr:false`).  Engine loaded from
`public/duel/engine.wasm`, sha256-checked against the build stamp.

## 8. Forward-compat constraints (phase 3 seams — respected now, built later)

- Round input = ONE CommandBatch blob per side (later: one signed channel message;
  round = 4 signed messages total via commit(A)/commit(B)/reveal(A)/reveal(B)).
- State stays compact & serializable, ~2 KB budget (both-sign optimized proofs).
- Reject-not-crash on every byte of state/move input (pre-auth IsValid surface).
- Config-in/state-out purity (config becomes channel metadata `custom` bytes).
- Economy-inert until the owner decides rating coupling (same Rating/Sweetness vs
  a separate duel rating) — deliberately open.
- Deferred with it: stake scope (whole roster vs staked vs participants; starter
  immunity + 48-slot fallback), turn clock / DISPUTE_BLOCKS + params-KV knobs,
  fighter locking while a channel is open, matchmaking (browse-open vs queue),
  session keys + relay + dispute UI (`matchOver = finished OR seen-open-and-gone`),
  WCHI stakes (needs a g/ name decision).  All recorded here so phase 3 starts
  from a list, not archaeology.

## 9. Testing summary

Engine: golden vectors · native==WASM parity gate · malformed-input fuzz ·
self-play soak.  FE: vitest for store/AI/loadout/playback-log mapping (+ wasm
bridge test driving the real engine.wasm under node); `npm run render:shots`
eyes-on-pixels for the duel stage; existing suites stay green (`make check`
untouched — the GSP build doesn't compile `duel/` in phase 1).

## 10. Acceptance (phase 1 done =)

A logged-in player opens Duels from Tournaments, picks 3 of their real fighters +
loadouts, and plays a full 3v3 vs the AI in the real UI — simultaneous picks,
clash multipliers visibly mattering, cooldowns forcing rotation, KOs, victory/
defeat modal — on desktop and the 360 px mobile tier, with all engine gates and FE
tests green.  Balance tunables all live in duel-stats.json.  The owner can answer
"is it fun?" and iterate numbers without touching code.

## 11. Implementation notes (built 2026-07-13 — deviations from the spec above, intent preserved)

These are implementation-level decisions taken during the build; the game rules (§4) and seams (§8)
are unchanged. Recorded here so the spec and the code agree.

**Integer `>>8` (÷256) scaling instead of a float/"fpm" multiplier.** §4 sketched clash multipliers as
fractional scalars. The engine implements them as **zero-dependency fixed-point integers**: each
multiplier is a `uint16_t` in /256 units (`adv_mult_256 = 384` = ×1.5, `dis_mult_256 = 170` ≈ ×0.664,
neutral = `256` = ×1.0, `block_pct_256` likewise), and damage is `dmg = (power * mult) >> 8`, with block
applying `dmg = (dmg * (256 - block_pct_256)) >> 8` (`duel/src/engine.cpp`; tunables in
`duel/src/stats_gen.h`). No floats anywhere in the resolver — the arithmetic is bit-for-bit identical on
every platform, which is what makes the native==WASM parity gate meaningful (and is a hard requirement
for the phase-3 game-channel path where both peers must agree on state). Balance is still authored as
human numbers in `duel-stats.json`; codegen emits the /256 forms. Intent (advantage/disadvantage/block
scaling) preserved.

**Playback uses each fighter's OWN `AnimationID` clip, not a move→clip map.** §7's UI sketch implied a
per-move animation. The assets carry only a single showcase/attack clip per fighter plus `Idle01` (no
per-move, hit, or KO clips), so the 3D stage drives every attacker between its own AnimationID clip and
Idle, and renders flinch / KO topple+fade / recover / floating damage numbers **code-side**. This is a
presentation choice only — it consumes the same resolve log and changes no engine output.

**Canonical-encoding tightenings (reject, don't normalise).** Both `duel_init` and the state decoder
reject any non-canonical buffer rather than silently accepting it — a malleability guard for the
channel path where a buffer's bytes are consensus. Rejected: a non-zero **reserved** byte (config) or
reserved u16 (state), a non-zero per-treat **pad** byte, and **filler-slot cooldowns** that aren't 0
(cooldown bytes for move slots `>= move_count` MUST be 0; filler move slots use the `0xFF` sentinel).
See `duel/src/engine.cpp` (DecodeConfig/DecodeState) and `duel/src/{wire,state}.h`.

**Golden + parity gate (27 vectors).** Determinism is locked by `duel/test/parity.mjs`, which replays
the golden vectors through BOTH the native C++ build and `engine.wasm` and asserts byte-identical state
+ logs (`PARITY OK (27 vectors)`), alongside 29 C++ unit tests (clash/block/speed/retarget/cooldown/
win/round-cap/determinism/self-play-soak/fuzz). The vector set pins the consensus tie-break paths
explicitly: retarget to lowest-hp among multiple living enemies, the equal-hp retarget tie (→ lowest
slot), equal-nonzero-speed cross-side ordering (side ASC before slot ASC), and a `duel_init` reject
(rc -1, empty out). `make check` is untouched — `duel/` is not compiled into the GSP in phase 1.

**`duel_view` deferred to the channel phase.** The C ABI (§11 "API") lists `duel_view(state_bytes) → JSON`,
but phase 1 leaves it a reserved, unimplemented stub (frozen signature, body hard-rejects). The web
client decodes state client-side instead (`tf-frontend/src/duel/wire.ts` `decodeState`), so no
engine-side view render is needed until the game-channel phase.

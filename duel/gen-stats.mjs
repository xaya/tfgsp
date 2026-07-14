#!/usr/bin/env node
// duel/gen-stats.mjs — codegen for the duel engine's move-stat table.
//
// Single source of truth is duel/data/duel-stats.json (hand-authored: power/speed/
// cooldown/effect/description, plus optional hits/max_uses, per move GUID, plus a
// top-level "tunables" block). This script:
//   1. parses proto/roconfig/fightermoveblueprint.pb.text for the 39 authored moves
//      (AuthoredId/Name/Quality/MoveType — MoveType and Quality default to 0 if the
//      text-proto omits them, which proto2 optional fields can do for their zero value),
//   2. ASCII-sorts the 39 AuthoredId GUIDs to fix the engine's dense move index (wire
//      contract: "position in the ASCII-sorted list of all 39 GUIDs" — see the plan's
//      "Wire formats" section),
//   3. cross-checks duel-stats.json's GUID keys against the blueprint's GUID set and
//      FAILS LOUDLY (nonzero exit) on any mismatch in either direction, and validates
//      every move's effect/description/hits/max_uses (combat-depth Task 3) so an
//      illegal move (e.g. a "guard" stance on a non-Blocking move) is unrepresentable,
//   4. emits duel/src/stats_gen.h (C++ table, consumed by the native/wasm engine) and
//      tf-frontend/src/data/duel-stats.ts (TS mirror, consumed by the FE UI/AI) from the
//      same merged, dense-ordered data — so the two are byte-for-byte in sync by
//      construction, never hand-copied.
//
// Usage: node duel/gen-stats.mjs   (then `bash duel/build.sh test` and commit both repos)
import { readFileSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const HERE = dirname(fileURLToPath(import.meta.url));
const REPO = resolve(HERE, '..');
const BLUEPRINT = join(REPO, 'proto', 'roconfig', 'fightermoveblueprint.pb.text');
const STATS_JSON = join(HERE, 'data', 'duel-stats.json');
const OUT_CPP = join(HERE, 'src', 'stats_gen.h');
const FE_DIR = process.env.FE_DIR || resolve(REPO, '..', 'tf-frontend');
const OUT_TS = join(FE_DIR, 'src', 'data', 'duel-stats.ts');

// MoveType enum (database/fighter.hpp pxd::MoveType / proto/fighter_move_blueprint.proto
// MoveType field) — verified: Heavy=0, Speedy=1, Tricky=2, Distance=3, Blocking=4.
const MOVE_TYPE_NAMES = ['Heavy', 'Speedy', 'Tricky', 'Distance', 'Blocking'];

// Move effects (combat-depth Task 3): the kEff* byte codes DuelMoveStat.effect
// carries, and their C++ symbol names. Effects partition into exactly two
// groups, each legal on exactly one side of the MoveType 4 (Blocking) split —
// see the partition check in the entries map below.
const EFFECT_CODES = { damage: 0, block: 1, guard: 2, aoe: 3, heal: 4, siphon: 5, shield: 6, counter: 7 };
const EFFECT_CPP_SYMBOL = {
  damage: 'kEffDamage', block: 'kEffBlock', guard: 'kEffGuard', aoe: 'kEffAoe',
  heal: 'kEffHeal', siphon: 'kEffSiphon', shield: 'kEffShield', counter: 'kEffCounter',
};
const STANCE_EFFECTS = new Set(['block', 'guard', 'shield', 'counter']); // legal ONLY on MoveType 4
const ACTION_EFFECTS = new Set(['damage', 'aoe', 'heal', 'siphon']); // legal ONLY on MoveType 0-3

function fail(msg) {
  console.error(`gen-stats.mjs: ${msg}`);
  process.exit(1);
}

// ---- 1. parse the blueprint text-proto ----
function parseBlueprint(text) {
  const blockRe = /fightermoveblueprints:\s*\{\s*key:\s*"([^"]+)"\s*value:\s*\{([\s\S]*?)\n {2}\}\n\}/g;
  const moves = [];
  let m;
  while ((m = blockRe.exec(text))) {
    const [, key, body] = m;
    const aid = body.match(/AuthoredId:\s*"([^"]+)"/)?.[1];
    const name = body.match(/Name:\s*"([^"]+)"/)?.[1];
    if (!aid || !name) fail(`blueprint entry "${key}" missing AuthoredId or Name`);
    // proto2 optional fields may be omitted from text-proto when they'd hold the
    // type's zero value. MoveType 0 == Heavy is a legitimate value, so an omitted
    // MoveType correctly defaults to 0. Quality has no meaningful zero (authored
    // moves are always Quality 1..4), so an omitted/zero (or otherwise out-of-range)
    // Quality is a data error: parse it, then FAIL LOUDLY on the 1..4 range check
    // below rather than silently emitting a wrong quality via the `?? 0` default.
    const quality = Number(body.match(/Quality:\s*(\d+)/)?.[1] ?? 0);
    const moveType = Number(body.match(/MoveType:\s*(\d+)/)?.[1] ?? 0);
    if (moveType < 0 || moveType > 4) fail(`blueprint entry "${key}" has out-of-range MoveType ${moveType}`);
    if (quality < 1 || quality > 4) fail(`blueprint entry "${key}" has out-of-range Quality ${quality} (expected 1..4)`);
    moves.push({ key, aid, name, quality, moveType });
  }
  return moves;
}

const blueprintMoves = parseBlueprint(readFileSync(BLUEPRINT, 'utf8'));
if (blueprintMoves.length !== 39) {
  fail(`expected 39 blueprint moves, found ${blueprintMoves.length} in ${BLUEPRINT}`);
}

// ---- 2. dense index = ASCII sort of the 39 GUIDs ----
const dense = [...blueprintMoves].sort((a, b) => (a.aid < b.aid ? -1 : a.aid > b.aid ? 1 : 0));

// ---- 3. merge duel-stats.json, fail loudly on any GUID-set mismatch ----
const raw = JSON.parse(readFileSync(STATS_JSON, 'utf8'));
const { tunables, ...statsByGuid } = raw;
if (!tunables) fail(`${STATS_JSON} missing top-level "tunables"`);

const blueprintGuids = new Set(dense.map((m) => m.aid));
const jsonGuids = new Set(Object.keys(statsByGuid));
const missingFromJson = [...blueprintGuids].filter((g) => !jsonGuids.has(g));
const extraInJson = [...jsonGuids].filter((g) => !blueprintGuids.has(g));
if (missingFromJson.length || extraInJson.length) {
  fail(
    `${STATS_JSON} GUID keys don't match ${BLUEPRINT}'s AuthoredIds exactly.\n` +
      `  missing from duel-stats.json (${missingFromJson.length}): ${missingFromJson.join(', ') || '(none)'}\n` +
      `  extra in duel-stats.json (${extraInJson.length}): ${extraInJson.join(', ') || '(none)'}`,
  );
}

const TUNABLE_KEYS = [
  'hp_base',
  'hp_per_quality',
  'hp_per_sweetness',
  'adv_mult_256',
  'dis_mult_256',
  'block_pct_256',
  'round_cap',
  'team_size',
  'loadout_size',
  'sudden_start',
  'sudden_step',
  // combat-depth Task 3: new-verb tunables, wired up by Tasks 4/5/6 (AoE
  // splash %, guard block %, siphon lifesteal %, counter reflect %). Landed
  // now so kDuelMoves[*].effect has something to point at once the resolver
  // reads them; not consumed by any behaviour yet.
  'aoe_pct_256',
  'guard_pct_256',
  'siphon_pct_256',
  'counter_pct_256',
];
for (const k of TUNABLE_KEYS) {
  if (typeof tunables[k] !== 'number') fail(`tunables.${k} missing or not a number in ${STATS_JSON}`);
}

// Every key a move entry may carry. Anything else is a TYPO, and a typo must be
// louder than a default: `"max_use": 1` (missing the s) would otherwise sail
// through as max_uses=255 -- an "ultimate" with UNLIMITED uses -- and no gate
// downstream can catch it, because 255 and 1 are perfectly legal values. Golden
// regenerates cleanly, the C++ partition pin passes, parity is fine. This
// allow-list is the only thing standing between a one-character slip and a
// broken signature move.
const MOVE_KEYS = new Set([
  'power', 'speed', 'cooldown', 'effect', 'description', 'hits', 'max_uses',
]);

const entries = dense.map((bp, idx) => {
  const stat = statsByGuid[bp.aid];
  for (const f of ['power', 'speed', 'cooldown', 'effect', 'description']) {
    if (stat[f] === undefined) fail(`duel-stats.json entry ${bp.aid} (${bp.name}) missing "${f}"`);
  }
  for (const k of Object.keys(stat)) {
    if (!MOVE_KEYS.has(k)) {
      fail(
        `${bp.aid} (${bp.name}) has unknown key "${k}" — typo? ` +
          `(expected one of: ${[...MOVE_KEYS].join(', ')})`,
      );
    }
  }
  // `?? default` swallows an explicit null, so reject it here rather than
  // letting `"hits": null` silently mean "1".
  for (const f of ['hits', 'max_uses']) {
    if (stat[f] === null) fail(`${bp.aid} (${bp.name}) "${f}" is null — omit the key to take the default`);
  }
  if (stat.power < 10 || stat.power > 60) fail(`${bp.aid} (${bp.name}) power ${stat.power} outside [10,60]`);
  if (stat.speed < 1 || stat.speed > 120) fail(`${bp.aid} (${bp.name}) speed ${stat.speed} outside [1,120]`);
  if (stat.cooldown < 0 || stat.cooldown > 3) fail(`${bp.aid} (${bp.name}) cooldown ${stat.cooldown} outside [0,3]`);

  // Rule 1: missing/unknown effect.
  const effect = stat.effect;
  if (!Object.prototype.hasOwnProperty.call(EFFECT_CODES, effect)) {
    fail(
      `${bp.aid} (${bp.name}) has unknown effect ${JSON.stringify(effect)} ` +
        `(expected one of: ${Object.keys(EFFECT_CODES).join(', ')})`,
    );
  }

  // Rule 2: missing/empty description.
  if (typeof stat.description !== 'string' || stat.description.trim() === '') {
    fail(`${bp.aid} (${bp.name}) has a missing or empty "description"`);
  }

  // Rule 3: the effect/MoveType partition, enforced in BOTH directions. Stance
  // effects {block,guard,shield,counter} are legal ONLY on MoveType 4
  // (Blocking); action effects {damage,aoe,heal,siphon} are legal ONLY on
  // MoveType 0-3. This subsumes and replaces the old `block !== (moveType ===
  // 4)` check now that MoveType 4 generalises from "IS the block marker" to
  // "IS the stance marker".
  if (STANCE_EFFECTS.has(effect) && bp.moveType !== 4) {
    fail(
      `${bp.aid} (${bp.name}) has stance effect "${effect}" on MoveType ${MOVE_TYPE_NAMES[bp.moveType]} ` +
        `— stance effects {${[...STANCE_EFFECTS].join(', ')}} are legal ONLY on MoveType 4 (Blocking)`,
    );
  }
  if (ACTION_EFFECTS.has(effect) && bp.moveType === 4) {
    fail(
      `${bp.aid} (${bp.name}) has action effect "${effect}" on MoveType Blocking ` +
        `— action effects {${[...ACTION_EFFECTS].join(', ')}} are legal ONLY on MoveType 0-3`,
    );
  }

  // Rule 4: max_uses (optional, default 255 = unlimited).
  const maxUses = stat.max_uses ?? 255;
  if (!Number.isInteger(maxUses) || maxUses < 1 || maxUses > 255) {
    fail(`${bp.aid} (${bp.name}) max_uses ${maxUses} outside [1,255] (0 would make the move unusable)`);
  }

  // Rule 5: hits (optional, default 1 = normal; 2 = double-strike).
  const hits = stat.hits ?? 1;
  if (!Number.isInteger(hits) || hits < 1 || hits > 2) {
    fail(`${bp.aid} (${bp.name}) hits ${hits} outside [1,2]`);
  }

  return {
    idx,
    guid: bp.aid,
    name: bp.name,
    quality: bp.quality,
    type: bp.moveType,
    power: stat.power,
    speed: stat.speed,
    cooldown: stat.cooldown,
    effect,
    hits,
    maxUses,
    description: stat.description,
  };
});

// ---- 4a. emit duel/src/stats_gen.h ----
const cppRows = entries
  .map(
    (e) =>
      `    {${e.power}, ${e.speed}, ${e.cooldown}, ${e.type}, ${EFFECT_CPP_SYMBOL[e.effect]}, ${e.hits}, ${e.maxUses}}, // [${e.idx}] ${e.guid} ${e.name} (q${e.quality} ${MOVE_TYPE_NAMES[e.type]})`,
  )
  .join('\n');

const cpp = `// duel/src/stats_gen.h — GENERATED by duel/gen-stats.mjs — edit duel/data/duel-stats.json
//
// Move-stat table + tunables for the duel engine, in dense-index order (index == the
// move's AuthoredId's position in the ASCII sort of all 39 GUIDs in
// proto/roconfig/fightermoveblueprint.pb.text — the wire contract's dense move index).
// Regenerate with \`node duel/gen-stats.mjs\` after editing duel/data/duel-stats.json; this
// file and tf-frontend/src/data/duel-stats.ts are emitted together from the same source so
// they never drift.

#pragma once

#include <cstdint>

namespace duel {

// MoveType (matches database/fighter.hpp pxd::MoveType / the proto MoveType field):
// 0 Heavy, 1 Speedy, 2 Tricky, 3 Distance, 4 Blocking.
//
// Move effects (combat-depth Task 3): stance effects {block,guard,shield,counter}
// are legal ONLY on MoveType 4 (Blocking); action effects {damage,aoe,heal,siphon}
// are legal ONLY on MoveType 0-3. gen-stats.mjs enforces this partition in both
// directions, so an illegal (effect, type) pair can never reach this table.
enum : uint8_t { ${Object.entries(EFFECT_CODES)
  .map(([name, code]) => `${EFFECT_CPP_SYMBOL[name]} = ${code}`)
  .join(', ')} };

struct DuelMoveStat {
  uint16_t power; uint8_t speed; uint8_t cooldown; uint8_t type;
  uint8_t effect;    // kEff*
  uint8_t hits;      // 1 = normal; 2 = a double-strike (a move PROPERTY, never a proc)
  uint8_t max_uses;  // 255 = unlimited; 1-2 = a signature ultimate
};

constexpr DuelMoveStat kDuelMoves[39] = {
${cppRows}
};

struct DuelTunables {
  uint16_t hp_base;
  uint16_t hp_per_quality;
  uint16_t hp_per_sweetness;
  uint16_t adv_mult_256;
  uint16_t dis_mult_256;
  uint16_t block_pct_256;
  uint8_t round_cap;
  uint8_t team_size;
  uint8_t loadout_size;
  // Sudden death (combat-depth Task 2): from round \`sudden_start\` on, the arena
  // collapses -- every LIVING treat takes sudden_step * (playedRound -
  // sudden_start + 1) chip damage at end of round. It escalates past any
  // reachable max_hp well before round_cap, so the cap's Sum-HP tiebreak stops
  // being a win-on-time button (and turtling stops being a strategy).
  uint8_t sudden_start;
  uint8_t sudden_step;
  // combat-depth Task 3: new-verb tunables, wired up by Tasks 4/5/6 (AoE splash
  // %, guard block %, siphon lifesteal %, counter reflect %). Landed now so
  // kDuelMoves[*].effect has something to point at once the resolver reads
  // them; not consumed by any behaviour yet.
  uint16_t aoe_pct_256;
  uint16_t guard_pct_256;
  uint16_t siphon_pct_256;
  uint16_t counter_pct_256;
};

constexpr DuelTunables kTun = {
    ${tunables.hp_base}, ${tunables.hp_per_quality}, ${tunables.hp_per_sweetness},
    ${tunables.adv_mult_256}, ${tunables.dis_mult_256}, ${tunables.block_pct_256},
    ${tunables.round_cap}, ${tunables.team_size}, ${tunables.loadout_size},
    ${tunables.sudden_start}, ${tunables.sudden_step},
    ${tunables.aoe_pct_256}, ${tunables.guard_pct_256}, ${tunables.siphon_pct_256}, ${tunables.counter_pct_256}};

} // namespace duel
`;

// ---- 4b. emit tf-frontend/src/data/duel-stats.ts ----
const tsGuidIndexRows = entries.map((e) => `  '${e.guid}': ${e.idx},`).join('\n');
const tsMoveRows = entries
  .map(
    (e) =>
      `  { guid: '${e.guid}', name: ${JSON.stringify(e.name)}, type: ${e.type}, quality: ${e.quality}, power: ${e.power}, speed: ${e.speed}, cooldown: ${e.cooldown}, effect: '${e.effect}', hits: ${e.hits}, maxUses: ${e.maxUses}, description: ${JSON.stringify(e.description)} },`,
  )
  .join('\n');

const ts = `/**
 * GENERATED by tfgsp-polygon/duel/gen-stats.mjs — edit tfgsp-polygon/duel/data/duel-stats.json
 * (regen with \`node duel/gen-stats.mjs\` from the tfgsp-polygon repo, then commit this file here).
 *
 * TS mirror of duel/src/stats_gen.h. DUEL_MOVES is in dense-index order: index N is the
 * move whose AuthoredId sits at position N in the ASCII sort of all 39 GUIDs in
 * proto/roconfig/fightermoveblueprint.pb.text — that ordering is the duel engine's wire
 * contract for "dense move index" (see docs/superpowers/plans/2026-07-13-duels-prototype.md
 * "Wire formats"). DUEL_MOVE_INDEX is the same mapping keyed the other way (GUID -> index)
 * for encoding a fighter's on-chain moves into engine config bytes.
 */

/** MoveType: 0 Heavy, 1 Speedy, 2 Tricky, 3 Distance, 4 Blocking (matches the GSP's pxd::MoveType). */
export type DuelMoveType = 0 | 1 | 2 | 3 | 4;

/**
 * Move effect (combat-depth Task 3, mirrors kEff* in stats_gen.h). Partitions into
 * stance effects — legal ONLY on type 4 (Blocking) — and action effects — legal ONLY
 * on type 0-3; gen-stats.mjs enforces that split in both directions at codegen time.
 */
export type DuelEffect = 'damage' | 'block' | 'guard' | 'aoe' | 'heal' | 'siphon' | 'shield' | 'counter';

export interface DuelMoveStat {
  guid: string;
  name: string;
  type: DuelMoveType;
  quality: number;
  power: number;
  speed: number;
  cooldown: number;
  effect: DuelEffect;
  /** 1 = normal; 2 = a double-strike (a move PROPERTY, never a proc). */
  hits: number;
  /** 255 = unlimited; 1-2 = a signature ultimate. */
  maxUses: number;
  /** Plain player-facing text: what the move does, in the same breath as its flavour. */
  description: string;
}

/** Move AuthoredId GUID -> dense engine move index. */
export const DUEL_MOVE_INDEX: Record<string, number> = {
${tsGuidIndexRows}
};

/** All 39 duel moves, in dense-index order (array index === engine wire dense index). */
export const DUEL_MOVES: DuelMoveStat[] = [
${tsMoveRows}
];

export interface DuelTunables {
  hp_base: number;
  hp_per_quality: number;
  hp_per_sweetness: number;
  adv_mult_256: number;
  dis_mult_256: number;
  block_pct_256: number;
  round_cap: number;
  team_size: number;
  loadout_size: number;
  /** Sudden death: first round in which the arena chips every living treat (combat-depth Task 2). */
  sudden_start: number;
  /** Sudden-death chip = sudden_step * (playedRound - sudden_start + 1) — escalates, ignores block/shield. */
  sudden_step: number;
  /** combat-depth Task 3/4: AoE splash damage %, of a normal single-target hit's damage. */
  aoe_pct_256: number;
  /** combat-depth Task 3/5: Guard stance block % (stronger than a plain block). */
  guard_pct_256: number;
  /** combat-depth Task 3/5: Siphon lifesteal %, of damage dealt returned as healing. */
  siphon_pct_256: number;
  /** combat-depth Task 3/6: Counter reflect %, of the triggering blow's damage. */
  counter_pct_256: number;
}

export const DUEL_TUNABLES: DuelTunables = ${JSON.stringify(
  Object.fromEntries(TUNABLE_KEYS.map((k) => [k, tunables[k]])),
  null,
  2,
)};
`;

writeFileSync(OUT_CPP, cpp);
writeFileSync(OUT_TS, ts);
console.log(`gen-stats.mjs: wrote ${OUT_CPP}`);
console.log(`gen-stats.mjs: wrote ${OUT_TS}`);

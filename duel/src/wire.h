// duel/src/wire.h — frozen byte-buffer wire formats for the duel engine.
//
// This is the CONTRACT: config (in to duel_init), state (out of
// duel_init/duel_apply), and orders (in to duel_apply). All integers are
// little-endian. Full field-by-field layout and resolve semantics live in
// docs/superpowers/plans/2026-07-13-duels-prototype.md ("Wire formats"
// section) — this header only turns that prose into named constants/offsets
// so later tasks never hardcode magic numbers. Changing any layout here
// requires updating that plan doc first (it says so itself).
//
// Every length below is a function of team_size/loadout_size because the
// prototype's engine parameter allows team_size 1..5 and loadout_size 1..4;
// the plan's three concrete numbers (CFG_LEN 46 / STATE_LEN 152 /
// ORDERS_LEN 13) are for the reference 3v3, 4-move-loadout shape and are
// pinned below via static_assert so a formula typo fails the build, not a
// later test.
//
// v2 (combat-depth Task 1, docs/superpowers/plans/2026-07-14-duel-combat-
// depth.md): the ONLY task allowed to change this byte layout, because once
// game channels sign state bytes the layout is frozen forever. It bumps
// kVersion and grows each per-treat state record with `shield` (a free
// absorb-pool byte), 4 reserved bytes for future status effects (stun/dot/
// atk_mod/spd_mod), and a parallel `uses_left[]` array for limited-use
// ultimates -- every byte later combat-depth tasks will ever need is
// reserved NOW so none of them has to touch this layout again.

#pragma once

#include <cstdint>

namespace duel {
namespace wire {

constexpr uint8_t kVersion = 2;

// ---- bounds (config validation, Task 3) ----
constexpr uint8_t kMinTeamSize = 1;
constexpr uint8_t kMaxTeamSize = 5;
constexpr uint8_t kMinLoadoutSize = 1;
constexpr uint8_t kMaxLoadoutSize = 4;
constexpr uint8_t kMinQuality = 1;
constexpr uint8_t kMaxQuality = 4;
constexpr uint8_t kMinSweetness = 1;
constexpr uint8_t kMaxSweetness = 10;
constexpr uint32_t kMoveUniverse = 39; // dense move-index count (Task 2 codegen)

// ---- sentinel byte values ----
constexpr uint8_t kActionRecover = 0xFE;  // orders.action: free fallback move
constexpr uint8_t kTargetNone = 0xFF;     // orders.target: required for Recover
constexpr uint8_t kMoveSlotEmpty = 0xFF;  // config moves[]: filler past move_count
// orders.target AoE sentinel (a later task hooks this up). NB: numerically
// the same byte as kActionRecover (0xFE), but that sentinel lives on the
// ACTION field -- a different byte of the same orders record -- so there is
// no collision.
constexpr uint8_t kTargetAll = 0xFE;

// ---- state.phase values ----
constexpr uint8_t kPhaseActive = 0;
constexpr uint8_t kPhaseSide0Won = 1;
constexpr uint8_t kPhaseSide1Won = 2;
constexpr uint8_t kPhaseDraw = 3;

// ==================== config (duel_init input) ====================
// u8 version | u8 team_size | u8 loadout_size | u8 reserved=0
// per side (0,1) per slot (0..team_size-1):
//   u8 quality | u8 sweetness | u8 move_count | u8 moves[loadout_size]

constexpr uint32_t kCfgHeaderLen = 4;
constexpr uint32_t kCfgOffVersion = 0;
constexpr uint32_t kCfgOffTeamSize = 1;
constexpr uint32_t kCfgOffLoadoutSize = 2;
constexpr uint32_t kCfgOffReserved = 3;
constexpr uint32_t kCfgOffTeams = kCfgHeaderLen; // start of side-0 data

// Per-treat record, relative offsets.
constexpr uint32_t kCfgTreatOffQuality = 0;
constexpr uint32_t kCfgTreatOffSweetness = 1;
constexpr uint32_t kCfgTreatOffMoveCount = 2;
constexpr uint32_t kCfgTreatOffMoves = 3;
constexpr uint32_t kCfgTreatBaseLen = 3; // quality + sweetness + move_count

constexpr uint32_t CfgTreatLen(uint32_t loadoutSize) {
  return kCfgTreatBaseLen + loadoutSize;
}
constexpr uint32_t CfgSideLen(uint32_t teamSize, uint32_t loadoutSize) {
  return teamSize * CfgTreatLen(loadoutSize);
}
constexpr uint32_t CfgLen(uint32_t teamSize, uint32_t loadoutSize) {
  return kCfgHeaderLen + 2 * CfgSideLen(teamSize, loadoutSize);
}

// ==================== state (duel_init/duel_apply output) ====================
// u8 version | u8 team_size | u8 loadout_size | u8 phase
// u16 round | u16 reserved=0 (v2: the future team energy-pool slot; still
//   canonical-zero today -- no task spends it yet)
// per side per slot:
//   u16 hp | u16 max_hp | u8 quality | u8 sweetness | u8 move_count
//   u8 shield (v2: free absorb-pool byte, 0..255)
//   u8 reserved[4] (v2: MUST be 0 -- future stun/dot/atk_mod/spd_mod)
//   u8 moves[loadout_size] | u8 cooldowns[loadout_size]
//   u8 uses_left[loadout_size] (v2: 255 = unlimited; filler slots
//     (index >= move_count) MUST be 0, same canonical rule as cooldowns)

constexpr uint32_t kStateHeaderLen = 8;
constexpr uint32_t kStateOffVersion = 0;
constexpr uint32_t kStateOffTeamSize = 1;
constexpr uint32_t kStateOffLoadoutSize = 2;
constexpr uint32_t kStateOffPhase = 3;
constexpr uint32_t kStateOffRound = 4;    // u16 LE
constexpr uint32_t kStateOffReserved = 6; // u16 LE
// Same byte offset as kStateOffReserved, under the name this reserved slot
// will carry once a later task spends it (the team energy/action-point
// pool) -- both names exist so existing call sites keep working; validation
// is unchanged (still canonical-zero, see decode_state).
constexpr uint32_t kStateOffEnergyReserved = kStateOffReserved;
constexpr uint32_t kStateOffTeams = kStateHeaderLen;

// Per-treat record, relative offsets.
constexpr uint32_t kStateTreatOffHp = 0;       // u16 LE
constexpr uint32_t kStateTreatOffMaxHp = 2;    // u16 LE
constexpr uint32_t kStateTreatOffQuality = 4;
constexpr uint32_t kStateTreatOffSweetness = 5;
constexpr uint32_t kStateTreatOffMoveCount = 6;
constexpr uint32_t kStateTreatOffShield = 7;   // v2: was kStateTreatOffPad
constexpr uint32_t kStateTreatOffReserved = 8; // v2: 4 bytes, MUST be 0
constexpr uint32_t kStateTreatReservedLen = 4;
constexpr uint32_t kStateTreatOffMoves = 12;   // v2: was 8
constexpr uint32_t kStateTreatBaseLen = 12;    // v2: was 8 (up through reserved[4])

constexpr uint32_t StateTreatOffCooldowns(uint32_t loadoutSize) {
  return kStateTreatOffMoves + loadoutSize;
}
constexpr uint32_t StateTreatOffUsesLeft(uint32_t loadoutSize) {
  return StateTreatOffCooldowns(loadoutSize) + loadoutSize;
}
constexpr uint32_t StateTreatLen(uint32_t loadoutSize) {
  return kStateTreatBaseLen + 3 * loadoutSize; // v2: was 2 * loadoutSize
}
constexpr uint32_t StateSideLen(uint32_t teamSize, uint32_t loadoutSize) {
  return teamSize * StateTreatLen(loadoutSize);
}
constexpr uint32_t StateLen(uint32_t teamSize, uint32_t loadoutSize) {
  return kStateHeaderLen + 2 * StateSideLen(teamSize, loadoutSize);
}

// ==================== round orders (duel_apply input) ====================
// u8 version
// per side per slot: u8 action | u8 target

constexpr uint32_t kOrdersHeaderLen = 1;
constexpr uint32_t kOrdersOffVersion = 0;
constexpr uint32_t kOrdersOffTeams = kOrdersHeaderLen;

constexpr uint32_t kOrdersTreatOffAction = 0;
constexpr uint32_t kOrdersTreatOffTarget = 1;
constexpr uint32_t kOrdersTreatLen = 2;

constexpr uint32_t OrdersSideLen(uint32_t teamSize) {
  return teamSize * kOrdersTreatLen;
}
constexpr uint32_t OrdersLen(uint32_t teamSize) {
  return kOrdersHeaderLen + 2 * OrdersSideLen(teamSize);
}

// ==================== contract pin ====================
// The reference shape: 3v3, 4-move loadout (also the prototype's actual
// tunables in Task 2's duel-stats.json). If a formula edit ever changes
// these, the build breaks here instead of silently drifting from the plan.
constexpr uint32_t kRefTeamSize = 3;
constexpr uint32_t kRefLoadoutSize = 4;
static_assert(CfgLen(kRefTeamSize, kRefLoadoutSize) == 46,
              "CFG_LEN contract pin (3v3/loadout4) broken");
static_assert(StateLen(kRefTeamSize, kRefLoadoutSize) == 152,
              "STATE_LEN contract pin (3v3/loadout4) broken");
static_assert(OrdersLen(kRefTeamSize) == 13,
              "ORDERS_LEN contract pin (3v3/loadout4) broken");

} // namespace wire

// Opaque forward declaration reserved by the wire contract. The real
// per-move stat table and tunables (`DuelMoveStat`, `DuelTunables`) are
// codegen'd into stats_gen.h in Task 2; nothing in this scaffold defines or
// uses DuelStats yet.
struct DuelStats;

} // namespace duel

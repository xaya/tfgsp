// duel/src/engine.cpp — duel engine implementation.
//
// Implements the frozen C ABI (engine.h): duel_init decodes + validates a
// config buffer into an initial state; duel_apply validates a state+orders
// pair and resolves one round (speed ordering, clash pentagon, block stance,
// retarget, cooldowns, win/round-cap), emitting the new state plus that
// round's resolve log as JSON. Both entry points reject any non-canonical
// buffer rather than normalising it (a malleability guard for the channel
// path, where a buffer's bytes are consensus). duel_view remains a reserved
// stub until the channel phase (see its definition below). This file also
// owns the duel_alloc/duel_free scratch arena + duel_out_*/duel_log_* result
// buffers that the ABI sits on.
//
// Allocator scheme (duel_alloc/duel_free): a single static byte arena with
// a bump pointer that WRAPS TO THE START when a request no longer fits.
// `duel_free` is a permanent no-op. This is deliberate, not a placeholder
// to revisit: inputs here are config/state/orders buffers on the order of
// a few hundred bytes (wire.h's frozen shapes; STATE_LEN maxes out at 248
// bytes for team_size=5/loadout_size=4), so a 64 KiB arena holds hundreds
// of calls' worth of inputs before wrapping, and a real malloc/free would
// pull libc allocator machinery into the wasm build for no benefit.
//
// Why wrap-on-full is SAFE (and a hard reset at the start of each engine
// call would NOT be): callers allocate the input buffer(s) for one call,
// write into them, make the call, and never reuse an old allocation for a
// later call — that usage pattern is the documented ABI contract (see
// engine.h). Resetting the arena inside duel_init/duel_apply/duel_view
// would invalidate exactly the buffers the caller just allocated and
// passed INTO that call. Wrapping inside duel_alloc instead can only ever
// clobber allocations from long-dead calls: by the time the bump pointer
// has consumed all 64 KiB, the wrapped-over bytes belong to inputs whose
// calls completed long ago (outputs never live here — they go to the
// separate static g_out/g_log buffers).

#include "engine.h"
#include "jsonout.h"
#include "state.h"
#include "stats_gen.h"
#include "wire.h"

#include <cstdint>

// The `export_name` attribute is only meaningful (and only recognized) when
// targeting wasm32; native builds compile this same file with -Werror, so
// applying it unconditionally would turn "unknown attribute" into a hard
// build failure. `__wasm__` is predefined by the wasi-sdk toolchain.
#if defined(__wasm__)
#define DUEL_ABI(name) extern "C" __attribute__((export_name(name)))
#else
#define DUEL_ABI(name) extern "C"
#endif

// Targeted aliases instead of a TU-scope `using namespace duel;` -- only
// these names are needed outside the `namespace duel` block below.
using duel::DuelState;
using duel::TreatState;
using duel::kTun;
namespace wire = duel::wire;

namespace {

// ---- duel_alloc/duel_free scratch arena ----
// Sized well above any single wire-format buffer (at the max
// team_size=5/loadout_size=4 shape: CFG_LEN 74, STATE_LEN 248,
// ORDERS_LEN 21) to leave generous headroom before wrap-on-full kicks in.
constexpr uint32_t kArenaCap = 1u << 16; // 64 KiB
uint8_t g_arena[kArenaCap];
uint32_t g_arenaUsed = 0;

// ---- duel_out_*/duel_log_* result buffers ----
// Sized for the JSON view/log output (Tasks 3-4); a state buffer is at most
// a few hundred bytes and its JSON rendering a small multiple of that.
constexpr uint32_t kOutCap = 4096;
// Log capacity. Today's worst case is 2*team_size (max 5) = 10 action entries
// + 10 sudden-death chip entries * ~125 B each + the
// {"round":N,"actions":[...],"phase":P} wrapper ≈ 2.5 KB, but the headroom to
// 32 KiB is deliberate: AoE (Task 5) multiplies entries per action, and
// running out of log buffer is NOT a truncation we can ship (the client's
// JSON.parse would throw) — duel_apply hard-rejects on overflow instead, so
// this cap is a consensus-visible number, not a hint.
//
// DUEL_LOG_CAP is a build-time override with exactly ONE user: build.sh's
// duel_tests_logcap binary, which shrinks the cap so the overflow -> -1 reject
// path is reachable by a real duel_apply call (at 32 KiB no legal 5v5 round
// can come close). The shipped native + wasm builds never define it, so both
// compile with the same 32768 and stay byte-identical.
#ifndef DUEL_LOG_CAP
#define DUEL_LOG_CAP 32768
#endif
constexpr uint32_t kLogCap = DUEL_LOG_CAP;
uint8_t g_out[kOutCap];
uint32_t g_outLen = 0;
uint8_t g_log[kLogCap];
uint32_t g_logLen = 0;

// ---- Task 3: shared per-treat validation (config AND state decode use the
// identical quality/sweetness/move_count/moves[] rules -- wire.h's config
// and state formats deliberately share this sub-shape) ----

bool ValidTeamShape(uint32_t teamSize, uint32_t loadoutSize) {
  return teamSize >= wire::kMinTeamSize && teamSize <= wire::kMaxTeamSize &&
         loadoutSize >= wire::kMinLoadoutSize &&
         loadoutSize <= wire::kMaxLoadoutSize;
}

bool ValidTreatHeader(uint32_t quality, uint32_t sweetness, uint32_t moveCount,
                      uint32_t loadoutSize) {
  if (quality < wire::kMinQuality || quality > wire::kMaxQuality) {
    return false;
  }
  if (sweetness < wire::kMinSweetness || sweetness > wire::kMaxSweetness) {
    return false;
  }
  if (moveCount < 1 || moveCount > loadoutSize) {
    return false;
  }
  return true;
}

// moves[0..loadoutSize) must satisfy: entries < moveCount are dense move
// indices (< kMoveUniverse, unique within the treat); entries >= moveCount
// are the 0xFF filler sentinel. moveCount must already be validated
// (1..loadoutSize) by the caller.
bool ValidTreatMoves(const uint8_t* moves, uint32_t loadoutSize,
                     uint32_t moveCount) {
  bool seen[wire::kMoveUniverse] = {};
  for (uint32_t j = 0; j < loadoutSize; ++j) {
    uint8_t mv = moves[j];
    if (j < moveCount) {
      if (mv >= wire::kMoveUniverse || seen[mv]) {
        return false;
      }
      seen[mv] = true;
    } else if (mv != wire::kMoveSlotEmpty) {
      return false;
    }
  }
  return true;
}

// Validates a config buffer (wire.h CFG_*) end to end and fills `out` with
// team_size/loadout_size/phase=active/round=0 plus every treat's
// quality/sweetness/move_count/moves (hp/max_hp computed from kTun,
// cooldowns start at 0). Every read is bounds-checked against `len` before
// use: the length-equality check happens before any per-treat byte is
// touched, so every subsequent offset is guaranteed to be < len. Returns
// false (and leaves *out partially written -- callers must not use it) on
// any wire-contract violation.
bool DecodeConfig(const uint8_t* cfg, uint32_t len, DuelState* out) {
  if (cfg == nullptr || len < wire::kCfgHeaderLen) {
    return false;
  }
  uint8_t version = cfg[wire::kCfgOffVersion];
  uint32_t teamSize = cfg[wire::kCfgOffTeamSize];
  uint32_t loadoutSize = cfg[wire::kCfgOffLoadoutSize];
  if (version != wire::kVersion) {
    return false;
  }
  // Canonical encoding: the reserved byte MUST be 0 (a validating buffer
  // that differs only in ignored bytes would hash/sign differently from the
  // canonical bytes inside a game channel -- malleability hazard).
  if (cfg[wire::kCfgOffReserved] != 0) {
    return false;
  }
  if (!ValidTeamShape(teamSize, loadoutSize)) {
    return false;
  }
  if (len != wire::CfgLen(teamSize, loadoutSize)) {
    return false;
  }

  out->version = wire::kVersion;
  out->team_size = static_cast<uint8_t>(teamSize);
  out->loadout_size = static_cast<uint8_t>(loadoutSize);
  out->phase = wire::kPhaseActive;
  out->round = 0;

  const uint32_t treatLen = wire::CfgTreatLen(loadoutSize);
  const uint32_t sideLen = wire::CfgSideLen(teamSize, loadoutSize);
  for (uint32_t side = 0; side < 2; ++side) {
    for (uint32_t slot = 0; slot < teamSize; ++slot) {
      const uint32_t base = wire::kCfgOffTeams + side * sideLen + slot * treatLen;
      const uint32_t quality = cfg[base + wire::kCfgTreatOffQuality];
      const uint32_t sweetness = cfg[base + wire::kCfgTreatOffSweetness];
      const uint32_t moveCount = cfg[base + wire::kCfgTreatOffMoveCount];
      if (!ValidTreatHeader(quality, sweetness, moveCount, loadoutSize)) {
        return false;
      }
      const uint8_t* moves = cfg + base + wire::kCfgTreatOffMoves;
      if (!ValidTreatMoves(moves, loadoutSize, moveCount)) {
        return false;
      }

      TreatState& t = out->treats[side][slot];
      t.quality = static_cast<uint8_t>(quality);
      t.sweetness = static_cast<uint8_t>(sweetness);
      t.move_count = static_cast<uint8_t>(moveCount);
      // v2 (controller resolution R3): shield/reserved always seed to 0;
      // uses_left seeds 255 (unlimited) for a real slot, 0 (canonical
      // filler) otherwise. NOT kDuelMoves[m].max_uses -- that field doesn't
      // exist until Task 3, which repoints this seed at it (defaulting to
      // 255 for every move today, so this stays byte-identical then too).
      t.shield = 0;
      for (uint32_t k = 0; k < wire::kStateTreatReservedLen; ++k) {
        t.reserved[k] = 0;
      }
      for (uint32_t j = 0; j < loadoutSize; ++j) {
        t.moves[j] = moves[j];
        t.cooldowns[j] = 0;
        t.uses_left[j] = (j < moveCount) ? 255 : 0;
      }
      const uint32_t maxHp = kTun.hp_base + (quality - 1) * kTun.hp_per_quality +
                              sweetness * kTun.hp_per_sweetness;
      t.hp = static_cast<uint16_t>(maxHp);
      t.max_hp = static_cast<uint16_t>(maxHp);
    }
  }
  return true;
}

// ==================== Task 4: round resolve (duel_apply) ====================

constexpr uint8_t kTypeBlocking = 4;   // MoveType Blocking (stats_gen.h enum)
constexpr uint8_t kClashNeutral = 255; // sentinel: target has no move type

// Pentagon advantage (verbatim from src/logic_combat.cpp): does move type `a`
// beat move type `b`? Types: 0 Heavy,1 Speedy,2 Tricky,3 Distance,4 Blocking.
bool TypeBeats(uint8_t a, uint8_t b) {
  switch (a) {
    case 0: return b == 1 || b == 4; // Heavy > Speedy, Blocking
    case 1: return b == 2 || b == 3; // Speedy > Tricky, Distance
    case 2: return b == 0 || b == 4; // Tricky > Heavy, Blocking
    case 3: return b == 0 || b == 2; // Distance > Heavy, Tricky
    case 4: return b == 1 || b == 3; // Blocking > Speedy, Distance
    default: return false;
  }
}

// 384 advantage / 170 disadvantage / 256 neutral (same type, unrelated, or the
// target has no move type — Recover/skip).
uint16_t ClashMult(uint8_t atkType, uint8_t tgtClashType) {
  if (tgtClashType == kClashNeutral) {
    return 256;
  }
  if (TypeBeats(atkType, tgtClashType)) {
    return kTun.adv_mult_256;
  }
  if (TypeBeats(tgtClashType, atkType)) {
    return kTun.dis_mult_256;
  }
  return 256;
}

const char* ClashLabel(uint16_t mult) {
  if (mult == kTun.adv_mult_256) {
    return "adv";
  }
  if (mult == kTun.dis_mult_256) {
    return "dis";
  }
  return "neu";
}

// Append one round-log action entry (schema in the plan's "Log JSON"). `move`
// and `target` carry wire::kMoveSlotEmpty/kTargetNone (255) when absent
// (recover/skip/chip). On a "chip" entry, side/slot are the VICTIM (sudden
// death has no actor). Writes past capacity are dropped, but they latch
// b->overflow -- duel_apply turns that into a hard reject (see kLogCap).
//
// There is no `retargeted` field: fizzle-on-death (combat-depth Task 2)
// deleted the auto-retarget rule that was the only thing that could ever set
// it, and a permanently-false field is not something to freeze into a format
// that game channels will hash and sign.
void WriteLogEntry(duel::JsonBuf* b, bool first, uint32_t side, uint32_t slot,
                   const char* kind, uint32_t move, uint32_t target,
                   const char* clash, uint32_t dmg, bool blocked,
                   uint32_t targetHp, bool ko) {
  if (!first) {
    duel::jb_raw(b, ",");
  }
  duel::jb_raw(b, "{\"side\":");
  duel::jb_u32(b, side);
  duel::jb_raw(b, ",\"slot\":");
  duel::jb_u32(b, slot);
  duel::jb_raw(b, ",\"kind\":");
  duel::jb_str(b, kind);
  duel::jb_raw(b, ",\"move\":");
  duel::jb_u32(b, move);
  duel::jb_raw(b, ",\"target\":");
  duel::jb_u32(b, target);
  duel::jb_raw(b, ",\"clash\":");
  duel::jb_str(b, clash);
  duel::jb_raw(b, ",\"dmg\":");
  duel::jb_u32(b, dmg);
  duel::jb_raw(b, ",\"blocked\":");
  duel::jb_raw(b, blocked ? "true" : "false");
  duel::jb_raw(b, ",\"targetHp\":");
  duel::jb_u32(b, targetHp);
  duel::jb_raw(b, ",\"ko\":");
  duel::jb_raw(b, ko ? "true" : "false");
  duel::jb_raw(b, "}");
}

} // namespace

namespace duel {

// ---- Task 3: state wire codec, reused by Task 4's duel_apply/duel_view ----

uint32_t encode_state(const DuelState& s, uint8_t* out, uint32_t cap) {
  const uint32_t total = wire::StateLen(s.team_size, s.loadout_size);
  if (out == nullptr || cap < total) {
    return 0; // no partial write
  }
  out[wire::kStateOffVersion] = s.version;
  out[wire::kStateOffTeamSize] = s.team_size;
  out[wire::kStateOffLoadoutSize] = s.loadout_size;
  out[wire::kStateOffPhase] = s.phase;
  out[wire::kStateOffRound] = static_cast<uint8_t>(s.round & 0xFFu);
  out[wire::kStateOffRound + 1] = static_cast<uint8_t>((s.round >> 8) & 0xFFu);
  out[wire::kStateOffReserved] = 0;
  out[wire::kStateOffReserved + 1] = 0;

  const uint32_t teamSize = s.team_size;
  const uint32_t loadoutSize = s.loadout_size;
  const uint32_t treatLen = wire::StateTreatLen(loadoutSize);
  const uint32_t sideLen = wire::StateSideLen(teamSize, loadoutSize);
  for (uint32_t side = 0; side < 2; ++side) {
    for (uint32_t slot = 0; slot < teamSize; ++slot) {
      const TreatState& t = s.treats[side][slot];
      const uint32_t base = wire::kStateOffTeams + side * sideLen + slot * treatLen;
      out[base + wire::kStateTreatOffHp] = static_cast<uint8_t>(t.hp & 0xFFu);
      out[base + wire::kStateTreatOffHp + 1] =
          static_cast<uint8_t>((t.hp >> 8) & 0xFFu);
      out[base + wire::kStateTreatOffMaxHp] = static_cast<uint8_t>(t.max_hp & 0xFFu);
      out[base + wire::kStateTreatOffMaxHp + 1] =
          static_cast<uint8_t>((t.max_hp >> 8) & 0xFFu);
      out[base + wire::kStateTreatOffQuality] = t.quality;
      out[base + wire::kStateTreatOffSweetness] = t.sweetness;
      out[base + wire::kStateTreatOffMoveCount] = t.move_count;
      out[base + wire::kStateTreatOffShield] = t.shield;
      // Hard-zero on encode, regardless of what the struct holds: reserved is
      // canonical-MUST-be-0 (decode_state rejects any non-zero byte here), so
      // encode_state must never be able to emit a value it would itself
      // reject -- correctness must not depend on every TreatState producer
      // (decode paths today, but not statically enforced) having zeroed it.
      for (uint32_t k = 0; k < wire::kStateTreatReservedLen; ++k) {
        out[base + wire::kStateTreatOffReserved + k] = 0;
      }
      const uint32_t movesOff = base + wire::kStateTreatOffMoves;
      const uint32_t cdOff = base + wire::StateTreatOffCooldowns(loadoutSize);
      const uint32_t ulOff = base + wire::StateTreatOffUsesLeft(loadoutSize);
      for (uint32_t j = 0; j < loadoutSize; ++j) {
        out[movesOff + j] = t.moves[j];
        // Same hard-zero rule for filler cooldown/uses_left slots (index >=
        // move_count): canonical-MUST-be-0, so encode never trusts the
        // struct to already hold 0 there.
        out[cdOff + j] = (j < t.move_count) ? t.cooldowns[j] : 0;
        out[ulOff + j] = (j < t.move_count) ? t.uses_left[j] : 0;
      }
    }
  }
  return total;
}

bool decode_state(const uint8_t* buf, uint32_t len, DuelState* out) {
  if (buf == nullptr || out == nullptr || len < wire::kStateHeaderLen) {
    return false;
  }
  const uint8_t version = buf[wire::kStateOffVersion];
  const uint32_t teamSize = buf[wire::kStateOffTeamSize];
  const uint32_t loadoutSize = buf[wire::kStateOffLoadoutSize];
  const uint8_t phase = buf[wire::kStateOffPhase];
  if (version != wire::kVersion) {
    return false;
  }
  if (!ValidTeamShape(teamSize, loadoutSize)) {
    return false;
  }
  if (phase > wire::kPhaseDraw) {
    return false;
  }
  if (len != wire::StateLen(teamSize, loadoutSize)) {
    return false;
  }
  // Canonical encoding: the reserved u16 MUST be 0 (see DecodeConfig -- the
  // per-treat pad bytes get the same treatment in the loop below).
  if (buf[wire::kStateOffReserved] != 0 || buf[wire::kStateOffReserved + 1] != 0) {
    return false;
  }

  const uint16_t round = static_cast<uint16_t>(
      buf[wire::kStateOffRound] |
      (static_cast<uint16_t>(buf[wire::kStateOffRound + 1]) << 8));
  // An ACTIVE duel at or past the round cap is unreachable from duel_init +
  // duel_apply (apply always sets a terminal phase once round >= round_cap),
  // so accepting one would only ever admit a CRAFTED state -- and sudden
  // death (combat-depth Task 2) makes that dangerous: the chip term scales
  // with (round - sudden_start), so a forged round = 60000 would one-shot the
  // whole arena. Reject it here, at the only door into the engine. (A
  // terminal-phase state AT the cap is the legitimate end state of a
  // round-cap duel and still decodes -- duel_apply rejects it on `phase !=
  // active` instead.)
  if (phase == wire::kPhaseActive && round >= kTun.round_cap) {
    return false;
  }

  out->version = version;
  out->team_size = static_cast<uint8_t>(teamSize);
  out->loadout_size = static_cast<uint8_t>(loadoutSize);
  out->phase = phase;
  out->round = round;

  const uint32_t treatLen = wire::StateTreatLen(loadoutSize);
  const uint32_t sideLen = wire::StateSideLen(teamSize, loadoutSize);
  for (uint32_t side = 0; side < 2; ++side) {
    for (uint32_t slot = 0; slot < teamSize; ++slot) {
      const uint32_t base = wire::kStateOffTeams + side * sideLen + slot * treatLen;
      const uint32_t quality = buf[base + wire::kStateTreatOffQuality];
      const uint32_t sweetness = buf[base + wire::kStateTreatOffSweetness];
      const uint32_t moveCount = buf[base + wire::kStateTreatOffMoveCount];
      if (!ValidTreatHeader(quality, sweetness, moveCount, loadoutSize)) {
        return false;
      }
      // Canonical encoding (v2): each of the 4 reserved bytes MUST be 0 --
      // they're future stun/dot/atk_mod/spd_mod slots with no meaning yet,
      // so a non-zero value would be a second encoding of the same logical
      // state (same malleability rule as the header reserved u16 above).
      // `shield` (the byte this used to be a MUST-be-zero pad at) is a free
      // byte instead -- no check, any value decodes.
      for (uint32_t k = 0; k < wire::kStateTreatReservedLen; ++k) {
        if (buf[base + wire::kStateTreatOffReserved + k] != 0) {
          return false;
        }
      }
      const uint8_t* moves = buf + base + wire::kStateTreatOffMoves;
      if (!ValidTreatMoves(moves, loadoutSize, moveCount)) {
        return false;
      }

      TreatState& t = out->treats[side][slot];
      t.hp = static_cast<uint16_t>(
          buf[base + wire::kStateTreatOffHp] |
          (static_cast<uint16_t>(buf[base + wire::kStateTreatOffHp + 1]) << 8));
      t.max_hp = static_cast<uint16_t>(
          buf[base + wire::kStateTreatOffMaxHp] |
          (static_cast<uint16_t>(buf[base + wire::kStateTreatOffMaxHp + 1])
           << 8));
      t.quality = static_cast<uint8_t>(quality);
      t.sweetness = static_cast<uint8_t>(sweetness);
      t.move_count = static_cast<uint8_t>(moveCount);
      t.shield = buf[base + wire::kStateTreatOffShield]; // free byte, no validation
      for (uint32_t k = 0; k < wire::kStateTreatReservedLen; ++k) {
        t.reserved[k] = buf[base + wire::kStateTreatOffReserved + k]; // == 0, checked above
      }
      const uint32_t cdOff = base + wire::StateTreatOffCooldowns(loadoutSize);
      const uint32_t ulOff = base + wire::StateTreatOffUsesLeft(loadoutSize);
      for (uint32_t j = 0; j < loadoutSize; ++j) {
        const uint8_t cd = buf[cdOff + j];
        if (j >= moveCount && cd != 0) {
          // Canonical encoding: a filler slot has no move, so its cooldown
          // byte is meaningless -- it MUST be 0, or two byte-distinct
          // encodings of the same logical state would both decode (and then
          // resolve/re-encode differently once the end-of-round decrement
          // ticks the copied value). Same malleability rule as reserved/filler.
          return false;
        }
        const uint8_t ul = buf[ulOff + j];
        if (j >= moveCount && ul != 0) {
          // v2 (controller resolution R4): the identical rule for
          // uses_left -- a filler slot holds no move, so its uses_left byte
          // is equally meaningless and MUST be 0.
          return false;
        }
        t.moves[j] = moves[j];
        t.cooldowns[j] = cd;
        t.uses_left[j] = ul; // real slot: free byte, any value decodes
      }
    }
  }
  return true;
}

} // namespace duel

DUEL_ABI("duel_init")
int32_t duel_init(const uint8_t* cfg, uint32_t len) {
  g_outLen = 0;
  DuelState state{};
  if (!DecodeConfig(cfg, len, &state)) {
    return -1;
  }
  const uint32_t written = duel::encode_state(state, g_out, kOutCap);
  if (written == 0) {
    return -1;
  }
  g_outLen = written;
  return 0;
}

DUEL_ABI("duel_apply")
int32_t duel_apply(const uint8_t* st, uint32_t stLen, const uint8_t* ord,
                    uint32_t ordLen) {
  g_outLen = 0;
  g_logLen = 0;

  // ---- semantics 1: validate everything (reject leaves state unchanged) ----
  DuelState s{};
  if (!duel::decode_state(st, stLen, &s)) {
    return -1;
  }
  if (s.phase != wire::kPhaseActive) {
    return -1; // a finished duel takes no further rounds
  }

  const uint32_t teamSize = s.team_size;
  const uint32_t loadoutSize = s.loadout_size;

  if (ord == nullptr || ordLen != wire::OrdersLen(teamSize) ||
      ord[wire::kOrdersOffVersion] != wire::kVersion) {
    return -1;
  }

  uint8_t action[2][wire::kMaxTeamSize];
  uint8_t target[2][wire::kMaxTeamSize];
  for (uint32_t side = 0; side < 2; ++side) {
    for (uint32_t slot = 0; slot < teamSize; ++slot) {
      const uint32_t off = wire::kOrdersOffTeams +
                           side * wire::OrdersSideLen(teamSize) +
                           slot * wire::kOrdersTreatLen;
      const uint8_t a = ord[off + wire::kOrdersTreatOffAction];
      const uint8_t t = ord[off + wire::kOrdersTreatOffTarget];
      const TreatState& tr = s.treats[side][slot];
      if (tr.hp == 0) {
        // KO'd treat: canonical Recover encoding only.
        if (a != wire::kActionRecover || t != wire::kTargetNone) {
          return -1;
        }
      } else if (a == wire::kActionRecover) {
        if (t != wire::kTargetNone) {
          return -1;
        }
      } else {
        // A real move: a ready loadout index aimed at a real enemy slot.
        if (a >= tr.move_count || tr.cooldowns[a] != 0 || t >= teamSize) {
          return -1;
        }
      }
      action[side][slot] = a;
      target[side][slot] = t;
    }
  }

  // ---- semantics 2: block stances (living treat whose action is a real
  // Blocking move; fixed for the whole round -- a dead treat is never a valid
  // hit target, so "ends if KO'd mid-round" falls out for free) ----
  bool blockStance[2][wire::kMaxTeamSize] = {};
  for (uint32_t side = 0; side < 2; ++side) {
    for (uint32_t slot = 0; slot < teamSize; ++slot) {
      const TreatState& tr = s.treats[side][slot];
      if (tr.hp > 0 && action[side][slot] != wire::kActionRecover &&
          duel::kDuelMoves[tr.moves[action[side][slot]]].type == kTypeBlocking) {
        blockStance[side][slot] = true;
      }
    }
  }

  // ---- semantics 3: order actions by (speed DESC, THIS ROUND'S FIRST SIDE,
  // slot ASC). Recover / KO'd actions have speed 0.
  //
  // The side tie-break ALTERNATES with the round parity. A fixed `side ASC`
  // tie-break (what this used to be) handed side 0 every speed tie in every
  // round of every duel -- a systematic, permanent advantage, and in a mirror
  // match the only thing separating the two teams. Alternating it makes the
  // first-strike edge a shared resource instead of a birthright.
  //
  // The order stays TOTAL and deterministic: (side, slot) is unique, so ties
  // always bottom out in a strict comparison -- equal speed and equal side
  // means the same treat. ----
  const uint32_t firstSide = s.round & 1u; // round 0 -> side 0, round 1 -> side 1, ...
  uint8_t speed[2][wire::kMaxTeamSize];
  const uint32_t nActions = 2 * teamSize;
  uint8_t ordSide[2 * wire::kMaxTeamSize];
  uint8_t ordSlot[2 * wire::kMaxTeamSize];
  {
    uint32_t idx = 0;
    for (uint32_t side = 0; side < 2; ++side) {
      for (uint32_t slot = 0; slot < teamSize; ++slot) {
        const TreatState& tr = s.treats[side][slot];
        uint8_t sp = 0;
        if (tr.hp > 0 && action[side][slot] != wire::kActionRecover) {
          sp = duel::kDuelMoves[tr.moves[action[side][slot]]].speed;
        }
        speed[side][slot] = sp;
        ordSide[idx] = static_cast<uint8_t>(side);
        ordSlot[idx] = static_cast<uint8_t>(slot);
        ++idx;
      }
    }
    // Insertion sort (n <= 10): deterministic, no libc.
    for (uint32_t i = 1; i < nActions; ++i) {
      const uint8_t cs = ordSide[i], cl = ordSlot[i];
      const uint8_t csp = speed[cs][cl];
      uint32_t j = i;
      while (j > 0) {
        const uint8_t ps = ordSide[j - 1], pl = ordSlot[j - 1];
        const uint8_t psp = speed[ps][pl];
        const bool prevSideFirst = (ps == firstSide);
        const bool prevBefore =
            (psp > csp) ||
            (psp == csp && ((ps != cs) ? prevSideFirst : (pl < cl)));
        if (prevBefore) {
          break;
        }
        ordSide[j] = ps;
        ordSlot[j] = pl;
        --j;
      }
      ordSide[j] = cs;
      ordSlot[j] = cl;
    }
  }

  // ---- semantics 4: resolve each action in order, appending a log entry ----
  const uint16_t playedRound = s.round;
  duel::JsonBuf log{g_log, kLogCap, 0};
  uint32_t nEntries = 0; // only the FIRST entry omits its leading comma
  duel::jb_raw(&log, "{\"round\":");
  duel::jb_u32(&log, playedRound);
  duel::jb_raw(&log, ",\"actions\":[");

  for (uint32_t i = 0; i < nActions; ++i) {
    const uint32_t aS = ordSide[i];
    const uint32_t aL = ordSlot[i];
    TreatState& actor = s.treats[aS][aL];
    const uint8_t act = action[aS][aL];

    if (actor.hp == 0) {
      const uint32_t mv = (act == wire::kActionRecover) ? wire::kMoveSlotEmpty
                                                        : actor.moves[act];
      WriteLogEntry(&log, nEntries++ == 0, aS, aL, "skip", mv,
                    wire::kTargetNone, "neu", 0, false, 0, false);
      continue;
    }
    if (act == wire::kActionRecover) {
      WriteLogEntry(&log, nEntries++ == 0, aS, aL, "recover",
                    wire::kMoveSlotEmpty, wire::kTargetNone, "neu", 0, false, 0,
                    false);
      continue;
    }

    const uint8_t mvIdx = actor.moves[act];
    const uint32_t enemy = 1u - aS;
    const uint32_t tgtSlot = target[aS][aL];

    // FIZZLE-ON-DEATH (combat-depth Task 2). The blow was aimed at THAT treat;
    // if it is already dead when the swing lands, the swing WHIFFS. It is not
    // redirected onto some other enemy, and the overkill is not refunded --
    // the engine used to auto-retarget onto the lowest-hp living enemy, which
    // made over-committing free and turned target selection into a lookup
    // ("everyone hit the lowest HP") rather than a decision. Now committing a
    // second attacker to a treat that might already be dead is a BET, and any
    // enemy defensive action is a potential BAIT.
    //
    // The whiff still burns the move's cooldown -- semantics 5 below ticks off
    // `action[side][slot]`, not off whether the action connected -- so
    // over-committing costs you the turn AND the move.
    if (s.treats[enemy][tgtSlot].hp == 0) {
      WriteLogEntry(&log, nEntries++ == 0, aS, aL, "skip", mvIdx,
                    wire::kTargetNone, "neu", 0, false, 0, false);
      continue;
    }

    TreatState& tgt = s.treats[enemy][tgtSlot];
    const uint8_t atkType = duel::kDuelMoves[mvIdx].type;
    uint8_t tgtClash = kClashNeutral;
    const uint8_t tgtAct = action[enemy][tgtSlot];
    if (tgtAct != wire::kActionRecover) {
      tgtClash = duel::kDuelMoves[tgt.moves[tgtAct]].type;
    }
    const uint16_t mult = ClashMult(atkType, tgtClash);
    uint32_t dmg = (static_cast<uint32_t>(duel::kDuelMoves[mvIdx].power) * mult) >> 8;
    const bool blocked = blockStance[enemy][tgtSlot];
    if (blocked) {
      dmg = (dmg * (256u - kTun.block_pct_256)) >> 8;
    }
    const uint16_t newHp =
        (tgt.hp > dmg) ? static_cast<uint16_t>(tgt.hp - dmg) : 0;
    tgt.hp = newHp;
    const bool ko = (newHp == 0);
    WriteLogEntry(&log, nEntries++ == 0, aS, aL, "hit", mvIdx, tgtSlot,
                  ClashLabel(mult), dmg, blocked, newHp, ko);
  }

  // ---- semantics 5: end-of-round cooldowns (living treats only) ----
  for (uint32_t side = 0; side < 2; ++side) {
    for (uint32_t slot = 0; slot < teamSize; ++slot) {
      TreatState& tr = s.treats[side][slot];
      if (tr.hp == 0) {
        continue; // KO'd treats are out -- cooldowns untouched
      }
      const uint8_t act = action[side][slot];
      for (uint32_t j = 0; j < loadoutSize; ++j) {
        if (act != wire::kActionRecover && j == act) {
          tr.cooldowns[j] = duel::kDuelMoves[tr.moves[act]].cooldown;
        } else if (tr.cooldowns[j] > 0) {
          tr.cooldowns[j] = static_cast<uint8_t>(tr.cooldowns[j] - 1);
        }
      }
    }
  }

  // ---- semantics 5b: SUDDEN DEATH (combat-depth Task 2) ----
  //
  // From round `sudden_start` on, the arena itself starts collapsing: every
  // LIVING treat takes an escalating chip at end of round. This is what
  // guarantees the duel ENDS -- and, more importantly, what stops turtling
  // from becoming the meta: the round cap's tiebreak is by HP-sum, which turns
  // into a win-on-time button the moment healing exists (Task 6). With the
  // authored 12/6, the chip sums past the largest reachable max_hp (290) by
  // round 21, so the cap is effectively unreachable.
  //
  // It is NOT an attack -- it is the arena. It ignores the clash pentagon,
  // block stance, shield and guard alike. TASK 4 MUST NOT MAKE `shield` ABSORB
  // IT; there is nothing here to absorb.
  if (playedRound >= kTun.sudden_start) {
    const uint32_t chip =
        static_cast<uint32_t>(kTun.sudden_step) *
        (static_cast<uint32_t>(playedRound) - kTun.sudden_start + 1u);
    for (uint32_t side = 0; side < 2; ++side) {
      for (uint32_t slot = 0; slot < teamSize; ++slot) {
        TreatState& tr = s.treats[side][slot];
        if (tr.hp == 0) {
          continue; // KO is permanent: a dead treat takes no chip and logs none
        }
        const uint16_t newHp =
            (tr.hp > chip) ? static_cast<uint16_t>(tr.hp - chip) : 0;
        tr.hp = newHp;
        // The victim carries the entry's side/slot -- a chip has no actor.
        WriteLogEntry(&log, nEntries++ == 0, side, slot, "chip",
                      wire::kMoveSlotEmpty, wire::kTargetNone, "neu", chip,
                      false, newHp, newHp == 0);
      }
    }
  }

  // ---- semantics 6: advance round, decide winner (on the POST-chip hp) ----
  s.round = static_cast<uint16_t>(s.round + 1);
  bool side0Dead = true, side1Dead = true;
  uint32_t sum0 = 0, sum1 = 0;
  for (uint32_t slot = 0; slot < teamSize; ++slot) {
    if (s.treats[0][slot].hp > 0) side0Dead = false;
    if (s.treats[1][slot].hp > 0) side1Dead = false;
    sum0 += s.treats[0][slot].hp;
    sum1 += s.treats[1][slot].hp;
  }
  if (side0Dead && side1Dead) {
    // Both teams wiped in the same round. Only sudden death can do this (an
    // action can't be taken by a treat that's already dead, so a round of
    // ATTACKS can never wipe both sides), and it must be a DRAW: awarding it
    // to side 0 -- which is what checking side1Dead first would do -- hands
    // side 0 every symmetric mirror match, the exact bias the round-alternating
    // turn order above exists to remove.
    s.phase = wire::kPhaseDraw;
  } else if (side1Dead) {
    s.phase = wire::kPhaseSide0Won; // enemy of side0 is all dead
  } else if (side0Dead) {
    s.phase = wire::kPhaseSide1Won;
  } else if (s.round >= kTun.round_cap) {
    s.phase = (sum0 > sum1)   ? wire::kPhaseSide0Won
              : (sum1 > sum0) ? wire::kPhaseSide1Won
                              : wire::kPhaseDraw;
  }

  duel::jb_raw(&log, "],\"phase\":");
  duel::jb_u32(&log, s.phase);
  duel::jb_raw(&log, "}");

  // The log did not fit. jsonout.h drops past-cap writes, so what is in the
  // buffer now is TRUNCATED JSON -- the client's JSON.parse(readLog()) would
  // throw on it. Reject the round instead, BEFORE committing any output, so a
  // rejected round leaves the caller's state exactly as it was (the same
  // contract as every other reject path in this function).
  if (log.overflow) {
    g_outLen = 0;
    g_logLen = 0;
    return -1;
  }

  const uint32_t written = duel::encode_state(s, g_out, kOutCap);
  if (written == 0) {
    return -1; // unreachable for a decoded (valid-shape) state; stay safe
  }
  g_outLen = written;
  g_logLen = log.len;
  return 0;
}

// RESERVED — unimplemented stub until the channel phase. The signature is a
// frozen ABI slot (kept now so the export table never changes), but the body
// just hard-rejects (-1, duel_out_len cleared) — no view render exists yet.
// The web client decodes state client-side instead (tf-frontend
// src/duel/wire.ts decodeState), so phase 1 needs no engine-side view.
DUEL_ABI("duel_view")
int32_t duel_view(const uint8_t* st, uint32_t stLen) {
  (void)st;
  (void)stLen;
  g_outLen = 0;
  return -1;
}

DUEL_ABI("duel_out_ptr")
const uint8_t* duel_out_ptr(void) {
  return g_out;
}

DUEL_ABI("duel_out_len")
uint32_t duel_out_len(void) {
  return g_outLen;
}

DUEL_ABI("duel_log_ptr")
const uint8_t* duel_log_ptr(void) {
  return g_log;
}

DUEL_ABI("duel_log_len")
uint32_t duel_log_len(void) {
  return g_logLen;
}

DUEL_ABI("duel_alloc")
uint8_t* duel_alloc(uint32_t n) {
  if (n == 0 || n > kArenaCap) {
    return nullptr;
  }
  if (n > kArenaCap - g_arenaUsed) {
    // Wrap-on-full: recycle the arena from the start. Safe because callers
    // allocate all inputs for a call together, make the call, and never
    // reuse an allocation across calls (documented ABI contract, engine.h)
    // — anything this overwrites belongs to calls that finished long ago.
    g_arenaUsed = 0;
  }
  uint8_t* p = g_arena + g_arenaUsed;
  g_arenaUsed += n;
  return p;
}

DUEL_ABI("duel_free")
void duel_free(uint8_t* p) {
  (void)p; // bump allocator: free is a permanent no-op, see file header.
}

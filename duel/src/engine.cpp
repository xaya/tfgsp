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
// a few hundred bytes (wire.h's frozen shapes; STATE_LEN maxes out at 168
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
// team_size=5/loadout_size=4 shape: CFG_LEN 74, STATE_LEN 168,
// ORDERS_LEN 21) to leave generous headroom before wrap-on-full kicks in.
constexpr uint32_t kArenaCap = 1u << 16; // 64 KiB
uint8_t g_arena[kArenaCap];
uint32_t g_arenaUsed = 0;

// ---- duel_out_*/duel_log_* result buffers ----
// Sized for the JSON view/log output (Tasks 3-4); a state buffer is at most
// a few hundred bytes and its JSON rendering a small multiple of that.
constexpr uint32_t kOutCap = 4096;
// Worst case: 2*team_size (max 5) = 10 action entries * ~146 B each + the
// {"round":N,"actions":[...],"phase":P} wrapper ≈ 1.5 KB « 8192.
constexpr uint32_t kLogCap = 8192;
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
      for (uint32_t j = 0; j < loadoutSize; ++j) {
        t.moves[j] = moves[j];
        t.cooldowns[j] = 0;
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
// (recover/skip). Bounds-checked writes drop silently past capacity.
void WriteLogEntry(duel::JsonBuf* b, bool first, uint32_t side, uint32_t slot,
                   const char* kind, uint32_t move, uint32_t target,
                   bool retargeted, const char* clash, uint32_t dmg,
                   bool blocked, uint32_t targetHp, bool ko) {
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
  duel::jb_raw(b, ",\"retargeted\":");
  duel::jb_raw(b, retargeted ? "true" : "false");
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
      out[base + wire::kStateTreatOffPad] = 0;
      const uint32_t movesOff = base + wire::kStateTreatOffMoves;
      const uint32_t cdOff = base + wire::StateTreatOffCooldowns(loadoutSize);
      for (uint32_t j = 0; j < loadoutSize; ++j) {
        out[movesOff + j] = t.moves[j];
        out[cdOff + j] = t.cooldowns[j];
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

  out->version = version;
  out->team_size = static_cast<uint8_t>(teamSize);
  out->loadout_size = static_cast<uint8_t>(loadoutSize);
  out->phase = phase;
  out->round = static_cast<uint16_t>(
      buf[wire::kStateOffRound] |
      (static_cast<uint16_t>(buf[wire::kStateOffRound + 1]) << 8));

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
      if (buf[base + wire::kStateTreatOffPad] != 0) {
        return false; // canonical encoding: pad byte MUST be 0
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
      const uint32_t cdOff = base + wire::StateTreatOffCooldowns(loadoutSize);
      for (uint32_t j = 0; j < loadoutSize; ++j) {
        const uint8_t cd = buf[cdOff + j];
        if (j >= moveCount && cd != 0) {
          // Canonical encoding: a filler slot has no move, so its cooldown
          // byte is meaningless -- it MUST be 0, or two byte-distinct
          // encodings of the same logical state would both decode (and then
          // resolve/re-encode differently once the end-of-round decrement
          // ticks the copied value). Same malleability rule as pad/filler.
          return false;
        }
        t.moves[j] = moves[j];
        t.cooldowns[j] = cd;
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

  // ---- semantics 3: order actions by (speed DESC, side ASC, slot ASC).
  // Recover / KO'd actions have speed 0. (side, slot) is unique so the order
  // is total and deterministic. ----
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
        const bool prevBefore = (psp > csp) ||
                                (psp == csp &&
                                 (ps < cs || (ps == cs && pl < cl)));
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
  duel::jb_raw(&log, "{\"round\":");
  duel::jb_u32(&log, playedRound);
  duel::jb_raw(&log, ",\"actions\":[");

  for (uint32_t i = 0; i < nActions; ++i) {
    const uint32_t aS = ordSide[i];
    const uint32_t aL = ordSlot[i];
    const bool first = (i == 0);
    TreatState& actor = s.treats[aS][aL];
    const uint8_t act = action[aS][aL];

    if (actor.hp == 0) {
      const uint32_t mv = (act == wire::kActionRecover) ? wire::kMoveSlotEmpty
                                                        : actor.moves[act];
      WriteLogEntry(&log, first, aS, aL, "skip", mv, wire::kTargetNone, false,
                    "neu", 0, false, 0, false);
      continue;
    }
    if (act == wire::kActionRecover) {
      WriteLogEntry(&log, first, aS, aL, "recover", wire::kMoveSlotEmpty,
                    wire::kTargetNone, false, "neu", 0, false, 0, false);
      continue;
    }

    const uint8_t mvIdx = actor.moves[act];
    const uint32_t enemy = 1u - aS;
    uint32_t tgtSlot = target[aS][aL];
    bool retargeted = false;
    if (s.treats[enemy][tgtSlot].hp == 0) {
      // Retarget to the living enemy with lowest hp (tie: lowest slot).
      int best = -1;
      for (uint32_t k = 0; k < teamSize; ++k) {
        if (s.treats[enemy][k].hp > 0 &&
            (best < 0 ||
             s.treats[enemy][k].hp < s.treats[enemy][best].hp)) {
          best = static_cast<int>(k);
        }
      }
      if (best < 0) {
        // No living enemy: the move whiffs.
        WriteLogEntry(&log, first, aS, aL, "skip", mvIdx, wire::kTargetNone,
                      false, "neu", 0, false, 0, false);
        continue;
      }
      retargeted = (static_cast<uint32_t>(best) != tgtSlot);
      tgtSlot = static_cast<uint32_t>(best);
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
    WriteLogEntry(&log, first, aS, aL, "hit", mvIdx, tgtSlot, retargeted,
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

  // ---- semantics 6: advance round, decide winner ----
  s.round = static_cast<uint16_t>(s.round + 1);
  bool side0Dead = true, side1Dead = true;
  uint32_t sum0 = 0, sum1 = 0;
  for (uint32_t slot = 0; slot < teamSize; ++slot) {
    if (s.treats[0][slot].hp > 0) side0Dead = false;
    if (s.treats[1][slot].hp > 0) side1Dead = false;
    sum0 += s.treats[0][slot].hp;
    sum1 += s.treats[1][slot].hp;
  }
  if (side1Dead) {
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

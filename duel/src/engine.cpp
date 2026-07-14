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
//
// LOG CAPACITY -- the real worst-case arithmetic, recomputed for AoE
// (combat-depth Task 5; the plan's "10 actors x 5 victims = 50 entries" was an
// UNDERCOUNT -- it forgot that every splashed victim can also COUNTER).
//
// Longest possible entry (WriteLogEntry's schema, every field at its widest --
// kind "counter"/"recover" = 9 bytes with quotes, move/target/via/absorbed <=
// 255 = 3 digits, dmg <= 108 (the round-29 sudden-death chip) = 3, targetHp <=
// 290 = 3, clash "adv" = 5, blocked/ko "false" = 5), plus the comma that
// separates it from the previous entry:
//   {"side":0,"slot":0,"kind":"counter","move":255,"target":255,"via":255,
//    "clash":"adv","dmg":108,"blocked":false,"absorbed":255,"targetHp":290,
//    "ko":false}                                          = 151 B  (+1 comma)
//                                                         = 152 B / entry
// (Cross-check: a 1v1 recover-only round measures 325 B = 2 x 145 + 1 comma +
// the 34 B {"round":N,"actions":[ ... ],"phase":P} wrapper. The formula holds.)
//
// Worst case at the biggest legal shape (team_size 5, i.e. 2 x 5 = 10 actors),
// as a strict OVER-count -- assume every actor is an AoE that splashes all 5
// enemies AND every one of those 5 victims survives and ripostes:
//   10 actors x (5 aoe entries + 5 counter entries)      = 100 entries
//   + one sudden-death chip per living treat (10)        =  10 entries
//                                                        = 110 entries
//   110 x 152 B + the 34 B wrapper                       = 16,754 B
// -- half of the 32 KiB cap, so a legal round can never reach it. (The truly
// reachable maximum is lower still: a treat's move is either an AoE or a
// counter, never both, so the peak is 5 AoE casters x (5 + 5) + 5 single swings
// + 10 chips = 65 entries ~ 9.9 KB. The over-count is what we size against.)
// static_assert'd below, against the same formula the code uses.
//
// Running out of log buffer is NOT a truncation we can ship (the client's
// JSON.parse would throw) -- duel_apply hard-rejects on overflow instead, so
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

// The worst case above, as code. Asserted against the SHIPPED 32768 rather than
// kLogCap, because duel_tests_logcap compiles this same file with a deliberately
// tiny kLogCap (that is the whole point of it) and must still build.
constexpr uint32_t kLogShippedCap = 32768;
constexpr uint32_t kLogMaxEntryLen = 152;   // widest entry + its separating comma
constexpr uint32_t kLogWrapperLen = 34;     // {"round":N,"actions":[ ... ],"phase":P}
constexpr uint32_t kLogActors = 2 * wire::kMaxTeamSize;          // 10 at team_size 5
constexpr uint32_t kLogEntriesPerActor = 2 * wire::kMaxTeamSize; // <= 5 aoe + 5 counters
constexpr uint32_t kLogWorstEntries =
    kLogActors * kLogEntriesPerActor + kLogActors; // + one sudden-death chip each
static_assert(kLogWorstEntries * kLogMaxEntryLen + kLogWrapperLen <= kLogShippedCap,
              "the worst-case round log no longer fits kLogCap -- a legal round would be "
              "REJECTED (see the arithmetic above)");
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
      // v2 (controller resolution R3, closed by combat-depth Task 3): shield/
      // reserved always seed to 0; uses_left seeds from kDuelMoves[m].max_uses
      // (255 = unlimited) for a real slot, 0 (canonical filler) otherwise.
      // Every move defaults to max_uses=255 today, so this stays
      // byte-identical to the old literal-255 seed until a later task gives
      // some move a lower max_uses.
      t.shield = 0;
      for (uint32_t k = 0; k < wire::kStateTreatReservedLen; ++k) {
        t.reserved[k] = 0;
      }
      for (uint32_t j = 0; j < loadoutSize; ++j) {
        t.moves[j] = moves[j];
        t.cooldowns[j] = 0;
        t.uses_left[j] = (j < moveCount) ? duel::kDuelMoves[moves[j]].max_uses : 0;
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

constexpr uint8_t kClashNeutral = 255; // sentinel: target has no move type
constexpr uint8_t kNoGuardian = 255;   // sentinel: nobody is guarding this slot

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

// THE ONE DAMAGE FORMULA. Every factor is a /256 fixed-point fraction, and all three are
// FUSED into a SINGLE >>24 truncation -- never chained >>8 shifts, whose double
// truncation drifts by a point on some inputs, and native/wasm must agree bit-for-bit.
//   power  <= 60         (gen-stats.mjs bounds every authored power to [10,60])
//   clash  <= adv 384    (the pentagon: 384 adv / 170 dis / 256 neutral)
//   splash == kSplashFull for a single-target blow, kTun.aoe_pct_256 for an AoE
//            (combat-depth Task 5 -- an AoE trades per-target damage for reach)
//   mitig  == kSplashFull (nothing), 256 - block_pct_256 (the victim BLOCKED), or
//            guard_pct_256 (a guardian intercepted it)
// kSplashFull == 256 is an exact identity here: (x * 256) >> 24 == x >> 16, so a
// single-target blow's damage is byte-for-byte what the pre-AoE two-factor form produced.
constexpr uint32_t kSplashFull = 256;
static_assert(60u * static_cast<uint64_t>(kTun.adv_mult_256) *
                      (kTun.aoe_pct_256 > kSplashFull ? kTun.aoe_pct_256 : kSplashFull) *
                      (kTun.guard_pct_256 > kSplashFull ? kTun.guard_pct_256 : kSplashFull) <=
                  0xFFFFFFFFull,
              "BlowDamage's fused multiply overflows u32 at the current tunables");
uint32_t BlowDamage(uint32_t power, uint32_t clash, uint32_t splash, uint32_t mitig) {
  return (power * clash * splash * mitig) >> 24;
}

// Append one round-log action entry (schema in the plan's "Log JSON"). `move`
// and `target` carry wire::kMoveSlotEmpty/kTargetNone (255) when absent
// (recover/skip/chip/shield). On a "chip" entry, side/slot are the VICTIM
// (sudden death has no actor). Writes past capacity are dropped, but they latch
// b->overflow -- duel_apply turns that into a hard reject (see kLogCap).
//
// Per kind (combat-depth Task 4 added the last three):
//   hit      side/slot = attacker, target = the enemy slot it AIMED at, `via` =
//            the guardian that actually took it (255 = nobody), `dmg` = the
//            force that reached the final recipient, `absorbed` = how much of
//            that its shield ate, `targetHp`/`ko` = the FINAL RECIPIENT's.
//   guard    side/slot = the guardian, target = the ALLY slot it is covering.
//   shield   side/slot = the caster, `dmg` = the shield points actually GAINED
//            (the u8 pool clamps at 255, so this can be less than the move's
//            power). No damage, no target.
//   counter  side/slot = the treat that riposted, target = the attacker's slot
//            on the other side, `dmg` = the reflected damage, `absorbed` = what
//            the attacker's own shield ate of it, `targetHp`/`ko` = the
//            ATTACKER's. Shaped exactly like a `hit` on purpose.
//
// There is no `retargeted` field: fizzle-on-death (combat-depth Task 2)
// deleted the auto-retarget rule that was the only thing that could ever set
// it, and a permanently-false field is not something to freeze into a format
// that game channels will hash and sign.
void WriteLogEntry(duel::JsonBuf* b, bool first, uint32_t side, uint32_t slot,
                   const char* kind, uint32_t move, uint32_t target,
                   const char* clash, uint32_t dmg, bool blocked,
                   uint32_t targetHp, bool ko, uint32_t absorbed = 0,
                   uint32_t via = wire::kTargetNone) {
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
  duel::jb_raw(b, ",\"via\":");
  duel::jb_u32(b, via);
  duel::jb_raw(b, ",\"clash\":");
  duel::jb_str(b, clash);
  duel::jb_raw(b, ",\"dmg\":");
  duel::jb_u32(b, dmg);
  duel::jb_raw(b, ",\"blocked\":");
  duel::jb_raw(b, blocked ? "true" : "false");
  duel::jb_raw(b, ",\"absorbed\":");
  duel::jb_u32(b, absorbed);
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
        // A real move: a ready loadout index.
        if (a >= tr.move_count || tr.cooldowns[a] != 0) {
          return -1;
        }
        const uint8_t moveEff = duel::kDuelMoves[tr.moves[a]].effect;
        if (moveEff == duel::kEffGuard) {
          // GUARD (combat-depth Task 4) is the one move whose `target` names an
          // ALLY ON ITS OWN SIDE, not an enemy: in range, ALIVE (there is
          // nothing to cover otherwise), and NOT ITSELF -- self-guard is
          // meaningless, and silently allowing it would give one logical order
          // ("I guard nobody") two byte-encodings.
          if (t >= teamSize || t == slot || s.treats[side][t].hp == 0) {
            return -1;
          }
        } else if (moveEff == duel::kEffAoe) {
          // AOE (combat-depth Task 5) names NOBODY -- it hits every living enemy
          // -- so its target byte MUST be exactly kTargetAll. Accepting an AoE
          // "aimed" at a slot and then ignoring the byte would give ONE logical
          // order MANY byte-encodings, and game channels SIGN order bytes: one
          // encoding per logical order is the contract. (It needs no living
          // enemy to be legal: with none left the spray simply whiffs.)
          if (t != wire::kTargetAll) {
            return -1;
          }
        } else if (t >= teamSize) {
          // Every other move aims at an enemy slot -- and, the other direction of
          // that same canonical rule, must NOT use kTargetAll: 0xFE is >= any
          // legal team_size (max 5), so this one check rejects it. (A `shield`
          // deals no damage and ignores this byte entirely -- it is still
          // range-checked, and its log entry always reads target 255, so the
          // ignored value can never reach the state or the log.)
          return -1;
        }
      }
      action[side][slot] = a;
      target[side][slot] = t;
    }
  }

  // ---- semantics 2: STANCES (living treat whose action is a real stance move;
  // fixed for the whole round -- a dead treat is never a valid hit target, so
  // "ends if KO'd mid-round" falls out for free) ----
  //
  // THE STANCE SPLIT (combat-depth Task 4). MoveType 4 (Blocking) is the stance
  // corner, but it is no longer one verb with four power levels: the stance a
  // move grants is its EFFECT, and each buys exactly ONE thing.
  //   block   -> the flat damage reduction (kTun.block_pct_256 -- the BIGGEST of
  //              the four, which is what makes a plain block worth the turn)
  //   guard   -> intercept a single-target blow aimed at an ALLY (below)
  //   shield  -> a persisting absorb pool, raised at the caster's initiative in
  //              the resolve loop (it is an action, not a round-wide stance)
  //   counter -> reflect part of a landed blow back at whoever threw it
  // guard/shield/counter deliberately do NOT also reduce incoming damage. That
  // is what makes the four real CHOICES rather than strict upgrades of each
  // other -- and it is the direct answer to "what's the point doing block".
  bool blockStance[2][wire::kMaxTeamSize] = {};
  // guardedBy[side][slot] = the slot of the ally covering it, or kNoGuardian.
  // Built HERE, before any action resolves, so a same-round intercept is fully
  // computable no matter how the initiative falls. LOWEST GUARDIAN SLOT WINS a
  // tie (the ascending scan below keeps the first writer) -- a total,
  // deterministic rule; the engine has no coin to toss.
  uint8_t guardedBy[2][wire::kMaxTeamSize];
  for (uint32_t side = 0; side < 2; ++side) {
    for (uint32_t slot = 0; slot < teamSize; ++slot) {
      guardedBy[side][slot] = kNoGuardian;
    }
  }
  for (uint32_t side = 0; side < 2; ++side) {
    for (uint32_t slot = 0; slot < teamSize; ++slot) {
      const TreatState& tr = s.treats[side][slot];
      if (tr.hp == 0 || action[side][slot] == wire::kActionRecover) {
        continue;
      }
      const uint8_t eff = duel::kDuelMoves[tr.moves[action[side][slot]]].effect;
      if (eff == duel::kEffBlock) {
        blockStance[side][slot] = true;
      } else if (eff == duel::kEffGuard) {
        const uint8_t ward = target[side][slot]; // validated: ally, alive, != slot
        if (guardedBy[side][ward] == kNoGuardian) {
          guardedBy[side][ward] = static_cast<uint8_t>(slot);
        }
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
    const uint8_t eff = duel::kDuelMoves[mvIdx].effect;
    const uint32_t enemy = 1u - aS;
    const uint32_t tgtSlot = target[aS][aL];

    // GUARD (combat-depth Task 4): the map is already built (stance stage
    // above), so the action itself is pure theatre -- it deals NO damage, and
    // its `target` names the ALLY it is covering, not an enemy. Logging it here
    // (rather than in the stance stage) keeps every entry in initiative order.
    if (eff == duel::kEffGuard) {
      WriteLogEntry(&log, nEntries++ == 0, aS, aL, "guard", mvIdx, tgtSlot,
                    "neu", 0, false, 0, false);
      continue;
    }

    // SHIELD (combat-depth Task 4): a self-absorb pool worth the move's power,
    // raised AT THE CASTER'S INITIATIVE -- it is an action, not a round-wide
    // stance, so a blow that resolves BEFORE the caster acts finds no shield up
    // (that speed tension is the point; the pool's payoff is that it PERSISTS
    // across rounds until something eats it). It deals no damage and ignores
    // the order's target byte.
    if (eff == duel::kEffShield) {
      const uint32_t room = 255u - actor.shield; // u8 pool: raising it CLAMPS
      const uint32_t power = duel::kDuelMoves[mvIdx].power;
      const uint32_t gain = (power < room) ? power : room;
      actor.shield = static_cast<uint8_t>(actor.shield + gain);
      WriteLogEntry(&log, nEntries++ == 0, aS, aL, "shield", mvIdx,
                    wire::kTargetNone, "neu", gain, false, actor.hp, false);
      continue;
    }

    const uint8_t atkType = duel::kDuelMoves[mvIdx].type;
    const uint32_t power = duel::kDuelMoves[mvIdx].power;

    // The clash multiplier against the enemy in `defSlot`, read off THAT treat's OWN
    // picked move (Recover has no move type at all -> neutral). Single-target reads it
    // once; an AoE reads it PER VICTIM -- which is the whole point of "clash per target".
    auto ClashVs = [&](uint32_t defSlot) -> uint16_t {
      const uint8_t defAct = action[enemy][defSlot];
      uint8_t defClash = kClashNeutral;
      if (defAct != wire::kActionRecover) {
        defClash = duel::kDuelMoves[s.treats[enemy][defSlot].moves[defAct]].type;
      }
      return ClashMult(atkType, defClash);
    };

    // ==== THE FIXED INTERACTION ORDER (combat-depth Task 4) ====
    //   1. GUARD REDIRECT (single-target damage only -- an AoE ignores it)
    //   2. SHIELD ABSORB  (on whoever FINALLY receives the blow)
    //   3. HP
    // and then the COUNTER fires off `landed` -- the force that reached the
    // final recipient, BEFORE its shield ate any of it. Nothing here reorders.
    //
    // Steps 2, 3 and the counter are THIS lambda -- the single place damage ever reaches
    // a treat. The single-target path and the AoE path (Task 5) both go through it
    // verbatim, which is exactly why "a counter fires iff the FINAL RECIPIENT's own
    // picked move is a counter" holds for an AoE with NO special case: AoE beats Guard;
    // Counter beats AoE.
    //
    // A recipient FELLED by the blow does not riposte: KO is permanent, and a
    // dead treat takes no further part in the round (the same rule that stops a
    // dead guardian intercepting). Burst is the legible answer to a counter.
    //
    // The reflect is counter_pct of `landed` -- the blow's force, NOT the hp it
    // actually drained -- so your own shield holding does not silently disarm
    // your counter ("you swung at me, so you get punished"). It ignores clash
    // and guard, goes through the attacker's own shield (an absorb pool absorbs
    // anything), can KO, and can NEVER itself be countered: it is not a move, so
    // nothing re-enters this lambda.
    auto LandBlow = [&](const char* kind, uint32_t tgt, uint32_t rcvSlot, uint32_t via,
                        uint16_t mult, uint32_t landed, bool blocked) {
      TreatState& rcv = s.treats[enemy][rcvSlot];
      // 2. SHIELD ABSORB, on whoever finally receives it. The EXCESS CARRIES
      // THROUGH to hp -- a shield blunts a blow, it does not stop it.
      const uint32_t absorbed = (rcv.shield < landed) ? rcv.shield : landed;
      rcv.shield = static_cast<uint8_t>(rcv.shield - absorbed);
      const uint32_t toHp = landed - absorbed;
      // 3. HP.
      const uint16_t newHp =
          (rcv.hp > toHp) ? static_cast<uint16_t>(rcv.hp - toHp) : 0;
      rcv.hp = newHp;
      WriteLogEntry(&log, nEntries++ == 0, aS, aL, kind, mvIdx, tgt,
                    ClashLabel(mult), landed, blocked, newHp, newHp == 0, absorbed,
                    via);
      if (newHp == 0) {
        return; // the dead do not riposte
      }
      const uint8_t rcvAct = action[enemy][rcvSlot];
      if (rcvAct == wire::kActionRecover ||
          duel::kDuelMoves[rcv.moves[rcvAct]].effect != duel::kEffCounter) {
        return;
      }
      const uint32_t reflect = (landed * kTun.counter_pct_256) >> 8;
      const uint32_t rAbsorbed = (actor.shield < reflect) ? actor.shield : reflect;
      actor.shield = static_cast<uint8_t>(actor.shield - rAbsorbed);
      const uint32_t rToHp = reflect - rAbsorbed;
      const uint16_t atkHp =
          (actor.hp > rToHp) ? static_cast<uint16_t>(actor.hp - rToHp) : 0;
      actor.hp = atkHp;
      WriteLogEntry(&log, nEntries++ == 0, enemy, rcvSlot, "counter",
                    rcv.moves[rcvAct], aL, "neu", reflect, false, atkHp,
                    atkHp == 0, rAbsorbed);
    };

    // AOE (combat-depth Task 5). It names NOBODY (its target byte is kTargetAll, checked
    // above) and hits EVERY LIVING ENEMY, in SLOT ORDER, at kTun.aoe_pct_256 of power.
    // This is the move that breaks the old proof that spreading damage is never better
    // than concentrating it.
    //   - CLASH IS PER VICTIM, against that victim's own picked move (ClashVs above).
    //   - BLOCK applies per victim; SHIELD absorbs per victim (LandBlow).
    //   - GUARD IS IGNORED: the spray hits the ward directly and the guardian eats only
    //     its OWN splash. That is what makes AoE the answer to a Guard wall.
    //   - COUNTER STILL FIRES, per victim (LandBlow, no special case). AoE beats Guard;
    //     Counter beats AoE -- spray into three counter-stances and you pay three times.
    //   - If a counter FELLS THE CASTER mid-spray, the spray STOPS with it: KO is
    //     permanent and a dead treat takes no further part in the round (the same rule
    //     that stops a dead guardian intercepting). A counter on a low slot can shield
    //     the rest of its team from the rest of the blast.
    //   - If EVERY enemy is already dead (an earlier actor this round felled the last
    //     one), the spray whiffs as exactly ONE "skip" -- and still burns its cooldown.
    if (eff == duel::kEffAoe) {
      bool anyVictim = false;
      for (uint32_t vSlot = 0; vSlot < teamSize; ++vSlot) {
        if (actor.hp == 0) {
          break; // felled by a victim's counter: the rest of the spray never happens
        }
        if (s.treats[enemy][vSlot].hp == 0) {
          continue; // KO is permanent: a corpse is not splashed and logs nothing
        }
        anyVictim = true;
        const uint16_t mult = ClashVs(vSlot);
        uint32_t mitig = kSplashFull;
        bool blocked = false;
        if (blockStance[enemy][vSlot]) {
          blocked = true;
          mitig = 256u - kTun.block_pct_256;
        }
        const uint32_t landed = BlowDamage(power, mult, kTun.aoe_pct_256, mitig);
        // kind "aoe", never "hit": the client has to be able to group ONE swing's
        // victims, and "hit" must keep meaning single-target. `target` is the victim
        // itself and `via` is always 255 -- no guardian ever stands in front of a spray.
        LandBlow("aoe", vSlot, vSlot, wire::kTargetNone, mult, landed, blocked);
      }
      if (!anyVictim) {
        WriteLogEntry(&log, nEntries++ == 0, aS, aL, "skip", mvIdx,
                      wire::kTargetNone, "neu", 0, false, 0, false);
      }
      continue;
    }

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

    const uint16_t mult = ClashVs(tgtSlot);

    // 1. GUARD REDIRECT. The blow lands on the guardian instead, at guard_pct.
    // It is SINGLE-HOP and NEVER CHAINED: if A guards B and B guards C, a blow
    // at C goes to B and STOPS there -- it is not then passed on to A (chaining
    // is an infinite-loop hazard and a consensus footgun). A guardian KO'd
    // EARLIER IN THIS ROUND cannot intercept -- hence the hp check HERE, at
    // intercept time, not back when the map was built -- and the guard simply
    // lapses, landing the blow on the ward.
    //
    // Exactly ONE mitigation percentage applies to a blow: a guarded ward's own
    // block stance is irrelevant, because the ward is not the one being hit.
    uint32_t rcvSlot = tgtSlot;
    uint32_t via = wire::kTargetNone;
    uint32_t mitig = kSplashFull; // no mitigation
    bool blocked = false;
    const uint8_t guardian = guardedBy[enemy][tgtSlot];
    if (guardian != kNoGuardian && s.treats[enemy][guardian].hp > 0) {
      rcvSlot = guardian;
      via = guardian;
      mitig = kTun.guard_pct_256;
    } else if (blockStance[enemy][tgtSlot]) {
      blocked = true;
      mitig = 256u - kTun.block_pct_256;
    }
    LandBlow("hit", tgtSlot, rcvSlot, via, mult,
             BlowDamage(power, mult, kSplashFull, mitig), blocked);
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
  // It is NOT an attack -- it is the arena. It ignores the clash pentagon, the
  // block stance, the shield pool and the guard redirect alike (combat-depth
  // Task 4 kept it that way, deliberately: there is nothing here to absorb and
  // nobody to step in front of). It goes straight to hp.
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

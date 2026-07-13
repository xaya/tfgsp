// duel/src/engine.cpp — duel engine implementation.
//
// Task 1 (this scaffold): every ABI entry point below is a STUB — it
// validates nothing and rejects every call (-1, zero-length output). Real
// config/state decoding, resolve semantics, and JSON output land in Tasks
// 3-4; this file only pins the ABI surface + the allocator/output-buffer
// plumbing everything else will sit on top of.
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

using namespace duel;

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
        t.moves[j] = moves[j];
        t.cooldowns[j] = buf[cdOff + j];
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
  const uint32_t written = encode_state(state, g_out, kOutCap);
  if (written == 0) {
    return -1;
  }
  g_outLen = written;
  return 0;
}

DUEL_ABI("duel_apply")
int32_t duel_apply(const uint8_t* st, uint32_t stLen, const uint8_t* ord,
                    uint32_t ordLen) {
  (void)st;
  (void)stLen;
  (void)ord;
  (void)ordLen;
  g_outLen = 0;
  g_logLen = 0;
  return -1;
}

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

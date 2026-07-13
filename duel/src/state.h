// duel/src/state.h — internal state representation + wire codec.
//
// `DuelState`/`TreatState` are the engine's in-memory view of a state buffer
// (wire.h STATE_* layout); `encode_state`/`decode_state` are the bounds-
// checked byte<->struct codec shared by duel_init (Task 3, encode only) and
// duel_apply/duel_view (Task 4, both directions). Both functions are pure
// and allocation-free — callers own all buffers (see engine.cpp's static
// g_out for the one place encode_state's output ends up in this codebase
// today).
//
// decode_state enforces the FULL wire contract for a state buffer (mirrors
// duel_init's config validation): version, team_size/loadout_size bounds,
// exact length for that shape, canonical-zero reserved/pad bytes (non-zero
// = reject -- these buffers get hashed/signed in game channels, so exactly
// one byte encoding per logical state is the contract), and per-treat
// quality/sweetness/move_count/move-index/filler/uniqueness rules identical
// to the config's, plus filler-slot cooldowns (>= move_count) MUST be 0 --
// no move lives there, so a non-zero byte would be a second encoding of the
// same logical state. It does NOT
// enforce game-semantic invariants (e.g. hp <= max_hp, cooldown <= a move's
// authored cooldown) — those are Task 4's resolve-time concern, not a wire-
// format constraint; a structurally valid state with an out-of-range hp
// value only matters once real resolve logic reads it.

#pragma once

#include <cstdint>

#include "wire.h"

namespace duel {

struct TreatState {
  uint16_t hp;
  uint16_t max_hp;
  uint8_t quality;
  uint8_t sweetness;
  uint8_t move_count;
  uint8_t moves[wire::kMaxLoadoutSize];      // dense indices; [move_count, loadout_size) == 0xFF
  uint8_t cooldowns[wire::kMaxLoadoutSize];  // rounds until usable, 0 = ready
};

struct DuelState {
  uint8_t version;
  uint8_t team_size;
  uint8_t loadout_size;
  uint8_t phase;
  uint16_t round;
  TreatState treats[2][wire::kMaxTeamSize];  // [side][slot], slot < team_size valid
};

// Writes exactly wire::StateLen(s.team_size, s.loadout_size) bytes to `out`
// and returns that count. Returns 0 (no partial write) if `out` is null or
// `cap` is smaller than the required length. Does not itself validate that
// s.team_size/loadout_size are in-bounds -- callers (duel_init) only ever
// build a DuelState from already-validated config fields.
uint32_t encode_state(const DuelState& s, uint8_t* out, uint32_t cap);

// Validates `buf[0..len)` against the full state wire contract and fills
// `*out` on success. Returns false on any violation (length, version,
// bounds, move rules) -- `*out` is left partially written and MUST NOT be
// used by the caller in that case. Every read is bounds-checked against
// `len` before use (reject-not-crash: safe on truncated/fuzzed buffers).
bool decode_state(const uint8_t* buf, uint32_t len, DuelState* out);

}  // namespace duel

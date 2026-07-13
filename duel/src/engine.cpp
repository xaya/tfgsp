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

} // namespace

DUEL_ABI("duel_init")
int32_t duel_init(const uint8_t* cfg, uint32_t len) {
  (void)cfg;
  (void)len;
  g_outLen = 0;
  return -1;
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

// duel/src/engine.cpp — duel engine implementation.
//
// Task 1 (this scaffold): every ABI entry point below is a STUB — it
// validates nothing and rejects every call (-1, zero-length output). Real
// config/state decoding, resolve semantics, and JSON output land in Tasks
// 3-4; this file only pins the ABI surface + the allocator/output-buffer
// plumbing everything else will sit on top of.
//
// Allocator scheme (duel_alloc/duel_free): a single static byte arena with
// a bump pointer. `duel_alloc` hands out the next `n` bytes and advances the
// pointer; `duel_free` is a permanent no-op. This is deliberate, not a
// placeholder to revisit: inputs here are config/state/orders buffers on
// the order of a few hundred bytes (wire.h's frozen shapes), a duel's
// lifetime is one WASM instance's worth of calls, and there is no delete
// path (or need for one) in this engine's usage pattern. A real
// malloc/free would pull libc allocator machinery into the wasm build for
// no benefit here.

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
// Sized well above any single wire-format buffer (CFG/STATE/ORDERS_LEN are
// all under 128 bytes even at the max team_size=5/loadout_size=4 shape) to
// leave headroom for whatever a WASM host wants to stage across a call.
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
  if (n == 0 || n > kArenaCap - g_arenaUsed) {
    return nullptr;
  }
  uint8_t* p = g_arena + g_arenaUsed;
  g_arenaUsed += n;
  return p;
}

DUEL_ABI("duel_free")
void duel_free(uint8_t* p) {
  (void)p; // bump allocator: free is a permanent no-op, see file header.
}

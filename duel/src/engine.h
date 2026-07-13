// duel/src/engine.h — frozen C ABI for the duel engine.
//
// Pure functions over byte buffers (see wire.h for the exact formats):
// config in -> state out (duel_init), state+orders in -> new state + round
// log out (duel_apply), state in -> view JSON out (duel_view). Output
// buffers are read back through duel_out_*/duel_log_* rather than returned
// directly so the same signatures work unchanged for both the native test
// binary and the WASM export table (WASM functions can't return pointers
// into caller-owned memory the way a native shared lib could).
//
// Every function here is defined with `extern "C"` linkage in engine.cpp;
// the WASM build additionally exports each under its own name (see the
// DUEL_ABI macro in engine.cpp) so the reactor's export table is a flat list
// of `duel_*` symbols with zero imports.

#pragma once

#include <cstdint>

extern "C" {

// Validate + decode a config buffer (wire.h CFG_*) and write the initial
// state to the output buffer. Returns 0 on success (duel_out_ptr/
// duel_out_len hold the state), -1 on any validation failure, in which
// case duel_out_len is cleared to 0 (no stale output survives a reject).
int32_t duel_init(const uint8_t* cfg, uint32_t len);

// Validate + resolve one round of orders (wire.h ORDERS_*) against a state
// buffer (wire.h STATE_*). Returns 0 on success — duel_out_ptr/duel_out_len
// hold the new state, duel_log_ptr/duel_log_len hold that round's resolve
// log as JSON — or -1 on reject, in which case duel_out_len and
// duel_log_len are both cleared to 0 (no stale output survives a reject).
int32_t duel_apply(const uint8_t* st, uint32_t stLen,
                    const uint8_t* ord, uint32_t ordLen);

// Render a state buffer to view JSON (duel_out_ptr/duel_out_len). Returns 0
// on success, -1 if the state buffer fails validation (duel_out_len
// cleared to 0, as above).
int32_t duel_view(const uint8_t* st, uint32_t stLen);

const uint8_t* duel_out_ptr(void);
uint32_t duel_out_len(void);
const uint8_t* duel_log_ptr(void);
uint32_t duel_log_len(void);

// Scratch allocator for a WASM host to copy input bytes into before calling
// the functions above (native callers may just pass their own buffers
// directly — duel_alloc/duel_free exist for the WASM side only).
//
// ABI CONTRACT (allocation lifetime): a duel_alloc'd buffer is valid only
// for the input side of the NEXT engine call(s) the caller makes with it —
// allocate the input buffer(s) for one call, fill them, call, done. Do not
// hold an allocation across later calls or reuse it as input again: the
// backing store is a bump arena that wraps to the start when full (see
// engine.cpp), so old allocations may eventually be recycled. Under that
// pattern a wrap can only ever touch buffers from calls that already
// completed. duel_free is a no-op kept for ABI symmetry.
uint8_t* duel_alloc(uint32_t n);
void duel_free(uint8_t* p);

} // extern "C"

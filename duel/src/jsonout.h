// duel/src/jsonout.h — tiny fixed-buffer JSON appender.
//
// The engine writes its view/log JSON (duel_view, duel_apply's round log)
// through this instead of snprintf/sprintf: no libc I/O, no allocation, and
// crucially no libc formatting surface for the wasi-sdk build to accidentally
// pull WASI host imports through (the whole point of the zero-import
// reactor build — see duel/build.sh). Every helper is `inline` (safe to
// include from multiple translation units) and bounds-checked against
// `JsonBuf::cap`: writes past capacity are silently dropped rather than
// overflowing. Buffers are sized generously by callers; a truncated JSON
// output is a "buffer too small" bug to catch in tests, not something this
// header tries to report at runtime.

#pragma once

#include <cstdint>

namespace duel {

struct JsonBuf {
  uint8_t* p;
  uint32_t cap;
  uint32_t len;
};

// Append `n` raw bytes verbatim.
inline void jb_raw(JsonBuf* b, const char* s, uint32_t n) {
  for (uint32_t i = 0; i < n && b->len < b->cap; ++i) {
    b->p[b->len++] = static_cast<uint8_t>(s[i]);
  }
}

// Append a NUL-terminated literal (e.g. punctuation/field-name fragments
// like "{\"hp\":").
inline void jb_raw(JsonBuf* b, const char* s) {
  while (*s != '\0' && b->len < b->cap) {
    b->p[b->len++] = static_cast<uint8_t>(*s++);
  }
}

// Append a quoted JSON string. Callers only ever pass internal enum tokens
// ("hit", "adv", ...), never arbitrary/user text, so no escaping is done.
inline void jb_str(JsonBuf* b, const char* s) {
  jb_raw(b, "\"");
  jb_raw(b, s);
  jb_raw(b, "\"");
}

// Append an unsigned decimal integer (no libc itoa/snprintf).
inline void jb_u32(JsonBuf* b, uint32_t v) {
  char tmp[10]; // UINT32_MAX = "4294967295" = 10 digits
  int n = 0;
  do {
    tmp[n++] = static_cast<char>('0' + (v % 10));
    v /= 10;
  } while (v != 0);
  while (n > 0 && b->len < b->cap) {
    b->p[b->len++] = static_cast<uint8_t>(tmp[--n]);
  }
}

// Append a signed decimal integer.
inline void jb_i32(JsonBuf* b, int32_t v) {
  if (v < 0) {
    jb_raw(b, "-");
    // Widen before negating: -INT32_MIN overflows int32_t.
    jb_u32(b, static_cast<uint32_t>(-static_cast<int64_t>(v)));
  } else {
    jb_u32(b, static_cast<uint32_t>(v));
  }
}

} // namespace duel

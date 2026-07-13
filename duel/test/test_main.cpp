// duel/test/test_main.cpp — minimal assert-based native test runner.
//
// Native-only (libc/printf fine here — this binary never ships to wasm).
// `duel/build.sh` compiles this alongside engine.cpp under -fno-exceptions
// -fno-rtti, so this is deliberately NOT exception-based: `CHECK(cond)`
// records a failure and prints where, but keeps executing (a failed check
// does not abort the process the way <cassert>'s assert() would); `RUN(fn)`
// then reports whether any CHECK inside `fn` failed. Exit code is nonzero
// iff anything failed.

#include "../src/engine.h"
#include "../src/jsonout.h"
#include "../src/wire.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {

int g_tests_run = 0;
int g_tests_failed = 0;
int g_checks_failed_in_current_test = 0;

} // namespace

#define CHECK(cond)                                                          \
  do {                                                                       \
    if (!(cond)) {                                                           \
      ++g_checks_failed_in_current_test;                                    \
      std::printf("    CHECK failed: %s (%s:%d)\n", #cond, __FILE__,         \
                   __LINE__);                                                \
    }                                                                        \
  } while (0)

#define RUN(fn)                                                              \
  do {                                                                       \
    ++g_tests_run;                                                           \
    g_checks_failed_in_current_test = 0;                                    \
    fn();                                                                    \
    if (g_checks_failed_in_current_test == 0) {                             \
      std::printf("[ OK   ] %s\n", #fn);                                     \
    } else {                                                                 \
      ++g_tests_failed;                                                      \
      std::printf("[ FAIL ] %s\n", #fn);                                     \
    }                                                                        \
  } while (0)

namespace {

// duel_init must reject a null/zero-length config.
void test_duel_init_rejects_null_config() {
  CHECK(duel_init(nullptr, 0) == -1);
}

// duel_init must reject even a valid-LENGTH all-zero config (a zeroed
// config decodes to team_size=0/loadout_size=0, out of the [1,5]/[1,4]
// bounds — and today it's still just a stub, so -1 either way).
void test_duel_init_rejects_zeroed_config() {
  constexpr uint32_t kLen =
      duel::wire::CfgLen(duel::wire::kRefTeamSize, duel::wire::kRefLoadoutSize);
  uint8_t cfg[kLen] = {0};
  CHECK(duel_init(cfg, kLen) == -1);
  CHECK(duel_out_len() == 0);
}

// jsonout: all four helpers compose into exact expected bytes, filling a
// 24-byte buffer to exactly its capacity.
void test_jsonout_builds_exact_json() {
  uint8_t buf[24];
  duel::JsonBuf b{buf, sizeof(buf), 0};
  duel::jb_raw(&b, "{\"a\":");
  duel::jb_u32(&b, 42);
  duel::jb_raw(&b, ",\"b\":", 5); // explicit-length overload
  duel::jb_str(&b, "ok");
  duel::jb_raw(&b, ",\"c\":");
  duel::jb_i32(&b, -7);
  duel::jb_raw(&b, "}");
  const char expected[] = "{\"a\":42,\"b\":\"ok\",\"c\":-7}"; // 24 bytes
  CHECK(b.len == sizeof(buf));
  CHECK(b.len == sizeof(expected) - 1);
  CHECK(std::memcmp(buf, expected, b.len) == 0);
}

// jsonout: appending past capacity truncates instead of overflowing —
// jb_u32(12345) into a cap-4 buffer writes only 4 bytes, len stays == cap,
// and guard bytes beyond cap are untouched.
void test_jsonout_never_writes_past_cap() {
  uint8_t buf[8];
  std::memset(buf, 0xAB, sizeof(buf)); // guard pattern
  duel::JsonBuf b{buf, 4, 0};
  duel::jb_u32(&b, 12345);
  CHECK(b.len == 4);
  CHECK(std::memcmp(buf, "1234", 4) == 0); // most-significant digits first
  for (uint32_t i = 4; i < sizeof(buf); ++i) {
    CHECK(buf[i] == 0xAB); // no write past cap
  }
  duel::jb_raw(&b, "x"); // further appends on a full buffer are no-ops
  CHECK(b.len == 4);
}

} // namespace

int main() {
  RUN(test_duel_init_rejects_null_config);
  RUN(test_duel_init_rejects_zeroed_config);
  RUN(test_jsonout_builds_exact_json);
  RUN(test_jsonout_never_writes_past_cap);

  std::printf("---\n%d ran, %d failed\n", g_tests_run, g_tests_failed);
  return g_tests_failed == 0 ? 0 : 1;
}

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
#include "../src/stats_gen.h"
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

// ---- Task 2: stats_gen.h (duel/data/duel-stats.json codegen) ----
//
// kDuelMoves has no GUID field (DuelMoveStat is deliberately just
// {power,speed,cooldown,type} per the plan's frozen interface), so the
// "dense order matches the blueprint" check below is anchored on the two
// GUIDs at either end of the ASCII sort of all 39
// proto/roconfig/fightermoveblueprint.pb.text AuthoredIds (computed
// independently of gen-stats.mjs and hardcoded here as literals):
//   index 0  = "0090580c-04ef-9d84-e883-32f52c977b98" Gum Drop Kick
//              (Move_Common_Blocking_GumDropKick, Quality 1, MoveType 4 Blocking)
//   index 38 = "fa6144f9-ad49-8124-386f-351a7f1ab546" Float Like a Butter Cream
//              (Move_Uncommon_Tricky_FloatLikeAButterCream, Quality 2, MoveType 2 Tricky)
// type/cooldown are pure functions of (quality, moveType) per the Step 2
// formula (cooldown = clamp(quality-1,0,3), Blocking gets an extra -1 floored
// at 0) and are NOT touched by the hand-jitter, so they're exact-comparable;
// power/speed are jittered and only bounds-checked elsewhere.
void test_stats_gen_dense_order_matches_blueprint_ends() {
  CHECK(duel::kDuelMoves[0].type == 4);     // Blocking
  CHECK(duel::kDuelMoves[0].cooldown == 0); // quality 1 -> 0, Blocking -1 floored at 0
  CHECK(duel::kDuelMoves[38].type == 2);     // Tricky
  CHECK(duel::kDuelMoves[38].cooldown == 1); // quality 2 -> 1, not Blocking
}

// Every entry in range: power in [10,60], speed in [1,120], cooldown <= 3
// (wire.h's move universe is exactly 39 -- kMoveUniverse). At least 5
// Blocking-type (type==4) entries carry the "block" flag's meaning, since
// DuelMoveStat has no separate flags field: type==4 IS the block marker
// (see stats_gen.h / the plan's Task 2 test bullet).
void test_stats_gen_bounds_and_blocking_count() {
  CHECK(sizeof(duel::kDuelMoves) / sizeof(duel::kDuelMoves[0]) ==
        duel::wire::kMoveUniverse);
  int blockingCount = 0;
  for (uint32_t i = 0; i < duel::wire::kMoveUniverse; ++i) {
    const duel::DuelMoveStat& mv = duel::kDuelMoves[i];
    CHECK(mv.power >= 10 && mv.power <= 60);
    CHECK(mv.speed >= 1 && mv.speed <= 120);
    CHECK(mv.cooldown <= 3);
    CHECK(mv.type <= 4);
    if (mv.type == 4) {
      ++blockingCount;
    }
  }
  CHECK(blockingCount >= 5);
}

// kTun mirrors duel/data/duel-stats.json's top-level "tunables" verbatim
// (plan Task 2 Step 2 exact values).
void test_stats_gen_tunables_exact() {
  CHECK(duel::kTun.hp_base == 100);
  CHECK(duel::kTun.hp_per_quality == 30);
  CHECK(duel::kTun.hp_per_sweetness == 10);
  CHECK(duel::kTun.adv_mult_256 == 384);
  CHECK(duel::kTun.dis_mult_256 == 170);
  CHECK(duel::kTun.block_pct_256 == 102);
  CHECK(duel::kTun.round_cap == 30);
  CHECK(duel::kTun.team_size == 3);
  CHECK(duel::kTun.loadout_size == 4);
}

} // namespace

int main() {
  RUN(test_duel_init_rejects_null_config);
  RUN(test_duel_init_rejects_zeroed_config);
  RUN(test_jsonout_builds_exact_json);
  RUN(test_jsonout_never_writes_past_cap);
  RUN(test_stats_gen_dense_order_matches_blueprint_ends);
  RUN(test_stats_gen_bounds_and_blocking_count);
  RUN(test_stats_gen_tunables_exact);

  std::printf("---\n%d ran, %d failed\n", g_tests_run, g_tests_failed);
  return g_tests_failed == 0 ? 0 : 1;
}

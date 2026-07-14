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
#include "../src/state.h"
#include "../src/stats_gen.h"
#include "../src/wire.h"

#include <climits>
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

// ---- Task 3: duel_init (config validation + initial state) ----
//
// hp/max_hp are derived from kTun (stats_gen.h, Task 2's authored values:
// hp_base=100, hp_per_quality=30, hp_per_sweetness=10) via the plan's
// formula `max_hp = hp_base + (quality-1)*hp_per_quality +
// sweetness*hp_per_sweetness`:
//   q1 sw1  -> 100 + 0*30 + 1*10  = 110
//   q2 sw4  -> 100 + 1*30 + 4*10  = 170
//   q3 sw7  -> 100 + 2*30 + 7*10  = 230
//   q3 sw3  -> 100 + 2*30 + 3*10  = 190
//   q4 sw6  -> 100 + 3*30 + 6*10  = 250
//   q4 sw10 -> 100 + 3*30 + 10*10 = 290

uint16_t ReadU16LE(const uint8_t* p) {
  return static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8));
}

constexpr uint32_t kBaselineTeamSize = 3;
constexpr uint32_t kBaselineLoadoutSize = 4;
constexpr uint32_t kBaselineCfgLen =
    duel::wire::CfgLen(kBaselineTeamSize, kBaselineLoadoutSize); // 46
constexpr uint32_t kBaselineStateLen =
    duel::wire::StateLen(kBaselineTeamSize, kBaselineLoadoutSize); // 152

// The largest a state buffer can ever be (team_size=5, loadout_size=4) --
// scratch buffers below size off THIS, never a magic literal, so a future
// wire grow (more loadout slots, bigger team cap) fails loud at compile time
// instead of silently overflowing a stack array a sanitizer has to catch.
constexpr uint32_t kMaxStateLen =
    duel::wire::StateLen(duel::wire::kMaxTeamSize, duel::wire::kMaxLoadoutSize); // 248

// The plan's Task 3 vector: six treats spanning q1sw1 (3 moves) .. q4sw10
// (4 moves). Byte layout follows wire.h's CFG_* offsets exactly (header 4
// bytes, then 6 treats of quality|sweetness|move_count|moves[4]).
void MakeBaselineCfg(uint8_t cfg[kBaselineCfgLen]) {
  const uint8_t bytes[kBaselineCfgLen] = {
      // version, team_size, loadout_size, reserved
      duel::wire::kVersion, 3, 4, 0,
      // -- side 0 --
      // slot0: q1 sw1 mc3 moves[0,1,2,FF]
      1, 1, 3, 0, 1, 2, 0xFF,
      // slot1: q2 sw4 mc4 moves[3,4,5,6]
      2, 4, 4, 3, 4, 5, 6,
      // slot2: q3 sw7 mc2 moves[7,8,FF,FF]
      3, 7, 2, 7, 8, 0xFF, 0xFF,
      // -- side 1 --
      // slot0: q3 sw3 mc1 moves[9,FF,FF,FF]
      3, 3, 1, 9, 0xFF, 0xFF, 0xFF,
      // slot1: q4 sw6 mc3 moves[10,11,12,FF]
      4, 6, 3, 10, 11, 12, 0xFF,
      // slot2: q4 sw10 mc4 moves[13,14,15,16]
      4, 10, 4, 13, 14, 15, 16,
  };
  for (uint32_t i = 0; i < kBaselineCfgLen; ++i) {
    cfg[i] = bytes[i];
  }
}

// Byte-level check of one treat inside a duel_init output buffer, entirely
// via wire.h offsets (independent of the engine's own encode_state/
// decode_state so this actually exercises duel_init's output bytes, not
// just its own codec agreeing with itself).
//
// Also pins duel_init's v2 seeding contract (controller resolution R3): a
// fresh treat's shield is 0, its 4 reserved bytes are 0, and uses_left[j] is
// 255 (unlimited) for a real move slot / 0 for a filler slot -- NOT seeded
// from kDuelMoves[m].max_uses (that field doesn't exist until Task 3).
void CheckStateTreat(const uint8_t* out, uint32_t teamSize, uint32_t loadoutSize,
                      uint32_t side, uint32_t slot, uint8_t quality,
                      uint8_t sweetness, uint8_t moveCount,
                      const uint8_t moves[], uint16_t expectedHp) {
  const uint32_t sideLen = duel::wire::StateSideLen(teamSize, loadoutSize);
  const uint32_t treatLen = duel::wire::StateTreatLen(loadoutSize);
  const uint32_t base =
      duel::wire::kStateOffTeams + side * sideLen + slot * treatLen;
  CHECK(ReadU16LE(out + base + duel::wire::kStateTreatOffHp) == expectedHp);
  CHECK(ReadU16LE(out + base + duel::wire::kStateTreatOffMaxHp) == expectedHp);
  CHECK(out[base + duel::wire::kStateTreatOffQuality] == quality);
  CHECK(out[base + duel::wire::kStateTreatOffSweetness] == sweetness);
  CHECK(out[base + duel::wire::kStateTreatOffMoveCount] == moveCount);
  CHECK(out[base + duel::wire::kStateTreatOffShield] == 0); // fresh state: no shield yet
  for (uint32_t k = 0; k < duel::wire::kStateTreatReservedLen; ++k) {
    CHECK(out[base + duel::wire::kStateTreatOffReserved + k] == 0);
  }
  const uint32_t movesOff = base + duel::wire::kStateTreatOffMoves;
  const uint32_t cdOff = base + duel::wire::StateTreatOffCooldowns(loadoutSize);
  const uint32_t ulOff = base + duel::wire::StateTreatOffUsesLeft(loadoutSize);
  for (uint32_t j = 0; j < loadoutSize; ++j) {
    CHECK(out[movesOff + j] == moves[j]);
    CHECK(out[cdOff + j] == 0); // fresh state: every cooldown starts ready
    // R3: real slots seed unlimited (255); filler slots are canonically 0.
    CHECK(out[ulOff + j] == (j < moveCount ? 255 : 0));
  }
}

void test_duel_init_valid_3v3_produces_initial_state() {
  uint8_t cfg[kBaselineCfgLen];
  MakeBaselineCfg(cfg);

  CHECK(duel_init(cfg, kBaselineCfgLen) == 0);
  CHECK(duel_out_len() ==
        duel::wire::StateLen(kBaselineTeamSize, kBaselineLoadoutSize));
  CHECK(duel_out_len() == kBaselineStateLen);

  const uint8_t* out = duel_out_ptr();
  CHECK(out[duel::wire::kStateOffVersion] == duel::wire::kVersion);
  CHECK(out[duel::wire::kStateOffTeamSize] == kBaselineTeamSize);
  CHECK(out[duel::wire::kStateOffLoadoutSize] == kBaselineLoadoutSize);
  CHECK(out[duel::wire::kStateOffPhase] == duel::wire::kPhaseActive);
  CHECK(ReadU16LE(out + duel::wire::kStateOffRound) == 0);
  CHECK(ReadU16LE(out + duel::wire::kStateOffReserved) == 0);

  const uint8_t moves0[4] = {0, 1, 2, 0xFF};
  const uint8_t moves1[4] = {3, 4, 5, 6};
  const uint8_t moves2[4] = {7, 8, 0xFF, 0xFF};
  const uint8_t moves3[4] = {9, 0xFF, 0xFF, 0xFF};
  const uint8_t moves4[4] = {10, 11, 12, 0xFF};
  const uint8_t moves5[4] = {13, 14, 15, 16};
  CheckStateTreat(out, kBaselineTeamSize, kBaselineLoadoutSize, 0, 0, 1, 1, 3,
                   moves0, 110);
  CheckStateTreat(out, kBaselineTeamSize, kBaselineLoadoutSize, 0, 1, 2, 4, 4,
                   moves1, 170);
  CheckStateTreat(out, kBaselineTeamSize, kBaselineLoadoutSize, 0, 2, 3, 7, 2,
                   moves2, 230);
  CheckStateTreat(out, kBaselineTeamSize, kBaselineLoadoutSize, 1, 0, 3, 3, 1,
                   moves3, 190);
  CheckStateTreat(out, kBaselineTeamSize, kBaselineLoadoutSize, 1, 1, 4, 6, 3,
                   moves4, 250);
  CheckStateTreat(out, kBaselineTeamSize, kBaselineLoadoutSize, 1, 2, 4, 10, 4,
                   moves5, 290);
}

constexpr uint32_t kShape2v2Loadout2CfgLen = 24;
void Make2v2Loadout2Cfg(uint8_t cfg[kShape2v2Loadout2CfgLen]) {
  const uint8_t bytes[kShape2v2Loadout2CfgLen] = {
      duel::wire::kVersion, 2, 2, 0, // header
      1, 2, 1, 0, 0xFF,       // side0 slot0: q1 sw2 mc1 moves[0,FF]
      2, 5, 2, 1, 2,          // side0 slot1: q2 sw5 mc2 moves[1,2]
      3, 8, 1, 3, 0xFF,       // side1 slot0: q3 sw8 mc1 moves[3,FF]
      4, 1, 2, 4, 5,          // side1 slot1: q4 sw1 mc2 moves[4,5]
  };
  for (uint32_t i = 0; i < kShape2v2Loadout2CfgLen; ++i) cfg[i] = bytes[i];
}

// A non-3v3 shape (2v2, loadout 2) must work too -- the wire formulas, not
// team_size==3, are the actual contract.
void test_duel_init_valid_2v2_loadout2_shape() {
  constexpr uint32_t kTeamSize = 2;
  constexpr uint32_t kLoadoutSize = 2;
  constexpr uint32_t kCfgLen = duel::wire::CfgLen(kTeamSize, kLoadoutSize);
  CHECK(kCfgLen == 24); // 4 + 2*2*(3+2)
  static_assert(kCfgLen == kShape2v2Loadout2CfgLen, "");

  uint8_t cfg[kCfgLen];
  Make2v2Loadout2Cfg(cfg);

  CHECK(duel_init(cfg, kCfgLen) == 0);
  constexpr uint32_t kExpectedStateLen = duel::wire::StateLen(kTeamSize, kLoadoutSize);
  CHECK(kExpectedStateLen == 80); // 8 + 2*2*(12+3*2)
  CHECK(duel_out_len() == kExpectedStateLen);

  const uint8_t* out = duel_out_ptr();
  CHECK(out[duel::wire::kStateOffVersion] == duel::wire::kVersion);
  CHECK(out[duel::wire::kStateOffTeamSize] == kTeamSize);
  CHECK(out[duel::wire::kStateOffLoadoutSize] == kLoadoutSize);
  CHECK(out[duel::wire::kStateOffPhase] == duel::wire::kPhaseActive);

  const uint8_t moves00[2] = {0, 0xFF};
  const uint8_t moves01[2] = {1, 2};
  const uint8_t moves10[2] = {3, 0xFF};
  const uint8_t moves11[2] = {4, 5};
  CheckStateTreat(out, kTeamSize, kLoadoutSize, 0, 0, 1, 2, 1, moves00, 120);
  CheckStateTreat(out, kTeamSize, kLoadoutSize, 0, 1, 2, 5, 2, moves01, 180);
  CheckStateTreat(out, kTeamSize, kLoadoutSize, 1, 0, 3, 8, 1, moves10, 240);
  CheckStateTreat(out, kTeamSize, kLoadoutSize, 1, 1, 4, 1, 2, moves11, 200);
}

// Every reject case from the plan's Task 3 list, each mutating exactly one
// field of an otherwise-valid baseline config.
void test_duel_init_rejects_invalid_configs() {
  uint8_t cfg[kBaselineCfgLen];

  MakeBaselineCfg(cfg);
  // Relative to kVersion (not a hardcoded "2") so a future version bump can
  // never silently make this mutation ACCEPT a buffer it should reject.
  cfg[duel::wire::kCfgOffVersion] =
      static_cast<uint8_t>(duel::wire::kVersion + 1); // bad version
  CHECK(duel_init(cfg, kBaselineCfgLen) == -1);
  CHECK(duel_out_len() == 0);

  MakeBaselineCfg(cfg);
  cfg[duel::wire::kCfgOffTeamSize] = 0; // team_size 0
  CHECK(duel_init(cfg, kBaselineCfgLen) == -1);
  CHECK(duel_out_len() == 0);

  MakeBaselineCfg(cfg);
  cfg[duel::wire::kCfgOffTeamSize] = 6; // team_size 6 (> max 5)
  CHECK(duel_init(cfg, kBaselineCfgLen) == -1);
  CHECK(duel_out_len() == 0);

  MakeBaselineCfg(cfg);
  cfg[duel::wire::kCfgOffLoadoutSize] = 0; // loadout_size 0
  CHECK(duel_init(cfg, kBaselineCfgLen) == -1);
  CHECK(duel_out_len() == 0);

  MakeBaselineCfg(cfg);
  cfg[duel::wire::kCfgOffLoadoutSize] = 5; // loadout_size 5 (> max 4)
  CHECK(duel_init(cfg, kBaselineCfgLen) == -1);
  CHECK(duel_out_len() == 0);

  MakeBaselineCfg(cfg);
  CHECK(duel_init(cfg, kBaselineCfgLen - 1) == -1); // too short
  CHECK(duel_out_len() == 0);

  {
    uint8_t cfgLong[kBaselineCfgLen + 1];
    MakeBaselineCfg(cfgLong);
    cfgLong[kBaselineCfgLen] = 0; // extra trailing byte
    CHECK(duel_init(cfgLong, kBaselineCfgLen + 1) == -1); // too long
    CHECK(duel_out_len() == 0);
  }

  const uint32_t treat0Quality =
      duel::wire::kCfgOffTeams + duel::wire::kCfgTreatOffQuality;
  MakeBaselineCfg(cfg);
  cfg[treat0Quality] = 0; // quality 0
  CHECK(duel_init(cfg, kBaselineCfgLen) == -1);
  CHECK(duel_out_len() == 0);

  MakeBaselineCfg(cfg);
  cfg[treat0Quality] = 5; // quality 5 (> max 4)
  CHECK(duel_init(cfg, kBaselineCfgLen) == -1);
  CHECK(duel_out_len() == 0);

  const uint32_t treat0Sweetness =
      duel::wire::kCfgOffTeams + duel::wire::kCfgTreatOffSweetness;
  MakeBaselineCfg(cfg);
  cfg[treat0Sweetness] = 0; // sweetness 0
  CHECK(duel_init(cfg, kBaselineCfgLen) == -1);
  CHECK(duel_out_len() == 0);

  MakeBaselineCfg(cfg);
  cfg[treat0Sweetness] = 11; // sweetness 11 (> max 10)
  CHECK(duel_init(cfg, kBaselineCfgLen) == -1);
  CHECK(duel_out_len() == 0);

  const uint32_t treat0MoveCount =
      duel::wire::kCfgOffTeams + duel::wire::kCfgTreatOffMoveCount;
  MakeBaselineCfg(cfg);
  cfg[treat0MoveCount] = 0; // move_count 0
  CHECK(duel_init(cfg, kBaselineCfgLen) == -1);
  CHECK(duel_out_len() == 0);

  MakeBaselineCfg(cfg);
  cfg[treat0MoveCount] = 5; // move_count 5 (> loadout_size 4)
  CHECK(duel_init(cfg, kBaselineCfgLen) == -1);
  CHECK(duel_out_len() == 0);

  const uint32_t treat0Moves =
      duel::wire::kCfgOffTeams + duel::wire::kCfgTreatOffMoves;
  MakeBaselineCfg(cfg);
  cfg[treat0Moves + 0] = 39; // move index >= 39 (universe is 39)
  CHECK(duel_init(cfg, kBaselineCfgLen) == -1);
  CHECK(duel_out_len() == 0);

  MakeBaselineCfg(cfg);
  cfg[treat0Moves + 3] = 5; // filler slot (move_count=3, index3) must be 0xFF
  CHECK(duel_init(cfg, kBaselineCfgLen) == -1);
  CHECK(duel_out_len() == 0);

  MakeBaselineCfg(cfg);
  cfg[treat0Moves + 1] = cfg[treat0Moves + 0]; // duplicate move index (0,0,2)
  CHECK(duel_init(cfg, kBaselineCfgLen) == -1);
  CHECK(duel_out_len() == 0);

  MakeBaselineCfg(cfg);
  cfg[duel::wire::kCfgOffReserved] = 0x01; // non-canonical reserved byte
  CHECK(duel_init(cfg, kBaselineCfgLen) == -1);
  CHECK(duel_out_len() == 0);
}

// ---- Task 3: state codec (encode_state/decode_state) reused by Task 4 ----

void test_state_codec_roundtrip() {
  duel::DuelState s{};
  s.version = duel::wire::kVersion;
  s.team_size = 2;
  s.loadout_size = 3;
  s.phase = duel::wire::kPhaseActive;
  s.round = 7;

  {
    duel::TreatState& t = s.treats[0][0];
    t.hp = 55; t.max_hp = 120; t.quality = 2; t.sweetness = 5; t.move_count = 2;
    t.moves[0] = 1; t.moves[1] = 4; t.moves[2] = duel::wire::kMoveSlotEmpty;
    t.cooldowns[0] = 0; t.cooldowns[1] = 2; t.cooldowns[2] = 0;
    // v2 positive round-trip: shield = 42 and a real-slot uses_left = 2 must
    // survive byte-identically; the filler slot (index 2, move_count == 2)
    // stays canonically 0 (default zero-init, untouched here).
    t.shield = 42;
    t.uses_left[0] = 2; t.uses_left[1] = 200;
  }
  {
    duel::TreatState& t = s.treats[0][1];
    t.hp = 200; t.max_hp = 200; t.quality = 4; t.sweetness = 10; t.move_count = 3;
    t.moves[0] = 10; t.moves[1] = 20; t.moves[2] = 30;
    t.cooldowns[0] = 1; t.cooldowns[1] = 1; t.cooldowns[2] = 3;
    t.uses_left[0] = 255; t.uses_left[1] = 1; t.uses_left[2] = 0;
  }
  {
    duel::TreatState& t = s.treats[1][0];
    t.hp = 0; t.max_hp = 110; t.quality = 1; t.sweetness = 1; t.move_count = 1;
    t.moves[0] = 0; t.moves[1] = duel::wire::kMoveSlotEmpty; t.moves[2] = duel::wire::kMoveSlotEmpty;
    t.cooldowns[0] = 0; t.cooldowns[1] = 0; t.cooldowns[2] = 0;
    t.uses_left[0] = 255;
  }
  {
    duel::TreatState& t = s.treats[1][1];
    t.hp = 80; t.max_hp = 80; t.quality = 1; t.sweetness = 10; t.move_count = 3;
    t.moves[0] = 38; t.moves[1] = 5; t.moves[2] = 6;
    t.cooldowns[0] = 0; t.cooldowns[1] = 0; t.cooldowns[2] = 0;
    t.uses_left[0] = 0; t.uses_left[1] = 0; t.uses_left[2] = 0;
  }

  constexpr uint32_t kLen = duel::wire::StateLen(2, 3);
  CHECK(kLen == 92); // 8 + 2*2*(12+3*3)
  uint8_t buf[kLen];
  CHECK(duel::encode_state(s, buf, sizeof(buf)) == kLen);

  duel::DuelState decoded{};
  CHECK(duel::decode_state(buf, kLen, &decoded));
  CHECK(decoded.version == s.version);
  CHECK(decoded.team_size == s.team_size);
  CHECK(decoded.loadout_size == s.loadout_size);
  CHECK(decoded.phase == s.phase);
  CHECK(decoded.round == s.round);
  for (uint32_t side = 0; side < 2; ++side) {
    for (uint32_t slot = 0; slot < s.team_size; ++slot) {
      const duel::TreatState& a = s.treats[side][slot];
      const duel::TreatState& b = decoded.treats[side][slot];
      CHECK(a.hp == b.hp);
      CHECK(a.max_hp == b.max_hp);
      CHECK(a.quality == b.quality);
      CHECK(a.sweetness == b.sweetness);
      CHECK(a.move_count == b.move_count);
      CHECK(a.shield == b.shield);
      for (uint32_t k = 0; k < duel::wire::kStateTreatReservedLen; ++k) {
        CHECK(a.reserved[k] == b.reserved[k]);
      }
      for (uint32_t j = 0; j < s.loadout_size; ++j) {
        CHECK(a.moves[j] == b.moves[j]);
        CHECK(a.cooldowns[j] == b.cooldowns[j]);
        CHECK(a.uses_left[j] == b.uses_left[j]);
      }
    }
  }

  // encode(decode(buf)) reproduces buf byte-identically -- the actual
  // "encode -> decode -> encode" round-trip the v2 shield/uses_left fields
  // must satisfy (not just a struct-field comparison).
  uint8_t buf2[kLen];
  CHECK(duel::encode_state(decoded, buf2, sizeof(buf2)) == kLen);
  CHECK(std::memcmp(buf, buf2, kLen) == 0);

  // cap smaller than the required length: no partial write, 0 returned.
  uint8_t tiny[kLen - 1];
  CHECK(duel::encode_state(s, tiny, sizeof(tiny)) == 0);
}

void test_state_codec_decode_rejects_bad_state() {
  uint8_t cfg[kBaselineCfgLen];
  MakeBaselineCfg(cfg);
  CHECK(duel_init(cfg, kBaselineCfgLen) == 0);
  CHECK(duel_out_len() == kBaselineStateLen);
  uint8_t st[kBaselineStateLen];
  for (uint32_t i = 0; i < kBaselineStateLen; ++i) {
    st[i] = duel_out_ptr()[i];
  }

  duel::DuelState decoded{};
  CHECK(duel::decode_state(st, kBaselineStateLen, &decoded)); // sanity: the good copy decodes

  CHECK(duel::decode_state(st, kBaselineStateLen - 1, &decoded) == false); // too short
  {
    uint8_t big[kBaselineStateLen + 1]; // genuinely larger buffer, not just a bigger claimed len
    for (uint32_t i = 0; i < kBaselineStateLen; ++i) {
      big[i] = st[i];
    }
    big[kBaselineStateLen] = 0;
    CHECK(duel::decode_state(big, kBaselineStateLen + 1, &decoded) == false); // too long
  }

  uint8_t bad[kBaselineStateLen];
  auto reset = [&]() {
    for (uint32_t i = 0; i < kBaselineStateLen; ++i) {
      bad[i] = st[i];
    }
  };

  reset();
  // Relative to kVersion so a future bump can never silently ACCEPT this.
  bad[duel::wire::kStateOffVersion] = static_cast<uint8_t>(duel::wire::kVersion + 1);
  CHECK(duel::decode_state(bad, kBaselineStateLen, &decoded) == false); // bad version

  reset();
  bad[duel::wire::kStateOffTeamSize] = 0;
  CHECK(duel::decode_state(bad, kBaselineStateLen, &decoded) == false); // team_size 0

  reset();
  bad[duel::wire::kStateOffTeamSize] = 6;
  CHECK(duel::decode_state(bad, kBaselineStateLen, &decoded) == false); // team_size 6

  reset();
  bad[duel::wire::kStateOffLoadoutSize] = 0;
  CHECK(duel::decode_state(bad, kBaselineStateLen, &decoded) == false); // loadout_size 0

  reset();
  bad[duel::wire::kStateOffPhase] = 4;
  CHECK(duel::decode_state(bad, kBaselineStateLen, &decoded) == false); // phase out of range

  // side0/slot0's moves array: baseline echoes moves[0,1,2,0xFF], move_count 3.
  const uint32_t treat0MovesOff =
      duel::wire::kStateOffTeams + duel::wire::kStateTreatOffMoves;
  reset();
  bad[treat0MovesOff + 0] = 39;
  CHECK(duel::decode_state(bad, kBaselineStateLen, &decoded) == false); // move index >= 39

  reset();
  bad[treat0MovesOff + 1] = bad[treat0MovesOff + 0];
  CHECK(duel::decode_state(bad, kBaselineStateLen, &decoded) == false); // duplicate move

  reset();
  bad[treat0MovesOff + 3] = 5;
  CHECK(duel::decode_state(bad, kBaselineStateLen, &decoded) == false); // filler != 0xFF

  // Canonical encoding: non-zero reserved bytes must be rejected -- a buffer
  // that validates-but-hashes-differently from the canonical bytes would be
  // a signature-malleability hazard once states are hashed/signed inside
  // game channels.
  reset();
  bad[duel::wire::kStateOffReserved + 1] = 0x01; // header reserved u16 = 0x0100
  CHECK(duel::decode_state(bad, kBaselineStateLen, &decoded) == false);

  // v2: the treat-level reserved[4] bytes (future stun/dot/atk_mod/spd_mod)
  // MUST each independently be 0 -- 4 distinct reject cases. shield (the
  // byte the old pad-must-be-zero case used to sit on, offset 7) is now a
  // FREE byte instead -- see the positive 0xFF round-trip check below.
  const uint32_t treat0ReservedOff =
      duel::wire::kStateOffTeams + duel::wire::kStateTreatOffReserved;
  for (uint32_t k = 0; k < duel::wire::kStateTreatReservedLen; ++k) {
    reset();
    bad[treat0ReservedOff + k] = 0xFF;
    CHECK(duel::decode_state(bad, kBaselineStateLen, &decoded) == false); // reserved[k] != 0
  }

  // shield is a free byte now (v2) -- every value, including the old pad
  // sentinel 0xFF, must round-trip rather than reject.
  reset();
  bad[duel::wire::kStateOffTeams + duel::wire::kStateTreatOffShield] = 0xFF;
  CHECK(duel::decode_state(bad, kBaselineStateLen, &decoded)); // shield is free

  // Filler-slot cooldowns MUST be 0 too: side0/slot0 has move_count 3 with
  // loadout 4, so cooldown index 3 is a filler slot. A non-zero byte there is
  // a second byte-encoding of the same logical state (the value is
  // meaningless -- no move lives in that slot) -- reject it, or two
  // byte-distinct states would decode/resolve/re-encode differently
  // (malleability in hashed/signed channel states).
  const uint32_t treat0FillerCd =
      duel::wire::kStateOffTeams +
      duel::wire::StateTreatOffCooldowns(kBaselineLoadoutSize) + 3;
  reset();
  bad[treat0FillerCd] = 1;
  CHECK(duel::decode_state(bad, kBaselineStateLen, &decoded) == false); // filler cd != 0

  // v2 (controller resolution R4): the same rule for filler uses_left --
  // side0/slot0's index 3 is filler (move_count 3, loadout 4), so its
  // uses_left byte is meaningless and MUST be 0.
  const uint32_t treat0FillerUsesLeft =
      duel::wire::kStateOffTeams +
      duel::wire::StateTreatOffUsesLeft(kBaselineLoadoutSize) + 3;
  reset();
  bad[treat0FillerUsesLeft] = 1;
  CHECK(duel::decode_state(bad, kBaselineStateLen, &decoded) == false); // filler uses_left != 0

  // ... but a REAL slot's uses_left is a free byte (v2): any value round-trips.
  const uint32_t treat0RealUsesLeft =
      duel::wire::kStateOffTeams +
      duel::wire::StateTreatOffUsesLeft(kBaselineLoadoutSize) + 0;
  reset();
  bad[treat0RealUsesLeft] = 0xFF;
  CHECK(duel::decode_state(bad, kBaselineStateLen, &decoded)); // real-slot uses_left is free

  // ... and duel_apply rejects the same buffer outright (lens cleared).
  // Orders: all six treats Recover (version byte, then 0xFE/0xFF pairs).
  {
    reset();
    bad[treat0FillerCd] = 1; // back to a genuinely-invalid buffer for this check
    constexpr uint32_t kOrdLen = duel::wire::OrdersLen(kBaselineTeamSize); // 13
    uint8_t ord[kOrdLen];
    ord[duel::wire::kOrdersOffVersion] = duel::wire::kVersion;
    for (uint32_t i = duel::wire::kOrdersOffTeams; i < kOrdLen; i += 2) {
      ord[i] = duel::wire::kActionRecover;
      ord[i + 1] = duel::wire::kTargetNone;
    }
    CHECK(duel_apply(bad, kBaselineStateLen, ord, kOrdLen) == -1);
    CHECK(duel_out_len() == 0);
    CHECK(duel_log_len() == 0);
  }
}

// ---- Task 4: duel_apply (round resolve + log) shared helpers ----

// Build a fresh active DuelState header (treats zero-filled; caller sets them).
duel::DuelState MakeState(uint8_t teamSize, uint8_t loadoutSize,
                          uint16_t round = 0) {
  duel::DuelState s{};
  s.version = duel::wire::kVersion;
  s.team_size = teamSize;
  s.loadout_size = loadoutSize;
  s.phase = duel::wire::kPhaseActive;
  s.round = round;
  return s;
}

// Fill one treat: the first `mc` loadout slots take realMoves/realCds; the rest
// get the 0xFF filler + cooldown 0 (canonical, so encode/decode round-trips).
void SetTreat(duel::DuelState& s, int side, int slot, uint16_t hp, uint16_t maxhp,
              uint8_t q, uint8_t sw, uint8_t mc, const uint8_t* realMoves,
              const uint8_t* realCds) {
  duel::TreatState& t = s.treats[side][slot];
  t.hp = hp;
  t.max_hp = maxhp;
  t.quality = q;
  t.sweetness = sw;
  t.move_count = mc;
  for (int j = 0; j < s.loadout_size; ++j) {
    if (j < mc) {
      t.moves[j] = realMoves[j];
      t.cooldowns[j] = realCds[j];
    } else {
      t.moves[j] = duel::wire::kMoveSlotEmpty;
      t.cooldowns[j] = 0;
    }
  }
}

void OrdInit(uint8_t* ord) { ord[duel::wire::kOrdersOffVersion] = duel::wire::kVersion; }

void OrdSet(uint8_t* ord, int teamSize, int side, int slot, uint8_t a, uint8_t t) {
  const uint32_t off = duel::wire::kOrdersOffTeams +
                       side * duel::wire::OrdersSideLen(teamSize) +
                       slot * duel::wire::kOrdersTreatLen;
  ord[off + duel::wire::kOrdersTreatOffAction] = a;
  ord[off + duel::wire::kOrdersTreatOffTarget] = t;
}

void OrdRecoverAll(uint8_t* ord, int teamSize) {
  OrdInit(ord);
  for (int side = 0; side < 2; ++side) {
    for (int slot = 0; slot < teamSize; ++slot) {
      OrdSet(ord, teamSize, side, slot, duel::wire::kActionRecover,
             duel::wire::kTargetNone);
    }
  }
}

bool DecodeOut(duel::DuelState* out) {
  return duel::decode_state(duel_out_ptr(), duel_out_len(), out);
}

bool LogHas(const char* needle) {
  const uint8_t* p = duel_log_ptr();
  const uint32_t n = duel_log_len();
  const uint32_t m = static_cast<uint32_t>(std::strlen(needle));
  if (m == 0) return true;
  if (m > n) return false;
  for (uint32_t i = 0; i + m <= n; ++i) {
    if (std::memcmp(p + i, needle, m) == 0) return true;
  }
  return false;
}

int LogIndexOf(const char* needle) {
  const uint8_t* p = duel_log_ptr();
  const uint32_t n = duel_log_len();
  const uint32_t m = static_cast<uint32_t>(std::strlen(needle));
  if (m == 0 || m > n) return -1;
  for (uint32_t i = 0; i + m <= n; ++i) {
    if (std::memcmp(p + i, needle, m) == 0) return static_cast<int>(i);
  }
  return -1;
}

// Dense move indices used in the behavioral vectors (from stats_gen.h):
//   [3]  Coco Chaos          {power 24, speed 98, cd 0, Speedy}
//   [6]  Pucker Sucker       {power 24, speed 57, cd 0, Tricky}
//   [10] Super Sugary Rush   {power 51, speed 97, cd 3, Speedy}
//   [18] Sugar Shield        {power 12, speed 43, cd 0, Blocking}
//   [24] Pop Rock Pop        {power 26, speed 27, cd 0, Heavy}
//   [25] Bubble Trouble      {power 25, speed 76, cd 0, Distance}
//   [26] Vicious Jawbreaker  {power 40, speed 28, cd 2, Heavy}

// jsonout: jb_i32 must not overflow when negating INT32_MIN (classic edge; the
// helper widens to int64 first). Log/view use jb_u32, but this pins jb_i32.
void test_jsonout_i32_min() {
  uint8_t buf[16];
  duel::JsonBuf b{buf, sizeof(buf), 0};
  duel::jb_i32(&b, INT32_MIN);
  const char expected[] = "-2147483648"; // 11 chars, no overflow/UB
  CHECK(b.len == sizeof(expected) - 1);
  CHECK(std::memcmp(buf, expected, b.len) == 0);
}

// ---- Task 5: golden-vector plumbing ----
//
// ApplyVec/InitVec bundle a NAMED (cfg|state)+orders byte buffer so the same
// construction a test uses to exercise duel_init/duel_apply also feeds
// DumpGolden() (bottom of this file, `--dump-golden` mode) without
// duplicating any byte literals -- each MakeVec_*/Make*Cfg builder below is
// the single source of truth for that vector's bytes; the test function and
// DumpGolden() both call it.
struct ApplyVec {
  const char* name;
  uint8_t st[kMaxStateLen];
  uint32_t stLen;
  uint8_t ord[32];
  uint32_t ordLen;
};
struct InitVec {
  const char* name;
  uint8_t cfg[80];
  uint32_t cfgLen;
};

void PrintHexBytes(const uint8_t* p, uint32_t n) {
  static const char kHex[] = "0123456789abcdef";
  for (uint32_t i = 0; i < n; ++i) {
    std::putchar(kHex[p[i] >> 4]);
    std::putchar(kHex[p[i] & 0x0F]);
  }
}

// Prints s[0..n) as a JSON string body, escaping '"' and '\\'. The log
// buffer's own JSON content (jsonout.h) never emits any other character
// needing escape (digits/letters/`{}[]:,"-` only), so this covers everything
// actually reachable here.
void PrintJsonEscaped(const uint8_t* s, uint32_t n) {
  for (uint32_t i = 0; i < n; ++i) {
    const uint8_t c = s[i];
    if (c == '"' || c == '\\') std::putchar('\\');
    std::putchar(static_cast<char>(c));
  }
}

// One golden line per vector: {"name","rc","cfgHex","outHex","logJson":""}.
// `rc` lets parity.mjs assert the reject path too (a straightforward
// addition beyond the plan's {name,cfgHex|stateHex,ordersHex,outHex,logJson}
// minimum -- reject vectors are meaningless to byte-compare without it).
// logJson is always "" here: duel_init never touches the log buffer, so
// reading it here would surface whatever a PRIOR call left behind.
void DumpInitVector(const InitVec& v) {
  const int32_t rc = duel_init(v.cfg, v.cfgLen);
  std::printf("{\"name\":\"%s\",\"rc\":%d,\"cfgHex\":\"", v.name, rc);
  PrintHexBytes(v.cfg, v.cfgLen);
  std::printf("\",\"outHex\":\"");
  if (rc == 0) PrintHexBytes(duel_out_ptr(), duel_out_len());
  std::printf("\",\"logJson\":\"\"}\n");
}

// One golden line per vector: {"name","rc","stateHex","ordersHex","outHex",
// "logJson"} -- logJson is the round-log JSON text verbatim (empty string on
// reject, matching duel_log_len()==0 there).
void DumpApplyVector(const ApplyVec& v) {
  const int32_t rc = duel_apply(v.st, v.stLen, v.ord, v.ordLen);
  std::printf("{\"name\":\"%s\",\"rc\":%d,\"stateHex\":\"", v.name, rc);
  PrintHexBytes(v.st, v.stLen);
  std::printf("\",\"ordersHex\":\"");
  PrintHexBytes(v.ord, v.ordLen);
  std::printf("\",\"outHex\":\"");
  if (rc == 0) PrintHexBytes(duel_out_ptr(), duel_out_len());
  std::printf("\",\"logJson\":\"");
  if (rc == 0) PrintJsonEscaped(duel_log_ptr(), duel_log_len());
  std::printf("\"}\n");
}

// Init-reject vector: the baseline config with side0/slot0's quality set to 5
// (> max 4). duel_init must return -1 with empty output. Extracted from the
// inline quality-5 case in test_duel_init_rejects_invalid_configs into a
// builder (single source of truth for test + DumpGolden) so the parity gate
// exercises a duel_init REJECT path (rc -1, no outHex) across native/wasm,
// not only the accept path — decode-time field validation is exactly where
// codegen divergence could bite. Quality (a real config field, unlike the
// header version/reserved bytes) is chosen so the FE's typed-config replay
// can reconstruct and re-reject the same vector too (tf-frontend
// tests/duel/engine.test.ts) — its encodeConfig guard refuses quality 5
// client-side, so the bad config never even reaches a fresh duel.
InitVec MakeVec_InitRejectBadQuality() {
  InitVec v{};
  v.name = "init_reject_bad_quality";
  MakeBaselineCfg(v.cfg);
  v.cfg[duel::wire::kCfgOffTeams + duel::wire::kCfgTreatOffQuality] = 5; // > max 4 → reject
  v.cfgLen = kBaselineCfgLen;
  return v;
}

void test_duel_init_reject_vector() {
  InitVec v = MakeVec_InitRejectBadQuality();
  CHECK(duel_init(v.cfg, v.cfgLen) == -1);
  CHECK(duel_out_len() == 0);
}

// Case 1 — Clash advantage: Heavy(power 40) vs a target that picked Speedy →
// Heavy>Speedy → adv ×384 → dmg = (40*384)>>8 = 60.
ApplyVec MakeVec_ClashAdvantage() {
  ApplyVec v{};
  v.name = "clash_advantage";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mH[] = {26}, cH[] = {0}; // Heavy power 40
  const uint8_t mS[] = {3}, cS[] = {0};  // Speedy
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 0, 250, 250, 1, 1, 1, mS, cS);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 1, 0, 0, 0, 0);
  OrdSet(v.ord, 1, 1, 0, 0, 0);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

void test_apply_clash_advantage() {
  ApplyVec v = MakeVec_ClashAdvantage();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 190); // 250 - 60 (adv)
  CHECK(o.treats[0][0].hp == 235); // Speedy vs Heavy = dis 170: 24*170>>8 = 15
  CHECK(LogHas("\"clash\":\"adv\""));
  CHECK(LogHas("\"dmg\":60"));
  CHECK(o.phase == duel::wire::kPhaseActive);
}

// Case 2 — Disadvantage: Heavy vs a target that picked Tricky → Tricky>Heavy →
// dis ×170 → dmg = (40*170)>>8 = 26.
ApplyVec MakeVec_ClashDisadvantage() {
  ApplyVec v{};
  v.name = "clash_disadvantage";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mH[] = {26}, cH[] = {0}; // Heavy power 40
  const uint8_t mT[] = {6}, cT[] = {0};  // Tricky
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 0, 250, 250, 1, 1, 1, mT, cT);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 1, 0, 0, 0, 0);
  OrdSet(v.ord, 1, 1, 0, 0, 0);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

void test_apply_clash_disadvantage() {
  ApplyVec v = MakeVec_ClashDisadvantage();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 224); // 250 - 26 (dis)
  CHECK(LogHas("\"clash\":\"dis\""));
  CHECK(LogHas("\"dmg\":26"));
}

// Case 3 — Block stance. (A) Heavy(power 40) vs a Blocking target → adv ×384
// then block: ((40*384)>>8 *154)>>8 = 36. (B) the stance holds even when the
// blocker's own action fires LAST (attacker faster).
ApplyVec MakeVec_BlockStanceA() {
  ApplyVec v{};
  v.name = "block_stance_a";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mH[] = {26}, cH[] = {0}; // Heavy power 40, speed 28
  const uint8_t mB[] = {18}, cB[] = {0}; // Blocking, speed 43 (acts first here)
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 0, 250, 250, 1, 1, 1, mB, cB);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 1, 0, 0, 0, 0);
  OrdSet(v.ord, 1, 1, 0, 0, 0);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

ApplyVec MakeVec_BlockStanceLate() {
  ApplyVec v{};
  v.name = "block_stance_late_blocker";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mS[] = {3}, cS[] = {0};  // Speedy, speed 98 (fires first)
  const uint8_t mB[] = {18}, cB[] = {0}; // Blocking, speed 43 (fires last)
  SetTreat(s, 0, 0, 250, 250, 1, 1, 1, mS, cS);
  SetTreat(s, 1, 0, 250, 250, 1, 1, 1, mB, cB);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 1, 0, 0, 0, 0);
  OrdSet(v.ord, 1, 1, 0, 0, 0);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

void test_apply_block_stance() {
  // (A) exact 36
  {
    ApplyVec v = MakeVec_BlockStanceA();
    CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
    duel::DuelState o{};
    CHECK(DecodeOut(&o));
    CHECK(o.treats[1][0].hp == 214); // 250 - 36 (adv then block)
    CHECK(LogHas("\"blocked\":true"));
    CHECK(LogHas("\"dmg\":36"));
    CHECK(LogHas("\"clash\":\"adv\""));
  }
  // (B) stance active even though the blocker resolves last
  {
    ApplyVec v = MakeVec_BlockStanceLate();
    CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
    duel::DuelState o{};
    CHECK(DecodeOut(&o));
    // Speedy vs Blocking = dis 170: (24*170)>>8 = 15; block: (15*154)>>8 = 9.
    CHECK(o.treats[1][0].hp == 241); // 250 - 9, blocked though blocker acts last
    CHECK(LogHas("\"blocked\":true"));
    const int i0 = LogIndexOf("\"side\":0");
    const int i1 = LogIndexOf("\"side\":1");
    CHECK(i0 >= 0);
    CHECK(i1 >= 0);
    CHECK(i0 < i1); // attacker (speed 98) resolves before the blocker (speed 43)
  }
}

// Case 4 — Speed order + KO skip: a fast Speedy KOs a slower treat before it
// acts → the victim's own action is logged "skip".
ApplyVec MakeVec_SpeedOrderKoSkip() {
  ApplyVec v{};
  v.name = "speed_order_ko_skip";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mS[] = {10}, cS[] = {0}; // Speedy power 51, speed 97
  const uint8_t mD[] = {25}, cD[] = {0}; // Distance, speed 76
  SetTreat(s, 0, 0, 250, 250, 4, 10, 1, mS, cS);
  SetTreat(s, 1, 0, 40, 110, 1, 1, 1, mD, cD);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 1, 0, 0, 0, 0);
  OrdSet(v.ord, 1, 1, 0, 0, 0);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

void test_apply_speed_order_ko_skip() {
  ApplyVec v = MakeVec_SpeedOrderKoSkip();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 0);
  CHECK(LogHas("\"dmg\":76")); // Speedy>Distance adv: 51*384>>8 = 76
  CHECK(LogHas("\"ko\":true"));
  CHECK(LogHas("{\"side\":1,\"slot\":0,\"kind\":\"skip\"")); // victim skipped
  CHECK(o.phase == duel::wire::kPhaseSide0Won);
  const int i0 = LogIndexOf("\"side\":0"); // killer acts before victim
  const int i1 = LogIndexOf("\"side\":1");
  CHECK(i0 >= 0);
  CHECK(i1 >= 0);
  CHECK(i0 < i1);
}

// Case 5 — Retarget: two attackers aim at the same 1-hp treat; the first KOs
// it, the second retargets to the lowest-hp living enemy (retargeted:true).
ApplyVec MakeVec_Retarget() {
  ApplyVec v{};
  v.name = "retarget";
  duel::DuelState s = MakeState(2, 1);
  const uint8_t mS[] = {10}, cS[] = {0}; // Speedy power 51, speed 97
  const uint8_t mH[] = {26}, cH[] = {0}; // Heavy  power 40, speed 28
  const uint8_t mD[] = {24}, cD[] = {0};
  SetTreat(s, 0, 0, 250, 250, 4, 10, 1, mS, cS);
  SetTreat(s, 0, 1, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 0, 1, 110, 1, 1, 1, mD, cD);   // 1 hp → dies to attacker 1
  SetTreat(s, 1, 1, 50, 110, 1, 1, 1, mD, cD);  // living → retarget lands here
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 2, 0, 0, 0, 0); // both attackers aim at enemy slot 0
  OrdSet(v.ord, 2, 0, 1, 0, 0);
  OrdSet(v.ord, 2, 1, 0, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 2, 1, 1, duel::wire::kActionRecover, duel::wire::kTargetNone);
  v.ordLen = duel::wire::OrdersLen(2);
  return v;
}

void test_apply_retarget() {
  ApplyVec v = MakeVec_Retarget();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 0);
  CHECK(o.treats[1][1].hp == 10); // 50 - 40 (Heavy vs recover = neutral)
  CHECK(LogHas("\"retargeted\":true"));
  CHECK(LogHas(
      "{\"side\":0,\"slot\":1,\"kind\":\"hit\",\"move\":26,\"target\":1,"
      "\"retargeted\":true,\"clash\":\"neu\",\"dmg\":40,\"blocked\":false,"
      "\"targetHp\":10,\"ko\":false}"));
  CHECK(o.phase == duel::wire::kPhaseActive);
}

// Case 5b — Retarget among MULTIPLE living enemies: a fast Speedy KOs the
// aimed-at slot 0, then a slower Heavy (which also aimed at slot 0) must
// retarget to the LOWEST-HP living enemy — slot 2 (60 hp) — NOT the lowest
// living SLOT (slot 1, 100 hp). This is the comparator's hp-vs-slot branch,
// which the single-living-enemy retarget vector can't discriminate.
ApplyVec MakeVec_RetargetMultiLiving() {
  ApplyVec v{};
  v.name = "retarget_multi_living_lowest_hp";
  duel::DuelState s = MakeState(3, 1);
  const uint8_t mS[] = {10}, cS[] = {0}; // Speedy power 51, speed 97 — KOs slot 0
  const uint8_t mH[] = {26}, cH[] = {0}; // Heavy  power 40, speed 28 — retargets
  const uint8_t mD[] = {24}, cD[] = {0}; // filler move for the recovering treats
  SetTreat(s, 0, 0, 250, 250, 4, 10, 1, mS, cS);
  SetTreat(s, 0, 1, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 0, 2, 250, 250, 1, 1, 1, mD, cD);  // recovers
  SetTreat(s, 1, 0, 1, 110, 1, 1, 1, mD, cD);    // 1 hp → dies to attacker 0
  SetTreat(s, 1, 1, 100, 110, 1, 1, 1, mD, cD);  // higher-hp living (NOT chosen)
  SetTreat(s, 1, 2, 60, 110, 1, 1, 1, mD, cD);   // lowest-hp living → retarget lands
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 3, 0, 0, 0, 0); // Speedy → enemy slot 0
  OrdSet(v.ord, 3, 0, 1, 0, 0); // Heavy  → enemy slot 0 (dead by then → retarget)
  OrdSet(v.ord, 3, 0, 2, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 1, 0, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 1, 1, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 1, 2, duel::wire::kActionRecover, duel::wire::kTargetNone);
  v.ordLen = duel::wire::OrdersLen(3);
  return v;
}

void test_apply_retarget_multi_living() {
  ApplyVec v = MakeVec_RetargetMultiLiving();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 0);   // slot 0 KO'd by the fast Speedy
  CHECK(o.treats[1][1].hp == 100); // higher-hp living enemy left untouched
  CHECK(o.treats[1][2].hp == 20);  // lowest-hp living enemy chosen: 60 - 40
  CHECK(LogHas("\"retargeted\":true"));
  // The retargeted Heavy aims at slot 2 (lowest hp), NOT slot 1 (lowest slot).
  CHECK(LogHas(
      "{\"side\":0,\"slot\":1,\"kind\":\"hit\",\"move\":26,\"target\":2,"
      "\"retargeted\":true,\"clash\":\"neu\",\"dmg\":40,\"blocked\":false,"
      "\"targetHp\":20,\"ko\":false}"));
  CHECK(o.phase == duel::wire::kPhaseActive);
}

// Case 5c — Retarget TIE: after slot 0 is KO'd, the two living enemies (slots
// 1 and 2) have EQUAL hp, so the "lowest hp" comparator ties and must fall
// back to lowest SLOT — slot 1 wins, slot 2 is untouched.
ApplyVec MakeVec_RetargetTieLowestSlot() {
  ApplyVec v{};
  v.name = "retarget_tie_lowest_slot";
  duel::DuelState s = MakeState(3, 1);
  const uint8_t mS[] = {10}, cS[] = {0};
  const uint8_t mH[] = {26}, cH[] = {0};
  const uint8_t mD[] = {24}, cD[] = {0};
  SetTreat(s, 0, 0, 250, 250, 4, 10, 1, mS, cS);
  SetTreat(s, 0, 1, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 0, 2, 250, 250, 1, 1, 1, mD, cD);
  SetTreat(s, 1, 0, 1, 110, 1, 1, 1, mD, cD);   // dies to attacker 0
  SetTreat(s, 1, 1, 80, 110, 1, 1, 1, mD, cD);  // equal-hp living, lower slot → chosen
  SetTreat(s, 1, 2, 80, 110, 1, 1, 1, mD, cD);  // equal-hp living, higher slot
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 3, 0, 0, 0, 0);
  OrdSet(v.ord, 3, 0, 1, 0, 0);
  OrdSet(v.ord, 3, 0, 2, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 1, 0, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 1, 1, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 1, 2, duel::wire::kActionRecover, duel::wire::kTargetNone);
  v.ordLen = duel::wire::OrdersLen(3);
  return v;
}

void test_apply_retarget_tie_lowest_slot() {
  ApplyVec v = MakeVec_RetargetTieLowestSlot();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 0);  // KO'd
  CHECK(o.treats[1][1].hp == 40); // tie → lowest slot chosen: 80 - 40
  CHECK(o.treats[1][2].hp == 80); // equal-hp higher slot untouched
  CHECK(LogHas(
      "{\"side\":0,\"slot\":1,\"kind\":\"hit\",\"move\":26,\"target\":1,"
      "\"retargeted\":true,\"clash\":\"neu\",\"dmg\":40,\"blocked\":false,"
      "\"targetHp\":40,\"ko\":false}"));
  CHECK(o.phase == duel::wire::kPhaseActive);
}

// Case 5d — Equal NONZERO speed across sides: all four actors pick the same
// move (Coco Chaos, speed 98), so the (speed DESC, side ASC, slot ASC) order
// is decided entirely by the side/slot tie-breaks — side 0 before side 1 at
// equal speed, and slot ASC within a side. Pins the cross-side ordering the
// other vectors (which vary speed or leave one side idle) never exercise.
ApplyVec MakeVec_EqualSpeedSideOrder() {
  ApplyVec v{};
  v.name = "equal_speed_side_then_slot_order";
  duel::DuelState s = MakeState(2, 1);
  const uint8_t mC[] = {3}, cC[] = {0}; // Coco Chaos: speed 98 for all four actors
  SetTreat(s, 0, 0, 250, 250, 1, 1, 1, mC, cC);
  SetTreat(s, 0, 1, 250, 250, 1, 1, 1, mC, cC);
  SetTreat(s, 1, 0, 250, 250, 1, 1, 1, mC, cC);
  SetTreat(s, 1, 1, 250, 250, 1, 1, 1, mC, cC);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 2, 0, 0, 0, 0); // every actor aims at enemy slot 0 (no KO/retarget)
  OrdSet(v.ord, 2, 0, 1, 0, 0);
  OrdSet(v.ord, 2, 1, 0, 0, 0);
  OrdSet(v.ord, 2, 1, 1, 0, 0);
  v.ordLen = duel::wire::OrdersLen(2);
  return v;
}

void test_apply_equal_speed_side_order() {
  ApplyVec v = MakeVec_EqualSpeedSideOrder();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  // All four share speed 98 → order is (speed DESC, side ASC, slot ASC).
  const int i00 = LogIndexOf("{\"side\":0,\"slot\":0,");
  const int i01 = LogIndexOf("{\"side\":0,\"slot\":1,");
  const int i10 = LogIndexOf("{\"side\":1,\"slot\":0,");
  const int i11 = LogIndexOf("{\"side\":1,\"slot\":1,");
  CHECK(i00 >= 0);
  CHECK(i01 >= 0);
  CHECK(i10 >= 0);
  CHECK(i11 >= 0);
  CHECK(i00 < i01); // same-side, equal speed → slot ASC
  CHECK(i01 < i10); // equal speed ACROSS sides → side ASC (all of side 0 first)
  CHECK(i10 < i11);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 202); // hit by both side-0 actors: 250 - 24 - 24
  CHECK(o.treats[0][0].hp == 202); // hit by both side-1 actors
  CHECK(o.phase == duel::wire::kPhaseActive);
}

// Case 6 — Recover: (a) all moves cooling → 0xFE legal, logged "recover";
// (b) ordering a cooling move → reject; (c) KO'd treat encoded non-canonically
// → reject.
ApplyVec MakeVec_RecoverA() {
  ApplyVec v{};
  v.name = "recover_a";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t m0[] = {26}, c0[] = {2}; // its only move is cooling
  const uint8_t m1[] = {24}, c1[] = {0};
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, m0, c0);
  SetTreat(s, 1, 0, 250, 250, 1, 1, 1, m1, c1);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdRecoverAll(v.ord, 1);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

ApplyVec MakeVec_RecoverRejectCoolingMove() {
  ApplyVec v{};
  v.name = "recover_reject_cooling_move";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t m0[] = {26}, c0[] = {2};
  const uint8_t m1[] = {24}, c1[] = {0};
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, m0, c0);
  SetTreat(s, 1, 0, 250, 250, 1, 1, 1, m1, c1);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 1, 0, 0, 0, 0); // the cooling move
  OrdSet(v.ord, 1, 1, 0, duel::wire::kActionRecover, duel::wire::kTargetNone);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

// Case 6c, parameterized: side0/slot0 is KO'd (hp 0), everyone else alive
// and Recovering; the KO'd treat's order carries (action, target). The
// canonical encoding for a KO'd treat is exactly (0xFE, 0xFF) -- anything
// else must reject. Decode-time bit-validation like this is precisely where
// native/wasm codegen divergence would bite, hence golden coverage for both
// non-canonical variants.
ApplyVec MakeVec_KoOrder(const char* name, uint8_t action, uint8_t target) {
  ApplyVec v{};
  v.name = name;
  duel::DuelState s = MakeState(2, 1);
  const uint8_t m[] = {24}, c[] = {0};
  SetTreat(s, 0, 0, 0, 110, 1, 1, 1, m, c);   // KO'd
  SetTreat(s, 0, 1, 200, 200, 1, 1, 1, m, c); // alive
  SetTreat(s, 1, 0, 200, 200, 1, 1, 1, m, c);
  SetTreat(s, 1, 1, 200, 200, 1, 1, 1, m, c);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdRecoverAll(v.ord, 2);
  OrdSet(v.ord, 2, 0, 0, action, target);
  v.ordLen = duel::wire::OrdersLen(2);
  return v;
}

// KO'd treat ordered a real move (action != 0xFE).
ApplyVec MakeVec_KoNoncanonicalAction() {
  return MakeVec_KoOrder("ko_noncanonical_action", 0, duel::wire::kTargetNone);
}
// KO'd treat Recovering but with a real target (target != 0xFF).
ApplyVec MakeVec_KoNoncanonicalTarget() {
  return MakeVec_KoOrder("ko_noncanonical_target", duel::wire::kActionRecover, 0);
}

void test_apply_recover() {
  // (a) all-cooling treat may Recover
  {
    ApplyVec v = MakeVec_RecoverA();
    CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
    CHECK(LogHas("\"kind\":\"recover\""));
    duel::DuelState o{};
    CHECK(DecodeOut(&o));
    CHECK(o.phase == duel::wire::kPhaseActive);
    CHECK(o.round == 1);
    CHECK(o.treats[0][0].cooldowns[0] == 1); // recover ticks the cooldown down
  }
  // (b) ordering a cooling move rejects the whole apply
  {
    ApplyVec v = MakeVec_RecoverRejectCoolingMove();
    CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == -1);
    CHECK(duel_out_len() == 0);
    CHECK(duel_log_len() == 0);
  }
  // (c) KO'd treat must carry the canonical 0xFE/0xFF encoding
  {
    ApplyVec v = MakeVec_KoNoncanonicalAction(); // action != 0xFE
    CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == -1);
    CHECK(duel_out_len() == 0);
  }
  {
    ApplyVec v = MakeVec_KoNoncanonicalTarget(); // target != 0xFF
    CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == -1);
    CHECK(duel_out_len() == 0);
  }
}

// Case 7 — Cooldown cycle: a cd-2 move is unusable the next 2 rounds and usable
// on the 3rd. Drive four applies, carrying the state forward.
// Case 7 chain, parameterized: one builder is the single source of truth for
// every round of the cooldown cycle. Side0's Vicious Jawbreaker (authored
// cd 2) sits at cooldown `cd` in round `round`; side1 (Recover every round)
// is at `hp1` of max 290 (290 fresh, 250 after round A's 40-dmg neutral
// hit). `useMove` picks the order variant: the Jawbreaker (loadout index 0,
// legal iff cd==0) or Recover. The test below cross-checks these hand-built
// states against the REAL chained apply outputs with memcmp, so the builders
// can't drift from what the engine actually produces.
ApplyVec MakeVec_CooldownRound(const char* name, uint16_t round, uint8_t cd,
                               uint16_t hp1, bool useMove) {
  ApplyVec v{};
  v.name = name;
  duel::DuelState s = MakeState(1, 1, round);
  const uint8_t m0[] = {26}, c0[] = {cd}; // Vicious Jawbreaker: authored cd = 2
  const uint8_t m1[] = {24}, c1[] = {0};
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, m0, c0);
  SetTreat(s, 1, 0, hp1, 290, 4, 10, 1, m1, c1); // enough hp to survive
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  if (useMove) {
    OrdSet(v.ord, 1, 0, 0, 0, 0);
  } else {
    OrdSet(v.ord, 1, 0, 0, duel::wire::kActionRecover, duel::wire::kTargetNone);
  }
  OrdSet(v.ord, 1, 1, 0, duel::wire::kActionRecover, duel::wire::kTargetNone);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

// Round A: fresh, use the move (sets cd 2, hits 290 -> 250).
ApplyVec MakeVec_CooldownUse() {
  return MakeVec_CooldownRound("cooldown_use", 0, 0, 290, true);
}
// Round B: cd 2 -- ordering the cooling move rejects...
ApplyVec MakeVec_CooldownRejectCd2() {
  return MakeVec_CooldownRound("cooldown_reject_cd2", 1, 2, 250, true);
}
// ...and Recover ticks it to 1.
ApplyVec MakeVec_CooldownRecoverCd2() {
  return MakeVec_CooldownRound("cooldown_recover_cd2", 1, 2, 250, false);
}
// Round C: cd 1 -- still rejected...
ApplyVec MakeVec_CooldownRejectCd1() {
  return MakeVec_CooldownRound("cooldown_reject_cd1", 2, 1, 250, true);
}
// ...and Recover ticks it to 0.
ApplyVec MakeVec_CooldownRecoverCd1() {
  return MakeVec_CooldownRound("cooldown_recover_cd1", 2, 1, 250, false);
}
// Round D: cd 0 -- the move is usable again.
ApplyVec MakeVec_CooldownReuseReady() {
  return MakeVec_CooldownRound("cooldown_reuse_ready", 3, 0, 250, true);
}

// Asserts the engine's current output buffer byte-equals vector `v`'s input
// state -- the chain <-> hand-built-state cross-check used below.
void CheckOutMatchesVecState(const ApplyVec& v) {
  CHECK(duel_out_len() == v.stLen);
  CHECK(std::memcmp(duel_out_ptr(), v.st, v.stLen) == 0);
}

void test_apply_cooldown_cycle() {
  // Round A: use the move → cooldown set to 2. The output must byte-match
  // the hand-built round-B input state (builder/chain cross-check).
  {
    ApplyVec v = MakeVec_CooldownUse();
    CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
    duel::DuelState o{};
    CHECK(DecodeOut(&o));
    CHECK(o.treats[0][0].cooldowns[0] == 2);
    CheckOutMatchesVecState(MakeVec_CooldownRejectCd2());
  }
  // Round B: cd 2 → move rejected; recover → cd 1.
  {
    ApplyVec rej = MakeVec_CooldownRejectCd2();
    CHECK(duel_apply(rej.st, rej.stLen, rej.ord, rej.ordLen) == -1);
    ApplyVec rec = MakeVec_CooldownRecoverCd2();
    CHECK(duel_apply(rec.st, rec.stLen, rec.ord, rec.ordLen) == 0);
    duel::DuelState o{};
    CHECK(DecodeOut(&o));
    CHECK(o.treats[0][0].cooldowns[0] == 1);
    CheckOutMatchesVecState(MakeVec_CooldownRejectCd1());
  }
  // Round C: cd 1 → still rejected; recover → cd 0.
  {
    ApplyVec rej = MakeVec_CooldownRejectCd1();
    CHECK(duel_apply(rej.st, rej.stLen, rej.ord, rej.ordLen) == -1);
    ApplyVec rec = MakeVec_CooldownRecoverCd1();
    CHECK(duel_apply(rec.st, rec.stLen, rec.ord, rec.ordLen) == 0);
    duel::DuelState o{};
    CHECK(DecodeOut(&o));
    CHECK(o.treats[0][0].cooldowns[0] == 0);
    CheckOutMatchesVecState(MakeVec_CooldownReuseReady());
  }
  // Round D: cd 0 → usable again.
  {
    ApplyVec v = MakeVec_CooldownReuseReady();
    CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  }
}

// Case 8 — Win: KO all 3 enemies → phase 1, and a later apply on the finished
// duel rejects (phase != 0).
ApplyVec MakeVec_Win() {
  ApplyVec v{};
  v.name = "win";
  duel::DuelState s = MakeState(3, 1);
  const uint8_t mS[] = {10}, cS[] = {0}; // Speedy power 51 → 51 dmg vs recover
  const uint8_t mD[] = {24}, cD[] = {0};
  for (int slot = 0; slot < 3; ++slot) SetTreat(s, 0, slot, 250, 250, 4, 10, 1, mS, cS);
  for (int slot = 0; slot < 3; ++slot) SetTreat(s, 1, slot, 40, 110, 1, 1, 1, mD, cD);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 3, 0, 0, 0, 0);
  OrdSet(v.ord, 3, 0, 1, 0, 1);
  OrdSet(v.ord, 3, 0, 2, 0, 2);
  OrdSet(v.ord, 3, 1, 0, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 1, 1, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 1, 2, duel::wire::kActionRecover, duel::wire::kTargetNone);
  v.ordLen = duel::wire::OrdersLen(3);
  return v;
}

// Builds on MakeVec_Win()'s OUTPUT (a finished duel, phase side0-won) as its
// input state -- replays that first apply internally to get there, same
// two-call chain the test itself drives.
ApplyVec MakeVec_WinRejectAfter() {
  ApplyVec win = MakeVec_Win();
  duel_apply(win.st, win.stLen, win.ord, win.ordLen);
  ApplyVec v{};
  v.name = "win_reject_after";
  v.stLen = duel_out_len();
  for (uint32_t i = 0; i < v.stLen; ++i) v.st[i] = duel_out_ptr()[i];
  OrdRecoverAll(v.ord, 3);
  v.ordLen = duel::wire::OrdersLen(3);
  return v;
}

void test_apply_win_then_reject() {
  ApplyVec win = MakeVec_Win();
  CHECK(duel_apply(win.st, win.stLen, win.ord, win.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.phase == duel::wire::kPhaseSide0Won);
  CHECK(o.treats[1][0].hp == 0 && o.treats[1][1].hp == 0 && o.treats[1][2].hp == 0);

  ApplyVec rej = MakeVec_WinRejectAfter();
  CHECK(duel_apply(rej.st, rej.stLen, rej.ord, rej.ordLen) == -1);
  CHECK(duel_out_len() == 0);
  CHECK(duel_log_len() == 0);
}

// Case 9 — Round cap: drive round_cap recover-only rounds; the winner is the
// side with higher Σhp, equal → draw.
//
// Golden-vector note: rather than replaying round_cap-1 recover-only applies
// just to reach the decisive state (as the test below does, to also pin the
// round-by-round increment behavior along the way), these two builders
// hand-construct the state one round short of the cap directly via
// MakeState's `round` parameter -- round isn't range-checked on decode (see
// decode_state in engine.cpp), so this is a legal, purely-byte-built input
// like every other vector, and it reaches the exact same pre-decisive state
// without a 29-call loop in the dumper.
ApplyVec MakeVec_RoundCapUnequalFinal() {
  ApplyVec v{};
  v.name = "round_cap_unequal_final";
  duel::DuelState s = MakeState(1, 1, static_cast<uint16_t>(duel::kTun.round_cap - 1));
  const uint8_t m[] = {24}, c[] = {0};
  SetTreat(s, 0, 0, 200, 200, 1, 1, 1, m, c);
  SetTreat(s, 1, 0, 100, 100, 1, 1, 1, m, c);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdRecoverAll(v.ord, 1);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

ApplyVec MakeVec_RoundCapEqualFinal() {
  ApplyVec v{};
  v.name = "round_cap_equal_final";
  duel::DuelState s = MakeState(1, 1, static_cast<uint16_t>(duel::kTun.round_cap - 1));
  const uint8_t m[] = {24}, c[] = {0};
  SetTreat(s, 0, 0, 150, 150, 1, 1, 1, m, c);
  SetTreat(s, 1, 0, 150, 150, 1, 1, 1, m, c);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdRecoverAll(v.ord, 1);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

void test_apply_round_cap() {
  // (a) unequal totals → higher Σhp wins
  {
    duel::DuelState s = MakeState(1, 1);
    const uint8_t m[] = {24}, c[] = {0};
    SetTreat(s, 0, 0, 200, 200, 1, 1, 1, m, c);
    SetTreat(s, 1, 0, 100, 100, 1, 1, 1, m, c);
    uint8_t st[kMaxStateLen];
    uint32_t stLen = duel::encode_state(s, st, sizeof(st));
    uint8_t ord[32];
    OrdRecoverAll(ord, 1);
    for (int i = 1; i <= duel::kTun.round_cap; ++i) {
      CHECK(duel_apply(st, stLen, ord, duel::wire::OrdersLen(1)) == 0);
      duel::DuelState o{};
      CHECK(DecodeOut(&o));
      CHECK(o.round == static_cast<uint16_t>(i));
      if (i < duel::kTun.round_cap) {
        CHECK(o.phase == duel::wire::kPhaseActive);
      } else {
        CHECK(o.phase == duel::wire::kPhaseSide0Won);
      }
      stLen = duel_out_len();
      for (uint32_t k = 0; k < stLen; ++k) st[k] = duel_out_ptr()[k];
    }
  }
  // (b) equal totals → draw
  {
    duel::DuelState s = MakeState(1, 1);
    const uint8_t m[] = {24}, c[] = {0};
    SetTreat(s, 0, 0, 150, 150, 1, 1, 1, m, c);
    SetTreat(s, 1, 0, 150, 150, 1, 1, 1, m, c);
    uint8_t st[kMaxStateLen];
    uint32_t stLen = duel::encode_state(s, st, sizeof(st));
    uint8_t ord[32];
    OrdRecoverAll(ord, 1);
    for (int i = 1; i <= duel::kTun.round_cap; ++i) {
      CHECK(duel_apply(st, stLen, ord, duel::wire::OrdersLen(1)) == 0);
      stLen = duel_out_len();
      for (uint32_t k = 0; k < stLen; ++k) st[k] = duel_out_ptr()[k];
    }
    duel::DuelState o{};
    CHECK(DecodeOut(&o));
    CHECK(o.phase == duel::wire::kPhaseDraw);
    CHECK(o.round == duel::kTun.round_cap);
  }
}

// Case 10 — Determinism: the same (state, orders) applied twice yields
// byte-identical state + log output.
ApplyVec MakeVec_Deterministic() {
  ApplyVec v{};
  v.name = "deterministic";
  duel::DuelState s = MakeState(2, 1);
  const uint8_t mS[] = {10}, cS[] = {0};
  const uint8_t mH[] = {26}, cH[] = {0};
  const uint8_t mD[] = {24}, cD[] = {0};
  SetTreat(s, 0, 0, 250, 250, 4, 10, 1, mS, cS);
  SetTreat(s, 0, 1, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 0, 1, 110, 1, 1, 1, mD, cD);
  SetTreat(s, 1, 1, 50, 110, 1, 1, 1, mD, cD);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 2, 0, 0, 0, 0);
  OrdSet(v.ord, 2, 0, 1, 0, 0);
  OrdSet(v.ord, 2, 1, 0, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 2, 1, 1, duel::wire::kActionRecover, duel::wire::kTargetNone);
  v.ordLen = duel::wire::OrdersLen(2);
  return v;
}

void test_apply_deterministic() {
  ApplyVec v = MakeVec_Deterministic();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  uint8_t outA[kMaxStateLen];
  const uint32_t outALen = duel_out_len();
  CHECK(outALen <= sizeof(outA));
  for (uint32_t i = 0; i < outALen; ++i) outA[i] = duel_out_ptr()[i];
  uint8_t logA[1024];
  const uint32_t logALen = duel_log_len();
  CHECK(logALen <= sizeof(logA));
  for (uint32_t i = 0; i < logALen; ++i) logA[i] = duel_log_ptr()[i];

  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  CHECK(duel_out_len() == outALen);
  CHECK(std::memcmp(duel_out_ptr(), outA, outALen) == 0);
  CHECK(duel_log_len() == logALen);
  CHECK(std::memcmp(duel_log_ptr(), logA, logALen) == 0);
}

// ---- Task 4 Step 3: self-play soak + fuzz (test-side PRNG only) ----

uint32_t g_rng = 0x1234567u;
uint32_t Xr() {
  uint32_t x = g_rng;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  g_rng = x;
  return x;
}
uint32_t XrN(uint32_t n) { return Xr() % n; } // n > 0

// Build a random VALID config (all wire-contract invariants satisfied) so
// duel_init always accepts it. Returns the byte length.
uint32_t BuildRandomConfig(uint8_t* cfg) {
  const uint8_t teamSize = static_cast<uint8_t>(1 + XrN(5));
  const uint8_t loadoutSize = static_cast<uint8_t>(1 + XrN(4));
  cfg[duel::wire::kCfgOffVersion] = duel::wire::kVersion;
  cfg[duel::wire::kCfgOffTeamSize] = teamSize;
  cfg[duel::wire::kCfgOffLoadoutSize] = loadoutSize;
  cfg[duel::wire::kCfgOffReserved] = 0;
  uint32_t off = duel::wire::kCfgOffTeams;
  for (int side = 0; side < 2; ++side) {
    for (int slot = 0; slot < teamSize; ++slot) {
      const uint8_t mc = static_cast<uint8_t>(1 + XrN(loadoutSize));
      cfg[off++] = static_cast<uint8_t>(1 + XrN(4));  // quality
      cfg[off++] = static_cast<uint8_t>(1 + XrN(10)); // sweetness
      cfg[off++] = mc;
      bool used[duel::wire::kMoveUniverse] = {};
      for (int j = 0; j < loadoutSize; ++j) {
        if (j < mc) {
          uint8_t mv;
          do {
            mv = static_cast<uint8_t>(XrN(duel::wire::kMoveUniverse));
          } while (used[mv]);
          used[mv] = true;
          cfg[off++] = mv;
        } else {
          cfg[off++] = duel::wire::kMoveSlotEmpty;
        }
      }
    }
  }
  return off;
}

// 500 self-play games with random LEGAL orders: legal input never rejects, the
// state length is constant, every game terminates in ≤ round_cap+1 applies with
// a decided phase.
void test_selfplay_soak() {
  g_rng = 0xABCDEF01u;
  for (int game = 0; game < 500; ++game) {
    uint8_t cfg[80];
    const uint32_t cfgLen = BuildRandomConfig(cfg);
    CHECK(duel_init(cfg, cfgLen) == 0);
    // Formula-sized (not a magic 200): v2 grew StateTreatLen, so the old
    // literal 200 no longer covers the max team_size=5/loadout_size=4 shape
    // (kMaxStateLen == 248) that BuildRandomConfig can produce -- ASan
    // caught this as a stack buffer overflow the plain (unsanitized) build
    // didn't.
    uint8_t st[kMaxStateLen];
    uint32_t stLen = duel_out_len();
    const uint32_t constLen = stLen;
    for (uint32_t i = 0; i < stLen; ++i) st[i] = duel_out_ptr()[i];

    duel::DuelState s{};
    int applies = 0;
    bool ok = true;
    for (;;) {
      if (!duel::decode_state(st, stLen, &s)) {
        CHECK(false);
        ok = false;
        break;
      }
      if (s.phase != duel::wire::kPhaseActive) break; // decided
      if (applies > duel::kTun.round_cap) {           // must have decided by now
        CHECK(false);
        ok = false;
        break;
      }
      const uint32_t teamSize = s.team_size;
      uint8_t ord[32];
      OrdInit(ord);
      for (int side = 0; side < 2; ++side) {
        for (uint32_t slot = 0; slot < teamSize; ++slot) {
          const duel::TreatState& t = s.treats[side][slot];
          if (t.hp == 0) {
            OrdSet(ord, teamSize, side, slot, duel::wire::kActionRecover,
                   duel::wire::kTargetNone);
            continue;
          }
          uint8_t ready[4];
          int nready = 0;
          for (uint32_t j = 0; j < s.loadout_size && j < t.move_count; ++j) {
            if (t.cooldowns[j] == 0) ready[nready++] = static_cast<uint8_t>(j);
          }
          if (nready == 0 || (Xr() & 1u)) {
            OrdSet(ord, teamSize, side, slot, duel::wire::kActionRecover,
                   duel::wire::kTargetNone);
          } else {
            const uint8_t a = ready[XrN(static_cast<uint32_t>(nready))];
            const uint8_t tgt = static_cast<uint8_t>(XrN(teamSize));
            OrdSet(ord, teamSize, side, slot, a, tgt);
          }
        }
      }
      const uint32_t ordLen = duel::wire::OrdersLen(teamSize);
      if (duel_apply(st, stLen, ord, ordLen) != 0) {
        CHECK(false); // legal orders must never be rejected
        ok = false;
        break;
      }
      if (duel_out_len() != constLen) {
        CHECK(false); // state length is constant across a game
        ok = false;
        break;
      }
      stLen = duel_out_len();
      for (uint32_t i = 0; i < stLen; ++i) st[i] = duel_out_ptr()[i];
      ++applies;
    }
    CHECK(applies <= duel::kTun.round_cap + 1);
    if (ok) {
      CHECK(s.phase == duel::wire::kPhaseSide0Won ||
            s.phase == duel::wire::kPhaseSide1Won ||
            s.phase == duel::wire::kPhaseDraw);
    }
  }
}

// 10k byte-flip / truncation / random-byte fuzz of a valid state+orders pair:
// duel_apply must only ever return 0 or -1 (never crash / never trip a
// sanitizer — this test is the payload for build.sh's `sanitize` mode).
void test_fuzz_no_crash() {
  g_rng = 0x9E3779B9u;
  duel::DuelState s = MakeState(3, 4);
  const uint8_t mA[] = {10, 26}, cA[] = {0, 0};
  const uint8_t mB[] = {24, 3}, cB[] = {0, 0};
  for (int slot = 0; slot < 3; ++slot) SetTreat(s, 0, slot, 250, 250, 4, 10, 2, mA, cA);
  for (int slot = 0; slot < 3; ++slot) SetTreat(s, 1, slot, 200, 200, 1, 1, 2, mB, cB);
  uint8_t baseSt[kMaxStateLen];
  const uint32_t baseStLen = duel::encode_state(s, baseSt, sizeof(baseSt));
  uint8_t baseOrd[32];
  OrdInit(baseOrd);
  OrdSet(baseOrd, 3, 0, 0, 0, 0);
  OrdSet(baseOrd, 3, 0, 1, 0, 1);
  OrdSet(baseOrd, 3, 0, 2, 1, 2);
  OrdSet(baseOrd, 3, 1, 0, 0, 0);
  OrdSet(baseOrd, 3, 1, 1, 1, 1);
  OrdSet(baseOrd, 3, 1, 2, 0, 2);
  const uint32_t baseOrdLen = duel::wire::OrdersLen(3);

  for (int it = 0; it < 10000; ++it) {
    uint8_t st[256];
    uint8_t ord[64];
    for (uint32_t i = 0; i < sizeof(st); ++i) st[i] = static_cast<uint8_t>(XrN(256));
    for (uint32_t i = 0; i < sizeof(ord); ++i) ord[i] = static_cast<uint8_t>(XrN(256));
    for (uint32_t i = 0; i < baseStLen; ++i) st[i] = baseSt[i];
    for (uint32_t i = 0; i < baseOrdLen; ++i) ord[i] = baseOrd[i];
    uint32_t stLen = baseStLen;
    uint32_t ordLen = baseOrdLen;

    const int muts = 1 + static_cast<int>(XrN(4));
    for (int m = 0; m < muts; ++m) {
      switch (XrN(5)) {
        case 0:
          if (stLen) st[XrN(stLen)] ^= static_cast<uint8_t>(1u << XrN(8));
          break;
        case 1:
          if (ordLen) ord[XrN(ordLen)] ^= static_cast<uint8_t>(1u << XrN(8));
          break;
        case 2:
          stLen = XrN(baseStLen + 8); // truncate/extend (0 .. baseStLen+7)
          break;
        case 3:
          ordLen = XrN(baseOrdLen + 8);
          break;
        default:
          if (stLen) st[XrN(stLen)] = static_cast<uint8_t>(XrN(256));
          break;
      }
    }
    const int32_t rc = duel_apply(st, stLen, ord, ordLen);
    CHECK(rc == 0 || rc == -1);
  }
}

// ---- Task 5: `--dump-golden` ----
//
// Runs every behavioral vector (the Task 3 duel_init shape vectors + all ten
// Task 4 test_apply_* cases, decomposed into one line per distinct engine
// call) through the NATIVE binary and prints one golden JSON line each to
// stdout. `duel/build.sh golden` redirects this into duel/test/golden.json;
// `duel/test/parity.mjs` replays every line through the WASM build via the
// same C ABI and byte-compares. Pure JSON on stdout only -- no other output
// in this mode.
void DumpGolden() {
  {
    InitVec v{};
    v.name = "init_3v3_baseline";
    MakeBaselineCfg(v.cfg);
    v.cfgLen = kBaselineCfgLen;
    DumpInitVector(v);
  }
  {
    InitVec v{};
    v.name = "init_2v2_loadout2";
    Make2v2Loadout2Cfg(v.cfg);
    v.cfgLen = kShape2v2Loadout2CfgLen;
    DumpInitVector(v);
  }
  DumpInitVector(MakeVec_InitRejectBadQuality());
  DumpApplyVector(MakeVec_ClashAdvantage());
  DumpApplyVector(MakeVec_ClashDisadvantage());
  DumpApplyVector(MakeVec_BlockStanceA());
  DumpApplyVector(MakeVec_BlockStanceLate());
  DumpApplyVector(MakeVec_SpeedOrderKoSkip());
  DumpApplyVector(MakeVec_Retarget());
  DumpApplyVector(MakeVec_RetargetMultiLiving());
  DumpApplyVector(MakeVec_RetargetTieLowestSlot());
  DumpApplyVector(MakeVec_EqualSpeedSideOrder());
  DumpApplyVector(MakeVec_RecoverA());
  DumpApplyVector(MakeVec_RecoverRejectCoolingMove());
  DumpApplyVector(MakeVec_KoNoncanonicalAction());
  DumpApplyVector(MakeVec_KoNoncanonicalTarget());
  DumpApplyVector(MakeVec_CooldownUse());
  DumpApplyVector(MakeVec_CooldownRejectCd2());
  DumpApplyVector(MakeVec_CooldownRecoverCd2());
  DumpApplyVector(MakeVec_CooldownRejectCd1());
  DumpApplyVector(MakeVec_CooldownRecoverCd1());
  DumpApplyVector(MakeVec_CooldownReuseReady());
  DumpApplyVector(MakeVec_Win());
  DumpApplyVector(MakeVec_WinRejectAfter());
  DumpApplyVector(MakeVec_RoundCapUnequalFinal());
  DumpApplyVector(MakeVec_RoundCapEqualFinal());
  DumpApplyVector(MakeVec_Deterministic());
}

} // namespace

int main(int argc, char** argv) {
  if (argc > 1 && std::strcmp(argv[1], "--dump-golden") == 0) {
    DumpGolden();
    return 0;
  }

  RUN(test_duel_init_rejects_null_config);
  RUN(test_duel_init_rejects_zeroed_config);
  RUN(test_jsonout_builds_exact_json);
  RUN(test_jsonout_never_writes_past_cap);
  RUN(test_stats_gen_dense_order_matches_blueprint_ends);
  RUN(test_stats_gen_bounds_and_blocking_count);
  RUN(test_stats_gen_tunables_exact);
  RUN(test_duel_init_valid_3v3_produces_initial_state);
  RUN(test_duel_init_valid_2v2_loadout2_shape);
  RUN(test_duel_init_rejects_invalid_configs);
  RUN(test_duel_init_reject_vector);
  RUN(test_state_codec_roundtrip);
  RUN(test_state_codec_decode_rejects_bad_state);
  RUN(test_jsonout_i32_min);
  RUN(test_apply_clash_advantage);
  RUN(test_apply_clash_disadvantage);
  RUN(test_apply_block_stance);
  RUN(test_apply_speed_order_ko_skip);
  RUN(test_apply_retarget);
  RUN(test_apply_retarget_multi_living);
  RUN(test_apply_retarget_tie_lowest_slot);
  RUN(test_apply_equal_speed_side_order);
  RUN(test_apply_recover);
  RUN(test_apply_cooldown_cycle);
  RUN(test_apply_win_then_reject);
  RUN(test_apply_round_cap);
  RUN(test_apply_deterministic);
  RUN(test_selfplay_soak);
  RUN(test_fuzz_no_crash);

  std::printf("---\n%d ran, %d failed\n", g_tests_run, g_tests_failed);
  return g_tests_failed == 0 ? 0 : 1;
}

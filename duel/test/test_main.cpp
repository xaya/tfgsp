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

// The plan's Task 3 vector: six treats spanning q1sw1 (3 moves) .. q4sw10
// (4 moves). Byte layout follows wire.h's CFG_* offsets exactly (header 4
// bytes, then 6 treats of quality|sweetness|move_count|moves[4]).
void MakeBaselineCfg(uint8_t cfg[kBaselineCfgLen]) {
  const uint8_t bytes[kBaselineCfgLen] = {
      // version, team_size, loadout_size, reserved
      1, 3, 4, 0,
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
  CHECK(out[base + duel::wire::kStateTreatOffPad] == 0);
  const uint32_t movesOff = base + duel::wire::kStateTreatOffMoves;
  const uint32_t cdOff = base + duel::wire::StateTreatOffCooldowns(loadoutSize);
  for (uint32_t j = 0; j < loadoutSize; ++j) {
    CHECK(out[movesOff + j] == moves[j]);
    CHECK(out[cdOff + j] == 0); // fresh state: every cooldown starts ready
  }
}

void test_duel_init_valid_3v3_produces_initial_state() {
  uint8_t cfg[kBaselineCfgLen];
  MakeBaselineCfg(cfg);

  CHECK(duel_init(cfg, kBaselineCfgLen) == 0);
  CHECK(duel_out_len() ==
        duel::wire::StateLen(kBaselineTeamSize, kBaselineLoadoutSize));
  CHECK(duel_out_len() == 104);

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

// A non-3v3 shape (2v2, loadout 2) must work too -- the wire formulas, not
// team_size==3, are the actual contract.
void test_duel_init_valid_2v2_loadout2_shape() {
  constexpr uint32_t kTeamSize = 2;
  constexpr uint32_t kLoadoutSize = 2;
  constexpr uint32_t kCfgLen = duel::wire::CfgLen(kTeamSize, kLoadoutSize);
  CHECK(kCfgLen == 24); // 4 + 2*2*(3+2)

  const uint8_t cfg[kCfgLen] = {
      1, 2, 2, 0,             // header
      1, 2, 1, 0, 0xFF,       // side0 slot0: q1 sw2 mc1 moves[0,FF]
      2, 5, 2, 1, 2,          // side0 slot1: q2 sw5 mc2 moves[1,2]
      3, 8, 1, 3, 0xFF,       // side1 slot0: q3 sw8 mc1 moves[3,FF]
      4, 1, 2, 4, 5,          // side1 slot1: q4 sw1 mc2 moves[4,5]
  };

  CHECK(duel_init(cfg, kCfgLen) == 0);
  constexpr uint32_t kExpectedStateLen = duel::wire::StateLen(kTeamSize, kLoadoutSize);
  CHECK(kExpectedStateLen == 56); // 8 + 2*2*(8+2*2)
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
  cfg[duel::wire::kCfgOffVersion] = 2; // bad version
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
  }
  {
    duel::TreatState& t = s.treats[0][1];
    t.hp = 200; t.max_hp = 200; t.quality = 4; t.sweetness = 10; t.move_count = 3;
    t.moves[0] = 10; t.moves[1] = 20; t.moves[2] = 30;
    t.cooldowns[0] = 1; t.cooldowns[1] = 1; t.cooldowns[2] = 3;
  }
  {
    duel::TreatState& t = s.treats[1][0];
    t.hp = 0; t.max_hp = 110; t.quality = 1; t.sweetness = 1; t.move_count = 1;
    t.moves[0] = 0; t.moves[1] = duel::wire::kMoveSlotEmpty; t.moves[2] = duel::wire::kMoveSlotEmpty;
    t.cooldowns[0] = 0; t.cooldowns[1] = 0; t.cooldowns[2] = 0;
  }
  {
    duel::TreatState& t = s.treats[1][1];
    t.hp = 80; t.max_hp = 80; t.quality = 1; t.sweetness = 10; t.move_count = 3;
    t.moves[0] = 38; t.moves[1] = 5; t.moves[2] = 6;
    t.cooldowns[0] = 0; t.cooldowns[1] = 0; t.cooldowns[2] = 0;
  }

  constexpr uint32_t kLen = duel::wire::StateLen(2, 3);
  CHECK(kLen == 64); // 8 + 2*2*(8+2*3)
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
      for (uint32_t j = 0; j < s.loadout_size; ++j) {
        CHECK(a.moves[j] == b.moves[j]);
        CHECK(a.cooldowns[j] == b.cooldowns[j]);
      }
    }
  }

  // cap smaller than the required length: no partial write, 0 returned.
  uint8_t tiny[kLen - 1];
  CHECK(duel::encode_state(s, tiny, sizeof(tiny)) == 0);
}

void test_state_codec_decode_rejects_bad_state() {
  uint8_t cfg[kBaselineCfgLen];
  MakeBaselineCfg(cfg);
  CHECK(duel_init(cfg, kBaselineCfgLen) == 0);
  CHECK(duel_out_len() == 104);
  uint8_t st[104];
  for (uint32_t i = 0; i < 104; ++i) {
    st[i] = duel_out_ptr()[i];
  }

  duel::DuelState decoded{};
  CHECK(duel::decode_state(st, 104, &decoded)); // sanity: the good copy decodes

  CHECK(duel::decode_state(st, 103, &decoded) == false); // too short
  {
    uint8_t big[105]; // genuinely larger buffer, not just a bigger claimed len
    for (uint32_t i = 0; i < 104; ++i) {
      big[i] = st[i];
    }
    big[104] = 0;
    CHECK(duel::decode_state(big, 105, &decoded) == false); // too long
  }

  uint8_t bad[104];
  auto reset = [&]() {
    for (uint32_t i = 0; i < 104; ++i) {
      bad[i] = st[i];
    }
  };

  reset();
  bad[duel::wire::kStateOffVersion] = 2;
  CHECK(duel::decode_state(bad, 104, &decoded) == false); // bad version

  reset();
  bad[duel::wire::kStateOffTeamSize] = 0;
  CHECK(duel::decode_state(bad, 104, &decoded) == false); // team_size 0

  reset();
  bad[duel::wire::kStateOffTeamSize] = 6;
  CHECK(duel::decode_state(bad, 104, &decoded) == false); // team_size 6

  reset();
  bad[duel::wire::kStateOffLoadoutSize] = 0;
  CHECK(duel::decode_state(bad, 104, &decoded) == false); // loadout_size 0

  reset();
  bad[duel::wire::kStateOffPhase] = 4;
  CHECK(duel::decode_state(bad, 104, &decoded) == false); // phase out of range

  // side0/slot0's moves array: baseline echoes moves[0,1,2,0xFF], move_count 3.
  const uint32_t treat0MovesOff =
      duel::wire::kStateOffTeams + duel::wire::kStateTreatOffMoves;
  reset();
  bad[treat0MovesOff + 0] = 39;
  CHECK(duel::decode_state(bad, 104, &decoded) == false); // move index >= 39

  reset();
  bad[treat0MovesOff + 1] = bad[treat0MovesOff + 0];
  CHECK(duel::decode_state(bad, 104, &decoded) == false); // duplicate move

  reset();
  bad[treat0MovesOff + 3] = 5;
  CHECK(duel::decode_state(bad, 104, &decoded) == false); // filler != 0xFF
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
  RUN(test_duel_init_valid_3v3_produces_initial_state);
  RUN(test_duel_init_valid_2v2_loadout2_shape);
  RUN(test_duel_init_rejects_invalid_configs);
  RUN(test_state_codec_roundtrip);
  RUN(test_state_codec_decode_rejects_bad_state);

  std::printf("---\n%d ran, %d failed\n", g_tests_run, g_tests_failed);
  return g_tests_failed == 0 ? 0 : 1;
}

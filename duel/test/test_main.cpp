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
#include "moves_meta_gen.h" // TEST-ONLY: move names/qualities for --balance-report

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

// jsonout (combat-depth Task 2): a dropped write must LATCH JsonBuf::overflow.
// jsonout.h drops past-cap writes by design, and duel_apply turns this flag
// into a hard reject rather than handing the client truncated JSON — so every
// writer must flag, INCLUDING the partial-write paths (jb_raw stopping
// mid-string, jb_u32 stopping mid-number). A writer that drops a byte silently
// defeats the whole guard, so each is pinned separately below.
void test_jsonout_overflow_flag() {
  uint8_t buf[32];

  // A buffer that exactly fits its content does NOT overflow (an off-by-one
  // here would reject every legitimate round).
  {
    duel::JsonBuf b{buf, 5, 0};
    duel::jb_raw(&b, "12345");
    CHECK(b.len == 5);
    CHECK(b.overflow == false);
  }
  // jb_raw (NUL-terminated) stopping mid-string.
  {
    duel::JsonBuf b{buf, 4, 0};
    duel::jb_raw(&b, "hello");
    CHECK(b.len == 4);
    CHECK(b.overflow);
  }
  // jb_raw (explicit-length overload) stopping mid-buffer.
  {
    duel::JsonBuf b{buf, 2, 0};
    duel::jb_raw(&b, "hello", 5);
    CHECK(b.len == 2);
    CHECK(b.overflow);
  }
  // jb_u32 stopping MID-NUMBER: "12" of 12345 is not a truncation, it's a lie.
  {
    duel::JsonBuf b{buf, 2, 0};
    duel::jb_u32(&b, 12345);
    CHECK(b.len == 2);
    CHECK(b.overflow);
  }
  // jb_str: the body fits but the closing quote does not.
  {
    duel::JsonBuf b{buf, 3, 0};
    duel::jb_str(&b, "ok"); // "ok" needs 4 bytes with both quotes
    CHECK(b.len == 3);
    CHECK(b.overflow);
  }
  // jb_i32's negative path (delegates to jb_raw + jb_u32).
  {
    duel::JsonBuf b{buf, 2, 0};
    duel::jb_i32(&b, -12345);
    CHECK(b.overflow);
  }
  // Any write onto an already-full buffer flags, even a 1-byte one.
  {
    duel::JsonBuf b{buf, 1, 0};
    duel::jb_raw(&b, "x");
    CHECK(b.overflow == false);
    duel::jb_raw(&b, "y");
    CHECK(b.overflow);
  }
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

// The AoE caster every combat-depth Task 5 vector uses (its section is far below; the
// stats-table pin immediately after this needs the index too, so it lives up here).
constexpr uint8_t kAoeMove = 34; // Gold Rush (Distance, power 33, speed 69, cd 3, AOE)

// ---- combat-depth Task 3: kDuelMoves effect/hits/max_uses invariants ----
//
// Pins the codegen's contract from the C++ side: every entry's effect is a
// valid kEff*, the stance/action partition holds against type (stance
// effects {block,guard,shield,counter} legal ONLY on type==4 Blocking;
// action effects {damage,aoe,heal,siphon,groupheal} legal ONLY on type!=4 -- a
// total partition, so exactly one of isStance/isAction holds for every move),
// hits is in [1,2] and ONLY EVER > 1 on a plain damage move, and max_uses is in
// [1,255] (never the unusable 0). gen-stats.mjs enforces all of this at codegen
// time; this is the C++-side pin that would fail if a future regeneration ever
// produced a table that violated it.
void test_stats_gen_effect_partition_and_limits() {
  int aoeCount = 0, healCount = 0, groupHealCount = 0, siphonCount = 0;
  int multiHitCount = 0, limitedUseCount = 0;
  for (uint32_t i = 0; i < duel::wire::kMoveUniverse; ++i) {
    const duel::DuelMoveStat& mv = duel::kDuelMoves[i];
    CHECK(mv.effect <= duel::kEffGroupHeal); // a valid kEff* (0..8)
    const bool isStance = mv.effect == duel::kEffBlock || mv.effect == duel::kEffGuard ||
                           mv.effect == duel::kEffShield || mv.effect == duel::kEffCounter;
    const bool isAction = mv.effect == duel::kEffDamage || mv.effect == duel::kEffAoe ||
                           mv.effect == duel::kEffHeal || mv.effect == duel::kEffSiphon ||
                           mv.effect == duel::kEffGroupHeal;
    CHECK(isStance != isAction);       // total partition: exactly one holds
    CHECK(isStance == (mv.type == 4)); // stance <=> Blocking, both directions
    CHECK(mv.hits >= 1 && mv.hits <= 2);
    // combat-depth Task 6: the resolve loop strikes twice down the single-target DAMAGE
    // path and nowhere else, so hits > 1 on any other effect would be an authored lie.
    if (mv.hits > 1) {
      CHECK(mv.effect == duel::kEffDamage);
      ++multiHitCount;
    }
    CHECK(mv.max_uses >= 1); // max_uses <= 255 always holds (uint8_t)
    if (mv.max_uses != 255) {
      ++limitedUseCount;
    }
    if (mv.effect == duel::kEffAoe) ++aoeCount;
    if (mv.effect == duel::kEffHeal) ++healCount;
    if (mv.effect == duel::kEffGroupHeal) ++groupHealCount;
    if (mv.effect == duel::kEffSiphon) ++siphonCount;
  }
  // combat-depth Task 5 authored exactly five AoE moves (Golden Shower of Chips [22],
  // Conductive Coat [31], Gold Rush [34], Explosive Jawbreaker [16] and the deliberate q2
  // "small splash" Cinnamon Blast [36]) -- without them the whole AoE resolve path is
  // unreachable from the compile-time table, so this pins that the DATA landed too.
  CHECK(aoeCount == 5);
  CHECK(duel::kDuelMoves[kAoeMove].effect == duel::kEffAoe); // the vectors' caster
  // ...and combat-depth Task 6 authored exactly one of each of the last verbs. Same
  // reasoning: without the DATA, the resolve paths below are unreachable and untested.
  CHECK(healCount == 1);      // Sugar Rush [27]
  CHECK(groupHealCount == 1); // Super Sugary Rush [10]
  // combat-depth Task 7 moved the siphon UP a tier: it was on Pucker Sucker [6], a q1
  // STARTER, which handed the game's only lifesteal -- a VERB -- to the cheapest treats
  // and denied it to every other quality (a fighter only rolls moves of its own quality).
  // It now rides Toffee Tripper [5], a q3 Tricky; Pucker Sucker is plain damage.
  CHECK(siphonCount == 1);    // Toffee Tripper [5]
  CHECK(multiHitCount == 1);  // Limon Shuriken [15]
  CHECK(limitedUseCount == 2); // Super Sugary Rush [10] + Silver Knuckles [37], both 2
  CHECK(duel::kDuelMoves[27].effect == duel::kEffHeal);
  CHECK(duel::kDuelMoves[10].effect == duel::kEffGroupHeal);
  CHECK(duel::kDuelMoves[10].max_uses == 2);
  CHECK(duel::kDuelMoves[5].effect == duel::kEffSiphon);
  CHECK(duel::kDuelMoves[6].effect == duel::kEffDamage); // ...and the starter is plain again
  CHECK(duel::kDuelMoves[15].hits == 2);
  CHECK(duel::kDuelMoves[37].max_uses == 2);

  // combat-depth Task 7, THE STANCE LADDER. The q4 Blocking corner used to hold TWO plain
  // `block` moves that were strictly worse than the q1 one (same effect, longer cooldown,
  // nothing to show for it) -- the exact "what's the point doing block?" complaint that
  // started this epic. Heavy Gumdrop Kick [14] is now the game's biggest SHIELD (which also
  // gives q4 the one stance it was missing), and Tarnising Knee Drop [13] is the best BLOCK
  // (the heaviest swing of any stance). Pinned here because the whole fairness argument
  // rests on the DATA, and a regeneration that quietly dropped it would pass every
  // behavioural test below.
  CHECK(duel::kDuelMoves[14].effect == duel::kEffShield);
  CHECK(duel::kDuelMoves[13].effect == duel::kEffBlock);
  // The flat-throughput ladders (power x hits / (cooldown+1) is EQUAL across qualities, so
  // the rarer move buys BURST, never damage per round -- see duel-stats.json's _readme and
  // `duel_tests --balance-report`):
  //   shield  14 points/round at every quality: 14/cd0, 28/cd1, 42/cd2, 56/cd3
  CHECK(duel::kDuelMoves[18].power == 14 && duel::kDuelMoves[18].cooldown == 0); // q1
  CHECK(duel::kDuelMoves[12].power == 28 && duel::kDuelMoves[12].cooldown == 1); // q2
  CHECK(duel::kDuelMoves[11].power == 42 && duel::kDuelMoves[11].cooldown == 2); // q3
  CHECK(duel::kDuelMoves[14].power == 56 && duel::kDuelMoves[14].cooldown == 3); // q4
  //   block   12 damage/round at both qualities: 12/cd0 (q1), 24/cd1 (q4)
  CHECK(duel::kDuelMoves[0].power == 12 && duel::kDuelMoves[0].cooldown == 0);
  CHECK(duel::kDuelMoves[13].power == 24 && duel::kDuelMoves[13].cooldown == 1);
  //   counter 11 damage/round at both qualities: 22/cd1 (q3), 33/cd2 (q4)
  CHECK(duel::kDuelMoves[8].power == 22 && duel::kDuelMoves[8].cooldown == 1);
  CHECK(duel::kDuelMoves[21].power == 33 && duel::kDuelMoves[21].cooldown == 2);
  //   guard's power is UNUSED by the engine (the guard branch never reads it): pinned at
  //   the table minimum so it cannot be mistaken for a lever.
  CHECK(duel::kDuelMoves[1].effect == duel::kEffGuard);
  CHECK(duel::kDuelMoves[1].power == 10);
}

// kTun mirrors duel/data/duel-stats.json's top-level "tunables" verbatim
// (plan Task 2 Step 2 exact values; combat-depth Task 3 adds the four
// new-verb percentages, not consumed by any behaviour yet).
void test_stats_gen_tunables_exact() {
  CHECK(duel::kTun.hp_base == 100);
  // Combat-depth Task 7 (the balance pass): 30 -> 10. At 30 an Epic carried up to +90 hp
  // over a Common -- ~2.6x tankier at the same sweetness -- which is a PURE NUMBERS
  // advantage and the exact thing "rarity buys verbs, not numbers" forbids. At 10 the gap
  // is +30 (~15% at a shared sweetness bracket), so a rare treat's real edge is its
  // toolkit. Sweetness (+10/level, the progression axis anyone can climb) still dwarfs it.
  CHECK(duel::kTun.hp_per_quality == 10);
  CHECK(duel::kTun.hp_per_sweetness == 10);
  // Combat-depth Task 7: the clash pentagon swung +50%/-34%, which read as LUCK and
  // drowned out every other decision. Narrowed to +25%/-20%, it is one input among
  // several. dis_mult_256 MUST stay strictly below adv_mult_256 -- BlowDamage's
  // overflow static_assert takes adv as the maximum clash multiplier.
  CHECK(duel::kTun.adv_mult_256 == 320);
  CHECK(duel::kTun.dis_mult_256 == 205);
  CHECK(duel::kTun.dis_mult_256 < duel::kTun.adv_mult_256);
  // Combat-depth Task 4: 102 (40%) -> 154 (60%). A plain `block` buys the
  // BIGGEST flat reduction of the four stances -- that is what makes it worth
  // the turn next to guard/shield/counter, which reduce nothing at all.
  CHECK(duel::kTun.block_pct_256 == 154);
  CHECK(duel::kTun.round_cap == 30);
  CHECK(duel::kTun.team_size == 3);
  CHECK(duel::kTun.loadout_size == 4);
  // Combat-depth Task 2 (controller resolution R2): sudden death. chip is
  // 6, 12, 18, ... from round 12, whose running sum passes the largest
  // reachable max_hp (230 = q4 sw10, since Task 7 cut hp_per_quality to 10) by
  // round 20 -- so every duel ends well inside round_cap, and the cap's Sum-HP
  // tiebreak stops being a win-on-time button once heals exist.
  CHECK(duel::kTun.sudden_start == 12);
  CHECK(duel::kTun.sudden_step == 6);
  // Combat-depth Task 7: aoe_pct_256 60% -> 50%. An AoE's raw power is now a normal blow
  // of its tier (Task 7 deflated it: the AoEs used to carry ~51 power AND the 60% splash,
  // so each victim took MORE than a focused blow -- the reach came free). Half of a normal
  // blow, on every foe, is the honest price. guard_pct_256 ~70%, siphon_pct_256 50%,
  // counter_pct_256 40% all re-checked against the retuned powers and kept.
  CHECK(duel::kTun.aoe_pct_256 == 128);
  CHECK(duel::kTun.guard_pct_256 == 179);
  CHECK(duel::kTun.siphon_pct_256 == 128);
  CHECK(duel::kTun.counter_pct_256 == 102);
}

// ---- Task 3: duel_init (config validation + initial state) ----
//
// hp/max_hp are derived from kTun (stats_gen.h; combat-depth Task 7 cut
// hp_per_quality from 30 to 10 -- quality must not buy a stat line) via the
// formula `max_hp = hp_base + (quality-1)*hp_per_quality +
// sweetness*hp_per_sweetness`, i.e. 100 + (q-1)*10 + sw*10:
//   q1 sw1  -> 100 + 0*10 + 1*10  = 110
//   q2 sw4  -> 100 + 1*10 + 4*10  = 150
//   q3 sw7  -> 100 + 2*10 + 7*10  = 190
//   q3 sw3  -> 100 + 2*10 + 3*10  = 150
//   q4 sw6  -> 100 + 3*10 + 6*10  = 190
//   q4 sw10 -> 100 + 3*10 + 10*10 = 230  (the largest max_hp reachable at all)
// Note SWEETNESS, the axis anyone can climb, now dwarfs quality: +100 vs +30.

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
// Also pins duel_init's v2 seeding contract (controller resolution R3,
// closed by combat-depth Task 3): a fresh treat's shield is 0, its 4
// reserved bytes are 0, and uses_left[j] is kDuelMoves[m].max_uses for a
// real move slot / 0 for a filler slot. Asserted against the TABLE, not
// against a literal 255: combat-depth Task 6 authored the first moves with a
// real max_uses (2), and a fresh duel must seed a signature move with its two
// uses, not with the unlimited sentinel.
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
    // R3: a real slot seeds its move's authored max_uses (255 == unlimited);
    // a filler slot is canonically 0.
    CHECK(out[ulOff + j] ==
          (j < moveCount ? duel::kDuelMoves[moves[j]].max_uses : 0));
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
                   moves1, 150);
  CheckStateTreat(out, kBaselineTeamSize, kBaselineLoadoutSize, 0, 2, 3, 7, 2,
                   moves2, 190);
  CheckStateTreat(out, kBaselineTeamSize, kBaselineLoadoutSize, 1, 0, 3, 3, 1,
                   moves3, 150);
  CheckStateTreat(out, kBaselineTeamSize, kBaselineLoadoutSize, 1, 1, 4, 6, 3,
                   moves4, 190);
  CheckStateTreat(out, kBaselineTeamSize, kBaselineLoadoutSize, 1, 2, 4, 10, 4,
                   moves5, 230);
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
  CheckStateTreat(out, kTeamSize, kLoadoutSize, 0, 1, 2, 5, 2, moves01, 160);
  CheckStateTreat(out, kTeamSize, kLoadoutSize, 1, 0, 3, 8, 1, moves10, 200);
  CheckStateTreat(out, kTeamSize, kLoadoutSize, 1, 1, 4, 1, 2, moves11, 140);
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
// `shield` (combat-depth Task 4) seeds the absorb pool a PRIOR round left
// behind -- the vectors that pin "a shield persists across rounds until
// consumed" start from exactly such a state.
//
// `realUses` (combat-depth Task 6, default null) seeds uses_left for the real
// slots. NULL means "as a fresh duel would": uses_left[j] =
// kDuelMoves[moves[j]].max_uses -- the SAME rule duel_init uses (DecodeConfig),
// so a hand-built vector is always a state duel_init could actually have
// produced. This default is load-bearing, not cosmetic: it used to leave
// uses_left at the struct's zero-init, which encoded real-slot uses_left = 0 in
// EVERY hand-built vector. That was harmless while the byte was inert, but the
// moment Task 6 made `uses_left == 0` a hard reject, all 89 of them would have
// become illegal orders. Pass an explicit array only to pin the spent/limited
// paths (a signature move down to its last use, or already spent).
void SetTreat(duel::DuelState& s, int side, int slot, uint16_t hp, uint16_t maxhp,
              uint8_t q, uint8_t sw, uint8_t mc, const uint8_t* realMoves,
              const uint8_t* realCds, uint8_t shield = 0,
              const uint8_t* realUses = nullptr) {
  duel::TreatState& t = s.treats[side][slot];
  t.hp = hp;
  t.max_hp = maxhp;
  t.quality = q;
  t.sweetness = sw;
  t.move_count = mc;
  t.shield = shield;
  for (int j = 0; j < s.loadout_size; ++j) {
    if (j < mc) {
      t.moves[j] = realMoves[j];
      t.cooldowns[j] = realCds[j];
      t.uses_left[j] = (realUses != nullptr)
                           ? realUses[j]
                           : duel::kDuelMoves[realMoves[j]].max_uses;
    } else {
      t.moves[j] = duel::wire::kMoveSlotEmpty;
      t.cooldowns[j] = 0;
      t.uses_left[j] = 0; // canonical filler
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

// Non-overlapping occurrences of `needle` in the round log (used to pin
// "exactly one chip entry PER LIVING VICTIM, and none for a KO'd one").
int LogCount(const char* needle) {
  const uint8_t* p = duel_log_ptr();
  const uint32_t n = duel_log_len();
  const uint32_t m = static_cast<uint32_t>(std::strlen(needle));
  if (m == 0 || m > n) return 0;
  int count = 0;
  for (uint32_t i = 0; i + m <= n;) {
    if (std::memcmp(p + i, needle, m) == 0) {
      ++count;
      i += m;
    } else {
      ++i;
    }
  }
  return count;
}

// Dense move indices used in the behavioral vectors (from stats_gen.h, AFTER the
// combat-depth Task 7 balance pass -- every power/cooldown below moved, and the
// expected numbers in the vectors moved with them):
//   [0]  Gum Drop Kick       {power 12, speed 46, cd 0, Blocking, block}
//   [1]  Gilded Bonds        {power 10, speed 45, cd 2, Blocking, GUARD -- power UNUSED}
//   [2]  Quicksilver Slice   {power 30, speed 93, cd 2, Speedy}
//   [3]  Coco Chaos          {power 23, speed 98, cd 0, Speedy}
//   [5]  Toffee Tripper      {power 27, speed 54, cd 1, Tricky, SIPHON}  (Task 7 moved it here)
//   [6]  Pucker Sucker       {power 26, speed 57, cd 0, Tricky}          (Task 7: plain damage)
//   [8]  Berry Bounce        {power 22, speed 46, cd 1, Blocking, COUNTER}
//   [10] Super Sugary Rush   {power 48, speed 97, cd 3, Speedy, GROUPHEAL, max_uses 2}
//   [11] Candied Shell       {power 42, speed 47, cd 2, Blocking, SHIELD}
//   [12] Chewy Absorption    {power 28, speed 42, cd 1, Blocking, SHIELD}
//   [13] Tarnising Knee Drop {power 24, speed 49, cd 1, Blocking, block} (Task 7: the best block)
//   [14] Heavy Gumdrop Kick  {power 56, speed 49, cd 3, Blocking, SHIELD} (Task 7: was a 2nd block)
//   [15] Limon Shuriken      {power 18, speed 75, cd 2, Distance, hits 2}
//   [18] Sugar Shield        {power 14, speed 43, cd 0, Blocking, SHIELD}
//   [21] Bouncing Barrage    {power 33, speed 45, cd 2, Blocking, COUNTER}
//   [24] Pop Rock Pop        {power 26, speed 33, cd 0, Heavy}
//   [25] Bubble Trouble      {power 24, speed 76, cd 0, Distance}
//   [26] Vicious Jawbreaker  {power 31, speed 31, cd 2, Heavy}
//   [27] Sugar Rush          {power 42, speed 82, cd 2, Speedy, HEAL}
//   [32] Sugary Sweep        {power 25, speed 64, cd 0, Tricky}
//   [34] Gold Rush           {power 33, speed 69, cd 3, Distance, AOE}   (kAoeMove)
//   [37] Silver Knuckles     {power 40, speed 30, cd 3, Heavy, max_uses 2}
//
// Combat-depth Task 6 gave [10]/[12] their real verbs, and Task 7 moved the SIPHON off the
// q1 starter [6] and onto the q3 [5], so the older vectors that used them as a generic
// Tricky/fast-Speedy/block move were re-pointed -- a vector that means "a plain damage
// move" must not silently become a vector that means "a siphon", and vice versa.

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

// Case 1 — Clash advantage: Heavy(power 31) vs a target that picked Speedy →
// Heavy>Speedy → adv ×320 → dmg = (31*320)>>8 = 38. (Task 7 narrowed the pentagon
// from ×384/×170 to ×320/×205: +25%/-20% instead of +50%/-34%, so a clash is one
// input among several rather than the only one that matters.)
ApplyVec MakeVec_ClashAdvantage() {
  ApplyVec v{};
  v.name = "clash_advantage";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mH[] = {26}, cH[] = {0}; // Heavy power 31
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
  CHECK(o.treats[1][0].hp == 212); // 250 - 38 (adv: (31*320)>>8)
  CHECK(o.treats[0][0].hp == 232); // Speedy vs Heavy = dis 205: (23*205)>>8 = 18
  CHECK(LogHas("\"clash\":\"adv\""));
  CHECK(LogHas("\"dmg\":38"));
  CHECK(o.phase == duel::wire::kPhaseActive);
}

// Case 2 — Disadvantage: Heavy vs a target that picked Tricky → Tricky>Heavy →
// dis ×205 → dmg = (31*205)>>8 = 24.
ApplyVec MakeVec_ClashDisadvantage() {
  ApplyVec v{};
  v.name = "clash_disadvantage";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mH[] = {26}, cH[] = {0}; // Heavy power 31
  const uint8_t mT[] = {32}, cT[] = {0}; // Sugary Sweep (Tricky, plain damage)
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
  CHECK(o.treats[1][0].hp == 226); // 250 - 24 (dis)
  CHECK(LogHas("\"clash\":\"dis\""));
  CHECK(LogHas("\"dmg\":24"));
}

// Case 3 — Block stance. (A) Heavy(power 31) vs a Blocking target → adv ×320
// then block: (31*320*102)>>16 = 15 (block_pct_256 is 154, so the surviving
// fraction is 256-154 = 102, and the two multiplies are FUSED into one >>16
// rather than chained through two >>8 truncations).
// (B) the stance holds even when the blocker's own action fires LAST (attacker
// faster). Both use Gum Drop Kick [0] -- the only effects that still buy the
// flat reduction are `block`, and Sugar Shield [18] / Chewy Absorption [12] are
// both SHIELD moves (Tasks 4 and 6), which buy no reduction at all.
ApplyVec MakeVec_BlockStanceA() {
  ApplyVec v{};
  v.name = "block_stance_a";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mH[] = {26}, cH[] = {0}; // Heavy power 31, speed 31
  const uint8_t mB[] = {0}, cB[] = {0};  // Gum Drop Kick (block, power 12), speed 46 (acts first here)
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
  const uint8_t mB[] = {0}, cB[] = {0};  // Gum Drop Kick (block), speed 46 (fires last)
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
  // (A) exact 15 -- a plain block still eats 60% of the blow: (31*320*102)>>16.
  {
    ApplyVec v = MakeVec_BlockStanceA();
    CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
    duel::DuelState o{};
    CHECK(DecodeOut(&o));
    CHECK(o.treats[1][0].hp == 235); // 250 - 15 (adv then block)
    CHECK(LogHas("\"blocked\":true"));
    CHECK(LogHas("\"dmg\":15"));
    CHECK(LogHas("\"clash\":\"adv\""));
    CHECK(o.treats[1][0].shield == 0); // a `block` raises no absorb pool
  }
  // (B) stance active even though the blocker resolves last
  {
    ApplyVec v = MakeVec_BlockStanceLate();
    CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
    duel::DuelState o{};
    CHECK(DecodeOut(&o));
    // Speedy vs Blocking = dis 205, then block: (23*205*102)>>16 = 7.
    CHECK(o.treats[1][0].hp == 243); // 250 - 7, blocked though blocker acts last
    CHECK(LogHas("\"blocked\":true"));
    const int i0 = LogIndexOf("\"side\":0");
    const int i1 = LogIndexOf("\"side\":1");
    CHECK(i0 >= 0);
    CHECK(i1 >= 0);
    CHECK(i0 < i1); // attacker (speed 98) resolves before the blocker (speed 46)
  }
}

// Case 4 — Speed order + KO skip: a fast Speedy KOs a slower treat before it
// acts → the victim's own action is logged "skip".
ApplyVec MakeVec_SpeedOrderKoSkip() {
  ApplyVec v{};
  v.name = "speed_order_ko_skip";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mS[] = {2}, cS[] = {0};  // Quicksilver Slice (Speedy 30), speed 93
  const uint8_t mD[] = {25}, cD[] = {0}; // Distance, speed 76
  SetTreat(s, 0, 0, 250, 250, 4, 10, 1, mS, cS);
  // 30 hp: the Task 7 retune shrank this blow from 75 to 37 (weaker moves AND a
  // narrower pentagon), so the seed comes down with it -- the vector has to still
  // mean "the fast one KILLS before the slow one swings".
  SetTreat(s, 1, 0, 30, 110, 1, 1, 1, mD, cD);
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
  CHECK(LogHas("\"dmg\":37")); // Speedy>Distance adv: (30*320)>>8 = 37
  CHECK(LogHas("\"ko\":true"));
  CHECK(LogHas("{\"side\":1,\"slot\":0,\"kind\":\"skip\"")); // victim skipped
  CHECK(o.phase == duel::wire::kPhaseSide0Won);
  const int i0 = LogIndexOf("\"side\":0"); // killer acts before victim
  const int i1 = LogIndexOf("\"side\":1");
  CHECK(i0 >= 0);
  CHECK(i1 >= 0);
  CHECK(i0 < i1);
}

// Case 5 — FIZZLE-ON-DEATH (combat-depth Task 2). Two attackers aim at the
// same 1-hp treat; the faster one KOs it, and the second one's blow WHIFFS —
// it is NOT auto-retargeted onto a living enemy (that rule, and the
// `retargeted` log field it set, are gone). The whiffed swing still burns its
// cooldown, so over-committing costs the turn AND the move. This is the rule
// that turns target selection from a lookup into a bet you can lose.
ApplyVec MakeVec_FizzleOnDeath() {
  ApplyVec v{};
  v.name = "fizzle_on_death";
  duel::DuelState s = MakeState(2, 1);
  const uint8_t mS[] = {2}, cS[] = {0};  // Quicksilver Slice (Speedy 50), speed 93 → KOs slot 0
  const uint8_t mH[] = {26}, cH[] = {0}; // Heavy power 40, speed 28, authored cd 2 → whiffs
  const uint8_t mD[] = {24}, cD[] = {0};
  SetTreat(s, 0, 0, 250, 250, 4, 10, 1, mS, cS);
  SetTreat(s, 0, 1, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 0, 1, 110, 1, 1, 1, mD, cD);    // 1 hp → dies to the Speedy
  SetTreat(s, 1, 1, 110, 110, 1, 1, 1, mD, cD);  // FULL hp → must stay untouched
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 2, 0, 0, 0, 0); // both attackers aim at enemy slot 0
  OrdSet(v.ord, 2, 0, 1, 0, 0);
  OrdSet(v.ord, 2, 1, 0, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 2, 1, 1, duel::wire::kActionRecover, duel::wire::kTargetNone);
  v.ordLen = duel::wire::OrdersLen(2);
  return v;
}

void test_apply_fizzle_on_death() {
  ApplyVec v = MakeVec_FizzleOnDeath();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 0);   // KO'd by the fast Speedy
  CHECK(o.treats[1][1].hp == 110); // the OTHER enemy is at FULL hp: no auto-retarget
  // The whiffed swing is logged as a `skip` with dmg 0 and no target.
  CHECK(LogHas(
      "{\"side\":0,\"slot\":1,\"kind\":\"skip\",\"move\":26,\"target\":255,"
      "\"via\":255,\"clash\":\"neu\",\"dmg\":0,\"blocked\":false,"
      "\"absorbed\":0,\"targetHp\":0,\"ko\":false}"));
  CHECK(!LogHas("\"retargeted\"")); // the dead field is gone from the format
  // ...and it still BURNS the cooldown (Vicious Jawbreaker's authored cd is 2):
  // over-committing costs you the turn *and* the move.
  CHECK(o.treats[0][1].cooldowns[0] == 2);
  CHECK(o.phase == duel::wire::kPhaseActive);
}

// Case 5b — Fizzle with MULTIPLE living enemies available: the old rule picked
// the lowest-hp living enemy (slot 2, 60 hp) for the whiffed Heavy. Now NOBODY
// is picked — both living enemies end the round untouched.
ApplyVec MakeVec_FizzleMultiLivingUntouched() {
  ApplyVec v{};
  v.name = "fizzle_multi_living_untouched";
  duel::DuelState s = MakeState(3, 1);
  const uint8_t mS[] = {2}, cS[] = {0};  // Quicksilver Slice (Speedy 50), speed 93 — KOs slot 0
  const uint8_t mH[] = {26}, cH[] = {0}; // Heavy  power 40, speed 28 — whiffs
  const uint8_t mD[] = {24}, cD[] = {0}; // filler move for the recovering treats
  SetTreat(s, 0, 0, 250, 250, 4, 10, 1, mS, cS);
  SetTreat(s, 0, 1, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 0, 2, 250, 250, 1, 1, 1, mD, cD);  // recovers
  SetTreat(s, 1, 0, 1, 110, 1, 1, 1, mD, cD);    // 1 hp → dies to attacker 0
  SetTreat(s, 1, 1, 100, 110, 1, 1, 1, mD, cD);  // living → untouched
  SetTreat(s, 1, 2, 60, 110, 1, 1, 1, mD, cD);   // the old retarget victim → untouched
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 3, 0, 0, 0, 0); // Speedy → enemy slot 0
  OrdSet(v.ord, 3, 0, 1, 0, 0); // Heavy  → enemy slot 0 (dead by then → whiffs)
  OrdSet(v.ord, 3, 0, 2, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 1, 0, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 1, 1, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 1, 2, duel::wire::kActionRecover, duel::wire::kTargetNone);
  v.ordLen = duel::wire::OrdersLen(3);
  return v;
}

void test_apply_fizzle_multi_living_untouched() {
  ApplyVec v = MakeVec_FizzleMultiLivingUntouched();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 0);   // slot 0 KO'd by the fast Speedy
  CHECK(o.treats[1][1].hp == 100); // untouched
  CHECK(o.treats[1][2].hp == 60);  // untouched (the old rule hit THIS one for 40)
  CHECK(LogHas("{\"side\":0,\"slot\":1,\"kind\":\"skip\",\"move\":26,\"target\":255,"
               "\"via\":255,\"clash\":\"neu\",\"dmg\":0"));
  CHECK(o.phase == duel::wire::kPhaseActive);
}

// Case 5c/5d — FAIR TURN ORDER (combat-depth Task 2). All four actors pick the
// same move (Coco Chaos, speed 98), so the whole order is decided by the
// tie-breaks: speed DESC → THIS ROUND'S FIRST SIDE → slot ASC. The first side
// alternates with the round parity (`round & 1`), so side 0 no longer wins
// every speed tie for the whole duel (a systematic, permanent advantage).
// Two vectors, identical but for the round number, pin BOTH parities — a test
// that only checked round 0 would pass against the old side-ASC code.
ApplyVec MakeVec_EqualSpeedRound(const char* name, uint16_t round) {
  ApplyVec v{};
  v.name = name;
  duel::DuelState s = MakeState(2, 1, round);
  const uint8_t mC[] = {3}, cC[] = {0}; // Coco Chaos: speed 98 for all four actors
  SetTreat(s, 0, 0, 250, 250, 1, 1, 1, mC, cC);
  SetTreat(s, 0, 1, 250, 250, 1, 1, 1, mC, cC);
  SetTreat(s, 1, 0, 250, 250, 1, 1, 1, mC, cC);
  SetTreat(s, 1, 1, 250, 250, 1, 1, 1, mC, cC);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 2, 0, 0, 0, 0); // every actor aims at enemy slot 0 (nobody dies)
  OrdSet(v.ord, 2, 0, 1, 0, 0);
  OrdSet(v.ord, 2, 1, 0, 0, 0);
  OrdSet(v.ord, 2, 1, 1, 0, 0);
  v.ordLen = duel::wire::OrdersLen(2);
  return v;
}

// Round 0 (EVEN) → side 0 acts first.
ApplyVec MakeVec_EqualSpeedEvenRound() {
  return MakeVec_EqualSpeedRound("equal_speed_even_round_side0_first", 0);
}
// Round 1 (ODD) → side 1 acts first.
ApplyVec MakeVec_EqualSpeedOddRound() {
  return MakeVec_EqualSpeedRound("equal_speed_odd_round_side1_first", 1);
}

void test_apply_turn_order_alternates_by_round() {
  // Even round: side 0 first (both of its slots), then side 1; slot ASC within.
  {
    ApplyVec v = MakeVec_EqualSpeedEvenRound();
    CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
    const int i00 = LogIndexOf("{\"side\":0,\"slot\":0,");
    const int i01 = LogIndexOf("{\"side\":0,\"slot\":1,");
    const int i10 = LogIndexOf("{\"side\":1,\"slot\":0,");
    const int i11 = LogIndexOf("{\"side\":1,\"slot\":1,");
    CHECK(i00 >= 0 && i01 >= 0 && i10 >= 0 && i11 >= 0);
    CHECK(i00 < i01); // same side, equal speed → slot ASC
    CHECK(i01 < i10); // equal speed ACROSS sides → the round's first side (0) goes first
    CHECK(i10 < i11);
    duel::DuelState o{};
    CHECK(DecodeOut(&o));
    CHECK(o.treats[1][0].hp == 204); // hit by both side-0 actors: 250 - 23 - 23
    CHECK(o.treats[0][0].hp == 204); // hit by both side-1 actors
    CHECK(o.phase == duel::wire::kPhaseActive);
  }
  // Odd round: the SAME state+orders, one round later → side 1 goes first.
  {
    ApplyVec v = MakeVec_EqualSpeedOddRound();
    CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
    const int i00 = LogIndexOf("{\"side\":0,\"slot\":0,");
    const int i01 = LogIndexOf("{\"side\":0,\"slot\":1,");
    const int i10 = LogIndexOf("{\"side\":1,\"slot\":0,");
    const int i11 = LogIndexOf("{\"side\":1,\"slot\":1,");
    CHECK(i00 >= 0 && i01 >= 0 && i10 >= 0 && i11 >= 0);
    CHECK(i10 < i11); // slot ASC still holds within the first side
    CHECK(i11 < i00); // ...but side 1 now goes FIRST
    CHECK(i00 < i01);
    duel::DuelState o{};
    CHECK(DecodeOut(&o));
    CHECK(o.treats[1][0].hp == 204); // damage is unchanged: only the ORDER flips
    CHECK(o.treats[0][0].hp == 204);
    CHECK(o.phase == duel::wire::kPhaseActive);
  }
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
// is at `hp1` of max 230 (230 fresh, 199 after round A's 31-dmg neutral
// hit -- Task 7 retuned the Jawbreaker to power 31, and 230 is now the largest
// max_hp any treat can have). `useMove` picks the order variant: the Jawbreaker (loadout index 0,
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
  SetTreat(s, 1, 0, hp1, 230, 4, 10, 1, m1, c1); // enough hp to survive (q4 sw10 = 230)
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

// Round A: fresh, use the move (sets cd 2, hits 230 -> 199).
ApplyVec MakeVec_CooldownUse() {
  return MakeVec_CooldownRound("cooldown_use", 0, 0, 230, true);
}
// Round B: cd 2 -- ordering the cooling move rejects...
ApplyVec MakeVec_CooldownRejectCd2() {
  return MakeVec_CooldownRound("cooldown_reject_cd2", 1, 2, 199, true);
}
// ...and Recover ticks it to 1.
ApplyVec MakeVec_CooldownRecoverCd2() {
  return MakeVec_CooldownRound("cooldown_recover_cd2", 1, 2, 199, false);
}
// Round C: cd 1 -- still rejected...
ApplyVec MakeVec_CooldownRejectCd1() {
  return MakeVec_CooldownRound("cooldown_reject_cd1", 2, 1, 199, true);
}
// ...and Recover ticks it to 0.
ApplyVec MakeVec_CooldownRecoverCd1() {
  return MakeVec_CooldownRound("cooldown_recover_cd1", 2, 1, 199, false);
}
// Round D: cd 0 -- the move is usable again.
ApplyVec MakeVec_CooldownReuseReady() {
  return MakeVec_CooldownRound("cooldown_reuse_ready", 3, 0, 199, true);
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
  const uint8_t mS[] = {2}, cS[] = {0};  // Quicksilver Slice (Speedy 30) → 30 dmg vs recover
  const uint8_t mD[] = {24}, cD[] = {0};
  for (int slot = 0; slot < 3; ++slot) SetTreat(s, 0, slot, 250, 250, 4, 10, 1, mS, cS);
  // 25 hp each: the Task 7 retune took this blow from 50 to 30 (a Recover target has no
  // move type, so there is no clash either way) -- the seeds come down so the vector
  // still means "all three enemies are wiped in one round".
  for (int slot = 0; slot < 3; ++slot) SetTreat(s, 1, slot, 25, 110, 1, 1, 1, mD, cD);
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

// ---- Case 9 — SUDDEN DEATH (combat-depth Task 2) ----
//
// From `sudden_start` on, the arena collapses: at end of round (after the
// cooldown tick, before the win check) every LIVING treat takes
// `sudden_step * (playedRound - sudden_start + 1)` chip damage. It ignores
// clash/block/shield/guard entirely (it is not an attack), it can KO, and the
// win check sees the post-chip HP. Every vector below drives recover-only
// orders, so the chip is the ONLY damage in the round.
ApplyVec MakeVec_SuddenDeathRound(const char* name, uint16_t round, uint16_t hp0,
                                  uint16_t hp1) {
  ApplyVec v{};
  v.name = name;
  duel::DuelState s = MakeState(1, 1, round);
  const uint8_t m[] = {24}, c[] = {0};
  SetTreat(s, 0, 0, hp0, 250, 1, 1, 1, m, c);
  SetTreat(s, 1, 0, hp1, 250, 1, 1, 1, m, c);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdRecoverAll(v.ord, 1);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

// Round 11 — the last round BEFORE sudden death: no chip at all.
ApplyVec MakeVec_SuddenDeathNotYet() {
  return MakeVec_SuddenDeathRound("sudden_death_not_yet_round11", 11, 250, 250);
}
// Round 12 (== sudden_start) — the first chip: 6 * (12 - 12 + 1) = 6.
ApplyVec MakeVec_SuddenDeathFirstChip() {
  return MakeVec_SuddenDeathRound("sudden_death_first_chip_round12", 12, 250, 250);
}
// Round 15 — the chip has escalated: 6 * (15 - 12 + 1) = 24.
ApplyVec MakeVec_SuddenDeathEscalated() {
  return MakeVec_SuddenDeathRound("sudden_death_escalated_round15", 15, 250, 250);
}

void test_apply_sudden_death_escalates() {
  // (a) one round short of sudden_start: nothing is chipped.
  {
    ApplyVec v = MakeVec_SuddenDeathNotYet();
    CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
    duel::DuelState o{};
    CHECK(DecodeOut(&o));
    CHECK(o.treats[0][0].hp == 250);
    CHECK(o.treats[1][0].hp == 250);
    CHECK(LogCount("\"kind\":\"chip\"") == 0);
  }
  // (b) round 12 == sudden_start: chip is exactly sudden_step * 1 == 6, one
  // entry per LIVING treat, and the entry carries the victim's own side/slot
  // (there is no actor) with move/target 255 and clash "neu".
  {
    ApplyVec v = MakeVec_SuddenDeathFirstChip();
    CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
    duel::DuelState o{};
    CHECK(DecodeOut(&o));
    CHECK(o.treats[0][0].hp == 244); // 250 - 6
    CHECK(o.treats[1][0].hp == 244);
    CHECK(LogCount("\"kind\":\"chip\"") == 2);
    CHECK(LogHas(
        "{\"side\":0,\"slot\":0,\"kind\":\"chip\",\"move\":255,\"target\":255,"
        "\"via\":255,\"clash\":\"neu\",\"dmg\":6,\"blocked\":false,"
        "\"absorbed\":0,\"targetHp\":244,\"ko\":false}"));
    CHECK(LogHas(
        "{\"side\":1,\"slot\":0,\"kind\":\"chip\",\"move\":255,\"target\":255,"
        "\"via\":255,\"clash\":\"neu\",\"dmg\":6,\"blocked\":false,"
        "\"absorbed\":0,\"targetHp\":244,\"ko\":false}"));
    CHECK(o.phase == duel::wire::kPhaseActive);
  }
  // (c) round 15: chip = 6 * (15 - 12 + 1) = 24 — it escalates every round.
  {
    ApplyVec v = MakeVec_SuddenDeathEscalated();
    CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
    duel::DuelState o{};
    CHECK(DecodeOut(&o));
    CHECK(o.treats[0][0].hp == 226); // 250 - 24
    CHECK(o.treats[1][0].hp == 226);
    CHECK(LogHas("\"kind\":\"chip\",\"move\":255,\"target\":255,\"via\":255,"
                 "\"clash\":\"neu\",\"dmg\":24"));
  }
}

// Round 15 (chip 24) with a KO'd treat and a treat the chip is about to fell:
// the KO'd one takes NO chip and gets NO chip entry; the 20-hp one dies TO the
// chip (ko:true, targetHp 0). Side 1 survives on its other slot, so the duel
// stays active — the win check ran on the POST-chip HP either way.
ApplyVec MakeVec_SuddenDeathKo() {
  ApplyVec v{};
  v.name = "sudden_death_ko_round15";
  duel::DuelState s = MakeState(2, 1, 15);
  const uint8_t m[] = {24}, c[] = {0};
  SetTreat(s, 0, 0, 100, 250, 1, 1, 1, m, c); // 100 - 24 = 76
  SetTreat(s, 0, 1, 0, 250, 1, 1, 1, m, c);   // already KO'd → no chip, no entry
  SetTreat(s, 1, 0, 20, 250, 1, 1, 1, m, c);  // 20 - 24 → 0, KO'd BY the chip
  SetTreat(s, 1, 1, 200, 250, 1, 1, 1, m, c); // 200 - 24 = 176
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdRecoverAll(v.ord, 2);
  v.ordLen = duel::wire::OrdersLen(2);
  return v;
}

void test_apply_sudden_death_ko() {
  ApplyVec v = MakeVec_SuddenDeathKo();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[0][0].hp == 76);
  CHECK(o.treats[0][1].hp == 0);  // stays 0: a KO'd treat takes no chip
  CHECK(o.treats[1][0].hp == 0);  // felled BY the chip
  CHECK(o.treats[1][1].hp == 176);
  CHECK(LogCount("\"kind\":\"chip\"") == 3); // 3 living victims, not 4
  CHECK(!LogHas("{\"side\":0,\"slot\":1,\"kind\":\"chip\"")); // ...and not the KO'd one
  CHECK(LogHas("{\"side\":1,\"slot\":0,\"kind\":\"chip\",\"move\":255,\"target\":255,"
               "\"via\":255,\"clash\":\"neu\",\"dmg\":24,\"blocked\":false,"
               "\"absorbed\":0,\"targetHp\":0,\"ko\":true}"));
  CHECK(o.phase == duel::wire::kPhaseActive); // side 1 still has slot 1
}

// Sudden death must TERMINATE every duel well inside round_cap, even the
// worst case: two max-hp treats (q4 sw10 = 290) that never attack. Chip sums
// to 270 through round 20 and 330 through round 21, so both fall on round 21.
//
// Both sides being wiped in the SAME round is only reachable because of the
// chip (a treat's own action can never kill an already-dead attacker), and it
// must be a DRAW — resolving it as "side 0 wins" would hand side 0 every
// symmetric mirror match, exactly the bias the round-alternating turn order
// exists to remove.
void test_apply_sudden_death_terminates_and_mutual_ko_is_a_draw() {
  duel::DuelState s = MakeState(1, 1);
  const uint8_t m[] = {24}, c[] = {0};
  SetTreat(s, 0, 0, 290, 290, 4, 10, 1, m, c);
  SetTreat(s, 1, 0, 290, 290, 4, 10, 1, m, c);
  uint8_t st[kMaxStateLen];
  uint32_t stLen = duel::encode_state(s, st, sizeof(st));
  uint8_t ord[32];
  OrdRecoverAll(ord, 1);

  int rounds = 0;
  duel::DuelState o{};
  for (;;) {
    CHECK(duel::decode_state(st, stLen, &o));
    if (o.phase != duel::wire::kPhaseActive) break;
    CHECK(rounds < duel::kTun.round_cap); // must never reach the cap
    CHECK(duel_apply(st, stLen, ord, duel::wire::OrdersLen(1)) == 0);
    stLen = duel_out_len();
    for (uint32_t k = 0; k < stLen; ++k) st[k] = duel_out_ptr()[k];
    ++rounds;
  }
  CHECK(rounds == 22);  // played rounds 0..21; round 21 is the fatal chip (60)
  CHECK(o.round == 22); // and it ended a full 8 rounds inside round_cap 30
  CHECK(o.treats[0][0].hp == 0);
  CHECK(o.treats[1][0].hp == 0);
  CHECK(o.phase == duel::wire::kPhaseDraw); // mutual wipe → draw, NOT side 0
}

// Case 10 — Round cap: the Σhp tiebreak still decides a duel that somehow
// survives to the cap; equal Σhp → draw.
//
// Reaching the cap at all now takes a hand-built state: sudden death ends
// every duel that starts from duel_init by ~round 21 (see the test above).
// Both builders therefore construct the state one round short of the cap
// (MakeState's `round` parameter — legal, and decode_state accepts any active
// round < round_cap) with enough HP for both treats to SURVIVE round 29's
// 108-point chip (6 * (29 - 12 + 1)), so the cap — not a chip KO — is what
// decides them.
ApplyVec MakeVec_RoundCapUnequalFinal() {
  ApplyVec v{};
  v.name = "round_cap_unequal_final";
  duel::DuelState s = MakeState(1, 1, static_cast<uint16_t>(duel::kTun.round_cap - 1));
  const uint8_t m[] = {24}, c[] = {0};
  SetTreat(s, 0, 0, 200, 200, 1, 1, 1, m, c); // 200 - 108 = 92
  SetTreat(s, 1, 0, 150, 150, 1, 1, 1, m, c); // 150 - 108 = 42
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
  // (a) unequal totals → higher Σhp wins (both survived the round-29 chip)
  {
    ApplyVec v = MakeVec_RoundCapUnequalFinal();
    CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
    duel::DuelState o{};
    CHECK(DecodeOut(&o));
    CHECK(o.round == duel::kTun.round_cap);
    CHECK(o.treats[0][0].hp == 92);
    CHECK(o.treats[1][0].hp == 42);
    CHECK(o.phase == duel::wire::kPhaseSide0Won);
  }
  // (b) equal totals → draw
  {
    ApplyVec v = MakeVec_RoundCapEqualFinal();
    CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
    duel::DuelState o{};
    CHECK(DecodeOut(&o));
    CHECK(o.round == duel::kTun.round_cap);
    CHECK(o.treats[0][0].hp == 42);
    CHECK(o.treats[1][0].hp == 42);
    CHECK(o.phase == duel::wire::kPhaseDraw);
  }
}

// R6 — an ACTIVE state at/past the round cap is unreachable from duel_init +
// duel_apply (apply always sets a terminal phase once round >= round_cap), and
// with sudden death it is actively dangerous (a crafted round = 60000 makes the
// chip term enormous). decode_state must reject it outright — a terminal-phase
// state AT the cap is the legitimate end state and must still decode.
void test_decode_rejects_active_state_at_round_cap() {
  const uint8_t m[] = {24}, c[] = {0};
  uint8_t buf[kMaxStateLen];
  duel::DuelState o{};

  // active, round == round_cap - 1 → the last legal active round: accepted.
  {
    duel::DuelState s = MakeState(1, 1, static_cast<uint16_t>(duel::kTun.round_cap - 1));
    SetTreat(s, 0, 0, 100, 100, 1, 1, 1, m, c);
    SetTreat(s, 1, 0, 100, 100, 1, 1, 1, m, c);
    const uint32_t len = duel::encode_state(s, buf, sizeof(buf));
    CHECK(duel::decode_state(buf, len, &o));
  }
  // active, round == round_cap → rejected (and duel_apply rejects it too).
  {
    duel::DuelState s = MakeState(1, 1, duel::kTun.round_cap);
    SetTreat(s, 0, 0, 100, 100, 1, 1, 1, m, c);
    SetTreat(s, 1, 0, 100, 100, 1, 1, 1, m, c);
    const uint32_t len = duel::encode_state(s, buf, sizeof(buf));
    CHECK(duel::decode_state(buf, len, &o) == false);
    uint8_t ord[32];
    OrdRecoverAll(ord, 1);
    CHECK(duel_apply(buf, len, ord, duel::wire::OrdersLen(1)) == -1);
    CHECK(duel_out_len() == 0);
    CHECK(duel_log_len() == 0);
  }
  // active, absurd round (the crafted-chip hazard) → rejected.
  {
    duel::DuelState s = MakeState(1, 1, 60000);
    SetTreat(s, 0, 0, 100, 100, 1, 1, 1, m, c);
    SetTreat(s, 1, 0, 100, 100, 1, 1, 1, m, c);
    const uint32_t len = duel::encode_state(s, buf, sizeof(buf));
    CHECK(duel::decode_state(buf, len, &o) == false);
  }
  // ...but a FINISHED duel at the cap is the legitimate terminal state: accepted.
  {
    duel::DuelState s = MakeState(1, 1, duel::kTun.round_cap);
    s.phase = duel::wire::kPhaseDraw;
    SetTreat(s, 0, 0, 100, 100, 1, 1, 1, m, c);
    SetTreat(s, 1, 0, 100, 100, 1, 1, 1, m, c);
    const uint32_t len = duel::encode_state(s, buf, sizeof(buf));
    CHECK(duel::decode_state(buf, len, &o));
    CHECK(o.phase == duel::wire::kPhaseDraw);
  }
}

// Case 10 — Determinism: the same (state, orders) applied twice yields
// byte-identical state + log output.
ApplyVec MakeVec_Deterministic() {
  ApplyVec v{};
  v.name = "deterministic";
  duel::DuelState s = MakeState(2, 1);
  const uint8_t mS[] = {2}, cS[] = {0};
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

// ================= combat-depth Task 4: Guard, Shield, Counter =================
//
// The Blocking corner stops being a single verb with four power levels. The
// stance an actor holds is now decided by its move's EFFECT, and each stance
// buys exactly ONE thing:
//   block   -> the flat damage reduction (the biggest one: 60%)
//   guard   -> intercept a single-target blow aimed at an ALLY, at guard_pct
//   shield  -> a persisting absorb pool (u8), consumed before HP
//   counter -> reflect counter_pct of the blow BACK at whoever landed it
// guard/shield/counter deliberately do NOT also reduce incoming damage -- the
// tests below pin that ("the stance split"), because it is the whole reason
// plain block is still a real choice.
//
// Fixed interaction order, pinned throughout: 1. guard redirect -> 2. shield
// absorb (on whoever FINALLY receives it) -> 3. HP; then the counter fires off
// `landed` (the force that reached the final recipient, BEFORE its shield ate
// any of it).

// --- Guard ---

// A blow aimed at the ward lands on the GUARDIAN instead, at guard_pct (179):
// (26 * 256 * 179) >> 16 = 18. The ward is untouched, and the hit entry's `via`
// names the guardian.
ApplyVec MakeVec_GuardIntercept() {
  ApplyVec v{};
  v.name = "guard_intercept";
  duel::DuelState s = MakeState(2, 1);
  const uint8_t mG[] = {1}, cG[] = {0};  // Gilded Bonds (guard), speed 45
  const uint8_t mH[] = {24}, cH[] = {0}; // Pop Rock Pop (Heavy 26), speed 27
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 0, 1, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mG, cG); // the GUARDIAN
  SetTreat(s, 1, 1, 250, 250, 3, 6, 1, mH, cH); // the WARD
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 2, 0, 0, 0, 1); // attacker -> enemy slot 1 (the ward)
  OrdSet(v.ord, 2, 0, 1, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 2, 1, 0, 0, 1); // guard -> ALLY slot 1
  OrdSet(v.ord, 2, 1, 1, 0, 0); // the ward swings back at enemy slot 0
  v.ordLen = duel::wire::OrdersLen(2);
  return v;
}

void test_apply_guard_intercepts() {
  ApplyVec v = MakeVec_GuardIntercept();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 232); // the GUARDIAN ate it: 250 - 18
  CHECK(o.treats[1][1].hp == 250); // the ward is untouched
  // A guard deals NO damage of its own -- side0/slot0 is only down by the
  // ward's own 26-power swing (Heavy vs Heavy = neutral), not a point more.
  CHECK(o.treats[0][0].hp == 224);
  CHECK(LogHas("{\"side\":1,\"slot\":0,\"kind\":\"guard\",\"move\":1,\"target\":1,"
               "\"via\":255,\"clash\":\"neu\",\"dmg\":0,\"blocked\":false,"
               "\"absorbed\":0,\"targetHp\":0,\"ko\":false}"));
  // The blow: aimed at slot 1, RECEIVED by slot 0 (via), and not "blocked" --
  // guard is not block, and buys no flat reduction of its own.
  CHECK(LogHas("{\"side\":0,\"slot\":0,\"kind\":\"hit\",\"move\":24,\"target\":1,"
               "\"via\":0,\"clash\":\"neu\",\"dmg\":18,\"blocked\":false,"
               "\"absorbed\":0,\"targetHp\":232,\"ko\":false}"));
}

// The three guard-target rejects. Each is an otherwise-legal 2v2 whose ONLY
// defect is the guard's target byte -- so "-1" can only be that rule.
ApplyVec MakeVec_GuardRejectTarget(const char* name, uint8_t guardTarget,
                                   uint16_t allyHp) {
  ApplyVec v{};
  v.name = name;
  duel::DuelState s = MakeState(2, 1);
  const uint8_t mG[] = {1}, cG[] = {0};
  const uint8_t mH[] = {24}, cH[] = {0};
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 0, 1, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mG, cG);
  SetTreat(s, 1, 1, allyHp, 250, 3, 6, 1, mH, cH);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 2, 0, 0, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 2, 0, 1, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 2, 1, 0, 0, guardTarget);
  // A KO'd ally can only carry the canonical Recover encoding.
  if (allyHp == 0) {
    OrdSet(v.ord, 2, 1, 1, duel::wire::kActionRecover, duel::wire::kTargetNone);
  } else {
    OrdSet(v.ord, 2, 1, 1, 0, 0);
  }
  v.ordLen = duel::wire::OrdersLen(2);
  return v;
}

// Guarding YOURSELF is meaningless -- reject it rather than silently allowing a
// second encoding of "I guard nobody".
ApplyVec MakeVec_GuardRejectSelf() {
  return MakeVec_GuardRejectTarget("guard_reject_self", 0, 250);
}
// Guarding a DEAD ally: there is nothing to cover.
ApplyVec MakeVec_GuardRejectDeadAlly() {
  return MakeVec_GuardRejectTarget("guard_reject_dead_ally", 1, 0);
}
// Guarding a slot that does not exist (team_size is 2).
ApplyVec MakeVec_GuardRejectOutOfRange() {
  return MakeVec_GuardRejectTarget("guard_reject_out_of_range", 2, 250);
}

void test_apply_guard_rejects_bad_target() {
  ApplyVec self = MakeVec_GuardRejectSelf();
  CHECK(duel_apply(self.st, self.stLen, self.ord, self.ordLen) == -1);
  CHECK(duel_out_len() == 0);
  CHECK(duel_log_len() == 0);

  ApplyVec dead = MakeVec_GuardRejectDeadAlly();
  CHECK(duel_apply(dead.st, dead.stLen, dead.ord, dead.ordLen) == -1);
  CHECK(duel_out_len() == 0);

  ApplyVec oor = MakeVec_GuardRejectOutOfRange();
  CHECK(duel_apply(oor.st, oor.stLen, oor.ord, oor.ordLen) == -1);
  CHECK(duel_out_len() == 0);
}

// Two allies guard the same ward: the LOWEST guardian slot wins the tie (a
// total, deterministic rule -- the alternative is a coin the engine cannot toss).
ApplyVec MakeVec_GuardTieLowestSlot() {
  ApplyVec v{};
  v.name = "guard_tie_lowest_slot";
  duel::DuelState s = MakeState(3, 1);
  const uint8_t mG[] = {1}, cG[] = {0};
  const uint8_t mH[] = {24}, cH[] = {0};
  for (int slot = 0; slot < 3; ++slot) SetTreat(s, 0, slot, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mG, cG); // guardian A (wins the tie)
  SetTreat(s, 1, 1, 250, 250, 3, 6, 1, mG, cG); // guardian B
  SetTreat(s, 1, 2, 250, 250, 3, 6, 1, mH, cH); // the ward
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 3, 0, 0, 0, 2); // -> enemy slot 2 (the ward)
  OrdSet(v.ord, 3, 0, 1, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 0, 2, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 1, 0, 0, 2); // both guard ally slot 2
  OrdSet(v.ord, 3, 1, 1, 0, 2);
  OrdSet(v.ord, 3, 1, 2, duel::wire::kActionRecover, duel::wire::kTargetNone);
  v.ordLen = duel::wire::OrdersLen(3);
  return v;
}

void test_apply_guard_lowest_slot_wins_tie() {
  ApplyVec v = MakeVec_GuardTieLowestSlot();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 232); // guardian A took the blow (250 - 18)
  CHECK(o.treats[1][1].hp == 250); // guardian B did not
  CHECK(o.treats[1][2].hp == 250); // the ward is untouched
  CHECK(LogHas("\"target\":2,\"via\":0,"));
}

// A guardian KO'd EARLIER IN THE SAME ROUND cannot intercept: the guard lapses
// and the blow lands on the ward. (The map is built before any action resolves,
// so the hp check has to happen at INTERCEPT time, not at map-build time.)
ApplyVec MakeVec_GuardDeadGuardianLapses() {
  ApplyVec v{};
  v.name = "guard_dead_guardian_lapses";
  duel::DuelState s = MakeState(2, 1);
  const uint8_t mS[] = {2}, cS[] = {0};  // Quicksilver Slice (Speedy 30), speed 93
  const uint8_t mH[] = {24}, cH[] = {0}; // Pop Rock Pop (Heavy 26), speed 33
  const uint8_t mG[] = {1}, cG[] = {0};  // Gilded Bonds (guard), speed 45
  SetTreat(s, 0, 0, 250, 250, 4, 10, 1, mS, cS);
  SetTreat(s, 0, 1, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 0, 20, 250, 3, 6, 1, mG, cG);  // the guardian, 20 hp (see the clash below)
  SetTreat(s, 1, 1, 250, 250, 3, 6, 1, mH, cH); // the ward
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 2, 0, 0, 0, 0); // the fast Speedy KOs the guardian first
  OrdSet(v.ord, 2, 0, 1, 0, 1); // ...then this blow aims at the ward
  OrdSet(v.ord, 2, 1, 0, 0, 1); // the guard that will never fire
  OrdSet(v.ord, 2, 1, 1, duel::wire::kActionRecover, duel::wire::kTargetNone);
  v.ordLen = duel::wire::OrdersLen(2);
  return v;
}

void test_apply_guard_dead_guardian_cannot_intercept() {
  ApplyVec v = MakeVec_GuardDeadGuardianLapses();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  // Speedy vs the guardian's Blocking = dis 205: (30*205)>>8 = 24 > 20 hp.
  CHECK(o.treats[1][0].hp == 0);
  // The guard lapsed: the ward takes the SECOND blow in full (26, neutral --
  // it is recovering), not the 18 a live guardian would have absorbed for it.
  CHECK(o.treats[1][1].hp == 224);
  CHECK(LogHas("{\"side\":0,\"slot\":1,\"kind\":\"hit\",\"move\":24,\"target\":1,"
               "\"via\":255,\"clash\":\"neu\",\"dmg\":26,\"blocked\":false,"
               "\"absorbed\":0,\"targetHp\":224,\"ko\":false}"));
  // ...and the dead guardian's own action is a plain skip, never a "guard".
  CHECK(LogHas("{\"side\":1,\"slot\":0,\"kind\":\"skip\""));
  CHECK(LogCount("\"kind\":\"guard\"") == 0);
}

// SINGLE HOP: A guards B, B guards C. A blow at C is redirected to B and STOPS
// THERE -- it is not then redirected on to A. (Chaining is an infinite-loop
// hazard and a consensus footgun.)
ApplyVec MakeVec_GuardNoChain() {
  ApplyVec v{};
  v.name = "guard_no_chain";
  duel::DuelState s = MakeState(3, 1);
  const uint8_t mG[] = {1}, cG[] = {0};
  const uint8_t mH[] = {24}, cH[] = {0};
  for (int slot = 0; slot < 3; ++slot) SetTreat(s, 0, slot, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mG, cG); // A guards B
  SetTreat(s, 1, 1, 250, 250, 3, 6, 1, mG, cG); // B guards C
  SetTreat(s, 1, 2, 250, 250, 3, 6, 1, mH, cH); // C, the ward
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 3, 0, 0, 0, 2); // blow at C
  OrdSet(v.ord, 3, 0, 1, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 0, 2, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 1, 0, 0, 1); // A -> B
  OrdSet(v.ord, 3, 1, 1, 0, 2); // B -> C
  OrdSet(v.ord, 3, 1, 2, duel::wire::kActionRecover, duel::wire::kTargetNone);
  v.ordLen = duel::wire::OrdersLen(3);
  return v;
}

void test_apply_guard_redirect_never_chains() {
  ApplyVec v = MakeVec_GuardNoChain();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][1].hp == 232); // B took it (250 - 18) and the hop ENDED
  CHECK(o.treats[1][0].hp == 250); // A -- who guards B -- is untouched
  CHECK(o.treats[1][2].hp == 250); // C, the original ward, is untouched
  CHECK(LogHas("\"target\":2,\"via\":1,"));
}

// --- Shield ---

// A shield move raises a self-absorb pool worth its `power` and deals NO damage.
// The pool is eaten BEFORE hp and the excess carries through: Heavy 26 vs a
// Blocking target = adv 320 -> (26*320)>>8 = 32; shield 14 eats 14, hp takes the
// remaining 18. And because `shield` is not `block`, it buys no flat reduction at
// all -- the blow lands for its full 32.
ApplyVec MakeVec_ShieldRaiseAbsorb() {
  ApplyVec v{};
  v.name = "shield_raise_absorb";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mH[] = {24}, cH[] = {0}; // Pop Rock Pop (Heavy 26), speed 33
  const uint8_t mS[] = {18}, cS[] = {0}; // Sugar Shield (shield 14), speed 43
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mS, cS);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 1, 0, 0, 0, 0);
  OrdSet(v.ord, 1, 1, 0, 0, 0);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

void test_apply_shield_raise_and_absorb() {
  ApplyVec v = MakeVec_ShieldRaiseAbsorb();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 232);   // 250 - (32 - 14 absorbed)
  CHECK(o.treats[1][0].shield == 0); // the pool is spent exactly
  CHECK(o.treats[0][0].hp == 250);   // a shield deals NO damage
  CHECK(LogHas("{\"side\":1,\"slot\":0,\"kind\":\"shield\",\"move\":18,"
               "\"target\":255,\"via\":255,\"clash\":\"neu\",\"dmg\":14,"
               "\"blocked\":false,\"absorbed\":0,\"targetHp\":250,\"ko\":false}"));
  CHECK(LogHas("{\"side\":0,\"slot\":0,\"kind\":\"hit\",\"move\":24,\"target\":0,"
               "\"via\":255,\"clash\":\"adv\",\"dmg\":32,\"blocked\":false,"
               "\"absorbed\":14,\"targetHp\":232,\"ko\":false}"));
}

// A shield PERSISTS ACROSS ROUNDS until consumed. Round 1: a 50-point pool eats
// a whole 26-damage blow (hp untouched, 24 left in the pool). Round 2 replays
// the SAME orders against round 1's OUTPUT state: the last 24 points are eaten
// exactly, and only the 2-point remainder reaches hp.
ApplyVec MakeVec_ShieldPersistRound1() {
  ApplyVec v{};
  v.name = "shield_persist_round1";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mH[] = {24}, cH[] = {0};
  const uint8_t mS[] = {18}, cS[] = {0};
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mS, cS, /*shield=*/50);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 1, 0, 0, 0, 0);
  // The defender just recovers: the ONLY shield in play is the one it walked
  // into the round already holding.
  OrdSet(v.ord, 1, 1, 0, duel::wire::kActionRecover, duel::wire::kTargetNone);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

// Round 2 = round 1's output state, same orders (Pop Rock Pop's cooldown is 0).
ApplyVec MakeVec_ShieldPersistRound2() {
  ApplyVec r1 = MakeVec_ShieldPersistRound1();
  duel_apply(r1.st, r1.stLen, r1.ord, r1.ordLen);
  ApplyVec v{};
  v.name = "shield_persist_round2";
  v.stLen = duel_out_len();
  for (uint32_t i = 0; i < v.stLen; ++i) v.st[i] = duel_out_ptr()[i];
  v.ordLen = r1.ordLen;
  for (uint32_t i = 0; i < v.ordLen; ++i) v.ord[i] = r1.ord[i];
  return v;
}

void test_apply_shield_persists_between_rounds() {
  ApplyVec r1 = MakeVec_ShieldPersistRound1();
  CHECK(duel_apply(r1.st, r1.stLen, r1.ord, r1.ordLen) == 0);
  {
    duel::DuelState o{};
    CHECK(DecodeOut(&o));
    CHECK(o.treats[1][0].hp == 250);    // the pool ate the WHOLE blow
    CHECK(o.treats[1][0].shield == 24); // 50 - 26, carried into round 2
    CHECK(LogHas("\"absorbed\":26,\"targetHp\":250"));
  }
  ApplyVec r2 = MakeVec_ShieldPersistRound2();
  CHECK(duel_apply(r2.st, r2.stLen, r2.ord, r2.ordLen) == 0);
  {
    duel::DuelState o{};
    CHECK(DecodeOut(&o));
    CHECK(o.treats[1][0].hp == 248);   // 250 - (26 - 24 absorbed)
    CHECK(o.treats[1][0].shield == 0); // consumed exactly, not clamped early
    CHECK(LogHas("\"absorbed\":24,\"targetHp\":248"));
  }
}

// The pool is a u8: raising it past 255 CLAMPS (250 + a 14-power Sugar Shield
// grants only the 5 points that fit) rather than wrapping.
ApplyVec MakeVec_ShieldClamp255() {
  ApplyVec v{};
  v.name = "shield_clamp_255";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mH[] = {24}, cH[] = {0};
  const uint8_t mS[] = {18}, cS[] = {0};
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mS, cS, /*shield=*/250);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 1, 0, 0, 0, 0);
  OrdSet(v.ord, 1, 1, 0, 0, 0);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

void test_apply_shield_clamps_at_255() {
  ApplyVec v = MakeVec_ShieldClamp255();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  // 250 + min(14, 255-250) = 255, then the 32-damage blow eats 32 of it.
  CHECK(o.treats[1][0].shield == 223);
  CHECK(o.treats[1][0].hp == 250); // nothing reached hp
  CHECK(LogHas("\"kind\":\"shield\",\"move\":18,\"target\":255,\"via\":255,"
               "\"clash\":\"neu\",\"dmg\":5,")); // only the 5 points that FIT
}

// --- Counter ---

// A counter-stance treat reflects counter_pct (102/256 = 40%) of `landed` back
// at whoever struck it. Berry Bounce (Blocking) also swings normally: Blocking
// vs Heavy = dis 205 -> (22*205)>>8 = 17 on the attacker. The attacker's Heavy vs
// Blocking = adv 320 -> (26*320)>>8 = 32 -- landing IN FULL, because a counter
// buys no flat reduction (the stance split) -- and the reflect is (32*102)>>8 = 12.
ApplyVec MakeVec_CounterReflects() {
  ApplyVec v{};
  v.name = "counter_reflects";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mH[] = {24}, cH[] = {0}; // Pop Rock Pop (Heavy 26), speed 33
  const uint8_t mC[] = {8}, cC[] = {0};  // Berry Bounce (counter 22), speed 46
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mC, cC);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 1, 0, 0, 0, 0);
  OrdSet(v.ord, 1, 1, 0, 0, 0);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

void test_apply_counter_reflects() {
  ApplyVec v = MakeVec_CounterReflects();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 218); // 250 - 32: NOT reduced, a counter isn't a block
  CHECK(o.treats[0][0].hp == 221); // 250 - 17 (its swing) - 12 (the reflect)
  CHECK(LogHas("{\"side\":1,\"slot\":0,\"kind\":\"counter\",\"move\":8,"
               "\"target\":0,\"via\":255,\"clash\":\"neu\",\"dmg\":12,"
               "\"blocked\":false,\"absorbed\":0,\"targetHp\":221,\"ko\":false}"));
}

// A REDIRECTED blow triggers NO counter -- it fires off the FINAL RECIPIENT's
// own move, and the final recipient here is the guardian (whose move is
// `guard`, never `counter`). No special case: it falls straight out of the rule.
ApplyVec MakeVec_CounterNoneOnRedirect() {
  ApplyVec v{};
  v.name = "counter_none_on_redirect";
  duel::DuelState s = MakeState(2, 1);
  const uint8_t mH[] = {24}, cH[] = {0};
  const uint8_t mG[] = {1}, cG[] = {0}; // Gilded Bonds (guard)
  const uint8_t mC[] = {8}, cC[] = {0}; // Berry Bounce (counter)
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 0, 1, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mG, cG); // guardian
  SetTreat(s, 1, 1, 250, 250, 3, 6, 1, mC, cC); // the COUNTER-stance ward
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 2, 0, 0, 0, 1); // strike the counter-er...
  OrdSet(v.ord, 2, 0, 1, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 2, 1, 0, 0, 1); // ...but the guardian steps in front
  OrdSet(v.ord, 2, 1, 1, 0, 0);
  v.ordLen = duel::wire::OrdersLen(2);
  return v;
}

void test_apply_counter_none_on_redirected_blow() {
  ApplyVec v = MakeVec_CounterNoneOnRedirect();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  // Heavy vs the ward's Blocking = adv 320, then guard_pct: (26*320*179)>>16 = 22.
  CHECK(o.treats[1][0].hp == 228); // the guardian ate it
  CHECK(o.treats[1][1].hp == 250); // the counter-er never got hit...
  CHECK(LogCount("\"kind\":\"counter\"") == 0); // ...so it never countered
  // The attacker is only down by the ward's own Berry Bounce swing (dis: 17).
  CHECK(o.treats[0][0].hp == 233);
}

// A reflect CANNOT ITSELF BE COUNTERED -- it is not a move. Both sides hold a
// counter stance and both swing: exactly TWO counter entries (one per blow), not
// four, and certainly not a recursion.
ApplyVec MakeVec_CounterReflectNotCountered() {
  ApplyVec v{};
  v.name = "counter_reflect_not_countered";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mB[] = {8}, cB[] = {0};  // Berry Bounce (counter 22), speed 46
  const uint8_t mR[] = {21}, cR[] = {0}; // Bouncing Barrage (counter 33), speed 45
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mB, cB);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mR, cR);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 1, 0, 0, 0, 0);
  OrdSet(v.ord, 1, 1, 0, 0, 0);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

void test_apply_counter_reflect_cannot_be_countered() {
  ApplyVec v = MakeVec_CounterReflectNotCountered();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  // Blocking vs Blocking = neutral throughout. Side 0 swings first (speed 46):
  // 22 lands, side 1 reflects (22*102)>>8 = 8. Side 1 then swings: 33 lands,
  // side 0 reflects (33*102)>>8 = 13.
  CHECK(o.treats[1][0].hp == 215); // 250 - 22 - 13 (side 0's own reflect)
  CHECK(o.treats[0][0].hp == 209); // 250 - 8 (the reflect) - 33
  CHECK(LogCount("\"kind\":\"counter\"") == 2); // ONE per blow. No recursion.
}

// The reflect goes through the attacker's own SHIELD (an absorb pool absorbs
// anything -- it does not care where the damage came from).
ApplyVec MakeVec_CounterReflectAbsorbed() {
  ApplyVec v{};
  v.name = "counter_reflect_absorbed_by_shield";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mH[] = {24}, cH[] = {0};
  const uint8_t mC[] = {8}, cC[] = {0};
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mH, cH, /*shield=*/20);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mC, cC);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 1, 0, 0, 0, 0);
  OrdSet(v.ord, 1, 1, 0, 0, 0);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

void test_apply_counter_reflect_absorbed_by_shield() {
  ApplyVec v = MakeVec_CounterReflectAbsorbed();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  // Berry Bounce lands 17 -> the 20-pool eats it all (3 left, hp untouched).
  // The attacker's own 32 then reflects 12 -> the 3 remaining points are eaten,
  // and only 9 reaches hp.
  CHECK(o.treats[0][0].hp == 241);
  CHECK(o.treats[0][0].shield == 0);
  CHECK(LogHas("\"kind\":\"counter\",\"move\":8,\"target\":0,\"via\":255,"
               "\"clash\":\"neu\",\"dmg\":12,\"blocked\":false,\"absorbed\":3,"
               "\"targetHp\":241,\"ko\":false}"));
}

// The reflect is counter_pct of `landed` -- the blow's force at the moment it
// reached the recipient -- NEVER `drained` -- the recipient's actual hp
// decrease after ITS OWN shield ate part of the blow. The two are
// indistinguishable in every OTHER counter vector above, because none of them
// leaves the counter-stance recipient holding a nonzero shield: with
// shield == 0, absorbed == 0 and landed == drained by construction. This is
// the one state where they diverge, and it is the only vector that would
// catch the reflect silently being refactored onto `drained` (the quantity
// Task 6's siphon correctly uses).
//
// Attacker's Heavy vs the counter-er's Blocking = adv 320 -> landed =
// (26*320)>>8 = 32. The counter-er's OWN 20-point shield absorbs 20 of that
// (drained = 12), but the reflect must still be counter_pct of the full 32:
// (32*102)>>8 = 12 -- NOT (12*102)>>8 = 4.
ApplyVec MakeVec_CounterReflectsLandedNotDrained() {
  ApplyVec v{};
  v.name = "counter_reflects_landed_not_drained";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mH[] = {24}, cH[] = {0}; // Pop Rock Pop (Heavy 26), speed 33
  const uint8_t mC[] = {8}, cC[] = {0};  // Berry Bounce (counter 22), speed 46
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mC, cC, /*shield=*/20); // the counter-er ALSO shielded
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 1, 0, 0, 0, 0);
  OrdSet(v.ord, 1, 1, 0, 0, 0);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

void test_apply_counter_reflects_landed_not_drained() {
  ApplyVec v = MakeVec_CounterReflectsLandedNotDrained();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  // Berry Bounce swings first (speed 46): Blocking vs Heavy = dis 205 ->
  // (22*205)>>8 = 17, straight to the attacker's hp (it holds no shield).
  CHECK(o.treats[0][0].hp == 221); // 250 - 17 (its swing) - 12 (the reflect, off `landed`)
  // The attacker's 32 lands second: the counter-er's 20-point shield absorbs
  // 20, leaving 12 to reach hp (250 - 12 = 238) -- but the reflect below is
  // computed off the pre-absorb 32, not this drained 12.
  CHECK(o.treats[1][0].hp == 238);
  CHECK(o.treats[1][0].shield == 0);
  CHECK(LogHas("{\"side\":0,\"slot\":0,\"kind\":\"hit\",\"move\":24,\"target\":0,"
               "\"via\":255,\"clash\":\"adv\",\"dmg\":32,\"blocked\":false,"
               "\"absorbed\":20,\"targetHp\":238,\"ko\":false}"));
  CHECK(LogHas("{\"side\":1,\"slot\":0,\"kind\":\"counter\",\"move\":8,"
               "\"target\":0,\"via\":255,\"clash\":\"neu\",\"dmg\":12,"
               "\"blocked\":false,\"absorbed\":0,\"targetHp\":221,\"ko\":false}"));
}

// A reflect CAN KO the attacker (it is real damage, and the win check sees it).
ApplyVec MakeVec_CounterReflectKos() {
  ApplyVec v{};
  v.name = "counter_reflect_kos_attacker";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mH[] = {24}, cH[] = {0};
  const uint8_t mC[] = {8}, cC[] = {0};
  SetTreat(s, 0, 0, 20, 250, 3, 6, 1, mH, cH); // 20 hp: eats 17, then the 12-point reflect
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mC, cC);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 1, 0, 0, 0, 0);
  OrdSet(v.ord, 1, 1, 0, 0, 0);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

void test_apply_counter_reflect_can_ko() {
  ApplyVec v = MakeVec_CounterReflectKos();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[0][0].hp == 0);   // 20 - 17 = 3, then the 12-point reflect
  CHECK(o.treats[1][0].hp == 218); // the counter-er still took the blow
  CHECK(LogHas("\"kind\":\"counter\",\"move\":8,\"target\":0,\"via\":255,"
               "\"clash\":\"neu\",\"dmg\":12,\"blocked\":false,\"absorbed\":0,"
               "\"targetHp\":0,\"ko\":true}"));
  CHECK(o.phase == duel::wire::kPhaseSide1Won); // the win check sees the reflect
}

// A KILLING blow disarms the riposte: a counter-stance treat FELLED by the blow
// does not reflect (KO permanence -- a dead treat takes no further part in the
// round, the same rule that stops a dead guardian intercepting). Burst is the
// legible answer to a counter.
ApplyVec MakeVec_CounterKilledDoesNotReflect() {
  ApplyVec v{};
  v.name = "counter_killed_does_not_reflect";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mJ[] = {26}, cJ[] = {0}; // Vicious Jawbreaker (Heavy 31), speed 31
  const uint8_t mC[] = {8}, cC[] = {0};  // Berry Bounce (counter), speed 46
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mJ, cJ);
  SetTreat(s, 1, 0, 30, 250, 3, 6, 1, mC, cC); // 30 hp vs a 38-damage blow
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 1, 0, 0, 0, 0);
  OrdSet(v.ord, 1, 1, 0, 0, 0);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

void test_apply_counter_killed_does_not_reflect() {
  ApplyVec v = MakeVec_CounterKilledDoesNotReflect();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 0);                // (31*320)>>8 = 38 >= 30
  CHECK(o.treats[0][0].hp == 233);              // only Berry Bounce's own 17
  CHECK(LogCount("\"kind\":\"counter\"") == 0); // the dead do not riposte
  CHECK(o.phase == duel::wire::kPhaseSide0Won);
}

// --- Sudden death vs the new verbs ---
//
// The chip is the ARENA COLLAPSING, not an attack: it ignores Guard (no
// redirect) and Shield (nothing to absorb) alike. Round 12, chip 6.
ApplyVec MakeVec_SuddenDeathIgnoresGuardShield() {
  ApplyVec v{};
  v.name = "sudden_death_ignores_guard_and_shield";
  duel::DuelState s = MakeState(2, 1, 12);
  const uint8_t mG[] = {1}, cG[] = {0};
  const uint8_t mH[] = {24}, cH[] = {0};
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 0, 1, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mG, cG);              // guards slot 1
  SetTreat(s, 1, 1, 250, 250, 3, 6, 1, mH, cH, /*shield=*/50); // and is shielded
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 2, 0, 0, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 2, 0, 1, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 2, 1, 0, 0, 1); // the guard is up...
  OrdSet(v.ord, 2, 1, 1, duel::wire::kActionRecover, duel::wire::kTargetNone);
  v.ordLen = duel::wire::OrdersLen(2);
  return v;
}

void test_apply_sudden_death_ignores_guard_and_shield() {
  ApplyVec v = MakeVec_SuddenDeathIgnoresGuardShield();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 244);    // the guardian is chipped like anyone else
  CHECK(o.treats[1][1].hp == 244);    // ...and the ward takes its OWN chip
  CHECK(o.treats[1][1].shield == 50); // the pool is untouched: nothing to absorb
  CHECK(o.treats[0][0].hp == 244);
  CHECK(LogCount("\"kind\":\"chip\"") == 4);
}

// ======================= combat-depth Task 5: AoE =======================
//
// An AoE hits EVERY LIVING ENEMY, in slot order, at kTun.aoe_pct_256 of the move's
// power -- and the clash multiplier is computed PER VICTIM, against that victim's OWN
// picked move. It is the move that breaks the old "spreading damage is never better
// than concentrating it" proof, and it is the designated answer to a Guard wall:
//
//   AoE BEATS GUARD -- it hits the ward directly; the guardian does not intercept.
//   COUNTER BEATS AoE -- every countering victim reflects, so a caster who sprays into
//                        a wall of counter-stances pays for it (per victim, no cap).
//
// Block applies per victim, shield absorbs per victim, and each victim gets its OWN log
// entry with kind "aoe" (never "hit" -- the frontend has to be able to group one swing's
// victims, and "hit" must keep meaning single-target).
//
// Every vector below is a 3v3 whose side0/slot0 casts Gold Rush [34] (Distance, power
// 51, speed 69, cd 3). At aoe_pct_256 = 154 the three clash outcomes give three DIFFERENT
// numbers, which is exactly what pins "per victim":
//   adv  (51*384*154*256)>>24 = 46      dis  (51*170*154*256)>>24 = 20
//   neu  (51*256*154*256)>>24 = 30      dis+block (51*170*154*102)>>24 = 8

// Case A — three living enemies, each holding a DIFFERENT move TYPE, so the three
// splash damages must all differ. A test whose enemies clashed identically would pass
// even if the clash were computed ONCE against the wrong treat.
//   enemy slot0 picks Heavy    -> Distance BEATS Heavy      -> adv -> 20
//   enemy slot1 picks Speedy   -> Speedy BEATS Distance     -> dis -> 13
//   enemy slot2 picks Distance -> same type                 -> neu -> 16
// (Gold Rush is power 33 at aoe_pct 128 after Task 7: an AoE's raw power is now an
// ORDINARY blow of its tier, and the splash tax is what it pays for reaching everyone --
// it used to carry 51 power AND the 60% splash, i.e. a full blow on every foe, for free.)
ApplyVec MakeVec_AoePerVictimClash() {
  ApplyVec v{};
  v.name = "aoe_per_victim_clash";
  duel::DuelState s = MakeState(3, 1);
  const uint8_t mA[] = {kAoeMove}, cA[] = {0}; // the caster
  const uint8_t mH[] = {24}, cH[] = {0};       // Pop Rock Pop  (Heavy,    speed 27)
  const uint8_t mS[] = {3}, cS[] = {0};        // Coco Chaos    (Speedy,   speed 98)
  const uint8_t mD[] = {25}, cD[] = {0};       // Bubble Trouble (Distance, speed 76)
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mA, cA);
  SetTreat(s, 0, 1, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 0, 2, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 1, 250, 250, 3, 6, 1, mS, cS);
  SetTreat(s, 1, 2, 250, 250, 3, 6, 1, mD, cD);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 3, 0, 0, 0, duel::wire::kTargetAll); // the AoE: no slot, kTargetAll
  OrdSet(v.ord, 3, 0, 1, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 0, 2, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 1, 0, 0, 0); // all three enemies swing back at the caster
  OrdSet(v.ord, 3, 1, 1, 0, 0);
  OrdSet(v.ord, 3, 1, 2, 0, 0);
  v.ordLen = duel::wire::OrdersLen(3);
  return v;
}

void test_apply_aoe_hits_every_living_enemy_per_victim_clash() {
  ApplyVec v = MakeVec_AoePerVictimClash();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 230); // 250 - 20 (adv: Distance > Heavy)
  CHECK(o.treats[1][1].hp == 237); // 250 - 13 (dis: Speedy > Distance)
  CHECK(o.treats[1][2].hp == 234); // 250 - 16 (neu: Distance vs Distance)
  CHECK(LogCount("\"kind\":\"aoe\"") == 3); // one entry PER VICTIM, never one "hit"
  CHECK(LogCount("\"kind\":\"hit\"") == 3); // ...and the three enemy swings ARE hits
  CHECK(LogHas("{\"side\":0,\"slot\":0,\"kind\":\"aoe\",\"move\":34,\"target\":0,"
               "\"via\":255,\"clash\":\"adv\",\"dmg\":20,\"blocked\":false,"
               "\"absorbed\":0,\"targetHp\":230,\"ko\":false}"));
  CHECK(LogHas("{\"side\":0,\"slot\":0,\"kind\":\"aoe\",\"move\":34,\"target\":1,"
               "\"via\":255,\"clash\":\"dis\",\"dmg\":13,\"blocked\":false,"
               "\"absorbed\":0,\"targetHp\":237,\"ko\":false}"));
  CHECK(LogHas("{\"side\":0,\"slot\":0,\"kind\":\"aoe\",\"move\":34,\"target\":2,"
               "\"via\":255,\"clash\":\"neu\",\"dmg\":16,\"blocked\":false,"
               "\"absorbed\":0,\"targetHp\":234,\"ko\":false}"));
  // The victims are splashed in SLOT ORDER (deterministic, not initiative order).
  const int i0 = LogIndexOf("\"kind\":\"aoe\",\"move\":34,\"target\":0");
  const int i1 = LogIndexOf("\"kind\":\"aoe\",\"move\":34,\"target\":1");
  const int i2 = LogIndexOf("\"kind\":\"aoe\",\"move\":34,\"target\":2");
  CHECK(i0 >= 0 && i1 > i0 && i2 > i1);
  // The caster (Distance) eats all three swings: Coco Chaos adv 28, Bubble Trouble neu
  // 24, Pop Rock Pop dis 20 -- and never counters (its own move is an aoe, not a counter).
  CHECK(o.treats[0][0].hp == 178); // 250 - 28 - 24 - 20
  CHECK(LogCount("\"kind\":\"counter\"") == 0);
  CHECK(o.treats[0][0].cooldowns[0] == 3); // Gold Rush's authored cd
  CHECK(o.phase == duel::wire::kPhaseActive);
}

// Case B — BLOCK applies per victim, SHIELD absorbs per victim.
//   slot0 holds a `block` stance (Blocking, so also dis) -> (33*205*128*102)>>24 = 5
//   slot1 walked in with a 10-point shield pool and recovers -> neu 16, 10 absorbed,
//         only 6 reaches hp, pool spent exactly
//   slot2 recovers bare -> neu 16 straight to hp
ApplyVec MakeVec_AoeBlockAndShieldPerVictim() {
  ApplyVec v{};
  v.name = "aoe_block_and_shield_per_victim";
  duel::DuelState s = MakeState(3, 1);
  const uint8_t mA[] = {kAoeMove}, cA[] = {0};
  const uint8_t mB[] = {0}, cB[] = {0};  // Gum Drop Kick (Blocking/block, power 12)
  const uint8_t mH[] = {24}, cH[] = {0};
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mA, cA);
  SetTreat(s, 0, 1, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 0, 2, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mB, cB);
  // 10, not 20: the Task 7 splash is 16, so a 10-point pool is still "eaten EXACTLY, with
  // the excess carrying through to hp" -- which is the rule this vector exists to pin.
  SetTreat(s, 1, 1, 250, 250, 3, 6, 1, mH, cH, /*shield=*/10);
  SetTreat(s, 1, 2, 250, 250, 3, 6, 1, mH, cH);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 3, 0, 0, 0, duel::wire::kTargetAll);
  OrdSet(v.ord, 3, 0, 1, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 0, 2, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 1, 0, 0, 0); // the blocker swings back too
  OrdSet(v.ord, 3, 1, 1, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 1, 2, duel::wire::kActionRecover, duel::wire::kTargetNone);
  v.ordLen = duel::wire::OrdersLen(3);
  return v;
}

void test_apply_aoe_block_and_shield_per_victim() {
  ApplyVec v = MakeVec_AoeBlockAndShieldPerVictim();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 245);   // 250 - 5 (dis THEN blocked)
  CHECK(o.treats[1][1].hp == 244);   // 250 - (16 - 10 absorbed)
  CHECK(o.treats[1][1].shield == 0); // the pool is spent exactly
  CHECK(o.treats[1][2].hp == 234);   // 250 - 16, bare
  CHECK(LogHas("\"kind\":\"aoe\",\"move\":34,\"target\":0,\"via\":255,\"clash\":\"dis\","
               "\"dmg\":5,\"blocked\":true,\"absorbed\":0,\"targetHp\":245,\"ko\":false}"));
  CHECK(LogHas("\"kind\":\"aoe\",\"move\":34,\"target\":1,\"via\":255,\"clash\":\"neu\","
               "\"dmg\":16,\"blocked\":false,\"absorbed\":10,\"targetHp\":244,\"ko\":false}"));
  // Gum Drop Kick (Blocking) BEATS Distance: (12*320)>>8 = 15 on the caster.
  CHECK(o.treats[0][0].hp == 235);
}

// Case C — AoE IGNORES GUARD. slot0 guards slot1; the spray hits the ward DIRECTLY
// (`via` 255 on every aoe entry) and the guardian takes only its OWN splash. This is what
// makes AoE the answer to a Guard wall.
ApplyVec MakeVec_AoeIgnoresGuard() {
  ApplyVec v{};
  v.name = "aoe_ignores_guard";
  duel::DuelState s = MakeState(3, 1);
  const uint8_t mA[] = {kAoeMove}, cA[] = {0};
  const uint8_t mG[] = {1}, cG[] = {0}; // Gilded Bonds (Blocking/guard)
  const uint8_t mH[] = {24}, cH[] = {0};
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mA, cA);
  SetTreat(s, 0, 1, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 0, 2, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mG, cG); // the GUARDIAN
  SetTreat(s, 1, 1, 250, 250, 3, 6, 1, mH, cH); // the WARD
  SetTreat(s, 1, 2, 250, 250, 3, 6, 1, mH, cH);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 3, 0, 0, 0, duel::wire::kTargetAll);
  OrdSet(v.ord, 3, 0, 1, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 0, 2, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 1, 0, 0, 1); // guard -> ALLY slot 1
  OrdSet(v.ord, 3, 1, 1, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 1, 2, duel::wire::kActionRecover, duel::wire::kTargetNone);
  v.ordLen = duel::wire::OrdersLen(3);
  return v;
}

void test_apply_aoe_ignores_guard() {
  ApplyVec v = MakeVec_AoeIgnoresGuard();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 237); // the guardian takes only its OWN splash (dis 13)
  CHECK(o.treats[1][1].hp == 234); // the WARD takes its full 16: the guard did NOT intercept
  CHECK(o.treats[1][2].hp == 234);
  CHECK(LogCount("\"kind\":\"guard\"") == 1); // the guard action still happened...
  CHECK(!LogHas("\"kind\":\"aoe\",\"move\":34,\"target\":1,\"via\":0")); // ...and never fired
  CHECK(LogHas("\"kind\":\"aoe\",\"move\":34,\"target\":1,\"via\":255,\"clash\":\"neu\","
               "\"dmg\":16,\"blocked\":false,\"absorbed\":0,\"targetHp\":234,\"ko\":false}"));
  CHECK(o.treats[0][0].hp == 250); // a guard deals no damage
}

// Case D — COUNTER FIRES PER VICTIM. Two of the three enemies hold counter stances, so
// the one spray is reflected TWICE at the caster. This must fall out of Task 4's rule
// ("a counter fires iff the FINAL RECIPIENT's own picked move is a counter") with no
// special case at all -- AoE beats Guard, Counter beats AoE.
ApplyVec MakeVec_AoeCounteredPerVictim() {
  ApplyVec v{};
  v.name = "aoe_countered_per_victim";
  duel::DuelState s = MakeState(3, 1);
  const uint8_t mA[] = {kAoeMove}, cA[] = {0};
  const uint8_t mC1[] = {8}, cC1[] = {0};  // Berry Bounce      (counter, power 20, speed 46)
  const uint8_t mC2[] = {21}, cC2[] = {0}; // Bouncing Barrage  (counter, power 24, speed 45)
  const uint8_t mH[] = {24}, cH[] = {0};
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mA, cA);
  SetTreat(s, 0, 1, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 0, 2, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mC1, cC1);
  SetTreat(s, 1, 1, 250, 250, 3, 6, 1, mC2, cC2);
  SetTreat(s, 1, 2, 250, 250, 3, 6, 1, mH, cH);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 3, 0, 0, 0, duel::wire::kTargetAll);
  OrdSet(v.ord, 3, 0, 1, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 0, 2, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 1, 0, 0, 0); // both counter-stances also swing at the caster
  OrdSet(v.ord, 3, 1, 1, 0, 0);
  OrdSet(v.ord, 3, 1, 2, duel::wire::kActionRecover, duel::wire::kTargetNone);
  v.ordLen = duel::wire::OrdersLen(3);
  return v;
}

void test_apply_aoe_triggers_counter_per_victim() {
  ApplyVec v = MakeVec_AoeCounteredPerVictim();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  // Both counter-stances are Blocking, so the spray lands dis (13) on each; slot 2
  // recovers and takes the neutral 16.
  CHECK(o.treats[1][0].hp == 237);
  CHECK(o.treats[1][1].hp == 237);
  CHECK(o.treats[1][2].hp == 234);
  CHECK(LogCount("\"kind\":\"aoe\"") == 3);
  CHECK(LogCount("\"kind\":\"counter\"") == 2); // ONE PER COUNTERING VICTIM, not one per swing
  // The caster pays for spraying into two counters: 2 x (13*102)>>8 = 2 x 5 reflected,
  // then both Blocking swings land adv on its Distance move (27 + 41).
  CHECK(o.treats[0][0].hp == 172); // 250 - 5 - 5 - 27 - 41
  CHECK(LogHas("{\"side\":1,\"slot\":0,\"kind\":\"counter\",\"move\":8,\"target\":0,"
               "\"via\":255,\"clash\":\"neu\",\"dmg\":5,"));
  CHECK(LogHas("{\"side\":1,\"slot\":1,\"kind\":\"counter\",\"move\":21,\"target\":0,"
               "\"via\":255,\"clash\":\"neu\",\"dmg\":5,"));
}

// Case E — the one move that can cash TWO KILLS in a round: both low-hp enemies fall to
// the same spray.
ApplyVec MakeVec_AoeKillsTwo() {
  ApplyVec v{};
  v.name = "aoe_kills_two_at_once";
  duel::DuelState s = MakeState(3, 1);
  const uint8_t mA[] = {kAoeMove}, cA[] = {0};
  const uint8_t mH[] = {24}, cH[] = {0};
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mA, cA);
  SetTreat(s, 0, 1, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 0, 2, 250, 250, 3, 6, 1, mH, cH);
  // Every enemy RECOVERS (no move type -> the splash is neutral, 16), so 12 hp dies to it.
  SetTreat(s, 1, 0, 12, 250, 3, 6, 1, mH, cH);  // dies to the 16-point splash
  SetTreat(s, 1, 1, 12, 250, 3, 6, 1, mH, cH);  // ...and so does this one
  SetTreat(s, 1, 2, 250, 250, 3, 6, 1, mH, cH);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 3, 0, 0, 0, duel::wire::kTargetAll);
  OrdSet(v.ord, 3, 0, 1, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 0, 2, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 1, 0, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 1, 1, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 1, 2, duel::wire::kActionRecover, duel::wire::kTargetNone);
  v.ordLen = duel::wire::OrdersLen(3);
  return v;
}

void test_apply_aoe_kills_two_at_once() {
  ApplyVec v = MakeVec_AoeKillsTwo();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 0);   // 12 - 16
  CHECK(o.treats[1][1].hp == 0);   // 12 - 16
  CHECK(o.treats[1][2].hp == 234); // 250 - 16
  CHECK(LogCount("\"ko\":true") == 2); // TWO kills, one swing
  CHECK(o.phase == duel::wire::kPhaseActive); // ...but slot 2 still stands
}

// Case F — every enemy is already dead when the spray resolves (an earlier actor in the
// SAME round felled the last one): the AoE WHIFFS as exactly ONE "skip" entry, and still
// burns its cooldown.
ApplyVec MakeVec_AoeAllDeadWhiffs() {
  ApplyVec v{};
  v.name = "aoe_all_dead_whiffs";
  duel::DuelState s = MakeState(2, 1);
  const uint8_t mA[] = {kAoeMove}, cA[] = {0}; // speed 69 -- acts SECOND
  const uint8_t mS[] = {2}, cS[] = {0};        // Quicksilver Slice, speed 93 -- acts FIRST
  const uint8_t mH[] = {24}, cH[] = {0};
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mA, cA);
  SetTreat(s, 0, 1, 250, 250, 4, 10, 1, mS, cS);
  SetTreat(s, 1, 0, 0, 110, 1, 1, 1, mH, cH); // already KO'd
  SetTreat(s, 1, 1, 1, 110, 1, 1, 1, mH, cH); // 1 hp: the Speedy fells the LAST living enemy
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 2, 0, 0, 0, duel::wire::kTargetAll);
  OrdSet(v.ord, 2, 0, 1, 0, 1);
  OrdSet(v.ord, 2, 1, 0, duel::wire::kActionRecover, duel::wire::kTargetNone); // KO'd: canonical
  OrdSet(v.ord, 2, 1, 1, duel::wire::kActionRecover, duel::wire::kTargetNone);
  v.ordLen = duel::wire::OrdersLen(2);
  return v;
}

void test_apply_aoe_whiffs_when_every_enemy_is_dead() {
  ApplyVec v = MakeVec_AoeAllDeadWhiffs();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(LogCount("\"kind\":\"aoe\"") == 0); // nothing to splash
  CHECK(LogHas("{\"side\":0,\"slot\":0,\"kind\":\"skip\",\"move\":34,\"target\":255,"
               "\"via\":255,\"clash\":\"neu\",\"dmg\":0,\"blocked\":false,"
               "\"absorbed\":0,\"targetHp\":0,\"ko\":false}"));
  CHECK(o.treats[0][0].cooldowns[0] == 3); // the whiff still burns the move
  CHECK(o.phase == duel::wire::kPhaseSide0Won);
}

// Case G — a COUNTER that FELLS THE CASTER mid-spray STOPS IT. KO is permanent and a dead
// treat takes no further part in the round (the same rule that stops a dead guardian
// intercepting), so the enemies later in slot order are never splashed. It is the sharpest
// edge of "Counter beats AoE": a counter-stance on the FIRST slot can shield the rest of
// the team from a spray outright.
ApplyVec MakeVec_AoeCasterFelledMidSpray() {
  ApplyVec v{};
  v.name = "aoe_caster_felled_mid_spray";
  duel::DuelState s = MakeState(3, 1);
  const uint8_t mA[] = {kAoeMove}, cA[] = {0};
  const uint8_t mC[] = {8}, cC[] = {0}; // Berry Bounce (counter)
  const uint8_t mH[] = {24}, cH[] = {0};
  SetTreat(s, 0, 0, 5, 250, 3, 6, 1, mA, cA); // 5 hp: the first reflect (5) fells it
  SetTreat(s, 0, 1, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 0, 2, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mC, cC); // the counter-stance, in SLOT 0
  SetTreat(s, 1, 1, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 2, 250, 250, 3, 6, 1, mH, cH);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 3, 0, 0, 0, duel::wire::kTargetAll);
  OrdSet(v.ord, 3, 0, 1, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 0, 2, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 1, 0, 0, 0);
  OrdSet(v.ord, 3, 1, 1, duel::wire::kActionRecover, duel::wire::kTargetNone);
  OrdSet(v.ord, 3, 1, 2, duel::wire::kActionRecover, duel::wire::kTargetNone);
  v.ordLen = duel::wire::OrdersLen(3);
  return v;
}

void test_apply_aoe_caster_felled_mid_spray_stops() {
  ApplyVec v = MakeVec_AoeCasterFelledMidSpray();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[0][0].hp == 0);   // felled by slot 0's reflect (5 hp - 5)
  CHECK(o.treats[1][0].hp == 237); // it still ate its own splash (dis 13)
  CHECK(o.treats[1][1].hp == 250); // ...and the spray STOPPED with the caster
  CHECK(o.treats[1][2].hp == 250);
  CHECK(LogCount("\"kind\":\"aoe\"") == 1);
  CHECK(LogCount("\"kind\":\"counter\"") == 1);
  CHECK(LogHas("\"kind\":\"counter\",\"move\":8,\"target\":0,\"via\":255,\"clash\":\"neu\","
               "\"dmg\":5,\"blocked\":false,\"absorbed\":0,\"targetHp\":0,\"ko\":true}"));
  // ...and the counter-stance's own swing then finds a corpse and whiffs.
  CHECK(LogHas("{\"side\":1,\"slot\":0,\"kind\":\"skip\""));
  CHECK(o.phase == duel::wire::kPhaseActive); // side 0 still has slots 1 and 2
}

// --- Validation: CANONICAL ORDERS (one logical order, exactly one byte encoding) ---
//
// An AoE REQUIRES target == kTargetAll (0xFE); a non-AoE must NEVER use it. Both
// directions reject: accepting an AoE "aimed" at a slot and then ignoring the byte would
// give one logical order many encodings, and game channels SIGN order bytes.
ApplyVec MakeVec_AoeBadTarget(const char* name, uint8_t action, uint8_t target) {
  ApplyVec v{};
  v.name = name;
  duel::DuelState s = MakeState(3, 2);
  const uint8_t mA[] = {kAoeMove, 24}, cA[] = {0, 0}; // loadout: [0] the AoE, [1] a plain Heavy
  const uint8_t mH[] = {24, 25}, cH[] = {0, 0};
  SetTreat(s, 0, 0, 250, 250, 3, 6, 2, mA, cA);
  SetTreat(s, 0, 1, 250, 250, 3, 6, 2, mH, cH);
  SetTreat(s, 0, 2, 250, 250, 3, 6, 2, mH, cH);
  for (int slot = 0; slot < 3; ++slot) SetTreat(s, 1, slot, 250, 250, 3, 6, 2, mH, cH);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdRecoverAll(v.ord, 3);
  OrdSet(v.ord, 3, 0, 0, action, target);
  v.ordLen = duel::wire::OrdersLen(3);
  return v;
}

// An AoE aimed at a SLOT -> reject (not "accepted, byte ignored").
ApplyVec MakeVec_AoeRejectSlotTarget() {
  return MakeVec_AoeBadTarget("aoe_reject_slot_target", 0, 0);
}
// An AoE with the Recover sentinel (0xFF) -> reject.
ApplyVec MakeVec_AoeRejectTargetNone() {
  return MakeVec_AoeBadTarget("aoe_reject_target_none", 0, duel::wire::kTargetNone);
}
// A NON-AoE (loadout slot 1, a plain Heavy) using kTargetAll -> reject.
ApplyVec MakeVec_NonAoeRejectTargetAll() {
  return MakeVec_AoeBadTarget("non_aoe_reject_target_all", 1, duel::wire::kTargetAll);
}
// ...and the accept control: the SAME state with the canonical AoE order resolves.
ApplyVec MakeVec_AoeCanonicalTargetAccepts() {
  return MakeVec_AoeBadTarget("aoe_canonical_target_accepts", 0, duel::wire::kTargetAll);
}

void test_apply_aoe_target_byte_is_canonical() {
  ApplyVec slotTgt = MakeVec_AoeRejectSlotTarget();
  CHECK(duel_apply(slotTgt.st, slotTgt.stLen, slotTgt.ord, slotTgt.ordLen) == -1);
  CHECK(duel_out_len() == 0);
  CHECK(duel_log_len() == 0);

  ApplyVec noneTgt = MakeVec_AoeRejectTargetNone();
  CHECK(duel_apply(noneTgt.st, noneTgt.stLen, noneTgt.ord, noneTgt.ordLen) == -1);
  CHECK(duel_out_len() == 0);

  ApplyVec nonAoe = MakeVec_NonAoeRejectTargetAll();
  CHECK(duel_apply(nonAoe.st, nonAoe.stLen, nonAoe.ord, nonAoe.ordLen) == -1);
  CHECK(duel_out_len() == 0);

  // The accept control: without it, "always reject" would pass the three above.
  ApplyVec ok = MakeVec_AoeCanonicalTargetAccepts();
  CHECK(duel_apply(ok.st, ok.stLen, ok.ord, ok.ordLen) == 0);
  CHECK(LogCount("\"kind\":\"aoe\"") == 3);
}

// ====== combat-depth Task 6: heal, group-heal, siphon, uses, double-strikes ======
//
// The last of the verbs. Every one of them is bounded by the SAME rule, and it is the
// hardest rule in the engine:
//
//   KO IS PERMANENT. An unguarded heal is an accidental REVIVE.
//
// A heal on a dead ally is an explicit NO-OP -- not a reject (that would strand a duel
// whose only legal heal target died between the order being picked and the round
// resolving), not a revive. Every healing path below goes through the one hp == 0 guard,
// and the vectors here exist mostly to prove it holds from every direction: single heal,
// group heal, a siphon whose attacker was felled by the riposte before it could drink.
//
// Move stats used (stats_gen.h): Sugar Rush [27] {37 power, speed 82, cd 2, Speedy, HEAL};
// Super Sugary Rush [10] {51, 97, cd 3, Speedy, GROUPHEAL, max_uses 2}; Pucker Sucker [6]
// {24, 57, cd 0, Tricky, SIPHON}; Limon Shuriken [15] {41, 75, cd 2, Distance, hits 2};
// Silver Knuckles [37] {45, 30, cd 3, Heavy, max_uses 2}.

// --- Heal ---

// A heal restores `power` to an ALLY on the caster's OWN side. It is not a blow: no clash
// multiplier, and the ally's block/shield are irrelevant. 100 + 37 = 137.
ApplyVec MakeVec_HealWoundedAlly() {
  ApplyVec v{};
  v.name = "heal_wounded_ally";
  duel::DuelState s = MakeState(2, 1);
  const uint8_t mHeal[] = {27}, cHeal[] = {0}; // Sugar Rush (heal 37), speed 82
  const uint8_t mH[] = {24}, cH[] = {0};       // Pop Rock Pop
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mHeal, cHeal);
  SetTreat(s, 0, 1, 100, 250, 3, 6, 1, mH, cH); // the WOUNDED ally
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 1, 250, 250, 3, 6, 1, mH, cH);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdRecoverAll(v.ord, 2);
  OrdSet(v.ord, 2, 0, 0, 0, 1); // heal -> ALLY slot 1 (own side!)
  v.ordLen = duel::wire::OrdersLen(2);
  return v;
}

void test_apply_heal_wounded_ally() {
  ApplyVec v = MakeVec_HealWoundedAlly();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[0][1].hp == 142);         // 100 + 42, no clash multiplier anywhere
  CHECK(o.treats[1][0].hp == 250);         // a heal deals NO damage to anyone
  CHECK(o.treats[0][0].cooldowns[0] == 2); // ...and burns its cooldown like any move
  CHECK(LogHas("{\"side\":0,\"slot\":0,\"kind\":\"heal\",\"move\":27,\"target\":1,"
               "\"via\":255,\"clash\":\"neu\",\"dmg\":42,\"blocked\":false,"
               "\"absorbed\":0,\"targetHp\":142,\"ko\":false}"));
}

// Healing a FULL-HP ally is legal and simply WASTED: overheal is not an error, and the
// log reports the amount ACTUALLY restored (0), not the move's power.
ApplyVec MakeVec_HealFullHpWasted() {
  ApplyVec v = MakeVec_HealWoundedAlly();
  v.name = "heal_full_hp_wasted";
  duel::DuelState s = MakeState(2, 1);
  const uint8_t mHeal[] = {27}, cHeal[] = {0};
  const uint8_t mH[] = {24}, cH[] = {0};
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mHeal, cHeal);
  SetTreat(s, 0, 1, 250, 250, 3, 6, 1, mH, cH); // already FULL
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 1, 250, 250, 3, 6, 1, mH, cH);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  return v;
}

void test_apply_heal_full_hp_is_wasted_not_an_error() {
  ApplyVec v = MakeVec_HealFullHpWasted();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0); // legal, not a reject
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[0][1].hp == 250); // clamped at max_hp: no hp over the cap, ever
  CHECK(LogHas("\"kind\":\"heal\",\"move\":27,\"target\":1,\"via\":255,\"clash\":\"neu\","
               "\"dmg\":0,\"blocked\":false,\"absorbed\":0,\"targetHp\":250,\"ko\":false}"));
}

// *** THE LOAD-BEARING VECTOR OF THIS WHOLE TASK ***
//
// A heal on a DEAD ally is a NO-OP. Not a reject -- the order is legal, it is simply
// wasted -- and above all NOT A REVIVE: hp stays exactly 0, and the WIN CHECK is
// unaffected. Here side 0's slot 1 is already a corpse and its slot 0 is felled in this
// same round, so side 0 is WIPED and loses -- even though its dying act was to pour a
// heal into the corpse. If an unguarded heal could revive, this duel would still be
// ACTIVE, and that is exactly the bug this vector exists to make impossible.
ApplyVec MakeVec_HealDeadAllyIsNoop() {
  ApplyVec v{};
  v.name = "heal_dead_ally_is_a_noop";
  duel::DuelState s = MakeState(2, 1);
  const uint8_t mHeal[] = {27}, cHeal[] = {0}; // Sugar Rush (heal), speed 82 -- acts FIRST
  const uint8_t mJ[] = {26}, cJ[] = {0};       // Vicious Jawbreaker (Heavy 31), speed 31
  const uint8_t mH[] = {24}, cH[] = {0};
  SetTreat(s, 0, 0, 20, 250, 3, 6, 1, mHeal, cHeal); // 20 hp: the Jawbreaker fells it
  SetTreat(s, 0, 1, 0, 250, 3, 6, 1, mH, cH);        // ALREADY DEAD
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mJ, cJ);
  SetTreat(s, 1, 1, 250, 250, 3, 6, 1, mH, cH);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdRecoverAll(v.ord, 2); // the dead ally: canonical Recover, as the wire demands
  OrdSet(v.ord, 2, 0, 0, 0, 1); // heal -> the CORPSE in ally slot 1
  OrdSet(v.ord, 2, 1, 0, 0, 0); // Heavy -> enemy slot 0 (Heavy > Speedy: adv 38 >= 20 hp)
  v.ordLen = duel::wire::OrdersLen(2);
  return v;
}

void test_apply_heal_on_a_dead_ally_is_a_noop_never_a_revive() {
  ApplyVec v = MakeVec_HealDeadAllyIsNoop();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0); // LEGAL -- a no-op, not a reject
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[0][1].hp == 0); // *** NO REVIVE. Exactly 0. ***
  CHECK(o.treats[0][0].hp == 0); // the healer itself is felled by the Jawbreaker
  // ...so side 0 is WIPED and LOSES. A revive would have left this duel active.
  CHECK(o.phase == duel::wire::kPhaseSide1Won);
  // The wasted heal still LOGS -- the player aimed it there, and the client has to be
  // able to show that it did nothing (dmg 0, targetHp 0, and no `ko` on a corpse).
  CHECK(LogHas("{\"side\":0,\"slot\":0,\"kind\":\"heal\",\"move\":27,\"target\":1,"
               "\"via\":255,\"clash\":\"neu\",\"dmg\":0,\"blocked\":false,"
               "\"absorbed\":0,\"targetHp\":0,\"ko\":false}"));
}

// SELF-HEAL IS LEGAL (target == the caster's own slot) -- unlike guard, which rejects
// covering itself. In a 1v1 it is the only heal there is.
ApplyVec MakeVec_HealSelf() {
  ApplyVec v{};
  v.name = "heal_self";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mHeal[] = {27}, cHeal[] = {0};
  const uint8_t mH[] = {24}, cH[] = {0};
  SetTreat(s, 0, 0, 100, 250, 3, 6, 1, mHeal, cHeal);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mH, cH);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdRecoverAll(v.ord, 1);
  OrdSet(v.ord, 1, 0, 0, 0, 0); // target == OWN slot
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

void test_apply_heal_self_is_legal() {
  ApplyVec v = MakeVec_HealSelf();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[0][0].hp == 142); // 100 + 42, poured into itself
  CHECK(LogHas("\"kind\":\"heal\",\"move\":27,\"target\":0,"));
}

// A heal names an ALLY SLOT, so an out-of-range index and the kTargetAll sentinel are
// both rejects (0xFE >= any legal team_size -- the same one check catches it).
ApplyVec MakeVec_HealBadTarget(const char* name, uint8_t target) {
  ApplyVec v = MakeVec_HealWoundedAlly();
  v.name = name;
  OrdSet(v.ord, 2, 0, 0, 0, target);
  return v;
}
ApplyVec MakeVec_HealRejectOutOfRange() {
  return MakeVec_HealBadTarget("heal_reject_out_of_range", 2); // team_size is 2
}
ApplyVec MakeVec_HealRejectTargetAll() {
  return MakeVec_HealBadTarget("heal_reject_target_all", duel::wire::kTargetAll);
}

void test_apply_heal_rejects_bad_target() {
  ApplyVec oor = MakeVec_HealRejectOutOfRange();
  CHECK(duel_apply(oor.st, oor.stLen, oor.ord, oor.ordLen) == -1);
  CHECK(duel_out_len() == 0);
  ApplyVec all = MakeVec_HealRejectTargetAll();
  CHECK(duel_apply(all.st, all.stLen, all.ord, all.ordLen) == -1);
  CHECK(duel_out_len() == 0);
}

// --- Group-heal ---

// A group-heal names NOBODY (kTargetAll) and restores (power * aoe_pct_256) >> 8 =
// (48*128)>>8 = 24 to EVERY LIVING ALLY INCLUDING THE CASTER, in slot order. aoe_pct is
// reused deliberately as the SPREAD: reaching everyone costs per-head potency, exactly as
// it does for an AoE. Overheal clamps per ally; DEAD ALLIES GET NOTHING and log nothing --
// a group-heal is not a mass revive either.
ApplyVec MakeVec_GroupHeal() {
  ApplyVec v{};
  v.name = "group_heal";
  duel::DuelState s = MakeState(3, 1);
  const uint8_t mG[] = {10}, cG[] = {0}; // Super Sugary Rush (groupheal 48, max_uses 2)
  const uint8_t mH[] = {24}, cH[] = {0};
  SetTreat(s, 0, 0, 200, 250, 4, 10, 1, mG, cG); // the caster heals ITSELF too: +24
  SetTreat(s, 0, 1, 240, 250, 3, 6, 1, mH, cH);  // clamps: only 10 of the 24 fits
  SetTreat(s, 0, 2, 0, 250, 3, 6, 1, mH, cH);    // DEAD: gets nothing, logs nothing
  for (int slot = 0; slot < 3; ++slot) SetTreat(s, 1, slot, 250, 250, 3, 6, 1, mH, cH);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdRecoverAll(v.ord, 3);
  OrdSet(v.ord, 3, 0, 0, 0, duel::wire::kTargetAll);
  v.ordLen = duel::wire::OrdersLen(3);
  return v;
}

void test_apply_group_heal() {
  ApplyVec v = MakeVec_GroupHeal();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[0][0].hp == 224); // 200 + 24 -- the caster is an ally too
  CHECK(o.treats[0][1].hp == 250); // 240 + min(24, 10) -- clamped at max_hp
  CHECK(o.treats[0][2].hp == 0);   // *** DEAD STAYS DEAD. No mass revive. ***
  CHECK(LogCount("\"kind\":\"heal\"") == 2); // one entry per LIVING ally; the corpse gets none
  CHECK(LogHas("{\"side\":0,\"slot\":0,\"kind\":\"heal\",\"move\":10,\"target\":0,"
               "\"via\":255,\"clash\":\"neu\",\"dmg\":24,\"blocked\":false,"
               "\"absorbed\":0,\"targetHp\":224,\"ko\":false}"));
  CHECK(LogHas("{\"side\":0,\"slot\":0,\"kind\":\"heal\",\"move\":10,\"target\":1,"
               "\"via\":255,\"clash\":\"neu\",\"dmg\":10,\"blocked\":false,"
               "\"absorbed\":0,\"targetHp\":250,\"ko\":false}"));
  // Deterministic SLOT ORDER (not initiative order).
  const int i0 = LogIndexOf("\"kind\":\"heal\",\"move\":10,\"target\":0");
  const int i1 = LogIndexOf("\"kind\":\"heal\",\"move\":10,\"target\":1");
  CHECK(i0 >= 0 && i1 > i0);
  CHECK(o.treats[0][0].uses_left[0] == 1); // the ultimate spent one of its two uses
  CHECK(o.treats[1][0].hp == 250);         // a group-heal deals no damage
}

// A group-heal REQUIRES kTargetAll, exactly as an AoE does -- one logical order, one byte
// encoding. A slot index there is a hard reject, not a silently-ignored byte.
ApplyVec MakeVec_GroupHealRejectSlotTarget() {
  ApplyVec v = MakeVec_GroupHeal();
  v.name = "group_heal_reject_slot_target";
  OrdSet(v.ord, 3, 0, 0, 0, 0); // "aimed" at ally slot 0 -> REJECT
  return v;
}

void test_apply_group_heal_requires_target_all() {
  ApplyVec bad = MakeVec_GroupHealRejectSlotTarget();
  CHECK(duel_apply(bad.st, bad.stLen, bad.ord, bad.ordLen) == -1);
  CHECK(duel_out_len() == 0);
  CHECK(duel_log_len() == 0);
  // The accept control (without it, "always reject" would pass the check above).
  ApplyVec ok = MakeVec_GroupHeal();
  CHECK(duel_apply(ok.st, ok.stLen, ok.ord, ok.ordLen) == 0);
}

// --- Life siphon ---

// A siphon is a NORMAL SINGLE-TARGET BLOW first -- clash, block, shield and guard all
// apply -- and then heals the attacker by (siphon_pct_256 * DRAINED) >> 8.
//
// DRAINED, NOT LANDED. `drained` is the victim's ACTUAL hp decrease, after its shield ate
// its share; `landed` is the force that reached it. The counter (Task 4) uses the OTHER
// one. They are NOT one number: see the fully-shielded vector below, which is the one
// state where they visibly diverge, and its counter-side twin
// (counter_reflects_landed_not_drained). Collapsing them into a single "damage dealt"
// helper passes every other gate in this repo while silently deleting a consensus rule.
//
// Toffee Tripper (Tricky 27) vs a Heavy target: Tricky BEATS Heavy -> adv 320 ->
// landed = (27*320)>>8 = 33, drained 33 (no shield) -> heal = (33*128)>>8 = 16.
ApplyVec MakeVec_SiphonDrains() {
  ApplyVec v{};
  v.name = "siphon_drains";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mS[] = {5}, cS[] = {0};  // Toffee Tripper (siphon 27), speed 54
  const uint8_t mH[] = {24}, cH[] = {0}; // Pop Rock Pop (Heavy 26), speed 27
  SetTreat(s, 0, 0, 100, 250, 3, 6, 1, mS, cS);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mH, cH);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 1, 0, 0, 0, 0);
  OrdSet(v.ord, 1, 1, 0, 0, 0);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

void test_apply_siphon_heals_off_drained() {
  ApplyVec v = MakeVec_SiphonDrains();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 217); // 250 - 33
  // 100 + 16 (siphoned) - 20 (the Heavy's dis-205 swing: (26*205)>>8) = 96.
  CHECK(o.treats[0][0].hp == 96);
  CHECK(LogHas("{\"side\":0,\"slot\":0,\"kind\":\"hit\",\"move\":5,\"target\":0,"
               "\"via\":255,\"clash\":\"adv\",\"dmg\":33,\"blocked\":false,"
               "\"absorbed\":0,\"targetHp\":217,\"ko\":false}"));
  // The lifesteal rides in as its OWN `heal` entry on the attacker (target == its own
  // slot), so the client can float a green +16 over it.
  CHECK(LogHas("{\"side\":0,\"slot\":0,\"kind\":\"heal\",\"move\":5,\"target\":0,"
               "\"via\":255,\"clash\":\"neu\",\"dmg\":16,\"blocked\":false,"
               "\"absorbed\":0,\"targetHp\":116,\"ko\":false}"));
}

// *** THE OVERKILL-DRAIN PIN. *** LandBlow's `drained` is `fatal ? rcv.hp : toHp` -- a
// siphon into a victim who does NOT have `toHp` hp left to give drains only what was there,
// never the blow's full force. This is the SAME shape of hole as `drained` vs `landed`
// above: every other siphon vector in this file lands on a victim with room to spare, so
// `const uint32_t drained = toHp;` (the blow's full force, ignoring how much hp the victim
// actually had) passes every one of them, sanitizers AND native-vs-wasm parity while
// silently deleting this rule -- a future refactor collapsing the ternary would give every
// FINISHING siphon a ~6x lifesteal swing, in signed channel state.
//
// Same clash as siphon_drains (Tricky beats Heavy -> adv 320 -> landed = (27*320)>>8 = 33),
// but the victim has only 5 hp: fatal, so `drained` = 5 (not 33) -> heal = (5*128)>>8 = 2.
// The victim dies to the blow before its own (slower, speed 33) turn comes round, so it
// never swings back either -- the attacker's final hp is 100 + 2 = 102, not the 116 the
// inverted rule would give (100 + ((33*128)>>8) = 116, siphon_drains' own number).
ApplyVec MakeVec_SiphonOverkillDrainsOnlyWhatWasThere() {
  ApplyVec v{};
  v.name = "siphon_overkill_drains_only_what_was_there";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mS[] = {5}, cS[] = {0};  // Toffee Tripper (siphon 27), speed 54
  const uint8_t mH[] = {24}, cH[] = {0}; // Pop Rock Pop (Heavy 26), speed 27
  SetTreat(s, 0, 0, 100, 250, 3, 6, 1, mS, cS);
  SetTreat(s, 1, 0, 5, 250, 3, 6, 1, mH, cH); // 5 hp: the 33-force blow overkills it
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 1, 0, 0, 0, 0);
  OrdSet(v.ord, 1, 1, 0, 0, 0);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

void test_apply_siphon_overkill_drains_only_what_was_there() {
  ApplyVec v = MakeVec_SiphonOverkillDrainsOnlyWhatWasThere();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 0); // overkilled: the 33-force blow only had 5 hp to take
  // 100 + 2 (siphoned off the 5 it ACTUALLY drained, not the 33 that landed); the victim
  // is dead before its own slower turn, so no counter-swing lands back either.
  CHECK(o.treats[0][0].hp == 102);
  CHECK(LogHas("{\"side\":0,\"slot\":0,\"kind\":\"hit\",\"move\":5,\"target\":0,"
               "\"via\":255,\"clash\":\"adv\",\"dmg\":33,\"blocked\":false,"
               "\"absorbed\":0,\"targetHp\":0,\"ko\":true}"));
  CHECK(LogHas("{\"side\":0,\"slot\":0,\"kind\":\"heal\",\"move\":5,\"target\":0,"
               "\"via\":255,\"clash\":\"neu\",\"dmg\":2,\"blocked\":false,"
               "\"absorbed\":0,\"targetHp\":102,\"ko\":false}")); // NOT 16 (the toHp answer)
  CHECK(o.phase == duel::wire::kPhaseSide0Won);
}

// A BLOCKED siphon drains LESS, so it heals less: adv 320 then block (surviving 102) ->
// landed = (27*320*102)>>16 = 13, drained 13 -> heal = (13*128)>>8 = 6 (not 16).
ApplyVec MakeVec_SiphonBlockedHealsLess() {
  ApplyVec v{};
  v.name = "siphon_blocked_heals_less";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mS[] = {5}, cS[] = {0}; // Toffee Tripper (siphon 27), speed 54
  const uint8_t mB[] = {0}, cB[] = {0}; // Gum Drop Kick (block 12), speed 46
  SetTreat(s, 0, 0, 100, 250, 3, 6, 1, mS, cS);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mB, cB);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 1, 0, 0, 0, 0);
  OrdSet(v.ord, 1, 1, 0, 0, 0);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

void test_apply_siphon_blocked_heals_less() {
  ApplyVec v = MakeVec_SiphonBlockedHealsLess();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 237); // 250 - 13 (adv, then blocked)
  // 100 + 6 (half of the 13 it actually drained) - 9 (Gum Drop Kick's dis swing) = 97.
  CHECK(o.treats[0][0].hp == 97);
  CHECK(LogHas("\"kind\":\"heal\",\"move\":5,\"target\":0,\"via\":255,\"clash\":\"neu\","
               "\"dmg\":6,")); // 6, not the 16 an unblocked drain would have fed it
}

// *** THE `drained` vs `landed` PIN. *** A blow the victim's SHIELD ate whole drains
// NOTHING -- so it siphons NOTHING, even though it LANDED at full force. "I can only
// drain what I actually take out of you."
//
// The victim recovers (no move type -> neutral clash), so landed = (27*256)>>8 = 27, and
// its 50-point shield absorbs all 27: drained = 0 -> heal = 0. A siphon wired to `landed`
// would heal (27*128)>>8 = 13 here, and every other siphon vector in this file would still
// pass.
ApplyVec MakeVec_SiphonFullyShieldedHealsNothing() {
  ApplyVec v{};
  v.name = "siphon_fully_shielded_heals_nothing";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mS[] = {5}, cS[] = {0}; // Toffee Tripper (siphon 27), speed 54
  const uint8_t mH[] = {24}, cH[] = {0};
  SetTreat(s, 0, 0, 100, 250, 3, 6, 1, mS, cS);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mH, cH, /*shield=*/50);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 1, 0, 0, 0, 0);
  OrdSet(v.ord, 1, 1, 0, duel::wire::kActionRecover, duel::wire::kTargetNone);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

void test_apply_siphon_fully_shielded_heals_nothing() {
  ApplyVec v = MakeVec_SiphonFullyShieldedHealsNothing();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 250);    // the pool ate the WHOLE blow: hp untouched
  CHECK(o.treats[1][0].shield == 23); // 50 - 27
  CHECK(o.treats[0][0].hp == 100);    // *** and the siphon drank NOTHING ***
  CHECK(LogHas("\"kind\":\"hit\",\"move\":5,\"target\":0,\"via\":255,\"clash\":\"neu\","
               "\"dmg\":27,\"blocked\":false,\"absorbed\":27,\"targetHp\":250,\"ko\":false}"));
  CHECK(LogHas("\"kind\":\"heal\",\"move\":5,\"target\":0,\"via\":255,\"clash\":\"neu\","
               "\"dmg\":0,")); // NOT 13 -- `drained` is 0, however hard the blow landed
}

// A FIZZLED siphon (its target died earlier in the round) siphons nothing: there is no
// blow at all, so there is nothing to drain -- one `skip`, no `heal` entry.
ApplyVec MakeVec_SiphonFizzledHealsNothing() {
  ApplyVec v{};
  v.name = "siphon_fizzled_heals_nothing";
  duel::DuelState s = MakeState(2, 1);
  const uint8_t mC[] = {3}, cC[] = {0};  // Coco Chaos (Speedy 23), speed 98 -- kills first
  const uint8_t mS[] = {5}, cS[] = {0};  // Toffee Tripper (siphon), speed 54 -- too late
  const uint8_t mH[] = {24}, cH[] = {0};
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mC, cC);
  SetTreat(s, 0, 1, 100, 250, 3, 6, 1, mS, cS);
  SetTreat(s, 1, 0, 1, 250, 3, 6, 1, mH, cH); // 1 hp: dies to the Speedy
  SetTreat(s, 1, 1, 250, 250, 3, 6, 1, mH, cH);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdRecoverAll(v.ord, 2);
  OrdSet(v.ord, 2, 0, 0, 0, 0); // both aim at enemy slot 0
  OrdSet(v.ord, 2, 0, 1, 0, 0);
  v.ordLen = duel::wire::OrdersLen(2);
  return v;
}

void test_apply_siphon_fizzled_heals_nothing() {
  ApplyVec v = MakeVec_SiphonFizzledHealsNothing();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 0);
  CHECK(o.treats[0][1].hp == 100); // the siphoner drank NOTHING off a corpse
  CHECK(LogCount("\"kind\":\"heal\"") == 0);
  CHECK(LogHas("{\"side\":0,\"slot\":1,\"kind\":\"skip\",\"move\":5,"));
}

// A GUARDED siphon drains from the GUARDIAN -- it is the one that actually took the hit.
// Neutral clash (the ward recovers) then guard_pct: landed = (27*179)>>8 = 18 on the
// guardian, drained 18 -> heal = (18*128)>>8 = 9.
ApplyVec MakeVec_SiphonGuardedDrainsGuardian() {
  ApplyVec v{};
  v.name = "siphon_guarded_drains_the_guardian";
  duel::DuelState s = MakeState(2, 1);
  const uint8_t mS[] = {5}, cS[] = {0};  // Toffee Tripper (siphon), speed 54
  const uint8_t mG[] = {1}, cG[] = {0};  // Gilded Bonds (guard), speed 45
  const uint8_t mH[] = {24}, cH[] = {0};
  SetTreat(s, 0, 0, 100, 250, 3, 6, 1, mS, cS);
  SetTreat(s, 0, 1, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mG, cG); // the GUARDIAN
  SetTreat(s, 1, 1, 250, 250, 3, 6, 1, mH, cH); // the WARD (recovers)
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdRecoverAll(v.ord, 2);
  OrdSet(v.ord, 2, 0, 0, 0, 1); // the siphon aims at the ward...
  OrdSet(v.ord, 2, 1, 0, 0, 1); // ...and the guardian steps in front of it
  v.ordLen = duel::wire::OrdersLen(2);
  return v;
}

void test_apply_siphon_guarded_drains_the_guardian() {
  ApplyVec v = MakeVec_SiphonGuardedDrainsGuardian();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 232); // the GUARDIAN bled 18...
  CHECK(o.treats[1][1].hp == 250); // ...and the ward not at all
  CHECK(o.treats[0][0].hp == 109); // 100 + 9: drained off whoever actually took it
  CHECK(LogHas("\"kind\":\"hit\",\"move\":5,\"target\":1,\"via\":0,\"clash\":\"neu\","
               "\"dmg\":18,"));
  CHECK(LogHas("\"kind\":\"heal\",\"move\":5,\"target\":0,\"via\":255,\"clash\":\"neu\","
               "\"dmg\":9,"));
}

// A siphon CANNOT OVERHEAL: 245/250 drinks 16 but keeps only the 5 that fit.
ApplyVec MakeVec_SiphonCannotOverheal() {
  ApplyVec v = MakeVec_SiphonDrains();
  v.name = "siphon_cannot_overheal";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mS[] = {5}, cS[] = {0}; // Toffee Tripper (siphon 27), speed 54
  const uint8_t mH[] = {24}, cH[] = {0};
  SetTreat(s, 0, 0, 245, 250, 3, 6, 1, mS, cS); // 5 points of room, 16 of lifesteal
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mH, cH);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  return v;
}

void test_apply_siphon_cannot_overheal() {
  ApplyVec v = MakeVec_SiphonCannotOverheal();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[0][0].hp == 230); // 245 + min(16, 5) = 250, then the Heavy's 20
  CHECK(LogHas("\"kind\":\"heal\",\"move\":5,\"target\":0,\"via\":255,\"clash\":\"neu\","
               "\"dmg\":5,\"blocked\":false,\"absorbed\":0,\"targetHp\":250,\"ko\":false}"));
}

// A siphoner FELLED BY THE RIPOSTE drinks NOTHING. KO is permanent, so the corpse does not
// heal itself back up off the blow that killed it -- the same hp == 0 guard that stops a
// heal reviving an ally, reached from the other direction. (The blow still lands: its
// victim is down 33 all the same.)
ApplyVec MakeVec_SiphonFelledByCounterHealsNothing() {
  ApplyVec v{};
  v.name = "siphon_felled_by_counter_heals_nothing";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mS[] = {5}, cS[] = {0}; // Toffee Tripper (siphon 27), speed 54
  const uint8_t mC[] = {8}, cC[] = {0}; // Berry Bounce (counter), speed 46
  SetTreat(s, 0, 0, 5, 250, 3, 6, 1, mS, cS); // 5 hp: the reflect (13) fells it
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mC, cC);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 1, 0, 0, 0, 0);
  OrdSet(v.ord, 1, 1, 0, 0, 0);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

void test_apply_siphon_felled_by_counter_heals_nothing() {
  ApplyVec v = MakeVec_SiphonFelledByCounterHealsNothing();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  // Tricky vs Blocking = adv 320 -> 33 lands; the reflect is (33*102)>>8 = 13 >= 5 hp.
  CHECK(o.treats[1][0].hp == 217);
  CHECK(o.treats[0][0].hp == 0);             // *** dead, and it stays dead ***
  CHECK(LogCount("\"kind\":\"heal\"") == 0); // a corpse drinks nothing
  CHECK(o.phase == duel::wire::kPhaseSide1Won);
}

// --- Limited-use ultimates (max_uses) ---
//
// The owner's replacement for action points: the same "can I afford this now?" tension at
// a fraction of the cost. 255 is the UNLIMITED sentinel and is never decremented; an
// action whose uses_left is 0 is a HARD REJECT.

// Silver Knuckles [37] (max_uses 2) + Pop Rock Pop [24] (unlimited) in one loadout: round
// 1 spends the ultimate, round 2 (on round 1's OUTPUT state) proves uses_left SURVIVED the
// encode/decode -- and that the unlimited move never ticks down at all.
ApplyVec MakeVec_UsesRound1() {
  ApplyVec v{};
  v.name = "uses_round1_spend";
  duel::DuelState s = MakeState(1, 2);
  const uint8_t mA[] = {37, 24}, cA[] = {0, 0}; // [0] the ultimate, [1] the unlimited move
  const uint8_t mB[] = {24, 25}, cB[] = {0, 0};
  SetTreat(s, 0, 0, 250, 250, 4, 10, 2, mA, cA);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 2, mB, cB);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 1, 0, 0, 0, 0); // Silver Knuckles
  OrdSet(v.ord, 1, 1, 0, 0, 0);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

// Round 2 = round 1's output, now using the UNLIMITED move (Silver Knuckles is cooling).
ApplyVec MakeVec_UsesRound2() {
  ApplyVec r1 = MakeVec_UsesRound1();
  duel_apply(r1.st, r1.stLen, r1.ord, r1.ordLen);
  ApplyVec v{};
  v.name = "uses_round2_unlimited_never_ticks";
  v.stLen = duel_out_len();
  for (uint32_t i = 0; i < v.stLen; ++i) v.st[i] = duel_out_ptr()[i];
  OrdInit(v.ord);
  OrdSet(v.ord, 1, 0, 0, 1, 0); // the unlimited move
  OrdSet(v.ord, 1, 1, 0, 0, 0);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

// The SPENT ultimate: the same state with uses_left[0] == 0. Ordering it is a hard reject.
ApplyVec MakeVec_UsesSpentRejects() {
  ApplyVec v{};
  v.name = "uses_spent_rejects";
  duel::DuelState s = MakeState(1, 2);
  const uint8_t mA[] = {37, 24}, cA[] = {0, 0};
  const uint8_t uA[] = {0, 255}; // Silver Knuckles: SPENT
  const uint8_t mB[] = {24, 25}, cB[] = {0, 0};
  SetTreat(s, 0, 0, 250, 250, 4, 10, 2, mA, cA, /*shield=*/0, uA);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 2, mB, cB);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 1, 0, 0, 0, 0); // the SPENT move -> -1
  OrdSet(v.ord, 1, 1, 0, 0, 0);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

// The accept control: the SAME state, but with its last use left. Without this, "always
// reject" would pass the vector above.
ApplyVec MakeVec_UsesLastOneAccepts() {
  ApplyVec v = MakeVec_UsesSpentRejects();
  v.name = "uses_last_one_accepts";
  duel::DuelState s = MakeState(1, 2);
  const uint8_t mA[] = {37, 24}, cA[] = {0, 0};
  const uint8_t uA[] = {1, 255}; // ONE use left
  const uint8_t mB[] = {24, 25}, cB[] = {0, 0};
  SetTreat(s, 0, 0, 250, 250, 4, 10, 2, mA, cA, /*shield=*/0, uA);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 2, mB, cB);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  return v;
}

void test_apply_limited_uses() {
  // Round 1: the ultimate lands and BURNS a use (2 -> 1); the unlimited move is untouched.
  ApplyVec r1 = MakeVec_UsesRound1();
  CHECK(duel_apply(r1.st, r1.stLen, r1.ord, r1.ordLen) == 0);
  {
    duel::DuelState o{};
    CHECK(DecodeOut(&o));
    CHECK(o.treats[0][0].uses_left[0] == 1);   // 2 - 1
    CHECK(o.treats[0][0].uses_left[1] == 255); // the unlimited sentinel: NEVER decremented
    CHECK(o.treats[0][0].cooldowns[0] == 3);   // Silver Knuckles' authored cd
    // Heavy 40 vs Heavy = neutral: 250 - 40 = 210.
    CHECK(o.treats[1][0].hp == 210);
  }

  // Round 2 replays that OUTPUT state: uses_left survived the encode/decode round-trip,
  // and spending the UNLIMITED move leaves 255 at 255.
  ApplyVec r2 = MakeVec_UsesRound2();
  CHECK(duel_apply(r2.st, r2.stLen, r2.ord, r2.ordLen) == 0);
  {
    duel::DuelState o{};
    CHECK(DecodeOut(&o));
    CHECK(o.treats[0][0].uses_left[0] == 1);   // SURVIVED the round-trip, untouched
    CHECK(o.treats[0][0].uses_left[1] == 255); // still unlimited after being used
    CHECK(o.treats[0][0].cooldowns[0] == 2);   // ...and its cooldown ticked 3 -> 2
  }

  // A SPENT move is a hard reject -- the same door every other illegal order is refused at.
  ApplyVec spent = MakeVec_UsesSpentRejects();
  CHECK(duel_apply(spent.st, spent.stLen, spent.ord, spent.ordLen) == -1);
  CHECK(duel_out_len() == 0);
  CHECK(duel_log_len() == 0);

  // ...and its LAST use is still perfectly legal.
  ApplyVec last = MakeVec_UsesLastOneAccepts();
  CHECK(duel_apply(last.st, last.stLen, last.ord, last.ordLen) == 0);
  {
    duel::DuelState o{};
    CHECK(DecodeOut(&o));
    CHECK(o.treats[0][0].uses_left[0] == 0); // spent to exactly 0 -- and it stops there
  }
}

// --- Double-strike (hits: 2) ---
//
// A move PROPERTY, never a proc: there is no RNG anywhere in this engine. Limon Shuriken
// [15] resolves its damage branch TWICE; each strike clashes independently, can KO
// independently, gets its OWN log entry, and is punished by the recipient's counter
// separately.

// Two strikes, two entries. Distance BEATS Heavy -> adv 320 -> 22 per strike, 44 total.
// (Task 7 rebuilt Limon Shuriken around its BURST, not its per-strike power: 18 x 2 = 36
// per cast, which is what the balance report compares against a single blow of its tier.)
ApplyVec MakeVec_DoubleStrike() {
  ApplyVec v{};
  v.name = "double_strike";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mL[] = {15}, cL[] = {0}; // Limon Shuriken (Distance 18, hits 2), speed 75
  const uint8_t mH[] = {24}, cH[] = {0}; // Pop Rock Pop (Heavy 26), speed 33
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mL, cL);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mH, cH);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 1, 0, 0, 0, 0);
  OrdSet(v.ord, 1, 1, 0, 0, 0);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

void test_apply_double_strike() {
  ApplyVec v = MakeVec_DoubleStrike();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 206);            // 250 - 22 - 22
  CHECK(LogCount("\"move\":15") == 2);        // TWO entries -- the client plays two impacts
  CHECK(LogHas("\"kind\":\"hit\",\"move\":15,\"target\":0,\"via\":255,\"clash\":\"adv\","
               "\"dmg\":22,\"blocked\":false,\"absorbed\":0,\"targetHp\":228,\"ko\":false}"));
  CHECK(LogHas("\"kind\":\"hit\",\"move\":15,\"target\":0,\"via\":255,\"clash\":\"adv\","
               "\"dmg\":22,\"blocked\":false,\"absorbed\":0,\"targetHp\":206,\"ko\":false}"));
  // The Heavy swings back into a Distance move -- Distance BEATS Heavy, so it is at a
  // disadvantage: (26*205)>>8 = 20.
  CHECK(o.treats[0][0].hp == 230);
}

// If STRIKE 1 KILLS, STRIKE 2 NEVER HAPPENS -- and emits no entry. Consistent with
// fizzle-on-death: you cannot beat a corpse.
ApplyVec MakeVec_DoubleStrikeStopsOnKo() {
  ApplyVec v{};
  v.name = "double_strike_stops_on_ko";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mL[] = {15}, cL[] = {0};
  const uint8_t mH[] = {24}, cH[] = {0};
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mL, cL);
  SetTreat(s, 1, 0, 20, 250, 3, 6, 1, mH, cH); // 20 hp: strike 1 (22) fells it
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 1, 0, 0, 0, 0);
  OrdSet(v.ord, 1, 1, 0, 0, 0);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

void test_apply_double_strike_stops_on_ko() {
  ApplyVec v = MakeVec_DoubleStrikeStopsOnKo();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 0);
  CHECK(LogCount("\"move\":15") == 1); // ONE entry: the second shuriken is never thrown
  CHECK(LogHas("\"kind\":\"hit\",\"move\":15,\"target\":0,\"via\":255,\"clash\":\"adv\","
               "\"dmg\":22,\"blocked\":false,\"absorbed\":0,\"targetHp\":0,\"ko\":true}"));
  CHECK(o.phase == duel::wire::kPhaseSide0Won);
}

// EACH STRIKE IS A BLOW, so each triggers the recipient's counter: two strikes into a
// counter-stance = TWO reflects. No special case -- it falls out of Task 4's rule.
// Blocking BEATS Distance -> dis 205 -> 14 per strike; each reflects (14*102)>>8 = 5.
ApplyVec MakeVec_DoubleStrikeCounteredTwice() {
  ApplyVec v{};
  v.name = "double_strike_countered_twice";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mL[] = {15}, cL[] = {0}; // Limon Shuriken, speed 75 -- strikes first
  const uint8_t mC[] = {8}, cC[] = {0};  // Berry Bounce (counter 22), speed 46
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mL, cL);
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mC, cC);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 1, 0, 0, 0, 0);
  OrdSet(v.ord, 1, 1, 0, 0, 0);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

void test_apply_double_strike_is_countered_twice() {
  ApplyVec v = MakeVec_DoubleStrikeCounteredTwice();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[1][0].hp == 222);              // 250 - 14 - 14
  CHECK(LogCount("\"move\":15") == 2);
  CHECK(LogCount("\"kind\":\"counter\"") == 2); // ONE PER STRIKE -- you pay twice
  // 250 - 5 - 5 (the two reflects) - 27 (Berry Bounce's own adv swing) = 213.
  CHECK(o.treats[0][0].hp == 213);
}

// A COUNTER THAT FELLS THE ATTACKER between the strikes STOPS the second one -- KO is
// permanent, and a dead treat takes no further part in the round (the same rule that stops
// an AoE caster mid-spray).
ApplyVec MakeVec_DoubleStrikeAttackerFelledMidStrike() {
  ApplyVec v{};
  v.name = "double_strike_attacker_felled_mid_strike";
  duel::DuelState s = MakeState(1, 1);
  const uint8_t mL[] = {15}, cL[] = {0};
  const uint8_t mC[] = {8}, cC[] = {0}; // Berry Bounce (counter), speed 46
  SetTreat(s, 0, 0, 5, 250, 3, 6, 1, mL, cL); // 5 hp: strike 1's reflect (5) fells it
  SetTreat(s, 1, 0, 250, 250, 3, 6, 1, mC, cC);
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdInit(v.ord);
  OrdSet(v.ord, 1, 0, 0, 0, 0);
  OrdSet(v.ord, 1, 1, 0, 0, 0);
  v.ordLen = duel::wire::OrdersLen(1);
  return v;
}

void test_apply_double_strike_attacker_felled_mid_strike() {
  ApplyVec v = MakeVec_DoubleStrikeAttackerFelledMidStrike();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  CHECK(o.treats[0][0].hp == 0);
  CHECK(o.treats[1][0].hp == 236);              // struck ONCE (250 - 14), never twice
  CHECK(LogCount("\"move\":15") == 1);
  CHECK(LogCount("\"kind\":\"counter\"") == 1);
  CHECK(o.phase == duel::wire::kPhaseSide1Won);
}

// The GUARD REDIRECT is recomputed PER STRIKE. Strike 1 lands on the guardian and KILLS
// it; the guard then LAPSES, so strike 2 lands on the ward it was covering -- at full
// force, because the guardian is no longer there to take it at guard_pct.
ApplyVec MakeVec_DoubleStrikeGuardLapsesMidStrike() {
  ApplyVec v{};
  v.name = "double_strike_guard_lapses_mid_strike";
  duel::DuelState s = MakeState(2, 1);
  const uint8_t mL[] = {15}, cL[] = {0}; // Limon Shuriken (hits 2), speed 75
  const uint8_t mG[] = {1}, cG[] = {0};  // Gilded Bonds (guard), speed 45
  const uint8_t mH[] = {24}, cH[] = {0};
  SetTreat(s, 0, 0, 250, 250, 3, 6, 1, mL, cL);
  SetTreat(s, 0, 1, 250, 250, 3, 6, 1, mH, cH);
  SetTreat(s, 1, 0, 10, 250, 3, 6, 1, mG, cG); // the GUARDIAN, 10 hp -- strike 1 fells it
  SetTreat(s, 1, 1, 250, 250, 3, 6, 1, mH, cH); // the WARD (recovers -> neutral clash)
  v.stLen = duel::encode_state(s, v.st, sizeof(v.st));
  OrdRecoverAll(v.ord, 2);
  OrdSet(v.ord, 2, 0, 0, 0, 1); // both strikes aim at enemy slot 1 (the ward)
  OrdSet(v.ord, 2, 1, 0, 0, 1); // guard -> ALLY slot 1
  v.ordLen = duel::wire::OrdersLen(2);
  return v;
}

void test_apply_double_strike_guard_lapses_mid_strike() {
  ApplyVec v = MakeVec_DoubleStrikeGuardLapsesMidStrike();
  CHECK(duel_apply(v.st, v.stLen, v.ord, v.ordLen) == 0);
  duel::DuelState o{};
  CHECK(DecodeOut(&o));
  // Strike 1: neutral (the ward recovers) then guard_pct -> (18*179)>>8 = 12 >= 10 hp.
  CHECK(o.treats[1][0].hp == 0);
  // Strike 2: the guard has LAPSED, so it lands on the ward in full: 250 - 18 = 232.
  CHECK(o.treats[1][1].hp == 232);
  CHECK(LogHas("\"kind\":\"hit\",\"move\":15,\"target\":1,\"via\":0,\"clash\":\"neu\","
               "\"dmg\":12,\"blocked\":false,\"absorbed\":0,\"targetHp\":0,\"ko\":true}"));
  CHECK(LogHas("\"kind\":\"hit\",\"move\":15,\"target\":1,\"via\":255,\"clash\":\"neu\","
               "\"dmg\":18,\"blocked\":false,\"absorbed\":0,\"targetHp\":232,\"ko\":false}"));
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
          // A move is orderable iff it is off cooldown AND has a use left (combat-depth
          // Task 6: uses_left == 0 is a hard reject, so a soak that kept offering a spent
          // ultimate would fail on a LEGAL-orders-only contract it was itself breaking).
          uint8_t ready[4];
          int nready = 0;
          for (uint32_t j = 0; j < s.loadout_size && j < t.move_count; ++j) {
            if (t.cooldowns[j] == 0 && t.uses_left[j] != 0) {
              ready[nready++] = static_cast<uint8_t>(j);
            }
          }
          if (nready == 0 || (Xr() & 1u)) {
            OrdSet(ord, teamSize, side, slot, duel::wire::kActionRecover,
                   duel::wire::kTargetNone);
            continue;
          }
          const uint8_t a = ready[XrN(static_cast<uint32_t>(nready))];
          // A GUARD move (combat-depth Task 4) names a LIVING ALLY that is not
          // itself -- an enemy slot is not a legal target for it. If the side
          // has no such ally (a 1v1, or the last treat standing), the move has
          // no legal order at all this round and Recover is the only line.
          if (duel::kDuelMoves[t.moves[a]].effect == duel::kEffGuard) {
            uint8_t allies[duel::wire::kMaxTeamSize];
            uint32_t nAllies = 0;
            for (uint32_t k = 0; k < teamSize; ++k) {
              if (k != slot && s.treats[side][k].hp > 0) {
                allies[nAllies++] = static_cast<uint8_t>(k);
              }
            }
            if (nAllies == 0) {
              OrdSet(ord, teamSize, side, slot, duel::wire::kActionRecover,
                     duel::wire::kTargetNone);
              continue;
            }
            OrdSet(ord, teamSize, side, slot, a, allies[XrN(nAllies)]);
            continue;
          }
          // An AOE (Task 5) and a GROUP-HEAL (Task 6) name NOBODY: their target byte MUST
          // be kTargetAll, and a slot index there is a hard reject. (Neither needs a
          // living target to be a LEGAL order -- with none left they simply whiff.)
          const uint8_t eff = duel::kDuelMoves[t.moves[a]].effect;
          if (eff == duel::kEffAoe || eff == duel::kEffGroupHeal) {
            OrdSet(ord, teamSize, side, slot, a, duel::wire::kTargetAll);
            continue;
          }
          // Everything else names a SLOT INDEX: an ALLY slot for a `heal` (self-heal is
          // legal, and a dead ally is a legal-but-wasted no-op), an ENEMY slot for every
          // damaging move. Both are range-checked identically, so one random slot serves.
          OrdSet(ord, teamSize, side, slot, a,
                 static_cast<uint8_t>(XrN(teamSize)));
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
  const uint8_t mA[] = {2, 26}, cA[] = {0, 0};
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

// ---- Task 2 (combat depth): the hard log cap ----
//
// RUN ONLY BY duel_tests_logcap (`--logcap-only`), the twin binary build.sh
// compiles with -DDUEL_LOG_CAP=400. jsonout.h drops past-cap writes, so a
// round whose log doesn't fit must be REJECTED (-1, both output buffers
// cleared) rather than emitted as truncated JSON — the client's
// JSON.parse(readLog()) would throw on it. At the shipped 32 KiB cap no legal
// round can come close, so this is the only way to drive the real duel_apply
// down that path; the shrunk cap makes it reachable without weakening the
// shipped engine (the native+wasm builds never define DUEL_LOG_CAP, so they
// stay byte-identical to each other).
//
// Both halves matter: the 3v3 round must reject, and the 1v1 round (whose log
// DOES fit in 320 bytes) must still succeed — otherwise "always return -1"
// would pass.
void test_apply_log_overflow_rejects() {
  const uint8_t m[] = {24}, c[] = {0};

  // Positive control: a 1v1 recover-only round's log is 325 bytes — it fits.
  {
    duel::DuelState s = MakeState(1, 1);
    SetTreat(s, 0, 0, 250, 250, 1, 1, 1, m, c);
    SetTreat(s, 1, 0, 250, 250, 1, 1, 1, m, c);
    uint8_t st[kMaxStateLen];
    const uint32_t stLen = duel::encode_state(s, st, sizeof(st));
    uint8_t ord[32];
    OrdRecoverAll(ord, 1);
    CHECK(duel_apply(st, stLen, ord, duel::wire::OrdersLen(1)) == 0);
    CHECK(duel_log_len() > 0);
    CHECK(duel_out_len() > 0);
  }

  // A 3v3 round's log is 909 bytes — it does NOT fit. Reject, don't truncate:
  // rc -1 with BOTH output buffers cleared, so a rejected round leaves the
  // caller's state untouched (same contract as every other reject path).
  {
    duel::DuelState s = MakeState(3, 1);
    for (int slot = 0; slot < 3; ++slot) {
      SetTreat(s, 0, slot, 250, 250, 1, 1, 1, m, c);
      SetTreat(s, 1, slot, 250, 250, 1, 1, 1, m, c);
    }
    uint8_t st[kMaxStateLen];
    const uint32_t stLen = duel::encode_state(s, st, sizeof(st));
    uint8_t ord[32];
    OrdRecoverAll(ord, 3);
    CHECK(duel_apply(st, stLen, ord, duel::wire::OrdersLen(3)) == -1);
    CHECK(duel_out_len() == 0);
    CHECK(duel_log_len() == 0);
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
  DumpApplyVector(MakeVec_FizzleOnDeath());
  DumpApplyVector(MakeVec_FizzleMultiLivingUntouched());
  DumpApplyVector(MakeVec_EqualSpeedEvenRound());
  DumpApplyVector(MakeVec_EqualSpeedOddRound());
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
  DumpApplyVector(MakeVec_SuddenDeathNotYet());
  DumpApplyVector(MakeVec_SuddenDeathFirstChip());
  DumpApplyVector(MakeVec_SuddenDeathEscalated());
  DumpApplyVector(MakeVec_SuddenDeathKo());
  DumpApplyVector(MakeVec_RoundCapUnequalFinal());
  DumpApplyVector(MakeVec_RoundCapEqualFinal());
  DumpApplyVector(MakeVec_Deterministic());
  // combat-depth Task 4: Guard, Shield, Counter.
  DumpApplyVector(MakeVec_GuardIntercept());
  DumpApplyVector(MakeVec_GuardRejectSelf());
  DumpApplyVector(MakeVec_GuardRejectDeadAlly());
  DumpApplyVector(MakeVec_GuardRejectOutOfRange());
  DumpApplyVector(MakeVec_GuardTieLowestSlot());
  DumpApplyVector(MakeVec_GuardDeadGuardianLapses());
  DumpApplyVector(MakeVec_GuardNoChain());
  DumpApplyVector(MakeVec_ShieldRaiseAbsorb());
  DumpApplyVector(MakeVec_ShieldPersistRound1());
  DumpApplyVector(MakeVec_ShieldPersistRound2());
  DumpApplyVector(MakeVec_ShieldClamp255());
  DumpApplyVector(MakeVec_CounterReflects());
  DumpApplyVector(MakeVec_CounterNoneOnRedirect());
  DumpApplyVector(MakeVec_CounterReflectNotCountered());
  DumpApplyVector(MakeVec_CounterReflectAbsorbed());
  DumpApplyVector(MakeVec_CounterReflectsLandedNotDrained());
  DumpApplyVector(MakeVec_CounterReflectKos());
  DumpApplyVector(MakeVec_CounterKilledDoesNotReflect());
  DumpApplyVector(MakeVec_SuddenDeathIgnoresGuardShield());
  // combat-depth Task 5: AoE.
  DumpApplyVector(MakeVec_AoePerVictimClash());
  DumpApplyVector(MakeVec_AoeBlockAndShieldPerVictim());
  DumpApplyVector(MakeVec_AoeIgnoresGuard());
  DumpApplyVector(MakeVec_AoeCounteredPerVictim());
  DumpApplyVector(MakeVec_AoeKillsTwo());
  DumpApplyVector(MakeVec_AoeAllDeadWhiffs());
  DumpApplyVector(MakeVec_AoeCasterFelledMidSpray());
  DumpApplyVector(MakeVec_AoeRejectSlotTarget());
  DumpApplyVector(MakeVec_AoeRejectTargetNone());
  DumpApplyVector(MakeVec_NonAoeRejectTargetAll());
  DumpApplyVector(MakeVec_AoeCanonicalTargetAccepts());
  // combat-depth Task 6: heal, group-heal, siphon, limited uses, double-strikes.
  DumpApplyVector(MakeVec_HealWoundedAlly());
  DumpApplyVector(MakeVec_HealFullHpWasted());
  DumpApplyVector(MakeVec_HealDeadAllyIsNoop());
  DumpApplyVector(MakeVec_HealSelf());
  DumpApplyVector(MakeVec_HealRejectOutOfRange());
  DumpApplyVector(MakeVec_HealRejectTargetAll());
  DumpApplyVector(MakeVec_GroupHeal());
  DumpApplyVector(MakeVec_GroupHealRejectSlotTarget());
  DumpApplyVector(MakeVec_SiphonDrains());
  DumpApplyVector(MakeVec_SiphonOverkillDrainsOnlyWhatWasThere());
  DumpApplyVector(MakeVec_SiphonBlockedHealsLess());
  DumpApplyVector(MakeVec_SiphonFullyShieldedHealsNothing());
  DumpApplyVector(MakeVec_SiphonFizzledHealsNothing());
  DumpApplyVector(MakeVec_SiphonGuardedDrainsGuardian());
  DumpApplyVector(MakeVec_SiphonCannotOverheal());
  DumpApplyVector(MakeVec_SiphonFelledByCounterHealsNothing());
  DumpApplyVector(MakeVec_UsesRound1());
  DumpApplyVector(MakeVec_UsesRound2());
  DumpApplyVector(MakeVec_UsesSpentRejects());
  DumpApplyVector(MakeVec_UsesLastOneAccepts());
  DumpApplyVector(MakeVec_DoubleStrike());
  DumpApplyVector(MakeVec_DoubleStrikeStopsOnKo());
  DumpApplyVector(MakeVec_DoubleStrikeCounteredTwice());
  DumpApplyVector(MakeVec_DoubleStrikeAttackerFelledMidStrike());
  DumpApplyVector(MakeVec_DoubleStrikeGuardLapsesMidStrike());
}

// ================= combat-depth Task 7: `--balance-report` =================
//
// A DEV TOOL, not a test and not engine code: it reads the compile-time move table and
// prints the evidence the balance pass is judged on. It touches no engine entry point, so
// it cannot move a golden vector or a wasm byte.
//
// It answers three questions, in order of importance:
//
//   1. IS ANY MOVE STRICTLY DOMINATED?  A move is dominated when another move of the SAME
//      TYPE and the SAME EFFECT, at the SAME OR LOWER quality, is >= on all three of
//      burst, speed and damage-per-round. A q4 that is dominated by a q1 is the bug this
//      whole task exists to kill ("what's the point doing block?"), so this section must
//      print NONE.
//
//      Why BURST (power x hits) and not raw `power` on the first axis: `power` is
//      PER STRIKE, so a double-strike's 18 is not comparable to a single blow's 25 -- the
//      cast lands 36. For all 38 single-hit moves burst == power and the two readings are
//      identical; on the one multi-hit move burst is the only honest one.
//
//   2. DOES QUALITY BUY DAMAGE?  It must not. dpr = burst / (cooldown + 1) is the
//      textbook throughput of ONE move, but it badly understates a long-cooldown move in a
//      real fight, because a fighter holds FOUR moves and simply plays another one while
//      that one cools. So the kit section below does the honest thing and SIMULATES: for
//      every 4-move subset of a quality's pool it greedily plays the hardest-hitting ready
//      move for 24 rounds, and reports the BEST kit that quality can roll. Those four
//      numbers -- not dpr -- are what "rarity must not buy numbers" actually means, and
//      they are what to keep inside a narrow band.
//
//   3. WHAT VERBS DOES EACH QUALITY GET?  This is what rarity is allowed to buy.
//
// (A fighter only ever rolls moves whose quality equals its own -- the on-chain rule --
// which is what makes "a quality's pool IS its toolkit" true, and what makes a per-quality
// kit simulation meaningful in the first place.)

const char* kTypeNames[5] = {"Heavy", "Speedy", "Tricky", "Distance", "Blocking"};
const char* kEffectNames[9] = {"damage",  "block",  "guard",   "aoe",     "heal",
                                "siphon",  "shield", "counter", "groupheal"};

// power x hits: what ONE cast puts out. For a shield/heal/groupheal this is POINTS
// (granted/restored), not damage -- see the legend the report prints.
uint32_t MoveBurst(uint32_t i) {
  return duel::kDuelMoves[i].power * duel::kDuelMoves[i].hits;
}

// Throughput in TENTHS (integer only -- this binary shares engine.cpp's no-float rule even
// though nothing here is consensus): burst per round if the move is played on cooldown.
uint32_t MoveDprX10(uint32_t i) {
  return MoveBurst(i) * 10u / (duel::kDuelMoves[i].cooldown + 1u);
}

bool EffectDealsDamage(uint8_t eff) {
  return eff == duel::kEffDamage || eff == duel::kEffSiphon || eff == duel::kEffBlock ||
         eff == duel::kEffCounter || eff == duel::kEffAoe;
}

// The damage ONE cast of this move puts on the board. `spread` counts an AoE against a
// full enemy team (aoe_pct of power, on each of team_size foes) instead of ignoring it --
// the two kit metrics below are exactly this switch.
uint32_t KitCastValue(uint32_t i, bool spread) {
  const duel::DuelMoveStat& m = duel::kDuelMoves[i];
  if (m.effect == duel::kEffAoe) {
    if (!spread) return 0;
    return ((m.power * duel::kTun.aoe_pct_256) >> 8) * duel::kTun.team_size;
  }
  if (!EffectDealsDamage(m.effect)) return 0; // a shield/heal/guard turn deals none
  return MoveBurst(i);
}

// Play a 4-move kit for 24 rounds, always casting the hardest-hitting READY move (a move
// is ready when its cooldown is 0 and it has a use left), and return the average damage
// per round in TENTHS. Cooldowns tick exactly as duel_apply ticks them: the cast move is
// SET to its cooldown, every other move decrements.
constexpr uint32_t kKitRounds = 24;
uint32_t SimKit(const uint8_t* kit, uint32_t n, bool spread) {
  uint8_t cd[4] = {0, 0, 0, 0};
  uint8_t uses[4] = {0, 0, 0, 0};
  for (uint32_t j = 0; j < n; ++j) uses[j] = duel::kDuelMoves[kit[j]].max_uses;
  uint32_t total = 0;
  for (uint32_t r = 0; r < kKitRounds; ++r) {
    int best = -1;
    uint32_t bestVal = 0;
    for (uint32_t j = 0; j < n; ++j) {
      if (cd[j] != 0 || uses[j] == 0) continue;
      const uint32_t v = KitCastValue(kit[j], spread);
      if (v > bestVal) {
        bestVal = v;
        best = static_cast<int>(j);
      }
    }
    if (best >= 0) { // nothing worth casting -> the round is a Recover, and cds still tick
      total += bestVal;
      if (uses[best] != 255) --uses[best];
      cd[best] = duel::kDuelMoves[kit[best]].cooldown;
    }
    for (uint32_t j = 0; j < n; ++j) {
      if (static_cast<int>(j) != best && cd[j] > 0) --cd[j];
    }
  }
  return total * 10u / kKitRounds;
}

// The best 4-move kit a fighter of this quality can roll, by brute force over every subset
// of its pool (13 choose 4 = 715 at worst).
void BestKit(uint8_t quality, bool spread, uint32_t* outX10, uint8_t* outKit) {
  uint8_t pool[duel::wire::kMoveUniverse];
  uint32_t n = 0;
  for (uint32_t i = 0; i < duel::wire::kMoveUniverse; ++i) {
    if (duel::kDuelMoveMeta[i].quality == quality) pool[n++] = static_cast<uint8_t>(i);
  }
  *outX10 = 0;
  for (uint32_t a = 0; a < n; ++a) {
    for (uint32_t b = a + 1; b < n; ++b) {
      for (uint32_t c = b + 1; c < n; ++c) {
        for (uint32_t d = c + 1; d < n; ++d) {
          const uint8_t kit[4] = {pool[a], pool[b], pool[c], pool[d]};
          const uint32_t v = SimKit(kit, 4, spread);
          if (v > *outX10) {
            *outX10 = v;
            for (uint32_t j = 0; j < 4; ++j) outKit[j] = kit[j];
          }
        }
      }
    }
  }
}

void PrintKitLine(const char* label, uint8_t quality, bool spread) {
  uint32_t x10 = 0;
  uint8_t kit[4] = {0, 0, 0, 0};
  BestKit(quality, spread, &x10, kit);
  std::printf("  q%u %s %3u.%u/round  [", quality, label, x10 / 10, x10 % 10);
  for (uint32_t j = 0; j < 4; ++j) {
    std::printf("%s%s", j ? ", " : "", duel::kDuelMoveMeta[kit[j]].name);
  }
  std::printf("]\n");
}

void BalanceReport() {
  std::printf("=========================== DUEL BALANCE REPORT ===========================\n");
  std::printf("RARITY BUYS VERBS, NOT NUMBERS. A fighter only rolls moves of its OWN quality,\n");
  std::printf("so a quality's pool IS its toolkit.\n\n");
  std::printf("burst = power x hits (what ONE cast puts out).  dpr = burst / (cooldown+1).\n");
  std::printf("For shield/heal/groupheal the unit is POINTS granted/restored, not damage --\n");
  std::printf("those moves deal none. A guard NEVER reads power (see duel-stats.json _readme):\n");
  std::printf("its numbers are shown as `-`.\n\n");

  // ---- 1. the table, grouped by type then effect then quality ----
  for (uint8_t type = 0; type < 5; ++type) {
    std::printf("---- %s ----\n", kTypeNames[type]);
    std::printf("  %-28s q  eff        pow hits burst spd cd  dpr  uses\n", "move");
    for (uint8_t eff = 0; eff < 9; ++eff) {
      for (uint8_t q = 1; q <= 4; ++q) {
        for (uint32_t i = 0; i < duel::wire::kMoveUniverse; ++i) {
          const duel::DuelMoveStat& m = duel::kDuelMoves[i];
          if (m.type != type || m.effect != eff || duel::kDuelMoveMeta[i].quality != q) continue;
          const bool unusedPower = (m.effect == duel::kEffGuard);
          // 16, not 8: gcc's -Wformat-truncation cannot prove dpr's range from the
          // u32 arithmetic, and a warning is an error in this build.
          char burstBuf[16], dprBuf[16], powBuf[16];
          if (unusedPower) {
            std::snprintf(powBuf, sizeof(powBuf), "%s", "-");
            std::snprintf(burstBuf, sizeof(burstBuf), "%s", "-");
            std::snprintf(dprBuf, sizeof(dprBuf), "%s", "-");
          } else {
            std::snprintf(powBuf, sizeof(powBuf), "%u", m.power);
            std::snprintf(burstBuf, sizeof(burstBuf), "%u", MoveBurst(i));
            const uint32_t d = MoveDprX10(i);
            std::snprintf(dprBuf, sizeof(dprBuf), "%u.%u", d / 10, d % 10);
          }
          char usesBuf[16];
          if (m.max_uses == 255) {
            std::snprintf(usesBuf, sizeof(usesBuf), "%s", "-");
          } else {
            std::snprintf(usesBuf, sizeof(usesBuf), "%u", m.max_uses);
          }
          std::printf("  %-28s q%u %-9s %3s %4u %5s %3u %2u %5s %4s\n",
                      duel::kDuelMoveMeta[i].name, q, kEffectNames[eff], powBuf, m.hits,
                      burstBuf, m.speed, m.cooldown, dprBuf, usesBuf);
        }
      }
    }
    std::printf("\n");
  }

  // ---- 2. THE ACCEPTANCE CRITERION: strictly-dominated moves ----
  std::printf("---- STRICTLY DOMINATED MOVES ----\n");
  std::printf("(same type + same effect, dominator's quality <= the dominated move's,\n");
  std::printf(" and the dominated move is <= on burst AND speed AND dpr -- i.e. a move you\n");
  std::printf(" would never, in any situation, prefer.)\n");
  int dominated = 0;
  for (uint32_t y = 0; y < duel::wire::kMoveUniverse; ++y) {
    const duel::DuelMoveStat& my = duel::kDuelMoves[y];
    if (my.effect == duel::kEffGuard) continue; // power is unused: nothing to compare
    for (uint32_t x = 0; x < duel::wire::kMoveUniverse; ++x) {
      if (x == y) continue;
      const duel::DuelMoveStat& mx = duel::kDuelMoves[x];
      if (mx.type != my.type || mx.effect != my.effect) continue;
      if (duel::kDuelMoveMeta[x].quality > duel::kDuelMoveMeta[y].quality) continue;
      if (MoveBurst(y) <= MoveBurst(x) && my.speed <= mx.speed &&
          MoveDprX10(y) <= MoveDprX10(x)) {
        std::printf("  !! %s (q%u) is DOMINATED by %s (q%u) -- same %s/%s\n",
                    duel::kDuelMoveMeta[y].name, duel::kDuelMoveMeta[y].quality,
                    duel::kDuelMoveMeta[x].name, duel::kDuelMoveMeta[x].quality,
                    kTypeNames[my.type], kEffectNames[my.effect]);
        ++dominated;
      }
    }
  }
  if (dominated == 0) {
    std::printf("  NONE. No move is a strict upgrade of a cheaper one.\n");
  }
  std::printf("\n");

  // ---- 3. does quality buy damage? (the number that actually matters) ----
  std::printf("---- BEST-KIT DAMAGE PER ROUND (greedy, 24 rounds, every 4-move subset) ----\n");
  std::printf("FOCUSED = single-target output only (an AoE counts 0 -- it cannot focus).\n");
  for (uint8_t q = 1; q <= 4; ++q) PrintKitLine("FOCUSED", q, false);
  std::printf("SPREAD  = the same, but an AoE is credited against a full enemy team of %u.\n",
              duel::kTun.team_size);
  for (uint8_t q = 1; q <= 4; ++q) PrintKitLine("SPREAD ", q, true);
  std::printf("\n");

  // ---- 4. what rarity IS allowed to buy ----
  std::printf("---- THE TOOLKIT (the verbs each quality can roll) ----\n");
  for (uint8_t q = 1; q <= 4; ++q) {
    std::printf("  q%u:", q);
    for (uint8_t eff = 0; eff < 9; ++eff) {
      int n = 0;
      for (uint32_t i = 0; i < duel::wire::kMoveUniverse; ++i) {
        if (duel::kDuelMoveMeta[i].quality == q && duel::kDuelMoves[i].effect == eff) ++n;
      }
      if (n > 0) std::printf(" %s x%d", kEffectNames[eff], n);
    }
    std::printf("\n");
  }
  std::printf("\n");
  std::printf("---- HP (damage never scales with quality; hp barely does) ----\n");
  std::printf("  max_hp = %u + (quality-1) x %u + sweetness x %u\n", duel::kTun.hp_base,
              duel::kTun.hp_per_quality, duel::kTun.hp_per_sweetness);
  std::printf("  q1 sw1 = %u ... q4 sw10 = %u (an Epic is +%u hp over a Common at equal sweetness)\n",
              duel::kTun.hp_base + duel::kTun.hp_per_sweetness,
              duel::kTun.hp_base + 3u * duel::kTun.hp_per_quality + 10u * duel::kTun.hp_per_sweetness,
              3u * duel::kTun.hp_per_quality);
  std::printf("  clash pentagon: adv x%u/256 (+%u%%), dis x%u/256 (-%u%%)\n", duel::kTun.adv_mult_256,
              (duel::kTun.adv_mult_256 - 256u) * 100u / 256u, duel::kTun.dis_mult_256,
              (256u - duel::kTun.dis_mult_256) * 100u / 256u);
  std::printf("==========================================================================\n");
}

} // namespace

int main(int argc, char** argv) {
  if (argc > 1 && std::strcmp(argv[1], "--dump-golden") == 0) {
    DumpGolden();
    return 0;
  }
  // combat-depth Task 7: the balance evidence tool (dev-only; see BalanceReport).
  if (argc > 1 && std::strcmp(argv[1], "--balance-report") == 0) {
    BalanceReport();
    return 0;
  }
  // duel_tests_logcap only (build.sh compiles it with -DDUEL_LOG_CAP=320):
  // every OTHER test would correctly fail against a 320-byte log buffer, so
  // this binary runs exactly the one test the shrunk cap exists for.
  if (argc > 1 && std::strcmp(argv[1], "--logcap-only") == 0) {
    RUN(test_apply_log_overflow_rejects);
    std::printf("---\n%d ran, %d failed\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
  }

  RUN(test_duel_init_rejects_null_config);
  RUN(test_duel_init_rejects_zeroed_config);
  RUN(test_jsonout_builds_exact_json);
  RUN(test_jsonout_never_writes_past_cap);
  RUN(test_jsonout_overflow_flag);
  RUN(test_stats_gen_dense_order_matches_blueprint_ends);
  RUN(test_stats_gen_bounds_and_blocking_count);
  RUN(test_stats_gen_effect_partition_and_limits);
  RUN(test_stats_gen_tunables_exact);
  RUN(test_duel_init_valid_3v3_produces_initial_state);
  RUN(test_duel_init_valid_2v2_loadout2_shape);
  RUN(test_duel_init_rejects_invalid_configs);
  RUN(test_duel_init_reject_vector);
  RUN(test_state_codec_roundtrip);
  RUN(test_state_codec_decode_rejects_bad_state);
  RUN(test_decode_rejects_active_state_at_round_cap);
  RUN(test_jsonout_i32_min);
  RUN(test_apply_clash_advantage);
  RUN(test_apply_clash_disadvantage);
  RUN(test_apply_block_stance);
  RUN(test_apply_speed_order_ko_skip);
  RUN(test_apply_fizzle_on_death);
  RUN(test_apply_fizzle_multi_living_untouched);
  RUN(test_apply_turn_order_alternates_by_round);
  RUN(test_apply_recover);
  RUN(test_apply_cooldown_cycle);
  RUN(test_apply_win_then_reject);
  RUN(test_apply_sudden_death_escalates);
  RUN(test_apply_sudden_death_ko);
  RUN(test_apply_sudden_death_terminates_and_mutual_ko_is_a_draw);
  RUN(test_apply_round_cap);
  RUN(test_apply_deterministic);
  RUN(test_apply_guard_intercepts);
  RUN(test_apply_guard_rejects_bad_target);
  RUN(test_apply_guard_lowest_slot_wins_tie);
  RUN(test_apply_guard_dead_guardian_cannot_intercept);
  RUN(test_apply_guard_redirect_never_chains);
  RUN(test_apply_shield_raise_and_absorb);
  RUN(test_apply_shield_persists_between_rounds);
  RUN(test_apply_shield_clamps_at_255);
  RUN(test_apply_counter_reflects);
  RUN(test_apply_counter_none_on_redirected_blow);
  RUN(test_apply_counter_reflect_cannot_be_countered);
  RUN(test_apply_counter_reflect_absorbed_by_shield);
  RUN(test_apply_counter_reflects_landed_not_drained);
  RUN(test_apply_counter_reflect_can_ko);
  RUN(test_apply_counter_killed_does_not_reflect);
  RUN(test_apply_sudden_death_ignores_guard_and_shield);
  RUN(test_apply_aoe_hits_every_living_enemy_per_victim_clash);
  RUN(test_apply_aoe_block_and_shield_per_victim);
  RUN(test_apply_aoe_ignores_guard);
  RUN(test_apply_aoe_triggers_counter_per_victim);
  RUN(test_apply_aoe_kills_two_at_once);
  RUN(test_apply_aoe_whiffs_when_every_enemy_is_dead);
  RUN(test_apply_aoe_caster_felled_mid_spray_stops);
  RUN(test_apply_aoe_target_byte_is_canonical);
  RUN(test_apply_heal_wounded_ally);
  RUN(test_apply_heal_full_hp_is_wasted_not_an_error);
  RUN(test_apply_heal_on_a_dead_ally_is_a_noop_never_a_revive);
  RUN(test_apply_heal_self_is_legal);
  RUN(test_apply_heal_rejects_bad_target);
  RUN(test_apply_group_heal);
  RUN(test_apply_group_heal_requires_target_all);
  RUN(test_apply_siphon_heals_off_drained);
  RUN(test_apply_siphon_overkill_drains_only_what_was_there);
  RUN(test_apply_siphon_blocked_heals_less);
  RUN(test_apply_siphon_fully_shielded_heals_nothing);
  RUN(test_apply_siphon_fizzled_heals_nothing);
  RUN(test_apply_siphon_guarded_drains_the_guardian);
  RUN(test_apply_siphon_cannot_overheal);
  RUN(test_apply_siphon_felled_by_counter_heals_nothing);
  RUN(test_apply_limited_uses);
  RUN(test_apply_double_strike);
  RUN(test_apply_double_strike_stops_on_ko);
  RUN(test_apply_double_strike_is_countered_twice);
  RUN(test_apply_double_strike_attacker_felled_mid_strike);
  RUN(test_apply_double_strike_guard_lapses_mid_strike);
  RUN(test_selfplay_soak);
  RUN(test_fuzz_no_crash);

  std::printf("---\n%d ran, %d failed\n", g_tests_run, g_tests_failed);
  return g_tests_failed == 0 ? 0 : 1;
}

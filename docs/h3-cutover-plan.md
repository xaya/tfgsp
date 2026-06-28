# H3 / Stage 2b — event-driven ongoings cutover (verified plan)

> Built 2026-06-28 from workflow `w23y26mg8` (add-sites/guards/json maps) + first-hand verification of the
> resolver, proto, fork-order, and OngoingsTable API. The capstone of the event-driven-GSP requirement
> ([[event-driven-gsp]]): move ongoing operations OUT of the per-player proto BLOB into the height-keyed
> `ongoing_operations` table so an idle block touches nothing (no per-block O(players) scan/rewrite).

## OngoingsTable API (VERIFIED, database/ongoings.{hpp,cpp})

`CreateNew(unsigned startHeight)->Handle` (ctor stamps proto.StartHeight=startHeight, ongoings.cpp:32).
Handle (OngoingOperation): `SetHeight(unsigned absoluteResolveHeight)`, `GetHeight()`, `SetOwner(string)`,
`GetOwner()`, `GetProto()`, `MutableProto()`, `GetId()`. Table: `GetById(id)`, `QueryAll()` (ORDER BY id),
`QueryForHeight(h)` (WHERE height<=h ORDER BY id), `QueryForOwner(owner)` (WHERE owner=? ORDER BY id),
`CountForOwner(owner)`, `DeleteForHeight(h)` (WHERE height=h EXACT), `DeleteForOwner`, `DeleteById(id)`.
Table is built (Stage 2a) but **DEAD** — no live writer today; all live ongoings live in the player proto.

## Proto refs (proto/ongoing.proto `OngoinOperation`)

BlocksLeft=1, Type=2, RecipeID=3, FighterDatabaseID=4, ExpeditionBlueprintID=5(str), AppliedGoodyKeyName=6(str),
ItemDatabaseID=7(str, UNUSED), RewardID=8, StartHeight=9. OngoingType enum (database/xayaplayer.hpp:44):
INVALID=0, COOK_RECIPE=1, EXPEDITION=2, DECONSTRUCTION=3, COOK_SWEETENER=4.
Player field being vacated: `proto/xaya_player.proto:50 repeated OngoinOperation ongoings = 7;` -> `reserved 7;`.

## Fork-order analysis (CRITICAL — verified first-hand)

- TODAY (logic.cpp TickAndResolveOngoings ~670-753): outer = players `ORDER BY name`, inner = per-player
  proto-array insertion order; decrement all blocksleft each block, resolve those hitting 0.
- AFTER: `QueryForHeight(ctx.Height())` = ALL due rows across ALL players `ORDER BY id` (global creation order).
- ALL 4 Resolve* draw the shared per-block `rnd` (ResolveExpedition NextInt+GenerateActivityReward;
  ResolveSweetener 3×NextInt+GAR; ResolveDeconstruction NextInt candy; ResolveCookingRecepie fighters.CreateNew).
  => when ≥2 players resolve the same block, the resolution ORDER differs (name-grouped vs global-id) => RNG draw
  assignment differs => different rewards => **GOLDEN REGEN**. The new order is fully DETERMINISTIC (ORDER BY id)
  so NO fork on the live chain. Within ONE player, insertion order == id order, so single-player order preserved.
- No Resolve* re-schedules an ongoing (verified) => deleting resolved rows at end-of-tick is safe (new ops get
  height = ctx.Height()+duration ≥ +1 > current h, never caught by DeleteForHeight(current)).

## New resolver (logic.cpp TickAndResolveOngoings)

Materialize-then-resolve (avoid cursor invalidation; deterministic id order):
```
OngoingsTable ongoings(db);
auto res = ongoings.QueryForHeight(ctx.Height());   // height<=now ORDER BY id
std::vector<...> due;  // collect (id, owner, proto) fully first
while (res.Step()) { auto op = ongoings.GetFromResult(res); due.push_back({op->GetId(), op->GetOwner(), op->GetProto()}); }
XayaPlayersTable xp(db);
for (const auto& d : due) {           // already ORDER BY id
  auto a = xp.GetByName(d.owner, ctx.RoConfig());
  switch ((OngoingType)d.proto.type()) { COOK_RECIPE->ResolveCookingRecepie(a, d.proto.recipeid(),...);
    EXPEDITION->ResolveExpedition(a, d.proto.expeditionblueprintid(), d.proto.fighterdatabaseid(),...);
    DECONSTRUCTION->ResolveDeconstruction(a, d.proto.fighterdatabaseid(),...);
    COOK_SWEETENER->ResolveSweetener(a, d.proto.appliedgoodykeyname(), d.proto.fighterdatabaseid(), d.proto.rewardid(),...); }
  a.reset();                          // flush player proto between ops (writes compose)
  ongoings.DeleteById(d.id);
}
```
Drop the blocksleft decrement + the GetOngoingsSize()>0 guard + the erase/break loop entirely. An idle block
now runs one indexed `WHERE height<=now` SELECT that returns 0 rows -> touches nothing (the P-E1 win).

## 4 add-ongoing sites (src/moveprocessor.cpp) — replace add_ongoings() with table row

Each: `auto op = ongoings.CreateNew(ctx.Height()); op->SetOwner(name); op->SetHeight(ctx.Height()+DUR);
op->MutableProto().set_type(...); <ref fields>;` DROP set_blocksleft, DON'T set_startheight (CreateNew does it).
- **MaybeDeconstructFighter ~4355**: DECONSTRUCTION(3); FighterDatabaseID=fighterID; DUR=params().deconstruction_blocks() (no clamp).
- **MaybeGoForExpedition ~4705**: EXPEDITION(2); INSIDE `for(auto& ft: fighter)` => ONE ROW PER FIGHTER;
  FighterDatabaseID=ft, ExpeditionBlueprintID=eid, AppliedGoodyKeyName if non-empty; DUR=clamped(duration,≥1) per iter (local copy!).
- **MaybeCookSweetener ~4792**: COOK_SWEETENER(4); RecipeID=fighterDb->recipeid() (read before reset), AppliedGoodyKeyName=sid, RewardID=rid, FighterDatabaseID=fighterID; DUR=clamped(duration,≥1).
- **MaybeCookRecepie ~4914**: COOK_RECIPE(1); RecipeID=rid, AppliedGoodyKeyName if non-empty; DUR=clamped(duration,≥1). (Remove stray LOG "Settign duration".)

## 6 guard/count sites — proto-ongoings loop -> table query

- **XayaPlayer::GetOngoingsSize() (xayaplayer.hpp:230)**: KEEP as accessor, reimplement
  `return OngoingsTable(db).CountForOwner(name);` (XayaPlayer holds Database& db + name). **This keeps ~40 test
  call sites compiling unchanged.** Move body to xayaplayer.cpp if header include of ongoings.hpp is awkward.
- **logic.cpp:687** `if(GetOngoingsSize()>0)`: DISAPPEARS (whole per-player tick replaced by the new resolver).
- **ParseRewardData ~1494** (expedition-in-progress), **ParseBuyData ~1843** (COOK_RECIPE slot count),
  **ParseDeconstructRewardData ~1904** (decon-finished), **ParseExpeditionData ~2806** (expedition dup):
  replace `for(const auto& o : a.GetProto().ongoings())` with
  `auto res = ongoings.QueryForOwner(a.GetName()); while(res.Step()){ auto op=ongoings.GetFromResult(res); ...op->GetProto()... }`.
  TYPED filters port verbatim. ParseBuyData MUST type-filter COOK_RECIPE (not CountForOwner — that over-counts).

## JSON (src/gamestatejson.cpp) — derive blocksleft, build owner->rows map once

Build ONE `std::map<string, vector<row>>` from `OngoingsTable::QueryAll()` (ORDER BY id => deterministic, matches
old insertion order so FullState hash ordering is stable). Sites: 193 (Convert<FighterInstance> per-fighter
blocksleft), 424/432 (COOK_RECIPE/COOK_SWEETENER recipeid injection into recepies list), 442 (primary 'ongoings'
render — `blocksleft = row.GetHeight()-ctx.Height()` replaces proto.blocksleft()), 672/680 (User() cook recipeid
collection). Add `#include database/ongoings.hpp`. Pure JSON (no RNG) — can't fork, but FullState hash breaks if
order/value diverges. Tournament blocksleft at 206-212 is a separate TournamentInstance field — leave it.

## pending.cpp — NON-consensus, optional cleanup only

Pending builds transient proto::OngoinOperation in PendingState::ongoings (never persisted), ToJson emits
type/ival/sval and NO blocksleft. The 3 set_blocksleft(duration) calls (170,199,245) are DEAD for display ->
drop for DRY (or leave). No consensus change. pending.hpp:193 vector stays (proto type retained as row payload).

## Plumbing BLOCKER

BaseMoveProcessor has NO OngoingsTable member (moveprocessor.hpp ~90-114). Add `OngoingsTable ongoings;` +
init from `db` in the ctor(s) + `#include "database/ongoings.hpp"`. logic.cpp resolver/json instantiate locally.

## Test impact

- Keeping GetOngoingsSize() backed by CountForOwner => the ~40 `EXPECT_EQ(a->GetOngoingsSize(),N)` sites in
  logic_tests/moveprocessor_tests/goldenreplay_tests KEEP WORKING (values may shift with golden regen).
- goldenreplay_tests.cpp 284-299 read GetOngoingsSize() (accessor) — fine. Verify none read `.ongoings()` proto directly.
- **ADD a reorg test**: apply N blocks creating+resolving ongoings -> ProcessBackwards -> assert state
  byte-identical (the SQLite-session undo auto-captures ongoing_operations INSERT/DELETE; this proves it).

## Golden

REGEN expected (resolution-order RNG reassignment when multi-player same-block; also player proto shrinks).
If single-player golden scenario keeps order, may stay byte-identical — follow the regen workflow either way:
GOLDEN_REGEN=1 ./goldenreplay_tests -> docker cp back -> re-run PASS -> run twice byte-identical -> sanity-check
diff (recipe/fighter/reward outcomes equivalent) -> commit golden.json with code.

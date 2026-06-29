# Quality-audit findings — status tracker

See `quality-audit-2026-06-29.md` for plan + baseline. Status: TODO / DONE / DEFERRED / REJECTED.

## Fix-now (71)

| # | Pass | File:Lines | Cat | Sev | Golden | Title | Status |
|---|------|-----------|-----|-----|--------|-------|--------|
| FN1 | A2 | src/logic.cpp:263-289 (also 324-348, 383-407, 555-581, 1275-1301) | money-correctness | medium | regen | Weighted reward roll uses `<=` -> first reward over-weighted, last reward can be unreachable | DEFERRED(user: reward balance) |
| FN2 | A2 | src/logic.cpp:549-573, 1269-1293 | overengineering | low | neutral | fpm::fixed_24_8 used for integer reward weights (ResolveExpedition, ProcessTournaments) | DEFERRED(user: reward balance) |
| FN3 | B | src/logic.cpp:1103-1122 vs 1146-1165 | dry-duplication | low | neutral | Team-collection loop duplicated between Running and Listed branches in ProcessTournaments | DONE |
| FN4 | B | src/logic.hpp:173 | dead-code | low | neutral | EloGetNewRatings() declared but never defined or called | DONE |
| FN5 | B | src/logic.cpp:187-189 | taurion-baggage | low | neutral | Dead fork-gate param: isFork/fork threaded into RecipeInstance::Generate but never used | DONE |
| FN6 | B | src/logic.cpp:547 | dead-code | low | neutral | Unused local `rating` in ResolveExpedition | DONE |
| FN7 | B | src/logic.cpp:1223-1244 | dead-code | low | neutral | Vestigial playersAwarded map / dead cap check in tournament rewards | DONE |
| FN8 | B | src/logic.cpp:1262-1267 | clarity | low | neutral | Unresolved reward-table aborts ALL tournament + queue processing for the block | DONE |
| FN9 | B | src/logic.cpp:1173-1177 | dead-code | low | neutral | Empty else block in ProcessTournaments Listed branch | DONE |
| FN10 | A1 | src/logic.cpp:599,642 | money-correctness | low | neutral | cookCost sentinel -1 can reach AddBalance on out-of-range recipe quality | DONE |
| FN11 | B | src/logic.cpp:742-765 | taurion-baggage | low | neutral | Confusing lmt/rmt swap with leftover scratch comment in ExecuteOneMoveAgainstAnother | DEFERRED(B2) |
| FN12 | B | src/logic.cpp:1625-1637 | clarity | low | neutral | `sortedFighterNames` is never sorted; per-100-block full GetStateAsJson only feeds an RPC display hash | DONE |
| FN13 | A1 | src/moveprocessor.cpp:351 | determinism | medium | neutral | Floating-point literal in consensus reroll-price math (0.14 * COIN) | DONE |
| FN14 | A2 | src/moveprocessor.cpp:1869-1871 | money-correctness | high | regen | Duplicate root.append(resEntry) writes the tournament demand-queue entry twice | DONE |
| FN15 | B | src/moveprocessor.cpp:456-460 | dead-code | low | neutral | Unreachable `if (cmd.isObject())` after `if (!cmd.isString()) return;` (3 copies) | DONE |
| FN16 | B | src/moveprocessor.cpp:1569-1590 | dead-code | medium | neutral | playerHasAtLeastOneEpicTreat computed via O(fighters) scan but never enforced | DONE |
| FN17 | B | src/moveprocessor.cpp:1403-1413 | dead-code | low | neutral | Duplicated identical fighter["o"].isInt() validation block in ParseTransfigureData | DONE |
| FN18 | B | src/moveprocessor.cpp:1466-1477 | dry-duplication | low | neutral | Same O(n^2) dedup-and-reject pattern copy-pasted 5 times | DONE |
| FN19 | B | src/moveprocessor.cpp:1116 | dead-code | low | neutral | Dead `const std::string& name` parameter duplicating a.GetName() in many Parse* helpers | DONE |
| FN20 | B | src/moveprocessor.cpp:587-607 | overengineering | low | neutral | Redundant `const Context& ctx` parameter shadowing the member in GetRecepieObjectFromID/GetCandyKeyNameFromID | DONE |
| FN21 | B | src/moveprocessor.cpp:600-602 | dead-code | low | neutral | Unreachable `break;` after `return candy.first;` | DONE |
| FN22 | B | src/moveprocessor.cpp:376-402 | dry-duplication | low | neutral | ParseCrystalPurchase loops over crystalbundles twice | DONE |
| FN23 | B | src/moveprocessor.cpp:839-844 | taurion-baggage | low | neutral | Vestigial vCHI / coin-transfer comment in ProcessOne | DONE |
| FN24 | B | src/moveprocessor.cpp:249 | clarity | low | neutral | Copy-paste 'Could not solve sweetener entry' log strings in goody parsers | DONE |
| FN25 | A2 | src/moveprocessor.cpp:3474-3477 | money-correctness | high | regen | Transfigure fuel data uses uncommon cook cost for ALL four rarities | DONE |
| FN26 | A2 | src/moveprocessor.cpp:2800-2809 | money-correctness | high | regen | Armor reward update applied to a by-value copy and silently discarded | DONE |
| FN27 | B | src/moveprocessor.cpp:2760-2776 | dead-code | medium | neutral | Unreachable duplicate GeneratedRecipe reward branch | DONE |
| FN28 | A2 | src/moveprocessor.cpp:1869-1871 | dry-duplication | medium | regen | Tournament demand-queue entry appended twice (copy-paste) | DONE |
| FN29 | B | src/pending.cpp:75-112 | dead-code | low | neutral | onChainPlayerFighterData built (incorrectly) but never read | DONE |
| FN30 | B | src/moveprocessor.cpp:2154-2177 | dry-duplication | medium | neutral | Copy-map-to-vector-and-sort + goody-find scaffolding duplicated 5x | DONE |
| FN31 | B | src/moveprocessor.cpp:3292-3299 | dead-code | low | neutral | Candy rarity lookup is dead generality (hard-coded 0.1) | DONE |
| FN32 | B | src/moveprocessor.cpp:3513-3520 | clarity | low | neutral | 296-element name-price lookup table inlined and rebuilt every call | DONE |
| FN33 | B | src/params.hpp:36-56 | dead-code | medium | neutral | Entire Params class is dead (no methods, never read) | DEFERRED(B2) |
| FN34 | C | src/gamestatejson.cpp:260-276 | taurion-baggage | medium | regen | Dead 'activities' JSON emitter for an unpopulated table | TODO |
| FN35 | B | src/jsonutils.cpp:45-53 | dead-code | medium | neutral | CoinAmountFromJson declared/defined but never called | DONE |
| FN36 | B | src/jsonutils.cpp:33-40 | taurion-baggage | low | neutral | Stale vCHI comment on coin/id constants | DONE |
| FN37 | B | src/rest.cpp:22-54 | overengineering | medium | neutral | REST API is dead scaffolding: Process() always 404s, dead flag, dead stop machinery | DONE |
| FN38 | B | src/rest.hpp:65-72 | dead-code | low | neutral | RestClient class never instantiated | DONE |
| FN39 | B | src/gamestatejson.cpp:703-858 | dry-duplication | medium | neutral | UserTournaments duplicates the foreign-fighter collection block 3x | DONE |
| FN40 | A1 | src/gamestatejson.cpp:735-742 | clarity | medium | neutral | UserTournaments state 1/2 branch dereferences fighter handle without null check | DONE |
| FN41 | B | src/gamestatejson.cpp:383-397 | dry-duplication | low | neutral | In-progress cook-recipe id collection duplicated between Convert<XayaPlayer> and User | DONE |
| FN42 | B | src/main.cpp:204-211 | dead-code | low | neutral | Dead local nonAnsiDetected in main() | DONE |
| FN43 | B | src/main.cpp:40-41 | dead-code | low | neutral | Unused <locale>/<codecvt> includes in main.cpp | DONE |
| FN44 | B | src/main.cpp:165-167 | clarity | low | neutral | Version log prints literal string "GIT_VERSION" | DONE |
| FN45 | B | src/main.cpp:55-56 | taurion-baggage | low | neutral | xaya_rpc_url help text still says 'Xaya Core' | DONE |
| FN46 | A1 | database/xayaplayer.tpp:38-42 | clarity | high | neutral | Role-validation bound `val <= 6` contradicts PlayerRole enum (8, 15) -> latent consensus halt | DONE |
| FN47 | B | database/xayaplayer.cpp:143-152, 388-401 | dead-code | medium | neutral | Dead stub methods on XayaPlayer (GetIsMine/SendCHI/HasRole/GrantRole/RevokeRole) | DONE |
| FN48 | B | database/xayaplayer.hpp:84-85 | dead-code | low | neutral | GetPlayerRoleFromColumn template is never used | DONE |
| FN49 | B | database/xayaplayer.hpp:279-280 | dead-code | low | neutral | recipe_slots / roster_slots are write-only dead fields | DONE |
| FN50 | B | database/inventory.cpp:55-117 | dead-code | medium | neutral | QuantityProduct (GMP bignum helper) is dead outside tests; pulls in the whole GMP dependency | DEFERRED(B2) |
| FN51 | B | database/activity.hpp:118-122 | dead-code | low | neutral | IsDirtyActivityData() is never called | DONE |
| FN52 | B | database/recipe.cpp:194 | dead-code | low | neutral | Dead `bool fork` parameter threaded through RecipeInstance::Generate and all call sites | DONE |
| FN53 | B | database/recipe.cpp:256, 284 | dead-code | low | neutral | Dead local `biggetRollSoFar` in RecipeInstance::Generate | DONE |
| FN54 | A1 | database/recipe.cpp:144 | clarity | medium | neutral | UniqueHandles type string mismatch: 'recepie' on load vs 'recipe' on create defeats the duplicate-handle guard | DONE |
| FN55 | A1 | database/fighter.cpp:266-290 | money-correctness | medium | neutral | RerollName: top-of-table payment yields the LOWEST probability bonus (probabilityIndex stays 0) | DONE |
| FN56 | A1 | database/xayaplayer.cpp:131, 138 | clarity | low | neutral | Out-of-sequence SQL bind index ?108 for the inventory column | DONE |
| FN57 | B | database/globaldata.cpp:40-158 | dry-duplication | low | neutral | Four identical SELECT getters and four identical UPDATE setters in GlobalData | DONE |
| FN58 | B | database/globaldata.hpp:22-25 | dead-code | low | neutral | Unused includes (<set>, amount.hpp) and stale 'money supply' class comment in GlobalData | DONE |
| FN59 | B | database/inventory.hpp:1-3 | taurion-baggage | low | neutral | Stale 'Taurion blockchain game' copyright headers and vestigial 'after fork' prestige comments | DONE |
| FN60 | B | database/fighter.cpp:110-131 | overengineering | low | neutral | Animations are re-collected and re-sorted inside the per-move loop in FighterInstance constructor | DONE |
| FN61 | B | database/recipe.cpp:87-138 | dry-duplication | low | neutral | RecipeInstance proto constructor takes CraftedRecipe by value and duplicates the field-copy of the string-GUID constructor | DEFERRED(B2) |
| FN62 | C | proto/config.proto:104-105 | dead-code | low | neutral | Params.prestige_total_treats_mod is never read by any C++ code | TODO |
| FN63 | C | proto/activity.proto:24-48 | taurion-baggage | medium | regen | Activity message / activities table is a fully dead feature (never populated) | TODO |
| FN64 | C | proto/ongoing.proto:44-45 | dead-code | low | neutral | OngoinOperation.ItemDatabaseID (field 7) is completely unused | TODO |
| FN65 | C | proto/tournament_blueprint.proto:47-48 | dead-code | medium | regen | MaxRewardQuality is never applied as a reward cap (dead in both blueprint protos + data tables) | TODO |
| FN66 | C | proto/activity_reward_instance.proto:26 | dead-code | low | regen | Deconstruction.DeconstructionId (field 1) is write-only | TODO |
| FN67 | C | proto/fighter.proto:37-38 | money-correctness | low | neutral | FighterSaleEntry.Price is uint32 while ExchangePrice is uint64 (silent narrowing of sale price) | TODO |
| FN68 | C | proto/config.proto:129-180 | taurion-baggage | low | neutral | Vestigial Taurion comments in ConfigData (vehicles / safe zones / prizes) | TODO |
| FN69 | A1 | src/moveprocessor.cpp:351 | money-correctness | low | neutral | Floating-point money threshold 0.14 * COIN in a consensus path | DONE |
| FN70 | A1 | src/moveprocessor.cpp:3743-3751 | money-correctness | low | neutral | Exchange payout lacks a value-conservation guard (sale_percentage > 1.0 would mint) | DONE |
| FN71 | A1 | src/moveprocessor.cpp:3729-3751 | money-correctness | low | neutral | Seller handle dereferenced without null-check in exchange payout | DONE |

## Deferred (9) — design-level, not in this sweep

| # | File:Lines | Cat | Title |
|---|-----------|-----|-------|
| DEF1 | src/logic.cpp:234-409 (sweetener x3), 515-581 (expedition), 1247-1301 (tournament) | dry-duplication | Reward-table lookup + weighted-roll + GenerateActivityReward block copy-pasted 5 times |
| DEF2 | src/logic.cpp:1380-1422, 1496, 1509 | overengineering | Per-block full FighterTable scans (CheckFightersForSale + SetFreeTransfiguringFighters) violate event-driven design |
| DEF3 | src/logic.cpp:1424-1472, 1085, 1494, 1513 | overengineering | Per-block tournament scans: ReopenMissingTournaments is O(blueprints x tournaments), ProcessTournaments QueryAll scans completed instances |
| DEF4 | src/moveprocessor.cpp:1154-1182 | dry-duplication | Load-fighter + owner + status validation copy-pasted across ~10 Parse* helpers |
| DEF5 | src/pending.cpp:763-931 | dry-duplication | Move-key dispatch table duplicated between pending and Try*Action |
| DEF6 | src/gamestatejson.cpp:187-237 | overengineering | Convert<FighterInstance> runs per-fighter OngoingsTable and RewardsTable scans (N+1) |
| DEF7 | database/activity.cpp:26-155 | dry-duplication | Per-entity table boilerplate is copy-pasted across activity/reward/recipe/fighter/tournament/ongoings |
| DEF8 | database/recipe.cpp:261,289,329-331,340,438-440,452 | determinism | Raw float proto probabilities used directly in consensus RNG-candidate selection math |
| DEF9 | database/fighter.cpp:301,328 | determinism | Raw float FighterName.Probability truncated to int in consensus reroll RNG selection |

## Rejected (2) — refuted in-code

- REJ1 src/moveprocessor.cpp:162-193 — Unbounded sweetener `total` multiplied into cost with no upper cap
- REJ2 database/xayaplayer.cpp:113-117, 169-181 — GetOngoingsSize and CollectTournaments are never called

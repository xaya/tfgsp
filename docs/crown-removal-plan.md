# Crown-holder removal plan (ready to execute, has ONE blocker)

Decision (user, this session): **remove the "crown thing" entirely** — both the
WCHI payment-split and the special-tournament/crown feature. "it was never in the
original treatfighter."

This was investigated with a 4-facet analysis + an adversarial verifier (see the
session transcript / scratchpad `crown-investigation.js`). Summary of why it's
safe and correct to remove, and the one thing that blocks finishing it.

## What the crown-holder payment-split actually is

When a player makes a WCHI-paid move (`pc` crystal bundle, `nr` name reroll), the
payment arrives as `moveObj["out"]` (a map address→amount). `ExtractMoveBasics`
(`src/moveprocessor.cpp` ~71-300) rebuilds the "crown holders" (special-tournament
winners) from `specialTournamentsTbl`, derives a per-tier fraction, and rejects
the move unless every holder was paid its tier share. `TryCrystalPurchase` /
`TryNameRerollPurchase` then sum the map into `paidToDev`, check `paidToDev >=
cost`, mint, and zero the map (the "C8" double-spend guard).

**Key finding: the split has NO economic effect.** The GSP never pays anyone —
`out` is the player's *own* on-chain WCHI transfer that already happened. The
crown machinery is pure input-validation that the player split their payment a
particular way. The only load-bearing check is `paidToDev >= cost`. The tier
machinery is the source of every hazard: CRYS-7 (an addressless holder writes a
guessed address into another player's account during validation AND silently
drops every paid purchase chain-wide), CRYS-2 (one zero entry disables all
validation), the REGTEST-vs-Polygon hardcoded-address divergence, and ~140 lines
of dead post-zeroing deduction loops that only feed a warning log.

## THE BLOCKER (why this wasn't done autonomously)

The split touches the **live WCHI payment representation on Polygon**, which I
could not verify while the user was asleep without risking the highest-stakes
money path.

- On Polygon a paid move is `XayaAccounts.move('p', name, json, nonce, amount,
  receiver)` — a single WCHI `amount` to a single **ETH** `receiver`.
- The crown-split code matches against hardcoded **CHI** (base58 `C…`) addresses
  (`CcHSR3Ss39Ag8pwJ6nsVmEjcdXMhPfoMp3` etc.).
- ETH `out` keys can never equal CHI addresses, so `smallestPayFraction` stays 0,
  validation is skipped, `paidToDev` sums to 0, and `paidToDev < cost` →
  **paid purchases are almost certainly already REJECTED (broken) on Polygon.**
  (Needs live confirmation, but the logic says so.)

**To finish, we need TWO things from the user / live harness:**
1. The **foundation/dev Polygon address** that purchase WCHI should be paid to
   (an ETH `0x…` address), and
2. A **live check of how XayaX keys `moveObj["out"]`** on Polygon (exact string:
   lowercase `0x…`? checksummed? is it even an address-keyed map?). Bring the
   `forked-evm-testing` stack up, send one real WCHI-paid `pc` move, and log the
   `moveObj` the GSP receives.

Until (1)+(2) are known, the replacement validator would be guessing the address
format and could leave the live revenue path broken. Do NOT guess.

## The replacement (once unblocked) — single dev-address model

Add a per-chain config `dev_address` (Params proto + roconfig, regtest override).
Then:

1. `ExtractMoveBasics` (`moveprocessor.hpp:~136` decl; sole callers `ProcessOne`
   `moveprocessor.cpp:~1146` and `pending.cpp:~808`): change the out-param from
   `std::map<std::string,Amount>& paidToCrownHolders` to `Amount& paidToDev`.
   Replace the body's crown block (the `specialTournamentsTbl` loop, the
   addressless-holder candidate-injection incl. the `!readOnly` `set_address`
   write = CRYS-7, the REGTEST/MAIN branches, `smallestPayFraction` = CRYS-2, and
   the tier-fraction validation loop) with:
   `paidToDev = sum of out[dev_address]`.
2. `TryCrystalPurchase` / `TryNameRerollPurchase` (`moveprocessor.hpp:~158/163`):
   take `Amount& paidToDev`. Delete the per-holder re-query + `holderTier` rebuild
   + subtraction loops and the accumulation loops; keep `Parse*` (unchanged — they
   already take `const Amount& paidToDev`), the mint/reroll, and `paidToDev = 0`
   as the C8 guard.
3. `ProcessOne` leftover log: `LOG_IF(WARNING, paidToDev > 0)`.
4. Tests (~28 sites): `moveprocessor_tests.cpp` `ProcessWithDevPayment` (the 35-unit
   `/35`*tier `out` map), `pending_tests.cpp` (the `/28`*7 map), `security_tests.cpp`
   — change to a single `out[dev_address] = paidToDev` entry. Delete
   `TestFractionPaymentRounding` and tier-rejection tests; add a simple
   total>=cost test.
5. Frontend/wallet must send `out` to the one dev address (external; user owns it).

Consensus-safe (verified): no RPC/JSON/REST exposes the split; the `crownholder`
NAME in the anti-fork state hash (`logic.cpp:~2216`) is set only by tournament
lifecycle code, never the payment path. Golden replay / reorg / loadbench have
ZERO paid-move dependency (no regeneration needed for the split change itself).

## Special-tournament FEATURE removal (after the split is decoupled)

The split removal makes `ExtractMoveBasics` stop reading `specialTournamentsTbl`.
Then the special-tournament gameplay can be removed too (the user wants it gone):
- `database/specialtournament.{cpp,hpp}` + the `special_tournaments` schema table.
- Handlers in `moveprocessor.cpp`: `MaybeEnter/LeaveSpecialTournament`,
  `MaybeClaim…`, `TrySpecialTournamentAction`, and the crownholder reassignment
  in `logic.cpp` (~1205) + `ProcessSpecialTournaments` (~894).
- `gamestatejson` special-tournament serialisation + the `crownholder` field.
- The anti-fork state hash inclusion (`logic.cpp:~2216`) — REMOVING this changes
  the state hash; fine on the fresh pre-launch Polygon chain, but regenerate the
  golden and confirm reorg/goldenreplay stay green.
- Genesis seeding of special tournaments / crown holders in `InitialiseState`.
- Tests referencing special tournaments.

This is genesis/state-hash-changing — do it as its own commit, regenerate the
golden, and run the full clean `make check` gate.

## Also pending (small, safe): vestigial proto fields

`burnsale_balance` (`proto/xaya_player.proto`) and `burnsale_stages`
(`proto/config.proto` + `params.pb.text`) are now unreferenced after the burnsale
removal. Removing them needs a roconfig blob regen; left as a follow-up. They are
harmless (unset/unused) but not "clean".

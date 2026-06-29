# Crown-holder removal (DONE)

The "crown" mechanic (a special-tournament feature with a WCHI payment-split
across tournament winners) was never part of the original Treatfighter and has
been removed in full.  Done across three commits on `polygon-rewrite`:

- **A — `refactor(money): replace crown-holder payment-split with a single dev address`**
- **B — `refactor(game): remove the special-tournament / crown-holder feature`**
- **C — `refactor(proto): drop the vestigial burnsale proto fields`**

## Why it was wrong

The crown-holder payment-split had **no economic effect**: the GSP never pays
anyone — `moveObj["out"]` is the player's *own* on-chain WCHI transfer that
already happened.  The tier-split was pure input-validation reconstructed from
`specialTournamentsTbl` on every paid move, and it matched against hardcoded
**CHI base58** addresses (`CcHSR3Ss…`).  On Polygon a paid move pays WCHI to a
single **ETH** `receiver`, so the CHI keys could never match the `out` keys →
`paidToDev` summed to 0 → **paid purchases were rejected (broken) on Polygon.**
It was also the buggiest region (addressless-holder writes, zero-fraction
bypass, ~140 lines of dead post-zeroing math).

## What replaced it — single dev address

A per-config `dev_address` (`Params` field 36 in `proto/config.proto`, value in
`proto/roconfig/params.pb.text`).  `ExtractMoveBasics` now computes
`paidToDev = sum of out[k]` where `AsciiToLower(k) == AsciiToLower(dev_address)`.

**The load-bearing fact** (verified two ways — the deployed `xayax-eth` binary's
exported `ethutils::Address::GetChecksummed` symbol, and `eth-utils/src/address.cpp`):
XayaX's eth connector builds `out` in `ethchain.cpp::GetMoveDataFromLogs` as
`out[receiver.GetChecksummed()] = amount/1e8` — i.e. the key is the **`0x`-prefixed
EIP-55 checksummed** receiver address.  Ethereum addresses are case-insensitive,
so the GSP lowercases both sides; no new dependency, no dependence on XayaX's
exact casing.  `TryCrystalPurchase` / `TryNameRerollPurchase` keep gating on
`paidToDev >= cost` and the C8 single-purchase-per-payment guard (`paidToDev`
passed by reference, zeroed on consumption).

## Verified

- 97 unit + golden + 4 reorg + 2 reorg-game green; clean `make check` from
  distclean (Docker gate).
- Unit `CoinOperationTests.DevPaymentAddressIsCaseInsensitive` covers the
  checksummed-vs-config casing and a non-dev address minting nothing.
- **Live, on a forked Polygon chain through real XayaX** (`forked-evm-testing`,
  `livetest.py`): a real WCHI-paid `pc` move to `dev_address` mints 100 crystals;
  to any other address mints 0.

## Consensus note

The special-tournament removal changes genesis state, the AutoId sequence, the
RNG stream and the anti-fork state hash.  Safe **only** because this is the
fresh pre-launch Polygon relaunch (genesis pinned at height 89'246'000, empty
hash).  Never do this against a committed chain.  Golden was regenerated.

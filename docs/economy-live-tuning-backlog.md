# Economy: audit the crystal prices + make them LIVE-TUNABLE (backlog)

**Status:** owner-requested 2026-07-14, **not for now** — queued behind the duel combat-depth work.
Owner: *"we need to check the crystals prices etc, and also have a parameter like soccerverse we can
change with admin game thing so adjust the prices."*

## The good news: the mechanism already exists

`database/params.hpp` — its own docstring says it outright:

> *Runtime-tunable game parameters, held in the `parameters` KV table (name TEXT PRIMARY KEY → value
> INTEGER). **Mirrors Soccerverse's params table** so the balance knobs can be retuned live via a
> `g/tf` admin `param` move with no rebuild. Every knob is seeded at genesis (`InitialiseDatabase`),
> so `GetParam` CHECK-fails on an unset name — the authoritative value is always the on-chain
> seeded/admin value, never a drifting C++ literal (no silent hard fork).*

It is already proven in production use for the permadeath knobs
(`rarest_recipe_drop_divisor`, `tournament_loss_kills_enabled`, `tournament_capture_pct`,
`tournament_max_captures`) — see `docs/…` and the `g/tf` admin path, which is live-tested on the fork
(raw `{"cmd":…}` envelope; retune + restore observed on-chain).

## The problem: prices are NOT on that lever

Every crystal-denominated price is baked into **`proto/roconfig/params.pb.text`**, which is hashed
into the consensus-pinned roconfig blob. **Changing a price today = a hard fork.** That is exactly
backwards for the numbers most likely to need tuning after launch.

Current prices (crystals):

| Knob | Value | Where |
|---|---|---|
| `exchange_listing_fee` | 10 | `proto/roconfig/params.pb.text:17` |
| `common_recipe_cook_cost` | 15 | `:21` |
| `uncommon_recipe_cook_cost` | 30 | `:23` |
| `rare_recipe_cook_cost` | 60 | `:25` |
| `epic_recipe_cook_cost` | 120 | `:27` |

Also in scope for the audit (crystal **sources** and other sinks):
- **`proto/roconfig/goodybundle.pb.text`** — the WCHI→crystal bundles. This is the *only* faucet, so
  it sets the real-money value of every price above. **The whole economy is calibrated against it.**
- **Tournament `joinCost`** (per blueprint) — see `[[tf-economy-decisions]]`: the agreed direction is
  **tiered + burn** (anti-bot). Not yet done.
- **Sweetener costs**, and the WCHI-gated moves (`pc` purchase-crystals, `nr` name-reroll).

## The work

1. **Audit.** Model the full crystal loop end-to-end: faucet (goody bundles, WCHI in) → sinks (cook,
   listing fee, tournament join, sweeteners). Is it inflationary? Memory records the launch intent —
   *crystals are WCHI-gated and sink-heavy, deliberately NOT inflationary* — **verify that still
   holds against the real numbers.**
2. **Migrate the prices to the `parameters` KV** (the Soccerverse pattern), so they are admin-tunable
   live via `g/tf` with no rebuild and no fork:
   - Seed every price in `InitialiseDatabase` (genesis), exactly like the existing knobs. `GetParam`
     CHECK-fails on an unset name, which is the property that prevents a drifting C++ literal.
   - Read prices through `GameParams::GetParam` at the point of use — **never** from a C++ literal
     and (post-migration) never from roconfig.
   - **Consensus care:** this changes state transitions, so it needs golden-replay regeneration and
     the usual reorg tests. Do it **before mainnet genesis is pinned**, or it becomes a fork.
3. **Guardrails.** An admin `param` move must **bound** each price (a fat-fingered `cook_cost = 0` or
   `= 2^31` is a griefing/economy-destroying vector). Clamp on write, and pin the clamp with a test.
4. **Frontend.** Prices must be *read from chain state*, not hardcoded in `tf-frontend` — otherwise a
   live retune desyncs the UI from the GSP and players get "insufficient crystals" on a price the app
   showed them. **Audit `tf-frontend` for hardcoded costs as part of this.**

## Why it matters

Post-launch, the crystal price curve is the single knob most likely to need adjusting under real
player behaviour. If it needs a hard fork to move, it will never move.

Related: `[[tf-economy-decisions]]`, `docs/launch-punchlist.md`.

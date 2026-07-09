# Treat Fighter — Launch-day runbook (Polygon mainnet)

The ordered procedure for taking the game live. Companion to `docs/launch-punchlist.md`
(what must be true before this runbook runs). Every scripted step below has been
end-to-end tested against the fork/live RPC before launch day; the scripts live in
`contrib/` and the frontend's `scripts/`.

**Owner inputs needed on the day** (nothing else in this runbook needs a human decision):

| # | Input | Used in step |
|---|-------|--------------|
| 1 | The production **dev_address** (foundation wallet that receives all WCHI payments) | 3 |
| 2 | The **custody wallet** that will own `g/tf` (hardware wallet strongly recommended) | 2 |
| 3 | Production **Polygon RPC endpoint(s)** (paid/reliable, for xayax + frontend) | 4-5 |
| 4 | Go/no-go on **permadeath ON at day 1** (current default: ON, capture 50%) and **JoinCost** (current: all free) | 0 |

---

## 0. Preconditions (verify before starting)

- [ ] `docs/launch-punchlist.md` shows no open 🔴 item other than the ones this runbook executes.
- [ ] Both repos pushed + clean: `xaya/tf_polygon` (GSP) and `xaya/treatfighter_ui` (frontend).
- [ ] Contract pins verified against live Polygon (independent of our config):
      `cd tf-frontend && node scripts/verify-contracts.mjs`
      Expect: WCHI symbol/decimals ok, `XayaAccounts.wchiToken() == WCHI`, chainId 137.
- [ ] Product sign-offs recorded: permadeath ON/OFF at genesis (runtime-tunable later via
      `g/tf` admin: `tournament_kills_enabled`, `tournament_capture_pct`, `tournament_max_captures`),
      tournament JoinCost stays 0 at launch (raising it later = roconfig change + redeploy,
      NOT retunable at runtime — decide deliberately).
- [ ] The `tf-builder:local` Docker image builds the GSP green: `contrib/` scripts depend on it.

## 1. Freeze

No further merges to either repo after this point except the two config commits below
(steps 2-3 produce them). Announce the freeze.

## 2. Claim `g/tf` FIRST

The `g/tf` name is the game's admin channel (runtime balance retunes, emergency
`duration_scale_pct`, permadeath knobs). **It must be owned by the custody wallet before
the game is announced** — an attacker who registers it first owns your admin channel.

1. From the custody wallet, register the name `tf` in namespace `g` on the XayaAccounts
   contract (`0x8C12253F71091b9582908C8a44F78870Ec6F304F`): approve WCHI for the
   registration fee, then `register("g", "tf")`.
2. Verify ownership: `XayaAccounts.ownerOf(tokenIdForName("g","tf"))` == custody address.

Note: do NOT send the admin-path test move yet — the genesis is pinned LATER (step 4) at a
height *after* this moment, so a move sent now would land before genesis and never reach the
GSP. The admin path is proven in step 6.5, after the GSP is live and synced.

Custody notes: the name is an ERC-721 — it can be moved to a multisig/hardware wallet at
any time; whoever holds it holds live-balance control of the game. Do not hot-wallet it.

## 3. Set the production dev_address (both repos, lockstep)

```
cd tfgsp-polygon
contrib/set-dev-address.sh 0xPRODUCTION_ADDRESS        # exact EIP-55 checksummed form
```

The script checksum-validates, edits GSP roconfig + frontend config, regenerates the
roconfig blob, re-pins `LaunchConfigIsPinned`, runs the FULL GSP `make check` (golden must
stay byte-identical) + the frontend lockstep test, and commits both repos. Push both.

## 4. Pin genesis (LAST code commit before deploy)

```
cd tfgsp-polygon
contrib/repin-genesis.sh                    # uses tip-256 from a public RPC
# or: contrib/repin-genesis.sh --rpc $PROD_RPC --depth 256
```

Fetches the live Polygon tip, pins `height`/`hash` in `src/logic.cpp`, regenerates the
golden, runs the full `make check`, commits. Push. **Never change the pin after launch.**
(Players cannot act before the game is announced anyway; depth 256 ≈ 9 min of margin
against reorgs of the pinned block.)

## 5. Build + deploy the production stack

1. GSP image (the Dockerfile runs `make check` as a build gate):
   `docker build -f docker/Dockerfile -t tfd-polygon:launch .`
2. xayax connector configured for live Polygon: production RPC endpoint, accounts
   contract `0x8C12…304F`. **Docker networking: pin a non-192.168 subnet** (standing ops
   rule — Docker's default 192.168.0.0/20 grab cuts server access).
3. Start xayax → wait until it tracks the live tip. Start the GSP → it should sync from
   the pinned genesis to the tip in minutes (fresh chain, near-zero backfill).
4. Sync check: `getnullstate` returns `state: "up-to-date"` and the block hash matches a
   block explorer's hash for that height.
5. Frontend production build + serve:
   `npm run build` — the dev-wallet guard is part of the build script itself (refuses
   `NEXT_PUBLIC_DEV_WALLET=1` unless `TF_DEV_BUILD=1` acknowledges a fork TEST build, then
   scans the shipped bundle for burner keys afterwards).
   Serve with `npm run start` behind TLS; point it at the production GSP + RPC.

## 6. Live smoke test (real chain, small money)

From a throwaway wallet with a few WCHI + POL:

1. Register a `p/` test name through the UI; run `a/init` (grants starters).
2. Start the sw1 candy expedition; wait ~15 Polygon blocks; verify candy auto-credits.
3. Buy the T1 crystal bundle (0.14 WCHI) — **verify the WCHI lands at the production
   dev_address** on a block explorer. This is the one check that catches a wrong
   dev_address with real consequences while stakes are still tiny.
4. Enter + resolve the Choc Training tournament vs a second throwaway account.
5. **Prove the admin path with the real custody keys**: from the custody wallet send the
   no-op admin move `{"cmd":{"param":[{"n":"duration_scale_pct","v":100}]}}` on `g/tf`
   (sets the default — changes nothing) and confirm the GSP log shows
   `Processing 1 admin commands` for that block.

## 7. Announce

- Lift the freeze only for post-launch work; the genesis pin and dev_address are now
  immutable-by-policy.
- Watch: GSP log (admin commands, warnings), dev_address balance, exchange listings.
- Emergency levers (via `g/tf`, effective next block, no redeploy):
  `duration_scale_pct` (1-1000), `rarest_recipe_drop_divisor`, `tournament_kills_enabled`,
  `tournament_capture_pct` (0-256), `tournament_max_captures`.

## Rollback reality

- **Before step 7 (announce):** anything can be torn down and redone — nothing real is at
  stake yet; re-pin genesis and start over if needed.
- **After announce:** the chain is the source of truth. There is no rollback — only the
  admin levers above, frontend redeploys, and (worst case) a coordinated new gameid.

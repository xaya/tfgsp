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
- [ ] Both repos pushed + clean: `xaya/tfgsp` branch `polygon-rewrite` (GSP — NEVER merge to main without the owner) and `xaya/treatfighter_ui` (frontend).
- [ ] Contract pins verified against live Polygon (independent of our config):
      `cd tf-frontend && node scripts/verify-contracts.mjs`
      Expect: WCHI symbol/decimals ok, `XayaAccounts.wchiToken() == WCHI`, chainId 137.
- [ ] Product sign-offs recorded: permadeath ON/OFF at genesis (runtime-tunable later via
      `g/tf` admin: `tournament_loss_kills_enabled`, `tournament_capture_pct`, `tournament_max_captures`),
      tournament JoinCost stays 0 at launch (raising it later = roconfig change + redeploy,
      NOT retunable at runtime — decide deliberately).
- [ ] The `tf-builder:local` Docker image builds the GSP green: `contrib/` scripts depend on it.

## 1. Freeze

No further merges to either repo after this point except the two config commits below
(steps 3-4 produce them). Announce the freeze.

## 2. Secure `g/tf` custody FIRST

The `g/tf` name is the game's admin channel (runtime balance retunes, emergency
`duration_scale_pct`, permadeath knobs). **It must be owned by the custody wallet before
the game is announced.** Context: an attacker who registers an unclaimed admin name
first owns the admin channel — that race is already closed here, because `g/tf` is
ALREADY registered on live Polygon (confirmed by the 2026-07-10 audit). The remaining
risk is WHERE it sits, so this step is verify + transfer, not register:

1. Read the current owner on the XayaAccounts contract
   (`0x8C12253F71091b9582908C8a44F78870Ec6F304F`):
   `XayaAccounts.ownerOf(tokenIdForName("g","tf"))`.
2. Confirm that address is the intended custody wallet (owner input #2). If it is
   currently on a hot/dev wallet instead, transfer the name — it is a plain ERC-721:
   `safeTransferFrom(currentOwner, custodyWallet, tokenIdForName("g","tf"))` — then
   re-run the `ownerOf` check and confirm it now returns the custody wallet.

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
   **The GSP MUST run with `restart: unless-stopped` AND the shipped host-cron progress
   watchdog `contrib/gsp-progress-watchdog.sh`** (cron every 2 min; config via
   `GSP_URL`/`XAYAX_URL`/`CONTAINER`/`MAX_LAG`/`STATE_FILE` env or flags — see the script
   header). It `docker restart`s the GSP on EITHER observed wedge:
   - **half-dead RPC** (2026-07-09, fork): libxayagame can abort on a benign
     RPC/block-commit race (`sqlitestorage.cpp:433` `sqlite3_snapshot_open == SQLITE_BUSY`
     during a `getuser`) and the aborting process WEDGES — container Up, RPC gone — which
     no docker restart policy catches;
   - **responsive-but-stalled sync** (2026-07-10, live Polygon, audit P0-02): after missed
     ZMQ notifications the GSP kept answering RPC while frozen `catching-up` ~716k blocks
     behind a healthy XayaX for ~12 days. An RPC-liveness-only watchdog (the old fork
     `gsp-watchdog.sh` pattern) is BLIND to this — hence the watchdog's height-progress
     check and its bounded-lag check against XayaX `getblockchaininfo`.
   Recovery is clean in both cases (the GSP resumes from committed state), so auto-restart
   fully mitigates. The compose file also ships a `getnullstate` healthcheck on `tfd` for
   `docker ps` visibility — remember Docker NEVER restarts a merely-unhealthy container;
   the cron watchdog does the restarting.
4. Sync check: `getnullstate` returns `state: "up-to-date"` and the block hash matches a
   block explorer's hash for that height.
5. Frontend production build + serve:
   `npm run build` — the dev-wallet guard is part of the build script itself (refuses
   `NEXT_PUBLIC_DEV_WALLET=1` unless `TF_DEV_BUILD=1` acknowledges a fork TEST build, then
   scans the shipped bundle for burner keys afterwards).
   Serve with `npm run start` behind TLS; point it at the production GSP + RPC.
   **Set `TRUST_PROXY=1`** whenever a reverse proxy / TLS terminator fronts server.mjs and sets
   `X-Forwarded-For`: the SSE block-fanout's per-IP cap (`GET /events`) buckets on the proxy-set
   client address then. Without the flag behind a proxy, every browser shares the proxy's socket
   IP and the per-IP cap (8) silently acts as a GLOBAL cap; with the flag but NO trusted proxy,
   spoofed XFF headers would bypass the cap. Direct-exposed serve → leave it unset.
   SSE capacity knobs: `SSE_MAX_CLIENTS` (default 5000) bounds total concurrent `/events` streams,
   `SSE_MAX_PER_IP` (default 8) bounds one client's share. Upstream GSP cost is constant regardless
   (one shared `waitforchange`), so raising the cap only spends this process's memory (~9 KB per
   client — 10000 ≈ 90 MB, load-tested flat to 5000). Raise for a larger expected audience.

## 6. Live smoke test (real chain, small money)

From a throwaway wallet with a few WCHI + POL:

1. Register a `p/` test name through the UI; run `a/init` (grants starters).
2. Start the sw1 candy expedition; wait ~15 Polygon blocks; verify candy auto-credits.
3. Buy the T1 crystal bundle (0.14 WCHI) — **verify the WCHI lands at the production
   dev_address** on a block explorer. This is the one check that catches a wrong
   dev_address with real consequences while stakes are still tiny.
4. Enter + resolve the Choc Training tournament vs a second throwaway account.
5. **Prove the admin path with the real custody keys — EVERY emergency lever, once**:
   from the custody wallet send one no-op admin move on `g/tf` that sets each lever to
   its seeded default (changes nothing, but proves every accepted name end-to-end):

   ```
   {"cmd":{"param":[
     {"n":"duration_scale_pct","v":100},
     {"n":"rarest_recipe_drop_divisor","v":4},
     {"n":"tournament_loss_kills_enabled","v":1},
     {"n":"tournament_capture_pct","v":128},
     {"n":"tournament_max_captures","v":3}]}}
   ```

   Then confirm ALL of:
   - the GSP log shows `Processing 1 admin commands` for that block;
   - the log shows NO `Ignoring setparam for unknown name` / `Ignoring out-of-range` /
     `Refusing to remove` warning. An accepted lever logs nothing further; a typo'd name
     is SILENTLY skipped apart from that warning — exactly how the wrong lever name
     `tournament_kills_enabled` sat in this runbook as a documented-but-no-op kill switch
     until the 2026-07-10 audit (P1-05). Never trust the move alone; check the log.
   - the stored values. The levers live in the consensus `parameters` KV table, which is
     NOT exposed through any public RPC, so read it from the data volume on the docker
     host (read-only; safe while the daemon runs):

     ```
     sqlite3 -readonly "$(find "$(docker volume inspect -f '{{.Mountpoint}}' \
         treatfighter_tfd-data)" -name '*.sqlite' | head -1)" \
       'SELECT name, value FROM parameters ORDER BY name;'
     ```

     Expect exactly the five levers at their defaults
     (`duration_scale_pct=100`, `rarest_recipe_drop_divisor=4`,
     `tournament_capture_pct=128`, `tournament_loss_kills_enabled=1`,
     `tournament_max_captures=3`).

## 7. Announce

- Lift the freeze only for post-launch work; the genesis pin and dev_address are now
  immutable-by-policy.
- Watch: GSP log (admin commands, warnings), dev_address balance, exchange listings.
- Emergency levers (via `g/tf`, effective next block, no redeploy):
  `duration_scale_pct` (1-1000), `rarest_recipe_drop_divisor` (1-100000),
  `tournament_loss_kills_enabled` (0/1), `tournament_capture_pct` (0-256),
  `tournament_max_captures` (0-1000). These are the EXACT names accepted by
  `HandleSetParam` (`src/moveprocessor.cpp`) — an unknown name is silently ignored
  (a `LOG(WARNING)` is its only trace), which is how a wrong lever name survived in this
  document until the 2026-07-10 audit (P1-05). All five are proven live in step 6.5.

## Rollback reality

- **Before step 7 (announce):** anything can be torn down and redone — nothing real is at
  stake yet; re-pin genesis and start over if needed.
- **After announce:** the chain is the source of truth. There is no rollback — only the
  admin levers above, frontend redeploys, and (worst case) a coordinated new gameid.

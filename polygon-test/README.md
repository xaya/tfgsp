# Live Polygon integration tests

End-to-end tests that run the **real** Treatfighter stack on a **forked Polygon
mainnet**: Anvil (Foundry) forks Polygon → the real XayaX `eth` bridge →
the real `tfd` GSP → a helper JSON-RPC for sending moves and mining on demand.

This is the modern replacement for the old Xaya-Core (`xayad`) `gametest/*.py`,
which never applied to Treatfighter — Treatfighter runs on Polygon via XayaX,
there is no Xaya Core.

## What it proves

`integration.py` drives the actual production code path
(move → `XayaAccounts` contract → XayaX → ZMQ → `PXLogic` → SQLite → GSP RPC):

- **move round-trip** — a real `g/tf` move is processed and reflected in state;
- **reorg safety** — a move reorged out of Polygon (induced via Anvil
  snapshot/revert) is reverted *exactly* by the GSP's production `BlockDetach`
  path, while pre-reorg state is retained.

These complement the in-process C++ reorg tests (`src/reorg_tests.cpp` and
`src/reorg_game_tests.cpp`), which cover the undo primitive and the
`BlockAttach`/`BlockDetach` machinery without a live chain.

## Bringing up the harness

Uses <https://github.com/xaya/forked-evm-testing> (cloned outside this repo so
its secret-bearing `.env` never lands in git):

1. Build the GSP image (its `make check` gate runs the full C++ suite):

   ```sh
   docker build -f docker/Dockerfile -t tfd-polygon:latest .
   ```

2. Clone the harness next to this repo and configure `.env`:

   ```
   BLOCKCHAIN_ENDPOINT="<archival Polygon RPC, e.g. an Alchemy URL>"
   FORK_BLOCK_NUMBER="89246100"          # genesis+100 -> fast ~100-block sync
   ACCOUNTS_CONTRACT="0x8C12253F71091b9582908C8a44F78870Ec6F304F"
   GSP_IMAGE="tfd-polygon:latest"
   ```

   In the harness `docker-compose.yml`: point the `gsp` service at our image
   (`command: ["tfd","--xaya_rpc_url=http://xayax:8000", ...]`), pin the network
   to a non-192.168 subnet, and bind nginx to a free localhost port (we use
   `127.0.0.1:8133:80`).  An archival RPC is needed because we fork at an old
   block; the genesis (89,246,000) names `domob`/`andy`/`bob` already exist on
   Polygon, so moves are sent via account impersonation (no funds required).

3. `docker compose up -d --build`, then wait for the GSP to sync
   (`getnullstate` → `up-to-date`).

## Running

```sh
python3 polygon-test/integration.py          # default base http://localhost:8133
TF_FORK_BASE=http://localhost:8133 python3 polygon-test/integration.py
```

Exit code 0 = all passed. Stdlib only (no pip deps) — it talks to the harness's
`/gsp`, `/helper` and `/chain` JSON-RPC endpoints.

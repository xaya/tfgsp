# Treat Fighter — Game State Processor (Polygon)

The consensus daemon (`tfd`) for **Treat Fighter**, a candy-fighter strategy game on the
Xaya platform running on **Polygon** via [XayaX](https://github.com/xaya/xayax). Players
cook Treat Fighters from recipes and candy, send them on expeditions, sweeten them, and
battle them in tournaments — all game state is deterministically derived from on-chain
moves by this GSP.

Active development happens on the **`polygon-rewrite`** branch. `main` holds the old
(pre-Polygon) game and is not merged to.

## Architecture

- **Chain:** Polygon mainnet; moves are sent through the
  [XayaAccounts](https://polygonscan.com/address/0x8C12253F71091b9582908C8a44F78870Ec6F304F)
  contract and delivered to the GSP by a XayaX connector.
- **This repo:** the game rules (C++17, libxayagame, SQLite state), a JSON-RPC server for
  clients, and the read-only game configuration (`proto/roconfig`).
- **Client:** a web frontend (Next.js) in a separate repo talks JSON-RPC to this daemon.
- The daemon runs as a Linux Docker service only; there are no desktop/Windows builds.

## Building & testing

The supported build is the Docker one (which also gates on the full test suite):

    docker build -f docker/Dockerfile -t tfd-polygon .

`docker/docker-compose.yml` shows how `tfd` pairs with a XayaX Polygon connector.

For local hacking, the classic autotools flow works inside an environment with
libxayagame installed (e.g. the `xaya/libxayagame` image):

    ./autogen.sh && ./configure && make && make check

`make check` runs the unit suites, the golden-replay regression (byte-exact state
pinning), both reorg suites, and the consensus-determinism lint.

## Repository layout

| Path | Contents |
|---|---|
| `src/` | game logic, move processing, RPC server, golden replay |
| `database/` | SQLite-backed state objects (fighters, recipes, tournaments, …) |
| `proto/`, `proto2/` | protocol buffers + the pinned read-only config blob |
| `docker/` | production image + compose reference |
| `contrib/` | launch tooling (genesis re-pin, dev-address swap, determinism lint) |
| `polygon-test/` | live-chain integration test (against a forked-Polygon harness) |
| `docs/` | design docs, audits, launch punch-list and runbook |

## Launch

See `docs/launch-runbook.md` for the ordered mainnet launch procedure and
`docs/launch-punchlist.md` for what must be true before it runs.

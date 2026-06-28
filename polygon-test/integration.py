#!/usr/bin/env python3
# GSP for the tf blockchain game
# Copyright (C) 2024  Autonomous Worlds Ltd
# SPDX-License-Identifier: GPL-3.0-or-later

"""
End-to-end integration test for the Treatfighter GSP on the REAL Polygon stack.

This is the modern replacement for the dead, Xaya-Core-based gametest/*.py
(Treatfighter runs on Polygon via the XayaX eth bridge -- there is no xayad).

It drives the `xaya/forked-evm-testing` harness, which forks Polygon mainnet
with Anvil, runs the real XayaX `eth` bridge and our real `tfd` GSP against it,
and exposes a helper JSON-RPC for sending moves with impersonated accounts and
mining blocks on demand.  See polygon-test/README.md for how to bring it up.

What it proves, against the actual production code path (move -> XayaAccounts
contract -> XayaX -> ZMQ -> PXLogic -> SQLite -> GSP RPC):

  * MOVE ROUND-TRIP: a real `g/tf` move is processed and reflected in state.
  * REORG SAFETY: when Polygon reorgs out a block (induced via Anvil
    snapshot/revert), XayaX feeds the GSP a BlockDetach and the GSP reverts
    that move's effects EXACTLY, while state from before the reorg is retained.

Endpoints (override via TF_FORK_BASE, default http://localhost:8133):
  /gsp     -- the GSP JSON-RPC (getnullstate, getcurrentstate)
  /helper  -- the harness helper (sendmove, syncgsp, mineblock, ...)
  /chain   -- the Anvil fork (evm_snapshot, evm_revert)

Run (after `docker compose up` in the forked-evm-testing dir):
    python3 polygon-test/integration.py
Exit code 0 = all passed, 1 = a failure.
"""

import json
import os
import sys
import urllib.request

BASE = os.environ.get ("TF_FORK_BASE", "http://localhost:8133")
GSP = BASE + "/gsp"
HELPER = BASE + "/helper"
CHAIN = BASE + "/chain"

# A valid CHI payout address (from the golden scenario); the account NAME is
# what differentiates state, so the same address is fine for every player.
ADDR = "CGUpAcjsb6MDktSYg8yRDxDutr7FhWtdWC"


def rpc (url, method, params=None):
  """Minimal JSON-RPC 2.0 call; raises on transport or RPC error."""
  body = json.dumps ({
    "jsonrpc": "2.0", "id": 1, "method": method, "params": params or [],
  }).encode ()
  req = urllib.request.Request (url, body, {"content-type": "application/json"})
  with urllib.request.urlopen (req, timeout=120) as resp:
    data = json.load (resp)
  if data.get ("error"):
    raise RuntimeError ("%s -> %s" % (method, data["error"]))
  return data.get ("result")


def tf_move (inner):
  """Wraps an inner game move in the on-chain g/tf envelope."""
  return {"g": {"tf": inner}}


def send_init (name):
  """Sends an account-init move for `name` (auto-creates the account)."""
  rpc (HELPER, "sendmove",
       ["p", name, tf_move ({"a": {"init": {"address": ADDR}}})])
  rpc (HELPER, "syncgsp")


def player_names ():
  """Exact set of player names currently in GSP state."""
  st = rpc (GSP, "getcurrentstate")
  xp = st["gamestate"].get ("xayaplayers")
  if isinstance (xp, dict):
    names = list (xp.keys ())
  elif isinstance (xp, list):
    names = [p.get ("name") for p in xp]
  else:
    names = []
  return set (n for n in names if n)


def height ():
  return rpc (GSP, "getnullstate")["height"]


# ---------------------------------------------------------------------------

class Failure (Exception):
  pass


def check (cond, msg):
  if not cond:
    raise Failure (msg)
  print ("    ok: " + msg)


def test_move_round_trip ():
  """A real g/tf move is processed and reflected in GSP state."""
  print ("[ move-round-trip ]")
  ns = rpc (GSP, "getnullstate")
  check (ns["state"] == "up-to-date", "GSP is up-to-date before the move")
  check (ns["chain"] == "polygon", "GSP chain is polygon")
  check (ns["gameid"] == "tf", "GSP gameid is tf")

  before = player_names ()
  name = "andy"  # exists on Polygon -> drivable via impersonation
  send_init (name)
  after = player_names ()

  check (name in after, "player '%s' present after its move" % name)
  st = rpc (GSP, "getcurrentstate")["gamestate"]
  entry = None
  xp = st.get ("xayaplayers")
  for p in (xp if isinstance (xp, list) else []):
    if p.get ("name") == name:
      entry = p
  check (entry is not None and entry.get ("address") == ADDR,
         "the move's effect (address set) is in state")
  check (height () > ns["height"], "GSP advanced past the move block")


def test_reorg_reverts_exactly ():
  """A move reorged out of Polygon is reverted exactly by the GSP."""
  print ("[ reorg-reverts-exactly ]")
  rpc (HELPER, "syncgsp")
  snap = rpc (CHAIN, "evm_snapshot")
  baseline = player_names ()
  name = "bob"
  check (name not in baseline, "'%s' absent at the snapshot point" % name)

  send_init (name)
  check (name in player_names (), "'%s' present after its move" % name)

  # Reorg: discard the move's block, then mine a divergent (empty) block so
  # XayaX sees a different chain at that height and reorgs the GSP.
  check (rpc (CHAIN, "evm_revert", [snap]) is True, "chain reverted to snapshot")
  rpc (HELPER, "mineblock")
  rpc (HELPER, "syncgsp")

  after = player_names ()
  check (name not in after, "'%s' removed by the reorg (BlockDetach)" % name)
  check (baseline.issubset (after),
         "all pre-reorg players retained across the reorg")


def main ():
  tests = [test_move_round_trip, test_reorg_reverts_exactly]
  failed = 0
  for t in tests:
    try:
      t ()
    except Failure as e:
      print ("  FAIL: %s" % e)
      failed += 1
    except Exception as e:  # transport / harness not up
      print ("  ERROR: %s: %s" % (t.__name__, e))
      failed += 1
  print ()
  if failed:
    print ("FAILED: %d/%d test(s)" % (failed, len (tests)))
    return 1
  print ("PASSED: %d/%d tests" % (len (tests), len (tests)))
  return 0


if __name__ == "__main__":
  sys.exit (main ())

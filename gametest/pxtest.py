#   GSP for the tf blockchain game
#   Copyright (C) 2019-2020  Autonomous Worlds Ltd
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <https://www.gnu.org/licenses/>.

from xayagametest.testcase import XayaGameTest

import collections
import copy
import os
import os.path

GAMEID = "tf"

class Account (object):
  """
  Basic handle for an account (Xaya name) in the game state.
  """

  def __init__ (self, data):
    self.data = data

  def getName (self):
    return self.data["name"]

  def getRole (self):
    if "role" in self.data:
      return self.data["role"]
    return None

  def getBalance (self, type="available"):
    return self.data["balance"][type]

class PXTest (XayaGameTest):
  """
  Integration test for the Tauron game daemon.
  """

  cfg = None

  def __init__ (self):
    binary = self.getBuildPath ("src", "tfd")
    super (PXTest, self).__init__ (GAMEID, binary)

  def getBuildPath (self, *parts):
    """
    Returns the builddir (to get the GSP binary and roconfig file).
    It retrieves what should be the top builddir and then adds on the parts
    with os.path.join.
    """

    top = os.getenv ("top_builddir")
    if top is None:
      top = ".."

    return os.path.join (top, *parts)

  def splitPremine (self):
    """
    Splits the premine coin into smaller outputs, so that there are more
    than a single outputs in the wallet.  Some tests require this or else
    name_update's will fail for some reason.
    """

    for i in range (20):
      self.rpc.xaya.sendtoaddress (self.rpc.xaya.getnewaddress (), 100)
    self.generate (1)

  def advanceToHeight (self, targetHeight):
    """
    Mines blocks until we are exactly at the given target height.
    """

    n = targetHeight - self.rpc.xaya.getblockcount ()

    assert n >= 0
    if n > 0:
      self.generate (n)

    self.assertEqual (self.rpc.xaya.getblockcount (), targetHeight)

  def getRpc (self, method, *args, **kwargs):
    """
    Calls the given "read-type" RPC method on the game daemon and returns
    the "data" field (holding the main data).
    """

    return self.getCustomState ("data", method, *args, **kwargs)

  def moveWithPayment (self, name, move, devAmount):
    """
    Sends a move (name_update for the given name) and also includes the
    given payment to the developer address.
    """

    addr = self.roConfig ().params.dev_addr
    return self.sendMove (name, move, {"sendCoins": {addr: devAmount}})

  def initAccount (self, name, address):
    """
    Utility method to initialise an account.
    """

    move = {
      "a":
        {
          "init":
            {
              "address": address,
            },
        },
    }

    return self.sendMove (name, move)
    
  def getAccounts (self):
    """
    Returns all accounts with non-trivial data in the current game state.
    """

    res = {}
    for a in self.getRpc ("getxayaplayers"):
      handle = Account (a)
      nm = handle.getName ()
      assert nm not in res
      res[nm] = handle

    return res
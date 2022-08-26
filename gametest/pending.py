#!/usr/bin/env python3

#   GSP for the Taurion blockchain game
#   Copyright (C) 2019-2021  Autonomous Worlds Ltd
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

"""
Tests the tracking of pending moves.
"""

from pxtest import PXTest

import time


def sleepSome ():
  """
  Sleep for a short amount of time, which should be enough for the pending
  moves to be processed in the GSP.
  """

  time.sleep (0.1)


class PendingTest (PXTest):

  def run (self):
    self.collectPremine ()
    
    # Basic crystal purchase operation test

    self.mainLogger.info ("Creating test account.")
    
    add0 = self.rpc.xaya.getnewaddress ()
    add1 = self.rpc.xaya.getnewaddress ()
    add2 = self.rpc.xaya.getnewaddress ()
    add3 = self.rpc.xaya.getnewaddress ()
    add4 = self.rpc.xaya.getnewaddress ()
    add5 = self.rpc.xaya.getnewaddress ()
    add6 = self.rpc.xaya.getnewaddress ()
    add7 = self.rpc.xaya.getnewaddress ()
    
    self.initAccount ("andy", add0)
    
    self.initAccount ("xayatf1", add1)
    self.initAccount ("xayatf2", add2)
    self.initAccount ("xayatf3", add3)
    self.initAccount ("xayatf4", add4)
    self.initAccount ("xayatf5", add5)
    self.initAccount ("xayatf6", add6)
    self.initAccount ("xayatf7", add7)
    
    self.generate (10)
    self.syncGame ()
    
    fraction = 0.14 / 28
    leftover = 0.14 - (fraction * 28)
    
    amnt1 = fraction * 1
    amnt2 = fraction * 2
    amnt3 = fraction * 3
    amnt4 = fraction * 4
    amnt5 = fraction * 5
    amnt6 = fraction * 6
    amnt7 = fraction * 7 + leftover
    
    self.mainLogger.info (amnt1)
    self.mainLogger.info (amnt2)
    self.mainLogger.info (amnt3)
    self.mainLogger.info (amnt4)
    self.mainLogger.info (amnt5)
    self.mainLogger.info (amnt6)
    self.mainLogger.info (amnt7)      

    self.sendMove ("andy", {"pc": "T1"}, {"sendCoins": {add1: amnt1,add2: amnt2,add3: amnt3,add4: amnt4,add5: amnt5,add6: amnt6,add7: amnt7}})
    
    sleepSome ()
    
    self.assertEqual (self.getPendingState (), {
      "xayaplayers": [{'balance': 350, "crystalbundles": ["T1"], "name": "andy"}]
    })	

    self.generate (1)
    self.syncGame ()
    self.assertEqual (self.getPendingState (), {
      "xayaplayers": []
    })	  


if __name__ == "__main__":
  PendingTest ().main ()
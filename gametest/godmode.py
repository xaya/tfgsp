#!/usr/bin/env python3
# coding=utf8

#   GSP for the Taurion blockchain game
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

"""
Tests creation / initialisation of godmode.
"""

from pxtest import PXTest

import time
import unittest

def sleepSome ():
  """
  Sleep for a short amount of time, which should be enough for the pending
  moves to be processed in the GSP.
  """

  time.sleep (0.1)

class GodModeTest (PXTest):

  def run (self):
    self.collectPremine ()
    self.splitPremine ()
     
    accounts = self.getAccounts ()

    self.mainLogger.info ("Accounts are created with any moves...")
    self.sendMove ("domob", {"x": "foobar"})
    
    self.generate (1)
    self.syncGame ()  

    self.adminCommand ({"god": {"cost": 20000 }})
    self.adminCommand ({"god": {"version": "0.0.95f" }})
    self.adminCommand ({"god": {"vanillaurl": "orvaldmaxwell.io" }})
    
    sleepSome ()
    self.generate (1)
    self.syncGame ()    
    
    currentState = self.getCustomState ("gamestate", "getcurrentstate");
    self.assertEqual (currentState["version"], "0.0.95f")   
    self.assertEqual (currentState["vanillaurl"], "orvaldmaxwell.io") 
    self.assertEqual (currentState["multiplier"], 20000)

    names = ["andy", "evilandy"]
    self.mainLogger.info ("Creating test account.")
    
    add1 = self.rpc.xaya.getnewaddress ()
    add2 = self.rpc.xaya.getnewaddress ()
    add3 = self.rpc.xaya.getnewaddress ()
    add4 = self.rpc.xaya.getnewaddress ()
    add5 = self.rpc.xaya.getnewaddress ()
    add6 = self.rpc.xaya.getnewaddress ()
    add7 = self.rpc.xaya.getnewaddress ()
    
    self.initAccount ("xayatf1", add1)
    self.initAccount ("xayatf2", add2)
    self.initAccount ("xayatf3", add3)
    self.initAccount ("xayatf4", add4)
    self.initAccount ("xayatf5", add5)
    self.initAccount ("xayatf6", add6)
    self.initAccount ("xayatf7", add7)    
    
    for name in names:
        add0 = self.rpc.xaya.getnewaddress ()
        self.initAccount (name, add0)
        self.generate (1)
        self.syncGame ()            

    add_dev = self.rpc.xaya.getnewaddress ()
    
    fraction = 0.14 / 35
    amnt1 = fraction * 1
    amnt2 = fraction * 2
    amnt3 = fraction * 3
    amnt4 = fraction * 4
    amnt5 = fraction * 5
    amnt6 = fraction * 6
    amnt7 = fraction * 7
    amnt8 = fraction * 7
    
    self.sendMove ("andy", {"pc": "T1"}, {"sendCoins": {add1: amnt1,add2: amnt2,add3: amnt3,add4: amnt4,add5: amnt5,add6: amnt6,add7: amnt7, add_dev: amnt8}}) 
    self.assertEqual (self.getPendingState (), {
      "xayaplayers": []
    })	     
 
    self.generate (1)
    self.syncGame ()    
    self.adminCommand ({"god": {"cost": 1000 }})
    sleepSome ()
    self.generate (1)
    self.syncGame ()   

    currentState = self.getCustomState ("gamestate", "getcurrentstate");
    self.assertEqual (currentState["multiplier"], 1000)

    self.sendMove ("andy", {"pc": "T1"}, {"sendCoins": {add1: amnt1,add2: amnt2,add3: amnt3,add4: amnt4,add5: amnt5,add6: amnt6,add7: amnt7, add_dev: amnt8}}) 
    sleepSome ()
    self.assertEqual (self.getPendingState (), {'xayaplayers': [{'balance': 100, 'crystalbundles': ['T1'], 'name': 'andy'}]})	   

if __name__ == "__main__":
  GodModeTest ().main ()
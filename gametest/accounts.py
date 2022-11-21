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
Tests creation / initialisation of accounts.
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

class AccountsTest (PXTest):

  def run (self):
    self.collectPremine ()
     
    accounts = self.getAccounts ()

    self.mainLogger.info ("Accounts are created with any moves...")
    self.sendMove ("domob", {"x": "foobar"})
    self.generate (1)
    accounts = self.getAccounts ()
    assert "andy" not in accounts

    self.generate (1)
    
    #Lets conduct a test for destroy behaviour here
    self.sendMove ("domob", {"ca": {"d": {"rid": [1152]}}})
    sleepSome ()
    self.generate (1)
    self.syncGame ()    
    
    currentState = self.getCustomState ("gamestate", "getcurrentstate");
    
    allRecepies = currentState["recepies"]
    for rp in allRecepies:
        if rp["did"] == 1152:
            self.assertEqual (rp["owner"], "")    

if __name__ == "__main__":
  AccountsTest ().main ()
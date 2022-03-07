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
    self.initAccount ("andy", "p")
    self.generate (1)
    self.syncGame ()
    
    addr = "dHNvNaqcD7XPDnoRjAoyfcMpHRi5upJD7p" # todo: later, lets read this automatically on all platforms self.roConfig ().params.dev_addr
    self.sendMove ("andy", {"pc": "T1"}, {"sendCoins": {addr: 1}})    
    
    sleepSome ()
    
    self.assertEqual (self.getPendingState (), {
      "xayaplayers": [{'balance': 150, "crystalbundles": ["T1"], "name": "andy"}]
    })	

    self.generate (1)
    self.syncGame ()
    self.assertEqual (self.getPendingState (), {
      "xayaplayers": []
    })	  


if __name__ == "__main__":
  PendingTest ().main ()
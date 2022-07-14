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
import unittest

def sleepSome ():
  """
  Sleep for a short amount of time, which should be enough for the pending
  moves to be processed in the GSP.
  """

  time.sleep (0.1)


class SpecialTournamentTest (PXTest):

  def run (self):
    self.collectPremine ()
    self.splitPremine ()

    
    # Prepare the player

    self.mainLogger.info ("Creating test account.")
    self.initAccount ("andy", "p")
    self.generate (1)
    self.syncGame ()
    
    addr = "dHNvNaqcD7XPDnoRjAoyfcMpHRi5upJD7p" # todo: later, lets read this automatically on all platforms self.roConfig ().params.dev_addr
    self.sendMove ("andy", {"pc": "T1"}, {"sendCoins": {addr: 1}})    
    
    sleepSome ()
    
    self.assertEqual (self.getPendingState (), {
      "xayaplayers": [{'balance': 350, "crystalbundles": ["T1"], "name": "andy"}]
    })	

    self.generate (1)
    self.syncGame ()
    self.assertEqual (self.getPendingState (), {
      "xayaplayers": []
    })	  
    
    # Lets go on the expeditions and create treats, until we have 6 in our roster
    # Sanity check first
    currentState = self.getCustomState ("gamestate", "getcurrentstate")
    
    allFighters = currentState["fighters"]
    playerOwnedFighters = 0
    playerOwnedFightersIDS = []
    
    for fghtr in allFighters:
        if fghtr["owner"] == "andy":
            playerOwnedFighters = playerOwnedFighters + 1
            playerOwnedFightersIDS.append(fghtr["id"])
    
    self.assertEqual (playerOwnedFighters, 2)	    
    
    # All good, time to get 4 more fighters, first lets collects recepies and resources
    # Sanity check first, try and join expedition
    
    self.sendMove ("andy", {"exp": {"f": {"eid": "93ad71bb-cd8f-dc24-7885-2c3fd0013245", "fid": playerOwnedFightersIDS}}})
    sleepSome ()
    self.assertEqual (self.getPendingState (), {
    'xayaplayers': [{'name': 'andy', 'ongoings': [{'ival': 1150, 'sval': '93ad71bb-cd8f-dc24-7885-2c3fd0013245', 'type': 2}, {'ival': 1151, 'sval': '93ad71bb-cd8f-dc24-7885-2c3fd0013245', 'type': 2}]}]})	   
    
    self.generate (1)
    self.syncGame ()
    self.assertEqual (self.getPendingState (), {
      "xayaplayers": []
    })	 

    # Ok, all good so far, lets start accumulating resources we need
    saveUs = 0
    
    while saveUs < 2000:
        saveUs = saveUs + 1
        
        if saveUs % 2 == 0:
            self.sendMove ("andy", {"exp": {"f": {"eid": "93ad71bb-cd8f-dc24-7885-2c3fd0013245", "fid": playerOwnedFightersIDS}}})
        if saveUs % 2 != 0:
            self.sendMove ("andy", {"exp": {"f": {"eid": "a2512eaa-028a-1f84-6879-eb240ac80a3e", "fid": playerOwnedFightersIDS}}})
            
        self.generate (16)
        sleepSome ()
        self.syncGame ()
        
        if saveUs % 2 == 0:
            self.sendMove ("andy", {"exp": {"c": {"eid": "93ad71bb-cd8f-dc24-7885-2c3fd0013245"}}})
        if saveUs % 2 != 0:
            self.sendMove ("andy", {"exp": {"c": {"eid": "a2512eaa-028a-1f84-6879-eb240ac80a3e"}}})           
            
        self.generate (1)
        sleepSome ()
        self.syncGame ()        
        currentState = self.getCustomState ("gamestate", "getcurrentstate")
        allFighters = currentState["fighters"]
        allPlayers = currentState["xayaplayers"]
        
        for player in allPlayers:
            if player["name"] == "andy":
                if len(player["recepies"]) >= 4:
                    for rcp in player["recepies"]:
                        self.sendMove ("andy", {"ca": {"r": {"rid": rcp, "fid": 0}}})
                    self.generate (1)
                    sleepSome ()
                    self.syncGame ()
                            
        playerOwnedFightersIDS = []
        playerOwnedFighters2 = 0
        for fghtr in allFighters:
            if fghtr["owner"] == "andy":
                playerOwnedFighters2 = playerOwnedFighters2 + 1
                
                if len(playerOwnedFightersIDS) < 6:
                    playerOwnedFightersIDS.append(fghtr["id"])
                if playerOwnedFighters2 >= 6:
                    saveUs = 2000
                    break;
        
    # Now that we have 6 fightrers, we can join first tier special tournament
    # Lets get tier 1
    
    currentState = self.getCustomState ("gamestate", "getcurrentstate")
    special = currentState["specialtournaments"]
    tier1Id = 0
    
    for sp in special:
        if sp["tier"] == 1:
            tier1Id = sp["id"]
    
    
    self.sendMove ("andy", {"tms": {"e": {"tid": tier1Id, "fc": playerOwnedFightersIDS}}})
    self.generate (1)
    self.syncGame ()
    currentState = self.getCustomState ("gamestate", "getcurrentstate")
    totalPlayerFightersJoinedTheSpecialTournament = 0
    allFighters = currentState["fighters"]
    for fghtr in allFighters:
            if fghtr["owner"] == "andy" and fghtr["specialtournamentinstanceid"] == tier1Id:
                totalPlayerFightersJoinedTheSpecialTournament = totalPlayerFightersJoinedTheSpecialTournament + 1
                
    self.assertEqual(totalPlayerFightersJoinedTheSpecialTournament, 6)
    
    # All good, time to cycle until tournaments is resolved
    
    self.generate (3000)
    self.syncGame ()
    
    currentState = self.getCustomState ("gamestate", "getcurrentstate")
    totalPlayerFightersJoinedTheSpecialTournament = 0
    allFighters = currentState["fighters"]
    for fghtr in allFighters:
            if fghtr["owner"] == "andy" and fghtr["specialtournamentinstanceid"] == tier1Id:
                totalPlayerFightersJoinedTheSpecialTournament = totalPlayerFightersJoinedTheSpecialTournament + 1
                
    self.assertEqual(totalPlayerFightersJoinedTheSpecialTournament, 0)    
    
    #currentState = self.getCustomState ("gamestate", "getcurrentstate")
    #self.assertEqual (currentState, {
      #"xayaplayers": []
    #})
    
    

if __name__ == "__main__":
  SpecialTournamentTest ().main ()
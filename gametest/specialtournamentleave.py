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


# WARNING!!! THIS TEST MIGHT FAIL SOMETIMES, AS ITS A BIT RANDOMIZED 
# AND BUG IN BITCOIN CORE MAKES VERY HARD TO MAKE THIS 100% STABLE

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


class SpecialTournamentLeaveTest (PXTest):

  def run (self):
    self.collectPremine ()
    self.splitPremine ()
    self.splitPremine ()
    self.splitPremine ()
    
    names = ["andy", "evilandy"]
    names2 = ["andy", "evilandy", "xayatf1", "xayatf2", "xayatf3", "xayatf4", "xayatf5", "xayatf6", "xayatf7"]

    self.mainLogger.info ("Creating test account.")
    
    self.generate (1)
    self.syncGame ()        
    
    add1 = self.rpc.xaya.getnewaddress ()
    add2 = self.rpc.xaya.getnewaddress ()
    add3 = self.rpc.xaya.getnewaddress ()
    add4 = self.rpc.xaya.getnewaddress ()
    add5 = self.rpc.xaya.getnewaddress ()
    add6 = self.rpc.xaya.getnewaddress ()
    add7 = self.rpc.xaya.getnewaddress ()   
    
    currentState = self.getCustomState ("gamestate", "getcurrentstate")

    self.initAccount ("xayatf1", add1)
    self.initAccount ("xayatf2", add2)
    self.initAccount ("xayatf3", add3)
    self.initAccount ("xayatf4", add4)
    self.initAccount ("xayatf5", add5)
    self.initAccount ("xayatf6", add6)
    self.initAccount ("xayatf7", add7)    
    
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
    
    for name in names2:
        add0 = self.rpc.xaya.getnewaddress ()     
        if name == "andy" or name == "evilandy":
            self.initAccount (name, add0)
        for x in range(2): 
            self.generate (1)
            self.syncGame ()           
            self.sendMove (name, {"pc": "T1"}, {"sendCoins": {add1: amnt1,add2: amnt2,add3: amnt3,add4: amnt4,add5: amnt5,add6: amnt6,add7: amnt7, add_dev: amnt8}})
            self.generate (1)
            self.syncGame ()           
            self.sendMove (name, {"pc": "T1"}, {"sendCoins": {add1: amnt1,add2: amnt2,add3: amnt3,add4: amnt4,add5: amnt5,add6: amnt6,add7: amnt7, add_dev: amnt8}})
            self.generate (1)
            self.syncGame ()           
            self.sendMove (name, {"pc": "T1"}, {"sendCoins": {add1: amnt1,add2: amnt2,add3: amnt3,add4: amnt4,add5: amnt5,add6: amnt6,add7: amnt7, add_dev: amnt8}})
            self.generate (1)
            self.syncGame ()           
            self.sendMove (name, {"pc": "T1"}, {"sendCoins": {add1: amnt1,add2: amnt2,add3: amnt3,add4: amnt4,add5: amnt5,add6: amnt6,add7: amnt7, add_dev: amnt8}})
            self.generate (1)
            self.syncGame ()   
    
    sleepSome ()
    
    self.generate (1)
    self.syncGame ()
    self.assertEqual (self.getPendingState (), {
      "xayaplayers": []
    })	  
    
    # Lets go on the expeditions and create treats, until we have 6 in our roster
    # Sanity check first
    currentState = self.getCustomState ("gamestate", "getcurrentstate")
    allFighters = currentState["fighters"]
    
    allPlayers = currentState["xayaplayers"]
    for player in allPlayers:
        self.mainLogger.info ("Checking balance for " + player["name"])
        self.assertEqual(player["balance"]["available"], 800)
    
    playerOwnedFighters = {}
    playerOwnedFightersIDS = {}
    
    for name in names:
        playerOwnedFighters[name] = 0
        playerOwnedFightersIDS[name] = []
      
    for fghtr in allFighters:
        for name in names:
            if fghtr["owner"] == name:
                playerOwnedFighters[name] = playerOwnedFighters[name] + 1
                if playerOwnedFighters[name] <= 6:
                    playerOwnedFightersIDS[name].append(fghtr["id"])
            
    for name in names:
        self.assertEqual (playerOwnedFighters[name], 2)
    
    # All good, time to get 4 more fighters, first lets collects recepies and resources
    # Sanity check first, try and join expedition
    
    for name in names:
        self.sendMove (name, {"exp": {"f": {"eid": "93ad71bb-cd8f-dc24-7885-2c3fd0013245", "fid": playerOwnedFightersIDS[name]}}})
    #sleepSome ()
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
            for name in names:
                self.sendMove (name, {"exp": {"f": {"eid": "93ad71bb-cd8f-dc24-7885-2c3fd0013245", "fid": playerOwnedFightersIDS[name]}}})
                
        if saveUs % 2 != 0:
            for name in names:
                self.sendMove (name, {"exp": {"f": {"eid": "a2512eaa-028a-1f84-6879-eb240ac80a3e", "fid": playerOwnedFightersIDS[name]}}})
                               
        self.generate (20)
        sleepSome ()
        self.syncGame ()
        
        if saveUs % 2 == 0:
            for name in names:
                self.sendMove (name, {"exp": {"c": {"eid": "93ad71bb-cd8f-dc24-7885-2c3fd0013245"}}})
        if saveUs % 2 != 0:
            for name in names:
                self.sendMove (name, {"exp": {"c": {"eid": "a2512eaa-028a-1f84-6879-eb240ac80a3e"}}})     
                    
        self.generate (2)
        sleepSome ()
        self.syncGame ()        
        currentState = self.getCustomState ("gamestate", "getcurrentstate")
        allFighters = currentState["fighters"]
        allPlayers = currentState["xayaplayers"]
        
        for player in allPlayers:
            for name in names:
                if player["name"] == name:
                    if len(player["recepies"]) >= 4:
                        for rcp in player["recepies"]:
                            self.sendMove (name, {"ca": {"r": {"rid": rcp, "fid": 0}}})
                                            
        self.generate (20)
        sleepSome ()
        self.syncGame ()
                            
        playerOwnedFighters = {}
        playerOwnedFighters2 = {}
        playerOwnedFightersIDS = {}
        
        for name in names:
            playerOwnedFighters[name] = 0
            playerOwnedFighters2[name] = 0
            playerOwnedFightersIDS[name] = []       
        
        for fghtr in allFighters:
            for name in names:
                if fghtr["owner"] == name:
                    playerOwnedFighters2[name] = playerOwnedFighters2[name] + 1
                    self.sendMove (name, {"ca": {"cl": {"fid": fghtr["id"]}}})                
                    if len(playerOwnedFightersIDS[name]) < 6:
                        playerOwnedFightersIDS[name].append(fghtr["id"])
                        
                    allMoreThenSix = True
                    
                    for name2 in names:
                        if playerOwnedFighters2[name2] < 6:
                            allMoreThenSix = False
                        
                    if allMoreThenSix == True:
                        saveUs = 2000
                        break;
            

        self.generate (1)                        
        sleepSome ()
        self.syncGame () 
        
    # Now that we have 6 fightrers, we can join first tier special tournament
    # Lets get tier 1
    
    playerOwnedFighters2 = {}
    for name in names:
        playerOwnedFighters2[name] = 0
            

    currentState = self.getCustomState ("gamestate", "getcurrentstate")
    allFighters = currentState["fighters"]
    for fghtr in allFighters:
            for name in names:
                if fghtr["owner"] == name:
                    playerOwnedFighters2[name] = playerOwnedFighters2[name] + 1
    
    for name in names:
        valComp = playerOwnedFighters2[name] >= 6
        self.assertEqual (valComp, True)
    
    currentState = self.getCustomState ("gamestate", "getcurrentstate")
    special = currentState["specialtournaments"]
    tier1Id = 0
    
    # Now we need to test leaving, while holding the "crown", so the first thing we need is to
    # actually win the special tournament at all 
    
    saveUs = 0
    
    while saveUs < 2000:
        saveUs = saveUs + 1    
    
        currentState = self.getCustomState ("gamestate", "getcurrentstate")
        allPlayers = currentState["xayaplayers"]
        for player in allPlayers:
            if player["name"] == "andy":
                for sp in special:
                    if sp["tier"] == player["specialtournamentprestigetier"]:
                        tier1Id = sp["id"]    
    
        self.sendMove ("andy", {"tms": {"e": {"tid": tier1Id, "fc": playerOwnedFightersIDS["andy"]}}})
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
        
        self.generate (300)
        self.syncGame ()
        
        currentState = self.getCustomState ("gamestate", "getcurrentstate")
        totalPlayerFightersJoinedTheSpecialTournament = 0
        allFighters = currentState["fighters"]
        for fghtr in allFighters:
                if fghtr["owner"] == "andy" and fghtr["specialtournamentinstanceid"] == tier1Id:
                    totalPlayerFightersJoinedTheSpecialTournament = totalPlayerFightersJoinedTheSpecialTournament + 1
                    
        if totalPlayerFightersJoinedTheSpecialTournament == 6:
            saveUs = 2000
            break;
   
    # So now we are crown holder, hence we want to just leave the sp. tournament be
    
    self.sendMove ("andy", {"tms": {"l": {"tid": tier1Id}}})
    self.generate (1)
    self.syncGame ()    
    
    currentState = self.getCustomState ("gamestate", "getcurrentstate")
    totalPlayerFightersJoinedTheSpecialTournament = 0
    allFighters = currentState["fighters"]
    for fghtr in allFighters:
            if fghtr["owner"] == "andy" and fghtr["specialtournamentinstanceid"] == tier1Id:
                totalPlayerFightersJoinedTheSpecialTournament = totalPlayerFightersJoinedTheSpecialTournament + 1
                
    self.assertEqual(totalPlayerFightersJoinedTheSpecialTournament, 0)    

    comboname = ""
    
    allPlayers = currentState["xayaplayers"]
    for player in allPlayers:  
        if player["name"] == "andy":
            comboname = ("xayatf"+  str(player["specialtournamentprestigetier"]))
    
    # Additionally, lets check, that default player is holding the tournament now
    totalDefaultPlayerFightersJoinedTheSpecialTournament = 0
    for fghtr in allFighters:
            if fghtr["owner"] == comboname and fghtr["specialtournamentinstanceid"] == tier1Id:
                totalDefaultPlayerFightersJoinedTheSpecialTournament = totalDefaultPlayerFightersJoinedTheSpecialTournament + 1  
  
    self.assertEqual(totalDefaultPlayerFightersJoinedTheSpecialTournament, 6)
    
    # Finally, lets confirm, that we can joint same sp. tournament normally again and just compete in it once
    
    self.sendMove ("andy", {"tms": {"e": {"tid": tier1Id, "fc": playerOwnedFightersIDS["andy"]}}})
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
    
    self.generate (300)
    self.syncGame ()    
    
    currentState = self.getCustomState ("gamestate", "getcurrentstate")
    totalPlayerFightersJoinedTheSpecialTournament = 0
    allFighters = currentState["fighters"]
    for fghtr in allFighters:
            if fghtr["owner"] == "andy" and fghtr["specialtournamentinstanceid"] == tier1Id:
                totalPlayerFightersJoinedTheSpecialTournament = totalPlayerFightersJoinedTheSpecialTournament + 1
                
    # Lets check, maybe player andy won and holding crown again now
    special = currentState["specialtournaments"]
    skipLastTest = False
    for sp in special:
        if sp["crownholder"] == "andy":
            skipLastTest = True
        
                
    if skipLastTest == False:
        self.assertEqual(totalPlayerFightersJoinedTheSpecialTournament, 0)    
 
if __name__ == "__main__":
  SpecialTournamentLeaveTest ().main ()
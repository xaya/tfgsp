/*
    GSP for the tf blockchain game
    Copyright (C) 2019-2020  Autonomous Worlds Ltd

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <cmath>
#include <utility>
#include <fstream>

#include <xayautil/hash.hpp>
#include "logic.hpp"
#include "database/reward.hpp"
#include "database/recipe.hpp"
#include "database/ongoings.hpp"
#include "database/globaldata.hpp"

#include "proto/tournament_result.pb.h"

#include "moveprocessor.hpp"

#include "database/schema.hpp"

#include <glog/logging.h>


#include <sstream>

#include <chrono>
#include <thread>

/* Per-block tournament processing and fighter/tournament maintenance scans.
   Split out of logic.cpp; part of the pxd::PXLogic implementation. */

namespace pxd
{

/* Windowed retention cap for Completed tournaments: the per-block prune keeps
   this many most-recent Completed rows and deletes older ones, bounding disk
   while leaving late reward claims (and the frontend results display) working
   for recent tournaments.  */
constexpr unsigned MAX_RETAINED_COMPLETED_TOURNAMENTS = 10000;

void PXLogic::ProcessTournaments(Database& db, const Context& ctx, xaya::Random& rnd)
{
    TournamentTable tournamentsDatabase(db);
    FighterTable fighters(db);

    auto res = tournamentsDatabase.QueryActive ();

    bool tryAndStep = res.Step();
    while (tryAndStep)
    {
      auto tnm = tournamentsDatabase.GetFromResult (res, ctx.RoConfig ());  

      // for every fighter, create a pair with all other fighters who are not on the same team
      
      std::map<std::string, std::vector<uint32_t>> teams;
      std::vector<std::pair<uint32_t,uint32_t>> fighterPairs;
      std::map<std::string, fpm::fixed_24_8> participatingPlayerTotalScore;

      std::map<uint32_t, proto::TournamentResult*> fighterResults;

      // Group the tournament's fighters into per-owner teams (shared verbatim by
      // the Running and Listed branches below).
      auto collectTeams = [&] ()
      {
        for(auto participantFighter : tnm->GetInstance().fighters())
        {
            auto fighter = fighters.GetById (participantFighter, ctx.RoConfig ());
            if (fighter == nullptr) continue;
            std::string owner = fighter->GetOwner();

            if (teams.find(owner) == teams.end())
            {
              std::vector<uint32_t> newTeam;
              newTeam.push_back(participantFighter);

              teams.insert(std::pair<std::string, std::vector<uint32_t>>(owner, newTeam));
            }
            else
            {
              teams[owner].push_back(participantFighter);
            }

            fighter.reset();
        }

        tnm->MutableInstance().set_teamsjoined(teams.size());
      };

      if((pxd::TournamentState)(int32_t)tnm->GetInstance().state() == pxd::TournamentState::Running)
      {
        collectTeams();

        //Lets populate pairs now
        
        for(auto element1 = teams.begin() ; element1 != teams.end() ; ++element1) 
        {
            for(auto element2 = std::next(element1) ; element2 != teams.end() ; ++element2) 
            {                    
                for(int32_t e1 = 0; e1 < (int32_t)element1->second.size(); e1++)
                {
                  for(int32_t e2 = 0; e2 < (int32_t)element2->second.size(); e2++)
                  {
                      std::pair<uint32_t,uint32_t>  newPair= std::make_pair(element1->second[e1], element2->second[e2]);
                      fighterPairs.push_back(newPair);
                  }
                }                      
            }
        }
      }

      if((pxd::TournamentState)(int32_t)tnm->GetInstance().state() == pxd::TournamentState::Listed)
      {
        collectTeams();

        if((int32_t)tnm->GetProto().teamcount() * (int32_t)tnm->GetProto().teamsize() == tnm->GetInstance().fighters_size())
        {
          tnm->MutableInstance().set_state((int32_t)pxd::TournamentState::Running);
          tnm->MutableInstance().set_blocksleft(tnm->GetProto().duration());
        }
      }
      else if((pxd::TournamentState)(int32_t)tnm->GetInstance().state() == pxd::TournamentState::Running)
      {
          if((int32_t)tnm->GetInstance().blocksleft() > 0)
          {
            tnm->MutableInstance().set_blocksleft((int32_t)tnm->GetInstance().blocksleft() - 1);
          }
                        
          if(tnm->GetInstance().blocksleft() == 0)
          {
              for(auto participantFighter : tnm->GetInstance().fighters())
              {
                  proto::TournamentResult* newRslt = tnm->MutableInstance().add_results();
            
                  newRslt->set_fighterid(participantFighter);
                  newRslt->set_wins(0);
                  newRslt->set_draws(0);
                  newRslt->set_losses(0);
                  newRslt->set_ratingdelta(0);
                  
                  fighterResults.insert(std::pair<uint32_t, proto::TournamentResult*>(participantFighter, newRslt));
              }
              
              for(auto fPair: fighterPairs)
              {
                 ProcessFighterPair(fPair.first, fPair.second, fighterResults, participatingPlayerTotalScore, fighters, ctx, rnd);
              }
              
              std::string winnerName = "";
              fpm::fixed_24_8 biggestScoreSoFar = fpm::fixed_24_8(0);
              
              for(const auto& participant: participatingPlayerTotalScore)
              {
                 if(participant.second > biggestScoreSoFar)
                 {
                     winnerName = participant.first;
                     biggestScoreSoFar = participant.second;
                 }
              }
              
              tnm->MutableInstance().set_winnerid(winnerName);
              
              //Rewards creation goes now
              XayaPlayersTable xayaplayers(db);

              for(const auto& participant: participatingPlayerTotalScore)
              {
                  std::string rewardTableId = tnm->GetProto().baserewardstableid();
                  uint32_t rollCount = tnm->GetProto().baserollcount();
                  
                  if(participant.first == winnerName)
                  {
                     rewardTableId = tnm->GetProto().winnerrewardstableid();  
                     rollCount = tnm->GetProto().winnerrollcount();
                  }

                  auto a = xayaplayers.GetByName(participant.first, ctx.RoConfig());
                  pxd::proto::ActivityReward rewardTableDb;
                      
                  const auto& rewardsList = ctx.RoConfig()->activityrewards();
                  bool rewardsSolved = false;
                      
                  for(const auto& rewardsTable: rewardsList)
                  {
                    if(rewardsTable.second.authoredid() == rewardTableId)
                    {
                        rewardTableDb = rewardsTable.second;
                        rewardsSolved = true;
                        break;
                    }
                  }     

                  if(rewardsSolved == false)
                  {
                      /* Skip only this participant's reward on a misconfigured
                         reward-table id; do NOT abandon every remaining tournament
                         + the demand-queue creation by returning from the whole
                         function (which also left this tournament Running forever,
                         re-awarding every block). */
                      LOG (WARNING) << "Could not resolve tournament rewards with authID: " << rewardTableId;
                      continue;
                  }
                                        									
                  uint32_t totalWeight = 0;
                  for(auto& rw: rewardTableDb.rewards())
                  {
                     totalWeight += (uint32_t)rw.weight();
                  }

                  for(uint32_t roll = 0; roll < rollCount; ++roll)
                  {
                      int32_t rolCurNum = 0;

                      if(totalWeight != 0)
                      {
                        rolCurNum = rnd.NextInt((int32_t)totalWeight);
                      }

                      VLOG (1) << "rolCurNum: " << rolCurNum;

                      int32_t accumulatedWeight = 0;
                      int32_t posInTableList = 0;
                      for(auto& rw: rewardTableDb.rewards())
                      {
                          accumulatedWeight += rw.weight();

                          if(rolCurNum < accumulatedWeight)
                          {
                             GenerateActivityReward(0, "", tnm->GetId(), rw, ctx, db, a, rnd, posInTableList, rewardTableId, "");
                             break;
                          }

                          posInTableList++;
                      }
                  }
                  
                  a->MutableProto().set_tournamentscompleted(a->GetProto().tournamentscompleted() + 1);
				  
				  if(participant.first == winnerName)
                  {
					a->MutableProto().set_tournamentswon(a->GetProto().tournamentswon() + 1);
				  }

                  a->CalculatePrestige(ctx.RoConfig());              
                  a.reset();
              }

              for(auto participantFighter : tnm->GetInstance().fighters())
              {
                  auto fighter = fighters.GetById (participantFighter, ctx.RoConfig ());
                  if (fighter == nullptr) continue;
                  fighter->SetStatus(pxd::FighterStatus::Available);
                  fighter->MutableProto().set_tournamentinstanceid(0);
                  fighter->UpdateSweetness();
                  fighter.reset();
              }

              tnm->MutableInstance().set_state((int32_t)pxd::TournamentState::Completed);   

              LOG (INFO) << "Finished processing completed tournament with id: " << tnm->GetId();              
          }
      }

      tnm.reset();
      tryAndStep = res.Step ();
    }
	
	// If we have high demand for tournaments, lets solve it here, or drop it and wait again for other blocks
	
	GlobalData gd(db);
	std::string serializedDataString = gd.GetQueueData();
	  
	if(serializedDataString != "")
	{
		Json::Value root;
		Json::Reader reader;
		reader.parse(serializedDataString, root);
		
		std::map<std::string, int32_t> tournamentDemand;

        for (int32_t i = 0; i < (int32_t)root.size(); i++) 
		{		
            std::string kName = root[i]["tournamentauth"].asString();
			
			if (tournamentDemand.find(kName) == tournamentDemand.end())
			{
				tournamentDemand.insert(std::pair<std::string, int32_t>(kName, 0));
			}
       
            tournamentDemand[kName] += 1;
		}
		
		// Now that we have demand map, we can create tournaments
        for (auto it = tournamentDemand.begin(); it != tournamentDemand.end(); ++it) 
		{
           int32_t tournamentsDemand = it->second;
		   
		   if(tournamentsDemand >= 2)
		   {
			   int32_t totalToPopulate = tournamentsDemand / 2;
			   
			   for (int32_t i2 = 0; i2 < totalToPopulate; i2++) 
			   {
				   tournamentsDatabase.CreateNew(it->first, ctx.RoConfig());
			   }
			   }
        }		

		// Reset
		gd.SetQueueData("");
	}

	/* Windowed retention GC: prune old Completed tournaments down to the cap so
	   the table (and the active-scan exclusion list) stays bounded.  Runs after
	   the demand-queue drain so any tournaments created this block are in place
	   first.  Only the oldest excess beyond the cap is deleted (cheap at steady
	   state); recent Completed rows are kept for late reward claims and the
	   frontend results display.  */
	tournamentsDatabase.PruneCompleted (MAX_RETAINED_COMPLETED_TOURNAMENTS);
}

void PXLogic::CheckFightersForSale(Database& db, const Context& ctx)
{
    FighterTable fighters(db);

    /* DEF2: only Exchange fighters can have an expired listing, so the
       `fighters_by_status` index lets us touch just those rows instead of
       scanning the whole table every block.  Collect the expired ids first, then
       flip them -- never mutate the `status` column while the status-keyed cursor
       is still open.  */
    std::vector<Database::IdT> expired;
    {
      auto res = fighters.QueryForStatus (pxd::FighterStatus::Exchange);
      while (res.Step ())
      {
        auto fighterDb = fighters.GetFromResult (res, ctx.RoConfig ());
        if (fighterDb->GetProto ().exchangeexpire () < ctx.Height ())
          expired.push_back (fighterDb->GetId ());
      }
    }

    for (const auto id : expired)
    {
      auto fighterDb = fighters.GetById (id, ctx.RoConfig ());
      fighterDb->SetStatus (pxd::FighterStatus::Available);
    }
}

void PXLogic::SetFreeTransfiguringFighters(Database& db, const Context& ctx)
{
    FighterTable fighters(db);

    /* DEF2: same indexed pattern -- touch only Transfiguring fighters, and
       collect-then-mutate so the status flip doesn't disturb the status-keyed
       cursor mid-scan.  */
    std::vector<Database::IdT> ids;
    {
      auto res = fighters.QueryForStatus (pxd::FighterStatus::Transfiguring);
      while (res.Step ())
        ids.push_back (fighters.GetFromResult (res, ctx.RoConfig ())->GetId ());
    }

    for (const auto id : ids)
    {
      auto fighterDb = fighters.GetById (id, ctx.RoConfig ());
      fighterDb->SetStatus (pxd::FighterStatus::Available);
    }
}

void PXLogic::ReopenMissingTournaments(Database& db, const Context& ctx)
{
    TournamentTable tournamentsDatabase(db);   
    const auto& tournamentList = ctx.RoConfig()->tournamentbluprints();
    
    std::vector<std::pair<std::string, pxd::proto::TournamentBlueprint>> sortedTournamentListTypesmap;
    for (auto itr = tournamentList.begin(); itr != tournamentList.end(); ++itr)
        sortedTournamentListTypesmap.push_back(*itr);

    sort(sortedTournamentListTypesmap.begin(), sortedTournamentListTypesmap.end(), [=](std::pair<std::string, pxd::proto::TournamentBlueprint>& a, std::pair<std::string, pxd::proto::TournamentBlueprint>& b)
    {
        return a.first < b.first;
    } 
    );      

    for(const auto& tournamentBP: sortedTournamentListTypesmap)
    {
        bool weNeedToCreateNewInstance = true;

        /* DEF2: seek straight to THIS blueprint's active instances (via the
           `tournaments_by_name_state` index) instead of rescanning every active
           tournament once per blueprint.  `name` holds the blueprint authoredid;
           Completed rows are excluded, and the inner checks below still confirm
           authoredid / Listed / open-roster.  */
        auto res = tournamentsDatabase.QueryActiveForBlueprint (tournamentBP.second.authoredid ());

        bool tryAndStep = res.Step();
        while (tryAndStep)
        {
          auto tnm = tournamentsDatabase.GetFromResult (res, ctx.RoConfig ());
          
          if(tnm->GetProto().authoredid() == tournamentBP.second.authoredid())
          {
            if((pxd::TournamentState)(int32_t)tnm->GetInstance().state() == pxd::TournamentState::Listed)
            {
              if((int32_t)tnm->GetInstance().fighters_size() < (int32_t)tournamentBP.second.teamsize() * (int32_t)tournamentBP.second.teamcount())
              {
                weNeedToCreateNewInstance = false;
              }
            }
          }
          
          tnm.reset();
          tryAndStep = res.Step ();
        }
        
        if(weNeedToCreateNewInstance == true)
        {
            tournamentsDatabase.CreateNew(tournamentBP.second.authoredid(), ctx.RoConfig());
        }
    }
}

} // namespace pxd

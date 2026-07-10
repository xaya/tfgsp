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
#include "database/params.hpp"
#include "database/battlelosses.hpp"

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
          tnm->MutableInstance ().set_blocksleft (
              GameParams (db).ScaledDuration (tnm->GetProto ().duration (),
                                              tnm->GetProto ().minsweetness ()));
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

              // Change B (Epic 4x): divisor read once for the whole resolution.
              const uint32_t divisor
                  = (uint32_t) GameParams (db).GetParam ("rarest_recipe_drop_divisor");

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
                                        									
                  for(uint32_t roll = 0; roll < rollCount; ++roll)
                  {
                      const int32_t idx = PickWeightedRewardIndex (rewardTableDb, divisor, rnd);
                      if(idx >= 0)
                        GenerateActivityReward(0, "", tnm->GetId(), rewardTableDb.rewards(idx), ctx, db, a, rnd, (uint32_t)idx, rewardTableId, "");
                  }
                  
                  a->MutableProto().set_tournamentscompleted(a->GetProto().tournamentscompleted() + 1);
				  
				  if(participant.first == winnerName)
                  {
					a->MutableProto().set_tournamentswon(a->GetProto().tournamentswon() + 1);
				  }

                  a->CalculatePrestige(ctx.RoConfig());
                  a.reset();
              }

              /* Change A + C: tournament permadeath + capture (worst-performer,
                 shielded-strongest, capped captures).  Runs only when enabled and
                 there is a winner.  Each NON-winning team (ascending owner-name
                 order via the std::map `teams`) loses exactly ONE fighter -- but
                 NEVER its strongest.  The team's single highest-RATING loseable
                 fighter is shielded; among the rest the victim is the WORST
                 performer this tournament (fewest net wins = wins-losses, from
                 fighterResults), ties broken by ONE random draw.  A lone loseable
                 fighter is still lost.  Account-bound starters are immune and
                 excluded up front; a team of only starters loses no one.  The
                 victim is then a capture_pct/256 coin-flip: transferred to the
                 winner if the winner has a free roster slot AND has captured fewer
                 than tournament_max_captures this tournament, else destroyed.  A
                 battle_losses row is written and the loser's rewards_serial bumped
                 so the client reveals it.  The survivor-reset loop below flips the
                 surviving (and captured) fighters back to Available; destroyed ids
                 return null there and are skipped.  Determinism: teams is owner-name
                 ordered; loseable set, shield (max rating, lowest-id tiebreak) and
                 worst-net set are pure functions of consensus state; one victim
                 tiebreak draw then the capture draw; bumps/writes consume no rnd. */
              {
                GameParams gp(db);
                if (gp.GetParam ("tournament_loss_kills_enabled") == 1
                    && winnerName != "")
                  {
                    const uint32_t capturePct
                        = (uint32_t) gp.GetParam ("tournament_capture_pct");
                    const uint32_t maxCaptures
                        = (uint32_t) gp.GetParam ("tournament_max_captures");
                    const uint32_t maxInv
                        = ctx.RoConfig ()->params ().max_fighter_inventory_amount ();
                    BattleLossesTable battleLosses(db);

                    /* How many fighters the single winner has captured THIS
                       tournament -- capped at tournament_max_captures; further
                       victims are destroyed instead of captured. */
                    uint32_t capturedByWinner = 0;

                    for (const auto& team : teams)
                      {
                        if (team.first == winnerName)
                          continue;   /* the winner's team loses no one */

                        /* Loseable set = entered fighters that are NOT account-bound
                           starters, in deterministic roster order; track the single
                           highest-rating one to shield (ties -> lowest id). */
                        std::vector<uint32_t> loseable;
                        uint32_t shieldId = 0;
                        int64_t shieldRating = -1;
                        for (const uint32_t fid : team.second)
                          {
                            auto f = fighters.GetById (fid, ctx.RoConfig ());
                            if (f == nullptr)
                              continue;
                            const bool bound = f->GetProto ().isaccountbound ();
                            const int64_t rating
                                = (int64_t) f->GetProto ().rating ();
                            f.reset ();
                            if (bound)
                              continue;                 /* starter: immune */
                            loseable.push_back (fid);
                            if (rating > shieldRating
                                || (rating == shieldRating && fid < shieldId))
                              {
                                shieldRating = rating;
                                shieldId = fid;
                              }
                          }

                        if (loseable.empty ())
                          continue;   /* only immune starters -> team loses none */

                        /* Candidate victims = loseable minus the shielded (top-
                           rated) fighter -- unless it is the ONLY loseable fighter
                           (then it is still lost). */
                        std::vector<uint32_t> candidates;
                        candidates.reserve (loseable.size ());
                        for (const uint32_t fid : loseable)
                          if (loseable.size () == 1 || fid != shieldId)
                            candidates.push_back (fid);

                        /* The victim is the WORST performer this tournament among
                           the candidates: fewest net wins (wins - losses), read
                           from fighterResults (populated above by
                           ProcessFighterPair).  Net is computed once per candidate
                           (DRY: one source for the threshold and the tie set), then
                           ties for worst are broken by ONE random draw (constant
                           count -> deterministic RNG stream). */
                        std::vector<std::pair<uint32_t, int32_t>> scored;
                        scored.reserve (candidates.size ());
                        int32_t worstNet = 0;
                        for (const uint32_t fid : candidates)
                          {
                            const auto it = fighterResults.find (fid);
                            const int32_t net = (it != fighterResults.end ())
                                ? (int32_t) it->second->wins ()
                                    - (int32_t) it->second->losses ()
                                : 0;
                            if (scored.empty () || net < worstNet)
                              worstNet = net;
                            scored.emplace_back (fid, net);
                          }
                        std::vector<uint32_t> worst;
                        worst.reserve (scored.size ());
                        for (const auto& sc : scored)
                          if (sc.second == worstNet)
                            worst.push_back (sc.first);

                        const uint32_t victimId = worst[rnd.NextInt (worst.size ())];
                        auto victim = fighters.GetById (victimId, ctx.RoConfig ());
                        if (victim == nullptr)
                          continue;

                        const uint32_t roll = rnd.NextInt (256);
                        const bool rollWon = roll < capturePct;
                        const bool slotFree
                            = fighters.CountForOwner (winnerName) < maxInv;
                        const bool underCap = capturedByWinner < maxCaptures;
                        const bool capture = rollWon && slotFree && underCap;

                        const std::string victimName = victim->GetProto ().name ();

                        /* Loser-side record (outcome 0=destroyed, 1=captured). */
                        battleLosses.Add (team.first, victimId, victimName,
                                          capture ? 1 : 0, winnerName, tnm->GetId ());

                        /* Winner-side notification code (owner = winner) so the
                           winner can reveal what became of the fighter it defeated:
                             2 = captured it;
                             3 = it was DESTROYED because the winner's roster was full;
                             4 = it was DESTROYED because the per-tournament capture
                                 cap (tournament_max_captures) was already reached.
                           The two "miss" codes fire ONLY when a capture was rolled
                           (roll < capture_pct) but blocked -- a plain lost coin-flip
                           is just a destroy the winner is not notified about.  0 =
                           nothing to tell the winner.  When BOTH block, the CAP wins:
                           tournament_max_captures is the binding per-tournament limit
                           (freeing a roster slot would not help), so it is the more
                           accurate reason to surface.  No RNG is consumed here. */
                        uint32_t winnerOutcome = 0;
                        if (capture)                    winnerOutcome = 2;
                        else if (rollWon && !underCap)  winnerOutcome = 4;  /* cap reached */
                        else if (rollWon)               winnerOutcome = 3;  /* under cap, roster full */
                        if (winnerOutcome != 0)
                          battleLosses.Add (winnerName, victimId, victimName,
                                            winnerOutcome, team.first, tnm->GetId ());

                        if (capture)
                          {
                            victim->SetOwner (winnerName);   /* keeps all stats/armor */
                            victim.reset ();                 /* flush owner change */
                            ++capturedByWinner;              /* against the cap */
                          }
                        else
                          {
                            victim.reset ();                 /* release before delete */
                            /* P1-03: a destroy (coin-flip loss, roster-full or
                               capture-cap overflow alike) also drops the victim's
                               retained owner="" source recipe row; a CAPTURE keeps
                               it -- the transferred fighter still references it. */
                            fighters.DeleteWithSourceRecipe (victimId,
                                                             ctx.RoConfig ());
                          }

                        /* Winner-side aggregate update (mirrors the loser bump below):
                           recompute prestige when the roster changed (capture) and
                           bump the winner's reveal serial once if we notified them, so
                           the client surfaces the capture/miss independently of any
                           tournament reward the winner also drew. */
                        if (winnerOutcome != 0)
                          {
                            auto wp = xayaplayers.GetByName (winnerName,
                                                             ctx.RoConfig ());
                            if (wp != nullptr)
                              {
                                if (capture)
                                  wp->CalculatePrestige (ctx.RoConfig ());
                                wp->MutableProto ().set_rewards_serial (
                                    wp->GetProto ().rewards_serial () + 1);
                                wp.reset ();
                              }
                          }

                        auto lp = xayaplayers.GetByName (team.first, ctx.RoConfig ());
                        if (lp != nullptr)
                          {
                            lp->MutableProto ().set_rewards_serial (
                                lp->GetProto ().rewards_serial () + 1);
                            lp->CalculatePrestige (ctx.RoConfig ());
                            lp.reset ();
                          }
                      }
                  }
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

    /* Event-driven expiry (exchange-scale): the (status, expire) index returns ONLY the listings whose
       window has closed, so an idle block does ~zero work here — no scan of the live-listing set.
       Collect ids first, then flip -- never mutate `status` while the status-keyed cursor is open. */
    std::vector<Database::IdT> expired;
    {
      auto res = fighters.QueryExpiredListings (ctx.Height ());
      while (res.Step ())
        expired.push_back (fighters.GetFromResult (res, ctx.RoConfig ())->GetId ());
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

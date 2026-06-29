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

/* Combat + Elo: move match-ups, rating updates and fighter-pair processing.
   Split out of logic.cpp; part of the pxd::PXLogic implementation. */

namespace pxd
{

fpm::fixed_24_8 PXLogic::ExecuteOneMoveAgainstAnother(const Context& ctx, std::string lmv, std::string rmv)
{
    const auto& moveList = ctx.RoConfig()->fightermoveblueprints();
    
    /* leftMoveType = the left move's (lmv) type; rightMoveType = the right
       move's (rmv) type.  The outcome table below is keyed by the right move
       (outer switch) then the left move (inner) and returns lmv's score. */
    fpm::fixed_24_8 rightMoveType = fpm::fixed_24_8(0);
    fpm::fixed_24_8 leftMoveType = fpm::fixed_24_8(0);

    for(auto& moveCandidate: moveList)
    {
        if(moveCandidate.second.authoredid() == lmv)
        {
            leftMoveType = fpm::fixed_24_8(moveCandidate.second.movetype());
            break;
        }
    }
  
    for(auto& moveCandidate: moveList)
    {
        if(moveCandidate.second.authoredid() == rmv)
        {
            rightMoveType = fpm::fixed_24_8(moveCandidate.second.movetype());
            break;
        }
    }  
    
    if(rightMoveType == leftMoveType)
    {
        return fpm::fixed_24_8(0.5);
    }
    
    pxd::MoveType rightMoveTypeM = (pxd::MoveType)(int32_t)rightMoveType;
    pxd::MoveType leftMoveTypeM = (pxd::MoveType)(int32_t)leftMoveType;
    
    switch (rightMoveTypeM)  {
        case pxd::MoveType::Heavy:
             switch (leftMoveTypeM)
             {
                case pxd::MoveType::Speedy:
                    return fpm::fixed_24_8(0);
                case pxd::MoveType::Tricky:
                    return fpm::fixed_24_8(1);
                case pxd::MoveType::Distance:
                    return fpm::fixed_24_8(1);
                case pxd::MoveType::Blocking:
                    return fpm::fixed_24_8(0);
                case pxd::MoveType::Heavy:
                    return fpm::fixed_24_8(0);                    
             }
             return fpm::fixed_24_8(0);
        case pxd::MoveType::Speedy:
             switch (leftMoveTypeM)
             {
                case pxd::MoveType::Heavy:
                    return fpm::fixed_24_8(1);
                case pxd::MoveType::Tricky:
                    return fpm::fixed_24_8(0);
                case pxd::MoveType::Distance:
                    return fpm::fixed_24_8(0);
                case pxd::MoveType::Blocking:
                    return fpm::fixed_24_8(1);
                case pxd::MoveType::Speedy:
                    return fpm::fixed_24_8(1);                    
             }
             return fpm::fixed_24_8(0);
        case pxd::MoveType::Tricky:
             switch (leftMoveTypeM)
             {
                case pxd::MoveType::Heavy:
                    return fpm::fixed_24_8(0);
                case pxd::MoveType::Speedy:
                    return fpm::fixed_24_8(1);
                case pxd::MoveType::Distance:
                    return fpm::fixed_24_8(1);
                case pxd::MoveType::Blocking:
                    return fpm::fixed_24_8(0);
                case pxd::MoveType::Tricky:
                    return fpm::fixed_24_8(0);                    
             }
             return fpm::fixed_24_8(0);   
        case pxd::MoveType::Distance:
             switch (leftMoveTypeM)
             {
                case pxd::MoveType::Heavy:
                    return fpm::fixed_24_8(0);
                case pxd::MoveType::Speedy:
                    return fpm::fixed_24_8(1);
                case pxd::MoveType::Tricky:
                    return fpm::fixed_24_8(0);
                case pxd::MoveType::Blocking:
                    return fpm::fixed_24_8(1);
                case pxd::MoveType::Distance:
                    return fpm::fixed_24_8(1);                    
             }
             return fpm::fixed_24_8(0);   
        case pxd::MoveType::Blocking:
             switch (leftMoveTypeM)
             {
                case pxd::MoveType::Heavy:
                    return fpm::fixed_24_8(1);
                case pxd::MoveType::Speedy:
                    return fpm::fixed_24_8(0);
                case pxd::MoveType::Tricky:
                    return fpm::fixed_24_8(1);
                case pxd::MoveType::Distance:
                    return fpm::fixed_24_8(0);
                case pxd::MoveType::Blocking:
                    return fpm::fixed_24_8(1);                    
             }
             return fpm::fixed_24_8(0);                
        default:
            return fpm::fixed_24_8(0);
    }    
}

 /**
 * FIDE (the World Chess Foundation), gives players with less than 30 played games a K-factor of 25. 
 * Normal players get a K-factor of 15 and pro's get a K-factor of 10.  (Pro = 2400 rating)
 * Once you reach a pro status, you're K-factor never changes, even if your rating drops.
 * 
 * For now set to 15 for everyone.
 */

void PXLogic::CreateEloRating(const Context& ctx, fpm::fixed_24_8& ratingA, fpm::fixed_24_8& ratingB, fpm::fixed_24_8& scoreA, fpm::fixed_24_8& scoreB, fpm::fixed_24_8& expectedA, 
fpm::fixed_24_8& expectedB, fpm::fixed_24_8& newRatingA, fpm::fixed_24_8& newRatingB)
{    
  int32_t KFACTOR = ctx.RoConfig()->params().elok_factor();
  fpm::fixed_24_8 ALMS = fpm::fixed_24_8::from_raw_value(ctx.RoConfig()->params().alms());
  
  fpm::fixed_24_8 val1 = fpm::fixed_24_8(( ratingB - ratingA) / 400);
  fpm::fixed_24_8 val2 = fpm::fixed_24_8(( ratingA - ratingB) / 400);
  
  expectedA = 1 / (1 + (fpm::pow(fpm::fixed_24_8(10), val1)));
  expectedB = 1 / (1 + (fpm::pow(fpm::fixed_24_8(10), val2)));
  
  if (scoreA == fpm::fixed_24_8(0))
  {
      newRatingA = ratingA + (ALMS * (KFACTOR * ( scoreA - expectedA )));
      newRatingB = ratingB + (KFACTOR * ( scoreB - expectedB )); 
  }
  else if (scoreB == fpm::fixed_24_8(0))
  {
      newRatingA = ratingA + (KFACTOR * ( scoreA - expectedA ));
      newRatingB = ratingB + (ALMS  * (KFACTOR * ( scoreB - expectedB )));
  }  
  else
  {
      newRatingA = ratingA + (KFACTOR * ( scoreA - expectedA ));
      newRatingB = ratingB + (KFACTOR * ( scoreB - expectedB ));      
  }
}




void PXLogic::ProcessFighterPair(int64_t fighter1, int64_t fighter2, std::map<uint32_t, proto::TournamentResult*>& fighterResults, std::map<std::string, fpm::fixed_24_8>& participatingPlayerTotalScore, FighterTable& fighters, const Context& ctx, xaya::Random& rnd)
{
   
   
   auto rhs = fighters.GetById (fighter1, ctx.RoConfig ()); 
   auto lhs = fighters.GetById (fighter2, ctx.RoConfig ()); 
   
   std::set<std::string> rhsMovesUnshuffled;
   std::set<std::string> lhsMovesUnshuffled;
   
   for(auto move: rhs->GetProto().moves())
   {
       rhsMovesUnshuffled.insert(move);
   }
   
   for(auto move: lhs->GetProto().moves())
   {
       lhsMovesUnshuffled.insert(move);
   }                 
   
   //Random shuffle goes here
   
   std::vector<std::string> rhsMoves;
   std::vector<std::string> lhsMoves;                 
   
   while(rhsMovesUnshuffled.size() > 0)
   {
      auto iter = rhsMovesUnshuffled.begin();
      std::advance(iter, rnd.NextInt(rhsMovesUnshuffled.size()));
      std::string randomMove = *iter;
      rhsMoves.push_back(randomMove);
      rhsMovesUnshuffled.erase(randomMove);
   }
   
   while(lhsMovesUnshuffled.size() > 0)
   {
      auto iter = lhsMovesUnshuffled.begin();
      std::advance(iter, rnd.NextInt(lhsMovesUnshuffled.size()));
      std::string randomMove = *iter;
      lhsMoves.push_back(randomMove);
      lhsMovesUnshuffled.erase(randomMove);
   }    

   fpm::fixed_24_8 rScore = fpm::fixed_24_8(0);
   fpm::fixed_24_8 lScore = fpm::fixed_24_8(0);
   
   int32_t count = rhsMoves.size();
   
   if(lhsMoves.size() < rhsMoves.size())
   {
       count = lhsMoves.size();
       rScore += (rhsMoves.size() - lhsMoves.size())  * 1;
   }
   else
   {
      lScore += (lhsMoves.size() - rhsMoves.size())  * 1; 
   }
   
   for(int32_t g = 0; g < count; g++)
   {
       std::string rmove = rhsMoves[g];
       std::string lmove = lhsMoves[g];
       
       fpm::fixed_24_8 result = ExecuteOneMoveAgainstAnother(ctx, lmove, rmove);

       rScore += result;
       lScore += 1 - result;
   }
   
   uint32_t winner  = 0;
   if(rScore == lScore)
   {
       winner = 1;
   }
   else if(rScore > lScore)
   {
       winner = 2;
   }

   //Rating and rewards calculation starts now
   
   fpm::fixed_24_8 ratingA = fpm::fixed_24_8(lhs->GetProto().rating());
   fpm::fixed_24_8 ratingB = fpm::fixed_24_8(rhs->GetProto().rating());                 

   pxd::MatchResultType lwin = pxd::MatchResultType::Win;
   pxd::MatchResultType rwin = pxd::MatchResultType::Lose;
   
   fpm::fixed_24_8 scoreA = fpm::fixed_24_8(0);
   fpm::fixed_24_8 scoreB = fpm::fixed_24_8(0);
   
   if (participatingPlayerTotalScore.find(lhs->GetOwner()) == participatingPlayerTotalScore.end())
   {
       participatingPlayerTotalScore.insert(std::pair<std::string, fpm::fixed_24_8>(lhs->GetOwner(), fpm::fixed_24_8(0)));
   }
   
   if (participatingPlayerTotalScore.find(rhs->GetOwner()) == participatingPlayerTotalScore.end())
   {
       participatingPlayerTotalScore.insert(std::pair<std::string, fpm::fixed_24_8>(rhs->GetOwner(), fpm::fixed_24_8(0)));
   }                 
   
   switch (winner)  
   {
      case 0:
          lwin = pxd::MatchResultType::Win;
          rwin = pxd::MatchResultType::Lose;
          participatingPlayerTotalScore[lhs->GetOwner()] += 1;
          scoreA = fpm::fixed_24_8(1);

          fighterResults[fighter1]->set_losses(fighterResults[fighter1]->losses() + 1);
          fighterResults[fighter2]->set_wins(fighterResults[fighter2]->wins() + 1);

          break;
      case 1:
          lwin = pxd::MatchResultType::Draw;
          rwin = pxd::MatchResultType::Draw;
          scoreA = fpm::fixed_24_8(0.5);
          scoreB = fpm::fixed_24_8(0.5);
          participatingPlayerTotalScore[lhs->GetOwner()] += fpm::fixed_24_8(0.5);
          participatingPlayerTotalScore[rhs->GetOwner()] += fpm::fixed_24_8(0.5);

          fighterResults[fighter1]->set_draws(fighterResults[fighter1]->draws() + 1);
          fighterResults[fighter2]->set_draws(fighterResults[fighter2]->draws() + 1);

          break;
      case 2:
          lwin = pxd::MatchResultType::Lose;
          rwin = pxd::MatchResultType::Win;
          scoreB = fpm::fixed_24_8(1);
		  
		  participatingPlayerTotalScore[rhs->GetOwner()] += fpm::fixed_24_8(1);
		          
          fighterResults[fighter2]->set_losses(fighterResults[fighter2]->losses() + 1);
          fighterResults[fighter1]->set_wins(fighterResults[fighter1]->wins() + 1);

          break;
   }                 

   fpm::fixed_24_8 expectedA = fpm::fixed_24_8(0);
   fpm::fixed_24_8 expectedB = fpm::fixed_24_8(0);
   fpm::fixed_24_8 newRatingA = fpm::fixed_24_8(0);
   fpm::fixed_24_8 newRatingB = fpm::fixed_24_8(0);

   CreateEloRating(ctx, ratingA, ratingB, scoreA, scoreB, expectedA, expectedB, newRatingA, newRatingB);

   fpm::fixed_24_8 lRatingDelta = fpm::fixed_24_8(lhs->GetProto().rating());
   fpm::fixed_24_8 rRatingDelta = fpm::fixed_24_8(rhs->GetProto().rating());
   
   uint32_t newRatingForLhs = (uint32_t)std::max(0, (int32_t)newRatingA);
   uint32_t newRatingForRhs = (uint32_t)std::max(0, (int32_t)newRatingB);
   
   lhs->MutableProto().set_rating(newRatingForLhs);
   rhs->MutableProto().set_rating(newRatingForRhs);

   lRatingDelta = lhs->GetProto().rating() - lRatingDelta;
   rRatingDelta = rhs->GetProto().rating() - rRatingDelta;

   lhs->MutableProto().set_totalmatches(lhs->GetProto().totalmatches() + 1);
   rhs->MutableProto().set_totalmatches(rhs->GetProto().totalmatches() + 1);

   if(lwin == pxd::MatchResultType::Win)
   {
     lhs->MutableProto().set_matcheswon(lhs->GetProto().matcheswon() + 1);
   }
   if(lwin == pxd::MatchResultType::Lose)
   {
     lhs->MutableProto().set_matcheslost(lhs->GetProto().matcheslost() + 1);
   }

   if(rwin == pxd::MatchResultType::Win)
   {
     rhs->MutableProto().set_matcheswon(rhs->GetProto().matcheswon() + 1);
   }
   if(rwin == pxd::MatchResultType::Lose)
   {
     rhs->MutableProto().set_matcheslost(rhs->GetProto().matcheslost() + 1);
   }

   fighterResults[fighter2]->set_ratingdelta(fighterResults[fighter2]->ratingdelta() + (int32_t)lRatingDelta);
   fighterResults[fighter1]->set_ratingdelta(fighterResults[fighter1]->ratingdelta() + (int32_t)rRatingDelta);

   rhs.reset();
   lhs.reset();    
}

} // namespace pxd

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

#include "logic.hpp"
#include "database/reward.hpp"

#include "moveprocessor.hpp"

#include "database/schema.hpp"

#include <glog/logging.h>

namespace pxd
{

SQLiteGameDatabase::SQLiteGameDatabase (xaya::SQLiteDatabase& d, PXLogic& g)
  : game(g)
{
  SetDatabase (d);
}

Database::IdT
SQLiteGameDatabase::GetNextId ()
{
  return game.Ids ("pxd").GetNext ();
}

Database::IdT
SQLiteGameDatabase::GetLogId ()
{
  return game.Ids ("log").GetNext ();
}

void
PXLogic::UpdateState (Database& db, xaya::Random& rnd,
                      const xaya::Chain chain,
                      const Json::Value& blockData)
{
  const auto& blockMeta = blockData["block"];
  CHECK (blockMeta.isObject ());
  const auto& heightVal = blockMeta["height"];
  CHECK (heightVal.isUInt64 ());
  const unsigned height = heightVal.asUInt64 ();
  const auto& timestampVal = blockMeta["timestamp"];
  CHECK (timestampVal.isInt64 ());
  const int64_t timestamp = timestampVal.asInt64 ();

  Context ctx(chain, height, timestamp);

  UpdateState (db, rnd, ctx, blockData);
}

std::vector<uint32_t> PXLogic::GenerateActivityReward(const uint32_t fighterID, const std::string blueprintAuthID, const uint32_t tournamentID, const pxd::proto::AuthoredActivityReward rw, const Context& ctx, Database& db, std::unique_ptr<XayaPlayer>& a, xaya::Random& rnd, const uint32_t posInTableList, const std::string basedRewardsTableAuthId)
{
  RecipeInstanceTable recipeTbl(db);
  RewardsTable rewardsTbl(db);
    
  std::vector<uint32_t> iDS;
    
  if((int8_t)rw.type() != (int8_t)RewardType::None)
  {
      auto newReward = rewardsTbl.CreateNew(a->GetName());              
      newReward->MutableProto().set_expeditionid(blueprintAuthID);
      newReward->MutableProto().set_rewardid(basedRewardsTableAuthId);
      newReward->MutableProto().set_tournamentid(tournamentID);
      newReward->MutableProto().set_positionintable(posInTableList);
      newReward->MutableProto().set_fighterid(fighterID);

      if((RewardType)(int)rw.type() == RewardType::CraftedRecipe)
      {
          auto newRecipe = recipeTbl.CreateNew("", rw.craftedrecipeid(), ctx.RoConfig());          
          newReward->MutableProto().set_generatedrecipeid(newRecipe->GetId());
          iDS.push_back(newReward->GetId());
          
          LOG (WARNING) << "CraftedRecipe reward generated";
      }
      
      if((RewardType)(int)rw.type() == RewardType::GeneratedRecipe)
      {
          newReward->MutableProto().set_generatedrecipeid(pxd::RecipeInstance::Generate((pxd::Quality)(int)rw.generatedrecipequality(), ctx.RoConfig(), rnd, db));                      
          iDS.push_back(newReward->GetId());
          
          LOG (WARNING) << "GeneratedRecipe reward generated";
      }
      
      if((RewardType)(int)rw.type() == RewardType::List)
      {
          const auto& rewardsList = ctx.RoConfig()->activityrewards();
          for(const auto& rewardsTable: rewardsList)
          {   
            
            if(rewardsTable.second.authoredid() == rw.listtableid())
            {            
              uint32_t posInTableList2 = 0;
              
              for(auto& rw2: rewardsTable.second.rewards())
              {
                  std::vector<uint32_t> newIDS = GenerateActivityReward(fighterID, blueprintAuthID, tournamentID, rw2, ctx, db, a, rnd, posInTableList2, rw.listtableid());
                  
                  for(long long unsigned int j = 0; j < newIDS.size(); j++)
                  {
                    iDS.push_back(newIDS[j]);
                  }
                  
                  posInTableList2++;
              }
            }
            
            
          }     
      }
  }

  return iDS;
}

void PXLogic::ResolveExpedition(std::unique_ptr<XayaPlayer>& a, const std::string blueprintAuthID, const uint32_t fighterID, Database& db, const Context& ctx, xaya::Random& rnd)
{
    const auto& expeditionList = ctx.RoConfig()->expeditionblueprints();
    bool blueprintSolved = false;
    int rollCount = 0;
    
    const auto chain = ctx.Chain ();
    if (blueprintAuthID == "00000000-0000-0000-zzzz-zzzzzzzzzzzz" && chain == xaya::Chain::MAIN)
    {
        LOG (WARNING) << "Unit test blueprint is not allowed on the mainnet " << blueprintAuthID;
        return;
    }
    
    std::string basedRewardsTableAuthId = "";
    
    for(const auto& expedition: expeditionList)
    {
        if(expedition.second.authoredid() == blueprintAuthID)
        {
            rollCount = expedition.second.baserollcount();
            basedRewardsTableAuthId = expedition.second.baserewardstableid();
            blueprintSolved = true;
            break;
        }
    }   
    
    if(blueprintSolved == false)
    {
        LOG (WARNING) << "Could not resolve expedition in logic blueprint with authID: " << blueprintAuthID;
        return;              
    }
    
    pxd::proto::ActivityReward rewardTableDb;
    
    const auto& rewardsList = ctx.RoConfig()->activityrewards();
    bool rewardsSolved = false;
    
    for(const auto& rewardsTable: rewardsList)
    {
        if(rewardsTable.second.authoredid() == basedRewardsTableAuthId)
        {
            rewardTableDb = rewardsTable.second;
            rewardsSolved = true;
            break;
        }
    }     

    if(rewardsSolved == false)
    {
        LOG (WARNING) << "Could not resolve expedition rewards in logic  with authID: " << blueprintAuthID;
        return;             
    }
    
    FighterTable fighters(db);
    FighterTable::Handle fighter;
    fighter = fighters.GetById (fighterID, ctx.RoConfig ());
    
    if(fighter == nullptr)
    {
        LOG (WARNING) << "Could not resolve fighter with id: " << fighterID;
        return;           
    }

    
    fighter->SetStatus(FighterStatus::Available);
    
    uint32_t totalWeight = 0;
    for(auto& rw: rewardTableDb.rewards())
    {
       totalWeight += (uint32_t)rw.weight();
    }

    LOG (WARNING) << "Rolling for total possible awards: " << rollCount;
    
    for(int roll = 0; roll < rollCount; ++roll)
    {
        int rolCurNum = 0;
        
        if(totalWeight != 0)
        {
          rnd.NextInt(totalWeight);
        }
        
        int accumulatedWeight = 0;
        int posInTableList = 0;
        for(auto& rw: rewardTableDb.rewards())
        {
            accumulatedWeight += rw.weight();
            
            if(rolCurNum <= accumulatedWeight)
            {
               GenerateActivityReward(fighterID, blueprintAuthID, 0, rw, ctx, db, a, rnd, posInTableList, basedRewardsTableAuthId);
            }
            
             posInTableList++;
        }
    }

    //If we are in tutorial yet, lets advance tutorial counter
    
    if(a->GetFTUEState() == FTUEState::FirstExpeditionPending)
    {
        a->SetFTUEState(FTUEState::ResolveFirstExpedition);
    }    
}

void PXLogic::ResolveCookingRecepie(std::unique_ptr<XayaPlayer>& a, const uint32_t recepieID, Database& db, const Context& ctx, xaya::Random& rnd)
{
    /* We must check against treat slots once more,
    because at this point player could have more fighters cooked
    from the previously running instances*/
    FighterTable fighters(db);
    RecipeInstanceTable recipeTbl(db);
    
    uint32_t slots = fighters.CountForOwner(a->GetName());   
  
    if(slots >= ctx.RoConfig()->params().max_fighter_inventory_amount())
    {
        LOG (WARNING) << "Need to revert, not enough slots to host a new fighter for recepie with authid " << recepieID;   
        
        std::map<std::string, pxd::Quantity> fungibleItemAmountForDeduction;
        int32_t cookCost = -1;        
        
        pxd::RecipeInstanceTable::Handle recepie = recipeTbl.GetById(recepieID);
        
        if(recepie == nullptr)
        {
            LOG (ERROR) << "Fatal error, could not find recepie with id: " << recepieID;
            return;
        }
        
        if(recepie->GetProto().quality() == 0)
        {
            cookCost = ctx.RoConfig()->params().common_recipe_cook_cost();
        }
        
        if(recepie->GetProto().quality() == 1)
        {
            cookCost = ctx.RoConfig()->params().uncommon_recipe_cook_cost();
        }

        if(recepie->GetProto().quality() == 2)
        {
            cookCost = ctx.RoConfig()->params().rare_recipe_cook_cost();
        }

        if(recepie->GetProto().quality() == 3)
        {
            cookCost = ctx.RoConfig()->params().epic_recipe_cook_cost();
        } 
        
        for(const auto& candyNeeds: recepie->GetProto().requiredcandy())
        {
          std::string candyInventoryName = BaseMoveProcessor::GetCandyKeyNameFromID(candyNeeds.candytype(), ctx);
          pxd::Quantity quantity = candyNeeds.amount();
          fungibleItemAmountForDeduction.insert(std::pair<std::string, pxd::Quantity>(candyInventoryName, quantity));              
        }
        
        auto& playerInventory = a->GetInventory();
        for(const auto& itemToDeduct: fungibleItemAmountForDeduction)
        {
          playerInventory.AddFungibleCount(itemToDeduct.first, itemToDeduct.second);
        }
    
        a->AddBalance(cookCost); 
        
        recepie->SetOwner(a->GetName());
    }
    else
    {
        auto newFighter = fighters.CreateNew (a->GetName(), recepieID, ctx.RoConfig(), rnd);
        a->CalculatePrestige(ctx.RoConfig());
        
        //If we are in tutorial yet, lets advance tutorial counter
        
        if(a->GetFTUEState() == FTUEState::CookingFirstRecipe)
        {
            a->SetFTUEState(FTUEState::FirstExpedition);
        }
        
        if(a->GetFTUEState() == FTUEState::CookingSecondRecipe)
        {
            a->SetFTUEState(FTUEState::FirstTournament);
        }        
        
        /**
        recipeTbl.DeleteById(recepieID);  
        
        now, jere ideally we want to erase it. but, for example, fighter object stores recepie ID,
        and I am not sure will it be ever used somewhere else again? so, FOR NOW, lets not delete,
        just keep empty owner so it marks this as used this way
        */
    }        
}

void PXLogic::TickAndResolveOngoings(Database& db, const Context& ctx, xaya::Random& rnd)
{
    LOG (INFO) << "Ticking ongoing operations.";
    
    XayaPlayersTable xayaplayers(db);
    auto res = xayaplayers.QueryAll ();

    bool tryAndStep = res.Step();
    while (tryAndStep)
    {
      auto a = xayaplayers.GetFromResult (res, ctx.RoConfig ());
      auto ongoings = a->MutableProto().mutable_ongoings();
      
      std::vector<google::protobuf::internal::RepeatedPtrIterator<pxd::proto::OngoinOperation>> forErasing;
      for (auto it = ongoings->begin(); it != ongoings->end(); it++)
      {
          it->set_blocksleft(it->blocksleft() - 1);

          if(it->blocksleft() == 0)
          {
              if((pxd::OngoingType)it->type() == pxd::OngoingType::COOK_RECIPE)
              {
                LOG (INFO) << "Resolving oingoing operation for pxd::OngoingType::COOK_RECIPE";
                
                ResolveCookingRecepie(a, (uint32_t)it->recipeid(), db, ctx, rnd);
                forErasing.push_back(it);
              }
              
              if((pxd::OngoingType)it->type() == pxd::OngoingType::EXPEDITION)
              {
                LOG (INFO) << "Resolving oingoing operation for pxd::OngoingType::EXPEDITION";
                
                ResolveExpedition(a, it->expeditionblueprintid(), (uint32_t)it->fighterdatabaseid(), db, ctx, rnd);
                forErasing.push_back(it);
              }              
          }          
      }
      
      for(uint32_t t = 0; t < forErasing.size(); t++)
      {
          ongoings->erase(forErasing[t]);
      }
      
       tryAndStep = res.Step ();
    }    
}

uint32_t PXLogic::ExecuteOneMoveAgainstAnother(const Context& ctx, std::string lmv, std::string rmv)
{
    const auto& moveList = ctx.RoConfig()->fightermoveblueprints();
    
    uint32_t lmt = 0;
    uint32_t rmt = 0;
    
    for(auto& moveCandidate: moveList)
    {
        if(moveCandidate.second.authoredid() == lmv)
        {
            rmt = moveCandidate.second.movetype(); //in original code its vice versa, so... 0____0 rmt assigns lmv here
            break;
        }
    }
  
    for(auto& moveCandidate: moveList)
    {
        if(moveCandidate.second.authoredid() == rmv)
        {
            lmt = moveCandidate.second.movetype();
            break;
        }
    }  
    
    if(lmt == rmt)
    {
        return 50;
    }
    
    pxd::MoveType lmtM = (pxd::MoveType)lmt;
    pxd::MoveType rmtM = (pxd::MoveType)rmt;
    
    switch (lmtM)  {
        case pxd::MoveType::Heavy:
             switch (rmtM)
             {
                case pxd::MoveType::Speedy:
                    return 0;
                case pxd::MoveType::Tricky:
                    return 100;
                case pxd::MoveType::Distance:
                    return 100;
                case pxd::MoveType::Blocking:
                    return 0;
                case pxd::MoveType::Heavy:
                    return 0;                    
             }
             return 0;
        case pxd::MoveType::Speedy:
             switch (rmtM)
             {
                case pxd::MoveType::Heavy:
                    return 100;
                case pxd::MoveType::Tricky:
                    return 0;
                case pxd::MoveType::Distance:
                    return 0;
                case pxd::MoveType::Blocking:
                    return 100;
                case pxd::MoveType::Speedy:
                    return 100;                    
             }
             return 0;
        case pxd::MoveType::Tricky:
             switch (rmtM)
             {
                case pxd::MoveType::Heavy:
                    return 0;
                case pxd::MoveType::Speedy:
                    return 100;
                case pxd::MoveType::Distance:
                    return 100;
                case pxd::MoveType::Blocking:
                    return 0;
                case pxd::MoveType::Tricky:
                    return 0;                    
             }
             return 0;   
        case pxd::MoveType::Distance:
             switch (rmtM)
             {
                case pxd::MoveType::Heavy:
                    return 0;
                case pxd::MoveType::Speedy:
                    return 100;
                case pxd::MoveType::Tricky:
                    return 0;
                case pxd::MoveType::Blocking:
                    return 100;
                case pxd::MoveType::Distance:
                    return 100;                    
             }
             return 0;   
        case pxd::MoveType::Blocking:
             switch (rmtM)
             {
                case pxd::MoveType::Heavy:
                    return 100;
                case pxd::MoveType::Speedy:
                    return 0;
                case pxd::MoveType::Tricky:
                    return 100;
                case pxd::MoveType::Distance:
                    return 0;
                case pxd::MoveType::Blocking:
                    return 100;                    
             }
             return 0;                
        default:
            return 0;
    }    
}

 /**
 * FIDE (the World Chess Foundation), gives players with less than 30 played games a K-factor of 25. 
 * Normal players get a K-factor of 15 and pro's get a K-factor of 10.  (Pro = 2400 rating)
 * Once you reach a pro status, you're K-factor never changes, even if your rating drops.
 * 
 * For now set to 15 for everyone.
 */

void PXLogic::CreateEloRating(const Context& ctx, uint32_t& ratingA, uint32_t& ratingB, uint32_t& scoreA, uint32_t& scoreB, uint32_t& expectedA, 
uint32_t& expectedB, uint32_t& newRatingA, uint32_t& newRatingB)
{    
  int KFACTOR = ctx.RoConfig()->params().elok_factor();
  float ALMS = ctx.RoConfig()->params().alms();
  
  expectedA = 100 / (100 + (std::pow(10, ( ratingB - ratingA) / 40000)) );
  expectedB = 100 / (100 + (std::pow(10, ( ratingA - ratingB) / 40000)) );
  
  if (scoreA == 0)
  {
      ratingA = ratingA + (ALMS * (KFACTOR * ( scoreA - expectedA )));
      ratingB = ratingB + (KFACTOR * ( scoreB - expectedB )); 
  }
  else if (scoreB == 0)
  {
      ratingA = ratingA + (KFACTOR * ( scoreA - expectedA ));
      ratingB = ratingB + (ALMS  * (KFACTOR * ( scoreB - expectedB )));
  }  
  else
  {
      ratingA = ratingA + (KFACTOR * ( scoreA - expectedA ));
      ratingB = ratingB + (KFACTOR * ( scoreB - expectedB ));      
  }
  
}

void PXLogic::ProcessTournaments(Database& db, const Context& ctx, xaya::Random& rnd)
{
    LOG (INFO) << "Processing tournaments...";
    
    TournamentTable tournamentsDatabase(db);
    FighterTable fighters(db);
    
    auto res = tournamentsDatabase.QueryAll ();

    bool tryAndStep = res.Step();
    while (tryAndStep)
    {
      auto tnm = tournamentsDatabase.GetFromResult (res, ctx.RoConfig ());  

      if((pxd::TournamentState)(int)tnm->GetInstance().state() == pxd::TournamentState::Listed)
      {
        if((int)tnm->GetProto().teamcount() * (int)tnm->GetProto().teamsize() == tnm->GetInstance().fighters_size())
        {
          tnm->MutableInstance().set_state((int)pxd::TournamentState::Running);
          tnm->MutableInstance().set_blocksleft(tnm->GetProto().duration());
        }
        else
        {

        }
      }
      else if((pxd::TournamentState)(int)tnm->GetInstance().state() == pxd::TournamentState::Running)
      {
          if((int)tnm->GetInstance().blocksleft() > 0)
          {
            tnm->MutableInstance().set_blocksleft((int)tnm->GetInstance().blocksleft() - 1);
          }
          
          if(tnm->GetInstance().blocksleft() == 0)
          {
              // for every fighter, create a pair with all other fighters who are not on the same team
              
              std::map<std::string, std::vector<uint32_t>> teams;
              std::vector<std::pair<uint32_t,uint32_t>> fighterPairs;
              std::map<std::string, uint32_t> participatingPlayerTotalScore;

              //Lets collect teams
              
              for(auto participantFighter : tnm->GetInstance().fighters())
              {
                  auto fighter = fighters.GetById (participantFighter, ctx.RoConfig ());
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
              
              //Lets populate pairs now
              
              for(auto element1 = teams.begin() ; element1 != teams.end() ; ++element1) 
              {
                  for(auto element2 = std::next(element1) ; element2 != teams.end() ; ++element2) 
                  {                    
                      for(long long unsigned int e1 = 0; e1 < element1->second.size(); e1++)
                      {
                        for(long long unsigned int e2 = 0; e2 < element2->second.size(); e2++)
                        {
                            std::pair<uint32_t,uint32_t>  newPair= std::make_pair(element1->second[e1], element2->second[e2]);
                            fighterPairs.push_back(newPair);
                        }
                      }                      
                  }
              }
              
              for(auto fPair: fighterPairs)
              {
                 auto rhs = fighters.GetById (fPair.first, ctx.RoConfig ()); 
                 auto lhs = fighters.GetById (fPair.second, ctx.RoConfig ()); 
                 
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

                 int rScore = 0;
                 int lScore = 0;
                 
                 int count = rhsMoves.size();
                 
                 if(lhsMoves.size() < rhsMoves.size())
                 {
                     count = lhsMoves.size();
                     rScore += (rhsMoves.size() - lhsMoves.size())  * 100;
                 }
                 else
                 {
                    lScore += (lhsMoves.size() - rhsMoves.size())  * 100; 
                 }
                 
                 for(int g = 0; g < count; g++)
                 {
                     std::string rmove = rhsMoves[g];
                     std::string lmove = rhsMoves[g];
                     
                     uint32_t result = ExecuteOneMoveAgainstAnother(ctx, lmove, rmove);
                     rScore += result;
                     lScore += 100 - result;
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
                 
                 uint32_t ratingA = lhs->GetProto().rating() * 100;
                 uint32_t ratingB = rhs->GetProto().rating() * 100;                 
                
                 pxd::MatchResultType lwin = pxd::MatchResultType::Win;
                 pxd::MatchResultType rwin = pxd::MatchResultType::Lose;
                 
                 uint32_t scoreA = 0;
                 uint32_t scoreB = 0;
                 
                 if (participatingPlayerTotalScore.find(lhs->GetOwner()) == participatingPlayerTotalScore.end())
                 {
                     participatingPlayerTotalScore.insert(std::pair<std::string, uint32_t>(lhs->GetOwner(), 0));
                 }
                 
                 if (participatingPlayerTotalScore.find(rhs->GetOwner()) == participatingPlayerTotalScore.end())
                 {
                     participatingPlayerTotalScore.insert(std::pair<std::string, uint32_t>(rhs->GetOwner(), 0));
                 }                 
                 
                 switch (winner)  
                 {
                    case 0:
                        lwin = pxd::MatchResultType::Win;
                        rwin = pxd::MatchResultType::Lose;
                        participatingPlayerTotalScore[lhs->GetOwner()] += 100;
                        scoreA = 100;
                        break;
                    case 1:
                        lwin = pxd::MatchResultType::Draw;
                        rwin = pxd::MatchResultType::Draw;
                        scoreA = 50;
                        scoreB = 50;
                        participatingPlayerTotalScore[lhs->GetOwner()] += 50;
                        participatingPlayerTotalScore[rhs->GetOwner()] += 50;
                        break;
                    case 2:
                        lwin = pxd::MatchResultType::Lose;
                        rwin = pxd::MatchResultType::Win;
                        scoreB = 100;
                        participatingPlayerTotalScore[rhs->GetOwner()] += 100;
                        break;
                 }                 

                 uint32_t expectedA = 0;
                 uint32_t expectedB = 0;
                 uint32_t newRatingA = 0;
                 uint32_t newRatingB = 0;

                 CreateEloRating(ctx, ratingA, ratingB, scoreA, scoreB, expectedA, expectedB, newRatingA, newRatingB);

                 uint32_t lRatingDelta = lhs->GetProto().rating();
                 uint32_t rRatingDelta = rhs->GetProto().rating();
                 
                 uint32_t newRatingForLhs = newRatingA / 100;
                 uint32_t newRatingForRhs = newRatingB / 100;
                 
                 lhs->MutableProto().set_rating(newRatingForLhs);
                 lhs->MutableProto().set_rating(newRatingForRhs);
                 
                 lRatingDelta = newRatingForLhs - lRatingDelta;
                 rRatingDelta = newRatingForRhs - rRatingDelta;
                 
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
                      
                 rhs.reset();
                 lhs.reset();
              }
              
              std::string winnerName = "";
              uint32_t biggestScoreSoFar = 0;
              
              for(const auto& participant: participatingPlayerTotalScore)
              {
                 if(participant.second > biggestScoreSoFar)
                 {
                     winnerName = participant.first;
                     biggestScoreSoFar = participant.second;
                 }
              }
              
              //Rewards creation goes now
              XayaPlayersTable xayaplayers(db);
              
              for(const auto& participant: participatingPlayerTotalScore)
              {
                  auto a = xayaplayers.GetByName(participant.first, ctx.RoConfig());
                  
                  std::string rewardTableId = tnm->GetProto().baserewardstableid();
                  uint32_t rollCount = tnm->GetProto().baserollcount();
                  
                  if(participant.first == winnerName)
                  {
                     rewardTableId = tnm->GetProto().winnerrewardstableid();  
                     rollCount = tnm->GetProto().winnerrollcount();
                  }
                  
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
                      LOG (WARNING) << "Could not resolve expedition rewards in logic  with authID: " << rewardTableId;
                      tnm.reset();
                      return;             
                  }
                                         
                  uint32_t totalWeight = 0;
                  for(auto& rw: rewardTableDb.rewards())
                  {
                     totalWeight += (uint32_t)rw.weight();
                  }

                  LOG (INFO) << "Rolling for total possible rewards: " << rollCount;
                      
                  for(uint32_t roll = 0; roll < rollCount; ++roll)
                  {
                      int rolCurNum = 0;
                      
                      if(totalWeight != 0)
                      {
                        rnd.NextInt(totalWeight);
                      }
                      
                      int accumulatedWeight = 0;
                      int posInTableList = 0;
                      for(auto& rw: rewardTableDb.rewards())
                      {
                          accumulatedWeight += rw.weight();
                          
                          if(rolCurNum <= accumulatedWeight)
                          {
                             GenerateActivityReward(0, "", tnm->GetId(), rw, ctx, db, a, rnd, posInTableList, rewardTableId);
                          }
                          
                           posInTableList++;
                      }
                  }                  
                  
                  a->MutableProto().set_tournamentscompleted(a->GetProto().tournamentscompleted());
                  a->MutableProto().set_tournamentswon(a->GetProto().tournamentswon());

                  LOG (INFO) << "Plater rating before " << a->GetPresitge();

                  a->CalculatePrestige(ctx.RoConfig());
                  
                  LOG (INFO) << "Plater rating after " << a->GetPresitge();
                  
                  a.reset();
              }

              for(auto participantFighter : tnm->GetInstance().fighters())
              {
                  auto fighter = fighters.GetById (participantFighter, ctx.RoConfig ());
                  fighter->SetStatus(pxd::FighterStatus::Available);
                  fighter->MutableProto().set_tournamentinstanceid(0);
                  fighter->UpdateSweetness();
                  fighter.reset();
              }

              tnm->MutableInstance().set_state((int)pxd::TournamentState::Completed);   

              LOG (INFO) << "Finished processing completed tournament with id: " << tnm->GetId();              
          }
      }

      tnm.reset();
      tryAndStep = res.Step ();
    }
}

void PXLogic::ReopenMissingTournaments(Database& db, const Context& ctx)
{
    TournamentTable tournamentsDatabase(db);
    
    const auto& tournamentList = ctx.RoConfig()->tournamentbluprints();
    for(const auto& tournamentBP: tournamentList)
    {
        bool weNeedToCreateNewInstance = true;
        
        //This is not efficient to loop all the time, we just need query with authid directly, so
        //move it outside proto, but for now will do i.e. // TODO
        auto res = tournamentsDatabase.QueryAll ();

        bool tryAndStep = res.Step();
        while (tryAndStep)
        {
          auto tnm = tournamentsDatabase.GetFromResult (res, ctx.RoConfig ());
          
          if(tnm->GetProto().authoredid() == tournamentBP.second.authoredid())
          {
            if((pxd::TournamentState)(int)tnm->GetInstance().state() == pxd::TournamentState::Listed)
            {
              if((int)tnm->GetInstance().fighters_size() < (int)tournamentBP.second.teamsize() * (int)tournamentBP.second.teamcount())
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

void
PXLogic::UpdateState (Database& db, xaya::Random& rnd,
                      const Context& ctx, const Json::Value& blockData)
{
    
  /** We process tournaments super super early, to make sure if we have some closed, next function bellow will
   ** open fresh ones for new block immedaitely. Also later move processor will not interfere with block counts logic
  */
  ProcessTournaments(db, ctx, rnd);    
    
  /** We run this very early, as we want to create tournament instances from blueprints ASAP.
  We are going to open tournaments instances if blueprint is not present ir game or already full and running */
  ReopenMissingTournaments(db, ctx);  
      
  MoveProcessor mvProc(db, rnd, ctx);
  mvProc.ProcessAdmin (blockData["admin"]);
  mvProc.ProcessAll (blockData["moves"]);
  
  /*The first thing we do, is try and resolve all pending ongoing operations
  from the last block. So if there count is minimal 1, they will guarantee
  to get solved before any new moves submitted by player */    
  TickAndResolveOngoings(db, ctx, rnd);

#ifdef ENABLE_SLOW_ASSERTS
  ValidateStateSlow (db, ctx);
#endif // ENABLE_SLOW_ASSERTS
}

void
PXLogic::SetupSchema (xaya::SQLiteDatabase& db)
{
  SetupDatabaseSchema (db);
}

void
PXLogic::GetInitialStateBlock (unsigned& height,
                               std::string& hashHex) const
{
  const xaya::Chain chain = GetChain ();
  switch (chain)
    {
    case xaya::Chain::MAIN:
      height = 3'011'108;
      hashHex
          = "a4c0b18c23f21c15df284710209f089925eb5582b0945cbc317f51899ceeaea4";
      break;

    case xaya::Chain::TEST:
      height = 112'000;
      hashHex
          = "9c5b83a5caaf7f4ce17cc1f38fdb1ed3e3e3e98e43d23d19a4810767d7df38b9";
      break;

    case xaya::Chain::REGTEST:
      height = 0;
      hashHex
          = "6f750b36d22f1dc3d0a6e483af45301022646dfc3b3ba2187865f5a7d6d83ab1";
      break;

    default:
      LOG (FATAL) << "Unexpected chain: " << xaya::ChainToString (chain);
    }
}

void
PXLogic::InitialiseState (xaya::SQLiteDatabase& db)
{
  SQLiteGameDatabase dbObj(db, *this);

  MoneySupply ms(dbObj);
  ms.InitialiseDatabase ();

  /* The initialisation uses up some auto IDs, namely for placed buildings.
     We start "regular" IDs at a later value to avoid shifting them always
     when we tweak initialisation, and thus having to potentially update test
     data and other stuff.  */
  Ids ("pxd").ReserveUpTo (1'000);
}

void
PXLogic::UpdateState (xaya::SQLiteDatabase& db, const Json::Value& blockData)
{
  SQLiteGameDatabase dbObj(db, *this);
  UpdateState (dbObj, GetContext ().GetRandom (),
               GetChain (), blockData);
}

Json::Value
PXLogic::GetStateAsJson (const xaya::SQLiteDatabase& db)
{
  SQLiteGameDatabase dbObj(const_cast<xaya::SQLiteDatabase&> (db), *this);
  const Context ctx(GetChain (),
                    Context::NO_HEIGHT, Context::NO_TIMESTAMP);
  
  GameStateJson gsj(dbObj, ctx);

  return gsj.FullState ();
}

Json::Value
PXLogic::GetCustomStateData (xaya::Game& game, const JsonStateFromRawDb& cb)
{
  return SQLiteGame::GetCustomStateData (game, "data",
      [this, &cb] (const xaya::SQLiteDatabase& db, const xaya::uint256& hash,
                   const unsigned height)
        {
          SQLiteGameDatabase dbObj(const_cast<xaya::SQLiteDatabase&> (db),
                                   *this);
          return cb (dbObj, hash, height);
        });
}

Json::Value
PXLogic::GetCustomStateData (xaya::Game& game,
                             const JsonStateFromDatabaseWithBlock& cb)
{
  return GetCustomStateData (game,
    [this, &cb] (Database& db, const xaya::uint256& hash, const unsigned height)
        {
          const Context ctx(GetChain (),
                            Context::NO_HEIGHT, Context::NO_TIMESTAMP);
          GameStateJson gsj(db, ctx);
          return cb (gsj, hash, height);
        });
}

Json::Value
PXLogic::GetCustomStateData (xaya::Game& game, const JsonStateFromDatabase& cb)
{
  return GetCustomStateData (game,
    [&cb] (GameStateJson& gsj, const xaya::uint256& hash, const unsigned height)
    {
      return cb (gsj);
    });
}


void
PXLogic::ValidateStateSlow (Database& db, const Context& ctx)
{
  LOG (INFO) << "Performing slow validation of the game-state database...";
}

} // namespace pxd

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

#include <xayautil/hash.hpp>
#include "logic.hpp"
#include "database/reward.hpp"

#include "proto/tournament_result.pb.h"

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

std::vector<uint32_t> PXLogic::GenerateActivityReward(const uint32_t fighterID, const std::string blueprintAuthID, const uint32_t tournamentID, const pxd::proto::AuthoredActivityReward rw, const Context& ctx, Database& db, std::unique_ptr<XayaPlayer>& a, xaya::Random& rnd, const uint32_t posInTableList, const std::string basedRewardsTableAuthId, const std::string sweetenerAuthID)
{
  RecipeInstanceTable recipeTbl(db);
  RewardsTable rewardsTbl(db);
    
  std::vector<uint32_t> iDS;
    
  if((int8_t)rw.type() != (int8_t)RewardType::None)
  {
    if((RewardType)(int)rw.type() == RewardType::List)
    {
        const auto& rewardsList = ctx.RoConfig()->activityrewards();
     
        std::vector<std::pair<std::string, pxd::proto::ActivityReward>> sortedActivityRewardTypesmap;
        for (auto itr = rewardsList.begin(); itr != rewardsList.end(); ++itr)
            sortedActivityRewardTypesmap.push_back(*itr);

        sort(sortedActivityRewardTypesmap.begin(), sortedActivityRewardTypesmap.end(), [=](std::pair<std::string, pxd::proto::ActivityReward>& a, std::pair<std::string, pxd::proto::ActivityReward>& b)
        {
            return a.first < b.first;
        } 
        );  
    
        for(const auto& rewardsTable: sortedActivityRewardTypesmap)
        {          
          if(rewardsTable.second.authoredid() == rw.listtableid())
          {            
            uint32_t posInTableList2 = 0;
            
            for(auto& rw2: rewardsTable.second.rewards())
            {
                std::vector<uint32_t> newIDS = GenerateActivityReward(fighterID, blueprintAuthID, tournamentID, rw2, ctx, db, a, rnd, posInTableList2, rw.listtableid(), sweetenerAuthID);
                
                for(long long unsigned int j = 0; j < newIDS.size(); j++)
                {
                  iDS.push_back(newIDS[j]);
                }
                
                posInTableList2++;
            }
          }
          
          
        }     
    }      
    else
    {
      auto newReward = rewardsTbl.CreateNew(a->GetName());              
      newReward->MutableProto().set_expeditionid(blueprintAuthID);
      newReward->MutableProto().set_rewardid(basedRewardsTableAuthId);
      newReward->MutableProto().set_tournamentid(tournamentID);
      newReward->MutableProto().set_positionintable(posInTableList);
      newReward->MutableProto().set_fighterid(fighterID);
      
      if(sweetenerAuthID != "")
      {
        newReward->MutableProto().set_sweetenerid(sweetenerAuthID);  
      }
      else
      {
        newReward->MutableProto().set_sweetenerid(""); 
      }

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
    }
  }

  return iDS;
}

void PXLogic::ResolveSweetener(std::unique_ptr<XayaPlayer>& a, std::string sweetenerAuthID, const uint32_t fighterID, const uint32_t rewardID, Database& db, const Context& ctx, xaya::Random& rnd)
{
  FighterTable fighters(db);
     
  pxd::proto::SweetenerBlueprint sweetenerBlueprint;
  for(auto swb: ctx.RoConfig()->sweetenerblueprints())
  {
    if(swb.second.authoredid() == sweetenerAuthID)
    {
        sweetenerBlueprint = swb.second;
        break;
    }
  } 

  auto fighterDb = fighters.GetById (fighterID, ctx.RoConfig ());
    
  if(sweetenerBlueprint.requiredsweetness() <= fighterDb->GetProto().highestappliedsweetener())
  {
      LOG (WARNING) << "Already applied equal or higher sweetener to this fighter " << fighterID;
      fighterDb->SetStatus(pxd::FighterStatus::Available);
      fighterDb.reset();
      return;          
  }            
    
  fighterDb->MutableProto().set_highestappliedsweetener(sweetenerBlueprint.requiredsweetness());   
  fighterDb->SetStatus(pxd::FighterStatus::Available);
  fighterDb.reset();  
    
  // rewards apply  
  if(sweetenerBlueprint.rewardchoices(rewardID).rewardstableid() != "")
  {
    pxd::proto::ActivityReward rewardTableDb;
    
    const auto& rewardsList = ctx.RoConfig()->activityrewards();
    bool rewardsSolved = false;
    
    for(const auto& rewardsTable: rewardsList)
    {
        if(rewardsTable.second.authoredid() == sweetenerBlueprint.rewardchoices(rewardID).rewardstableid())
        {
            rewardTableDb = rewardsTable.second;
            rewardsSolved = true;
            break;
        }
    }  

    if(rewardsSolved == false)
    {
        LOG (WARNING) << "Could not resolve reward table with id: " << sweetenerBlueprint.rewardchoices(rewardID).rewardstableid();
        return;            
    }    

    uint32_t totalWeight = 0;
    for(auto& rw: rewardTableDb.rewards())
    {
       totalWeight += (uint32_t)rw.weight();
    }

    LOG (WARNING) << "Rolling for total possible awards: " << sweetenerBlueprint.rewardchoices(rewardID).baserollcount();
    
    for(int roll = 0; roll < (int)sweetenerBlueprint.rewardchoices(rewardID).baserollcount(); ++roll)
    {
        int rolCurNum = 0;
        
        if(totalWeight != 0)
        {
          rolCurNum = rnd.NextInt(totalWeight);
        }
        
        int accumulatedWeight = 0;
        int posInTableList = 0;
        for(auto& rw: rewardTableDb.rewards())
        {
            accumulatedWeight += rw.weight();
            
            if(rolCurNum <= accumulatedWeight)
            {
               GenerateActivityReward(fighterID, "", 0, rw, ctx, db, a, rnd, posInTableList, sweetenerBlueprint.rewardchoices(rewardID).rewardstableid(), sweetenerAuthID);
               break;
            }
            
            posInTableList++;
        }
    }       
  }
  
  // moves rewards
  
  if(sweetenerBlueprint.rewardchoices(rewardID).moverewardstableid() != "")
  {
    pxd::proto::ActivityReward rewardTableDb;
    
    const auto& rewardsList = ctx.RoConfig()->activityrewards();
    bool rewardsSolved = false;
    
    for(const auto& rewardsTable: rewardsList)
    {
        if(rewardsTable.second.authoredid() == sweetenerBlueprint.rewardchoices(rewardID).moverewardstableid())
        {
            rewardTableDb = rewardsTable.second;
            rewardsSolved = true;
            break;
        }
    }  

    if(rewardsSolved == false)
    {
        LOG (WARNING) << "Could not resolve reward table with id: " << sweetenerBlueprint.rewardchoices(rewardID).moverewardstableid();
        return;            
    }    

    uint32_t totalWeight = 0;
    for(auto& rw: rewardTableDb.rewards())
    {
       totalWeight += (uint32_t)rw.weight();
    }

    LOG (WARNING) << "Rolling for total possible awards: " << sweetenerBlueprint.rewardchoices(rewardID).moverollcount();
    
    for(int roll = 0; roll < (int)sweetenerBlueprint.rewardchoices(rewardID).moverollcount(); ++roll)
    {
        int rolCurNum = 0;
        
        if(totalWeight != 0)
        {
          rolCurNum = rnd.NextInt(totalWeight);
        }
        
        int accumulatedWeight = 0;
        int posInTableList = 0;
        for(auto& rw: rewardTableDb.rewards())
        {
            accumulatedWeight += rw.weight();
            
            if(rolCurNum <= accumulatedWeight)
            {
               GenerateActivityReward(fighterID, "", 0, rw, ctx, db, a, rnd, posInTableList, sweetenerBlueprint.rewardchoices(rewardID).moverewardstableid(), sweetenerAuthID);
               break;
            }
            
            posInTableList++;
        }
    }       
  }  
  
  // armor rewards
  
  if(sweetenerBlueprint.rewardchoices(rewardID).armorrewardstableid() != "")
  {
    pxd::proto::ActivityReward rewardTableDb;
    
    const auto& rewardsList = ctx.RoConfig()->activityrewards();
    bool rewardsSolved = false;
    
    for(const auto& rewardsTable: rewardsList)
    {
        if(rewardsTable.second.authoredid() == sweetenerBlueprint.rewardchoices(rewardID).armorrewardstableid())
        {
            rewardTableDb = rewardsTable.second;
            rewardsSolved = true;
            break;
        }
    }  

    if(rewardsSolved == false)
    {
        LOG (WARNING) << "Could not resolve reward table with id: " << sweetenerBlueprint.rewardchoices(rewardID).armorrewardstableid();
        return;            
    }    

    uint32_t totalWeight = 0;
    for(auto& rw: rewardTableDb.rewards())
    {
       totalWeight += (uint32_t)rw.weight();
    }

    for(int roll = 0; roll < (int)sweetenerBlueprint.rewardchoices(rewardID).armorrollcount(); ++roll)
    {
        int rolCurNum = 0;
        
        if(totalWeight != 0)
        {
          rolCurNum = rnd.NextInt(totalWeight);
        }
        
        int accumulatedWeight = 0;
        int posInTableList = 0;
        for(auto& rw: rewardTableDb.rewards())
        {
            accumulatedWeight += rw.weight();
            
            if(rolCurNum <= accumulatedWeight)
            {
               GenerateActivityReward(fighterID, "", 0, rw, ctx, db, a, rnd, posInTableList, sweetenerBlueprint.rewardchoices(rewardID).armorrewardstableid(), sweetenerAuthID);
               break;
            }
            
            posInTableList++;
        }
    }       
  }    

  a->CalculatePrestige(ctx.RoConfig());  
 
}

void PXLogic::ResolveDeconstruction(std::unique_ptr<XayaPlayer>& a, const uint32_t fighterID, Database& db, const Context& ctx, xaya::Random& rnd)
{
    uint32_t returnPercent = ctx.RoConfig()->params().deconstruction_return_percent();

    FighterTable fighters(db);
    FighterTable::Handle fighter;
    fighter = fighters.GetById (fighterID, ctx.RoConfig ());
    
    if(fighter == nullptr)
    {
        LOG (WARNING) << "Could not resolve fighter with id: " << fighterID;
        return;           
    }

    RecipeInstanceTable recipeTbl(db); 
    pxd::RecipeInstanceTable::Handle recepie = recipeTbl.GetById(fighter->GetProto().recipeid());
    
    if(recepie == nullptr)
    {
        LOG (ERROR) << "Fatal error, could not find recepie with id: " << fighter->GetProto().recipeid();
        return;
    }    

    uint64_t total = 0;
    std::vector<std::string> candyTypes;
    
    for(const auto& val: recepie->GetProto().requiredcandy())
    {
        total += val.amount();
        candyTypes.push_back(BaseMoveProcessor::GetCandyKeyNameFromID(val.candytype(), ctx));
    }
    
    uint64_t recovered = (total * returnPercent) / 100;
    std::map<std::string, uint64_t> dict;
    
    for(long long unsigned int x =0; x < recovered; x++)
    {
        std::string candyType = candyTypes[rnd.NextInt(candyTypes.size())];
        
        if (dict.find(candyType) == dict.end())
        {
            dict.insert(std::pair<std::string, uint64_t>(candyType, 0));
        }
        
        dict[candyType] += 1;
    }
    
    RewardsTable rewardsTbl(db);
    auto newReward = rewardsTbl.CreateNew(a->GetName());              
    newReward->MutableProto().set_expeditionid("");
    newReward->MutableProto().set_rewardid("");
    newReward->MutableProto().set_tournamentid(0);
    newReward->MutableProto().set_positionintable(0);
    newReward->MutableProto().set_fighterid(fighterID);    
    
    for(const auto& entry: dict)
    {
      proto::Deconstruction* newD = newReward->MutableProto().add_deconstructions();
      newD->set_deconstructionid(fighterID);
      newD->set_candytype(entry.first);
      newD->set_quantity(entry.second);
    }
    
    recepie.reset();
    fighter.reset();
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
          rolCurNum = rnd.NextInt(totalWeight);
        }
        
        int accumulatedWeight = 0;
        int posInTableList = 0;
        for(auto& rw: rewardTableDb.rewards())
        {
            accumulatedWeight += rw.weight();
            
            if(rolCurNum <= accumulatedWeight)
            {
               GenerateActivityReward(fighterID, blueprintAuthID, 0, rw, ctx, db, a, rnd, posInTableList, basedRewardsTableAuthId, "");
               break;
            }
            
             posInTableList++;
        }
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
        
        if(recepie->GetProto().quality() == 1)
        {
            cookCost = ctx.RoConfig()->params().common_recipe_cook_cost();
        }
        
        if(recepie->GetProto().quality() == 2)
        {
            cookCost = ctx.RoConfig()->params().uncommon_recipe_cook_cost();
        }

        if(recepie->GetProto().quality() == 3)
        {
            cookCost = ctx.RoConfig()->params().rare_recipe_cook_cost();
        }

        if(recepie->GetProto().quality() == 4)
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
        
        LOG (WARNING) << "Cooked new fighter with id: " << newFighter->GetId();
        
        /**
        recipeTbl.DeleteById(recepieID);  
        
        now, jere ideally we want to erase it. but, for example, fighter object stores recepie ID,
        and I am not sure will it be ever used somewhere else again? so, FOR NOW, lets not delete,
        just keep empty owner so it marks this as used this way
        
        UPD: Its used in sweetener cauldron display, for example, so defo need to keep ID as fighter
        object references it
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
      auto& ongoings = *a->MutableProto().mutable_ongoings();
      
      
      bool erasingDone = false;
      
      for (auto it = ongoings.begin(); it != ongoings.end(); it++)
      {
        it->set_blocksleft(it->blocksleft() - 1);
      }
      
      while(erasingDone == false)
      {
        erasingDone = true;
        ongoings = *a->MutableProto().mutable_ongoings();
        
        for (auto it = ongoings.begin(); it != ongoings.end(); it++)
        {
            if(it->blocksleft() == 0)
            {
                if((pxd::OngoingType)it->type() == pxd::OngoingType::COOK_RECIPE)
                {
                  LOG (INFO) << "Resolving oingoing operation for pxd::OngoingType::COOK_RECIPE";
                  
                  ResolveCookingRecepie(a, (uint32_t)it->recipeid(), db, ctx, rnd);
                  ongoings.erase(it);
                  erasingDone = false;
                  break;
                }
                
                if((pxd::OngoingType)it->type() == pxd::OngoingType::EXPEDITION)
                {
                  LOG (INFO) << "Resolving oingoing operation for pxd::OngoingType::EXPEDITION";
                  
                  ResolveExpedition(a, it->expeditionblueprintid(), (uint32_t)it->fighterdatabaseid(), db, ctx, rnd);
                  ongoings.erase(it);
                  erasingDone = false;
                  break;
                }     

                if((pxd::OngoingType)it->type() == pxd::OngoingType::DECONSTRUCTION)
                {
                  LOG (INFO) << "Resolving oingoing operation for pxd::OngoingType::DECONSTRUCTION";
                    
                  ResolveDeconstruction(a, (uint32_t)it->fighterdatabaseid(), db, ctx, rnd);
                  ongoings.erase(it); 
                  erasingDone = false;
                  break;                  
                }  

                if((pxd::OngoingType)it->type() == pxd::OngoingType::COOK_SWEETENER)
                {
                  LOG (INFO) << "Resolving oingoing operation for pxd::OngoingType::COOK_SWEETENER";
                    
                  ResolveSweetener(a, it->appliedgoodykeyname(), it->fighterdatabaseid(), it->rewardid(), db, ctx, rnd);
                  ongoings.erase(it); 
                  erasingDone = false;
                  break;                  
                }                 
            }          
        }
      }

      tryAndStep = res.Step ();
    }    
}

fpm::fixed_24_8 PXLogic::ExecuteOneMoveAgainstAnother(const Context& ctx, std::string lmv, std::string rmv)
{
    const auto& moveList = ctx.RoConfig()->fightermoveblueprints();
    
    fpm::fixed_24_8 lmt = fpm::fixed_24_8(0);
    fpm::fixed_24_8 rmt = fpm::fixed_24_8(0);
    
    for(auto& moveCandidate: moveList)
    {
        if(moveCandidate.second.authoredid() == lmv)
        {
            rmt = fpm::fixed_24_8(moveCandidate.second.movetype()); //in original code its vice versa, so... 0____0 rmt assigns lmv here
            break;
        }
    }
  
    for(auto& moveCandidate: moveList)
    {
        if(moveCandidate.second.authoredid() == rmv)
        {
            lmt = fpm::fixed_24_8(moveCandidate.second.movetype());
            break;
        }
    }  
    
    if(lmt == rmt)
    {
        return fpm::fixed_24_8(0.5);
    }
    
    pxd::MoveType lmtM = (pxd::MoveType)(int)lmt;
    pxd::MoveType rmtM = (pxd::MoveType)(int)rmt;
    
    switch (lmtM)  {
        case pxd::MoveType::Heavy:
             switch (rmtM)
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
             switch (rmtM)
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
             switch (rmtM)
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
             switch (rmtM)
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
             switch (rmtM)
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
  int KFACTOR = ctx.RoConfig()->params().elok_factor();
  fpm::fixed_24_8 ALMS = fpm::fixed_24_8(ctx.RoConfig()->params().alms());
  
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

void PXLogic::ProcessTournaments(Database& db, const Context& ctx, xaya::Random& rnd)
{
    TournamentTable tournamentsDatabase(db);
    FighterTable fighters(db);
    
    auto res = tournamentsDatabase.QueryAll ();

    bool tryAndStep = res.Step();
    while (tryAndStep)
    {
      auto tnm = tournamentsDatabase.GetFromResult (res, ctx.RoConfig ());  

      // for every fighter, create a pair with all other fighters who are not on the same team
      
      std::map<std::string, std::vector<uint32_t>> teams;
      std::vector<std::pair<uint32_t,uint32_t>> fighterPairs;
      std::map<std::string, fpm::fixed_24_8> participatingPlayerTotalScore;

      std::map<uint32_t, proto::TournamentResult*> fighterResults;
      //Lets collect teams
      
      if((pxd::TournamentState)(int)tnm->GetInstance().state() == pxd::TournamentState::Running)
      {
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
        
        tnm->MutableInstance().set_teamsjoined(teams.size());
        
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
      }

      if((pxd::TournamentState)(int)tnm->GetInstance().state() == pxd::TournamentState::Listed)
      {          
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
        
        tnm->MutableInstance().set_teamsjoined(teams.size());          
          
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

                 fpm::fixed_24_8 rScore = fpm::fixed_24_8(0);
                 fpm::fixed_24_8 lScore = fpm::fixed_24_8(0);
                 
                 int count = rhsMoves.size();
                 
                 if(lhsMoves.size() < rhsMoves.size())
                 {
                     count = lhsMoves.size();
                     rScore += (rhsMoves.size() - lhsMoves.size())  * 1;
                 }
                 else
                 {
                    lScore += (lhsMoves.size() - rhsMoves.size())  * 1; 
                 }
                 
                 for(int g = 0; g < count; g++)
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
                        
                        fighterResults[fPair.first]->set_losses(fighterResults[fPair.first]->losses() + 1);
                        fighterResults[fPair.second]->set_wins(fighterResults[fPair.second]->wins() + 1);
                        break;
                    case 1:
                        lwin = pxd::MatchResultType::Draw;
                        rwin = pxd::MatchResultType::Draw;
                        scoreA = fpm::fixed_24_8(0.5);
                        scoreB = fpm::fixed_24_8(0.5);
                        participatingPlayerTotalScore[lhs->GetOwner()] += fpm::fixed_24_8(0.5);
                        participatingPlayerTotalScore[rhs->GetOwner()] += fpm::fixed_24_8(0.5);
                        
                        fighterResults[fPair.first]->set_draws(fighterResults[fPair.first]->draws() + 1);
                        fighterResults[fPair.second]->set_draws(fighterResults[fPair.second]->draws() + 1);                        
                        break;
                    case 2:
                        lwin = pxd::MatchResultType::Lose;
                        rwin = pxd::MatchResultType::Win;
                        scoreB = fpm::fixed_24_8(1);
                        participatingPlayerTotalScore[rhs->GetOwner()] += fpm::fixed_24_8(0.5);
                        
                        fighterResults[fPair.second]->set_losses(fighterResults[fPair.second]->losses() + 1);
                        fighterResults[fPair.first]->set_wins(fighterResults[fPair.first]->wins() + 1);                        
                        break;
                 }                 

                 fpm::fixed_24_8 expectedA = fpm::fixed_24_8(0);
                 fpm::fixed_24_8 expectedB = fpm::fixed_24_8(0);
                 fpm::fixed_24_8 newRatingA = fpm::fixed_24_8(0);
                 fpm::fixed_24_8 newRatingB = fpm::fixed_24_8(0);

                 CreateEloRating(ctx, ratingA, ratingB, scoreA, scoreB, expectedA, expectedB, newRatingA, newRatingB);

                 fpm::fixed_24_8 lRatingDelta = fpm::fixed_24_8(lhs->GetProto().rating());
                 fpm::fixed_24_8 rRatingDelta = fpm::fixed_24_8(rhs->GetProto().rating());
                 
                 uint32_t newRatingForLhs = (uint32_t)std::max(0, (int)newRatingA);
                 uint32_t newRatingForRhs = (uint32_t)std::max(0, (int)newRatingB);
                 //ratings unit test pls
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

                 fighterResults[fPair.second]->set_ratingdelta(fighterResults[fPair.second]->ratingdelta() + (int)lRatingDelta);
                 fighterResults[fPair.first]->set_ratingdelta(fighterResults[fPair.first]->ratingdelta() + (int)rRatingDelta);                                   
                      
                 rhs.reset();
                 lhs.reset();
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

                  for(uint32_t roll = 0; roll < rollCount; ++roll)
                  {
                      int rolCurNum = 0;
                      
                      if(totalWeight != 0)
                      {
                        rolCurNum = rnd.NextInt(totalWeight);
                      }
                      
                      int accumulatedWeight = 0;
                      int posInTableList = 0;
                      for(auto& rw: rewardTableDb.rewards())
                      {
                          accumulatedWeight += rw.weight();
                          
                          if(rolCurNum <= accumulatedWeight)
                          {
                             GenerateActivityReward(0, "", tnm->GetId(), rw, ctx, db, a, rnd, posInTableList, rewardTableId, "");
                             break;
                          }
                          
                           posInTableList++;
                      }
                  }                  
                  
                  a->MutableProto().set_tournamentscompleted(a->GetProto().tournamentscompleted());
                  a->MutableProto().set_tournamentswon(a->GetProto().tournamentswon());

                  a->CalculatePrestige(ctx.RoConfig());
                  
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

void PXLogic::CheckFightersForSale(Database& db, const Context& ctx)
{
    FighterTable fighters(db); 

    auto res = fighters.QueryAll ();

    bool tryAndStep = res.Step();
    while (tryAndStep)
    {
      auto fighterDb = fighters.GetFromResult (res, ctx.RoConfig ());
      
      if((pxd::FighterStatus)(int)fighterDb->GetStatus() == pxd::FighterStatus::Exchange) 
      {
         if(fighterDb->GetProto().exchangeexpire() < ctx.Height ())
         {
             fighterDb->SetStatus(pxd::FighterStatus::Available);
         }
      }
      
      fighterDb.reset();
      tryAndStep = res.Step ();
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
    
  /** We run this very early, as we want to create tournament instances from blueprints ASAP.
  We are going to open tournaments instances if blueprint is not present ir game or already full and running */
  ReopenMissingTournaments(db, ctx);  
      
  MoveProcessor mvProc(db, rnd, ctx);
  mvProc.ProcessAdmin (blockData["admin"]);
  mvProc.ProcessAll (blockData["moves"]);
  
  /** The first thing we do, is try and resolve all pending ongoing operations
  from the last block. So if there count is minimal 1, they will guarantee
  to get solved before any new moves submitted by player */    
  TickAndResolveOngoings(db, ctx, rnd);
  
  /** We need to see if any fighters is on exchange, and if its expiration
      is already off and we need to de-lest it automatically*/
  CheckFightersForSale(db, ctx);
  
  /** We process tournaments lastly, to make sure participiant count updates properly
  */
  ProcessTournaments(db, ctx, rnd);   

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
      height = 3'635'984;
      hashHex
          = "4148dd9c0cd1adfc9e60c4f0ac0005f9ef69c343dc272fb8e5452c1df08dcb60";
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
               
          
  Json::Value fState = GetStateAsJson(db);
  
  Json::StreamWriterBuilder builder;
  builder["indentation"] = "";
  
  
  // In order for state to match, we need it do be determenistic; Hence, lets 
  // calculate it not from state itself, but from something simple;

  uint64_t stateNumericValue = 0;
  stateNumericValue += fState["xayaplayers"].size();
  stateNumericValue += fState["activities"].size();
  stateNumericValue += fState["crystalbundles"].size();
  stateNumericValue += fState["fighters"].size();
  stateNumericValue += fState["rewards"].size();
  stateNumericValue += fState["recepies"].size();

  std::vector<std::string> sortedFighterNames;
  std::string finalStringForHasing = "";
  finalStringForHasing +=std::to_string(stateNumericValue);
  
  for(auto& ft: fState["fighters"])
  {
      sortedFighterNames.push_back(ft["name"].asString());
  }
  
  for(auto& name: sortedFighterNames)
  {
      finalStringForHasing += name;
  }
  
  xaya::uint256 heystring = xaya::SHA256::Hash(finalStringForHasing);
  
  const auto& blockMeta = blockData["block"];
  const auto& heightVal = blockMeta["height"];
  const uint64_t height = heightVal.asUInt64 ();  
  
  GameStateJson::latestKnownStateHash = heystring.ToHex ();
  GameStateJson::latestKnownStateBlock = height;                 
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

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

/* Ongoing-operation resolution: reward generation and the per-tick resolve handlers.
   Split out of logic.cpp; part of the pxd::PXLogic implementation. */

namespace pxd
{

std::vector<uint32_t> PXLogic::GenerateActivityReward(const uint32_t fighterID, const std::string blueprintAuthID, const uint32_t tournamentID, const pxd::proto::AuthoredActivityReward rw, const Context& ctx, Database& db, std::unique_ptr<XayaPlayer>& a, xaya::Random& rnd, const uint32_t posInTableList, const std::string basedRewardsTableAuthId, const std::string sweetenerAuthID)
{
  RecipeInstanceTable recipeTbl(db);
  RewardsTable rewardsTbl(db);
    
  std::vector<uint32_t> iDS;
    
  if((int8_t)rw.type() != (int8_t)RewardType::None)
  {
    if((RewardType)(int32_t)rw.type() == RewardType::List)
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
                
                for(int32_t j = 0; j < (int32_t)newIDS.size(); j++)
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
      /* H5: bound passive unclaimed-reward accumulation. At the cap the reward
         (and its generated recipe) is not created; the player must claim some
         first. Deconstruction rewards bypass this (see ResolveDeconstruction). */
      if(rewardsTbl.CountForOwner(a->GetName()) >= ctx.RoConfig()->params().max_unclaimed_reward_amount())
      {
        LOG (WARNING) << "Player " << a->GetName() << " at unclaimed-reward cap ("
                      << ctx.RoConfig()->params().max_unclaimed_reward_amount() << "); passive reward not granted";
        return iDS;
      }

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

      if((RewardType)(int32_t)rw.type() == RewardType::CraftedRecipe)
      {
          auto newRecipe = recipeTbl.CreateNew("", rw.craftedrecipeid(), ctx.RoConfig());          
          newReward->MutableProto().set_generatedrecipeid(newRecipe->GetId());
          iDS.push_back(newReward->GetId());
          
          LOG (INFO) << "CraftedRecipe reward generated";
      }
      
      if((RewardType)(int32_t)rw.type() == RewardType::GeneratedRecipe)
      {
          newReward->MutableProto().set_generatedrecipeid(pxd::RecipeInstance::Generate((pxd::Quality)(int32_t)rw.generatedrecipequality(), ctx.RoConfig(), rnd, db, ""));
          iDS.push_back(newReward->GetId());
          
          LOG (INFO) << "GeneratedRecipe reward generated";
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
  if (fighterDb == nullptr)
  {
      LOG (WARNING) << "Could not resolve fighter at sweetener resolve: " << fighterID;
      return;
  }

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

    for(int32_t roll = 0; roll < (int32_t)sweetenerBlueprint.rewardchoices(rewardID).baserollcount(); ++roll)
    {
        int32_t rolCurNum = 0;
        
        if(totalWeight != 0)
        {
          rolCurNum = rnd.NextInt(totalWeight);
        }
		
		VLOG (1) << "rolCurNum: " << rolCurNum;
        
        int32_t accumulatedWeight = 0;
        int32_t posInTableList = 0;
        for(auto& rw: rewardTableDb.rewards())
        {
            accumulatedWeight += rw.weight();

            if(rolCurNum < accumulatedWeight)
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
        LOG (WARNING) << "Could not resolve moves reward table with id: " << sweetenerBlueprint.rewardchoices(rewardID).moverewardstableid();
        return;            
    }    

    uint32_t totalWeight = 0;
    for(auto& rw: rewardTableDb.rewards())
    {
       totalWeight += (uint32_t)rw.weight();
    }

    VLOG (1) << "Rolling for total possible moves: " << sweetenerBlueprint.rewardchoices(rewardID).moverollcount();
    
    for(int32_t roll = 0; roll < (int32_t)sweetenerBlueprint.rewardchoices(rewardID).moverollcount(); ++roll)
    {
        int32_t rolCurNum = 0;
        
        if(totalWeight != 0)
        {
          rolCurNum = rnd.NextInt(totalWeight);
        }
        
		VLOG (1) << "rolCurNum: " << rolCurNum;
		
        int32_t accumulatedWeight = 0;
        int32_t posInTableList = 0;
        for(auto& rw: rewardTableDb.rewards())
        {
            accumulatedWeight += rw.weight();
            
            if(rolCurNum < accumulatedWeight)
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

    for(int32_t roll = 0; roll < (int32_t)sweetenerBlueprint.rewardchoices(rewardID).armorrollcount(); ++roll)
    {
        int32_t rolCurNum = 0;
        
        if(totalWeight != 0)
        {
          rolCurNum = rnd.NextInt(totalWeight);
        }
        
		VLOG (1) << "rolCurNum: " << rolCurNum;
		
        int32_t accumulatedWeight = 0;
        int32_t posInTableList = 0;
        for(auto& rw: rewardTableDb.rewards())
        {
            accumulatedWeight += rw.weight();
            
            if(rolCurNum < accumulatedWeight)
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
    
    for(uint64_t x =0; x < recovered; x++)
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
    int32_t rollCount = 0;
    
	const auto chain = ctx.Chain ();	
	
    if (blueprintAuthID == "00000000-0000-0000-zzzz-zzzzzzzzzzzz" && chain != xaya::Chain::REGTEST)
    {
        LOG (WARNING) << "Unit test blueprint is not allowed on a production chain " << blueprintAuthID;
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

    for(int32_t roll = 0; roll < rollCount; ++roll)
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
    
        /* cookCost is the -1 sentinel until a quality 1..4 branch overwrites it.
           Recipe quality is immutable and validated to 1..4 upstream, so this
           never fires on valid state; guard anyway so an unexpected quality
           halts loudly rather than silently subtracting from the balance. */
        CHECK_GE (cookCost, 0) << "cook refund hit unexpected recipe quality";
        a->AddBalance(cookCost);

        recepie->SetOwner(a->GetName());
    }
    else
    {
        auto newFighter = fighters.CreateNew (a->GetName(), recepieID, ctx.RoConfig(), rnd);
		
		
        a->CalculatePrestige(ctx.RoConfig());
        
        newFighter->SetStatus(FighterStatus::Cooked);
        
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

    /* P-E1 / event-driven: ongoings live in the height-keyed ongoing_operations
       table, each with an ABSOLUTE resolve height.  An idle block runs only this
       one indexed `WHERE height <= now` SELECT (returns nothing) -- no per-block
       scan or rewrite of every player.  We materialise the due rows first (in the
       deterministic `ORDER BY id` the query guarantees) so resolving + deleting
       cannot invalidate the cursor, and so the RNG-draw order is fixed across all
       nodes (creation order == id order). */
    OngoingsTable ongoings(db);

    struct DueOp { Database::IdT id; std::string owner; proto::OngoinOperation proto; };
    std::vector<DueOp> due;

    {
      auto res = ongoings.QueryForHeight (ctx.Height ());
      while (res.Step ())
      {
        auto op = ongoings.GetFromResult (res);
        due.push_back ({op->GetId (), op->GetOwner (), op->GetProto ()});
      }
    }

    if (due.empty ())
      return;

    XayaPlayersTable xayaplayers(db);

    for (const auto& d : due)
    {
      auto a = xayaplayers.GetByName (d.owner, ctx.RoConfig ());
      if (a == nullptr)
      {
        LOG (ERROR) << "Ongoing operation " << d.id << " owner '" << d.owner << "' no longer exists; dropping";
        ongoings.DeleteById (d.id);
        continue;
      }

      switch ((pxd::OngoingType) d.proto.type ())
      {
        case pxd::OngoingType::COOK_RECIPE:
          LOG (INFO) << "Resolving ongoing operation COOK_RECIPE";
          ResolveCookingRecepie (a, (uint32_t) d.proto.recipeid (), db, ctx, rnd);
          break;

        case pxd::OngoingType::EXPEDITION:
          LOG (INFO) << "Resolving ongoing operation EXPEDITION";
          ResolveExpedition (a, d.proto.expeditionblueprintid (), (uint32_t) d.proto.fighterdatabaseid (), db, ctx, rnd);
          break;

        case pxd::OngoingType::DECONSTRUCTION:
          LOG (INFO) << "Resolving ongoing operation DECONSTRUCTION";
          ResolveDeconstruction (a, (uint32_t) d.proto.fighterdatabaseid (), db, ctx, rnd);
          break;

        case pxd::OngoingType::COOK_SWEETENER:
          LOG (INFO) << "Resolving ongoing operation COOK_SWEETENER for " << d.owner << " and " << d.proto.appliedgoodykeyname () << " and " << d.proto.fighterdatabaseid ();
          ResolveSweetener (a, d.proto.appliedgoodykeyname (), d.proto.fighterdatabaseid (), d.proto.rewardid (), db, ctx, rnd);
          break;

        default:
          LOG (ERROR) << "Ongoing operation " << d.id << " has unknown type " << d.proto.type ();
          break;
      }

      a.reset ();
      ongoings.DeleteById (d.id);
    }
}

} // namespace pxd

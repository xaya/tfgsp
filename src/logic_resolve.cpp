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

void PXLogic::GenerateActivityReward(const uint32_t fighterID, const std::string& blueprintAuthID, const uint32_t tournamentID, const pxd::proto::AuthoredActivityReward& rw, const Context& ctx, Database& db, std::unique_ptr<XayaPlayer>& a, xaya::Random& rnd, const uint32_t posInTableList, const std::string& basedRewardsTableAuthId, const std::string& sweetenerAuthID, const uint32_t recursionDepth)
{
  /* CC-1: bound the recursive RewardType::List expansion. Reward tables
     reference sub-tables via listtableid(); a cyclic or pathologically-deep
     reference in the (pinned, dev-authored) roconfig would recurse until the
     stack overflows -> a deterministic chain halt. Real nesting is shallow, so
     stop past a safe cap rather than trust the config to be acyclic. */
  constexpr uint32_t MAX_REWARD_LIST_DEPTH = 16;
  if(recursionDepth > MAX_REWARD_LIST_DEPTH)
  {
    LOG (ERROR) << "Reward List recursion exceeded max depth " << MAX_REWARD_LIST_DEPTH
                << " (cyclic or too-deep reward-table config); stopping generation";
    return;
  }

  if((int8_t)rw.type() == (int8_t)RewardType::None)
    return;

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
              /* Same draw order as before: the sub-table is expanded in sorted
                 authoredid order, each entry recursed in field order. */
              GenerateActivityReward(fighterID, blueprintAuthID, tournamentID, rw2, ctx, db, a, rnd, posInTableList2, rw.listtableid(), sweetenerAuthID, recursionDepth + 1);
              posInTableList2++;
          }
        }
      }
      return;
  }

  /* Leaf reward: credit it straight into the player/fighter at resolve
     (no claim tx). */
  CreditActivityReward(a, fighterID, blueprintAuthID, tournamentID, rw, ctx, db, rnd, posInTableList, basedRewardsTableAuthId, sweetenerAuthID);
}

void PXLogic::CreditActivityReward(std::unique_ptr<XayaPlayer>& a, const uint32_t fighterID, const std::string& blueprintAuthID, const uint32_t tournamentID, const pxd::proto::AuthoredActivityReward& rw, const Context& ctx, Database& db, xaya::Random& rnd, const uint32_t posInTableList, const std::string& basedRewardsTableAuthId, const std::string& sweetenerAuthID)
{
  const RewardType type = (RewardType)(int32_t)rw.type();

  /* The true issuing source, derived from which resolve path we came through.
     (The armor branch used to hard-code Expedition -- D5 fixes it.) */
  const RewardSource source = (sweetenerAuthID != "")
      ? RewardSource::Sweetener
      : (tournamentID != 0 ? RewardSource::Tournament : RewardSource::Expedition);

  const auto bumpSerial = [&a] ()
  {
    a->MutableProto().set_rewards_serial(a->GetProto().rewards_serial() + 1);
  };

  switch(type)
  {
    case RewardType::Candy:
    {
      const std::string candyName = BaseMoveProcessor::GetCandyKeyNameFromID(rw.candytype(), ctx);
      a->GetInventory().AddFungibleCount(candyName, rw.quantity());
      bumpSerial();
      return;
    }

    case RewardType::Move:
    case RewardType::Animation:
    case RewardType::Armor:
    {
      /* HALT-01: the reward's fighter can already be gone -- a tournament reward
         carries fighterID 0, and a fighter can be transfigured/cook-replaced
         between rolls.  Drop the reward deterministically instead of ever
         dereferencing a null handle (which would SIGSEGV every node at the same
         height -> a permanent chain halt). */
      FighterTable fighters(db);
      auto fighter = fighters.GetById(fighterID, ctx.RoConfig());
      if(fighter == nullptr)
      {
        LOG (WARNING) << "Reward fighter " << fighterID << " no longer exists; dropping reward type " << (int32_t)type;
        return;
      }

      if(type == RewardType::Move)
      {
        fighter->MutableProto().add_moves()->assign(rw.moveid());
      }
      else if(type == RewardType::Animation)
      {
        fighter->MutableProto().set_animationid(rw.animationid());
      }
      else /* Armor */
      {
        bool armorTypeWasPresent = false;
        for(int i = 0; i < fighter->MutableProto().armorpieces_size(); ++i)
        {
          /* Mutable reference, not a copy: set_* must persist to the proto. */
          auto& armorPiece = *fighter->MutableProto().mutable_armorpieces(i);
          if(armorPiece.armortype() == rw.armortype())
          {
            armorTypeWasPresent = true;
            armorPiece.set_candy(rw.candytype());
            armorPiece.set_rewardsourceid(basedRewardsTableAuthId);
            armorPiece.set_rewardsource((uint32_t)source);
          }
        }
        if(armorTypeWasPresent == false)
        {
          auto newArmorPiece = fighter->MutableProto().add_armorpieces();
          newArmorPiece->set_candy(rw.candytype());
          newArmorPiece->set_rewardsourceid(basedRewardsTableAuthId);
          newArmorPiece->set_rewardsource((uint32_t)source);
          newArmorPiece->set_armortype(rw.armortype());
        }
      }

      fighter.reset();
      bumpSerial();
      return;
    }

    case RewardType::CraftedRecipe:
    case RewardType::GeneratedRecipe:
    {
      RecipeInstanceTable recipeTbl(db);
      const uint32_t maxRecipeSlots = ctx.RoConfig()->params().max_recipe_inventory_amount();

      /* Recipe is the only capped reward.  Slot free -> credit immediately by
         creating the recipe already owned by the player.  The GeneratedRecipe
         rnd draw fires here exactly as it did at resolve before. */
      if(recipeTbl.CountForOwner(a->GetName()) < maxRecipeSlots)
      {
        if(type == RewardType::CraftedRecipe)
          recipeTbl.CreateNew(a->GetName(), rw.craftedrecipeid(), ctx.RoConfig());
        else
          pxd::RecipeInstance::Generate((pxd::Quality)(int32_t)rw.generatedrecipequality(), ctx.RoConfig(), rnd, db, a->GetName());
        bumpSerial();
        return;
      }

      /* Slots full -> HOLD as a bounded overflow row (owner=player, recipe
         attached with owner="") that auto-drains when a slot frees.  This is now
         the ONLY user of the rewards table, so max_unclaimed_reward_amount bounds
         just the recipe-overflow queue.  At the cap the recipe is dropped (no
         generation, no row) -- matching the old at-cap drop, and keeping the
         GeneratedRecipe draw count identical to the free-slot path only when
         under the cap. */
      RewardsTable rewardsTbl(db);
      if(rewardsTbl.CountForOwner(a->GetName()) >= ctx.RoConfig()->params().max_unclaimed_reward_amount())
      {
        LOG (WARNING) << "Player " << a->GetName() << " at recipe-overflow cap ("
                      << ctx.RoConfig()->params().max_unclaimed_reward_amount() << "); recipe reward dropped";
        return;
      }

      auto newReward = rewardsTbl.CreateNew(a->GetName());
      newReward->MutableProto().set_expeditionid(blueprintAuthID);
      newReward->MutableProto().set_rewardid(basedRewardsTableAuthId);
      newReward->MutableProto().set_tournamentid(tournamentID);
      newReward->MutableProto().set_positionintable(posInTableList);
      newReward->MutableProto().set_fighterid(fighterID);
      newReward->MutableProto().set_sweetenerid(sweetenerAuthID);

      if(type == RewardType::CraftedRecipe)
        newReward->MutableProto().set_generatedrecipeid(recipeTbl.CreateNew("", rw.craftedrecipeid(), ctx.RoConfig())->GetId());
      else
        newReward->MutableProto().set_generatedrecipeid(pxd::RecipeInstance::Generate((pxd::Quality)(int32_t)rw.generatedrecipequality(), ctx.RoConfig(), rnd, db, ""));

      /* Do NOT bump the serial: a held recipe is not in inventory yet.  It bumps
         when it drains into ownership (DrainPendingRecipeRewards). */
      return;
    }

    default:
      LOG (WARNING) << "Unknown reward type not credited: " << (int32_t)type;
      return;
  }
}

void PXLogic::DrainPendingRecipeRewards(std::unique_ptr<XayaPlayer>& a, const Context& ctx, Database& db)
{
  RewardsTable rewardsTbl(db);
  RecipeInstanceTable recipeTbl(db);
  const uint32_t maxRecipeSlots = ctx.RoConfig()->params().max_recipe_inventory_amount();

  /* Collect this player's held recipe-overflow rows first (QueryForOwner is
     ORDER BY id == oldest-first), then drain -- never mutate recipe ownership
     while the owner-keyed cursor is still open.  Bounded by the overflow cap. */
  std::vector<std::pair<Database::IdT, uint32_t>> pending;   // {rewardRowId, generatedRecipeId}
  {
    auto res = rewardsTbl.QueryForOwner(a->GetName());
    while(res.Step())
    {
      auto rw = rewardsTbl.GetFromResult(res);
      if(rw->GetProto().generatedrecipeid() > 0)
        pending.push_back({rw->GetId(), rw->GetProto().generatedrecipeid()});
    }
  }

  for(const auto& p : pending)
  {
    if(recipeTbl.CountForOwner(a->GetName()) >= maxRecipeSlots)
      break;   // no more slots; leave the rest held for the next slot-free event

    auto rec = recipeTbl.GetById(p.second);
    if(rec != nullptr)
    {
      rec->SetOwner(a->GetName());
      rec.reset();
      a->MutableProto().set_rewards_serial(a->GetProto().rewards_serial() + 1);
    }
    /* Whether or not the recipe still existed, the holding row is now spent. */
    rewardsTbl.DeleteById(p.first);
  }
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
    
  // rewards apply. rewardID was bounds-checked at cook time
  // (moveprocessor_cooking.cpp) against this same pinned blueprint; re-guard the
  // repeated-field index here so a default/short blueprint can never out-of-bounds
  // index rewardchoices() -- that would be UB (a silent fork) in an NDEBUG build.
  // The candy/move/armor blocks below all index rewardchoices(rewardID), so the
  // guard must cover the whole span, not just the first block (the tier bump and
  // prestige above still stand: the cook succeeded, only the reward rolls drop).
  if(rewardID >= (uint32_t) sweetenerBlueprint.rewardchoices_size())
  {
    LOG (WARNING) << "sweetener rewardID " << rewardID << " out of range ("
                  << sweetenerBlueprint.rewardchoices_size() << "); skipping reward rolls";
    a->CalculatePrestige(ctx.RoConfig());
    return;
  }

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

    /* Identical rnd draws / order as before: `recovered` picks over candyTypes. */
    for(uint64_t x = 0; x < recovered; x++)
    {
        std::string candyType = candyTypes[rnd.NextInt(candyTypes.size())];
        dict[candyType] += 1;
    }

    /* Account-bound fighters recover nothing (matches the old claim behaviour,
       which skipped the payout when isaccountbound). */
    const bool isAccountBound = fighter->GetProto().isaccountbound();
    const uint32_t deconstructedRecipeId = fighter->GetProto().recipeid();

    recepie.reset();
    fighter.reset();

    /* Credit the recovered candy straight into inventory (no claim tx), then
       delete the fighter + its source recipe and recompute prestige -- the roster
       slot now frees at resolve instead of at a follow-up claim.  Candy is
       unbounded-safe, so it credits unconditionally (no cap gate). */
    if(isAccountBound == false && !dict.empty())
    {
      for(const auto& entry: dict)
        a->GetInventory().AddFungibleCount(entry.first, entry.second);
      a->MutableProto().set_rewards_serial(a->GetProto().rewards_serial() + 1);
    }

    fighters.DeleteById(fighterID);
    if(deconstructedRecipeId > 0)
      recipeTbl.DeleteById(deconstructedRecipeId);

    a->CalculatePrestige(ctx.RoConfig());
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

    /* Release the fighter handle BEFORE rolling rewards.  A Move/Armor/Animation
       reward credits via CreditActivityReward, which re-opens a FighterTable
       handle for this same fighter -- two live handles for the same id would trip
       libxayagame's UniqueHandles CHECK (a deterministic all-node abort = chain
       halt), and the stale outer handle would otherwise clobber the credited
       reward on flush.  reset() flushes the SetStatus write and adds no rnd draw,
       so the RNG stream is untouched.  Mirrors ResolveSweetener's pattern.  */
    fighter.reset();

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
        
        cookCost = BaseMoveProcessor::RecipeCookCostForQuality(recepie->GetProto().quality(), ctx.RoConfig());

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

        /* The recipe row is intentionally kept (not deleted): the cooked
           fighter references its recipe id (e.g. the sweetener-cauldron
           display reads it).  Cleared ownership marks it as used.  */
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

/*
    GSP for the tf blockchain game
    Copyright (C) 2019-2021  Autonomous Worlds Ltd

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

#include "moveprocessor.hpp"
#include "moveprocessor_internal.hpp"
#include "logic.hpp"   // PXLogic::DrainPendingRecipeRewards (recipe-slot-free drain)

#include "jsonutils.hpp"
#include "proto/config.pb.h"

#include <xayautil/jsonutils.hpp>

#include <xayagame/sqlitestorage.hpp>

#include <json/json.h>

#include <sstream>
#include <iostream>
#include <vector>
#include <string>

#include <chrono>
#include <thread>

/* Recipe cooking + sweetener cooking: parse, collect, destroy, resolve.
   Split out of moveprocessor.cpp; part of pxd::MoveProcessor. */

namespace pxd
{

  pxd::RecipeInstanceTable::Handle BaseMoveProcessor::GetRecepieObjectFromID(const uint32_t& ID)
  {
    return recipeTbl.GetById(ID);
  }

  std::string BaseMoveProcessor::GetCandyKeyNameFromID(const std::string& authID, const Context& ctx)
  {
      
    const auto& candylist = ctx.RoConfig()->candies();
    
    for(const auto& candy: candylist)
    {
        if(candy.second.authoredid() == authID)
        {
            return candy.first;
        }
    }      
      
    return "";
  }

  Amount
  BaseMoveProcessor::RecipeCookCostForQuality(int32_t quality, const RoConfig& cfg)
  {
    switch(quality)
    {
      case 1: return cfg->params().common_recipe_cook_cost();
      case 2: return cfg->params().uncommon_recipe_cook_cost();
      case 3: return cfg->params().rare_recipe_cook_cost();
      case 4: return cfg->params().epic_recipe_cook_cost();
      default: return -1;
    }
  }

  Json::Value BaseMoveProcessor::EvaluateFuelList(const Json::Value& fightersSubmited, const Json::Value& recipesSubmited, const Json::Value& candiesSubmited, const Json::Value& fightersNew, const Json::Value& recipesNew, const Json::Value& candiesNew, const Json::Value wholeFightersData,  const Json::Value wholeRecipeData, const Json::Value& candylist)
  {
	 Json::Value fighter(Json::objectValue);	 
	 Json::Value aif(Json::arrayValue);
	 Json::Value aic(Json::arrayValue);
	 Json::Value air(Json::arrayValue);
	 
	 for(auto& ft: fightersSubmited)
	 {
		 aif.append(ft);
	 }
	 
	 for(auto& rc: recipesSubmited)
	 {
		 air.append(rc);
	 }	 
	 
	 for(auto& cd: candiesSubmited)
	 {
		 Json::Value cData(Json::objectValue);
	     cData["n"] = cd["n"];
		 cData["a"] = cd["a"];
		 aic.append(cData);
	 }		 
	
	 fighter["if"] = aif;
	 fighter["ic"] = aic;
	 fighter["ir"] = air;

	 fpm::fixed_24_8 fuelPowerOriginal = MoveProcessor::CalculateFuelPower(fighter, wholeFightersData, wholeRecipeData);
	 
	 Json::Value outputNewValues(Json::objectValue);	 
	 Json::Value aif2(Json::arrayValue);
	 Json::Value aic2(Json::arrayValue);
	 Json::Value air2(Json::arrayValue);
	 
	 for(auto& ft: fightersNew)
	 {
		 Json::Value newEstimation = fighter;
		 newEstimation["if"].append(ft);
		 fpm::fixed_24_8 fuelPowerNewValue = MoveProcessor::CalculateFuelPower(newEstimation, wholeFightersData, wholeRecipeData);
		 fpm::fixed_24_8 diff = (fuelPowerNewValue - fuelPowerOriginal);
		 
		 Json::Value result(Json::objectValue);
		 result["i"] = ft;
		 result["v"] = (int32_t)fpm::floor(diff);
		 
		 aif2.append(result);
	 }
	
	 for(auto& rc: recipesNew)
	 {
		 Json::Value newEstimation = fighter;
		 newEstimation["ir"].append(rc);
		 fpm::fixed_24_8 fuelPowerNewValue = MoveProcessor::CalculateFuelPower(newEstimation, wholeFightersData, wholeRecipeData);
		 fpm::fixed_24_8 diff = (fuelPowerNewValue - fuelPowerOriginal);
		 
		 Json::Value result(Json::objectValue);
		 result["i"] = rc;
		 result["v"] = (int32_t)fpm::floor(diff);
		 
		 air2.append(result);
	 }	
	 
	 for(auto& cn: candiesNew)
	 {
		 Json::Value newEstimation = fighter;
		 
		 Json::Value cData(Json::objectValue);
	     cData["n"] = cn;
		 cData["a"] = 10;
		 newEstimation["ic"].append(cData);		 

		 fpm::fixed_24_8 fuelPowerNewValue = MoveProcessor::CalculateFuelPower(newEstimation, wholeFightersData, wholeRecipeData);
		 fpm::fixed_24_8 diff = (fuelPowerNewValue - fuelPowerOriginal);
		 
		 Json::Value result(Json::objectValue);
		 result["i"] = cn;
		 result["v"] = (int32_t)fpm::floor(diff);
		 
		 aic2.append(result);
	 }		 
	 
	 outputNewValues["if"] = aif2;
	 outputNewValues["ic"] = aic2;
	 outputNewValues["ir"] = air2;	 
	 outputNewValues["fp"] = (int32_t)fuelPowerOriginal;
	 
	 return outputNewValues;
  }	  


  bool
  BaseMoveProcessor::ParseSweetener(const XayaPlayer& a, const std::string& name, const Json::Value& sweetener, std::map<std::string, pxd::Quantity>& fungibleItemAmountForDeduction, int32_t& cookCost, uint32_t& fighterID, int32_t& duration, std::string& sweetenerKeyName)
  {
    if (!sweetener.isObject ())
    {
       return false;
    } 

    if (!sweetener["sid"].isString())
    {
       LOG (WARNING) << "sid is not a string";
       return false;
    }  

    if (!sweetener["fid"].isInt())
    {
       LOG (WARNING) << "fid is not a int";
       return false;
    }     

    if (!sweetener["rid"].isInt())
    {
       LOG (WARNING) << "rid is not a int";
       return false;
    }       

    fighterID = (uint32_t)sweetener["fid"].asInt();
    const int32_t rewardID = (uint32_t)sweetener["rid"].asInt();
    std::string sweetenerID = sweetener["sid"].asString();
    
    
    auto fighterDb = fighters.GetById (fighterID, ctx.RoConfig ());
    
    if(fighterDb == nullptr)
    {
      LOG (WARNING) << "Fatal error, could not get fighter with ID" << fighterID;
      fighterDb.reset();
      return false;                
    }
    
    if(fighterDb->GetOwner() != a.GetName())
    {
      LOG (WARNING) << "Fighter does not belong to: " << a.GetName();
      fighterDb.reset();
      return false;               
    }    
    
    if((pxd::FighterStatus)(int32_t)fighterDb->GetStatus() != pxd::FighterStatus::Available) 
    {
      LOG (WARNING) << "Fighter status is not available: " << fighterID;
      fighterDb.reset();
      return false;              
    }    

    pxd::proto::SweetenerBlueprint sweetenerBlueprint;
    for(auto swb: ctx.RoConfig()->sweetenerblueprints())
    {
        if(swb.second.authoredid() == sweetenerID)
        {
            sweetenerBlueprint = swb.second;
            sweetenerKeyName = swb.first;
            break;
        }
    }

    if(sweetenerKeyName == "")
    {
      LOG (WARNING) << "Sweetener uthored id not found for: " << sweetenerID;
      fighterDb.reset();
      return false;               
    }   

     auto& playerInventory = a.GetInventory();
     
     if(playerInventory.GetFungibleCount(sweetenerKeyName) == 0)
     {
      LOG (WARNING) << "Player does not have sweetener: " << sweetenerKeyName;
      fighterDb.reset();
      return false;             
     }
     
     if(a.GetBalance() < sweetenerBlueprint.cookcost())
     {
      LOG (WARNING) << "Player does not have enough crystals, needs: " << sweetenerBlueprint.cookcost();
      fighterDb.reset();
      return false;            
     }
     
     if(rewardID >= sweetenerBlueprint.rewardchoices_size() || rewardID < 0)
     {
      LOG (WARNING) << "Reward ID is invalid, must be in range of 0 to : " << sweetenerBlueprint.rewardchoices_size();
      fighterDb.reset();
      return false;           
     }
     
     cookCost = sweetenerBlueprint.cookcost();
     duration = sweetenerBlueprint.duration();
     
     bool playerHasEnoughIngridients = true;

     for(const auto& candyNeeds: sweetenerBlueprint.rewardchoices(rewardID).requiredcandy())
     {
        std::string candyInventoryName = GetCandyKeyNameFromID(candyNeeds.first, ctx);
        
        if(InventoryHasItem(candyInventoryName, playerInventory, candyNeeds.second) == false)
        {
            playerHasEnoughIngridients = false;
        }
        
        pxd::Quantity quantity = candyNeeds.second;
        
        fungibleItemAmountForDeduction.insert(std::pair<std::string, pxd::Quantity>(candyInventoryName, quantity));
     }
    
     if(playerHasEnoughIngridients == false)
     {
        LOG (WARNING) << "Not enough candy ingridients to cook " << sweetenerID;
        fighterDb.reset();
        return false;         
     }    

     if(sweetenerBlueprint.requiredsweetness() <= fighterDb->GetProto().highestappliedsweetener())
     {
        LOG (WARNING) << "Already applied equal or higher sweetener to this fighter " << fighterID;
        fighterDb.reset();
        return false;          
     }         
     
     if(sweetenerBlueprint.requiredsweetness() > fighterDb->GetProto().sweetness())
     {
        LOG (WARNING) << "Fighter isn't sweet enough " << fighterID;
        fighterDb.reset();
        return false;          
     }   

     if(sweetenerBlueprint.requiredsweetness() != (fighterDb->GetProto().highestappliedsweetener()+1))
     {
        LOG (WARNING) << "Can sweeten only to next level first, no jumps over " << fighterID;
        fighterDb.reset();
        return false;          
     }   

     fighterDb.reset();
     return true;   
  }      

  bool
  BaseMoveProcessor::ParseCookRecepie(const XayaPlayer& a, const std::string& name, const Json::Value& recepie, std::map<std::string, pxd::Quantity>& fungibleItemAmountForDeduction, int32_t& cookCost, int32_t& duration, std::string& weHaveApplibeGoodyName)
  {
    if (!recepie.isObject ())
    return false;

    if(!recepie["rid"].isInt())
    {
        return false;
    }
   
    if(!recepie["fid"].isInt())
    {
        return false;
    }   

    const int32_t recepieID = (uint32_t)recepie["rid"].asInt();
    const int32_t fighterID = recepie["fid"].asInt();
    
    auto& playerInventory = a.GetInventory();
    
    auto itemInventoryRecipe = GetRecepieObjectFromID(recepieID);

    if(itemInventoryRecipe == nullptr)
    {
        LOG (WARNING) << "Could not resolve key name from the authid for the item: " << recepieID;
        return false;       
    }      
    
    if(itemInventoryRecipe->GetOwner() != a.GetName())
    {
        LOG (WARNING) << "Recipe does not belong to the player or is already in process " << recepieID;
        return false;           
    }

    cookCost = RecipeCookCostForQuality(itemInventoryRecipe->GetProto().quality(), ctx.RoConfig());

    if(cookCost == -1)
    {
        LOG (WARNING) << "Cooking cost could not be resolved for the instance named: " << recepieID;
        return false;          
    }
    
    if(a.GetBalance() < cookCost)
    {
        LOG (WARNING) << "No enough crystals on balance to cook: " << recepieID;
        return false;              
    }
    
    bool playerHasEnoughIngridients = true;

    for(const auto& candyNeeds: itemInventoryRecipe->GetProto().requiredcandy())
    {
        std::string candyInventoryName = GetCandyKeyNameFromID(candyNeeds.candytype(), ctx);
        
        if(InventoryHasItem(candyInventoryName, playerInventory, candyNeeds.amount()) == false)
        {
            playerHasEnoughIngridients = false;
            LOG (WARNING) << "Missing " << candyInventoryName << "needs" << candyNeeds.amount();
        }
        
        pxd::Quantity quantity = candyNeeds.amount();
        
        fungibleItemAmountForDeduction.insert(std::pair<std::string, pxd::Quantity>(candyInventoryName, quantity));
    }

    
    if(playerHasEnoughIngridients == false)
    {
        LOG (WARNING) << "Not enough candy ingridients to cook " << recepieID;
        return false;         
    }
        
    uint32_t slots = fighters.CountForOwner(a.GetName());

    if(slots > ctx.RoConfig()->params().max_fighter_inventory_amount())
    {
        LOG (WARNING) << "Not enough slots to host a new fighter for " << recepieID;
        return false;
    }
    
    if (itemInventoryRecipe->GetProto().requiredfighterquality() > 0 && fighterID <= 0)
    {
        LOG (WARNING) << "[CookRecipe] A fighter is required! " << recepieID;
        return false;  
    }
    if (itemInventoryRecipe->GetProto().requiredfighterquality() > 0 && fighterID > 0)
    {
        auto fighter = fighters.GetById(fighterID, ctx.RoConfig());
        
        if(fighter == nullptr)
        {
          LOG (WARNING) << "Wrong fighter ID " << fighterID;
          return false; 
        }
        
        if(fighter->GetOwner() != a.GetName())
        {
          LOG (WARNING) << "Fighter does not belong to player " << fighterID;
          fighter.reset();
          return false;             
        }
        
        if((pxd::FighterStatus)(int32_t)fighter->GetStatus() != pxd::FighterStatus::Available) 
        {
          LOG (WARNING) << "Fighter status is not available: " << fighterID;
          fighter.reset();
          return false;              
        }       
        
        if(fighter->GetProto().quality() < itemInventoryRecipe->GetProto().requiredfighterquality())
        {
          LOG (WARNING) << "Fighter quality is too low " << fighterID;
          fighter.reset();
          return false;             
        }        
    }    
    
    fpm::fixed_24_8 reductionPercent = fpm::fixed_24_8(1);
    const auto& goodiesList = ctx.RoConfig()->goodies();
    auto sortedGoodyTypesmap = SortPairsByKey(goodiesList);

    FindApplicableGoody(playerInventory, pxd::GoodyType::PressureCooker, sortedGoodyTypesmap, weHaveApplibeGoodyName, reductionPercent);
    
    fpm::fixed_24_8 effective_duration = fpm::fixed_24_8(itemInventoryRecipe->GetProto().duration());
    if(weHaveApplibeGoodyName != "")
    {
        effective_duration = effective_duration * reductionPercent;
    }    

    duration = (int32_t)effective_duration;

    return true;    
  } 

  bool
  BaseMoveProcessor::ParseDestroyRecepie(const XayaPlayer& a, const std::string& name, const Json::Value& recepie, std::vector<uint32_t>& recepieIDS)
  {
    if (!recepie.isObject ())
    return false;

    if(!recepie["rid"].isArray())
    {
        return false;
    }
   
    for(auto ft: recepie["rid"])
    {
      if (!ft.isInt ()) return false;
      recepieIDS.push_back(ft.asInt());

      auto itemInventoryRecipe = GetRecepieObjectFromID(ft.asInt());

      if(itemInventoryRecipe == nullptr)
      {
          LOG (WARNING) << "Could not resolve key name from the authid for the item: " << ft.asInt();
          return false;       
      }      
      
      if(itemInventoryRecipe->GetOwner() != a.GetName())
      {
          LOG (WARNING) << "Recipe does not belong to the player or is already in process " << ft.asInt();
          return false;           
      }
    }

    return true;    
  }

 void MoveProcessor::DestroyUsedElements(std::unique_ptr<pxd::XayaPlayer>& player, const Json::Value& fighter)
 {
	const auto& itemFighter = fighter["if"];
	const auto& itemCandy = fighter["ic"];
	const auto& itemRecipe = fighter["ir"];	 
	
    for(auto& ft : itemFighter)
	{
		fighters.DeleteById(ft.asInt());
	}
	
    for(auto& rc : itemRecipe)
	{
		recipeTbl.DeleteById(rc.asInt());
	}	
	
    for(auto& cd : itemCandy)
	{
        std::string candyAuth = cd["n"].asString();
		Amount ca = cd["a"].asInt();	
		
        player->GetInventory().AddFungibleCount(BaseMoveProcessor::GetCandyKeyNameFromID(candyAuth, ctx), -ca);
	}

	/* Sacrificing recipes may have freed recipe slots -> drain any held
	   recipe-overflow rewards into ownership. */
	PXLogic::DrainPendingRecipeRewards(player, ctx, db);
 }

  void
  MoveProcessor::MaybeCookSweetener (const std::string& name, const Json::Value& sweetener)
  {   
    std::map<std::string, pxd::Quantity> fungibleItemAmountForDeduction;
    int32_t cookCost = -1;
    uint32_t fighterID = -1;    
    int32_t duration = -1;
    std::string sweetenerKeyName = "";

    auto a = xayaplayers.GetByName (name, ctx.RoConfig ());
    
    if(a == nullptr)
    {
      LOG (INFO) << "Player " << name << " not found";
      return;
    }    
    
    if(!ParseSweetener(*a, name, sweetener, fungibleItemAmountForDeduction, cookCost, fighterID, duration, sweetenerKeyName)) return;
    
    auto& playerInventory = a->GetInventory();
    for(const auto& itemToDeduct: fungibleItemAmountForDeduction)
    {
        playerInventory.AddFungibleCount(itemToDeduct.first, -itemToDeduct.second);
    }
    
    playerInventory.AddFungibleCount(sweetenerKeyName, -1);
      
    a->AddBalance(-cookCost);    
  
    auto fighterDb = fighters.GetById (fighterID, ctx.RoConfig ());
    fighterDb->SetStatus(pxd::FighterStatus::Cooking);
    const uint32_t sweetenedRecipeId = fighterDb->GetProto().recipeid();
    fighterDb.reset();

    /** We need to make it at least 1 block, else if will make no sense executing immediately logic flow wise*/
    if(duration < 1)
    {
        duration = 1;
    }

    /* H3: ongoing row with absolute resolve height (recipeid captured before reset). */
    auto newOp = ongoings.CreateNew(ctx.Height());
    newOp->SetOwner(name);
    newOp->SetHeight(ctx.Height() + duration);
    newOp->MutableProto().set_type((uint32_t)pxd::OngoingType::COOK_SWEETENER);
    newOp->MutableProto().set_recipeid(sweetenedRecipeId);
    newOp->MutableProto().set_appliedgoodykeyname(sweetener["sid"].asString());
    newOp->MutableProto().set_rewardid(sweetener["rid"].asInt());
    newOp->MutableProto().set_fighterdatabaseid(fighterID);
    newOp.reset();
  }

  void
  MoveProcessor::MaybeDestroyRecepie (const std::string& name, const Json::Value& recepie)
  {
    auto a = xayaplayers.GetByName (name, ctx.RoConfig ());
    
    if(a == nullptr)
    {
      LOG (INFO) << "Player " << name << " not found";
      return;
    }    
    
    std::vector<uint32_t> recepieIDS;
    
    if(!ParseDestroyRecepie(*a, name, recepie, recepieIDS)) return;

    for(auto& rcp: recepieIDS)
    {
      /* H4: a destroyed inventory recipe is owned by the player and not yet
         cooked (ParseDestroyRecepie validated ownership), so nothing references
         it -- delete the row outright instead of orphaning it as owner=''. */
      recipeTbl.DeleteById(rcp);
    }

    /* A destroyed recipe frees a recipe slot -> drain any held recipe-overflow
       rewards into ownership. */
    PXLogic::DrainPendingRecipeRewards(a, ctx, db);

    a->CalculatePrestige(ctx.RoConfig());
    a.reset();
    LOG (INFO) << "Destroy instance " << recepie << " submitted successfully ";
  }    

  void
  MoveProcessor::MaybeCookRecepie (const std::string& name, const Json::Value& recepie)
  {      
    std::map<std::string, pxd::Quantity> fungibleItemAmountForDeduction;
    int32_t cookCost = -1;
    int32_t duration = -1;
    std::string weHaveApplibeGoodyName = "";
    
    auto a = xayaplayers.GetByName (name, ctx.RoConfig ());
    
    if(a == nullptr)
    {
      LOG (INFO) << "Player " << name << " not found";
      return;
    }    
    
    if(!ParseCookRecepie(*a, name, recepie, fungibleItemAmountForDeduction, cookCost, duration, weHaveApplibeGoodyName)) return;
    
    auto& playerInventory = a->GetInventory();
    for(const auto& itemToDeduct: fungibleItemAmountForDeduction)
    {
        playerInventory.AddFungibleCount(itemToDeduct.first, -itemToDeduct.second);
    }
      
    a->AddBalance(-cookCost);
    
    const int32_t fighterID = recepie["fid"].asInt();
    if(fighterID > 0)
    {
        auto fighter = fighters.GetById(fighterID, ctx.RoConfig());

        /* Only the player's OWN, Available fighter may be consumed.  Without
           this guard a quality-0 recipe (whose fighter is NOT validated in
           ParseCookRecepie) could delete any fighter by id: griefing, and a
           chain-halt when the victim is a tournament participant or mid-cook
           (dangling reference -> null deref at resolution). */
        if(fighter != nullptr
           && fighter->GetOwner() == name
           && (pxd::FighterStatus)(int32_t)fighter->GetStatus() == pxd::FighterStatus::Available)
        {
          /* H4: the replaced fighter is consumed; delete its source recipe
             (referenced only by this fighter) so it does not leak. */
          const uint32_t consumedRecipeId = fighter->GetProto().recipeid();
          fighters.DeleteById(fighter->GetId());
          if(consumedRecipeId > 0)
          {
            recipeTbl.DeleteById(consumedRecipeId);
          }

          a->CalculatePrestige(ctx.RoConfig());
        }
    }

    /** We need to make it at least 1 block, else if will make no sense executing immediately logic flow wise*/
    if(duration < 1)
    {
        duration = 1;
    }

    /* H3: ongoing row with absolute resolve height. */
    auto newOp = ongoings.CreateNew(ctx.Height());
    newOp->SetOwner(name);
    newOp->SetHeight(ctx.Height() + duration);
    newOp->MutableProto().set_type((uint32_t)pxd::OngoingType::COOK_RECIPE);
    newOp->MutableProto().set_recipeid(recepie["rid"].asInt());

    auto itemInventoryRecipe = GetRecepieObjectFromID((uint32_t)recepie["rid"].asInt());
    itemInventoryRecipe->SetOwner("");
    itemInventoryRecipe.reset();

    /* Starting a cook de-owns the recipe -> a recipe slot just freed, so drain
       any held recipe-overflow rewards into ownership. */
    PXLogic::DrainPendingRecipeRewards(a, ctx, db);

    if(weHaveApplibeGoodyName != "")
    {
        playerInventory.AddFungibleCount(weHaveApplibeGoodyName, -1);
        newOp->MutableProto().set_appliedgoodykeyname(weHaveApplibeGoodyName);
    }
    newOp.reset();

    a->CalculatePrestige(ctx.RoConfig());
    a.reset();
    LOG (INFO) << "Cooking instance " << recepie << " submitted successfully ";
  }  

} // namespace pxd

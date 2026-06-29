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

/* Fighter transfigure / deconstruct and fuel-power computation.
   Split out of moveprocessor.cpp; part of pxd::MoveProcessor. */

namespace pxd
{

  bool BaseMoveProcessor::ParseDeconstructRewardData(const XayaPlayer& a, const Json::Value& fighter, uint32_t& fighterID)
  {
    if (!fighter.isObject ())
    {
      VLOG (1) << "Fighter is not an object" << fighter;
      return false;
    }

    if(!fighter["fid"].isInt())
    {
      LOG (WARNING) << "fid is not an int";
      return false;
    }
    
    fighterID = fighter["fid"].asInt();
    
    auto fighterDb = fighters.GetById (fighterID, ctx.RoConfig ());
    
    if(fighterDb == nullptr)
    {
      LOG (WARNING) << "Fatal erorr, could not get fighter with ID" << fighterID;
      fighterDb.reset();
      return false;                
    }
    
    if(fighterDb->GetOwner() != a.GetName())
    {
      LOG (WARNING) << "Fighter does not belong to: " << a.GetName();
      fighterDb.reset();
      return false;               
    }    
    
    if((pxd::FighterStatus)(int32_t)fighterDb->GetStatus() != pxd::FighterStatus::Deconstructing) 
    {
      LOG (WARNING) << "Fighter status is not in deconstruction: " << fighterID;
      fighterDb.reset();
      return false;              
    }    

    bool isFinished = true;
    
    {
      auto ores = ongoings.QueryForOwner(a.GetName());
      while(ores.Step())
      {
        auto op = ongoings.GetFromResult(ores);
        if(op->GetProto().fighterdatabaseid() == fighterID && op->GetProto().type() == (uint32_t)pxd::OngoingType::DECONSTRUCTION)
        {
            isFinished = false;
        }
      }
    }

    if(isFinished == false)
    {
      LOG (WARNING) << "Ongoing operation still presetent for fighter with id: " << fighterID;
      fighterDb.reset();
      return false;           
    }

    fighterDb.reset(); 
    return true;    
  }    

  bool BaseMoveProcessor::ParseTransfigureData(const XayaPlayer& a, const Json::Value& fighter)
  {
    if (!fighter.isObject ())
    {
      VLOG (1) << "Fighter is not an object";
      return false;
    }
	
    if(!fighter["fid"].isInt())
    {
      LOG (WARNING) << "fid is not an int";
      return false;
    }

    if(!fighter["o"].isInt())
    {
      LOG (WARNING) << "o is not an int";
      return false;
    }
	
    if(!fighter["if"].isArray())
    {
      LOG (WARNING) << "il is not an Array";
      return false;
    }	
	
    if(!fighter["ic"].isArray())
    {
      LOG (WARNING) << "ic is not an Array";
      return false;
    }

    if(!fighter["ir"].isArray())
    {
      LOG (WARNING) << "ir is not an Array";
      return false;
    }	

    auto fighterID = fighter["fid"].asInt();
    auto optionID = fighter["o"].asInt();	
	const auto& itemFighter = fighter["if"];
	const auto& itemCandy = fighter["ic"];
	const auto& itemRecipe = fighter["ir"];

	/* DoS cap (H1): bound the duplicate-detection and per-element work on the
	   attacker-supplied sacrifice arrays.  Real transfigures use a handful. */
	if(itemFighter.size() > MAX_MOVE_ARRAY
	   || itemCandy.size() > MAX_MOVE_ARRAY
	   || itemRecipe.size() > MAX_MOVE_ARRAY)
	{
	  LOG (WARNING) << "Transfigure sacrifice arrays exceed the size cap";
	  return false;
	}

	if(itemFighter.size() == 0)
	{
      LOG (WARNING) << "Fighter array is null";
      return false; 		
	}
	
    std::vector<uint32_t> fighterIDS;
    for (const auto& ft : itemFighter)
    {
      if (!ft.isInt ())
      {
        return false; 
      }
      
      fighterIDS.push_back(ft.asInt());
    }	
	
	if(HasDuplicates(fighterIDS))
	{
	  LOG (WARNING) << "Entry contained fighter duplicate ids";
	  return false;             
	}   

    std::vector<uint32_t> itemRecipeIDS;
    for (const auto& rc : itemRecipe)
    {
      if (!rc.isInt ())
      {
        return false; 
      }
      
      itemRecipeIDS.push_back(rc.asInt());
    }	
	
	if(HasDuplicates(itemRecipeIDS))
	{
	  LOG (WARNING) << "Entry contained fighter duplicate ids";
	  return false;             
	} 

    std::vector<std::string> itemCandyIDS;
    for (const auto& cd : itemCandy)
    {
        if (!cd.isObject ())
		{
		  VLOG (1) << "candy is not an object";
		  return false;
		}	

		if (!cd["a"].isInt ())
		{
		  VLOG (1) << "candy amount not integer";
		  return false;
		}	

		if (!cd["n"].isString ())
		{
		  VLOG (1) << "candy authid is not string";
		  return false;
		}		
      
      itemCandyIDS.push_back(cd["n"].asString());
    }	
	
	if(HasDuplicates(itemCandyIDS))
	{
	  LOG (WARNING) << "Entry contained fighter duplicate ids";
	  return false;             
	} 		
	
    auto fighterDb = fighters.GetById (fighterID, ctx.RoConfig ());
    
    if(fighterDb == nullptr)
    {
      LOG (WARNING) << "Fatal erorr, could not get fighter with ID" << fighterID;
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
      LOG (WARNING) << "Fighter status is busy for decmposition with id: " << fighterID;
      fighterDb.reset();
      return false;              
    }    	
	fighterDb.reset();
	
	if(optionID <=0 || optionID > 3)
	{
      LOG (WARNING) << "Option ID is out of range" << optionID;
      return false;   		
	}
	
	for(auto& ft : itemFighter)
	{
		if(!ft.isInt()) return false;
		
		auto fighterToSactifice = fighters.GetById (ft.asInt(), ctx.RoConfig ());
		if(fighterToSactifice == nullptr)
		{
		  LOG (WARNING) << "Fatal erorr, could not get fighter with ID" << ft;
		  return false;                
		}
		
		if(fighterToSactifice->GetOwner() != a.GetName())
		{
		  LOG (WARNING) << "Fighter does not belong to: " << a.GetName();
		  fighterToSactifice.reset();
		  return false;               
		}    
		
		if((pxd::FighterStatus)(int32_t)fighterToSactifice->GetStatus() != pxd::FighterStatus::Available)
		{
		  LOG (WARNING) << "Fighter status is busy for decomposition with id: " << ft;
		  fighterToSactifice.reset();
		  return false;              
		}    	
		fighterToSactifice.reset();			
	}
	
    for(auto& candy : itemCandy)
	{
		if (!candy.isObject ())
		{
		  VLOG (1) << "candy is not an object";
		  return false;
		}	

		if (!candy["a"].isInt ())
		{
		  VLOG (1) << "candy amount not integer";
		  return false;
		}	

		if (!candy["n"].isString ())
		{
		  VLOG (1) << "candy authid is not string";
		  return false;
		}		
 
        int32_t ca = candy["a"].asInt();
        if(a.GetInventory().GetFungibleCount(BaseMoveProcessor::GetCandyKeyNameFromID(candy["n"].asString(), ctx)) < ca)
			
        {
		  LOG (WARNING) << "Not sufficient candy amount, got " << a.GetInventory().GetFungibleCount(BaseMoveProcessor::GetCandyKeyNameFromID(candy["n"].asString(), ctx)) << " while needs " << ca << " for candy " << candy["n"].asString();
		  return false;   			
		}		

        if(ca < 10)
		{
		  LOG (WARNING) << "Minimum pack of candy is 10";
		  return false;   				
		}			
	}
     
	for(auto& recepieID : itemRecipe)
	{
		if (!recepieID.isInt ())
		{
		  VLOG (1) << "recepieID is not integer";
		  return false;
		}	
		
		auto itemInventoryRecipe = GetRecepieObjectFromID(recepieID.asInt());

		if(itemInventoryRecipe == nullptr)
		{
			LOG (WARNING) << "Could not resolve key name from the authid for the item: " << recepieID.asInt();
			itemInventoryRecipe.reset();
			return false;       
		}      
		
		if(itemInventoryRecipe->GetOwner() != a.GetName())
		{
			LOG (WARNING) << "Recipe does not belong to the player or is already in process ";
			itemInventoryRecipe.reset();
			return false;           
		}

        itemInventoryRecipe.reset();		
	}
	 
    return true; 	
  }

  std::vector<pxd::ArmorType> BaseMoveProcessor::ArmorTypeFromMoveType(pxd::MoveType moveType)
  {
	std::vector<pxd::ArmorType> pieceList;
    
	  switch(moveType) 
	  {
		 case pxd::MoveType::Heavy:
			pieceList.push_back(pxd::ArmorType::Head);
			pieceList.push_back(pxd::ArmorType::RightShoulder);
			pieceList.push_back(pxd::ArmorType::LeftShoulder);
			break;
		 case pxd::MoveType::Speedy:
			pieceList.push_back(pxd::ArmorType::UpperRightArm);
			pieceList.push_back(pxd::ArmorType::LowerRightArm);
			pieceList.push_back(pxd::ArmorType::UpperLeftArm);
			pieceList.push_back(pxd::ArmorType::LowerLeftArm);
			break;
		 case pxd::MoveType::Tricky:
			pieceList.push_back(pxd::ArmorType::RightHand);
			pieceList.push_back(pxd::ArmorType::Torso);
			pieceList.push_back(pxd::ArmorType::LeftHand);
			break;
		 case pxd::MoveType::Distance:
			pieceList.push_back(pxd::ArmorType::Waist);
			pieceList.push_back(pxd::ArmorType::UpperRightLeg);
			pieceList.push_back(pxd::ArmorType::UpperLeftLeg);
			break;
		 case pxd::MoveType::Blocking:
			pieceList.push_back(pxd::ArmorType::LowerRightLeg);
			pieceList.push_back(pxd::ArmorType::RightFoot);
			pieceList.push_back(pxd::ArmorType::LeftFoot);
			break;        
		
		 // you can have any number of case statements.
		 default : //Optional
			LOG (WARNING) << "Default should not be triggered for the moveType of type " << (uint32_t)moveType;
	  }

	  return pieceList;  
  }  

  bool BaseMoveProcessor::ParseDeconstructData(const XayaPlayer& a, const Json::Value& fighter, uint32_t& fighterID)
  {
    if (!fighter.isObject ())
    {
      VLOG (1) << "Fighter is not an object";
      return false;
    }

    if(!fighter["fid"].isInt())
    {
      LOG (WARNING) << "fid is not an int";
      return false;
    }
    
    fighterID = fighter["fid"].asInt();
    
    auto fighterDb = fighters.GetById (fighterID, ctx.RoConfig ());
    
    if(fighterDb == nullptr)
    {
      LOG (WARNING) << "Fatal erorr, could not get fighter with ID" << fighterID;
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
      LOG (WARNING) << "Fighter status is busy for decmposition with id: " << fighterID;
      fighterDb.reset();
      return false;              
    }    
    
    uint32_t totalF = fighters.CountForOwner(a.GetName());
    
    if(totalF == 1)
    {
      LOG (WARNING) << "Can not remove last roster fighter: " << totalF;
      fighterDb.reset();
      return false;           
    }

    fighterDb.reset(); 
    return true;    
  }    

 void MoveProcessor::MaybeClaimDeconstructionReward (const std::string& name, const Json::Value& fighter)
 {      
    auto a = xayaplayers.GetByName (name, ctx.RoConfig ());
    
    if(a == nullptr)
    {
      LOG (INFO) << "Player " << name << " not found";
      return;
    }
    
    uint32_t fighterID = 0;
    if(!ParseDeconstructRewardData(*a, fighter, fighterID))      
    {
      a.reset();
      return;
    }

    std::vector<uint32_t> rewardDatabaseIds;
    auto res = rewards.QueryForOwner(a->GetName());
    
    auto fighterDb = fighters.GetById (fighterID, ctx.RoConfig ());
    /* H4: capture the recipe this fighter was cooked from; once the fighter is
       deleted below nothing else references it, so it must be deleted too. */
    const uint32_t deconstructedRecipeId = (fighterDb != nullptr) ? fighterDb->GetProto().recipeid() : 0;
    bool tryAndStep = res.Step();
    while (tryAndStep)
    {
      auto rw = rewards.GetFromResult (res, ctx.RoConfig ());
      
      if(rw->GetProto().fighterid() == fighterID)
      {
        for(const auto& reward: rw->GetProto().deconstructions())
        {
          if(fighterDb->GetProto().isaccountbound() == false)
          {
             a->GetInventory().AddFungibleCount(reward.candytype(), reward.quantity()); 
          }
        }
          
        rewardDatabaseIds.push_back(rw->GetId());
      }
      
      rw.reset();
      tryAndStep = res.Step ();
    }      
    
    fighterDb.reset();
    
    for(const auto& rw: rewardDatabaseIds)
    {
      rewards.DeleteById(rw);
    }
    
    fighters.DeleteById(fighterID);
    if(deconstructedRecipeId > 0)
    {
      recipeTbl.DeleteById(deconstructedRecipeId);
    }

    a->CalculatePrestige(ctx.RoConfig());
    a.reset();
    
    
  } 

 fpm::fixed_24_8 MoveProcessor::CalculateFuelPower(const Json::Value& fighter, const Json::Value& wholeFighterData, const Json::Value& wholeRecipeData, bool outputDebug)
 {
    // Now that recipe formula is validated, lets calculates all the raw valies first
	
	if(outputDebug)
	{
		 LOG (WARNING) << "STARTING FUEL CALCULATIONS --------------------- ";  
	}
	
	fpm::fixed_24_8 fuel20 = fpm::fixed_24_8(0);
	fpm::fixed_24_8 fuel80 = fpm::fixed_24_8(0);
	
	const auto& itemFighter = fighter["if"];
	const auto& itemCandy = fighter["ic"];
	const auto& itemRecipe = fighter["ir"];	
	
	std::map<fpm::fixed_24_8, fpm::fixed_24_8> qualitiesUsedInTransfigure;
	std::map<fpm::fixed_24_8, fpm::fixed_24_8> sweetnersUsedInTransfigure;
	
	fpm::fixed_24_8 ratingsUsedInTransfigureAverage = fpm::fixed_24_8(0);
	fpm::fixed_24_8 ratingsUsedInTransfigureMin = fpm::fixed_24_8(9999999);
	fpm::fixed_24_8 ratingsUsedInTransfigureMax = fpm::fixed_24_8(-9999999);
	
	std::vector<std::string> namesUsed;
	std::vector<std::string> armorUsed;
	
    for(auto& ft : itemFighter)
	{
		if(outputDebug) LOG (WARNING) << "Evaluating fighter  --------------------- " << ft.asInt();  
		
	    std::stringstream keySS;
	    keySS << ft.asInt();
	    std::string keySSStr = keySS.str();
   
		auto fighterToSactifice = wholeFighterData[keySSStr];	
		
		// Fighter crystal cost
		fpm::fixed_24_8 basedQualityCost = fpm::fixed_24_8(0);
		fpm::fixed_24_8 basedQualityLevel = fpm::fixed_24_8(fighterToSactifice["q"].asInt());
		
		if(basedQualityLevel == fpm::fixed_24_8(1))
		{
			basedQualityCost = fpm::fixed_24_8(wholeFighterData["crcc"].asInt());
		}
		
		if(basedQualityLevel == fpm::fixed_24_8(2))
		{
			basedQualityCost = fpm::fixed_24_8(wholeFighterData["urcc"].asInt());
		}

		if(basedQualityLevel == fpm::fixed_24_8(3))
		{
			basedQualityCost = fpm::fixed_24_8(wholeFighterData["rrcc"].asInt());		
			// and common sacrificed
			basedQualityCost += fpm::fixed_24_8(wholeFighterData["crcc"].asInt());
			
		}

		if(basedQualityLevel == fpm::fixed_24_8(4))
		{
			basedQualityCost = fpm::fixed_24_8(wholeFighterData["ercc"].asInt());
			// and common sacrificed
			basedQualityCost += fpm::fixed_24_8(wholeFighterData["urcc"].asInt());			
		}		
		
	    fpm::fixed_24_8 appliedSW = fpm::fixed_24_8(fighterToSactifice["has"].asInt());
		fpm::fixed_24_8 sumToAdd = (appliedSW-basedQualityLevel) * fpm::fixed_24_8(50);
	
	    if(outputDebug) LOG (WARNING) << "basedQualityCost: " << (int32_t)basedQualityCost;  
		if(outputDebug) LOG (WARNING) << "sumToAdd: " << (int32_t)sumToAdd;
	
	    // We have our final raw crystal worth value now
		fpm::fixed_24_8 totalCrystalCost =  (basedQualityCost + sumToAdd);
		
		// Now we need to calculate diversity coefficient, based on:	
		//Quality, Rating, HighestAppliedSweetener, Name, ArmorPieces, 	
		// Based on rarity, we have different starting values
		// Epic always starts from 0.25;
		
		fpm::fixed_24_8 diversityFinalCoefficient = fpm::fixed_24_8(0);
		
		// Base
		if(basedQualityLevel == fpm::fixed_24_8(1))
		{
			diversityFinalCoefficient = fpm::fixed_24_8(10);
		}
		
		if(basedQualityLevel == fpm::fixed_24_8(2))
		{
			diversityFinalCoefficient = fpm::fixed_24_8(50);
		}

		if(basedQualityLevel == fpm::fixed_24_8(3))
		{
			diversityFinalCoefficient = fpm::fixed_24_8(100);
		}			
		
		if(basedQualityLevel == fpm::fixed_24_8(4))
		{
			diversityFinalCoefficient = fpm::fixed_24_8(250);
		}		
		
		// Quality
        if (qualitiesUsedInTransfigure.find(basedQualityLevel) == qualitiesUsedInTransfigure.end())
        {
            qualitiesUsedInTransfigure.insert(std::pair<fpm::fixed_24_8, fpm::fixed_24_8>(basedQualityLevel, fpm::fixed_24_8(1)));
        }
        else
        {
            qualitiesUsedInTransfigure[basedQualityLevel] = qualitiesUsedInTransfigure[basedQualityLevel] + fpm::fixed_24_8(1);
        }		
		fpm::fixed_24_8 qualityDiversitiCoefficient =  fpm::fixed_24_8(1) / (qualitiesUsedInTransfigure[basedQualityLevel] * qualitiesUsedInTransfigure[basedQualityLevel]);
		
		// Rating
		fpm::fixed_24_8 currentRating = fpm::fixed_24_8(fighterToSactifice["r"].asInt());
		fpm::fixed_24_8 rDiffAverage = fpm::abs(ratingsUsedInTransfigureAverage - currentRating) ;
		fpm::fixed_24_8 rDiffMin = fpm::abs(currentRating - ratingsUsedInTransfigureMin);
		fpm::fixed_24_8 rDiffMax = fpm::abs(currentRating - ratingsUsedInTransfigureMax);
		
		fpm::fixed_24_8 rMinimalRatingDiff = rDiffAverage;
		
		if(rDiffMin < rMinimalRatingDiff)
		{
			rMinimalRatingDiff = rDiffMin;
		}
		
		if(rDiffMax < rMinimalRatingDiff)
		{
			rMinimalRatingDiff = rDiffMax;
		}		
		
		if(ratingsUsedInTransfigureAverage == fpm::fixed_24_8(0))
		{
			ratingsUsedInTransfigureAverage = currentRating;
		}
		else
		{
		    ratingsUsedInTransfigureAverage=  (ratingsUsedInTransfigureAverage + currentRating) / fpm::fixed_24_8(2);
		}
		
		if(currentRating < ratingsUsedInTransfigureMin)
		{
			ratingsUsedInTransfigureMin = currentRating;
		}
		
		if(currentRating > ratingsUsedInTransfigureMax)
		{
			ratingsUsedInTransfigureMax = currentRating;
		}	
		
		fpm::fixed_24_8 ratingDiversitiCoefficient  = rMinimalRatingDiff / fpm::fixed_24_8(1000);
		
		// HighestAppliedSweetner

        fpm::fixed_24_8 basedSweetLevel = fpm::fixed_24_8(fighterToSactifice["has"].asInt());
        if (sweetnersUsedInTransfigure.find(basedSweetLevel) == sweetnersUsedInTransfigure.end())
        {
            sweetnersUsedInTransfigure.insert(std::pair<fpm::fixed_24_8, fpm::fixed_24_8>(basedSweetLevel, fpm::fixed_24_8(1)));
        }
        else
        {
            sweetnersUsedInTransfigure[basedSweetLevel] = sweetnersUsedInTransfigure[basedSweetLevel] + fpm::fixed_24_8(1);
        }		
		fpm::fixed_24_8 sweetnerDiversitiCoefficient =  fpm::fixed_24_8(1) / (sweetnersUsedInTransfigure[basedSweetLevel] * sweetnersUsedInTransfigure[basedSweetLevel]);		
		
		// Name	
		std::vector<std::string> output;
		std::string originalName = fighterToSactifice["n"].asString();
		std::vector<std::string> outputNamePieces;
		std::string::size_type prev_pos = 0, pos = 0;

		while((pos = originalName.find(' ', pos)) != std::string::npos)
		{
			std::string substring( originalName.substr(prev_pos, pos-prev_pos) );
			output.push_back(substring);
			prev_pos = ++pos;
		}
		output.push_back(originalName.substr(prev_pos, pos-prev_pos));
		
		fpm::fixed_24_8 uniquePiecesCollected = fpm::fixed_24_8(0);
		
		for(auto& piece : output)
		{
			if(std::find(namesUsed.begin(), namesUsed.end(), piece) == namesUsed.end()) 
			{
				uniquePiecesCollected = uniquePiecesCollected + fpm::fixed_24_8(1);
				namesUsed.push_back(piece);
			}
		}	

        if(outputDebug) LOG (WARNING) << "namesUsed: " << (int32_t)namesUsed.size();
		
		fpm::fixed_24_8 nameDiversitiCoefficient = (uniquePiecesCollected * fpm::fixed_24_8(1)) / fpm::fixed_24_8(8);
		
		// ArmorPieces		
		fpm::fixed_24_8 uniqueArmorCollected = fpm::fixed_24_8(0);
		
		for(auto& armorPiece : fighterToSactifice["ap"])
		{
		    if(std::find(armorUsed.begin(), armorUsed.end(), armorPiece["c"].asString()) == armorUsed.end()) 
			{
				uniqueArmorCollected = uniqueArmorCollected + fpm::fixed_24_8(1);
				armorUsed.push_back(armorPiece["c"].asString());
			}				
		}
		
		if(outputDebug) LOG (WARNING) << "armorUsed: " << (int32_t)armorUsed.size();
		
		fpm::fixed_24_8 armorDiversitiCoefficient = (uniqueArmorCollected* fpm::fixed_24_8(1)) / fpm::fixed_24_8(8);
		
		
		if(outputDebug) LOG (WARNING) << "armorDiversitiCoefficient: " << (int32_t)(armorDiversitiCoefficient *  fpm::fixed_24_8(1000));
		if(outputDebug) LOG (WARNING) << "nameDiversitiCoefficient: " << (int32_t)(nameDiversitiCoefficient *  fpm::fixed_24_8(1000));
		if(outputDebug) LOG (WARNING) << "sweetnerDiversitiCoefficient: " << (int32_t)(sweetnerDiversitiCoefficient *  fpm::fixed_24_8(1000));
		if(outputDebug) LOG (WARNING) << "qualityDiversitiCoefficient: " << (int32_t)(qualityDiversitiCoefficient *  fpm::fixed_24_8(1000));
		if(outputDebug) LOG (WARNING) << "ratingDiversitiCoefficient: " << (int32_t)(ratingDiversitiCoefficient *  fpm::fixed_24_8(1000));
		
		diversityFinalCoefficient = diversityFinalCoefficient + armorDiversitiCoefficient + nameDiversitiCoefficient + sweetnerDiversitiCoefficient + qualityDiversitiCoefficient + ratingDiversitiCoefficient;
		
		if(diversityFinalCoefficient > fpm::fixed_24_8(1))
		{
			diversityFinalCoefficient = fpm::fixed_24_8(1);
		}
		
		if(diversityFinalCoefficient < fpm::fixed_24_8(0))
		{
			diversityFinalCoefficient = fpm::fixed_24_8(0);
		}
		
		if(outputDebug) LOG (WARNING) << "totalCrystalCost: " << (int32_t)totalCrystalCost;  
		if(outputDebug) LOG (WARNING) << "diversityFinalCoefficient: " << (int32_t)(diversityFinalCoefficient *  fpm::fixed_24_8(1000));  
		
		totalCrystalCost = totalCrystalCost * diversityFinalCoefficient;
        fuel80 = fuel80 + totalCrystalCost;
		
		if(outputDebug) LOG (WARNING) << "fuel80: " << (int32_t)(fuel80 *  fpm::fixed_24_8(1000));		
	}
	
	// now that we have fuel80 from sacrificed treats, lets look into recepies and candies
	
	fpm::fixed_24_8 candyRarityAccumulated = fpm::fixed_24_8(0);

    for(auto& candy : itemCandy)
	{
		fpm::fixed_24_8 cRarity = fpm::fixed_24_8(0.1);
		
        fpm::fixed_24_8 ca = fpm::fixed_24_8(candy["a"].asInt());		
		fpm::fixed_24_8 candySets = ca / fpm::fixed_24_8(10);

		candyRarityAccumulated = candyRarityAccumulated + (candySets * cRarity);	

        if(outputDebug) LOG (WARNING) << "candyRarityAccumulated: " << (int32_t)(candyRarityAccumulated *  fpm::fixed_24_8(1000));		
	}
	
	// Recipe difference coefficient should be simple, we do not want to overcomplicate 20% calculation for no reason
	// when main incomes comes from treats, so lets evaluate recipe quality only here
	
	fpm::fixed_24_8 recipeRarityAccumulated = fpm::fixed_24_8(0);
	std::map<fpm::fixed_24_8, fpm::fixed_24_8> qualitiesUsedInTransfigureRecipe;

    for(auto& recepieID : itemRecipe)
	{
		std::stringstream keySS;
	    keySS << recepieID.asInt();
	    std::string keySSStr = keySS.str();
		auto itemInventoryRecipe = wholeRecipeData[keySSStr];
        fpm::fixed_24_8 recipeQual = fpm::fixed_24_8(itemInventoryRecipe["q"].asInt());
		
        if (qualitiesUsedInTransfigureRecipe.find(recipeQual) == qualitiesUsedInTransfigureRecipe.end())
        {
            qualitiesUsedInTransfigureRecipe.insert(std::pair<fpm::fixed_24_8, fpm::fixed_24_8>(recipeQual, fpm::fixed_24_8(1)));
        }
        else
        {
            qualitiesUsedInTransfigureRecipe[recipeQual] = qualitiesUsedInTransfigureRecipe[recipeQual] + fpm::fixed_24_8(1);
        }

		recipeRarityAccumulated = recipeRarityAccumulated + ((fpm::fixed_24_8(1) / (qualitiesUsedInTransfigureRecipe[recipeQual]))) * 10 * (recipeQual * recipeQual);	
		
		if(outputDebug) LOG (WARNING) << "recipeRarityAccumulated: " << (int32_t)(recipeRarityAccumulated *  fpm::fixed_24_8(1000));
	}	
	
	// Now we can construct final fuel_20 value
	
	fuel20 = recipeRarityAccumulated + candyRarityAccumulated;
	
	if(outputDebug) LOG (WARNING) << "fuel20: " << (int32_t)(fuel20 *  fpm::fixed_24_8(1000));
	
	// so, fuel80 is totcal crustal cost, and fuel20 is rarity	
	// Firstly, lets imagine, how much fule it would be if 80 were actually 20
	
	fpm::fixed_24_8 imaginary100 = fuel80 * fpm::fixed_24_8(1.25);
	fpm::fixed_24_8 missingFuelPoints = imaginary100 - fuel80;
	
	if(outputDebug) LOG (WARNING) << "imaginary100: " << (int32_t)(imaginary100 *  fpm::fixed_24_8(1000));
	
	if(missingFuelPoints > fuel20)
	{
		missingFuelPoints = fuel20;
	}

	fpm::fixed_24_8 fuelPower = (fuel80 + missingFuelPoints) *  fpm::fixed_24_8(100);
	if(outputDebug) LOG (WARNING) << "fuelPower: " << (int32_t)(fuelPower);

	return fuelPower;
 }

 void MoveProcessor::MaybeTransfigureFighter (const std::string& name, const Json::Value& fighter)
 {      
    auto a = xayaplayers.GetByName (name, ctx.RoConfig ());
    
    if(a == nullptr)
    {
      LOG (INFO) << "Player " << name << " not found";
      return;
    }
	
    // Firstly, player selects what we wants to transmodify
    // 0 - Name
    // 1 - Move
    // 2 - Armor

    // Then, player needs to fill up FUEL meter
	// 80% of FUEL meter consists of treat sacrifices
	// 20% of FUEL meter consists of candy and recipe sacrifices
	
	// When fuel meter is 100% filled, it allows to transfigure
	// For names, it changes name to random COMMON name
	// For armor, it changes random armor piece to random armor from COMMON setstate
	
	// Fueling further on, FUEL meter will reach UNCOMMON, EPIC, RARE states on each other 100% fueling
	// For fuelling 100% once, player needs to spend [100?] crystal worth resources
	// Using same item type for fuelling returning deminishing returns (exponentially), the more diverse and unique are items, the cheaper is to fuel
	// Item diversity is calculated comparing all same type item parameters and finding out, how many of those differs from those which are already in (candys goes in bunch of 10)
	// For each item, tooltip will show on top of it, how much FUEL it can add based on current context
	// When fueling meter reaches EPIC, this means that:
	
	// Player now has option to also change move with 10% chance
	// Filling the meter will once more will get it into OVERDRIVE mode, adding another extra 10%
	// Its almost never possible to reach 100% due to diminishing retursn from running out of diversity for average player in normal circumstances
	
	// For filling 80%, we are calculating sacrifices treate CRYSTAL COST
	// For filling 20&, we are using item universal CHANCE, the lower is the CHANCE, the more it fills up
	// So far we can see, that ORDER of submited items might matter
	
	// Pseudocode for running the procedure looks as follow:
	// Loop through all sacrificed treats
	// For each of the treats, calculate its total crystal cost
	// Start adding total_80_fuel += treats_cost * diversity_coefficient;
	
	// Loop through all sacrificed candies and recipes
	// For each of the them, fetch rarity_value, candy goes in bunches of 10, 1 recipe is equal to 100 candies, so based unit is candy
	// Start adding total_20_fuel += (1 - rarity_value) * diversity_coefficient;
	
	// The lowest values of both total_80_fuel and total_20_fuel is a final value
	// int transfigurationPower = final_fuel_value / 100; [100 is based crystal cost, which needs to be balanced empirically]
	// if transfigurationPower == 1 then COMMON, e.t.c.
	
    if(!ParseTransfigureData(*a, fighter))      
    {
      a.reset();
      return;
    }	
	
	
	Json::Value wholeFightersData(Json::objectValue);
	Json::Value wholeRecipeData(Json::objectValue);

	const auto& itemFighter = fighter["if"];
	const auto& itemRecipe = fighter["ir"];		
	
	for(auto& ft : itemFighter)
	{
	    std::stringstream keySS;
	    keySS << ft.asInt();
	    std::string keySSStr = keySS.str();
		
		auto fighterDb2 = fighters.GetById (ft.asInt(), ctx.RoConfig ());
		
		wholeFightersData[keySSStr]["q"] = fighterDb2->GetProto().quality();
		wholeFightersData[keySSStr]["has"] = fighterDb2->GetProto().highestappliedsweetener();
		wholeFightersData[keySSStr]["r"] = fighterDb2->GetProto().rating();
		wholeFightersData[keySSStr]["n"] = fighterDb2->GetProto().name();
		
		Json::Value armorPieces(Json::arrayValue);
		for(auto& ap : fighterDb2->GetProto().armorpieces())
	    {
			Json::Value piece(Json::objectValue);
			piece["c"] = ap.candy();
			armorPieces.append(piece);
		}
	
		wholeFightersData[keySSStr]["ap"] = armorPieces;
		
		fighterDb2.reset();
	}
	
	for(auto& ir : itemRecipe)
	{
		std::stringstream keySS;
	    keySS << ir.asInt();
	    std::string keySSStr = keySS.str();
		
		auto recp = recipeTbl.GetById (ir.asInt());
		wholeRecipeData[keySSStr]["q"] = recp->GetProto().quality();
		recp.reset();
	}	
	
	/* CalculateFuelPower reads these by the sacrificed fighter's quality
	   (crcc=common q1, urcc=uncommon q2, rrcc=rare q3, ercc=epic q4); they must
	   use the matching per-rarity cost, not all-uncommon. */
	wholeFightersData["crcc"] = ctx.RoConfig()->params().common_recipe_cook_cost();
	wholeFightersData["urcc"] = ctx.RoConfig()->params().uncommon_recipe_cook_cost();
	wholeFightersData["rrcc"] = ctx.RoConfig()->params().rare_recipe_cook_cost();
	wholeFightersData["ercc"] = ctx.RoConfig()->params().epic_recipe_cook_cost();
	
	fpm::fixed_24_8 fuelPower = CalculateFuelPower(fighter, wholeFightersData, wholeRecipeData, false);

	// Now that we have our value, we can finally try and transifure the input_iterator
    fpm::fixed_24_8 fuelPowerUnit = fpm::floor(fuelPower / 25000);
	
	auto fighterID = fighter["fid"].asInt();
	auto fighterDb = fighters.GetById (fighterID, ctx.RoConfig ());	
	
	if(fuelPowerUnit >= fpm::fixed_24_8(1))
	{
		auto optionID = fighter["o"].asInt();
		
		LOG (WARNING) << "Rolling option: " << optionID;
		
		// Name	
		if(optionID == 1)
		{
			// fuelPowerUnit influences, which quality of the name we get
			
			auto nQL = pxd::Quality::Common;
			
			Amount cost = 84000000;
			
			if(fuelPowerUnit == fpm::fixed_24_8(2))
			{
				nQL = pxd::Quality::Uncommon;
			}
			else if(fuelPowerUnit == fpm::fixed_24_8(3))
			{
				nQL = pxd::Quality::Rare;
			}		
			else if(fuelPowerUnit >= fpm::fixed_24_8(4))
			{
				nQL = pxd::Quality::Epic;		
				static const std::vector<Amount> lookUpPrices =  {84000000,252000000,672000000,924000000,1344000000,1764000000,2268000000,2772000000,3276000000,3948000000,4620000000,5376000000,5796000000,6216000000,6720000000,7140000000,7644000000,8148000000,8736000000,9324000000,9912000000,10500000000,10836000000,11172000000,11508000000,11844000000,12180000000,12516000000,12852000000,13188000000,13608000000,13944000000,14364000000,14784000000,15120000000,15540000000,15960000000,16380000000,16800000000,17220000000,17724000000,18144000000,18480000000,18900000000,19236000000,19656000000,19992000000,20412000000,20832000000,21168000000,21588000000,22008000000,22428000000,22848000000,23268000000,23688000000,24108000000,24612000000,25032000000,25452000000,25956000000,26376000000,26880000000,27384000000,27804000000,28308000000,28812000000,29064000000,29316000000,29568000000,29820000000,30072000000,30324000000,30576000000,30828000000,31080000000,31332000000,31584000000,31920000000,32172000000,32424000000,32676000000,32928000000,33180000000,33516000000,33768000000,34020000000,34356000000,34608000000,34860000000,35196000000,35448000000,35700000000,36036000000,36288000000,36624000000,36876000000,37128000000,37464000000,37716000000,38052000000,38388000000,38640000000,38976000000,39228000000,39564000000,39900000000,40152000000,40488000000,40824000000,41076000000,41412000000,41748000000,42084000000,42336000000,42672000000,43008000000,43176000000,43344000000,43512000000,43680000000,43848000000,44016000000,44184000000,44352000000,44436000000,44604000000,44772000000,44940000000,45108000000,45276000000,45444000000,45612000000,45780000000,45948000000,46116000000,46284000000,46452000000,46620000000,46788000000,46956000000,47208000000,47376000000,47544000000,47712000000,47880000000,48048000000,48216000000,48384000000,48552000000,48720000000,48888000000,49056000000,49224000000,49392000000,49644000000,49812000000,49980000000,50148000000,50316000000,50484000000,50652000000,50820000000,51072000000,51240000000,51408000000,51576000000,51744000000,51912000000,52164000000,52332000000,52500000000,52668000000,52836000000,53088000000,53256000000,53424000000,53592000000,53844000000,54012000000,54180000000,54348000000,54516000000,54768000000,54936000000,55104000000,55356000000,55524000000,55692000000,55860000000,56112000000,56280000000,56448000000,56700000000,56868000000,57036000000,57204000000,57456000000,57624000000,57792000000,58044000000,58212000000,58464000000,58632000000,58800000000,59052000000,59220000000,59388000000,59640000000,59808000000,60060000000,60228000000,60396000000,60648000000,60816000000,61068000000,61236000000,61404000000,61656000000,61824000000,62076000000,62244000000,62496000000,62664000000,62916000000,63084000000,63336000000,63504000000,63756000000,63924000000,64176000000,64344000000,64596000000,64764000000,65016000000,65184000000,65436000000,65604000000,65856000000,66024000000,66276000000,66444000000,66696000000,66948000000,67116000000,67368000000,67536000000,67788000000,68040000000,68208000000,68460000000,68628000000,68880000000,69132000000,69300000000,69552000000,69804000000,69972000000,70224000000,70476000000,70644000000,70896000000,71148000000,71316000000,71568000000,71820000000,71988000000,72240000000,72492000000,72744000000,72912000000,73164000000,73416000000,73584000000,73836000000,74088000000,74340000000,74592000000,74760000000,75012000000,75264000000,75516000000,75684000000,75936000000,76188000000,76440000000,76692000000,76860000000,77112000000,77364000000,77616000000,77868000000,78120000000,78372000000,78540000000,78792000000,79044000000,79296000000,79548000000,79800000000,80052000000,80304000000,80556000000,80808000000,80976000000,81228000000,81480000000,81732000000,81984000000,82236000000,82488000000,82740000000,82992000000,83244000000,83496000000,83748000000};
				
				if(fuelPowerUnit - fpm::fixed_24_8(4) >= fpm::fixed_24_8(lookUpPrices.size()))
				{
					fuelPowerUnit = fpm::fixed_24_8(lookUpPrices.size()) - fpm::fixed_24_8(1);
				}
				
				cost = lookUpPrices[(int32_t)(fuelPowerUnit-fpm::fixed_24_8(4))];
			}	
			fighterDb->RerollName(cost, ctx.RoConfig (), rnd, nQL);
		}
		
		// Armor	
		if(optionID == 2)
		{		
			fpm::fixed_24_8 basedQualityLevel = fpm::fixed_24_8(fighterDb->GetProto().quality());
			fpm::fixed_24_8 minimumPowerToPass = fpm::fixed_24_8(basedQualityLevel);
			
			int32_t rollSuccess = rnd.NextInt((int32_t)fuelPowerUnit+1);
			fpm::fixed_24_8 valueRolled = fpm::fixed_24_8(rollSuccess);
			
			LOG (WARNING) << "Rolled for armor " << (int32_t)valueRolled << " with minimum power needed to pass " << (int32_t)minimumPowerToPass;
			
			if(valueRolled >= minimumPowerToPass)
			{
				const auto& fighterMoveBlueprintList = ctx.RoConfig ()->fightermoveblueprints();
				std::map<pxd::ArmorType, std::string> slotsUsed;
				
				for(auto& armorPiece : fighterDb->GetProto().armorpieces())
				{
					slotsUsed.insert(std::pair<pxd::ArmorType, std::string>((pxd::ArmorType)armorPiece.armortype(), armorPiece.candy()));		
				}

				auto sortedMoveBlueprintTypesmap = SortPairsByKey(fighterMoveBlueprintList);

				int32_t totalMoveSize = sortedMoveBlueprintTypesmap.size();
				auto moveBlueprint = sortedMoveBlueprintTypesmap[rnd.NextInt(totalMoveSize)];
		  
				std::vector<pxd::ArmorType> aType = ArmorTypeFromMoveType((pxd::MoveType)moveBlueprint.second.movetype());
				pxd::ArmorType randomElement = aType[rnd.NextInt(aType.size())];
				 
				if(slotsUsed.find(randomElement) == slotsUsed.end())
				{
					slotsUsed.insert(std::pair<pxd::ArmorType, std::string>(randomElement, moveBlueprint.second.candytype()));	   
					proto::ArmorPiece* newArmorPiece =  fighterDb->MutableProto().add_armorpieces();
					newArmorPiece->set_armortype((uint32_t)randomElement);
					newArmorPiece->set_candy(moveBlueprint.second.candytype());
					newArmorPiece->set_rewardsource(0);
					newArmorPiece->set_rewardsourceid("");
				}
				else
				{
					
					 for (int i = 0; i < fighterDb->MutableProto().armorpieces_size (); ++i)
					  {
						auto& data2 = *fighterDb->MutableProto().mutable_armorpieces(i);
						if(data2.armortype() == (uint32_t)randomElement)
						{
							pxd::ArmorType newType = aType[rnd.NextInt(aType.size())];
							
							while(newType == randomElement)
							{
								newType = aType[rnd.NextInt(aType.size())];
							}
							
							data2.set_armortype((uint32_t)newType);
							data2.set_candy(moveBlueprint.second.candytype());
							break;
						}
					}
				}	
			}	
		}		
		
		// Move	
		if(optionID == 3)
		{
			const auto& moveBlueprints = ctx.RoConfig()->fightermoveblueprints();
			std::vector<pxd::proto::FighterMoveBlueprint> generatedMoveblueprints;
			
			auto sortedMoveBlueprintTypesmap = SortPairsByKey(moveBlueprints);
			
            if(fuelPowerUnit >= fpm::fixed_24_8(fighterDb->GetProto().quality()))
			{			
		        if(fuelPowerUnit > fpm::fixed_24_8(39))
				{
					fuelPowerUnit = fpm::fixed_24_8(39);
				}
				
				int32_t rollSuccess = rnd.NextInt(40);
				
				LOG (WARNING) << "Rolled for move " << rollSuccess << " with treshhold needed to pass " << (int32_t)fuelPowerUnit;
				
				if(fpm::fixed_24_8(rollSuccess) < fuelPowerUnit)
				{
					int32_t moveSize = fighterDb->GetProto().moves_size();
					int32_t moveToAlter = rnd.NextInt(moveSize);

					auto& moveToAlterData = *fighterDb->MutableProto().mutable_moves(moveToAlter);
					std::string currentMove = moveToAlterData;
					std::string newMove = sortedMoveBlueprintTypesmap[rnd.NextInt(sortedMoveBlueprintTypesmap.size())].second.authoredid();
															
					while(currentMove == newMove)
					{	
						newMove = sortedMoveBlueprintTypesmap[rnd.NextInt(sortedMoveBlueprintTypesmap.size())].second.authoredid();
					}
					
					moveToAlterData = newMove;
				}
			}
		}
	}
	
	fighterDb->SetStatus(pxd::FighterStatus::Transfiguring);
	fighterDb.reset();		
	
	DestroyUsedElements(a, fighter);
	
	
	a->CalculatePrestige(ctx.RoConfig());
	
	a.reset();  
  }    

 void MoveProcessor::MaybeDeconstructFighter (const std::string& name, const Json::Value& fighter)
 {      
    auto a = xayaplayers.GetByName (name, ctx.RoConfig ());
    
    if(a == nullptr)
    {
      LOG (INFO) << "Player " << name << " not found";
      return;
    }
    
    uint32_t fighterID = 0;
    if(!ParseDeconstructData(*a, fighter, fighterID))      
    {
      a.reset();
      return;
    }

    auto fighterDb = fighters.GetById (fighterID, ctx.RoConfig ());
    fighterDb->SetStatus(pxd::FighterStatus::Deconstructing);
    
    /* H3: ongoing now lives in the height-keyed ongoing_operations table with an
       ABSOLUTE resolve height instead of a per-block BlocksLeft countdown. */
    auto newOp = ongoings.CreateNew(ctx.Height());
    newOp->SetOwner(name);
    newOp->SetHeight(ctx.Height() + ctx.RoConfig()->params().deconstruction_blocks());
    newOp->MutableProto().set_type((uint32_t)pxd::OngoingType::DECONSTRUCTION);
    newOp->MutableProto().set_fighterdatabaseid(fighterID);
    newOp.reset();

    fighterDb.reset();
    a.reset();
    
  }        

} // namespace pxd

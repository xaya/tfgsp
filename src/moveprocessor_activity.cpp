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

/* Tournament + expedition entry/claim/leave actions.
   Split out of moveprocessor.cpp; part of pxd::MoveProcessor. */

namespace pxd
{

  bool BaseMoveProcessor::ParseTournamentLeaveData(const XayaPlayer& a, const std::string& name, const Json::Value& tournament, uint32_t& tournamentID, std::vector<uint32_t>& fighterIDS)
  {
    if (!tournament.isObject ())
    {
      VLOG (1) << "Tournament is not an object";
      return false;
    }

    if(!tournament["tid"].isInt())
    {
      LOG (WARNING) << "tid is not an int";
      return false;
    }
    
    tournamentID = tournament["tid"].asInt();
    
    auto tournamentData = tournamentsTbl.GetById(tournamentID, ctx.RoConfig ());
    
    if(tournamentData == nullptr)
    {
      LOG (WARNING) << "Asking for non-existant tournament with id: " << tournamentID;
      return false;
    }    

    for(auto ft: tournamentData->GetInstance().fighters())
    {
        fighterIDS.push_back(ft);
    }
    
    const auto chain = ctx.Chain ();
    if(tournamentData->GetProto().authoredid() == "cbd2e78a-37ce-b864-793d-8dd27788a774" && chain != xaya::Chain::REGTEST)
    {
      LOG (WARNING) << "Can not leave tutorial tournament entry";
      return false;        
    }    
     
    if((pxd::TournamentState)(uint32_t)tournamentData->GetInstance().state() != pxd::TournamentState::Listed)
    {
      LOG (WARNING) << "Asking to leave already pending tournament with id: " << tournamentID;
      return false;        
    }
    
    bool playerIsParticipating = false;
    
    for(const auto fighterID : tournamentData->GetInstance().fighters())
    {
        auto fighterDb = fighters.GetById (fighterID, ctx.RoConfig ());
        
        if(fighterDb->GetOwner() == name)
        {
            playerIsParticipating = true;
            fighterDb.reset(); 
            break;
        }
     
        fighterDb.reset();     
    }
    
    if(playerIsParticipating == false)
    {
      LOG (WARNING) << "Player is not participating in the tournament with id: " << tournamentID;
      return false;        
    }

    return true;
  }  

  bool BaseMoveProcessor::ParseTournamentEntryData(const XayaPlayer& a, const Json::Value& tournament, uint32_t& tournamentID, std::vector<uint32_t>& fighterIDS)
  {
    if (!tournament.isObject ())
    {
      VLOG (1) << "Tournament is not an object";
      return false;
    }

    if(!tournament["tid"].isInt())
    {
      LOG (WARNING) << "tid is not an int";
      return false;
    }
    
    tournamentID = tournament["tid"].asInt();
    
    const auto& fghtrs = tournament["fc"];
    if (!fghtrs.isArray ())
    {
      LOG (WARNING) << "Fighter is not array for tournament with id: " << tournamentID;
      return false;
    }

    /* DoS cap (H2): bound the roster array before the O(n^2) dedup and the
       per-entry DB lookups below (mirrors the expedition fid / transfigure caps).  */
    if (fghtrs.size () > MAX_MOVE_ARRAY)
    {
      LOG (WARNING) << "Tournament fc array exceeds the size cap for id: " << tournamentID;
      return false;
    }

    auto tournamentData = tournamentsTbl.GetById(tournamentID, ctx.RoConfig ());
    
    for (const auto& ft : fghtrs)
    {
      if (!ft.isInt ())
      {
        LOG (WARNING) << "Fighter is not integer for tournament with id: " << tournamentID;
        return false; 
      }
      
      fighterIDS.push_back(ft.asInt());
    }
  
    if(tournamentData == nullptr)
    {
      LOG (WARNING) << "Asking for non-existant tournament with id: " << tournamentID;
      return false;
    }

    if((pxd::TournamentState)(int32_t)tournamentData->GetInstance().state() != pxd::TournamentState::Listed)
    {
      LOG (WARNING) << "Tournament is already running or completed for id: " << tournamentID;
      tournamentData.reset();
      return false;
    }
	
    if((int32_t)tournamentData->GetInstance().fighters_size() >= (int32_t)tournamentData->GetProto().teamsize() * (int32_t)tournamentData->GetProto().teamcount())
    {
	  // Lets fill the demand queue to create more tournament instances next block
	  
	  std::string serializedDataString = globalData.GetQueueData();
	  
	  if(serializedDataString == "")
	  {
		  Json::Value recJsonObj(Json::arrayValue);
		  
		  Json::Value resEntry(Json::objectValue);
		  resEntry["playername"] = a.GetName();
		  resEntry["tournamentauth"] = tournamentData->GetProto().authoredid();		  
		  recJsonObj.append(resEntry);
		  
          Json::StyledWriter writer;
          std::string json_str = writer.write(recJsonObj);	

          if(!readOnly) globalData.SetQueueData(json_str);		  
	  }
	  else
	  {
		  Json::Value root;
		  Json::Reader reader;
		  reader.parse(serializedDataString, root);
		  
		  std::string pName = a.GetName();
		  
		  bool alreadyContainsPlayer = false;
		  
		  for (Json::Value::ArrayIndex i = 0; i < root.size(); i++) 
		  {
			 if(root[i]["playername"].asString() == pName)
			 {
				 alreadyContainsPlayer = true;
			 }
		  }
		  
		  if(alreadyContainsPlayer == false)
		  {
			  Json::Value resEntry(Json::objectValue);
			  resEntry["playername"] = a.GetName();
			  resEntry["tournamentauth"] = tournamentData->GetProto().authoredid();		  
			  root.append(resEntry);	
              Json::StyledWriter writer;
              std::string json_str = writer.write(root);	

              if(!readOnly) globalData.SetQueueData(json_str);				  
		  }
	  }
	  
      LOG (WARNING) << "Tournament roster is already full for id, creating new one completely " << tournamentID;
      tournamentData.reset();
	  
      return false;        
    }    	
    
    if(HasDuplicates(fighterIDS))
    {
      LOG (WARNING) << "Roster entries must be unique values";
      tournamentData.reset();
      return false;            
    }
    
    //We check fighters now against entry validity
      
    for(int32_t r = 0; r < (int32_t)fighterIDS.size(); r++)
    {
        auto fighter = fighters.GetById (fighterIDS[r], ctx.RoConfig ());
        
        if(fighter == nullptr)
        {
          LOG (WARNING) << "Fatal error, could not get fighter with ID" << fighterIDS[r];
          return false;                
        }
        
        if(fighter->GetOwner() != a.GetName())
        {
          LOG (WARNING) << "Fighter does not belong to: " << a.GetName();
          tournamentData.reset();
          fighter.reset();
          return false;               
        }
        
        if(fighter->GetProto().sweetness() < tournamentData->GetProto().minsweetness())
        {
          LOG (WARNING) << "Fighter sweetness mismatch for tournament with id: " << tournamentID;
          tournamentData.reset();
          fighter.reset();
          return false;              
        }
        
        if(fighter->GetProto().sweetness() > tournamentData->GetProto().maxsweetness())
        {
          LOG (WARNING) << "Fighter sweetness mismatch for tournament with id: " << tournamentID;
          tournamentData.reset();
          fighter.reset();
          return false;              
        } 

        if((pxd::FighterStatus)(int32_t)fighter->GetStatus() != pxd::FighterStatus::Available)
        {
          LOG (WARNING) << "Fighter status is busy for tournament with id: " << tournamentID;
          tournamentData.reset();
          fighter.reset();
          return false;              
        }    

        fighter.reset();        
    }

    for(auto fighterAlreadyInside : tournamentData->GetInstance().fighters())
    {
        auto fighter = fighters.GetById (fighterAlreadyInside, ctx.RoConfig ());
        
        if(fighter == nullptr)
        {
          LOG (WARNING) << "Fatal error, could not get fighter with ID" << fighterAlreadyInside;
          return false;                
        }        
        
        if(fighter->GetOwner() == a.GetName())
        {
          tournamentData.reset();
          fighter.reset();
          LOG (WARNING) << "Some player fighters are already in tournament with id: " << tournamentID;
          return false;
        }
        
        fighter.reset();
    }
    
    if(fighterIDS.size() != tournamentData->GetProto().teamsize())
    {
      LOG (WARNING) << "Incorrect number of fighters sent to join tournament with id: " << tournamentID;
      tournamentData.reset();     
      return false;           
    }
    
    const auto chain = ctx.Chain ();
    if(tournamentData->GetProto().authoredid() == "cbd2e78a-37ce-b864-793d-8dd27788a774" && chain != xaya::Chain::REGTEST)
    {
      LOG (WARNING) << "Can't join FTUE tournament again! With id: " << tournamentID;
      tournamentData.reset();
      return false;          
    }
    
    if(a.GetBalance() < tournamentData->GetProto().joincost())
    {
      LOG (WARNING) << "Not enough crystals to join tournament with id: " << tournamentID;
      tournamentData.reset();
      return false;          
    }

    tournamentData.reset();
    
    return true;
  }

  bool BaseMoveProcessor::ParseExpeditionData(const XayaPlayer& a, const Json::Value& expedition, pxd::proto::ExpeditionBlueprint& expeditionBlueprint, std::vector<int32_t>& fightersIds, int32_t& duration, std::string& weHaveApplibeGoodyName)
  {
    if (!expedition.isObject ())
    return false;

    if(!expedition["eid"].isString())
    {
        return false;
    }
    
    if(!expedition["fid"].isInt() && !expedition["fid"].isArray())
    {
        return false;
    }    

    const std::string expeditionID = expedition["eid"].asString ();

    /* DoS cap (H2): bound the fighter-id party array before dedup/lookups.  */
    if(expedition["fid"].isArray() && expedition["fid"].size() > MAX_MOVE_ARRAY)
    {
      LOG (WARNING) << "Expedition fid array exceeds the size cap";
      return false;
    }

    if(expedition["fid"].isInt())
    {
      fightersIds.push_back(expedition["fid"].asInt()); 
    }
    else
    {
        for(auto ft: expedition["fid"])
        {
            if (!ft.isInt ()) return false;
            fightersIds.push_back(ft.asInt());
        }
        
        //Filter dupes if any
        
        if(HasDuplicates(fightersIds))
        {
          LOG (WARNING) << "Entry contained fighter duplicate ids for " << expedition;
          return false;             
        }            
    }
    
    for(int32_t s =0; s < (int32_t)fightersIds.size(); s++)
    {
      auto fighterDD = fighters.GetById (fightersIds[s], ctx.RoConfig ());
      
      if(fighterDD == nullptr)
      {
          LOG (WARNING) << "Could not resolve fighter with id: " << fightersIds[s]  << " with incoming as " << expedition;
          return false;           
      }
      
      if(a.GetName() != fighterDD->GetOwner())
      {
          LOG (WARNING) << "Fighter does not belong to the player, fighter id is: " << fightersIds[s];
          fighterDD.reset();
          return false;         
      }
      
      const auto& expeditionList = ctx.RoConfig()->expeditionblueprints();
      bool blueprintSolved = false;
      
      for(const auto& expedition: expeditionList)
      {
          if(expedition.second.authoredid() == expeditionID)
          {
              expeditionBlueprint = expedition.second;
              blueprintSolved = true;
              break;
          }
      }
      
      if(blueprintSolved == false)
      {
          LOG (WARNING) << "Could not resolve expedition blueprint with authID: " << expeditionID;
          fighterDD.reset();
          return false;              
      }
      
      const auto chain = ctx.Chain ();
      if(expeditionBlueprint.authoredid() == "c064e7f7-acbf-4f74-fab8-cccd7b2d4004" && chain != xaya::Chain::REGTEST)
      {
          LOG (WARNING) << "Could not resolve tutorial expedition twice " << expeditionID;
          fighterDD.reset();
          return false;            
      }
      
      if(fighterDD->GetProto().sweetness() < expeditionBlueprint.minsweetness())
      {
          LOG (WARNING) << "Trying to join expedition but fighter doesn't meet sweetness requirements: " << fightersIds[s];
          fighterDD.reset();
          return false;              
      }
      
      if(fighterDD->GetProto().sweetness() > expeditionBlueprint.maxsweetness())
      {
          LOG (WARNING) << "Trying to join expedition but fighter doesn't meet sweetness requirements: " << fightersIds[s];
          fighterDD.reset();
          return false;              
      }    
      
      if(fighterDD->GetStatus() != FighterStatus::Available)
      {
          LOG (WARNING) << "Trying to join expedition but fighter isn't available : " << fightersIds[s];
          fighterDD.reset();
          return false;              
      }     
      
      {
        auto ores = ongoings.QueryForOwner(a.GetName());
        while(ores.Step())
        {
          auto op = ongoings.GetFromResult(ores);
          if(op->GetProto().expeditionblueprintid() == expeditionID && op->GetProto().type() == (uint32_t)pxd::OngoingType::EXPEDITION)
          {
            LOG (WARNING) << "Player already has this expedition ongoing: " << expeditionID;
            return false;
          }
        }
      }
	  
      fighterDD.reset();  
    }
	
    const auto& goodiesList = ctx.RoConfig()->goodies();
    auto sortedGoodyTypesmap = SortPairsByKey(goodiesList);

    fpm::fixed_24_8 reductionPercent = fpm::fixed_24_8(1);
    FindApplicableGoody(a.GetInventory(), pxd::GoodyType::Espresso, sortedGoodyTypesmap, weHaveApplibeGoodyName, reductionPercent);

    fpm::fixed_24_8 effective_duration = fpm::fixed_24_8(expeditionBlueprint.duration());
    if(weHaveApplibeGoodyName != "")
    {
        effective_duration = effective_duration * reductionPercent;
    }    
    
    duration = (int32_t)effective_duration;
    
    return true;    
  }  

 void MoveProcessor::MaybeLeaveTournament (const std::string& name, const Json::Value& tournament)
  {      
    auto a = xayaplayers.GetByName (name, ctx.RoConfig ());
    
    if(a == nullptr)
    {
      LOG (INFO) << "Player " << name << " not found";
      return;
    }
    
    uint32_t tournamentID = 0;
    std::vector<uint32_t> fighterIDS;

    if(!ParseTournamentLeaveData(*a, name, tournament, tournamentID, fighterIDS))      
    {
      a.reset();
      return;
    }

    auto tournamentData = tournamentsTbl.GetById(tournamentID, ctx.RoConfig ());   
    for(int32_t r = 0; r < (int32_t)fighterIDS.size(); r++)
    {
        auto fighter = fighters.GetById (fighterIDS[r], ctx.RoConfig ());
        
        if(fighter->GetOwner() == name)
        {
          fighter->MutableProto().set_tournamentinstanceid(0);
          fighter->SetStatus(pxd::FighterStatus::Available);
        
          for (auto it = tournamentData->GetInstance().fighters().begin(); it != tournamentData->GetInstance().fighters().end(); it++)
          {
              if (*it == fighterIDS[r])
              {
                  auto& mF = *tournamentData->MutableInstance().mutable_fighters();
                  mF.erase(it);
                  break;
              }
          }

          LOG (INFO) << "Fighter " << fighter->GetId() << " leaves tournament ";
          
          fighter.reset();
        }
    }

    /* Leaving forfeits the join cost; do NOT charge it again here.  The old
       code double-deducted (chain-halt via AddBalance CHECK_GE underflow once
       a non-zero joincost ships), and a +refund would be a faucet because
       this runs OUTSIDE the ownership-validated loop above. */
    tournamentData.reset();
    
    a.reset();
    
    LOG (INFO) << "Tournament " << tournamentID << " left successfully ";
  }    

  void MoveProcessor::MaybeEnterTournament (const std::string& name, const Json::Value& tournament)
  {      
    auto a = xayaplayers.GetByName (name, ctx.RoConfig ());
    
    if(a == nullptr)
    {
      LOG (INFO) << "Player " << name << " not found";
      return;
    }
    
    uint32_t tournamentID = 0;
    std::vector<uint32_t> fighterIDS;   
    
    if(!ParseTournamentEntryData(*a, tournament, tournamentID, fighterIDS))       
    {
      a.reset();
      return;
    }

    auto tournamentData = tournamentsTbl.GetById(tournamentID, ctx.RoConfig ());
    
    for(int32_t r = 0; r < (int32_t)fighterIDS.size(); r++)
    {
        auto fighter = fighters.GetById (fighterIDS[r], ctx.RoConfig ());
        fighter->MutableProto().set_tournamentinstanceid(tournamentID);
        fighter->SetStatus(pxd::FighterStatus::Tournament);
      
        tournamentData->MutableInstance().add_fighters(fighterIDS[r]);

        LOG (INFO) << "Fighter " << tournamentData->MutableInstance().fighters(tournamentData->MutableInstance().fighters_size()-1) << " enters tournament ";
        
        fighter.reset();
    }

    a->AddBalance (-tournamentData->GetProto().joincost()); 
    tournamentData.reset();
    
    a.reset();
    
    LOG (INFO) << "Tournament " << tournamentID << " entered successfully ";
  }      

  void MoveProcessor::MaybeGoForExpedition (const std::string& name, const Json::Value& expedition)
  {      
    auto a = xayaplayers.GetByName (name, ctx.RoConfig ());
    
    if(a == nullptr)
    {
      LOG (INFO) << "Player " << name << " not found";
      return;
    }    
    
    std::string weHaveApplibeGoodyName = "";
    int32_t duration = -1;
    pxd::proto::ExpeditionBlueprint expeditionBlueprint;
    std::vector<int32_t> fighter;
    
    if(!ParseExpeditionData(*a, expedition, expeditionBlueprint, fighter, duration, weHaveApplibeGoodyName)) return;
    
    for(auto& ft: fighter)
    {
      auto fighterDD = fighters.GetById (ft, ctx.RoConfig ());

      /** We need to make it at least 1 block, else if will make no sense executing immediately logic flow wise*/
      int32_t d = duration;
      if(d < 1)
      {
          d = 1;
      }

      /* H3: one ongoing_operations row per fighter, absolute resolve height. */
      auto newOp = ongoings.CreateNew(ctx.Height());
      newOp->SetOwner(name);
      newOp->SetHeight(ctx.Height() + d);
      newOp->MutableProto().set_type((uint32_t)pxd::OngoingType::EXPEDITION);
      newOp->MutableProto().set_fighterdatabaseid(ft);
      newOp->MutableProto().set_expeditionblueprintid(expedition["eid"].asString ());
      if(weHaveApplibeGoodyName != "")
      {
          newOp->MutableProto().set_appliedgoodykeyname(weHaveApplibeGoodyName);
      }
      newOp.reset();

      fighterDD->SetStatus(FighterStatus::Expedition);
      fighterDD->MutableProto().set_expeditioninstanceid(expedition["eid"].asString ());

      LOG (INFO) << "Expedition " << expedition << " submitted successfully ";
      fighterDD.reset();
    }
    
    if(weHaveApplibeGoodyName != "")
    {
      a->GetInventory().AddFungibleCount(weHaveApplibeGoodyName, -1);
    }    
    
    a.reset();
  }    

} // namespace pxd

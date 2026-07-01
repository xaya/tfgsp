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

/* Crystal / sweetener / goody / name-reroll purchases (parse + try).
   Split out of moveprocessor.cpp; part of pxd::MoveProcessor. */

namespace pxd
{

bool
BaseMoveProcessor::ParseSweetenerPurchase(const Json::Value& mv, Amount& cost, const std::string& name, std::string& fungibleName, Amount balance, Amount& total)
{
  const auto& cmd = mv["ps"];
  
  if(!cmd.isString())
  {
      return false;
  }
  
  total = 1;
  std::string cmdStr = cmd.asString();
  try
  {
	  if (cmdStr.find(",") != std::string::npos) 
	  {
		std::vector<std::string> result;
		std::stringstream ss (cmdStr);
		std::string item;
        char delim = ',';

		while (std::getline (ss, item, delim)) {
			result.push_back (item);
		}	  
		
		if (result.size () >= 2)
		{
			total = std::stoi(result[1]);
			cmdStr = result[0];
		}
	  }	  
  }
  catch(...)
  {
  }
  
  if(total <=1) total = 1;
  
  bool exists = false;
  const auto& sweetenerList = ctx.RoConfig()->sweetenerblueprints();
  
  for(const auto& swtnr: sweetenerList)
  {
      if(swtnr.second.authoredid() == cmdStr)
      {
          cost = swtnr.second.price();
          fungibleName = swtnr.first;
          exists = true;
          break;
      }
  }
  
  if(exists == false)
  {
      LOG (WARNING) << "Could not solve sweetener entry for: " << cmdStr;
      return false;      
  }
  
  if (balance < cost * total)
  {
      LOG (WARNING)
          << "Required amount to purchase bundle not paid by " << name
          << " (only have " << balance << ")";
      return false;
  }     

  return true;  
}

bool
BaseMoveProcessor::ParseGoodyBundlePurchase(const Json::Value& mv, Amount& cost, const std::string& name, std::map<std::string, uint64_t>& fungibles, Amount balance)
{
  const auto& cmd = mv["pgb"];
  
  if(!cmd.isString())
  {
      return false;
  }
  
  bool exists = false;
  const auto& gbList = ctx.RoConfig()->goodybundles();
  const auto& gdList = ctx.RoConfig()->goodies();
  
  for(const auto& gb: gbList)
  {
    if(gb.second.authoredid() == cmd.asString())
    {
        cost = gb.second.price();
        
        for(const auto& entry: gb.second.bundledgoodies())
        {
          for(const auto& gd: gdList)
          {
            if(gd.second.authoredid() == entry.goodyid())
            {
              if (fungibles.find(gd.first) == fungibles.end())
              {
                fungibles.insert(std::pair<std::string, uint64_t>(gd.first, entry.quantity()));
              }
              else
              {
                fungibles[gd.first] += entry.quantity();
              }
            }
          }
        }

      exists = true;
      break;
    }
  }

  if(exists == false)
  {
      LOG (WARNING) << "Could not solve goody bundle entry for: " << cmd;
      return false;      
  }
  
  if (balance < cost)
  {
      LOG (WARNING)
          << "Required amount to purchase bundle not paid by " << name
          << " (only have " << balance << ")";
      return false;
  }     

  return true;  
}

bool
BaseMoveProcessor::ParseGoodyPurchase(const Json::Value& mv, Amount& cost, const std::string& name, std::string& fungibleName, Amount balance, uint64_t& uses)
{
  const auto& cmd = mv["pg"];
  
  if(!cmd.isString())
  {
      return false;
  }
  
  bool exists = false;
  const auto& gdList = ctx.RoConfig()->goodies();
  
  for(const auto& gd: gdList)
  {
      if(gd.second.authoredid() == cmd.asString())
      {
          cost = gd.second.price();
          uses = gd.second.uses();
          fungibleName = gd.first;
          exists = true;
          break;
      }
  }

  if(exists == false)
  {
      LOG (WARNING) << "Could not solve goody entry for: " << cmd;
      return false;      
  }
  
  if (balance < cost)
  {
      LOG (WARNING)
          << "Required amount to purchase bundle not paid by " << name
          << " (only have " << balance << ")";
      return false;
  }     

  return true;  
}

bool
BaseMoveProcessor::ParseNameRerollPurchase(const Json::Value& mv, int64_t& treatId, Amount& cost, const std::string& name, const Amount& paidToDev)
{
  const auto& cmd = mv["nr"];
  
  if(!cmd.isInt())
  {
      return false;
  }
  
  treatId = cmd.asInt ();

  auto fighterDb = fighters.GetById (treatId, ctx.RoConfig ());

  if(fighterDb == nullptr)
  {
	  LOG (WARNING) << "Fatal error, could not get fighter with ID" << treatId;
	  fighterDb.reset();
	  return false;                
  }

  if(fighterDb->GetOwner() != name)
  {
	  LOG (WARNING) << "Fighter does not belong to: " << name;
	  fighterDb.reset();
	  return false;               
  }    

  if(fighterDb->GetProto().isnamererolled() == true)
  {
	  LOG (WARNING) << "Can not reroll name again for fighter: " << name;
	  fighterDb.reset();
	  return false;               
  }      

  if((pxd::FighterStatus)(int32_t)fighterDb->GetStatus() != pxd::FighterStatus::Available) 
  {
	  LOG (WARNING) << "Fighter status is not available: " << treatId;
	  fighterDb.reset();
	  return false;              
  }      
  fighterDb.reset();
     
  VLOG (1) << "Trying to purchace reroll, amount paid left: " << paidToDev;

  /* Integer satoshi math (== 14000000); no raw double in a consensus path. */
  Amount needed = 14 * COIN / 100;
  
  if (paidToDev < needed)
  {
      LOG (WARNING) << "Required amount can not be less then a minimum fraction, we have " << paidToDev << " while need " << needed;
      return false;
  }     
  
  cost = paidToDev;    
  return true;
}

bool
BaseMoveProcessor::ParseCrystalPurchase(const Json::Value& mv, std::string& bundleKeyCode, Amount& cost, Amount& crystalAmount, const std::string& name, const Amount& paidToDev)
{
  const auto& cmd = mv["pc"];
  
  if(!cmd.isString())
  {
      return false;
  }
  
  bundleKeyCode = cmd.asString ();
  auto bundle_key_name = "CrystalsPurchase_" + bundleKeyCode;
  
  int64_t chiPRICE_sats = -1;

  for(auto& bundle: ctx.RoConfig ()->crystalbundles())
  {
      if(bundle.first == bundle_key_name)
      {
          auto& vals = bundle.second;
          chiPRICE_sats = vals.chicostsats();
          crystalAmount = vals.quantity();
          break;
      }
  }

  if(chiPRICE_sats == -1)
  {
      LOG (WARNING) << "Could not solve crystal bundle entry for: " << bundle_key_name;
      return false;
  }

  int64_t multiplier = globalData.GetChiMultiplier();

  /* CRYS-1 fix: the cost used to be chiPRICE_sats * (multiplier / 1000), whose
     inner integer division truncates to 0 for any multiplier in 1..999, making
     'cost' 0 so a zero-payment move minted the bundle for FREE.  Multiply first,
     then divide, so the 1000ths granularity actually applies and the cost only
     reaches 0 for a genuinely free bundle.  At the default multiplier (1000)
     this is identical to the old result (cost == chiPRICE_sats). */
  cost = chiPRICE_sats * multiplier / 1000;
  VLOG (1) << "Trying to purchace bundle, amount paid left: " << paidToDev << "with multiplier as " << multiplier << " and totcal cost being as " << cost;
  
  if (paidToDev < cost)
  {
      LOG (WARNING)
          << "Required amount to purchase bundle not paid by " << name
          << " (only have " << paidToDev << ")";
      return false;
  }     
     
  return true;
}

bool BaseMoveProcessor::InventoryHasItem(const std::string& itemKeyName, const Inventory& inventory, const google::protobuf::uint64 amount)
{      
   for (const auto& entry : inventory.GetFungible ())
   {
      if(entry.first == itemKeyName && entry.second >= amount)
      {
         return true;
      }
   }
  
  return false;
}

void
BaseMoveProcessor::TrySweetenerPurchase (const std::string& name, const Json::Value& mv)
{
  const auto& cmd = mv["ps"];
  if (!cmd.isString ()) return;

  LOG (INFO) << "Attempting to purchase sweetener through move: " << cmd;
  
  auto player = xayaplayers.GetByName (name, ctx.RoConfig());
  CHECK (player != nullptr);

  Amount cost = 0;
  Amount total = 1;
  std::string fungibleName = "";
  if(!ParseSweetenerPurchase(mv, cost, name, fungibleName, player->GetBalance(), total))
  {
      return;
  }
  
  LOG (INFO) << "GOOD: " << cmd << " as cost " << cost * total;
  player->AddBalance (-cost * total); 
  player->GetInventory().AddFungibleCount(fungibleName, total);
  player.reset();
}

void
BaseMoveProcessor::TryGoodyPurchase (const std::string& name, const Json::Value& mv)
{
  const auto& cmd = mv["pg"];
  if (!cmd.isString ()) return;

  VLOG (1) << "Attempting to purchase goody through move: " << cmd;
  
  const auto player = xayaplayers.GetByName (name, ctx.RoConfig());
  CHECK (player != nullptr);

  Amount cost = 0;
  std::string fungibleName = "";
  uint64_t uses = 0;
  if(!ParseGoodyPurchase(mv, cost, name, fungibleName, player->GetBalance(), uses))
  {
      return;
  }
  
  player->AddBalance(-cost); 
  player->GetInventory().AddFungibleCount(fungibleName, uses);
  
}

void
BaseMoveProcessor::TryGoodyBundlePurchase (const std::string& name, const Json::Value& mv)
{
  const auto& cmd = mv["pgb"];
  if (!cmd.isString ()) return;

  VLOG (1) << "Attempting to purchase goody bundle through move: " << cmd;
  
  const auto player = xayaplayers.GetByName (name, ctx.RoConfig());
  CHECK (player != nullptr);

  Amount cost = 0;
  std::map<std::string, uint64_t> fungibles;
  if(!ParseGoodyBundlePurchase(mv, cost, name, fungibles, player->GetBalance()))
  {
      return;
  }
  
  player->AddBalance (-cost); 
  
  for(const auto& fbl: fungibles)
  {
    player->GetInventory().AddFungibleCount(fbl.first, fbl.second);
  }
}

void
BaseMoveProcessor::TryNameRerollPurchase (const std::string& name, const Json::Value& mv, Amount& paidToDev, const RoConfig& cfg, xaya::Random& rnd)
{
  const auto& cmd = mv["nr"];
  if (!cmd.isInt()) return;

  VLOG (1) << "Attempting to reroll name through move: " << cmd;

  Amount cost = 0;
  int64_t treatId = 0;

  if (!ParseNameRerollPurchase (mv, treatId, cost, name, paidToDev))
    return;

  auto fighterDb = fighters.GetById (treatId, ctx.RoConfig ());
  fighterDb->RerollName (cost, cfg, rnd, pxd::Quality::Common);
  fighterDb.reset ();

  /* C8 (twin): consume the dev payment so the same on-chain payment cannot be
     replayed by a repeated nr/pc command in an array move. */
  paidToDev = 0;
}

void
BaseMoveProcessor::TryCrystalPurchase (const std::string& name, const Json::Value& mv, Amount& paidToDev)
{
  const auto& cmd = mv["pc"];
  if (!cmd.isString ()) return;

  VLOG (1) << "Attempting to purchase crystal bundle through move: " << cmd;

  auto player = xayaplayers.GetByName (name, ctx.RoConfig ());
  CHECK (player != nullptr);

  Amount cost = 0;
  Amount crystalAmount = 0;
  std::string bundleKeyCode = "";

  if (!ParseCrystalPurchase (mv, bundleKeyCode, cost, crystalAmount, name, paidToDev))
    return;

  player->AddBalance (crystalAmount);
  player.reset ();

  /* C8: consume the dev payment so a repeated pc/nr command in the same
     (array) move cannot re-mint against the same on-chain payment. */
  paidToDev = 0;
}

} // namespace pxd

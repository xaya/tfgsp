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
#include "burnsale.hpp"

#include "forks.hpp"
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

namespace
{

/** Clock used for timing the callbacks.  */
using PerformanceTimer = std::chrono::high_resolution_clock;

/** Duration type used for reporting callback timings.  */
using CallbackDuration = std::chrono::microseconds;
/** Unit (as string) for the callback timings.  */
constexpr const char* CALLBACK_DURATION_UNIT = "us";

} // anonymous namespace


namespace pxd
{


/*************************************************************************** */

BaseMoveProcessor::BaseMoveProcessor (Database& d,
                                      const Context& c)
  : ctx(c), db(d), xayaplayers(db), recipeTbl(db), fighters(db), moneySupply(db), rewards(db), tournamentsTbl(db), specialTournamentsTbl(db), globalData(db)
{}


bool
BaseMoveProcessor::ExtractMoveBasics (const Json::Value& moveObj,
                                      std::string& name, Json::Value& mv,
                                      std::map<std::string, Amount>& paidToCrownHolders,
                                      Amount& burnt)
{
  VLOG (1) << "Processing move:\n" << moveObj;
  CHECK (moveObj.isObject ());

  CHECK (moveObj.isMember ("move"));
  mv = moveObj["move"];
  
  if (!mv.isObject () && !mv.isArray ())
  {
    LOG (WARNING) << "Move is not an object and not array: " << mv;
    return false;
  }

  const auto& nameVal = moveObj["name"];
  CHECK (nameVal.isString ());
  name = nameVal.asString ();

  const auto& outVal = moveObj["out"];
  
  std::map<std::string, int32_t> holderTier;
  std::string tier1holderName = "";
  
  //Lets get names of current crown holders
  auto res = specialTournamentsTbl.QueryAll();
  bool tryAndStep = res.Step();
  
  std::string xAddressRegtest = "";
  bool haveMissingAddress = false;
  std::string missingPlayerName = "";
  
  while (tryAndStep)
  {
    auto spctrm = specialTournamentsTbl.GetFromResult (res, ctx.RoConfig ());
    std::string xName = spctrm->GetProto().crownholder();
    
    auto player = xayaplayers.GetByName (xName, ctx.RoConfig());
    if(player == nullptr)
    {
      LOG (WARNING) << "Failed to get player with name " << xName;
      return false;
    }
    
    std::string xAddress = player->GetProto().address();
	xAddressRegtest = xAddress;
    
    if(xAddress == "")
    {
      LOG (WARNING) << "Failed to get valid address for player " << xName;
	  xAddress = "missing";      
	  haveMissingAddress = true;
	  missingPlayerName = xName;
    }
    
    xaya::Chain chain = ctx.Chain();
	if(xName == name)
	{
		if(xAddress != "CcHSR3Ss39Ag8pwJ6nsVmEjcdXMhPfoMp3" && chain != xaya::Chain::REGTEST)
		{
            xAddress = "CU1DM3eBtBH1qbUup33LSP2LNkZnbtkAxA"; //burn bogus address    			
		}
	}	
	
    player.reset();
    
	if(xAddress != "missing")
	{
		paidToCrownHolders.insert(std::pair<std::string, Amount>(xAddress,0));
		holderTier.insert(std::pair<std::string, int32_t>(xAddress,(int32_t)spctrm->GetProto().tier()));
	}
	
    if((int32_t)spctrm->GetProto().tier() == 1)
    {
        tier1holderName = xAddress;
    }
        
    spctrm.reset();
    tryAndStep = res.Step ();
  }
  
  if(haveMissingAddress)
  {
	  // we must issue temporarily address here, before player updates to properly one
	  std::vector<std::string> candidates;
	  candidates.push_back("CSkszVUahNNaj9ENPzAepSuCme4PEZXzgp");
	  candidates.push_back("CPHa1fMuAowhBhNGtcyERPntC6aN89q5Wb");
	  candidates.push_back("CHjEjjeZJEJLoJLxtsRL54m6RMB8vFRngf");
	  candidates.push_back("CKMSbLJwLHKAY8aT2BnVZ7fVSTdD81v9rm");
	  candidates.push_back("CZsJo8YykDhoeVmYMTXp9v3EzbN7i3KhU5");
	  candidates.push_back("CX7VSMEoGqyKKxL4qfCLyDNsqgCPZiP6eD");
	  candidates.push_back("Cd9LyMvE3MkrWysTujDhEmBiUU16rkmHnU");
    	 
	  auto player = xayaplayers.GetByName (missingPlayerName, ctx.RoConfig());
	  if(player == nullptr)
	  {
		  LOG (WARNING) << "Failed to get player with name " << missingPlayerName;
		  return false;
	  }     
	  
	  for(auto& newAddressCandidate: candidates)
	  {
		   if (holderTier.count(newAddressCandidate) == 0) 
		   {
				player->MutableProto().set_address(newAddressCandidate);
				break;
		   }
	  }

      player.reset();
      return false;	  
  }
  
  

  xaya::Chain chain = ctx.Chain();
  if(chain == xaya::Chain::REGTEST)
  {
    paidToCrownHolders.insert(std::pair<std::string, Amount>(xAddressRegtest,0));
    holderTier.insert(std::pair<std::string, int32_t>(xAddressRegtest,8)); 
  }
  else
  {
    paidToCrownHolders.insert(std::pair<std::string, Amount>("CcHSR3Ss39Ag8pwJ6nsVmEjcdXMhPfoMp3",0));
    holderTier.insert(std::pair<std::string, int32_t>("CcHSR3Ss39Ag8pwJ6nsVmEjcdXMhPfoMp3",8)); 
  }

  if(paidToCrownHolders.size() == 7 && chain == xaya::Chain::REGTEST)
  {
    paidToCrownHolders.insert(std::pair<std::string, Amount>("CcHSR3Ss39Ag8pwJ6nsVmEjcdXMhPfoMp3",0));
    holderTier.insert(std::pair<std::string, int32_t>("CcHSR3Ss39Ag8pwJ6nsVmEjcdXMhPfoMp3",8)); 	  
  }
  
  //Every out here must much the address of crownholder and its tier;
  Amount smallestPayFraction = 0;
  
  if(outVal.isObject())
  {
    for(auto& outValue: paidToCrownHolders)
    {
      if(outValue.first == tier1holderName)
      {
        Amount amnt;
		
		if(!outVal[outValue.first].isDouble ()) continue;
		
        if(xaya::ChiAmountFromJson(outVal[outValue.first], amnt) == false)
        {
          continue;        
        }
        
        smallestPayFraction = amnt;
	  } 
    }
  }

  if(smallestPayFraction != 0)
  {
      //Lets filter small additional fee, which we have to get rid of electrum bug
      smallestPayFraction = smallestPayFraction / 1000;
      smallestPayFraction = smallestPayFraction * 1000;

      Amount totalAmountPaid = 0;
      
      for(auto& outValue: outVal)
      {
          Amount amnt;
		  
		  if(!outValue.isDouble ()) return false;
		  
          if(xaya::ChiAmountFromJson(outValue, amnt) == false)
          {
            LOG (WARNING) << "Failed to extract amount from " << outValue;
            return false;                    
          }          
          totalAmountPaid += amnt;
      }
      
    if(outVal.isObject())
    {
        for(auto& outValue: paidToCrownHolders)
        {
            int32_t myTier = holderTier[outValue.first];
			
			if(myTier > 7) myTier = 7;
            
            Amount amnt;
            if(xaya::ChiAmountFromJson(outVal[outValue.first], amnt) == false)
            {			  
			  if(chain == xaya::Chain::REGTEST && outValue.first == "CcHSR3Ss39Ag8pwJ6nsVmEjcdXMhPfoMp3")
			  {
				  xaya::ChiAmountFromJson(outVal[xAddressRegtest], amnt);
			  }
			  else
			  {
              LOG (WARNING) << "Failed to extract amount from " << outVal[outValue.first] << "for key" << outValue.first;
              return false;              
			  }			  
            }
            
            Amount needToPay = myTier * smallestPayFraction;
 
			if(amnt != needToPay)
			{
				if(myTier == 7 && amnt >= needToPay)
				{
				}
				else
				{
				  if(amnt < needToPay)
				  {
				  LOG (WARNING) << "Invalid fraction paid for " << outValue.first << " he/she has tier " << myTier << " but payments was" << amnt << " where needed to pay " << needToPay << " as fracton was " << smallestPayFraction;
				  return false;
				  }
				}
			}      

            paidToCrownHolders[outValue.first] = amnt;
        }
    }
  }
  
  if (moveObj.isMember ("burnt"))
    CHECK (xaya::ChiAmountFromJson (moveObj["burnt"], burnt));
  else
    burnt = 0;

  return true;
}

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
		
		total = std::stoi(result[1]);
		cmdStr = result[0];
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
          << "Required amount to purchace bundle character not paid by " << name
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
      LOG (WARNING) << "Could not solve sweetener entry for: " << cmd;
      return false;      
  }
  
  if (balance < cost)
  {
      LOG (WARNING)
          << "Required amount to purchace bundle character not paid by " << name
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
      LOG (WARNING) << "Could not solve sweetener entry for: " << cmd;
      return false;      
  }
  
  if (balance < cost)
  {
      LOG (WARNING)
          << "Required amount to purchace bundle character not paid by " << name
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
	  LOG (WARNING) << "Fatal erorr, could not get fighter with ID" << treatId;
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

  if((pxd::FighterStatus)(int)fighterDb->GetStatus() != pxd::FighterStatus::Available) 
  {
	  LOG (WARNING) << "Fighter status is not available: " << treatId;
	  fighterDb.reset();
	  return false;              
  }      
  fighterDb.reset();
     
  VLOG (1) << "Trying to purchace reroll, amount paid left: " << paidToDev;

  Amount needed = 0.14 * COIN;
  
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
  
  bool bundleNameSolved = false;
  
  for(auto& bundle: ctx.RoConfig ()->crystalbundles())
  {
    if(bundle.first == bundle_key_name)
    {
      bundleNameSolved = true;
    }
  }     
     
  if(bundleNameSolved == false)
  {
      return false;
  }      
     
  float chiPRICE = -1;

  for(auto& bundle: ctx.RoConfig ()->crystalbundles())
  {
      if(bundle.first == bundle_key_name)
      {
          auto& vals = bundle.second;
          chiPRICE = vals.chicost();
          crystalAmount = vals.quantity();
          break;
      }
  }
  
  if(chiPRICE == -1)
  {
      LOG (WARNING) << "Could not solve crystal bundle entry for: " << bundle_key_name;
      return false;
  }
  
  int64_t multiplier = globalData.GetChiMultiplier();
  
 
  cost = chiPRICE * COIN * (multiplier / 1000);
  VLOG (1) << "Trying to purchace bundle, amount paid left: " << paidToDev << "with multiplier as " << multiplier << " and totcal cost being as " << cost;
  
  if (paidToDev < cost)
  {
      LOG (WARNING)
          << "Required amount to purchace bundle character not paid by " << name
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

  if (cmd.isObject ())
  {
      LOG (WARNING) << "Purchase sweetener is object: " << cmd;
      return;
  } 
  
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

  if (cmd.isObject ())
  {
      LOG (WARNING) << "Purchase goody is object: " << cmd;
      return;
  } 
  
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

  if (cmd.isObject ())
  {
      LOG (WARNING) << "Purchase goody bundle is object: " << cmd;
      return;
  } 
  
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
BaseMoveProcessor::TryNameRerollPurchase (const std::string& name, const Json::Value& mv, std::map<std::string, Amount>& paidToCrownHolders, const RoConfig& cfg, xaya::Random& rnd)
{
  const auto& cmd = mv["nr"];
  if (!cmd.isInt()) return;

  VLOG (1) << "Attempting to reroll name through move: " << cmd;

  Amount paidToDev = 0;
  Amount cost = 0;
  int64_t treatId = 0;
  Amount fraction = paidToDev / 35;
  
  for(auto& payFraction: paidToCrownHolders)
  {
      paidToDev += payFraction.second;
  }
  
  if(!ParseNameRerollPurchase(mv, treatId, cost, name, paidToDev)) 
  {
      return;
  }

  auto fighterDb = fighters.GetById (treatId, ctx.RoConfig ());
  fighterDb->RerollName(cost, cfg, rnd, pxd::Quality::Common);
  fighterDb.reset();

  std::map<std::string, int32_t> holderTier;
  
  auto res = specialTournamentsTbl.QueryAll();
  bool tryAndStep = res.Step();
  
  std::string xAddressRegtest = "";
  
  while (tryAndStep)
  {
    auto spctrm = specialTournamentsTbl.GetFromResult (res, ctx.RoConfig ());
    std::string xName = spctrm->GetProto().crownholder();
    
    auto pPlayer = xayaplayers.GetByName (xName, ctx.RoConfig());
    if(pPlayer == nullptr)
    {
      LOG (ERROR) << "Failed to get player with name " << xName;
      return;
    }
    
    std::string xAddress = pPlayer->GetProto().address();
    xAddressRegtest = xAddress;
    // Its important to test that address is valid, else we must provide working substitute,
    // or someone might use this just to hard other crown holders
    
    //todo
    
    if(xAddress == "")
    {
      LOG (ERROR) << "Failed to get valid address for player, we must inject XAYA default to keep this consitant for other players " << xName;
      xAddress = "CcHSR3Ss39Ag8pwJ6nsVmEjcdXMhPfoMp3";
      //player.reset();   
      //return;        
    }
    
    pPlayer.reset();    
    
    holderTier.insert(std::pair<std::string, int32_t>(xAddress,(int32_t)spctrm->GetProto().tier()));
    spctrm.reset();
    tryAndStep = res.Step ();
  }

  // This needs to be either developers foundation address or just burned, to make sure 
  // system abuse is prevented by accumulating infinite amount of crystals and overtaking
  // the game with multiple accounts
  
  xaya::Chain chain = ctx.Chain();
  if(chain == xaya::Chain::REGTEST)
  {
    holderTier.insert(std::pair<std::string, int32_t>(xAddressRegtest,8)); 	  
  }
  else
  {
	holderTier.insert(std::pair<std::string, int32_t>("CcHSR3Ss39Ag8pwJ6nsVmEjcdXMhPfoMp3",8));  
  }
  
  if(paidToCrownHolders.size() == 7 && chain == xaya::Chain::REGTEST)
  {
    holderTier.insert(std::pair<std::string, int32_t>("CcHSR3Ss39Ag8pwJ6nsVmEjcdXMhPfoMp3",8)); 	  
  } 
  
  for(auto& entry: paidToCrownHolders)
  {
      if(holderTier[entry.first] == 7)
      {
        Amount fraction = cost / 35;
        Amount leftover = cost - fraction * 35;
        paidToCrownHolders[entry.first] -= leftover;
      }
      
	  int32_t multiplier = holderTier[entry.first];
	  if(multiplier > 7) multiplier = 7;
	  
      paidToCrownHolders[entry.first] -= fraction * multiplier;
  }
  
  paidToDev -= cost;   
}

void
BaseMoveProcessor::TryCrystalPurchase (const std::string& name, const Json::Value& mv, std::map<std::string, Amount>& paidToCrownHolders)
{
  const auto& cmd = mv["pc"];
  if (!cmd.isString ()) return;

  VLOG (1) << "Attempting to purchase crystal bundle through move: " << cmd;

  auto player = xayaplayers.GetByName (name, ctx.RoConfig());
  CHECK (player != nullptr);

  if (cmd.isObject ())
  {
      LOG (WARNING)
          << "Purchase crystal bundle is object: " << cmd;
      return;
  }
  
  Amount cost = 0;
  Amount crystalAmount  = 0;
  std::string bundleKeyCode = "";
  
  Amount paidToDev = 0;
  Amount fraction = paidToDev / 35;
  
  for(auto& payFraction: paidToCrownHolders)
  {
      paidToDev += payFraction.second;
  }
  
  if(!ParseCrystalPurchase(mv, bundleKeyCode, cost, crystalAmount, name, paidToDev)) 
  {
      return;
  }

  player->AddBalance (crystalAmount); 
  player.reset();
  
  std::map<std::string, int32_t> holderTier;
  
  auto res = specialTournamentsTbl.QueryAll();
  bool tryAndStep = res.Step();
  
  std::string xAddressRegtest = "";
  
  while (tryAndStep)
  {
    auto spctrm = specialTournamentsTbl.GetFromResult (res, ctx.RoConfig ());
    std::string xName = spctrm->GetProto().crownholder();
    
    auto pPlayer = xayaplayers.GetByName (xName, ctx.RoConfig());
    if(pPlayer == nullptr)
    {
      LOG (ERROR) << "Failed to get player with name " << xName;
      return;
    }
    
    std::string xAddress = pPlayer->GetProto().address();
    xAddressRegtest = xAddress;
    // Its important to test that address is valid, else we must provide working substitute,
    // or someone might use this just to hard other crown holders
    
    //todo
    
    if(xAddress == "")
    {
      LOG (ERROR) << "Failed to get valid address for player, we must inject XAYA default to keep this consitant for other players " << xName;
      xAddress = "CcHSR3Ss39Ag8pwJ6nsVmEjcdXMhPfoMp3";
      //player.reset();   
      //return;        
    }
    
    pPlayer.reset();    
    
    holderTier.insert(std::pair<std::string, int32_t>(xAddress,(int32_t)spctrm->GetProto().tier()));
    spctrm.reset();
    tryAndStep = res.Step ();
  }

  // This needs to be either developers foundation address or just burned, to make sure 
  // system abuse is prevented by accumulating infinite amount of crystals and overtaking
  // the game with multiple accounts
  
  xaya::Chain chain = ctx.Chain();
  if(chain == xaya::Chain::REGTEST)
  {
    holderTier.insert(std::pair<std::string, int32_t>(xAddressRegtest,8)); 	  
  }
  else
  {
	holderTier.insert(std::pair<std::string, int32_t>("CcHSR3Ss39Ag8pwJ6nsVmEjcdXMhPfoMp3",8));  
  }
  
  if(paidToCrownHolders.size() == 7 && chain == xaya::Chain::REGTEST)
  {
    holderTier.insert(std::pair<std::string, int32_t>("CcHSR3Ss39Ag8pwJ6nsVmEjcdXMhPfoMp3",8)); 	  
  }  
  
  for(auto& entry: paidToCrownHolders)
  {
      if(holderTier[entry.first] == 7)
      {
        Amount fraction = cost / 35;
        Amount leftover = cost - fraction * 35;
        paidToCrownHolders[entry.first] -= leftover;
      }
      
	  int32_t multiplier = holderTier[entry.first];
	  if(multiplier > 7) multiplier = 7;
	  
      paidToCrownHolders[entry.first] -= fraction * multiplier;
  }
  
  paidToDev -= cost;   
}

  pxd::RecipeInstanceTable::Handle BaseMoveProcessor::GetRecepieObjectFromID(const uint32_t& ID, const Context& ctx)
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
            break;
        }
    }      
      
    return "";
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

	 fpm::fixed_24_8 fuelPowerOriginal = MoveProcessor::CalculateFuelPower(fighter, wholeFightersData, wholeRecipeData, candylist, false);
	 
	 Json::Value outputNewValues(Json::objectValue);	 
	 Json::Value aif2(Json::arrayValue);
	 Json::Value aic2(Json::arrayValue);
	 Json::Value air2(Json::arrayValue);
	 
	 for(auto& ft: fightersNew)
	 {
		 Json::Value newEstimation = fighter;
		 newEstimation["if"].append(ft);
		 fpm::fixed_24_8 fuelPowerNewValue = MoveProcessor::CalculateFuelPower(newEstimation, wholeFightersData, wholeRecipeData, candylist, false);
		 fpm::fixed_24_8 diff = (fuelPowerNewValue - fuelPowerOriginal);
		 
		 Json::Value result(Json::objectValue);
		 result["i"] = ft;
		 result["v"] = (int)fpm::floor(diff);
		 
		 aif2.append(result);
	 }
	
	 for(auto& rc: recipesNew)
	 {
		 Json::Value newEstimation = fighter;
		 newEstimation["ir"].append(rc);
		 fpm::fixed_24_8 fuelPowerNewValue = MoveProcessor::CalculateFuelPower(newEstimation, wholeFightersData, wholeRecipeData, candylist, false);
		 fpm::fixed_24_8 diff = (fuelPowerNewValue - fuelPowerOriginal);
		 
		 Json::Value result(Json::objectValue);
		 result["i"] = rc;
		 result["v"] = (int)fpm::floor(diff);
		 
		 air2.append(result);
	 }	
	 
	 for(auto& cn: candiesNew)
	 {
		 Json::Value newEstimation = fighter;
		 
		 Json::Value cData(Json::objectValue);
	     cData["n"] = cn;
		 cData["a"] = 10;
		 newEstimation["ic"].append(cData);		 

		 fpm::fixed_24_8 fuelPowerNewValue = MoveProcessor::CalculateFuelPower(newEstimation, wholeFightersData, wholeRecipeData, candylist, false);
		 fpm::fixed_24_8 diff = (fuelPowerNewValue - fuelPowerOriginal);
		 
		 Json::Value result(Json::objectValue);
		 result["i"] = cn;
		 result["v"] = (int)fpm::floor(diff);
		 
		 aic2.append(result);
	 }		 
	 
	 outputNewValues["if"] = aif2;
	 outputNewValues["ic"] = aic2;
	 outputNewValues["ir"] = air2;	 
	 outputNewValues["fp"] = (int)fuelPowerOriginal;
	 
	 return outputNewValues;
  }	  
  
bool
BaseMoveProcessor::ParseCoinTransferBurn (const XayaPlayer& a,
                                          const Json::Value& moveObj,
                                          CoinTransferBurn& op,
                                          Amount& burntChi)
{
  if(moveObj.isArray())
  {
      return false;
  }
  CHECK (moveObj.isObject ());
  const auto& cmd = moveObj["vc"];
  if (!cmd.isObject ())
    return false;

  Amount balance = a.GetBalance ();
  Amount total = 0;

  const auto& mint = cmd["m"];
  if (mint.isObject () && mint.empty ())
    {
      const Amount soldBefore = moneySupply.Get ("burnsale");
      op.minted = ComputeBurnsaleAmount (burntChi, soldBefore, ctx);
      balance += op.minted;
    }
  else
    {
      op.minted = 0;
      LOG_IF (WARNING, !mint.isNull ()) << "Invalid mint command: " << mint;
    }

  const auto& burn = cmd["b"];
  if (CoinAmountFromJson (burn, op.burnt))
    {
      if (total + op.burnt <= balance)
        total += op.burnt;
      else
        {
          LOG (WARNING)
              << a.GetName () << " has only a balance of " << balance
              << ", can't burn " << op.burnt << " coins";
          op.burnt = 0;
        }
    }
  else
    op.burnt = 0;

  const auto& transfers = cmd["t"];
  if (transfers.isObject ())
    {
      op.transfers.clear ();
      for (auto it = transfers.begin (); it != transfers.end (); ++it)
        {
          CHECK (it.key ().isString ());
          const std::string to = it.key ().asString ();

          Amount amount;
          if (!CoinAmountFromJson (*it, amount))
            {
              LOG (WARNING)
                  << "Invalid coin transfer from " << a.GetName ()
                  << " to " << to << ": " << *it;
              continue;
            }

          if (total + amount > balance)
            {
              LOG (WARNING)
                  << "Transfer of " << amount << " from " << a.GetName ()
                  << " to " << to << " would exceed the balance";
              continue;
            }

          /* Self transfers are a no-op, so we just ignore them (and do not
             update the total sent, so the balance is still available for
             future transfers).  */
          if (to == a.GetName ())
            continue;

          total += amount;
          CHECK (op.transfers.emplace (to, amount).second);
        }
    }

  CHECK_LE (total, balance);
  return total > 0 || op.minted > 0;
}

void
MoveProcessor::ProcessAll (const Json::Value& moveArray)
{
  const auto start = PerformanceTimer::now ();
  CHECK (moveArray.isArray ());
  LOG (INFO) << "Processing " << moveArray.size () << " moves...";

  for (const auto& m : moveArray)
  {
    ProcessOne (m);
  }
  
  const auto end = PerformanceTimer::now ();
  LOG (INFO) << "Processing all moves took " << std::chrono::duration_cast<CallbackDuration> (end - start).count (); 
}

void
MoveProcessor::ProcessAdmin (const Json::Value& admArray)
{
  CHECK (admArray.isArray ());
  LOG (INFO) << "Processing " << admArray.size () << " admin commands...";

  for (const auto& cmd : admArray)
    {
      CHECK (cmd.isObject ());
      ProcessOneAdmin (cmd["cmd"]);
    }
}

void
MoveProcessor::ProcessOneAdmin (const Json::Value& cmd)
{
  if (!cmd.isObject ())
    return;

  HandleGodMode (cmd["god"]);
}

void
MoveProcessor::HandleGodMode (const Json::Value& cmd)
{
    if (!cmd.isObject ())
    return;

    if (!ctx.RoConfig()->params ().god_mode ())
    {
      LOG (WARNING) << "God mode command ignored: " << cmd;
      return;
    }
	
	MaybeSetNewCostMultiplier (cmd["cost"]);
	MaybeSetNewVersionIdentifier (cmd["version"]);
	MaybeSetNewVanillaDownloadUrl (cmd["vanillaurl"]);
}

void
MoveProcessor::MaybeSetNewCostMultiplier (const Json::Value& cmd)
{
  if (!cmd.isInt ())
    return;

  int64_t mt = cmd.asInt();

  globalData.SetChiMultiplier(mt);
}

void
MoveProcessor::MaybeSetNewVersionIdentifier (const Json::Value& cmd)
{
  if (!cmd.isString ())
    return;

  std::string dd = cmd.asString();
  globalData.SetVersion(dd);
}

void
MoveProcessor::MaybeSetNewVanillaDownloadUrl (const Json::Value& cmd)
{
  if (!cmd.isString ())
    return;

  std::string dd = cmd.asString();
  globalData.SetUrl(dd);
}

void
MoveProcessor::TryCoinOperation (const std::string& name,
                                 const Json::Value& mv,
                                 Amount& burntChi)
{
  auto a = xayaplayers.GetByName (name, ctx.RoConfig ());
  CHECK (a != nullptr);

  CoinTransferBurn op;
  if (!ParseCoinTransferBurn (*a, mv, op, burntChi))
  return;

  if (op.minted > 0)
  {
    LOG (INFO) << name << " minted " << op.minted << " coins in the burnsale";
    a->AddBalance (op.minted);
    const Amount oldBurnsale = a->GetProto ().burnsale_balance ();
    a->MutableProto ().set_burnsale_balance (oldBurnsale + op.minted);
    moneySupply.Increment ("burnsale", op.minted);
  }

  if (op.burnt > 0)
  {
    LOG (INFO) << name << " is burning " << op.burnt << " coins";
    a->AddBalance (-op.burnt);
  }

  for (const auto& entry : op.transfers)
  {
    /* Transfers to self are a no-op, but we have to explicitly handle
       them here.  Else we would run into troubles by having a second
       active Account handle for the same account.  */
    if (entry.first == name)
      continue;

    LOG (INFO)
        << name << " is sending " << entry.second
        << " coins to " << entry.first;
    a->AddBalance (-entry.second);

    auto to = xayaplayers.GetByName (entry.first, ctx.RoConfig ());
    if (to == nullptr)
      {
        LOG (INFO)
            << "Creating uninitialised account for coin recipient "
            << entry.first;
        to = xayaplayers.CreateNew (entry.first, ctx.RoConfig (), rnd);
      }
    to->AddBalance (entry.second);
  }
  
  a.reset();
}

void
MoveProcessor::ProcessOne (const Json::Value& moveObj)
{
  std::string name;
  Json::Value mv;
  std::map<std::string, Amount> paidToCrownHolders;
  Amount  burnt;
  
  if (!ExtractMoveBasics (moveObj, name, mv, paidToCrownHolders, burnt))
  {
    return;
  }
  
  std::vector<Json::Value> moves;
  
  if(mv.isObject())
  {
    moves.push_back(mv);
  }
  else
  {
      for(auto& mvx: mv)
      {
          moves.push_back(mvx);
      }
  }
  
  /* Ensure that the account database entry exists.  In other words, we
     have accounts (although perhaps uninitialised) for everyone who
     ever sent a TF move.  */
  if (xayaplayers.GetByName (name, ctx.RoConfig ()) == nullptr)
  {
    LOG (INFO) << "Creating uninitialised account for " << name;
    xayaplayers.CreateNew (name, ctx.RoConfig (), rnd);
  }

  /* Handle coin transfers before other game operations.  They are even
     valid without a properly initialised account (so that vCHI works as
     a real cryptocurrency, not necessarily tied to the game).
     This also ensures that if funds run out, then the explicit transfers
     are done with priority over the other operations that may require coins
     implicitly.  */
     
  for(auto& mrl: moves)
  {
    //TryCoinOperation (name, mrl, burnt);// not need for final TF release, but lets keep if needed to burn coins in the future
    
	/* We perform account updates first.  That ensures that it is possible to
    e.g. choose one's faction and create characters in a single move.  */
    TryXayaPlayerUpdate (name, mrl["a"]);
	
	 // Block until launch time  
	 xaya::Chain chain = ctx.Chain();
	 if(chain != xaya::Chain::REGTEST)
	 {
		  if(ctx.Height () < 5008228)
		  {
			  LOG (WARNING) << "Game release not launched yet, ignoring move";
			  continue;
		  }
	 }	
	
    /* Handle crystal purchace now */
    TryCrystalPurchase (name, mrl, paidToCrownHolders);
	
    /* Handle name reroll purchase*/
    TryNameRerollPurchase (name, mrl, paidToCrownHolders, ctx.RoConfig (), rnd);	
    
    /* Handle sweetener purchace now */
    TrySweetenerPurchase (name, mrl);  
    
    /* Handle goody purchace now */
    TryGoodyPurchase (name, mrl);   
    
    /* Handle goody bundle purchace now */
    TryGoodyBundlePurchase (name, mrl);    
    
    if(chain == xaya::Chain::REGTEST)
    {
    /*Running special unittest from python*/
      MaybeSQLTestInjection(name, mrl);  
      MaybeSQLTestInjection2(name, mrl);    
    }  

    /* We are trying all kind of cooking actions*/
    TryCookingAction (name, mrl["ca"]);
    
    /* We are trying all kind of expedition actions*/
    TryExpeditionAction (name, mrl["exp"]);  
    
    /* We are trying all kind of tournament related actions*/
    TryTournamentAction (name, mrl["tm"]);  

    /* We are trying all kind of special tournament related actions*/
    TrySpecialTournamentAction (name, mrl["tms"], ctx);     

    /* We are trying all kind of fighter related actions*/
    TryFighterAction (name, mrl["f"]);     
  }
  
  Amount totalPaidLeft = 0;

  for(auto& entry: paidToCrownHolders)
  {
    totalPaidLeft += entry.second;
  }
      
  /* If any burnt or paid-to-dev coins are left, it means probably something
     has gone wrong and the user overpaid due to a frontend bug.  */
  LOG_IF (WARNING, totalPaidLeft > 0 || burnt > 0)
      << "At the end of the move, " << name
      << " has " << totalPaidLeft << " paid-to-dev and "
      << burnt << " burnt CHI satoshi left";
}

  bool BaseMoveProcessor::ParseTournamentRewardData(const XayaPlayer& a, const std::string& name, const Json::Value& tournament, std::vector<uint32_t>& rewardDatabaseIds, uint32_t& tournamentID)
  {
    if (!tournament.isObject ())
    return false;

    if(!tournament["tid"].isInt())
    {
        return false;
    }
    
    tournamentID = tournament["tid"].asInt();
    auto tournamentData = tournamentsTbl.GetById(tournamentID, ctx.RoConfig ());
    
    if(tournamentData == nullptr)
    {
        LOG (WARNING) << "Could not resolve tournament entry with authID: " << tournamentID;
        return false;              
    }    
        
    auto res = rewards.QueryForOwner(a.GetName());
    
    int totalEntries = 0;
    bool tryAndStep = res.Step();
    while (tryAndStep)
    {
      auto rw = rewards.GetFromResult (res, ctx.RoConfig ());
      
      if(rw->GetProto().tournamentid() == tournamentID)
      {
        totalEntries++;
        rewardDatabaseIds.push_back(rw->GetId());  
      }
      
      tryAndStep = res.Step ();
    }
   
    if(totalEntries <= 0)
    {
        LOG (WARNING) << "Could not find any relevan rewards for tournament: " << tournamentID;
        return false;              
    }   
    
    return true;
  }        
  
  bool BaseMoveProcessor::ParseRewardData(const XayaPlayer& a, const std::string& name, const Json::Value& expedition, std::vector<uint32_t>& rewardDatabaseIds, std::vector<std::string>& expeditionIDArray)
  {
    if (!expedition.isObject ())
    return false;

    if(!expedition["eid"].isString() && !expedition["eid"].isArray())
    {
        return false;
    }
    
    std::vector<std::string> authIds;
    
    if(expedition["eid"].isString())
    {
      authIds.push_back(expedition["eid"].asString ());   
    }
    else
    {
      for(auto& eID: expedition["eid"])
      {
        authIds.push_back(eID.asString ());   
      }
    }
    
    for(auto expeditionID : authIds)
    {
      expeditionIDArray.push_back(expeditionID);
      
      pxd::proto::ExpeditionBlueprint expeditionBlueprint;
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
          return false;              
      }    
      
      for(const auto& ongoing: a.GetProto().ongoings())
      {
          if(ongoing.expeditionblueprintid() == expeditionID && ongoing.type() == (uint32_t)pxd::OngoingType::EXPEDITION)
          {
            LOG (WARNING) << "Could not get rewards, expedition is in process: " << expeditionID;
            return false;                
          }
      }
      
      auto res = rewards.QueryForOwner(a.GetName());
      
      int totalEntries = 0;
      bool tryAndStep = res.Step();
      while (tryAndStep)
      {
        auto rw = rewards.GetFromResult (res, ctx.RoConfig ());
        
        if(rw->GetProto().expeditionid() == expeditionID)
        {
          totalEntries++;
          rewardDatabaseIds.push_back(rw->GetId());
        }
        
        tryAndStep = res.Step ();
      }
     
      if(totalEntries <= 0)
      {
          LOG (WARNING) << "Could not find any relevan rewards for expedition: " << expeditionID;
          return false;              
      }   
    }
    
    return true;
  }      
  
  bool BaseMoveProcessor::ParseSpecialTournamentLeaveData(const XayaPlayer& a, const std::string& name, const Json::Value& tournament, uint32_t& tournamentID, std::vector<uint32_t>& fighterIDS)
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
    
    auto tournamentData = specialTournamentsTbl.GetById(tournamentID, ctx.RoConfig ());
    
    if(tournamentData == nullptr)
    {
      LOG (WARNING) << "Asking for non-existant special tournament with id: " << tournamentID;
      return false;
    }    
    
    auto res3 = fighters.QueryAll ();
    bool tryAndStep3 = res3.Step();
    
    while (tryAndStep3)
    {
        auto fghtr = fighters.GetFromResult (res3, ctx.RoConfig ()); 
        
        if(fghtr->GetOwner() == name && fghtr->GetProto().specialtournamentinstanceid() == tournamentID)
        {
            
            // If this fighter is currently also a crown holder, he cannot leave;
            if(tournamentData->GetProto().crownholder() == fghtr->GetOwner())
            {
               fghtr.reset();
               tournamentData.reset();
               LOG (WARNING) << "Asking to leave a crown holder for special tournament with id: " << tournamentID;
               return false;                 
            }
            
            
            fighterIDS.push_back(fghtr->GetId());
        }
        
        fghtr.reset();
        tryAndStep3 = res3.Step();        
    }              
    
    if((pxd::SpecialTournamentState)(uint32_t)tournamentData->GetProto().state() != pxd::SpecialTournamentState::Listed)
    {
      LOG (WARNING) << "Asking to leave already calculating special tournament with id: " << tournamentID;
      tournamentData.reset();
      return false;        
    }
        
    if(fighterIDS.size() != 6)
    {
      LOG (WARNING) << "Not all fighter are participating in the tournament with id: " << tournamentID;
      tournamentData.reset();
      return false;        
    }

    return true;
  }    
  
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
    if(tournamentData->GetProto().authoredid() == "cbd2e78a-37ce-b864-793d-8dd27788a774" && chain == xaya::Chain::MAIN)
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
  
  bool BaseMoveProcessor::ParseFighterForSaleData(const XayaPlayer& a, const std::string& name, const Json::Value& fighter, uint32_t& fighterID, uint32_t& duration, Amount& price, Amount& listingfee)
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
    
    if(!fighter["p"].isInt())
    {
      LOG (WARNING) << "p is not an int";
      return false;
    }        
    
    if(!fighter["d"].isInt())
    {
      LOG (WARNING) << "d is not an int";
      return false;
    }    
    
    fighterID = fighter["fid"].asInt();
    duration = fighter["d"].asInt();
    price = fighter["p"].asInt();
    
    listingfee = ctx.RoConfig()->params().exchange_listing_fee();
    
    if(price <= 1)
    {
      LOG (WARNING) << "Incorrect price" << price;
      return false;         
    }
    
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
    
    if(fighterDb->GetProto().isaccountbound() == true)
    {
      LOG (WARNING) << "Can not sale account bound fighter: " << a.GetName();
      fighterDb.reset();
      return false;               
    }      
    
    if((pxd::FighterStatus)(int)fighterDb->GetStatus() != pxd::FighterStatus::Available) 
    {
      LOG (WARNING) << "Fighter status is not available: " << fighterID;
      fighterDb.reset();
      return false;              
    }    
    
    if(duration < 1 || duration > 14)
    {
      LOG (WARNING) << "Invalid duration: " << duration;
      fighterDb.reset();
      return false;         
    }
    
    if(a.GetBalance() < ctx.RoConfig()->params().exchange_listing_fee())
    {
      LOG (WARNING) << "Not enough crystal balance for listing fee " << ctx.RoConfig()->params().exchange_listing_fee();
      fighterDb.reset();
      return false;          
    }

    fighterDb.reset();   
    return true;    
  }    
  
  bool BaseMoveProcessor::ParseRemoveBuyData(const XayaPlayer& a, const std::string& name, const Json::Value& fighter, uint32_t& fighterID)
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
      LOG (WARNING) << "Fighter is not owned by: " << a.GetName();
      fighterDb.reset();
      return false;               
    }    
    
    if((pxd::FighterStatus)(int)fighterDb->GetStatus() != pxd::FighterStatus::Exchange)
    {
      LOG (WARNING) << "Fighter status is not on exchange: " << fighterID;
      fighterDb.reset();
      return false;              
    }    
    
    fighterDb.reset(); 
    return true;    
  }        
  
  bool BaseMoveProcessor::ParseBuyData(const XayaPlayer& a, const std::string& name, const Json::Value& fighter, uint32_t& fighterID, Amount& exchangeprice)
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
    
    exchangeprice = fighterDb->GetProto().exchangeprice();
    
    if(fighterDb->GetOwner() == a.GetName())
    {
      LOG (WARNING) << "Can noy buy own fighter, its nonsense: " << a.GetName();
      fighterDb.reset();
      return false;               
    }    
    
    if((pxd::FighterStatus)(int)fighterDb->GetStatus() != pxd::FighterStatus::Exchange)
    {
      LOG (WARNING) << "Fighter status is not on exchange: " << fighterID << " but is " << (int)fighterDb->GetStatus();
      fighterDb.reset();
      return false;              
    }    
    
    if(a.GetBalance() < (Amount)fighterDb->GetProto().exchangeprice())
    {
      LOG (WARNING) << "Not enough crystals to buy: " << fighterID;
      fighterDb.reset();
      return false;         
    }
    
    if((uint64_t)fighterDb->GetProto().exchangeexpire() <= ctx.Height())
    {
      LOG (WARNING) << "Listing expired for: " << fighterID;
      fighterDb.reset();
      return false;         
    }  

    uint32_t slots = fighters.CountForOwner(a.GetName());
    uint32_t slots_already_cooking = 0;
    
    for(const auto& ongoing: a.GetProto().ongoings())
    {
        if(ongoing.type() == (uint32_t)pxd::OngoingType::COOK_RECIPE)
        {
            slots_already_cooking++;
        }
    } 
    
    slots += slots_already_cooking;

    if(slots > ctx.RoConfig()->params().max_fighter_inventory_amount())
    {
        LOG (WARNING) << "Not enough slots to host a new fighter for " << fighterID;
        return false;         
    }        

    fighterDb.reset(); 
    return true;    
  }      

  bool BaseMoveProcessor::ParseDeconstructRewardData(const XayaPlayer& a, const std::string& name, const Json::Value& fighter, uint32_t& fighterID)
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
    
    if((pxd::FighterStatus)(int)fighterDb->GetStatus() != pxd::FighterStatus::Deconstructing) 
    {
      LOG (WARNING) << "Fighter status is not in deconstruction: " << fighterID;
      fighterDb.reset();
      return false;              
    }    

    bool isFinished = true;
    
    for(const auto& ongoing: a.GetProto().ongoings())
    {
        if(ongoing.fighterdatabaseid() == fighterID && ongoing.type() == (uint32_t)pxd::OngoingType::DECONSTRUCTION)
        {
            isFinished = false;
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

  bool BaseMoveProcessor::ParseTransfigureData(const XayaPlayer& a, const std::string& name, const Json::Value& fighter)
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
      LOG (WARNING) << "fid is not an int";
      return false;
    }	
	
    if(!fighter["o"].isInt())
    {
      LOG (WARNING) << "fid is not an int";
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
	
	int32_t sizeBefore = fighterIDS.size();
	auto end = fighterIDS.end();
	for (auto it = fighterIDS.begin(); it != end; ++it) {
		end = std::remove(it + 1, end, *it);
	}
	fighterIDS.erase(end, fighterIDS.end());
	int32_t sizeAfter = fighterIDS.size();      
	if(sizeBefore != sizeAfter)
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
	
	sizeBefore = itemRecipeIDS.size();
	auto end2 = itemRecipeIDS.end();
	for (auto it = itemRecipeIDS.begin(); it != end2; ++it) {
		end2 = std::remove(it + 1, end2, *it);
	}
	itemRecipeIDS.erase(end2, itemRecipeIDS.end());
	sizeAfter = itemRecipeIDS.size();      
	if(sizeBefore != sizeAfter)
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
	
	sizeBefore = itemCandyIDS.size();
	auto end3 = itemCandyIDS.end();
	for (auto it = itemCandyIDS.begin(); it != end3; ++it) {
		end3 = std::remove(it + 1, end3, *it);
	}
	itemCandyIDS.erase(end3, itemCandyIDS.end());
	sizeAfter = itemCandyIDS.size();      
	if(sizeBefore != sizeAfter)
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
    
    if((pxd::FighterStatus)(int)fighterDb->GetStatus() != pxd::FighterStatus::Available)
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
	
	bool playerHasAtLeastOneEpicTreat = false;
	
    auto res3 = fighters.QueryForOwner (a.GetName());
    bool tryAndStep3 = res3.Step();
    
    while (tryAndStep3)
    {
        auto fghtr = fighters.GetFromResult (res3, ctx.RoConfig ()); 
        
        if(fghtr->GetProto().quality() == 4)
        {
            playerHasAtLeastOneEpicTreat = true;
        }
        
        fghtr.reset();
        tryAndStep3 = res3.Step();        
    }  	
	
	if(playerHasAtLeastOneEpicTreat == false)
	{
		LOG (WARNING) << "Player has no epic treats" << optionID;		
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
		
		if((pxd::FighterStatus)(int)fighterToSactifice->GetStatus() != pxd::FighterStatus::Available)
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
		
		auto itemInventoryRecipe = GetRecepieObjectFromID(recepieID.asInt(), ctx);

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

  bool BaseMoveProcessor::ParseDeconstructData(const XayaPlayer& a, const std::string& name, const Json::Value& fighter, uint32_t& fighterID)
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
    
    if((pxd::FighterStatus)(int)fighterDb->GetStatus() != pxd::FighterStatus::Available)
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
  
  bool BaseMoveProcessor::ParseSpecialTournamentEntryData(const XayaPlayer& a, const std::string& name, const Json::Value& tournament, uint32_t& tournamentID, std::vector<uint32_t>& fighterIDS)
  {
    if (!tournament.isObject ())
    {
      VLOG (1) << "Special Tournament is not an object";
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
    
    auto tournamentData = specialTournamentsTbl.GetById(tournamentID, ctx.RoConfig ());
    
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
      LOG (WARNING) << "Asking for non-existant special tournament with id: " << tournamentID;
      return false;
    }
    
    // This is important - making sure, that player prestige is eligble for this tournament tier
    int32_t tournamentTier = (int32_t)tournamentData->GetProto().tier();
    int32_t playerPrestigeBasedTier = (int32_t)a.GetProto().specialtournamentprestigetier();
    
	xaya::Chain chain = ctx.Chain();
	
	if(chain != xaya::Chain::REGTEST)
	{
		if(tournamentTier != playerPrestigeBasedTier)
		{
		  LOG (WARNING) << "Wrong tournament tier, expected  " << tournamentTier << " but actual player is " << playerPrestigeBasedTier;
		  tournamentData.reset();
		  return false;        
		}
	}

    if((pxd::SpecialTournamentState)(int)tournamentData->GetProto().state() != pxd::SpecialTournamentState::Listed)
    {
      LOG (WARNING) << "Special tournament is calculating, can not process move, tournament id is  " << tournamentID;
      tournamentData.reset();
      return false;
    }
    
    if((int)fighterIDS.size() != 6)
    {
      LOG (WARNING) << "Tournament roster must be exactly 6 fighters, we got: " << fighterIDS.size();
      tournamentData.reset();
      return false;        
    }    
    
    int32_t sizeBefore = fighterIDS.size();
    
    auto end = fighterIDS.end();
    for (auto it = fighterIDS.begin(); it != end; ++it) {
        end = std::remove(it + 1, end, *it);
    }
 
    fighterIDS.erase(end, fighterIDS.end());    

    int32_t sizeAfter = fighterIDS.size();      

    if(sizeBefore != sizeAfter)
    {
      LOG (WARNING) << "Roster entries must be unique values";
      tournamentData.reset();
      return false;            
    }   
    
    //We check fighters now against entry validity
      
    for(long long unsigned int r = 0; r < fighterIDS.size(); r++)
    {
        auto fighter = fighters.GetById (fighterIDS[r], ctx.RoConfig ());
        
        if(fighter == nullptr)
        {
          LOG (WARNING) << "Fatal erorr, could not get fighter with ID" << fighterIDS[r];
          return false;                
        }
        
        if(fighter->GetOwner() != a.GetName())
        {
          LOG (WARNING) << "Fighter does not belong to: " << a.GetName();
          tournamentData.reset();
          fighter.reset();
          return false;               
        }
        
        if((pxd::FighterStatus)(int)fighter->GetStatus() != pxd::FighterStatus::Available)
        {
          LOG (WARNING) << "Fighter status is busy for tournament with id: " << tournamentID;
          tournamentData.reset();
          fighter.reset();
          return false;              
        }    
        
        if(tournamentData->GetProto().crownholder() == a.GetName())
        {
          LOG (WARNING) << "Can not fight in tournaments with itself: " << a.GetName();
          tournamentData.reset();
          fighter.reset();
          return false;              
        }

        fighter.reset();        
    }
    
    auto res3 = fighters.QueryAll ();
    bool tryAndStep3 = res3.Step();
    
    while (tryAndStep3)
    {
        auto fghtr = fighters.GetFromResult (res3, ctx.RoConfig ()); 
        
        if(fghtr->GetOwner() == name && (pxd::FighterStatus)(int)fghtr->GetStatus() == pxd::FighterStatus::SpecialTournament)
        {
            LOG (WARNING) << "Some of the fighters are in the special tournament already: " << tournamentID;
            fghtr.reset();
            return false;
        }
        
        tryAndStep3 = res3.Step();
    }

    if(a.GetBalance() < 10)
    {
      LOG (WARNING) << "Not enough crystals to join special tournament with id: " << tournamentID;
      tournamentData.reset();
      return false;          
    }

    tournamentData.reset();
    
    return true;
  }  
  
  bool BaseMoveProcessor::ParseTournamentEntryData(const XayaPlayer& a, const std::string& name, const Json::Value& tournament, uint32_t& tournamentID, std::vector<uint32_t>& fighterIDS)
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

    if((pxd::TournamentState)(int)tournamentData->GetInstance().state() != pxd::TournamentState::Listed)
    {
      LOG (WARNING) << "Tournament is already running or completed for id: " << tournamentID;
      tournamentData.reset();
      return false;
    }
	
    if((int)tournamentData->GetInstance().fighters_size() >= (int)tournamentData->GetProto().teamsize() * (int)tournamentData->GetProto().teamcount())
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

          globalData.SetQueueData(json_str);		  
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

              root.append(resEntry);	
              Json::StyledWriter writer;
              std::string json_str = writer.write(root);	

              globalData.SetQueueData(json_str);				  
		  }
	  }
	  
      LOG (WARNING) << "Tournament roster is already full for id, creating new one completely " << tournamentID;
      tournamentData.reset();
	  
      return false;        
    }    	
    
    int32_t sizeBefore = fighterIDS.size();
    
    auto end = fighterIDS.end();
    for (auto it = fighterIDS.begin(); it != end; ++it) {
        end = std::remove(it + 1, end, *it);
    }
 
    fighterIDS.erase(end, fighterIDS.end());    

    int32_t sizeAfter = fighterIDS.size();      

    if(sizeBefore != sizeAfter)
    {
      LOG (WARNING) << "Roster entries must be unique values";
      tournamentData.reset();
      return false;            
    }
    
    //We check fighters now against entry validity
      
    for(long long unsigned int r = 0; r < fighterIDS.size(); r++)
    {
        auto fighter = fighters.GetById (fighterIDS[r], ctx.RoConfig ());
        
        if(fighter == nullptr)
        {
          LOG (WARNING) << "Fatal erorr, could not get fighter with ID" << fighterIDS[r];
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

        if((pxd::FighterStatus)(int)fighter->GetStatus() != pxd::FighterStatus::Available)
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
          LOG (WARNING) << "Fatal erorr, could not get fighter with ID" << fighterAlreadyInside;
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
    if(tournamentData->GetProto().authoredid() == "cbd2e78a-37ce-b864-793d-8dd27788a774" && chain == xaya::Chain::MAIN)
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
  
  bool BaseMoveProcessor::ParseExpeditionData(const XayaPlayer& a, const std::string& name, const Json::Value& expedition, pxd::proto::ExpeditionBlueprint& expeditionBlueprint, std::vector<int>& fightersIds, int32_t& duration, std::string& weHaveApplibeGoodyName)
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
    
    if(expedition["fid"].isInt())
    {
      fightersIds.push_back(expedition["fid"].asInt()); 
    }
    else
    {
        for(auto ft: expedition["fid"])
        {
            fightersIds.push_back(ft.asInt());
        }
        
        //Filter dupes if any
        
        int32_t sizeBefore = fightersIds.size();
        
        auto end = fightersIds.end();
        for (auto it = fightersIds.begin(); it != end; ++it) {
            end = std::remove(it + 1, end, *it);
        }
     
        fightersIds.erase(end, fightersIds.end());    

        int32_t sizeAfter = fightersIds.size();      

        if(sizeBefore != sizeAfter)
        {
          LOG (WARNING) << "Entry contained fighter duplicate ids for " << expedition;
          return false;             
        }            
    }
    
    for(long long unsigned int s =0; s < fightersIds.size(); s++)
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
      if(expeditionBlueprint.authoredid() == "c064e7f7-acbf-4f74-fab8-cccd7b2d4004" && chain == xaya::Chain::MAIN)
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
      
      for(const auto& ongoing: a.GetProto().ongoings())
      {
          if(ongoing.expeditionblueprintid() == expeditionID && ongoing.type() == (uint32_t)pxd::OngoingType::EXPEDITION)
          {
            LOG (WARNING) << "Player already has this expedition ongoing: " << expeditionID;
            return false;                
          }
      }	  
	  
      fighterDD.reset();  
    }
	
	// Additionally, no expedition, if user did not collect reward from previous one
	auto res = rewards.QueryForOwner(a.GetName()); 
	bool tryAndStep = res.Step();
	while (tryAndStep)
	{
	  auto rw = rewards.GetFromResult (res, ctx.RoConfig ());
	  
	  if(rw->GetProto().expeditionid() == expeditionID)
	  {
		LOG (WARNING) << "Reward for this expedition is not collected: " << expeditionID;
		return false;          
	  }
	  
	  tryAndStep = res.Step ();
	}	
	
    const auto& goodiesList = ctx.RoConfig()->goodies();
    
     std::vector<std::pair<std::string, pxd::proto::Goody>> sortedGoodyTypesmap;
        for (auto itr = goodiesList.begin(); itr != goodiesList.end(); ++itr)
            sortedGoodyTypesmap.push_back(*itr);

    sort(sortedGoodyTypesmap.begin(), sortedGoodyTypesmap.end(), [=](std::pair<std::string, pxd::proto::Goody>& a, std::pair<std::string, pxd::proto::Goody>& b)
    {
        return a.first < b.first;
    } 
    );        
    
    fpm::fixed_24_8 reductionPercent = fpm::fixed_24_8(1);
    
    for(const auto& goodie: sortedGoodyTypesmap)
    {
        if(a.GetInventory().GetFungibleCount(goodie.first) > 0)
        {
            if(goodie.second.goodytype() == (int8_t)pxd::GoodyType::Espresso)
            {
                weHaveApplibeGoodyName = goodie.first;
                reductionPercent = fpm::fixed_24_8(goodie.second.reductionpercent());
                break;
            }
        }
    }
    fpm::fixed_24_8 effective_duration = fpm::fixed_24_8(expeditionBlueprint.duration());
    if(weHaveApplibeGoodyName != "")
    {
        effective_duration = effective_duration * reductionPercent;
    }    
    
    duration = (int32_t)effective_duration;
    
    return true;    
  }  
  
  bool
  BaseMoveProcessor::ParseClaimSweetener(const XayaPlayer& a, const std::string& name, const Json::Value& sweetener, uint32_t& fighterID, std::vector<uint32_t>& rewardDatabaseIds, std::string& sweetenerAuthId)
  {
    if (!sweetener.isObject ())
    {
       return false;
    } 

    if (!sweetener["fid"].isInt())
    {
       LOG (WARNING) << "fid is not a int";
       return false;
    } 
    
    fighterID = (uint32_t)sweetener["fid"].asInt();
    auto fighterDb = fighters.GetById (fighterID, ctx.RoConfig ());       
    
    if(fighterDb == nullptr)
    {
      LOG (WARNING) << "Fighter is not present for id : " << fighterID;
      fighterDb.reset();
      return false;               
    }       
    
    if(fighterDb->GetOwner() != a.GetName())
    {
      LOG (WARNING) << "Fighter does not belong to: " << a.GetName();
      fighterDb.reset();
      return false;               
    }   

    if((pxd::FighterStatus)(int)fighterDb->GetStatus() != pxd::FighterStatus::Available) 
    {
      LOG (WARNING) << "Fighter status is not available: " << fighterID;
      fighterDb.reset();
      return false;              
    }    

    auto res = rewards.QueryForOwner(a.GetName()); 
    int totalEntries = 0;
    bool tryAndStep = res.Step();
    while (tryAndStep)
    {
      auto rw = rewards.GetFromResult (res, ctx.RoConfig ());
      
      if(rw->GetProto().fighterid() == fighterDb->GetId() && rw->GetProto().sweetenerid() != "")
      {
        totalEntries++;
        rewardDatabaseIds.push_back(rw->GetId());
        sweetenerAuthId = rw->GetProto().sweetenerid();
      }
      
      tryAndStep = res.Step ();
    }
    
    if(sweetenerAuthId == "")
    {
        LOG (WARNING) << "Could not resolve sweetenerID for fighter: " << fighterDb->GetId();
        return false;              
    }      
   
    if(totalEntries <= 0)
    {
        LOG (WARNING) << "Could not find any relevan rewards for sweetenwer fighter: " << fighterDb->GetId();
        return false;              
    }      
     
    fighterDb.reset();
    return true;  
    
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
    const int rewardID = (uint32_t)sweetener["rid"].asInt();
    std::string sweetenerID = sweetener["sid"].asString();
    
    
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
    
    if((pxd::FighterStatus)(int)fighterDb->GetStatus() != pxd::FighterStatus::Available) 
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
      LOG (WARNING) << "Reward ID is invalid, must be in raonge of 0 to : " << sweetenerBlueprint.rewardchoices_size();
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

	// Additionally, no sweetning, if user did not collect reward from previous one

	auto res = rewards.QueryForOwner(a.GetName()); 
	bool tryAndStep = res.Step();
	while (tryAndStep)
	{
	  auto rw = rewards.GetFromResult (res, ctx.RoConfig ());
	  
	  if(rw->GetProto().fighterid() == fighterID && rw->GetProto().sweetenerid() != "")
	  {
		LOG (WARNING) << "Reward for previous sweetness not collected for fighter id: " << fighterID;
		fighterDb.reset();
		return false;          
	  }
	  
	  tryAndStep = res.Step ();
	}	

     fighterDb.reset();
     return true;   
  }      
  
  bool BaseMoveProcessor::ParseCollectCookRecepie(const XayaPlayer& a, const std::string& name, const Json::Value& recepie, int32_t& fighterID)
  {
    if (!recepie.isObject ())
    return false;
   
    if(!recepie["fid"].isInt())
    {
        return false;
    }      

    fighterID = recepie["fid"].asInt();    
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
    
    if((pxd::FighterStatus)(int)fighter->GetStatus() != pxd::FighterStatus::Cooked) 
    {
      LOG (WARNING) << "Fighter status is not Cooked: " << fighterID;
      fighter.reset();
      return false;              
    }  

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
    
    auto itemInventoryRecipe = GetRecepieObjectFromID(recepieID, ctx);

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

    if(itemInventoryRecipe->GetProto().quality() == 1)
    {
        cookCost = ctx.RoConfig()->params().common_recipe_cook_cost();
    }
    
    if(itemInventoryRecipe->GetProto().quality() == 2)
    {
        cookCost = ctx.RoConfig()->params().uncommon_recipe_cook_cost();
    }

    if(itemInventoryRecipe->GetProto().quality() == 3)
    {
        cookCost = ctx.RoConfig()->params().rare_recipe_cook_cost();
    }

    if(itemInventoryRecipe->GetProto().quality() == 4)
    {
        cookCost = ctx.RoConfig()->params().epic_recipe_cook_cost();
    }  

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
        
        if((pxd::FighterStatus)(int)fighter->GetStatus() != pxd::FighterStatus::Available) 
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
    
     std::vector<std::pair<std::string, pxd::proto::Goody>> sortedGoodyTypesmap;
        for (auto itr = goodiesList.begin(); itr != goodiesList.end(); ++itr)
            sortedGoodyTypesmap.push_back(*itr);

    sort(sortedGoodyTypesmap.begin(), sortedGoodyTypesmap.end(), [=](std::pair<std::string, pxd::proto::Goody>& a, std::pair<std::string, pxd::proto::Goody>& b)
    {
        return a.first < b.first;
    } 
    );     
    
    for(const auto& goodie: sortedGoodyTypesmap)
    {
        if(playerInventory.GetFungibleCount(goodie.first) > 0)
        {
            if(goodie.second.goodytype() == (int8_t)pxd::GoodyType::PressureCooker)
            {
                weHaveApplibeGoodyName = goodie.first;
                reductionPercent = fpm::fixed_24_8(goodie.second.reductionpercent());
                break;
            }
        }
    }
    
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
      recepieIDS.push_back(ft.asInt());

      auto itemInventoryRecipe = GetRecepieObjectFromID(ft.asInt(), ctx);

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
  
  void MoveProcessor::MaybeClaimTournamentReward (const std::string& name, const Json::Value& tournament)
  {  
      std::vector<uint32_t> rewardDatabaseIds;      
      uint32_t tournamentID = 0;
      
      auto a = xayaplayers.GetByName (name, ctx.RoConfig ());
      
      if(!ParseTournamentRewardData(*a, name, tournament, rewardDatabaseIds, tournamentID))
      {
          a.reset();
          return;
      }

      a.reset();
      ClaimRewardsInnerLogic(name, rewardDatabaseIds);   
  }  
  
  void MoveProcessor::ClaimRewardsInnerLogic(std::string name, std::vector<uint32_t> rewardDatabaseIds)
  {
      auto a = xayaplayers.GetByName (name, ctx.RoConfig ());

      uint32_t curRecipeSlots = recipeTbl.CountForOwner(a->GetName());
      uint32_t maxRecipeSlots = ctx.RoConfig()->params().max_recipe_inventory_amount();
      
      std::vector<uint32_t> markedToRemove;
      
      const auto& rewardsList = ctx.RoConfig()->activityrewards();
      for(const auto& rw: rewardDatabaseIds)
      {
          /** Lets fetch reward from database, and grant it to the player. 
              While doing so, lets check against space limitations in inv. for recepies    
          */
          LOG (INFO) << "Ready to give with id " << rw;
          
          auto rewardData = rewards.GetById(rw);          
          pxd::proto::ActivityReward rewardTableDb;
 
          bool rewardsSolved = false;
          
          for(const auto& rewardsTable: rewardsList)
          {
              if(rewardsTable.second.authoredid() == rewardData->GetProto().rewardid())
              {
                  rewardTableDb = rewardsTable.second;
                  rewardsSolved = true;
                  break;
              }
          }           
          
          if(rewardsSolved == false)
          {
              LOG (ERROR) << "Fatal error, could not solve: " << rewardData->GetProto().rewardid();
              markedToRemove.push_back(rw);
              rewardData.reset();
              continue;
          }
          
          if((pxd::RewardType)(int)rewardTableDb.rewards(rewardData->GetProto().positionintable()).type() == pxd::RewardType::CraftedRecipe || (pxd::RewardType)(int)rewardTableDb.rewards(rewardData->GetProto().positionintable()).type() == pxd::RewardType::GeneratedRecipe)
          {
              if(curRecipeSlots >= maxRecipeSlots)
              {
                  LOG (INFO) << "Can not grant recipe, maxomus lots reached  of" << curRecipeSlots << " where max is " << maxRecipeSlots;
                  markedToRemove.push_back(rw);
                  rewardData.reset();
                  continue;
              }
              
              auto ourRec = recipeTbl.GetById(rewardData->GetProto().generatedrecipeid());
              ourRec->SetOwner(a->GetName());
              
              LOG (INFO) << "Granted " << " recipe " << rewardData->GetProto().generatedrecipeid() << " reward";
              curRecipeSlots++;
              ourRec.reset();              
          }
          
          else if((pxd::RewardType)(int)rewardTableDb.rewards(rewardData->GetProto().positionintable()).type() == pxd::RewardType::Candy)
          {              
              const std::string candyName = GetCandyKeyNameFromID(rewardTableDb.rewards(rewardData->GetProto().positionintable()).candytype(), ctx);
            
              a->GetInventory().AddFungibleCount(candyName, rewardTableDb.rewards(rewardData->GetProto().positionintable()).quantity());
              
              LOG (INFO) << "Granted " << rewardTableDb.rewards(rewardData->GetProto().positionintable()).quantity() << " candy " << candyName << " reward";
          }          
          
          else if((pxd::RewardType)(int)rewardTableDb.rewards(rewardData->GetProto().positionintable()).type() == pxd::RewardType::GeneratedRecipe)
          {
              if(curRecipeSlots >= maxRecipeSlots)
              {
                  LOG (INFO) << "Can not grant recipe, maxomus lots reached  of" << curRecipeSlots << " where max is " << maxRecipeSlots;
                  markedToRemove.push_back(rw);
                  rewardData.reset();
                  continue;
              }
              
              auto ourRec = recipeTbl.GetById(rewardData->GetProto().generatedrecipeid());
              ourRec->SetOwner(a->GetName());  

              LOG (INFO) << "Granted " << " recipe " << rewardData->GetProto().generatedrecipeid() << " as reward";  
              curRecipeSlots++;
              ourRec.reset();              
          }          
          
          else if((pxd::RewardType)(int)rewardTableDb.rewards(rewardData->GetProto().positionintable()).type() == pxd::RewardType::Move)
          {
              auto fighter = fighters.GetById(rewardData->GetProto().fighterid(), ctx.RoConfig());
              std::string* newMove = fighter->MutableProto().add_moves();
              newMove->assign(rewardTableDb.rewards(rewardData->GetProto().positionintable()).moveid());
              
              LOG (INFO) << "Granted " << " move to figher " << rewardData->GetProto().fighterid() << " as reward";   
          }   

          else if((pxd::RewardType)(int)rewardTableDb.rewards(rewardData->GetProto().positionintable()).type() == pxd::RewardType::Animation)
          {
              auto fighter = fighters.GetById(rewardData->GetProto().fighterid(), ctx.RoConfig());
              fighter->MutableProto().set_animationid(rewardTableDb.rewards(rewardData->GetProto().positionintable()).animationid()); //TODO into repeated list? Prob... But ok, ignore for now;
          
              LOG (INFO) << "Granted " << " animation to figher " << rewardData->GetProto().fighterid() << " as reward"; 
          }   

          else if((pxd::RewardType)(int)rewardTableDb.rewards(rewardData->GetProto().positionintable()).type() == pxd::RewardType::Armor)
          {
              auto fighter = fighters.GetById(rewardData->GetProto().fighterid(), ctx.RoConfig());
              
              bool armorTypeWasPresent = false;
              for(auto armorPiece: fighter->MutableProto().armorpieces())
              {
                  if(armorPiece.armortype() == rewardTableDb.rewards(rewardData->GetProto().positionintable()).armortype())
                  {
                      armorTypeWasPresent = true;
                      armorPiece.set_candy(rewardTableDb.rewards(rewardData->GetProto().positionintable()).candytype());
                      armorPiece.set_rewardsourceid(rewardData->GetProto().rewardid());
                      armorPiece.set_rewardsource((uint32_t)pxd::RewardSource::Expedition);
                  }
              }
              
              if(armorTypeWasPresent == false)
              {
                  auto newArmorPiece = fighter->MutableProto().add_armorpieces();
                  newArmorPiece->set_candy(rewardTableDb.rewards(rewardData->GetProto().positionintable()).candytype());
                  newArmorPiece->set_rewardsourceid(rewardData->GetProto().rewardid());
                  newArmorPiece->set_rewardsource((uint32_t)pxd::RewardSource::Expedition);   
                  newArmorPiece->set_armortype(rewardTableDb.rewards(rewardData->GetProto().positionintable()).armortype());                  
              }
              
              LOG (INFO) << "Granted " << " armor to figher " << rewardData->GetProto().fighterid();
          } 
          
          else
          {
             LOG (INFO) << "Unknown reward type was not granted " << rewardTableDb.rewards(rewardData->GetProto().positionintable()).type();
          }              
          
          rewards.DeleteById(rw);
      }      
      
      a.reset();
      
      for(auto entry: markedToRemove)
      {
          rewards.DeleteById(entry);
      }
  }
  
  void MoveProcessor::MaybeClaimReward (const std::string& name, const Json::Value& expedition)
  {      
      std::vector<uint32_t> rewardDatabaseIds;      
      std::vector<std::string> expeditionIDArray;
      
      auto a = xayaplayers.GetByName (name, ctx.RoConfig ());
      if(!ParseRewardData(*a, name, expedition, rewardDatabaseIds, expeditionIDArray))      
      {
          a.reset();
          return;
      }
      
      a.reset();
      ClaimRewardsInnerLogic(name, rewardDatabaseIds);  
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
    for(long long unsigned int r = 0; r < fighterIDS.size(); r++)
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

    a->AddBalance(-tournamentData->GetProto().joincost()); 
    tournamentData.reset();
    
    a.reset();
    
    LOG (INFO) << "Tournament " << tournamentID << " leaved succesfully ";
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
    if(!ParseDeconstructRewardData(*a, name, fighter, fighterID))      
    {
      a.reset();
      return;
    }

    std::vector<uint32_t> rewardDatabaseIds;
    auto res = rewards.QueryForOwner(a->GetName());
    
    auto fighterDb = fighters.GetById (fighterID, ctx.RoConfig ());  
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
    a->CalculatePrestige(ctx.RoConfig());
    a.reset();
    
    
  } 

void MoveProcessor::MaybePutFighterForSale (const std::string& name, const Json::Value& fighter)
 {      
    auto a = xayaplayers.GetByName (name, ctx.RoConfig ());
    
    if(a == nullptr)
    {
      LOG (INFO) << "Player " << name << " not found";
      return;
    }
    
    uint32_t fighterID = 0;
    uint32_t duration = 0;
    Amount price = 0;
    Amount listingfee = 0;
    
    if(!ParseFighterForSaleData(*a, name, fighter, fighterID, duration, price, listingfee))      
    {
      a.reset();
      return;
    }

    auto fighterDb = fighters.GetById (fighterID, ctx.RoConfig ());
    fighterDb->SetStatus(pxd::FighterStatus::Exchange);
    
    a->AddBalance(-listingfee);
    
    int secondsInOneDay = 24 * 60 * 60;
    int blocksInOneDay = secondsInOneDay / 30;
    fighterDb->MutableProto().set_exchangeexpire(ctx.Height () + (duration * blocksInOneDay));
    fighterDb->MutableProto().set_exchangeprice(price);
    
    fighterDb.reset(); 
    a.reset();        
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
 }

 fpm::fixed_24_8 MoveProcessor::CalculateFuelPower(const Json::Value& fighter, const Json::Value& wholeFighterData, const Json::Value& wholeRecipeData, const Json::Value& candylist, bool outputDebug)
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
	
	    if(outputDebug) LOG (WARNING) << "basedQualityCost: " << (int)basedQualityCost;  
		if(outputDebug) LOG (WARNING) << "sumToAdd: " << (int)sumToAdd;
	
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

        if(outputDebug) LOG (WARNING) << "namesUsed: " << (int)namesUsed.size();
		
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
		
		if(outputDebug) LOG (WARNING) << "armorUsed: " << (int)armorUsed.size();
		
		fpm::fixed_24_8 armorDiversitiCoefficient = (uniqueArmorCollected* fpm::fixed_24_8(1)) / fpm::fixed_24_8(8);
		
		
		if(outputDebug) LOG (WARNING) << "armorDiversitiCoefficient: " << (int)(armorDiversitiCoefficient *  fpm::fixed_24_8(1000));
		if(outputDebug) LOG (WARNING) << "nameDiversitiCoefficient: " << (int)(nameDiversitiCoefficient *  fpm::fixed_24_8(1000));
		if(outputDebug) LOG (WARNING) << "sweetnerDiversitiCoefficient: " << (int)(sweetnerDiversitiCoefficient *  fpm::fixed_24_8(1000));
		if(outputDebug) LOG (WARNING) << "qualityDiversitiCoefficient: " << (int)(qualityDiversitiCoefficient *  fpm::fixed_24_8(1000));
		if(outputDebug) LOG (WARNING) << "ratingDiversitiCoefficient: " << (int)(ratingDiversitiCoefficient *  fpm::fixed_24_8(1000));
		
		diversityFinalCoefficient = diversityFinalCoefficient + armorDiversitiCoefficient + nameDiversitiCoefficient + sweetnerDiversitiCoefficient + qualityDiversitiCoefficient + ratingDiversitiCoefficient;
		
		if(diversityFinalCoefficient > fpm::fixed_24_8(1))
		{
			diversityFinalCoefficient = fpm::fixed_24_8(1);
		}
		
		if(diversityFinalCoefficient < fpm::fixed_24_8(0))
		{
			diversityFinalCoefficient = fpm::fixed_24_8(0);
		}
		
		if(outputDebug) LOG (WARNING) << "totalCrystalCost: " << (int)totalCrystalCost;  
		if(outputDebug) LOG (WARNING) << "diversityFinalCoefficient: " << (int)(diversityFinalCoefficient *  fpm::fixed_24_8(1000));  
		
		totalCrystalCost = totalCrystalCost * diversityFinalCoefficient;
        fuel80 = fuel80 + totalCrystalCost;
		
		if(outputDebug) LOG (WARNING) << "fuel80: " << (int)(fuel80 *  fpm::fixed_24_8(1000));		
	}
	
	// now that we have fuel80 from sacrificed treats, lets look into recepies and candies
	
	fpm::fixed_24_8 candyRarityAccumulated = fpm::fixed_24_8(0);

    for(auto& candy : itemCandy)
	{
		std::string candyAuth = candy["n"].asString();
		fpm::fixed_24_8 cRarity = fpm::fixed_24_8(0);
		
        for(const auto& candyR: candylist)
        {
          if(candyR["i"].asString() == candyAuth)
          {	
	         cRarity = fpm::fixed_24_8(0.1); //fpm::fixed_24_8(candyR["r"].asInt());
	         break;
		  }
		}
		
        fpm::fixed_24_8 ca = fpm::fixed_24_8(candy["a"].asInt());		
		fpm::fixed_24_8 candySets = ca / fpm::fixed_24_8(10);

		candyRarityAccumulated = candyRarityAccumulated + (candySets * cRarity);	

        if(outputDebug) LOG (WARNING) << "candyRarityAccumulated: " << (int)(candyRarityAccumulated *  fpm::fixed_24_8(1000));		
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
		
		if(outputDebug) LOG (WARNING) << "recipeRarityAccumulated: " << (int)(recipeRarityAccumulated *  fpm::fixed_24_8(1000));
	}	
	
	// Now we can construct final fuel_20 value
	
	fuel20 = recipeRarityAccumulated + candyRarityAccumulated;
	
	if(outputDebug) LOG (WARNING) << "fuel20: " << (int)(fuel20 *  fpm::fixed_24_8(1000));
	
	// so, fuel80 is totcal crustal cost, and fuel20 is rarity	
	// Firstly, lets imagine, how much fule it would be if 80 were actually 20
	
	fpm::fixed_24_8 imaginary100 = fuel80 * fpm::fixed_24_8(1.25);
	fpm::fixed_24_8 missingFuelPoints = imaginary100 - fuel80;
	
	if(outputDebug) LOG (WARNING) << "imaginary100: " << (int)(imaginary100 *  fpm::fixed_24_8(1000));
	
	if(missingFuelPoints > fuel20)
	{
		missingFuelPoints = fuel20;
	}

	fpm::fixed_24_8 fuelPower = (fuel80 + missingFuelPoints) *  fpm::fixed_24_8(100);
	if(outputDebug) LOG (WARNING) << "fuelPower: " << (int)(fuelPower);

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
	
    if(!ParseTransfigureData(*a, name, fighter))      
    {
      a.reset();
      return;
    }	
	
	
	Json::Value wholeFightersData(Json::objectValue);
	Json::Value wholeRecipeData(Json::objectValue);
	Json::Value candyList(Json::arrayValue);
	
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
	
	const auto& ccds = ctx.RoConfig()->candies();
    for(auto& ic : ccds)
	{
		Json::Value cand(Json::objectValue);
		cand["i"] = ic.second.authoredid();
		cand["r"] = 10;	
		
		candyList.append(cand);
	}
	
	wholeFightersData["crcc"] = ctx.RoConfig()->params().uncommon_recipe_cook_cost();
	wholeFightersData["urcc"] = ctx.RoConfig()->params().uncommon_recipe_cook_cost();
	wholeFightersData["rrcc"] = ctx.RoConfig()->params().uncommon_recipe_cook_cost();
	wholeFightersData["ercc"] = ctx.RoConfig()->params().uncommon_recipe_cook_cost();
	
	fpm::fixed_24_8 fuelPower = CalculateFuelPower(fighter, wholeFightersData, wholeRecipeData, candyList, false);

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
				std::vector<Amount> lookUpPrices =  {84000000,252000000,672000000,924000000,1344000000,1764000000,2268000000,2772000000,3276000000,3948000000,4620000000,5376000000,5796000000,6216000000,6720000000,7140000000,7644000000,8148000000,8736000000,9324000000,9912000000,10500000000,10836000000,11172000000,11508000000,11844000000,12180000000,12516000000,12852000000,13188000000,13608000000,13944000000,14364000000,14784000000,15120000000,15540000000,15960000000,16380000000,16800000000,17220000000,17724000000,18144000000,18480000000,18900000000,19236000000,19656000000,19992000000,20412000000,20832000000,21168000000,21588000000,22008000000,22428000000,22848000000,23268000000,23688000000,24108000000,24612000000,25032000000,25452000000,25956000000,26376000000,26880000000,27384000000,27804000000,28308000000,28812000000,29064000000,29316000000,29568000000,29820000000,30072000000,30324000000,30576000000,30828000000,31080000000,31332000000,31584000000,31920000000,32172000000,32424000000,32676000000,32928000000,33180000000,33516000000,33768000000,34020000000,34356000000,34608000000,34860000000,35196000000,35448000000,35700000000,36036000000,36288000000,36624000000,36876000000,37128000000,37464000000,37716000000,38052000000,38388000000,38640000000,38976000000,39228000000,39564000000,39900000000,40152000000,40488000000,40824000000,41076000000,41412000000,41748000000,42084000000,42336000000,42672000000,43008000000,43176000000,43344000000,43512000000,43680000000,43848000000,44016000000,44184000000,44352000000,44436000000,44604000000,44772000000,44940000000,45108000000,45276000000,45444000000,45612000000,45780000000,45948000000,46116000000,46284000000,46452000000,46620000000,46788000000,46956000000,47208000000,47376000000,47544000000,47712000000,47880000000,48048000000,48216000000,48384000000,48552000000,48720000000,48888000000,49056000000,49224000000,49392000000,49644000000,49812000000,49980000000,50148000000,50316000000,50484000000,50652000000,50820000000,51072000000,51240000000,51408000000,51576000000,51744000000,51912000000,52164000000,52332000000,52500000000,52668000000,52836000000,53088000000,53256000000,53424000000,53592000000,53844000000,54012000000,54180000000,54348000000,54516000000,54768000000,54936000000,55104000000,55356000000,55524000000,55692000000,55860000000,56112000000,56280000000,56448000000,56700000000,56868000000,57036000000,57204000000,57456000000,57624000000,57792000000,58044000000,58212000000,58464000000,58632000000,58800000000,59052000000,59220000000,59388000000,59640000000,59808000000,60060000000,60228000000,60396000000,60648000000,60816000000,61068000000,61236000000,61404000000,61656000000,61824000000,62076000000,62244000000,62496000000,62664000000,62916000000,63084000000,63336000000,63504000000,63756000000,63924000000,64176000000,64344000000,64596000000,64764000000,65016000000,65184000000,65436000000,65604000000,65856000000,66024000000,66276000000,66444000000,66696000000,66948000000,67116000000,67368000000,67536000000,67788000000,68040000000,68208000000,68460000000,68628000000,68880000000,69132000000,69300000000,69552000000,69804000000,69972000000,70224000000,70476000000,70644000000,70896000000,71148000000,71316000000,71568000000,71820000000,71988000000,72240000000,72492000000,72744000000,72912000000,73164000000,73416000000,73584000000,73836000000,74088000000,74340000000,74592000000,74760000000,75012000000,75264000000,75516000000,75684000000,75936000000,76188000000,76440000000,76692000000,76860000000,77112000000,77364000000,77616000000,77868000000,78120000000,78372000000,78540000000,78792000000,79044000000,79296000000,79548000000,79800000000,80052000000,80304000000,80556000000,80808000000,80976000000,81228000000,81480000000,81732000000,81984000000,82236000000,82488000000,82740000000,82992000000,83244000000,83496000000,83748000000};
				
				if(fuelPowerUnit - fpm::fixed_24_8(4) >= fpm::fixed_24_8(lookUpPrices.size()))
				{
					fuelPowerUnit = fpm::fixed_24_8(lookUpPrices.size()) - fpm::fixed_24_8(1);
				}
				
				cost = lookUpPrices[(int)(fuelPowerUnit-fpm::fixed_24_8(4))];
			}	
			fighterDb->RerollName(cost, ctx.RoConfig (), rnd, nQL);
		}
		
		// Armor	
		if(optionID == 2)
		{		
			fpm::fixed_24_8 basedQualityLevel = fpm::fixed_24_8(fighterDb->GetProto().quality());
			fpm::fixed_24_8 minimumPowerToPass = fpm::fixed_24_8(basedQualityLevel);
			
			int32_t rollSuccess = rnd.NextInt((int)fuelPowerUnit+1);
			fpm::fixed_24_8 valueRolled = fpm::fixed_24_8(rollSuccess);
			
			LOG (WARNING) << "Rolled for armor " << (int)valueRolled << " with minimum power needed to pass " << (int)minimumPowerToPass;
			
			if(valueRolled >= minimumPowerToPass)
			{
				const auto& fighterMoveBlueprintList = ctx.RoConfig ()->fightermoveblueprints();
				std::map<pxd::ArmorType, std::string> slotsUsed;
				
				for(auto& armorPiece : fighterDb->GetProto().armorpieces())
				{
					slotsUsed.insert(std::pair<pxd::ArmorType, std::string>((pxd::ArmorType)armorPiece.armortype(), armorPiece.candy()));		
				}

				std::vector<std::pair<std::string, pxd::proto::FighterMoveBlueprint>> sortedMoveBlueprintTypesmap;
				for (auto itr = fighterMoveBlueprintList.begin(); itr != fighterMoveBlueprintList.end(); ++itr)
					  sortedMoveBlueprintTypesmap.push_back(*itr);

				sort(sortedMoveBlueprintTypesmap.begin(), sortedMoveBlueprintTypesmap.end(), [=](std::pair<std::string, pxd::proto::FighterMoveBlueprint>& a, std::pair<std::string, pxd::proto::FighterMoveBlueprint>& b)
				{
					  return a.first < b.first;
				} 
				);  

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
			
			std::vector<std::pair<std::string, pxd::proto::FighterMoveBlueprint>> sortedMoveBlueprintTypesmap;
			for (auto itr = moveBlueprints.begin(); itr != moveBlueprints.end(); ++itr)
				sortedMoveBlueprintTypesmap.push_back(*itr);

			sort(sortedMoveBlueprintTypesmap.begin(), sortedMoveBlueprintTypesmap.end(), [=](std::pair<std::string, pxd::proto::FighterMoveBlueprint>& a, std::pair<std::string, pxd::proto::FighterMoveBlueprint>& b)
			{
				return a.first < b.first;
			} 
			);     			
			
            if(fuelPowerUnit >= fpm::fixed_24_8(fighterDb->GetProto().quality()))
			{			
		        if(fuelPowerUnit > fpm::fixed_24_8(39))
				{
					fuelPowerUnit = fpm::fixed_24_8(39);
				}
				
				int32_t rollSuccess = rnd.NextInt(40);
				
				LOG (WARNING) << "Rolled for move " << rollSuccess << " with treshhold needed to pass " << (int)fuelPowerUnit;
				
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
    if(!ParseDeconstructData(*a, name, fighter, fighterID))      
    {
      a.reset();
      return;
    }

    auto fighterDb = fighters.GetById (fighterID, ctx.RoConfig ());
    fighterDb->SetStatus(pxd::FighterStatus::Deconstructing);
    
    proto::OngoinOperation* newOp = a->MutableProto().add_ongoings();
    
    newOp->set_type((uint32_t)pxd::OngoingType::DECONSTRUCTION);
    newOp->set_fighterdatabaseid(fighterID);  
    newOp->set_blocksleft(ctx.RoConfig()->params().deconstruction_blocks());
    
    fighterDb.reset(); 
    a.reset();    
    
  }        
  
 void MoveProcessor::MaybeRemoveFighterFromExchange(const std::string& name, const Json::Value& fighter)
 {      
    auto a = xayaplayers.GetByName (name, ctx.RoConfig ());
    
    if(a == nullptr)
    {
      LOG (INFO) << "Player " << name << " not found";
      return;
    }
    
    uint32_t fighterID = 0;
    if(!ParseRemoveBuyData(*a, name, fighter, fighterID))      
    {
      a.reset();
      return;
    }

    auto fighterDb = fighters.GetById (fighterID, ctx.RoConfig ());
    fighterDb->SetStatus(pxd::FighterStatus::Available);
    fighterDb.reset();
  }    
  
 void MoveProcessor::MaybeBuyFighterFromExchange(const std::string& name, const Json::Value& fighter)
 {      
    auto a = xayaplayers.GetByName (name, ctx.RoConfig ());
    
    if(a == nullptr)
    {
      LOG (INFO) << "Player " << name << " not found";
      return;
    }
    
    uint32_t fighterID = 0;
    Amount exchangeprice = 0;
    if(!ParseBuyData(*a, name, fighter, fighterID, exchangeprice))      
    {
      a.reset();
      return;
    }

    auto fighterDb = fighters.GetById (fighterID, ctx.RoConfig ());
    fighterDb->SetStatus(pxd::FighterStatus::Available);
    auto a2 = xayaplayers.GetByName (fighterDb->GetOwner(), ctx.RoConfig ());

    a->AddBalance (-exchangeprice);

    
    fpm::fixed_24_8 reductionPercent = fpm::fixed_24_8(ctx.RoConfig()->params().exchange_sale_percentage());
    fpm::fixed_24_8 priceToPay = fpm::fixed_24_8(exchangeprice);
    fpm::fixed_24_8 finalPrice = priceToPay * reductionPercent; 

    if(finalPrice == priceToPay)
	{
		finalPrice = priceToPay - 1;
	}
	
    a2->AddBalance((int32_t)finalPrice);

    proto::FighterSaleEntry* newSale = fighterDb->MutableProto().add_salehistory();    
    newSale->set_selltime(ctx.Timestamp());
    newSale->set_price(exchangeprice);
    newSale->set_fromowner(fighterDb->GetOwner());
    newSale->set_toowner(name);

    fighterDb->SetOwner(name);
    fighterDb.reset();
     
    a->CalculatePrestige(ctx.RoConfig());
    a2->CalculatePrestige(ctx.RoConfig());

    a.reset();  
    a2.reset();
	
	RecalculatePlayerTiers(db, ctx); 	
  }   
  
  void MoveProcessor::RecalculatePlayerTiers(Database& db, const Context& ctx)
  {
    // All players must be distributed evenly between special tournament tiers
    // based on their rating. This gets a bit messy, if ratings are similar
    // at the beginning of the game, so ...
    
    // Minimum gap between tiers rating has to be 500
    // If not, maximum tiers will not be reachable at the beginning of the game, until more rating accumulated
    
    // Then, players are evenly distributed amoung 10 tiers
    
    XayaPlayersTable xayaplayers(db);
    
    auto res = xayaplayers.QueryAll ();

    std::map<std::string,int64_t> playerPrestigeCollection;
    int64_t highestPrestige = 0;
    
    bool tryAndStep = res.Step();
    while (tryAndStep)
    {
      auto a = xayaplayers.GetFromResult (res, ctx.RoConfig ());
      
      int64_t prst = a->GetPresitge();
      
      playerPrestigeCollection.insert(std::pair<std::string, int64_t>(a->GetName(), prst));
      
      if(highestPrestige < prst)
      {
         highestPrestige = prst;
      }          
      
      a.reset();
      tryAndStep = res.Step ();
    }
    
    int64_t oneGradeStep = highestPrestige / 7;
    
    if(oneGradeStep < 500)
    {
        oneGradeStep = 500;
    }
    
    for(auto& player : playerPrestigeCollection)
    {
        int32_t pTier = std::round(((player.second - 1000)) / oneGradeStep);
        
        if(pTier < 1)
        {
            pTier = 1;
        }
        
        auto a = xayaplayers.GetByName(player.first, ctx.RoConfig());
        a->MutableProto().set_specialtournamentprestigetier(pTier);
        a.reset();
    }
  }  

  void MoveProcessor::MaybeEnterSpecialTournament (const std::string& name, const Json::Value& tournament)
  { 
    auto a = xayaplayers.GetByName (name, ctx.RoConfig ());
    
    if(a == nullptr)
    {
      LOG (INFO) << "Player " << name << " not found";
      return;
    }
    
    uint32_t tournamentID = 0;
    std::vector<uint32_t> fighterIDS;   
    
    if(!ParseSpecialTournamentEntryData(*a, name, tournament, tournamentID, fighterIDS))       
    {
      a.reset();
      return;
    }

    //auto tournamentData = specialTournamentsTbl.GetById(tournamentID, ctx.RoConfig ());
    
    for(long long unsigned int r = 0; r < fighterIDS.size(); r++)
    {
        auto fighter = fighters.GetById (fighterIDS[r], ctx.RoConfig ());
        fighter->SetStatus(pxd::FighterStatus::SpecialTournament);
        fighter->MutableProto().set_specialtournamentinstanceid(tournamentID);
        fighter->MutableProto().set_specialtournamentstatus((int)pxd::SpecialTournamentStatus::Listed);
      
        LOG (INFO) << "Fighter " << fighterIDS[r] << " enters special tournament ";
        
        fighter.reset();
    }

    a->AddBalance (-10); 
    //tournamentData.reset();
    
    a.reset();
    
    LOG (WARNING) << "Special Tournament " << tournamentID << " entered succesfully ";  
  }  
  
  void MoveProcessor::MaybeSQLTestInjection2(const std::string& name, const Json::Value& injection)
  {
    const auto& cmd = injection["injection2"];
    if (!cmd.isObject ()) 
    {
        return;   
    }
    
    XayaPlayersTable xayaplayers(db);
    FighterTable tbl3(db);
    RecipeInstanceTable tbl2(db);
    
    for(int d = 151; d < 161; d++)
    {
      std::ostringstream s;
      s << "testft" << d;
      std::string nName(s.str());        

      auto xp = xayaplayers.CreateNew (nName, ctx.RoConfig(), rnd);
      // Here we want to boost player prestige to make him reach higher special touenament tiers, soo..
      xp->MutableProto().set_tournamentswon(rnd.NextInt(d));  
      xp->MutableProto().set_tournamentscompleted(d); 
      xp.reset();
      
      int fCtt = rnd.NextInt(14 + 60);
      
      for(int se = 0; se < fCtt; se++)
      {
        auto r0 = tbl2.CreateNew(nName, "5864a19b-c8c0-2d34-eaef-9455af0baf2c", ctx.RoConfig());
        int iDD = r0->GetId();
        r0.reset();          
      
        auto ft1 = tbl3.CreateNew (nName, iDD, ctx.RoConfig(), rnd);
        ft1.reset();
      }
      
      xp = xayaplayers.GetByName(nName, ctx.RoConfig());
      xp->CalculatePrestige(ctx.RoConfig());
      LOG (WARNING) << "Injected " << " prestinge is " << xp->GetPresitge();
      
      xp.reset();
      
    }
    
    LOG (WARNING) << "Injected";
  }  
  
  void MoveProcessor::MaybeSQLTestInjection(const std::string& name, const Json::Value& injection)
  {
    const auto& cmd = injection["injection"];
    if (!cmd.isObject ()) 
    {
        return;   
    }
    
    XayaPlayersTable xayaplayers(db);
    FighterTable tbl3(db);
    RecipeInstanceTable tbl2(db);
    
    for(int d = 0; d < 150; d++)
    {
      std::ostringstream s;
      s << "testft" << d;
      std::string nName(s.str());        

      auto xp = xayaplayers.CreateNew (nName, ctx.RoConfig(), rnd);
      xp.reset();
      
      for(int se = 0; se < 4; se++) // we want to have exactly 6 (2+4) to test special tournaments
      {
        auto r0 = tbl2.CreateNew(nName, "5864a19b-c8c0-2d34-eaef-9455af0baf2c", ctx.RoConfig());
        int iDD = r0->GetId();
        r0.reset();          
      
        auto ft1 = tbl3.CreateNew (nName, iDD, ctx.RoConfig(), rnd);
        ft1.reset();
      }
    }
    
    LOG (WARNING) << "Injected";
  }
  
  void MoveProcessor::MaybeLeaveSpecialTournament (const std::string& name, const Json::Value& tournament)
  {    
    auto a = xayaplayers.GetByName (name, ctx.RoConfig ());
    
    if(a == nullptr)
    {
      LOG (INFO) << "Player " << name << " not found";
      return;
    }
    
    uint32_t tournamentID = 0;
    std::vector<uint32_t> fighterIDS;

    if(!ParseSpecialTournamentLeaveData(*a, name, tournament, tournamentID, fighterIDS))      
    {
      a.reset();
      return;
    }

    auto tournamentData = specialTournamentsTbl.GetById(tournamentID, ctx.RoConfig ());   
    for(long long unsigned int r = 0; r < fighterIDS.size(); r++)
    {
        auto fighter = fighters.GetById (fighterIDS[r], ctx.RoConfig ());
        
        if(fighter->GetOwner() == name)
        {
          fighter->MutableProto().set_specialtournamentinstanceid(0);
          fighter->SetStatus(pxd::FighterStatus::Available);
          fighter->MutableProto().set_specialtournamentstatus((int)pxd::SpecialTournamentStatus::Listed);
          
          LOG (INFO) << "Fighter " << fighter->GetId() << " leaves tournament ";
          
          fighter.reset();
        }
    }

    tournamentData.reset();    
    a.reset();
    
    LOG (INFO) << "Special tournament " << tournamentID << " leaved succesfully ";  
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
    
    if(!ParseTournamentEntryData(*a, name, tournament, tournamentID, fighterIDS))       
    {
      a.reset();
      return;
    }

    auto tournamentData = tournamentsTbl.GetById(tournamentID, ctx.RoConfig ());
    
    for(long long unsigned int r = 0; r < fighterIDS.size(); r++)
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
    
    LOG (INFO) << "Tournament " << tournamentID << " entered succesfully ";
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
    std::vector<int> fighter;
    
    if(!ParseExpeditionData(*a, name, expedition, expeditionBlueprint, fighter, duration, weHaveApplibeGoodyName)) return;
    
    for(auto& ft: fighter)
    {
      auto fighterDD = fighters.GetById (ft, ctx.RoConfig ());
        
      proto::OngoinOperation* newOp = a->MutableProto().add_ongoings();
      
      newOp->set_type((uint32_t)pxd::OngoingType::EXPEDITION);
      newOp->set_fighterdatabaseid(ft);
      newOp->set_expeditionblueprintid(expedition["eid"].asString ());

      fighterDD->SetStatus(FighterStatus::Expedition);
      fighterDD->MutableProto().set_expeditioninstanceid(expedition["eid"].asString ());
      
      if(weHaveApplibeGoodyName != "")
      {
          newOp->set_appliedgoodykeyname(weHaveApplibeGoodyName);
      }
      
      /** We need to make it at least 1 block, else if will make no sense executing immediately logic flow wise*/
      if(duration < 1)
      {
          duration = 1;
      }    
          
      newOp->set_blocksleft(duration);    
      LOG (INFO) << "Expedition " << expedition << " submitted succesfully ";
      fighterDD.reset();
    }
    
    if(weHaveApplibeGoodyName != "")
    {
      a->GetInventory().AddFungibleCount(weHaveApplibeGoodyName, -1);
    }    
    
    a.reset();
  }    
  
  void
  MoveProcessor::MaybeClaimSweetenerReward (const std::string& name, const Json::Value& sweetener)
  {    
    auto a = xayaplayers.GetByName (name, ctx.RoConfig ());
    
    if(a == nullptr)
    {
      LOG (INFO) << "Player " << name << " not found";
      return;
    }    
    
    uint32_t fighterID = -1; 
    std::vector<uint32_t> rewardDatabaseIds;  
    std::string sweetenerAuthId = "";
    
    if(!ParseClaimSweetener(*a, name, sweetener, fighterID, rewardDatabaseIds, sweetenerAuthId)) return;    
    
    a.reset();    
    ClaimRewardsInnerLogic(name, rewardDatabaseIds); 
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
    
    
    proto::OngoinOperation* newOp = a->MutableProto().add_ongoings();

    newOp->set_type((uint32_t)pxd::OngoingType::COOK_SWEETENER);
    newOp->set_recipeid(fighterDb->GetProto().recipeid());
    newOp->set_appliedgoodykeyname(sweetener["sid"].asString());
    newOp->set_rewardid(sweetener["rid"].asInt());
    newOp->set_fighterdatabaseid(fighterID);
    
    fighterDb.reset();
    
    /** We need to make it at least 1 block, else if will make no sense executing immediately logic flow wise*/
    if(duration < 1)
    {
        duration = 1;
    }    
    newOp->set_blocksleft(duration);    
  }  
  
  void MoveProcessor::MaybeCollectCookedRecepie(const std::string& name, const Json::Value& recepie)
  {     
      auto a = xayaplayers.GetByName (name, ctx.RoConfig ());  
      
      if(a == nullptr)
      {
        LOG (INFO) << "Player " << name << " not found";
        return;
      }    
      
      int32_t fighterID = 0;
      
      if(!ParseCollectCookRecepie(*a, name, recepie, fighterID)) return;     

      auto fighter = fighters.GetById(fighterID, ctx.RoConfig());
        
      if(fighter != nullptr)
      {
         fighter->SetStatus(pxd::FighterStatus::Available);          
      }
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
      auto itemInventoryRecipe = GetRecepieObjectFromID(rcp, ctx);
      itemInventoryRecipe->SetOwner("");
      itemInventoryRecipe.reset();
    }
    
    a->CalculatePrestige(ctx.RoConfig());
    a.reset();
    LOG (INFO) << "Destroy instance " << recepie << " submitted succesfully ";
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
        
        if(fighter != nullptr)
        {
          fighters.DeleteById(fighter->GetId());
          a->CalculatePrestige(ctx.RoConfig());
        }
    }

    proto::OngoinOperation* newOp = a->MutableProto().add_ongoings();

    newOp->set_type((uint32_t)pxd::OngoingType::COOK_RECIPE);
    newOp->set_recipeid(recepie["rid"].asInt());
    
    auto itemInventoryRecipe = GetRecepieObjectFromID((uint32_t)recepie["rid"].asInt(), ctx);
    itemInventoryRecipe->SetOwner("");
    

    if(weHaveApplibeGoodyName != "")
    {
        playerInventory.AddFungibleCount(weHaveApplibeGoodyName, -1);
        newOp->set_appliedgoodykeyname(weHaveApplibeGoodyName);
    }
    
    /** We need to make it at least 1 block, else if will make no sense executing immediately logic flow wise*/
    if(duration < 1)
    {
        duration = 1;
    }    

    LOG (WARNING) << "Settign duration as " << duration;
    newOp->set_blocksleft(duration);

    a->CalculatePrestige(ctx.RoConfig());
    a.reset();
    LOG (INFO) << "Cooking instance " << recepie << " submitted succesfully ";
  }  

  void
  MoveProcessor::MaybeInitXayaPlayer (const std::string& name,
                                   const Json::Value& init)
  {
    if (!init.isObject ())
      return;

    auto a = xayaplayers.GetByName (name, ctx.RoConfig ());
    CHECK (a != nullptr);
	
    const auto& address = init["address"];
    if (!address.isString ())
    {
        LOG (WARNING)
            << "Account initialisation does not specify address: " << address;
        a.reset();    
        return;
    }
	
	if(address == "")
	{
		a.reset();
		return;
	}
	
	// Lets allocate early access promised prizes
	
	if(name == "DonRinkula" || name == "PandaBoss" || name == "1 John")
	{
		auto r0 = recipeTbl.CreateNew(name, "b5850520-cb42-d804-d887-38e4577f62a6", ctx.RoConfig());
		r0.reset();
	}
	
    a->MutableProto().set_address(address.asString());
    a.reset();
    LOG (INFO)
        << "Initialised account " << name << " to address " << address;
  }
  
  void
  MoveProcessor::TryCookingAction (const std::string& name,
                                   const Json::Value& upd)
  {
    if (!upd.isObject ())
      return;

    /*Trying to cook recepie, optionally with the fighter */
	
	const auto& ref = upd["r"];
    if (!ref.isNull ())
	{
		MaybeCookRecepie (name, upd["r"]);
	}
    
    /*Trying to destroy recepie*/
	const auto& ref2 = upd["d"];
    if (!ref2.isNull ())
	{	
		MaybeDestroyRecepie (name, upd["d"]); 
	}	
    
    /*Trying to collect cooked recepie*/
	const auto& ref3 = upd["cl"];
    if (!ref3.isNull ())
	{
		MaybeCollectCookedRecepie (name, upd["cl"]); 
	}	
    
    /*Trying to cook sweetener */
	const auto& ref4 = upd["s"];
    if (!ref4.isNull ())
	{
		MaybeCookSweetener (name, upd["s"]);  
	}

    /*Trying to claim sweetener rewards */
	const auto& ref5 = upd["sc"];
    if (!ref5.isNull ())
	{
		MaybeClaimSweetenerReward (name, upd["sc"]);   
	}	
  } 
  
  void
  MoveProcessor::TryExpeditionAction (const std::string& name,
                                   const Json::Value& upd)
  {
    if (!upd.isObject ())
      return;

    /*Trying to send own fighter to expedition*/
	const auto& ref = upd["f"];
    if (!ref.isNull ())
	{	
		MaybeGoForExpedition (name, upd["f"]);
	}
    
    /*Trying to claim reward from the finished expedition*/
	const auto& ref2 = upd["c"];
    if (!ref2.isNull ())
	{	
		MaybeClaimReward (name, upd["c"]);    
	}
 
  }  
  
  void
  MoveProcessor::TrySpecialTournamentAction (const std::string& name, const Json::Value& upd, const Context& ctx)
  {
    if (!upd.isObject ())
      return;
    
    /*Trying to enter the special tournament*/
	const auto& ref = upd["e"];
    if (!ref.isNull ())
	{	
		MaybeEnterSpecialTournament (name, upd["e"]); 
	}
    
    /*Trying to leave the tournament*/
	const auto& ref2 = upd["l"];
    if (!ref2.isNull ())
	{
		MaybeLeaveSpecialTournament (name, upd["l"]);      
	}	
  }      

  void
  MoveProcessor::TryTournamentAction (const std::string& name,
                                   const Json::Value& upd)
  {
    if (!upd.isObject ())
      return;
    
    /*Trying to enter the tournament*/
	const auto& ref = upd["e"];
    if (!ref.isNull ())
	{	
		MaybeEnterTournament (name, upd["e"]); 
	}
    
    /*Trying to leave the tournament*/
	const auto& ref2 = upd["l"];
    if (!ref2.isNull ())
	{	
		MaybeLeaveTournament (name, upd["l"]);     
	}

    /*Trying to claim reward from the finished tournament*/
	const auto& ref3 = upd["c"];
    if (!ref3.isNull ())
	{	
		MaybeClaimTournamentReward (name, upd["c"]);  
	}    
  }     
  
  void
  MoveProcessor::TryFighterAction (const std::string& name,
                                   const Json::Value& upd)
  {
    if (!upd.isObject ())
      return;
    
    /*Trying to deconstruct the fighter*/
	const auto& ref = upd["d"];
    if (!ref.isNull ())
	{	
		MaybeDeconstructFighter (name, upd["d"]); 
	}
    
    /*Trying to claim reward after deconstruction is done*/
	const auto& ref2 = upd["c"];
    if (!ref2.isNull ())
	{	
		MaybeClaimDeconstructionReward (name, upd["c"]);  
	}

    /*Trying to auction the fighter*/
	const auto& ref3 = upd["s"];
    if (!ref3.isNull ())
	{	
		MaybePutFighterForSale(name, upd["s"]); 
	}

    /*Trying to buy the fighter*/
	const auto& ref4 = upd["b"];
    if (!ref4.isNull ())
	{
		MaybeBuyFighterFromExchange(name, upd["b"]);  
	}

    /*Trying to remove the listed fighter*/
	const auto& ref5 = upd["r"];
    if (!ref5.isNull ())
	{	
		MaybeRemoveFighterFromExchange(name, upd["r"]);     
	}

    /*Maybe transfigure fighter*/
	const auto& ref6 = upd["t"];
    if (!ref6.isNull ())
	{	
		MaybeTransfigureFighter (name, upd["t"]); 	
	}
      
  }    

  void
  MoveProcessor::TryXayaPlayerUpdate (const std::string& name,
                                   const Json::Value& upd)
  {
    if (!upd.isObject ())
      return;

	const auto& ref = upd["init"];
    if (!ref.isNull ())
	{
		MaybeInitXayaPlayer (name, upd["init"]);
	}
  }    


/* ************************************************************************** */

} // namespace pxd

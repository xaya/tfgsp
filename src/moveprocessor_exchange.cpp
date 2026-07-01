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

/* Fighter exchange: list-for-sale, remove, buy.
   Split out of moveprocessor.cpp; part of pxd::MoveProcessor. */

namespace pxd
{

  bool BaseMoveProcessor::ParseFighterForSaleData(const XayaPlayer& a, const Json::Value& fighter, uint32_t& fighterID, uint32_t& duration, Amount& price, Amount& listingfee)
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
    
    if(fighterDb->GetProto().isaccountbound() == true)
    {
      LOG (WARNING) << "Can not sale account bound fighter: " << a.GetName();
      fighterDb.reset();
      return false;               
    }      
    
    if((pxd::FighterStatus)(int32_t)fighterDb->GetStatus() != pxd::FighterStatus::Available) 
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

  bool BaseMoveProcessor::ParseRemoveBuyData(const XayaPlayer& a, const Json::Value& fighter, uint32_t& fighterID)
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
      LOG (WARNING) << "Fatal error, could not get fighter with ID" << fighterID;
      return false;                
    }
    
    if(fighterDb->GetOwner() != a.GetName())
    {
      LOG (WARNING) << "Fighter is not owned by: " << a.GetName();
      fighterDb.reset();
      return false;               
    }    
    
    if((pxd::FighterStatus)(int32_t)fighterDb->GetStatus() != pxd::FighterStatus::Exchange)
    {
      LOG (WARNING) << "Fighter status is not on exchange: " << fighterID;
      fighterDb.reset();
      return false;              
    }    
    
    fighterDb.reset(); 
    return true;    
  }        

  bool BaseMoveProcessor::ParseBuyData(const XayaPlayer& a, const Json::Value& fighter, uint32_t& fighterID, Amount& exchangeprice)
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
      LOG (WARNING) << "Fatal error, could not get fighter with ID" << fighterID;
      return false;                
    }
    
    exchangeprice = fighterDb->GetProto().exchangeprice();
    
    if(fighterDb->GetOwner() == a.GetName())
    {
      LOG (WARNING) << "Can noy buy own fighter, its nonsense: " << a.GetName();
      fighterDb.reset();
      return false;               
    }    
    
    if((pxd::FighterStatus)(int32_t)fighterDb->GetStatus() != pxd::FighterStatus::Exchange)
    {
      LOG (WARNING) << "Fighter status is not on exchange: " << fighterID << " but is " << (int32_t)fighterDb->GetStatus();
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
    
    {
      auto ores = ongoings.QueryForOwner(a.GetName());
      while(ores.Step())
      {
        auto op = ongoings.GetFromResult(ores);
        if(op->GetProto().type() == (uint32_t)pxd::OngoingType::COOK_RECIPE)
        {
            slots_already_cooking++;
        }
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
    
    if(!ParseFighterForSaleData(*a, fighter, fighterID, duration, price, listingfee))      
    {
      a.reset();
      return;
    }

    auto fighterDb = fighters.GetById (fighterID, ctx.RoConfig ());
    fighterDb->SetStatus(pxd::FighterStatus::Exchange);
    
    a->AddBalance(-listingfee);
    
    int32_t secondsInOneDay = 24 * 60 * 60;
    int32_t blocksInOneDay = secondsInOneDay / 30;
    fighterDb->MutableProto().set_exchangeexpire(ctx.Height () + (duration * blocksInOneDay));
    fighterDb->MutableProto().set_exchangeprice(price);
    
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
    if(!ParseRemoveBuyData(*a, fighter, fighterID))      
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
    if(!ParseBuyData(*a, fighter, fighterID, exchangeprice))      
    {
      a.reset();
      return;
    }

    auto fighterDb = fighters.GetById (fighterID, ctx.RoConfig ());

    /* Fetch and null-check the seller BEFORE mutating any state: ParseBuyData
       guarantees the listing (and therefore its owner account) exists, so this
       guard is dead on every reachable input, but it keeps the money-credit
       path symmetric with the buyer check above rather than dereferencing a
       possibly-null handle. */
    auto a2 = xayaplayers.GetByName (fighterDb->GetOwner(), ctx.RoConfig ());
    if (a2 == nullptr)
    {
      LOG (ERROR) << "Exchange seller " << fighterDb->GetOwner ()
                  << " not found; aborting buy";
      fighterDb.reset ();
      a.reset ();
      return;
    }

    fighterDb->SetStatus(pxd::FighterStatus::Available);

    a->AddBalance (-exchangeprice);

    
    /* EXCH-1 fix: the seller payout used to be computed with fpm::fixed_24_8,
       whose int32 backing store overflows for any exchangeprice >= 8'388'608
       (the raw value is price*256).  That produced a negative payout which trips
       AddBalance's CHECK_GE(balance, 0) and aborts EVERY node deterministically
       at the same block -> permanent chain halt.  Compute the payout in 64-bit
       integer math instead.  exchange_sale_percentage is stored directly as the
       fixed_24_8 raw value of the seller fraction (e.g. 246 == 246/256 ==
       0.9609375), so finalPrice = price*246/256 (price 500 -> 480, etc.). */
    const int64_t pctRaw = ctx.RoConfig ()->params ().exchange_sale_percentage ();
    /* Value-conservation invariant: the seller must never receive more than the
       buyer paid.  raw 256 == 1.0; a misconfigured percentage > 1.0 would mint
       crystals on every trade.  Halt deterministically (identical on all nodes,
       no fork) rather than mint. */
    CHECK_LE (pctRaw, 256) << "exchange_sale_percentage > 1.0 would mint crystals";
    Amount finalPrice = (exchangeprice * pctRaw) / 256;

    if (finalPrice == exchangeprice)
      finalPrice = exchangeprice - 1;

    a2->AddBalance (finalPrice);
	
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
	
  }   

} // namespace pxd

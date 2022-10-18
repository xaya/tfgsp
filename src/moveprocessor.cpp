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

#include <fpm/fixed.hpp>
#include <fpm/math.hpp>

#include "burnsale.hpp"

#include "forks.hpp"
#include "jsonutils.hpp"
#include "proto/config.pb.h"

#include <xayautil/jsonutils.hpp>

#include <xayagame/sqlitestorage.hpp>

#include <sstream>

namespace pxd
{


/*************************************************************************** */

BaseMoveProcessor::BaseMoveProcessor (Database& d,
                                      const Context& c)
  : ctx(c), db(d), xayaplayers(db), recipeTbl(db), fighters(db), moneySupply(db), rewards(db), tournamentsTbl(db), specialTournamentsTbl(db)
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
  if (!mv.isObject ())
    {
      LOG (WARNING) << "Move is not an object: " << mv;
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
    
    if(xAddress == "")
    {
      LOG (WARNING) << "Failed to get valid address for player " << xName;
      return false;        
    }
    
    player.reset();
    
    paidToCrownHolders.insert(std::pair<std::string, Amount>(xAddress,0));
    holderTier.insert(std::pair<std::string, int32_t>(xAddress,(int32_t)spctrm->GetProto().tier()));
    
    if((int32_t)spctrm->GetProto().tier() == 1)
    {
        tier1holderName = xAddress;
    }
        
    spctrm.reset();
    tryAndStep = res.Step ();
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
        if(xaya::ChiAmountFromJson(outVal[outValue.first], amnt) == false)
        {
          LOG (WARNING) << "Failed to extract amount from " << outVal[outValue.first] << " for name " << outValue.first << " outValDumpBeing " << outVal ;
          continue;         
        }
        
        smallestPayFraction = amnt;
      }
    }
  }

  Amount totalAmountPaid = 0;
  
  for(auto& outValue: outVal)
  {
      Amount amnt;
      if(xaya::ChiAmountFromJson(outValue, amnt) == false)
      {
        LOG (WARNING) << "Failed to extract amount from " << outValue;
        continue;                     
      }          
      totalAmountPaid += amnt;
  }
  
  Amount leftOverDueToPrecisionError = totalAmountPaid - smallestPayFraction * 28;
    
  if(outVal.isObject())
  {
    for(auto& outValue: paidToCrownHolders)
    {
        int32_t myTier = holderTier[outValue.first];
        
        Amount amnt;
        if(xaya::ChiAmountFromJson(outVal[outValue.first], amnt) == false)
        {
          LOG (WARNING) << "Failed to extract amount from " << outVal[outValue.first];
          continue;                    
        }

        if(amnt != myTier * smallestPayFraction)
        {
            if(myTier == 7 && amnt == myTier * smallestPayFraction + leftOverDueToPrecisionError)
            {
            }
            else
            {
              LOG (WARNING) << "Invalid fraction paid for " << outValue.first << " he/she has tier " << myTier << " but payments was" << amnt;
              return false;
            }
        }      
        
        paidToCrownHolders[outValue.first] = amnt;
    }
  }
  
  if (moveObj.isMember ("burnt"))
    CHECK (xaya::ChiAmountFromJson (moveObj["burnt"], burnt));
  else
    burnt = 0;

  return true;
}

bool
BaseMoveProcessor::ParseSweetenerPurchase(const Json::Value& mv, Amount& cost, const std::string& name, std::string& fungibleName, Amount balance)
{
  const auto& cmd = mv["ps"];
  
  if(!cmd.isString())
  {
      return false;
  }
  
  bool exists = false;
  const auto& sweetenerList = ctx.RoConfig()->sweetenerblueprints();
  
  for(const auto& swtnr: sweetenerList)
  {
      if(swtnr.second.authoredid() == cmd.asString())
      {
          cost = swtnr.second.price();
          fungibleName = swtnr.first;
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
  
  VLOG (1) << "Trying to purchace bundle, amount paid left: " << paidToDev;
  cost = chiPRICE * COIN;
  
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

  LOG (WARNING) << "Attempting to purchase sweetener through move: " << cmd;
  
  auto player = xayaplayers.GetByName (name, ctx.RoConfig());
  CHECK (player != nullptr);

  if (cmd.isObject ())
  {
      LOG (WARNING) << "Purchase sweetener is object: " << cmd;
      return;
  } 
  
  Amount cost = 0;
  std::string fungibleName = "";
  if(!ParseSweetenerPurchase(mv, cost, name, fungibleName, player->GetBalance()))
  {
      return;
  }
   LOG (WARNING) << "GOOD: " << cmd << " as cost " << cost;
  player->AddBalance (-cost); 
  player->GetInventory().AddFungibleCount(fungibleName, 1);
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
  
  Amount fraction = paidToDev / 28;

  std::map<std::string, int32_t> holderTier;
  
  auto res = specialTournamentsTbl.QueryAll();
  bool tryAndStep = res.Step();
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
    
    // Its important to test that address is valid, else we must provide working substitute,
    // or someone might use this just to hard other crown holders
    
    //todo
    
    if(xAddress == "")
    {
      LOG (ERROR) << "Failed to get valid address for player, we must inject XAYA default to keep this consitant for other players " << xName;
      xAddress = "CWXvFB9MuGVxCXohBaAStPZmJprqKL7kMm";
      //player.reset();   
      //return;        
    }
    
    pPlayer.reset();    
    
    holderTier.insert(std::pair<std::string, int32_t>(xAddress,(int32_t)spctrm->GetProto().tier()));
    spctrm.reset();
    tryAndStep = res.Step ();
  }    
  
  for(auto& entry: paidToCrownHolders)
  {
      if(holderTier[entry.first] == 7)
      {
        Amount fraction = cost / 28;
        Amount leftover = cost - fraction * 28;
        paidToCrownHolders[entry.first] -= leftover;
      }
      
      paidToCrownHolders[entry.first] -= fraction * holderTier[entry.first];
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

bool
BaseMoveProcessor::ParseCoinTransferBurn (const XayaPlayer& a,
                                          const Json::Value& moveObj,
                                          CoinTransferBurn& op,
                                          Amount& burntChi)
{
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
  CHECK (moveArray.isArray ());
  LOG (INFO) << "Processing " << moveArray.size () << " moves...";

  for (const auto& m : moveArray)
    ProcessOne (m);
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
    return;

  /* Ensure that the account database entry exists.  In other words, we
     have accounts (although perhaps uninitialised) for everyone who
     ever sent a Taurion move.  */
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
  TryCoinOperation (name, mv, burnt);
  
  /* Handle crystal purchace now */
  TryCrystalPurchase (name, mv, paidToCrownHolders);
  
  /* Handle sweetener purchace now */
  TrySweetenerPurchase (name, mv);  
  
  /* Handle goody purchace now */
  TryGoodyPurchase (name, mv);   
  
  /* Handle goody bundle purchace now */
  TryGoodyBundlePurchase (name, mv);    
  
  xaya::Chain chain = ctx.Chain();
  if(chain == xaya::Chain::REGTEST)
  {
  /*Running special unittest from python*/
    MaybeSQLTestInjection(name, mv);  
    MaybeSQLTestInjection2(name, mv);    
  }  

  /* At this point, we terminate if the game-play itself has not started.
     This is more or less when the "game world is created"*/
  if (!ctx.Forks ().IsActive (Fork::GameStart))
    return;

  /* We perform account updates first.  That ensures that it is possible to
     e.g. choose one's faction and create characters in a single move.  */
  TryXayaPlayerUpdate (name, mv["a"]);

  /* We are trying all kind of cooking actions*/
  TryCookingAction (name, mv["ca"]);
  
  /* We are trying all kind of expedition actions*/
  TryExpeditionAction (name, mv["exp"]);  
  
  /* We are trying all kind of tournament related actions*/
  TryTournamentAction (name, mv["tm"]);  

  /* We are trying all kind of special tournament related actions*/
  TrySpecialTournamentAction (name, mv["tms"], ctx);     

  /* We are trying all kind of fighter related actions*/
  TryFighterAction (name, mv["f"]);     
      
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
    
    if(price <= 0)
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
      LOG (WARNING) << "Fighter status is not on exchange: " << fighterID;
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
    
    if(tournamentTier != playerPrestigeBasedTier)
    {
      LOG (WARNING) << "Wrong tournament tier, expected  " << tournamentTier << " but actual player is " << playerPrestigeBasedTier;
      tournamentData.reset();
      return false;        
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
      LOG (WARNING) << "Tournament roster is already full for id: " << tournamentID;
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
      
      fighterDD.reset();
    
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
    if(ctx.Height () > 4265751) // HARD FORK INITIATING
    {
      a->AddBalance (-exchangeprice);
      
      fpm::fixed_24_8 reductionPercent = fpm::fixed_24_8(ctx.RoConfig()->params().exchange_sale_percentage());
      fpm::fixed_24_8 priceToPay = fpm::fixed_24_8(fighterDb->GetProto().exchangeprice());
      fpm::fixed_24_8 finalPrice = priceToPay * reductionPercent;    
      
      a2->AddBalance((int32_t)finalPrice);
    }
    else
    {
      int feeMultipler = 100 - ctx.RoConfig()->params().exchange_sale_percentage();
      
      a->AddBalance (-fighterDb->GetProto().exchangeprice());
      a2->AddBalance(fighterDb->GetProto().exchangeprice() * feeMultipler);        
    }

    fighterDb->SetOwner(name);
    fighterDb.reset();
     
    a->CalculatePrestige(ctx.RoConfig());
    a2->CalculatePrestige(ctx.RoConfig());
  
    a.reset();  
    a2.reset();
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
    if (a->GetProto().address() != "")
    {
      LOG (WARNING) << "Account " << name << " address is already initialised";
      a.reset();
      return;
    }

    const auto& address = init["address"];
    if (!address.isString ())
    {
        LOG (WARNING)
            << "Account initialisation does not specify address: " << address;
        a.reset();    
        return;
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
    MaybeCookRecepie (name, upd["r"]);
    
    if(ctx.Height () > 4265751) // HARD FORK INITIATING
    {
      /*Trying to destroy recepie*/
      MaybeDestroyRecepie (name, upd["d"]);    
    }
    
    /*Trying to collect cooked recepie*/
    MaybeCollectCookedRecepie (name, upd["cl"]);    
    
    /*Trying to cook sweetener */
    MaybeCookSweetener (name, upd["s"]);  

    /*Trying to claim sweetener rewards */
    MaybeClaimSweetenerReward (name, upd["sc"]);      
  } 
  
  void
  MoveProcessor::TryExpeditionAction (const std::string& name,
                                   const Json::Value& upd)
  {
    if (!upd.isObject ())
      return;

    /*Trying to send own fighter to expedition*/
    MaybeGoForExpedition (name, upd["f"]);
    
    /*Trying to claim reward from the finished expedition*/
    MaybeClaimReward (name, upd["c"]);    
 
  }  
  
  void
  MoveProcessor::TrySpecialTournamentAction (const std::string& name, const Json::Value& upd, const Context& ctx)
  {
    if (!upd.isObject ())
      return;
    
    /*Trying to enter the special tournament*/
    MaybeEnterSpecialTournament (name, upd["e"]); 
    
    /*Trying to leave the tournament*/
    MaybeLeaveSpecialTournament (name, upd["l"]);        
  }      

  void
  MoveProcessor::TryTournamentAction (const std::string& name,
                                   const Json::Value& upd)
  {
    if (!upd.isObject ())
      return;
    
    /*Trying to enter the tournament*/
    MaybeEnterTournament (name, upd["e"]); 
    
    /*Trying to leave the tournament*/
    MaybeLeaveTournament (name, upd["l"]);     

    /*Trying to claim reward from the finished tournament*/
    MaybeClaimTournamentReward (name, upd["c"]);      
  }     
  
  void
  MoveProcessor::TryFighterAction (const std::string& name,
                                   const Json::Value& upd)
  {
    if (!upd.isObject ())
      return;
    
    /*Trying to deconstruct the fighter*/
    MaybeDeconstructFighter (name, upd["d"]); 
    
    /*Trying to claim reward after deconstruction is done*/
    MaybeClaimDeconstructionReward (name, upd["c"]);  

    /*Trying to auction the fighter*/
    MaybePutFighterForSale(name, upd["s"]); 

    /*Trying to buy the fighter*/
    MaybeBuyFighterFromExchange(name, upd["b"]);  

    /*Trying to remove the listed fighter*/
    MaybeRemoveFighterFromExchange(name, upd["r"]);        
      
  }    

  void
  MoveProcessor::TryXayaPlayerUpdate (const std::string& name,
                                   const Json::Value& upd)
  {
    if (!upd.isObject ())
      return;

    MaybeInitXayaPlayer (name, upd["init"]);
  }    


/* ************************************************************************** */

} // namespace pxd

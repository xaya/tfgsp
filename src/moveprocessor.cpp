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

#include <sstream>

namespace pxd
{


/*************************************************************************** */

BaseMoveProcessor::BaseMoveProcessor (Database& d,
                                      const Context& c)
  : ctx(c), db(d), xayaplayers(db), moneySupply(db), fighters(db)
{}

bool
BaseMoveProcessor::ExtractMoveBasics (const Json::Value& moveObj,
                                      std::string& name, Json::Value& mv,
                                      Amount& paidToDev,
                                      Amount& burnt) const
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

  paidToDev = 0;
  const auto& outVal = moveObj["out"];
  const auto& devAddr = ctx.RoConfig()->params ().dev_addr ();
  if (outVal.isObject () && outVal.isMember (devAddr))
    CHECK (xaya::ChiAmountFromJson (outVal[devAddr], paidToDev));

  if (moveObj.isMember ("burnt"))
    CHECK (xaya::ChiAmountFromJson (moveObj["burnt"], burnt));
  else
    burnt = 0;

  return true;
}

bool
BaseMoveProcessor::ParseCrystalPurchase(const Json::Value& mv, std::string& bundleKeyCode, Amount& cost, Amount& crystalAmount, const std::string& name, const Amount& paidToDev)
{
  const auto& cmd = mv["pc"];
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
BaseMoveProcessor::TryCrystalPurchase (const std::string& name,
                                         const Json::Value& mv,
                                         Amount& paidToDev)
{
  const auto& cmd = mv["pc"];
  if (!cmd.isString ()) return;

  VLOG (1) << "Attempting to purchase crystal bundle through move: " << cmd;

  const auto player = xayaplayers.GetByName (name, ctx.RoConfig());
  CHECK (player != nullptr && player->IsInitialised ());

  if (cmd.isObject ())
  {
      LOG (WARNING)
          << "Purchase crystal bundle is object: " << cmd;
      return;
  }
  
  Amount cost = 0;
  Amount crystalAmount  = 0;
  std::string bundleKeyCode = "";
  if(!ParseCrystalPurchase(mv, bundleKeyCode, cost, crystalAmount, name, paidToDev)) return;


  paidToDev -= cost; 
  player->AddBalance (crystalAmount); 
  
  VLOG (1) << "After purchacing bundle, paid to dev left: " << paidToDev;

  return;
}

  std::string BaseMoveProcessor::GetRecepieKeyNameFromID(const std::string& authID, const Context& ctx)
  {
      
    const auto& recepiesList = ctx.RoConfig()->recepies();
    
    for(const auto& recepieX: recepiesList)
    {
        if(recepieX.second.authoredid() == authID)
        {
            return recepieX.first;
            break;
        }
    }      
      
    return "";
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
        to = xayaplayers.CreateNew (entry.first, ctx.RoConfig ());
      }
    to->AddBalance (entry.second);
  }
}

void
MoveProcessor::ProcessOne (const Json::Value& moveObj)
{
  std::string name;
  Json::Value mv;
  Amount paidToDev, burnt;
  if (!ExtractMoveBasics (moveObj, name, mv, paidToDev, burnt))
    return;

  /* Ensure that the account database entry exists.  In other words, we
     have accounts (although perhaps uninitialised) for everyone who
     ever sent a Taurion move.  */
  if (xayaplayers.GetByName (name, ctx.RoConfig ()) == nullptr)
    {
      LOG (INFO) << "Creating uninitialised account for " << name;
      xayaplayers.CreateNew (name, ctx.RoConfig ());
    }

  /* Handle coin transfers before other game operations.  They are even
     valid without a properly initialised account (so that vCHI works as
     a real cryptocurrency, not necessarily tied to the game).
     This also ensures that if funds run out, then the explicit transfers
     are done with priority over the other operations that may require coins
     implicitly.  */
  TryCoinOperation (name, mv, burnt);

  /* Handle crystal purchace now */
  TryCrystalPurchase (name, mv, paidToDev);

  /* At this point, we terminate if the game-play itself has not started.
     This is more or less when the "game world is created"*/
  if (!ctx.Forks ().IsActive (Fork::GameStart))
    return;

  /* We perform account updates first.  That ensures that it is possible to
     e.g. choose one's faction and create characters in a single move.  */
  TryXayaPlayerUpdate (name, mv["a"]);

  /* We are trying to update tutorial tracking variable*/
  TryTFTutorialUpdate (name, mv["tu"]);

  /* We are trying all kind of cooking actions*/
  TryCookingAction (name, mv["ca"]);
  
  /* If there is no account (after potentially updating/initialising it),
     then let's not try to process any more updates.  This explicitly
     enforces that accounts have to be initialised before doing anything
     else, even if perhaps some changes wouldn't actually require access
     to an account in their processing.  */
  if (!xayaplayers.GetByName (name, ctx.RoConfig ())->IsInitialised ())
    {
      VLOG (1)
          << "Account " << name << " is not yet initialised,"
          << " ignoring parts of the move " << moveObj;
      return;
    }
    
  /* If any burnt or paid-to-dev coins are left, it means probably something
     has gone wrong and the user overpaid due to a frontend bug.  */
  LOG_IF (WARNING, paidToDev > 0 || burnt > 0)
      << "At the end of the move, " << name
      << " has " << paidToDev << " paid-to-dev and "
      << burnt << " burnt CHI satoshi left";
}

  void
  MoveProcessor::MaybeUpdateFTUEState (const std::string& name,
                                   const Json::Value& init)
  {
    if (!init.isObject ())
      return;

    auto a = xayaplayers.GetByName (name, ctx.RoConfig ());
    CHECK (a != nullptr);

    const auto& ftuestateVal = init["ftuestate"];
    if (!ftuestateVal.isString ())
      {
        LOG (WARNING)
            << "Name update does not specify ftuestate: " << init;
        return;
      }
      
      
    const FTUEState ftuestate = FTUEStateFromString (ftuestateVal.asString ());
    
    switch (ftuestate)
    {
      case FTUEState::Invalid:
        LOG (WARNING) << "Invalid ftuestate specified for account: " << init;
        return;

      default:
        break;
    }    
    
    const FTUEState ftuestateCurrent = a->GetFTUEState ();
    
    int s1val = static_cast<int>(ftuestate);
    int s2val = static_cast<int>(ftuestateCurrent);
    
    if(s1val <= s2val)
    {
        LOG (WARNING) << "Ftuestate name update can only update forward: " << init;
        return;        
    }
    
    if (init.size () != 1)
    {
        LOG (WARNING) << "Ftuestate name update has extra fields: " << init;
        return;
    }

    a->SetFTUEState (ftuestate);
    LOG (INFO)
        << "Setting account " << name << " to ftuestate "
        << FTUEStateToString (ftuestate);
        
    //Lets also initialize account at this stage if its no yet. We do not need this at all,
    //but maybe will become handy in the future. Can always remove later on, as its harmless
    //and is triggered only during tutorial flag checks
    
    if (a->IsInitialised () == false)
    {
        a->SetRole(PlayerRole::PLAYER);
    }
  }
  
  bool
  BaseMoveProcessor::ParseCookRecepie(const XayaPlayer& a, const std::string& name, const Json::Value& recepie, std::map<std::string, pxd::Quantity>& fungibleItemAmountForDeduction, int32_t& cookCost, int32_t& duration)
  {
    if (!recepie.isObject ())
    return false;

    const std::string recepieID = recepie["rid"].asString ();
    const std::string fighterID = recepie["fid"].asString ();
    
    auto& playerInventory = a.GetInventory();
    
    std::string itemInventoryName = GetRecepieKeyNameFromID(recepieID, ctx);
    const auto& recepiesList = ctx.RoConfig()->recepies();
    

    if(itemInventoryName == "")
    {
        LOG (WARNING) << "Could not resolve key name from the authid for the item: " << recepieID;
        return false;       
    }    
    
    if(InventoryHasItem(itemInventoryName, playerInventory, 1) == false)
    {
        LOG (WARNING) << "Player inventory does not have recepie instance named: " << itemInventoryName;
        return false;       
    }
    
    for(const auto& recepie: recepiesList)
    {
        if(recepie.second.authoredid() == recepieID)
        {
            if(recepie.second.quality() == 0)
            {
                cookCost = ctx.RoConfig()->params().common_recipe_cook_cost();
            }
            
            if(recepie.second.quality() == 1)
            {
                cookCost = ctx.RoConfig()->params().uncommon_recipe_cook_cost();
            }

            if(recepie.second.quality() == 2)
            {
                cookCost = ctx.RoConfig()->params().rare_recipe_cook_cost();
            }

            if(recepie.second.quality() == 3)
            {
                cookCost = ctx.RoConfig()->params().epic_recipe_cook_cost();
            }  

            duration = recepie.second.duration();
        }
    }
    
    if(cookCost == -1)
    {
        LOG (WARNING) << "Cooking cost could not be resolved for the instance named: " << itemInventoryName;
        return false;          
    }
    
    if(a.GetBalance() < cookCost)
    {
        LOG (WARNING) << "No enough crystals on balance to cook: " << itemInventoryName;
        return false;              
    }
    
    bool playerHasEnoughIngridients = true;
    bool testWasPerformed = false;
    
    for(const auto& recepie: recepiesList)
    {
        if(recepie.second.authoredid() == recepieID)
        {
            testWasPerformed = true;
            
            for(const auto& candyNeeds: recepie.second.requiredcandy())
            {
                std::string candyInventoryName = GetCandyKeyNameFromID(candyNeeds.candytype(), ctx);
                
                if(InventoryHasItem(candyInventoryName, playerInventory, candyNeeds.amount()) == false)
                {
                    playerHasEnoughIngridients = false;
                }
                
                pxd::Quantity quantity = candyNeeds.amount();
                
                fungibleItemAmountForDeduction.insert(std::pair<std::string, pxd::Quantity>(candyInventoryName, quantity));
            }
        }
    }
    
    if(testWasPerformed == false)
    {
        LOG (WARNING) << "Failed to test for candy amounts for " << itemInventoryName;
        return false;          
    }
    
    if(playerHasEnoughIngridients == false)
    {
        LOG (WARNING) << "Not enough candy ingridients to cook " << itemInventoryName;
        return false;         
    }
        
    uint32_t slots = 0; //TODO, get from total player owning fighters   
  
    if(slots > ctx.RoConfig()->params().max_fighter_inventory_amount())
    {
        LOG (WARNING) << "Not enough slots to host a new fighter for " << itemInventoryName;
        return false;         
    }

    return true;    
  }
  
  void
  MoveProcessor::MaybeCookRecepie (const std::string& name, const Json::Value& recepie)
  {      
    std::map<std::string, pxd::Quantity> fungibleItemAmountForDeduction;
    int32_t cookCost = -1;
    int32_t duration = -1;
       
    auto a = xayaplayers.GetByName (name, ctx.RoConfig ());
    
    if(!ParseCookRecepie(*a, name, recepie, fungibleItemAmountForDeduction, cookCost, duration)) return;
    
    auto& playerInventory = a->GetInventory();
    for(const auto& itemToDeduct: fungibleItemAmountForDeduction)
    {
        playerInventory.AddFungibleCount(itemToDeduct.first, -itemToDeduct.second);
    }
    
    playerInventory.AddFungibleCount(GetRecepieKeyNameFromID(recepie["rid"].asString (), ctx), -1);
    
    a->AddBalance(-cookCost);
    
    //TODO REMOVE FIGHTER
    //TODO USE APPLICABLE GOODIE - check maybe ok NOW>?

    proto::OngoinOperation* newOp = a->MutableProto().add_ongoings();
    newOp->set_blocksleft(duration);
    newOp->set_type((uint32_t)pxd::OngoingType::COOK_RECIPE);
    newOp->set_itemauthid(recepie["rid"].asString ());

    a->CalculatePrestige(ctx.RoConfig());
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
    if (a->IsInitialised ())
      {
        LOG (WARNING) << "Account " << name << " is already initialised";
        return;
      }

    const auto& roleVal = init["role"];
    if (!roleVal.isString ())
      {
        LOG (WARNING)
            << "Account initialisation does not specify role: " << init;
        return;
      }
    const PlayerRole role = PlayerRoleFromString (roleVal.asString ());
    switch (role)
      {
      case PlayerRole::INVALID:
        LOG (WARNING) << "Invalid role specified for account: " << init;
        return;

      default:
        break;
      }

    if (init.size () != 1)
      {
        LOG (WARNING) << "Account initialisation has extra fields: " << init;
        return;
      }

    a->SetRole (role);
    LOG (INFO)
        << "Initialised account " << name << " to role "
        << PlayerRoleToString (role);
  }
  
  void
  MoveProcessor::TryTFTutorialUpdate (const std::string& name,
                                   const Json::Value& upd)
  {
    if (!upd.isObject ())
      return;

    MaybeUpdateFTUEState (name, upd["t"]);
  }   

  void
  MoveProcessor::TryCookingAction (const std::string& name,
                                   const Json::Value& upd)
  {
    if (!upd.isObject ())
      return;

    /*Trying to cook recepie, optionally with the fighter */
    MaybeCookRecepie (name, upd["r"]);
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

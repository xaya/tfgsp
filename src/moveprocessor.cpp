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
  : ctx(c), db(d), xayaplayers(db), recipeTbl(db), fighters(db), moneySupply(db), rewards(db), tournamentsTbl(db)
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
  
  /* We are trying all kind of expedition actions*/
  TryExpeditionAction (name, mv["exp"]);  
  
  /* We are trying all kind of tournament related actions*/
  TryTournamentAction (name, mv["tm"]);    
  
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
  
  bool BaseMoveProcessor::ParseRewardData(const XayaPlayer& a, const std::string& name, const Json::Value& expedition, std::vector<uint32_t>& rewardDatabaseIds)
  {
    if (!expedition.isObject ())
    return false;

    if(!expedition["eid"].isString())
    {
        return false;
    }
    
    const std::string expeditionID = expedition["eid"].asString ();
    
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
        tryAndStep = res.Step ();
      }
    }
   
    if(totalEntries <= 0)
    {
        LOG (WARNING) << "Could not find any relevan rewards for expedition: " << expeditionID;
        return false;              
    }   
    
    return true;
  }      
  
  bool BaseMoveProcessor::ParseTournamentEntryData(const XayaPlayer& a, const std::string& name, const Json::Value& tournament, uint32_t& tournamentID, std::vector<uint32_t>& fighterIDS)
  {
    if (!tournament.isObject ())
    {
      return false;
    }

    if(!tournament["tid"].isInt())
    {
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
    
    if(tournamentData->GetProto().authoredid() == "cbd2e78a-37ce-b864-793d-8dd27788a774" && (int)a.GetFTUEState() > (int)pxd::FTUEState::JoinFirstTournament)
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
  
  bool BaseMoveProcessor::ParseExpeditionData(const XayaPlayer& a, const std::string& name, const Json::Value& expedition, pxd::proto::ExpeditionBlueprint& expeditionBlueprint, FighterTable::Handle& fighter, int32_t& duration, std::string& weHaveApplibeGoodyName)
  {
    if (!expedition.isObject ())
    return false;

    if(!expedition["eid"].isString())
    {
        return false;
    }
    
    if(!expedition["fid"].isInt())
    {
        return false;
    }    

    const std::string expeditionID = expedition["eid"].asString ();
    const int fighterID = expedition["fid"].asInt();
    
    fighter = fighters.GetById (fighterID, ctx.RoConfig ());
    
    if(fighter == nullptr)
    {
        LOG (WARNING) << "Could not resolve fighter with id: " << fighterID;
        return false;           
    }
    
    if(a.GetName() != fighter->GetOwner())
    {
        LOG (WARNING) << "Fighter does not belong to the player, fighter id is: " << fighterID;
        fighter.reset();
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
        fighter.reset();
        return false;              
    }
    
    if(expeditionBlueprint.authoredid() == "c064e7f7-acbf-4f74-fab8-cccd7b2d4004" && (int)a.GetFTUEState() > (int)FTUEState::FirstExpeditionPending)
    {
        LOG (WARNING) << "Could not resolve tutorial expedition twice " << expeditionID;
        fighter.reset();
        return false;            
    }
    
    if(fighter->GetProto().sweetness() < expeditionBlueprint.minsweetness())
    {
        LOG (WARNING) << "Trying to join expedition but fighter doesn't meet sweetness requirements: " << fighterID;
        fighter.reset();
        return false;              
    }
    
    if(fighter->GetProto().sweetness() > expeditionBlueprint.maxsweetness())
    {
        LOG (WARNING) << "Trying to join expedition but fighter doesn't meet sweetness requirements: " << fighterID;
        fighter.reset();
        return false;              
    }    
    
    if(fighter->GetStatus() != FighterStatus::Available)
    {
        LOG (WARNING) << "Trying to join expedition but fighter isn't available : " << fighterID;
        fighter.reset();
        return false;              
    }     
    
    const auto& goodiesList = ctx.RoConfig()->goodies();
    float reductionPercent = 1;
    
    for(const auto& goodie: goodiesList)
    {
        if(a.GetInventory().GetFungibleCount(goodie.first) > 0)
        {
            if(goodie.second.goodytype() == (int8_t)pxd::GoodyType::Espresso)
            {
                weHaveApplibeGoodyName = goodie.first;
                reductionPercent = (float)goodie.second.reductionpercent();
                break;
            }
        }
    }
    int32_t effective_duration = expeditionBlueprint.duration();
    if(weHaveApplibeGoodyName != "")
    {
        effective_duration = effective_duration * reductionPercent;
    }    
    
    duration = effective_duration;
    
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

    const uint32_t recepieID = (uint32_t)recepie["rid"].asInt();
    //const uint32_t fighterID = recepie["fid"].asInt();
    
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

    if(itemInventoryRecipe->GetProto().quality() == 0)
    {
        cookCost = ctx.RoConfig()->params().common_recipe_cook_cost();
    }
    
    if(itemInventoryRecipe->GetProto().quality() == 1)
    {
        cookCost = ctx.RoConfig()->params().uncommon_recipe_cook_cost();
    }

    if(itemInventoryRecipe->GetProto().quality() == 2)
    {
        cookCost = ctx.RoConfig()->params().rare_recipe_cook_cost();
    }

    if(itemInventoryRecipe->GetProto().quality() == 3)
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
    
    float reductionPercent = 1;
    const auto& goodiesList = ctx.RoConfig()->goodies();
    for(const auto& goodie: goodiesList)
    {
        if(playerInventory.GetFungibleCount(goodie.first) > 0)
        {
            if(goodie.second.goodytype() == (int8_t)pxd::GoodyType::PressureCooker)
            {
                weHaveApplibeGoodyName = goodie.first;
                reductionPercent = (float)goodie.second.reductionpercent();
                break;
            }
        }
    }
    
    int32_t effective_duration = itemInventoryRecipe->GetProto().duration();;
    if(weHaveApplibeGoodyName != "")
    {
        effective_duration = effective_duration * reductionPercent;
    }    

    duration = effective_duration;

    return true;    
  }
  
  void MoveProcessor::MaybeClaimReward (const std::string& name, const Json::Value& expedition)
  {      
      std::vector<uint32_t> rewardDatabaseIds;      
      auto a = xayaplayers.GetByName (name, ctx.RoConfig ());
      
      if(!ParseRewardData(*a, name, expedition, rewardDatabaseIds)) return;

      LOG (INFO) << "Ready to give " << rewardDatabaseIds.size() << " rewards";

      uint32_t curRecipeSlots = recipeTbl.CountForOwner(a->GetName());
      uint32_t maxRecipeSlots = ctx.RoConfig()->params().max_recipe_inventory_amount();
      
      const auto& rewardsList = ctx.RoConfig()->activityrewards();
      for(const auto& rw: rewardDatabaseIds)
      {
          /** Lets fetch reward from database, and grant it to the player. 
              While doing so, lets check against space limitations in inv. for recepies    
          */
          
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
              continue;
          }
          
          if((pxd::RewardType)(int)rewardTableDb.rewards(rewardData->GetProto().positionintable()).type() == pxd::RewardType::CraftedRecipe || (pxd::RewardType)(int)rewardTableDb.rewards(rewardData->GetProto().positionintable()).type() == pxd::RewardType::GeneratedRecipe)
          {
              if(curRecipeSlots >= maxRecipeSlots)
              {
                  LOG (INFO) << "Can not grant recipe, maxomus lots reached  of" << curRecipeSlots << " where max is " << maxRecipeSlots;
                  continue;
              }
          }
          
          if((pxd::RewardType)(int)rewardTableDb.rewards(rewardData->GetProto().positionintable()).type() == pxd::RewardType::Candy)
          {              
              std::string candyName = GetCandyKeyNameFromID(rewardTableDb.rewards(rewardData->GetProto().positionintable()).candytype(), ctx);
              a->GetInventory().AddFungibleCount(candyName, rewardTableDb.rewards(rewardData->GetProto().positionintable()).quantity());
              
              LOG (INFO) << "Granted " << rewardTableDb.rewards(rewardData->GetProto().positionintable()).quantity() << " candy " << candyName << " reward";
          }          
          
          if((pxd::RewardType)(int)rewardTableDb.rewards(rewardData->GetProto().positionintable()).type() == pxd::RewardType::CraftedRecipe)
          {         
              auto ourRec = recipeTbl.GetById(rewardData->GetProto().generatedrecipeid());
              ourRec->SetOwner(a->GetName());
              
              LOG (INFO) << "Granted " << " recipe " << rewardData->GetProto().generatedrecipeid() << " reward";
              ourRec.reset();
          }
          
          if((pxd::RewardType)(int)rewardTableDb.rewards(rewardData->GetProto().positionintable()).type() == pxd::RewardType::GeneratedRecipe)
          {
              auto ourRec = recipeTbl.GetById(rewardData->GetProto().generatedrecipeid());
              ourRec->SetOwner(a->GetName());  

              LOG (INFO) << "Granted " << " recipe " << rewardData->GetProto().generatedrecipeid() << " as reward";  
              ourRec.reset();              
          }          
          
          if((pxd::RewardType)(int)rewardTableDb.rewards(rewardData->GetProto().positionintable()).type() == pxd::RewardType::Move)
          {
              auto fighter = fighters.GetById(rewardData->GetProto().fighterid(), ctx.RoConfig());
              std::string* newMove = fighter->MutableProto().add_moves();
              newMove->assign(rewardTableDb.rewards(rewardData->GetProto().positionintable()).moveid());
              
              LOG (INFO) << "Granted " << " move to figher " << rewardData->GetProto().fighterid() << " as reward";   
          }   

          if((pxd::RewardType)(int)rewardTableDb.rewards(rewardData->GetProto().positionintable()).type() == pxd::RewardType::Animation)
          {
              auto fighter = fighters.GetById(rewardData->GetProto().fighterid(), ctx.RoConfig());
              fighter->MutableProto().set_animationid(rewardTableDb.rewards(rewardData->GetProto().positionintable()).animationid()); //TODO into repeated list? Prob... But ok, ignore for now;
          
              LOG (INFO) << "Granted " << " animation to figher " << rewardData->GetProto().fighterid() << " as reward"; 
          }   

          if((pxd::RewardType)(int)rewardTableDb.rewards(rewardData->GetProto().positionintable()).type() == pxd::RewardType::Armor)
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
          
          rewards.DeleteById(rw);
      }
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
    
    if(!ParseTournamentEntryData(*a, name, tournament, tournamentID, fighterIDS)) return;

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

    a->AddBalance (tournamentData->GetProto().joincost()); 
    tournamentData.reset();
    
    if(a->GetFTUEState() == FTUEState::JoinFirstTournament)
    {
      a->SetFTUEState(FTUEState::FirstTournamentPending);
    }     
    
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
    FighterTable::Handle fighter;
    
    if(!ParseExpeditionData(*a, name, expedition, expeditionBlueprint, fighter, duration, weHaveApplibeGoodyName)) return;
    
    proto::OngoinOperation* newOp = a->MutableProto().add_ongoings();
    
    newOp->set_type((uint32_t)pxd::OngoingType::EXPEDITION);
    newOp->set_fighterdatabaseid(expedition["fid"].asInt());
    newOp->set_expeditionblueprintid(expedition["eid"].asString ());

    fighter->SetStatus(FighterStatus::Expedition);
    fighter->MutableProto().set_expeditioninstanceid(expedition["eid"].asString ());
    
    if(weHaveApplibeGoodyName != "")
    {
        a->GetInventory().AddFungibleCount(weHaveApplibeGoodyName, -1);
        newOp->set_appliedgoodykeyname(weHaveApplibeGoodyName);
    }
    
    /** We need to make it at least 1 block, else if will make no sense executing immediately logic flow wise*/
    if(duration < 1)
    {
        duration = 1;
    }    
        
    newOp->set_blocksleft(duration);    
        
    //If we are in tutorial yet, lets advance tutorial counter
    
    if(a->GetFTUEState() == FTUEState::FirstExpedition || a->GetFTUEState() == FTUEState::JoinFirstExpedition)
    {
        a->SetFTUEState(FTUEState::FirstExpeditionPending);
    }    

    LOG (INFO) << "Expedition " << expedition << " submitted succesfully ";
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
    
    //TODO REMOVE FIGHTER

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

    newOp->set_blocksleft(duration);

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
  MoveProcessor::TryTournamentAction (const std::string& name,
                                   const Json::Value& upd)
  {
    if (!upd.isObject ())
      return;
    
    /*Trying to enter the tournament*/
    MaybeEnterTournament (name, upd["e"]);    
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

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

#include "pending.hpp"

#include "gamestatejson.hpp"
#include "jsonutils.hpp"
#include "logic.hpp"

#include <type_traits>

namespace pxd
{

/* ************************************************************************** */

void
PendingState::Clear ()
{
  xayaplayers.clear ();
}

PendingState::XayaPlayerState&
PendingState::GetXayaPlayerState (const XayaPlayer& a)
{
  const auto& name = a.GetName ();

  const auto mit = xayaplayers.find (name);
  if (mit == xayaplayers.end ())
    {
      const auto ins = xayaplayers.emplace (name, XayaPlayerState ());
      CHECK (ins.second);
      VLOG (1)
          << "Account " << name << " was not yet pending, adding pending entry";
      return ins.first->second;
    }

  VLOG (1) << "Account " << name << " is already pending, updating entry";
  return mit->second;
}

void
PendingState::AddCoinTransferBurn (const XayaPlayer& a, const CoinTransferBurn& op)
{
  VLOG (1) << "Adding pending coin operation for " << a.GetName ();

  auto& aState = GetXayaPlayerState (a);

  if (aState.coinOps == nullptr)
    {
      aState.coinOps = std::make_unique<CoinTransferBurn> (op);
      return;
    }

  aState.coinOps->minted += op.minted;
  aState.coinOps->burnt += op.burnt;
  for (const auto& entry : op.transfers)
    aState.coinOps->transfers[entry.first] += entry.second;
}

void
PendingState::AddCrystalPurchase (const XayaPlayer& a, std::string crystalBundleKey)
{
  VLOG (1) << "Adding pending crystal purchase operation for " << a.GetName ();

  auto& aState = GetXayaPlayerState (a);
  aState.balance = a.GetBalance();
  aState.crystalpurchace.push_back(crystalBundleKey);
}

void
PendingState::AddRecepieCookingInstance (const XayaPlayer& a, int32_t duration)
{
  VLOG (1) << "Adding pending recepie cooking operation for name" << a.GetName ();

  auto& aState = GetXayaPlayerState (a);
  aState.balance = a.GetBalance();
  
  proto::OngoinOperation newOp;
  newOp.set_blocksleft(duration);
  newOp.set_type((int8_t)pxd::OngoingType::COOK_RECIPE);  
  
  aState.ongoings.push_back(newOp);
}

void
PendingState::AddExpeditionInstance (const XayaPlayer& a, int32_t duration)
{
  VLOG (1) << "Adding pending expedition operation for name" << a.GetName ();

  auto& aState = GetXayaPlayerState (a);
  aState.balance = a.GetBalance();
  
  proto::OngoinOperation newOp;
  newOp.set_blocksleft(duration);
  newOp.set_type((int8_t)pxd::OngoingType::EXPEDITION);  
  
  aState.ongoings.push_back(newOp);
}

void
PendingState::AddTournamentEntries (const XayaPlayer& a, uint32_t tournamentID, std::vector<uint32_t> fighterIDS)
{
  VLOG (1) << "Adding pending tournament entries for name" << a.GetName ();
  
  auto& aState = GetXayaPlayerState (a);
  
  const auto mit = aState.tournamentEntries.find (tournamentID);
  if (mit == aState.tournamentEntries.end ())
  {
    aState.tournamentEntries.insert(std::pair<uint32_t, std::vector<uint32_t>>(tournamentID, fighterIDS));  
  }
  else
  {
    for(const auto& val: fighterIDS)
    {
      aState.tournamentEntries[tournamentID].push_back(val); 
    }
  }  
}

void
PendingState::AddRewardIDs (const XayaPlayer& a, std::vector<uint32_t> rewardDatabaseIds)
{
  VLOG (1) << "Adding pending claim reward operations for name" << a.GetName ();

  auto& aState = GetXayaPlayerState (a);
  aState.rewardDatabaseIds = rewardDatabaseIds;
}

Json::Value
PendingState::XayaPlayerState::ToJson () const
{
  Json::Value res(Json::objectValue);

  if (coinOps != nullptr)
    {
      Json::Value coin(Json::objectValue);
      coin["minted"] = IntToJson (coinOps->minted);
      coin["burnt"] = IntToJson (coinOps->burnt);

      Json::Value transfers(Json::objectValue);
      for (const auto& entry : coinOps->transfers)
        transfers[entry.first] = IntToJson (entry.second);
      coin["transfers"] = transfers;

      res["coinops"] = coin;
    }
  
  if(crystalpurchace.size() > 0)
  {
      Json::Value bundles(Json::arrayValue);
      for(const auto& bundle: crystalpurchace) 
      {
          bundles.append(bundle);
      }  

      res["crystalbundles"] = bundles;   
      res["balance"] = balance;       
  }  

  if(ongoings.size() > 0)
  {
      Json::Value operations(Json::arrayValue);
      for(const auto& op: ongoings) 
      {
          operations.append((int8_t)op.type());
      }  

      res["ongoings"] = operations;        
  }  
    
  if(rewardDatabaseIds.size() > 0)
  {
    Json::Value dtbrw(Json::arrayValue);
    for(const auto& rw: rewardDatabaseIds) 
    {
      dtbrw.append(rw);
    }  
      
    res["claimingrewards"] = dtbrw;  
  }
  
  if(tournamentEntries.size() > 0)
  {
    Json::Value trnmentr(Json::arrayValue);
    for(const auto& rw: tournamentEntries) 
    {
      Json::Value trnmentr2(Json::arrayValue);
      Json::Value trnm(Json::objectValue);
      trnm["id"] = rw.first;
      
      for(const auto& val: rw.second)
      {
        trnmentr2.append(val);
      }
      
      trnm["fids"] = trnmentr2;
      trnmentr.append(trnm);
    }  
      
    res["tournamententries"] = trnmentr;      
  }
  
  return res;
}

namespace
{

/**
 * Converts a map of entries (building, character, account states) to
 * a JSON array.
 */
template <typename Map>
  Json::Value
  StateMapToJsonArray (const Map& m, const std::string& keyField)
{
  Json::Value res(Json::arrayValue);
  for (const auto& entry : m)
    {
      auto val = entry.second.ToJson ();
      if (std::is_integral<typename Map::key_type>::value)
        val[keyField] = IntToJson (entry.first);
      else
        val[keyField] = entry.first;
      res.append (val);
    }
  return res;
}

} // anonymous namespace

Json::Value
PendingState::ToJson () const
{
  Json::Value res(Json::objectValue);

   res["xayaplayers"] = StateMapToJsonArray (xayaplayers, "name");

  return res;
}

void
PendingStateUpdater::ProcessMove (const Json::Value& moveObj)
{
  std::string name, bundleKeyCode;
  Json::Value mv;
  Amount paidToDev, burnt;

  if (!ExtractMoveBasics (moveObj, name, mv, paidToDev, burnt))
    return;

  auto a = xayaplayers.GetByName (name, ctx.RoConfig ());
  if (a == nullptr)
    {
      /* This is also triggered for moves actually registering an account,
         so it not something really "bad" we need to warn about.  */
      VLOG (1)
          << "Account " << name
          << " does not exist, ignoring pending move " << moveObj;
          
      LOG (WARNING)
          << "Account " << name
          << " does not exist, ignoring pending move " << moveObj;          
      return;
    }
  const bool xayaPlayerInit = a->IsInitialised ();

  CoinTransferBurn coinOps;
  if (ParseCoinTransferBurn (*a, mv, coinOps, burnt))
    state.AddCoinTransferBurn (*a, coinOps);

  /*If we have any recepies trying to submit for cooking, check here*/
  std::map<std::string, pxd::Quantity> fungibleItemAmountForDeduction;
  int32_t cookCost = -1;
  int32_t duration = -1;
  std::string weHaveApplibeGoodyName = "";
  
  Json::Value& upd = mv["ca"];
  if(upd.isObject())
  {
    pxd::proto::ExpeditionBlueprint expeditionBlueprint;
    FighterTable::Handle fighter;
    
    if(ParseCookRecepie(*a, name, upd["r"], fungibleItemAmountForDeduction, cookCost, duration, weHaveApplibeGoodyName))
    {
      state.AddRecepieCookingInstance(*a, duration);
    }
  }
  
  Json::Value& upd2 = mv["exp"];   
  if(upd2.isObject())
  {   
    pxd::proto::ExpeditionBlueprint expeditionBlueprint;
    FighterTable::Handle fighter; 

    if(ParseExpeditionData(*a, name, upd2["f"], expeditionBlueprint, fighter, duration, weHaveApplibeGoodyName))
    {
      state.AddExpeditionInstance(*a, duration);
    }  

    std::vector<uint32_t> rewardDatabaseIds; 
    
    if(ParseRewardData(*a, name, upd2["c"], rewardDatabaseIds))
    {
      state.AddRewardIDs(*a, rewardDatabaseIds);
    }  
    
  }
  
  Json::Value& upd3 = mv["tm"];   
  
  if(upd3.isObject())
  {   
    uint32_t tournamentID = 0;
    std::vector<uint32_t> fighterIDS;   
    
    if(ParseTournamentEntryData(*a, name, upd3["e"], tournamentID, fighterIDS))
    {
      state.AddTournamentEntries(*a, tournamentID, fighterIDS);  
    }
    
  }  
  
  /*If we have any crystal bundles purchases pending, lets add them here*/
 
  Amount cost = 0;
  Amount crystalAmount  = 0;
  
  if(ParseCrystalPurchase(mv, bundleKeyCode, cost, crystalAmount, name, paidToDev))
  {
      state.AddCrystalPurchase(*a, bundleKeyCode);
  }  

  /* Release the account again.  It is not needed anymore, and some of
     the further operations may allocate another Account handle for
     the current name (while it is not allowed to have two active ones
     in parallel).  */
     
  a.reset ();

  /* If the account is not initialised yet, any other action is invalid anyway.
     If this is the init move itself, they would be actually fine, but we
     ignore this edge case for pending processing.  */
  if (!xayaPlayerInit)
    return;
}

/* ************************************************************************** */

PendingMoves::PendingMoves (PXLogic& rules)
  : xaya::SQLiteGame::PendingMoves(rules)
{}

void
PendingMoves::Clear ()
{
  state.Clear ();
}

void
PendingMoves::AddPendingMove (const Json::Value& mv)
{
  auto& db = const_cast<xaya::SQLiteDatabase&> (AccessConfirmedState ());
  PXLogic& rules = dynamic_cast<PXLogic&> (GetSQLiteGame ());
  SQLiteGameDatabase dbObj(db, rules);

  const auto& blk = GetConfirmedBlock ();
  const auto& heightVal = blk["height"];
  CHECK (heightVal.isUInt ());

  const Context ctx(GetChain (),
                    heightVal.asUInt () + 1, Context::NO_TIMESTAMP);

  PendingStateUpdater updater(dbObj, state, ctx);
  updater.ProcessMove (mv);
}

Json::Value
PendingMoves::ToJson () const
{
  return state.ToJson ();
}

/* ************************************************************************** */

} // namespace pxd

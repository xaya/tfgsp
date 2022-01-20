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
PendingState::AddClaimingSweetenerReward (const XayaPlayer& a, const std::string sweetenerAuthId, int32_t fighterID)
{
  VLOG (1) << "Adding pending claiming sweetener reward" << a.GetName ();

  auto& aState = GetXayaPlayerState (a);
  aState.sweetenerClaimingAuthIds.push_back(sweetenerAuthId);
  aState.sweetenerClaimingFightersIds.push_back(fighterID);
}

void
PendingState::AddSweetenerCookingInstance (const XayaPlayer& a, const std::string sweetenerKeyName, int32_t duration, int32_t fighterID)
{
  VLOG (1) << "Adding pending sweetener cooking operation for name" << a.GetName ();

  auto& aState = GetXayaPlayerState (a);
  
  aState.balance = a.GetBalance();
  
  proto::OngoinOperation newOp;
  newOp.set_blocksleft(duration);
  newOp.set_appliedgoodykeyname(sweetenerKeyName);
  newOp.set_fighterdatabaseid(fighterID);
  newOp.set_type((int8_t)pxd::OngoingType::COOK_SWEETENER);  
  
  aState.ongoings.push_back(newOp);
}

void
PendingState::AddRecepieCookingInstance (const XayaPlayer& a, int32_t duration, int32_t recepieID)
{
  VLOG (1) << "Adding pending recepie cooking operation for name" << a.GetName ();

  auto& aState = GetXayaPlayerState (a);
  aState.balance = a.GetBalance();

  proto::OngoinOperation newOp;
  newOp.set_blocksleft(duration);
  newOp.set_type((int8_t)pxd::OngoingType::COOK_RECIPE);  
  newOp.set_recipeid(recepieID);
  
  aState.ongoings.push_back(newOp);
}

void
PendingState::AddPurchasing (const XayaPlayer& a, std::string authID)
{
  VLOG (1) << "Adding pending Purchasingoperation for name" << a.GetName ();
  
  auto& aState = GetXayaPlayerState (a);
  aState.balance = a.GetBalance();
  aState.purchasing.push_back(authID);
}


void
PendingState::AddExpeditionInstance (const XayaPlayer& a, int32_t duration, std::string expeditionID, int32_t fighterID)
{
  VLOG (1) << "Adding pending expedition operation for name" << a.GetName ();

  auto& aState = GetXayaPlayerState (a);

  proto::OngoinOperation newOp;
  newOp.set_blocksleft(duration);
  newOp.set_type((int8_t)pxd::OngoingType::EXPEDITION);  
  newOp.set_expeditionblueprintid(expeditionID);
  newOp.set_fighterdatabaseid(fighterID);
  
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
PendingState::AddTournamentLeaves (const XayaPlayer& a, uint32_t tournamentID, std::vector<uint32_t> fighterIDS)
{
  VLOG (1) << "Adding pending tournament retracts for name" << a.GetName ();
  
  auto& aState = GetXayaPlayerState (a);
  
  const auto mit = aState.tournamentLeaves.find (tournamentID);
  if (mit == aState.tournamentLeaves.end ())
  {
    aState.tournamentLeaves.insert(std::pair<uint32_t, std::vector<uint32_t>>(tournamentID, fighterIDS));  
  }
  else
  {
    for(const auto& val: fighterIDS)
    {
      aState.tournamentLeaves[tournamentID].push_back(val); 
    }
  }
}

void
PendingState::AddFighterForBuyData (const XayaPlayer& a, uint32_t fighterID)
{
  VLOG (1) << "Adding fighting for buy for name" << a.GetName ();
  
  auto& aState = GetXayaPlayerState (a);
  aState.fightingForBuy.push_back(fighterID);  
}

void
PendingState::RemoveFromSaleData (const XayaPlayer& a, uint32_t fighterID)
{
  VLOG (1) << "Removing fighter from auction for buy for name" << a.GetName ();
  
  auto& aState = GetXayaPlayerState (a);
  aState.fightingRemoveSale.push_back(fighterID);  
}

void
PendingState::AddFighterForSaleData (const XayaPlayer& a, uint32_t fighterID)
{
  VLOG (1) << "Adding fighting for sale for name" << a.GetName ();
  
  auto& aState = GetXayaPlayerState (a);
  aState.fightingForSale.push_back(fighterID);  
}

void
PendingState::AddDeconstructionRewardData (const XayaPlayer& a, uint32_t fighterID)
{
  VLOG (1) << "Adding deconstruction reward claiming for name" << a.GetName ();
  
  auto& aState = GetXayaPlayerState (a);
  aState.deconstructionDataClaiming.push_back(fighterID);  
}

void
PendingState::AddDeconstructionData (const XayaPlayer& a, uint32_t fighterID)
{
  VLOG (1) << "Adding pending tournament retracts for name" << a.GetName ();
  
  auto& aState = GetXayaPlayerState (a);
  aState.deconstructionData.push_back(fighterID);  
}

void
PendingState::AddRewardIDs (const XayaPlayer& a, std::string expeditionName, std::vector<uint32_t> rewardDatabaseIds)
{
  VLOG (1) << "Adding pending claim reward operations for name" << a.GetName ();

  auto& aState = GetXayaPlayerState (a);
  
  const auto mit = aState.rewardDatabaseIds.find (expeditionName);
  if (mit == aState.rewardDatabaseIds.end ())
  {
    aState.rewardDatabaseIds.insert(std::pair<std::string, std::vector<uint32_t>>(expeditionName, rewardDatabaseIds));  
  }
  else
  {
    for(const auto& val: rewardDatabaseIds)
    {
      aState.rewardDatabaseIds[expeditionName].push_back(val); 
    }
  }
}

void
PendingState::AddTournamentRewardIDs (const XayaPlayer& a, uint32_t tournamentID, std::vector<uint32_t> rewardDatabaseIds)
{
  VLOG (1) << "Adding pending claim reward operations for name" << a.GetName ();

  auto& aState = GetXayaPlayerState (a);
  
  const auto mit = aState.rewardDatabaseIdsTournaments.find (tournamentID);
  if (mit == aState.rewardDatabaseIdsTournaments.end ())
  {
    aState.rewardDatabaseIdsTournaments.insert(std::pair<uint32_t, std::vector<uint32_t>>(tournamentID, rewardDatabaseIds));  
  }
  else
  {
    for(const auto& val: rewardDatabaseIds)
    {
      aState.rewardDatabaseIdsTournaments[tournamentID].push_back(val); 
    }
  }
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

  bool addBalance = false;

  if(ongoings.size() > 0)
  {
      Json::Value operations(Json::arrayValue);
      for(const auto& op: ongoings) 
      {
          Json::Value ongOBJ(Json::objectValue);
          ongOBJ["ival"] = -1;
          ongOBJ["sval"] = "";         
          ongOBJ["type"] = (int8_t)op.type();  
          
          if((int8_t)op.type() == (int8_t)pxd::OngoingType::EXPEDITION)
          {        
            ongOBJ["sval"] = op.expeditionblueprintid();
            ongOBJ["ival"] = IntToJson (op.fighterdatabaseid());
          }
          
          if((int8_t)op.type() == (int8_t)pxd::OngoingType::COOK_RECIPE)
          {        
            ongOBJ["ival"] = IntToJson (op.recipeid());
            addBalance = true;
          }       

          if((int8_t)op.type() == (int8_t)pxd::OngoingType::COOK_SWEETENER)
          {     
            ongOBJ["ival"] = IntToJson (op.fighterdatabaseid());
            ongOBJ["sval"] = op.appliedgoodykeyname();
            addBalance = true;
          }              
          
          operations.append(ongOBJ);
      }  

      res["ongoings"] = operations;        
  }

  if(addBalance)
  {
      res["balance"] = balance;
  }
    
  if(rewardDatabaseIds.size() > 0)
  {
    Json::Value dtbrw(Json::arrayValue);
    for(const auto& rw: rewardDatabaseIds) 
    {
      Json::Value rwOBJ(Json::objectValue);
      rwOBJ["type"] = rw.first;
      
      Json::Value idsArr(Json::arrayValue);
      for(const auto& vv: rw.second) 
      {
      idsArr.append(vv);
      }
      
      rwOBJ["ids"] = idsArr;
      
      dtbrw.append(rwOBJ);
    }  
        
    res["claimingrewards"] = dtbrw;  
  }
  
  if(rewardDatabaseIdsTournaments.size() > 0)
  {
    Json::Value dtbrw(Json::arrayValue);
    for(const auto& rw: rewardDatabaseIdsTournaments) 
    {
      Json::Value rwOBJ(Json::objectValue);
      rwOBJ["type"] = IntToJson(rw.first);
      
      Json::Value idsArr(Json::arrayValue);
      for(const auto& vv: rw.second) 
      {
      idsArr.append(vv);
      }
      
      rwOBJ["ids"] = idsArr;
      
      dtbrw.append(rwOBJ);
    }  
        
    res["claimingrewardstournament"] = dtbrw;  
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
  
  if(tournamentLeaves.size() > 0)
  {
    Json::Value trnmentr(Json::arrayValue);
    for(const auto& rw: tournamentLeaves) 
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
      
    res["tournamentleaves"] = trnmentr;      
  } 
  
  if(deconstructionData.size() > 0)
  {
    Json::Value fghttrs(Json::arrayValue);
    for(const auto& rw: deconstructionData) 
    {
      fghttrs.append(rw);
    }  
      
    res["deconstruction"] = fghttrs;      
  }    
  
  if(purchasing.size() > 0)
  {
    Json::Value purchasingArr(Json::arrayValue);
    for(const auto& pp: purchasing) 
    {
      purchasingArr.append(pp);
    }  
      
    res["purchasing"] = purchasingArr;   
    res["balance"] = balance;    
  }  
  
  if(sweetenerClaimingAuthIds.size() > 0)
  {
    Json::Value sweetClaimingArr(Json::arrayValue);
    for(const auto& sclm: sweetenerClaimingAuthIds) 
    {
      sweetClaimingArr.append(sclm);
    }  
      
    res["sweetclmauth"] = sweetClaimingArr;         
    
    Json::Value sweetClaimingFighterArr(Json::arrayValue);
    for(const auto& sclm: sweetenerClaimingFightersIds) 
    {
      sweetClaimingFighterArr.append(sclm);
    }  
      
    res["sweetclmfghtr"] = sweetClaimingFighterArr;       
  }
  
  if(fightingForSale.size() > 0)
  {
    Json::Value forSaleArr(Json::arrayValue);
    for(const auto& fs: fightingForSale) 
    {
      forSaleArr.append(fs);
    }  
      
    res["forsale"] = forSaleArr;   
    res["balance"] = balance;        
  }
  
  if(fightingForBuy.size() > 0)
  {
    Json::Value forBuyArr(Json::arrayValue);
    for(const auto& fb: fightingForBuy) 
    {
      forBuyArr.append(fb);
    }  
      
    res["forbuy"] = forBuyArr;          
  }  
  
  if(fightingRemoveSale.size() > 0)
  {
    Json::Value forRemoveArr(Json::arrayValue);
    for(const auto& fr: fightingRemoveSale) 
    {
      forRemoveArr.append(fr);
    }  
      
    res["forremove"] = forRemoveArr;          
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

  CoinTransferBurn coinOps;
  if (ParseCoinTransferBurn (*a, mv, coinOps, burnt))
    state.AddCoinTransferBurn (*a, coinOps);

  /*If we have any recepies trying to submit for cooking, check here*/
  std::map<std::string, pxd::Quantity> fungibleItemAmountForDeduction;
  int32_t cookCost = -1;
  uint32_t fighterID = -1;
  int32_t duration = -1;
  std::string weHaveApplibeGoodyName = "";
  std::string sweetenerKeyName = "";
  std::vector<uint32_t> rewardDatabaseIds;  
  Amount price = 0;
  
  Json::Value& upd = mv["ca"];
  if(upd.isObject())
  {
    pxd::proto::ExpeditionBlueprint expeditionBlueprint;
    FighterTable::Handle fighter;
    
    if(ParseCookRecepie(*a, name, upd["r"], fungibleItemAmountForDeduction, cookCost, duration, weHaveApplibeGoodyName))
    {
       state.AddRecepieCookingInstance(*a, duration, upd["r"]["rid"].asInt());
    }
    
    if(ParseSweetener(*a, name, upd["s"], fungibleItemAmountForDeduction, cookCost, fighterID, duration, sweetenerKeyName))
    {
       state.AddSweetenerCookingInstance(*a, sweetenerKeyName, duration, fighterID);  
    }
    
    std::string sweetenerAuthId = "";
    if(ParseClaimSweetener(*a, name, upd["sc"], fighterID, rewardDatabaseIds, sweetenerAuthId))
    {
       state.AddClaimingSweetenerReward(*a, sweetenerAuthId, fighterID);   
    }
  }
  
  Json::Value& upd2 = mv["exp"];   
  if(upd2.isObject())
  {   
    pxd::proto::ExpeditionBlueprint expeditionBlueprint;
    FighterTable::Handle fighter; 

    if(ParseExpeditionData(*a, name, upd2["f"], expeditionBlueprint, fighter, duration, weHaveApplibeGoodyName))
    {
      state.AddExpeditionInstance(*a, duration, expeditionBlueprint.authoredid(), fighter->GetId());
    }  

    std::string expeditionID = "";
    
    if(ParseRewardData(*a, name, upd2["c"], rewardDatabaseIds, expeditionID))
    {
      state.AddRewardIDs(*a, expeditionID, rewardDatabaseIds);
    }  
    
  }
  
  Json::Value& upd3 = mv["tm"];   
  
  if(upd3.isObject())
  {   
    uint32_t tournamentID = 0;
    std::vector<uint32_t> fighterIDS;   
    std::vector<uint32_t> fighterIDSL; 
    
    if(ParseTournamentEntryData(*a, name, upd3["e"], tournamentID, fighterIDS))
    {
       state.AddTournamentEntries(*a, tournamentID, fighterIDS);  
    }
    
    if(ParseTournamentLeaveData(*a, name, upd3["l"], tournamentID, fighterIDSL))
    {
       state.AddTournamentLeaves(*a, tournamentID, fighterIDSL);  
    }    
    
    
    if(ParseTournamentRewardData(*a, name, upd3["c"], rewardDatabaseIds, tournamentID))
    {
       state.AddTournamentRewardIDs(*a, tournamentID, rewardDatabaseIds); 
    }    
  }  
  
  Json::Value& upd4 = mv["f"];   
  
  if(upd4.isObject())
  {  
    if(ParseDeconstructData(*a, name, upd4["d"], fighterID))
    {
      state.AddDeconstructionData(*a, fighterID);  
    }
    
    if(ParseDeconstructRewardData(*a, name, upd4["c"], fighterID)) 
    {
      state.AddDeconstructionRewardData(*a, fighterID);  
    }    
    
    uint32_t durationI = -1;
    if(ParseFighterForSaleData(*a, name, upd4["s"], fighterID, durationI, price))
    {
      state.AddFighterForSaleData(*a, fighterID);  
    }   

    if(ParseBuyData(*a, name, upd4["b"], fighterID)) 
    {
      state.AddFighterForBuyData(*a, fighterID);  
    }        
    
    if(ParseRemoveBuyData(*a, name, upd4["r"], fighterID))
    {
      state.RemoveFromSaleData(*a, fighterID);  
    }
  }

  /*If we have any crystal bundles purchases pending, lets add them here*/
 
  Amount cost = 0;
  Amount crystalAmount  = 0;
  std::string fungibleName = "";
  
  if(ParseCrystalPurchase(mv, bundleKeyCode, cost, crystalAmount, name, paidToDev))
  {
      state.AddCrystalPurchase(*a, bundleKeyCode);
  }  
  
  if(ParseGoodyPurchase(mv, cost, name, fungibleName, a->GetBalance()))
  {
      state.AddPurchasing(*a, mv["pg"].asString());  
  }
  
  if(ParseSweetenerPurchase(mv, cost, name, fungibleName, a->GetBalance()))
  {
      state.AddPurchasing(*a, mv["ps"].asString());
  }  
  
  std::map<std::string, uint64_t> fungibles;
  if(ParseGoodyBundlePurchase(mv, cost, name, fungibles, a->GetBalance()))
  {
      state.AddPurchasing(*a, mv["pgb"].asString());
  }  

  /* Release the account again.  It is not needed anymore, and some of
     the further operations may allocate another Account handle for
     the current name (while it is not allowed to have two active ones
     in parallel).  */
     
  a.reset ();
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

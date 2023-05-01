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
PendingState::GetXayaPlayerState (const XayaPlayer& a, FighterTable& fighters, const pxd::RoConfig& config)
{
  const auto& name = a.GetName ();

  const auto mit = xayaplayers.find (name);
  if (mit == xayaplayers.end ())
    {
      const auto ins = xayaplayers.emplace (name, XayaPlayerState ());
      
      ins.first->second.balance = a.GetBalance();
      ins.first->second.onChainBalance = a.GetBalance();
      
      /*lets be double sure we are having correctly ordered map to avoid forks*/
      
      std::vector<std::pair<std::string, uint64_t>> sortedFungibles;
      for (const auto& entry : a.GetInventory().GetFungible ())
        sortedFungibles.push_back(std::pair<std::string, uint64_t>(entry.first, entry.second));

      sort(sortedFungibles.begin(), sortedFungibles.end(), [=](std::pair<std::string, uint64_t>& a, std::pair<std::string, uint64_t>& b)
      {
        return a.first < b.first;
      } 
      );        
      
      std::map<std::string, uint64_t> currentFungibleSetCC;

      for(auto& entry: sortedFungibles)
      {
          currentFungibleSetCC.insert(std::pair<std::string, uint64_t>(entry.first, entry.second));
      }      
      
      ins.first->second.currentFungibleSet = currentFungibleSetCC;
      std::map<std::string, uint64_t> fungiblesCopy(currentFungibleSetCC);
      ins.first->second.onChainFungibleSet = fungiblesCopy;     
      CHECK (ins.second);
	  
	  // Prepare data for transifugre proper checks
	 Json::Value wholeFightersData(Json::objectValue);  

	 auto res3 = fighters.QueryForOwner (name);
	 bool tryAndStep3 = res3.Step();
		
	 while (tryAndStep3)
	 {
			auto fighterDb = fighters.GetFromResult (res3, config); 
			
			std::stringstream keySS;
			keySS << fighterDb->GetId();
			std::string keySSStr = keySS.str();
			
			wholeFightersData[keySSStr]["name"] = fighterDb->GetProto().name();
			
			Json::Value armorPieces(Json::arrayValue);
			for(auto& ap : fighterDb->GetProto().armorpieces())
			{
				Json::Value piece(Json::objectValue);
				piece["candy"] = ap.candy();
				piece["armortype"] = ap.armortype();
				armorPieces.append(piece);
			}	
			wholeFightersData[keySSStr]["armorpieces"] = armorPieces;			
			
			Json::Value moves(Json::arrayValue);
			for(auto& mv : fighterDb->GetProto().moves())
			{
				moves.append(mv);
			}	
			wholeFightersData[keySSStr]["moves"] = moves;
			
			fighterDb.reset();
			tryAndStep3 = res3.Step();     

            ins.first->second.onChainPlayerFighterData.push_back(wholeFightersData);			
	 }

     VLOG (1) << "Account " << name << " was not yet pending, adding pending entry";
     return ins.first->second;
    }

  VLOG (1) << "Account " << name << " is already pending, updating entry";
  return mit->second;
}

void
PendingState::AddCoinTransferBurn (const XayaPlayer& a, const CoinTransferBurn& op, FighterTable& fighters, const pxd::RoConfig& config)
{
  VLOG (1) << "Adding pending coin operation for " << a.GetName ();

  auto& aState = GetXayaPlayerState (a, fighters, config);

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
PendingState::AddCrystalPurchase (const XayaPlayer& a, std::string crystalBundleKey, Amount crystalAmount, FighterTable& fighters, const pxd::RoConfig& config)
{
  VLOG (1) << "Adding pending crystal purchase operation for " << a.GetName ();

  auto& aState = GetXayaPlayerState (a, fighters, config);
  aState.balance += crystalAmount;
  aState.crystalpurchace.push_back(crystalBundleKey);
}

void
PendingState::AddClaimingSweetenerReward (const XayaPlayer& a, const std::string sweetenerAuthId, int32_t fighterID, FighterTable& fighters, const pxd::RoConfig& config)
{
  VLOG (1) << "Adding pending claiming sweetener reward" << a.GetName ();

  auto& aState = GetXayaPlayerState (a, fighters, config);
  aState.sweetenerClaimingAuthIds.push_back(sweetenerAuthId);
  aState.sweetenerClaimingFightersIds.push_back(fighterID);
}

void
PendingState::AddSweetenerCookingInstance (const XayaPlayer& a, const std::string sweetenerKeyName, int32_t duration, int32_t fighterID, Amount cookingCost, std::map<std::string, pxd::Quantity> fungibleItemAmountForDeduction, FighterTable& fighters, const pxd::RoConfig& config)
{
  VLOG (1) << "Adding pending sweetener cooking operation for name" << a.GetName ();

  auto& aState = GetXayaPlayerState (a, fighters, config);
  
  aState.balance -= cookingCost;
  
  proto::OngoinOperation newOp;
  newOp.set_blocksleft(duration);
  newOp.set_appliedgoodykeyname(sweetenerKeyName);
  newOp.set_fighterdatabaseid(fighterID);
  newOp.set_type((int8_t)pxd::OngoingType::COOK_SWEETENER);  
  
  for(auto& item: fungibleItemAmountForDeduction)
  {
      aState.currentFungibleSet[item.first] -= item.second;
  }
  
  aState.ongoings.push_back(newOp);
}

void PendingState::AddCookedRecepieCollectInstance(const XayaPlayer& a, int32_t fighterToCollectID, FighterTable& fighters, const pxd::RoConfig& config)
{
  VLOG (1) << "Adding pending collect cooked recepie" << a.GetName ();
  auto& aState = GetXayaPlayerState (a, fighters, config);
  aState.cookedFightersToCollect.push_back(fighterToCollectID);
}

void
PendingState::AddRecepieCookingInstance (const XayaPlayer& a, int32_t duration, int32_t recepieID, Amount cookingCost, std::map<std::string, pxd::Quantity> fungibleItemAmountForDeduction, FighterTable& fighters, const pxd::RoConfig& config)
{
  VLOG (1) << "Adding pending recepie cooking operation for name" << a.GetName ();

  auto& aState = GetXayaPlayerState (a, fighters, config);
  aState.balance -= cookingCost;

  proto::OngoinOperation newOp;
  newOp.set_blocksleft(duration);
  newOp.set_type((int8_t)pxd::OngoingType::COOK_RECIPE);  
  newOp.set_recipeid(recepieID);
  
  for(auto& item: fungibleItemAmountForDeduction)
  {
      aState.currentFungibleSet[item.first] -= item.second;
  }  
  
  aState.ongoings.push_back(newOp);
}

void
PendingState::AddRecepieDestroyInstance (const XayaPlayer& a, int32_t duration, std::vector<uint32_t>& recepieIDS, FighterTable& fighters, const pxd::RoConfig& config)
{
  VLOG (1) << "Adding pending recepie destroy operation for name" << a.GetName ();

  auto& aState = GetXayaPlayerState (a, fighters, config);
  
  for(auto& rcp: recepieIDS)
  {
    aState.destroyrecipe.push_back(rcp);
  }
}

void
PendingState::AddPurchasing (const XayaPlayer& a, std::string authID, Amount purchaseCost, FighterTable& fighters, const pxd::RoConfig& config)
{
  VLOG (1) << "Adding pending Purchasingoperation for name" << a.GetName ();
  
  auto& aState = GetXayaPlayerState (a, fighters, config);
  aState.balance -= purchaseCost;
  aState.purchasing.push_back(authID);
}


void
PendingState::AddExpeditionInstance (const XayaPlayer& a, int32_t duration, std::string expeditionID, std::vector<int> fighterID, FighterTable& fighters, const pxd::RoConfig& config)
{
  VLOG (1) << "Adding pending expedition operation for name" << a.GetName ();

  auto& aState = GetXayaPlayerState (a, fighters, config);

  for(auto& ft : fighterID)
  {
    proto::OngoinOperation newOp;
    newOp.set_blocksleft(duration);
    newOp.set_type((int8_t)pxd::OngoingType::EXPEDITION);  
    newOp.set_expeditionblueprintid(expeditionID);
    newOp.set_fighterdatabaseid(ft);
    
    aState.ongoings.push_back(newOp);
  }
}

void
PendingState::AddTournamentEntries (const XayaPlayer& a, uint32_t tournamentID, std::vector<uint32_t> fighterIDS, FighterTable& fighters, const pxd::RoConfig& config)
{
  VLOG (1) << "Adding pending tournament entries for name" << a.GetName ();
  
  auto& aState = GetXayaPlayerState (a, fighters, config);
  
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
PendingState::AddSpecialTournamentEntries (const XayaPlayer& a, uint32_t tournamentID, std::vector<uint32_t> fighterIDS, FighterTable& fighters, const pxd::RoConfig& config)
{
  VLOG (1) << "Adding pending special tournament entries for name" << a.GetName ();
  
  auto& aState = GetXayaPlayerState (a, fighters, config);
  
  aState.balance -= 10;  
  
  const auto mit = aState.specialTournamentEntries.find (tournamentID);
  if (mit == aState.specialTournamentEntries.end ())
  {
    aState.specialTournamentEntries.insert(std::pair<uint32_t, std::vector<uint32_t>>(tournamentID, fighterIDS));  
  }
  else
  {
    for(const auto& val: fighterIDS)
    {
      aState.specialTournamentEntries[tournamentID].push_back(val); 
    }
  }  
}

void
PendingState::AddSpecialTournamentLeaves (const XayaPlayer& a, uint32_t tournamentID, std::vector<uint32_t> fighterIDS, FighterTable& fighters, const pxd::RoConfig& config)
{
  VLOG (1) << "Adding pending special tournament retracts for name" << a.GetName ();
  
  auto& aState = GetXayaPlayerState (a, fighters, config);
  
  const auto mit = aState.specialTournamentLeaves.find (tournamentID);
  if (mit == aState.specialTournamentLeaves.end ())
  {
    aState.specialTournamentLeaves.insert(std::pair<uint32_t, std::vector<uint32_t>>(tournamentID, fighterIDS));  
  }
  else
  {
    for(const auto& val: fighterIDS)
    {
      aState.specialTournamentLeaves[tournamentID].push_back(val); 
    }
  }
}

void
PendingState::AddTournamentLeaves (const XayaPlayer& a, uint32_t tournamentID, std::vector<uint32_t> fighterIDS, FighterTable& fighters, const pxd::RoConfig& config)
{
  VLOG (1) << "Adding pending tournament retracts for name" << a.GetName ();
  
  auto& aState = GetXayaPlayerState (a, fighters, config);
  
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
PendingState::AddFighterForBuyData (const XayaPlayer& a, uint32_t fighterID, Amount exchangeprice, FighterTable& fighters, const pxd::RoConfig& config)
{
  VLOG (1) << "Adding fighting for buy for name" << a.GetName ();
  
  auto& aState = GetXayaPlayerState (a, fighters, config);
  
  aState.balance -= exchangeprice;
  aState.fightingForBuy.push_back(fighterID);  
}

void
PendingState::AddTransfigureData (const XayaPlayer& a, Json::Value data, FighterTable& fighters, const pxd::RoConfig& config)
{
  VLOG (1) << "Transfigure fighter of player" << a.GetName ();
  
  auto& aState = GetXayaPlayerState (a, fighters, config);
  aState.pendingTransfigure.push_back(data);  
}

void
PendingState::RemoveFromSaleData (const XayaPlayer& a, uint32_t fighterID, FighterTable& fighters, const pxd::RoConfig& config)
{
  VLOG (1) << "Removing fighter from auction for buy for name" << a.GetName ();
  
  auto& aState = GetXayaPlayerState (a, fighters, config);
  aState.fightingRemoveSale.push_back(fighterID);  
}

void
PendingState::AddFighterForSaleData (const XayaPlayer& a, uint32_t fighterID, Amount listingfee, FighterTable& fighters, const pxd::RoConfig& config)
{
  VLOG (1) << "Adding fighting for sale for name" << a.GetName ();
  
  auto& aState = GetXayaPlayerState (a, fighters, config);
  aState.balance -= listingfee;
  aState.fightingForSale.push_back(fighterID);  
}

void
PendingState::AddDeconstructionRewardData (const XayaPlayer& a, uint32_t fighterID, FighterTable& fighters, const pxd::RoConfig& config)
{
  VLOG (1) << "Adding deconstruction reward claiming for name" << a.GetName ();
  
  auto& aState = GetXayaPlayerState (a, fighters, config);
  aState.deconstructionDataClaiming.push_back(fighterID);  
}

void
PendingState::AddDeconstructionData (const XayaPlayer& a, uint32_t fighterID, FighterTable& fighters, const pxd::RoConfig& config)
{
  VLOG (1) << "Adding pending tournament retracts for name" << a.GetName ();
  
  auto& aState = GetXayaPlayerState (a, fighters, config);
  aState.deconstructionData.push_back(fighterID);  
}

void
PendingState::AddRewardIDs (const XayaPlayer& a, std::vector<std::string> expeditionIDArray, std::vector<uint32_t> rewardDatabaseIds, FighterTable& fighters, const pxd::RoConfig& config)
{
  VLOG (1) << "Adding pending claim reward operations for name" << a.GetName ();

  auto& aState = GetXayaPlayerState (a, fighters, config);
  
  for(auto& expeditionName: expeditionIDArray)
  {
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
}

void
PendingState::AddTournamentRewardIDs (const XayaPlayer& a, uint32_t tournamentID, std::vector<uint32_t> rewardDatabaseIds, FighterTable& fighters, const pxd::RoConfig& config)
{
  VLOG (1) << "Adding pending claim reward operations for name" << a.GetName ();

  auto& aState = GetXayaPlayerState (a, fighters, config);
  
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
  }  

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
          }       

          if((int8_t)op.type() == (int8_t)pxd::OngoingType::COOK_SWEETENER)
          {     
            ongOBJ["ival"] = IntToJson (op.fighterdatabaseid());
            ongOBJ["sval"] = op.appliedgoodykeyname();
          }              
          
          operations.append(ongOBJ);
      }  

      res["ongoings"] = operations;        
  }

  if(balance != onChainBalance)
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
  
  if(specialTournamentEntries.size() > 0)
  {
    Json::Value trnmentr(Json::arrayValue);
    for(const auto& rw: specialTournamentEntries) 
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
      
    res["specialtournamententries"] = trnmentr;      
  }  
  
  if(specialTournamentLeaves.size() > 0)
  {
    Json::Value trnmentr(Json::arrayValue);
    for(const auto& rw: specialTournamentLeaves) 
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
      
    res["specialtournamentleaves"] = trnmentr;      
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

  if(deconstructionDataClaiming.size() > 0)
  {
    Json::Value fghttrs(Json::arrayValue);
    for(const auto& rw: deconstructionDataClaiming) 
    {
      fghttrs.append(rw);
    }  
      
    res["deconstructionClaiming"] = fghttrs;      
  }     
  
  if(cookedFightersToCollect.size() > 0)
  {
    Json::Value fghttrs(Json::arrayValue);
    for(const auto& rw: cookedFightersToCollect) 
    {
      fghttrs.append(rw);
    }  
      
    res["cookedFightersToCollect"] = fghttrs;      
  }   
  
  if(destroyrecipe.size() > 0)
  {
    Json::Value rcp(Json::arrayValue);
    for(const auto& rw: destroyrecipe) 
    {
      rcp.append(rw);
    }  
      
    res["recipedestroy"] = rcp;      
  }  
  
  if(purchasing.size() > 0)
  {
    Json::Value purchasingArr(Json::arrayValue);
    for(const auto& pp: purchasing) 
    {
      purchasingArr.append(pp);
    }  
      
    res["purchasing"] = purchasingArr;       
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
  
  std::map<std::string, uint64_t> affectedMap;
  for(auto& item : currentFungibleSet)
  {
    for(auto& item2 : onChainFungibleSet)
    { 
      if(item.first == item2.first && item.second != item2.second)
      {
       affectedMap.insert(std::pair<std::string, uint64_t>(item.first, item.second)); 
      }
    }
  }
  
  if(affectedMap.size() > 0)
  {
    Json::Value affectedItemsArr(Json::arrayValue);
    
    for(const auto& it: affectedMap) 
    {
      Json::Value item(Json::objectValue);  
      item["name"] = it.first;
      item["value"] = it.second;
      affectedItemsArr.append(item);
    }  
      
    res["itemchange"] = affectedItemsArr;                
  }
  
  if(pendingTransfigure.size() > 0)
  {
	Json::Value transfigurers(Json::arrayValue);
	
    for(const auto& trsf: pendingTransfigure) 
    {		
	  //Lets detect, what actually did change? TODO :: this needs some deep refactoring of code, so lets do this front-end only for now
	 /*	
	 if(trsf["o"] == 1)
	 {
		 // Lets compare names of the fighter we submited before, and his name we have now
		auto fighterDb = fighters.GetById (trsf["fid"].asInt(), config);
		fighterDb.reset(); 
	 }

	  }*/
	  
	  
	  
	  transfigurers.append(trsf);
	}		
	
	res["transfigurers"] = transfigurers; 
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
  std::map<std::string, Amount> paidToCrownHolders;
  Amount  burnt;

  if (!ExtractMoveBasics (moveObj, name, mv, paidToCrownHolders, burnt))
    return;

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

  for(auto& mrl: moves)
  {
      CoinTransferBurn coinOps;
      if (ParseCoinTransferBurn (*a, mrl, coinOps, burnt))
        state.AddCoinTransferBurn (*a, coinOps, fighters, ctx.RoConfig ());

      /*If we have any recepies trying to submit for cooking, check here*/
      std::map<std::string, pxd::Quantity> fungibleItemAmountForDeduction;
      int32_t cookCost = -1;
      uint32_t fighterID = -1;
      int32_t duration = -1;
      std::string weHaveApplibeGoodyName = "";
      std::string sweetenerKeyName = "";
      std::vector<uint32_t> rewardDatabaseIds;  
      Amount price = 0;
      
      Json::Value& upd = mrl["ca"];
      if(upd.isObject())
      {
        pxd::proto::ExpeditionBlueprint expeditionBlueprint;
        FighterTable::Handle fighter;
        int32_t fighterIdToCollect = 0;
        
        if(ParseCollectCookRecepie(*a, name, upd["cl"], fighterIdToCollect))
        {
           state.AddCookedRecepieCollectInstance(*a, fighterIdToCollect, fighters, ctx.RoConfig ());
        }    
        
        std::vector<uint32_t> recepieIDS;
        if(ParseDestroyRecepie(*a, name, upd["d"], recepieIDS))
        {
           state.AddRecepieDestroyInstance(*a, duration, recepieIDS, fighters, ctx.RoConfig ());
        }    
        
        if(ParseCookRecepie(*a, name, upd["r"], fungibleItemAmountForDeduction, cookCost, duration, weHaveApplibeGoodyName))
        {
           state.AddRecepieCookingInstance(*a, duration, upd["r"]["rid"].asInt(), cookCost, fungibleItemAmountForDeduction, fighters, ctx.RoConfig ());
        }
        
        if(ParseSweetener(*a, name, upd["s"], fungibleItemAmountForDeduction, cookCost, fighterID, duration, sweetenerKeyName))
        {
           state.AddSweetenerCookingInstance(*a, sweetenerKeyName, duration, fighterID, cookCost, fungibleItemAmountForDeduction, fighters, ctx.RoConfig ());  
        }
        
        std::string sweetenerAuthId = "";
        if(ParseClaimSweetener(*a, name, upd["sc"], fighterID, rewardDatabaseIds, sweetenerAuthId))
        {
           state.AddClaimingSweetenerReward(*a, sweetenerAuthId, fighterID, fighters, ctx.RoConfig ());   
        }
      }
      
      Json::Value& upd2 = mrl["exp"];   
      if(upd2.isObject())
      {   
        pxd::proto::ExpeditionBlueprint expeditionBlueprint;
        std::vector<int> fighter; 

        if(ParseExpeditionData(*a, name, upd2["f"], expeditionBlueprint, fighter, duration, weHaveApplibeGoodyName))
        {
          state.AddExpeditionInstance(*a, duration, expeditionBlueprint.authoredid(), fighter, fighters, ctx.RoConfig ());
        }  

        std::vector<std::string> expeditionIDArray;
        
        if(ParseRewardData(*a, name, upd2["c"], rewardDatabaseIds, expeditionIDArray))
        {
          state.AddRewardIDs(*a, expeditionIDArray, rewardDatabaseIds, fighters, ctx.RoConfig ());
        }  
        
      }
      
      Json::Value& upd3 = mrl["tm"];   
      
      if(upd3.isObject())
      {   
        uint32_t tournamentID = 0;
        std::vector<uint32_t> fighterIDS;   
        std::vector<uint32_t> fighterIDSL; 
        
        if(ParseTournamentEntryData(*a, name, upd3["e"], tournamentID, fighterIDS))
        {
           state.AddTournamentEntries(*a, tournamentID, fighterIDS, fighters, ctx.RoConfig ());  
        }
        
        if(ParseTournamentLeaveData(*a, name, upd3["l"], tournamentID, fighterIDSL))
        {
           state.AddTournamentLeaves(*a, tournamentID, fighterIDSL, fighters, ctx.RoConfig ());  
        }    
        
        if(ParseTournamentRewardData(*a, name, upd3["c"], rewardDatabaseIds, tournamentID))
        {
           state.AddTournamentRewardIDs(*a, tournamentID, rewardDatabaseIds, fighters, ctx.RoConfig ()); 
        }    
      }  
      
      Json::Value& upd3x = mrl["tms"];
      
      if(upd3x.isObject())
      {   
        uint32_t tournamentID = 0;
        std::vector<uint32_t> fighterIDS;   
        std::vector<uint32_t> fighterIDSL;
        
        if(ParseSpecialTournamentEntryData(*a, name, upd3x["e"], tournamentID, fighterIDS))       
        {
           state.AddSpecialTournamentEntries(*a, tournamentID, fighterIDS, fighters, ctx.RoConfig ());  
        }
        
        if(ParseSpecialTournamentLeaveData(*a, name, upd3["l"], tournamentID, fighterIDSL))
        {
           state.AddSpecialTournamentLeaves(*a, tournamentID, fighterIDSL, fighters, ctx.RoConfig ());  
        }     
      }    
      
      Json::Value& upd4 = mrl["f"];   
      Amount exchangeprice = 0;
      Amount listingfee = 0;
      
      if(upd4.isObject())
      {  
        if(ParseDeconstructData(*a, name, upd4["d"], fighterID))
        {
          state.AddDeconstructionData(*a, fighterID, fighters, ctx.RoConfig ());  
        }
        
        if(ParseDeconstructRewardData(*a, name, upd4["c"], fighterID)) 
        {
          state.AddDeconstructionRewardData(*a, fighterID, fighters, ctx.RoConfig ());  
        }    
        
        uint32_t durationI = -1;
        if(ParseFighterForSaleData(*a, name, upd4["s"], fighterID, durationI, price, listingfee))
        {
          state.AddFighterForSaleData(*a, fighterID, listingfee, fighters, ctx.RoConfig ());  
        }   

        if(ParseBuyData(*a, name, upd4["b"], fighterID, exchangeprice)) 
        {
          state.AddFighterForBuyData(*a, fighterID, exchangeprice, fighters, ctx.RoConfig ());  
        }        
        
        if(ParseRemoveBuyData(*a, name, upd4["r"], fighterID))
        {
          state.RemoveFromSaleData(*a, fighterID, fighters, ctx.RoConfig ());  
        }
		
        if(ParseTransfigureData(*a, name, upd4["t"]))
        {
          state.AddTransfigureData(*a, upd4["t"], fighters, ctx.RoConfig ());  
        }		
      }

      /*If we have any crystal bundles purchases pending, lets add them here*/
     
      Amount cost = 0;
	  Amount total = 1;
      Amount crystalAmount  = 0;
      std::string fungibleName = "";
      uint64_t uses = 0;
      Amount paidToDev;
      
      if(ParseCrystalPurchase(mrl, bundleKeyCode, cost, crystalAmount, name, paidToDev))
      {
          state.AddCrystalPurchase(*a, bundleKeyCode, crystalAmount, fighters, ctx.RoConfig ());
      }  
      
      auto& aState = state.GetXayaPlayerState (*a, fighters, ctx.RoConfig ());
      if(ParseGoodyPurchase(mrl, cost, name, fungibleName, aState.balance, uses))
      {
          state.AddPurchasing(*a, mrl["pg"].asString(), cost, fighters, ctx.RoConfig ());  
      }
      
      if(ParseSweetenerPurchase(mrl, cost, name, fungibleName, aState.balance, total))
      {
          state.AddPurchasing(*a, mrl["ps"].asString(), cost, fighters, ctx.RoConfig ());
      }  
      
      std::map<std::string, uint64_t> fungibles;
      if(ParseGoodyBundlePurchase(mrl, cost, name, fungibles, aState.balance))
      {
          state.AddPurchasing(*a, mrl["pgb"].asString(), cost, fighters, ctx.RoConfig ());
      }  
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

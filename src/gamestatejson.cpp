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

#include "gamestatejson.hpp"

#include "database/xayaplayer.hpp"
#include "database/fighter.hpp"
#include "database/recipe.hpp"
#include "database/globaldata.hpp"
#include "database/specialtournament.hpp"
#include "database/reward.hpp"
#include "database/activity.hpp"
#include "database/moneysupply.hpp"

#include "jsonutils.hpp"


#include <algorithm>

namespace pxd
{
template <typename T, typename R>
  Json::Value
  GameStateJson::ResultsAsArray(T& tbl, Database::Result<R> res) const
{
  Json::Value arr(Json::arrayValue);

  while (res.Step ())
    {
      const auto h = tbl.GetFromResult (res, ctx.RoConfig ());
      arr.append (Convert (*h));
    }

  return arr;
}    

template <>
  Json::Value
  GameStateJson::Convert<RecipeInstance>(const RecipeInstance& recipe) const
{
  const auto& pb = recipe.GetProto ();
  Json::Value res(Json::objectValue);
  
  res["owner"] = recipe.GetOwner ();
  res["authoredid"] = pb.authoredid ();
  res["did"] = recipe.GetId ();
  
  if(pb.authoredid () == "generated")
  {
    res["animationid"] = pb.animationid ();
    res["name"] = pb.name ();
    res["duration"] = pb.duration ();
    res["fightername"] = pb.fightername ();
    res["fightertype"] = pb.fightertype ();
    res["quality"] = pb.quality ();
    res["isaccountbound"] = pb.isaccountbound ();
    res["requiredfighterquality"] = pb.requiredfighterquality ();
    
    Json::Value armorD(Json::arrayValue);
    for(auto& armor: pb.armor())
    {
       Json::Value arm(Json::objectValue);
       arm["candytype"] = armor.candytype();
       arm["armortype"] = armor.armortype();
       armorD.append (arm);       
    }
    res["armor"] = armorD;
    
    Json::Value moves(Json::arrayValue);
    for(auto& move: pb.moves())
    {
       moves.append (move);       
    }
    res["moves"] = moves;   

    Json::Value rcandy(Json::arrayValue);
    for(auto& candy: pb.requiredcandy())
    {
       Json::Value cd(Json::objectValue);
       cd["candytype"] = candy.candytype();
       cd["amount"] = candy.amount();
       rcandy.append (cd);       
    }
    res["requiredcandy"] = rcandy;
  }

  return res;
} 

template <>
  Json::Value
  GameStateJson::Convert<RewardInstance>(const RewardInstance& reward) const
{
  const auto& pb = reward.GetProto ();
  Json::Value res(Json::objectValue);
  
  res["owner"] = reward.GetOwner ();
  res["tournamentid"] = pb.tournamentid();
  res["expeditionid"] = pb.expeditionid();
  res["sweetenerid"] = pb.sweetenerid();
  res["generatedrecipeid"] = pb.generatedrecipeid();
  res["rewardid"] = pb.rewardid();
  res["positionintable"] = pb.positionintable();
  res["rid"] = reward.GetId();
  res["fighterid"] = pb.fighterid();
  
  return res;
} 

template <>
  Json::Value
  GameStateJson::Convert<FighterInstance>(const FighterInstance& fighter) const
{
  const auto& pb = fighter.GetProto ();
  Json::Value res(Json::objectValue);
  
  res["owner"] = fighter.GetOwner ();
  res["status"] = (int)fighter.GetStatus();
  res["recipeid"] = pb.recipeid();
  res["name"] = pb.name();
  res["isaccountbound"] = pb.isaccountbound ();
  res["id"] = fighter.GetId();
  res["expeditioninstanceid"] = pb.expeditioninstanceid();
  res["tournamentinstanceid"] = IntToJson (pb.tournamentinstanceid());
  res["fightertypeid"] = pb.fightertypeid();
  res["quality"] = IntToJson (pb.quality());
  res["rating"] = IntToJson (pb.rating());
  res["sweetness"] = IntToJson (pb.sweetness());
  res["highestappliedsweetener"] = IntToJson (pb.highestappliedsweetener());
  
  res["specialtournamentinstanceid"] = IntToJson (pb.specialtournamentinstanceid());
  res["specialtournamentstatus"] = IntToJson (pb.specialtournamentstatus());
  res["tournamentpoints"] = IntToJson (pb.tournamentpoints());
  res["lasttournamenttime"] = IntToJson (pb.lasttournamenttime());
  
  Json::Value moves(Json::arrayValue);
  
  for (int i = 0; i < pb.moves_size (); ++i)
  {
      const auto& data = pb.moves (i);

      Json::Value move(Json::objectValue);
      move["authoredid"] = data;
      moves.append (move);
  } 
 
  res["moves"] = moves;
  
  Json::Value armorPieces(Json::arrayValue);
  
  for (int i = 0; i < pb.armorpieces_size (); ++i)
  {
      const auto& data2 = pb.armorpieces (i);

      Json::Value armor(Json::objectValue);
      armor["candy"] = data2.candy();
      armor["armortype"] = IntToJson (data2.armortype());
      armor["rewardsource"] = IntToJson (data2.rewardsource());
      armor["rewardsourceid"] = data2.rewardsourceid();
      
      armorPieces.append (armor);
  }  
  
  res["armorpieces"] = armorPieces;
  res["animationid"] = pb.animationid();
  
  // If fighter is participating in any activity, front-end needs 'blocksleft' information for that
  res["blocksleft"] = IntToJson (0);
  
  XayaPlayersTable xayaplayers(db);
  auto a = xayaplayers.GetByName (fighter.GetOwner (), ctx.RoConfig ());
  for (auto it = a->GetProto().ongoings().begin(); it != a->GetProto().ongoings().end(); it++)
  {
      if(it->fighterdatabaseid() == fighter.GetId())
      { 
        res["blocksleft"] = IntToJson (it->blocksleft());        
      }
  }
  
  res["exchangeexpire"] = IntToJson (pb.exchangeexpire());
  res["exchangeprice"] = IntToJson (pb.exchangeprice());
  
  a.reset();

  if(pb.tournamentinstanceid() > 0)
  {
    TournamentTable tournamentsDatabase(db);
    auto tnm = tournamentsDatabase.GetById (pb.tournamentinstanceid(), ctx.RoConfig ());
    res["blocksleft"] = IntToJson (tnm->GetInstance().blocksleft());
    tnm.reset();
  }

  Json::Value deconstructsArray(Json::arrayValue);
  
  RewardsTable tbl(db);
  auto ourRewards = tbl.QueryForOwner(fighter.GetOwner ());
  bool stepLoop = ourRewards.Step ();
  while (stepLoop)
  {
      auto h = tbl.GetFromResult (ourRewards);
      
      if(h->GetProto().fighterid() == fighter.GetId() && h->GetProto().deconstructions_size() > 0)
      {
          for(const auto& dcdata: h->GetProto().deconstructions())
          {
              Json::Value dcObj(Json::objectValue);
              dcObj["candytype"] = dcdata.candytype();
              dcObj["quantity"] = dcdata.quantity();
              deconstructsArray.append(dcObj);
          }
      }
      
	  h.reset();
      stepLoop = ourRewards.Step ();
  }  
  
  res["deconstructions"] = deconstructsArray;
  
  Json::Value salesHistory(Json::arrayValue);
  
  for (int i = 0; i < pb.salehistory_size (); ++i)
  {
      const auto& data = pb.salehistory (i);

      Json::Value sale(Json::objectValue);
      sale["selltime"] = IntToJson (data.selltime());
      sale["fromowner"] = data.fromowner();
      sale["toowner"] = data.toowner();
      sale["price"] = IntToJson (data.price());
      salesHistory.append (sale);
  }   

  res["saleshistory"] = salesHistory;

  return res;
} 
 
template <>
  Json::Value
  GameStateJson::Convert<ActivityInstance>(const ActivityInstance& activity) const
{
  const auto& pb = activity.GetProto ();
  Json::Value res(Json::objectValue);
  
  res["state"] = IntToJson (pb.state());
  res["start_block"] = IntToJson (pb.startblock());
  res["duration"] = IntToJson (pb.duration());
  res["name"] = pb.name();
  res["owner"] = pb.owner();
  res["related_item_GUID"] = pb.relateditemguid();
  res["related_item_or_class_id"] = pb.relateditemorclassid();
  
  return res;
}  
 
template <>
  Json::Value
  GameStateJson::Convert<SpecialTournamentInstance>(const SpecialTournamentInstance& tournament) const
{
  const auto& pb = tournament.GetProto ();
  Json::Value res(Json::objectValue);
  
  res["id"] = tournament.GetId();
  res["tier"] = IntToJson (pb.tier());
  res["crownholder"] = pb.crownholder();
  res["state"] = IntToJson (pb.state());
  
  Json::Value matchresults(Json::arrayValue);
  
  for(const auto& lastdaymatch: pb.lastdaymatchresults())
  {
     Json::Value rslt(Json::objectValue);
     
     rslt["attacker"] = lastdaymatch.attacker();
     rslt["defender"] = lastdaymatch.defender();
     rslt["attackerpoints"] = IntToJson (lastdaymatch.attackerpoints());
     rslt["defenderpoints"] = IntToJson (lastdaymatch.defenderpoints());
     
     matchresults.append(rslt); 
  }
  
  Json::Value defendersarray(Json::arrayValue);
 
  FighterTable ftbl(db);  
  auto resDD = ftbl.QueryAll();
  while (resDD.Step ())
  {
      auto c = ftbl.GetFromResult (resDD, ctx.RoConfig ());
      
      if(c->GetProto().specialtournamentinstanceid() == tournament.GetId() && c->GetOwner() == pb.crownholder())
      {
         defendersarray.append(c->GetId());  
      }
	  
	  c.reset();
  } 
  
  res["ldresults"] = matchresults;
  res["defenders"] = defendersarray;
  
  GlobalData gd(db);
    
  //int64_t currentTime = ctx.Timestamp();
  int64_t lastTournamentTime = gd.GetLastTournamentTime();
  //int64_t timeDiff = currentTime - lastTournamentTime;  
    
  res["lasttournamenttile"] = IntToJson (lastTournamentTime);
  
  return res;
}   
 
template <>
  Json::Value
  GameStateJson::Convert<TournamentInstance>(const TournamentInstance& tournament) const
{
  const auto& pb = tournament.GetProto ();
  Json::Value res(Json::objectValue);
  
  res["tid"] = tournament.GetId();
  res["state"] = IntToJson (tournament.GetInstance().state());
  res["winnerid"] = tournament.GetInstance().winnerid();
  res["blocksleft"] = IntToJson (tournament.GetInstance().blocksleft());
  res["blueprint"] = pb.authoredid();
  res["teamsjoined"] = IntToJson (tournament.GetInstance().teamsjoined());

  Json::Value fighters(Json::arrayValue);
  
  for(const auto ft: tournament.GetInstance().fighters())
  {
     fighters.append(ft); 
  }
  
  res["fighters"] = fighters;
  
  Json::Value tResults(Json::arrayValue);
  
  for(const auto& result: tournament.GetInstance().results())
  {
     Json::Value rslt(Json::objectValue);
     
     rslt["fighterid"] = IntToJson (result.fighterid());
     rslt["wins"] = IntToJson (result.wins());
     rslt["draws"] = IntToJson (result.draws());
     rslt["losses"] = IntToJson (result.losses());
     rslt["ratingdelta"] = IntToJson (result.ratingdelta());
     
     tResults.append(rslt); 
  } 
  
  res["results"] = tResults;
  
  return res;
}   
 
template <>
  Json::Value
  GameStateJson::Convert<Inventory>(const Inventory& inv) const
{
  Json::Value fungible(Json::objectValue);
  for (const auto& entry : inv.GetFungible ())
    fungible[entry.first] = IntToJson (entry.second);

  Json::Value res(Json::objectValue);
  res["fungible"] = fungible;

  return res;
} 
 
template <>
  Json::Value
  GameStateJson::Convert<XayaPlayer>(const XayaPlayer& a) const
{
  const auto& pb = a.GetProto ();

  Json::Value res(Json::objectValue);
  res["name"] = a.GetName ();
  res["minted"] = IntToJson (pb.burnsale_balance ());

  Json::Value bal(Json::objectValue);
  bal["available"] = IntToJson (a.GetBalance ());
  res["balance"] = bal;
  res["role"] = PlayerRoleToString (a.GetRole ());

  res["inventory"] = Convert (a.GetInventory ());
  
  Json::Value recJsonObj(Json::arrayValue);
  RecipeInstanceTable recepiesTbl(db);
  auto ourRecepies = recepiesTbl.QueryForOwner(a.GetName ());
  
  bool stepLoop = ourRecepies.Step ();
  while (stepLoop)
  {
      const auto h = recepiesTbl.GetFromResult (ourRecepies);
      recJsonObj.append (h->GetId());
      stepLoop = ourRecepies.Step ();
  }
  
  // Becase for ONGOING_COOKING we set recepie owner to "",
  // here we need to add those additionally, as else they
  // will be missing from the front-end roster until fighter
  // is cooked
  
  for(auto& ongoing: pb.ongoings())
  {
    if((pxd::OngoingType)ongoing.type() == pxd::OngoingType::COOK_RECIPE)
    {
       recJsonObj.append (ongoing.recipeid()); 
    }
  }
  
  for(auto& ongoing: pb.ongoings())
  {
    if((pxd::OngoingType)ongoing.type() == pxd::OngoingType::COOK_SWEETENER)
    {
       recJsonObj.append (ongoing.recipeid()); 
    }
  }  

  res["recepies"] = recJsonObj;
    
  Json::Value ongoingOps(Json::arrayValue);
  
  for(auto& ongoing: pb.ongoings())
  {
      Json::Value ongOB(Json::objectValue);
      ongOB["type"] = ongoing.type();
      ongOB["blocksleft"] = IntToJson(ongoing.blocksleft());
      
      if((pxd::OngoingType)(int)ongoing.type() == pxd::OngoingType::COOK_RECIPE)
      {
        ongOB["recipeid"] = ongoing.recipeid();
      }
      
      if((pxd::OngoingType)(int)ongoing.type() == pxd::OngoingType::COOK_SWEETENER)
      {
        ongOB["recipeid"] = ongoing.recipeid();
      }      
      
      if((pxd::OngoingType)(int)ongoing.type() == pxd::OngoingType::EXPEDITION)
      {
        ongOB["fighterdatabaseid"] = ongoing.fighterdatabaseid();
        ongOB["expeditionblueprintid"] = ongoing.expeditionblueprintid();
      }
      
      ongoingOps.append(ongOB);
  }
  
  res["ongoings"] = ongoingOps;
  res["prestige"] = IntToJson (a.GetPresitge());
  
  res["valueepic"] = IntToJson (pb.valueepic ());
  res["valuerare"] = IntToJson (pb.valuerare ());
  res["valueuncommon"] = IntToJson (pb.valueuncommon ());
  res["valuecommon"] = IntToJson (pb.valuecommon ());
  res["fighteraverage"] = IntToJson (pb.fighteraverage ());
  res["tournamentperformance"] = IntToJson (pb.tournamentperformance ());
  res["specialtournamentprestigetier"] = IntToJson (pb.specialtournamentprestigetier ());
  res["address"] = pb.address ();
  

  return res;
}

Json::Value
GameStateJson::MoneySupply()
{
  const auto& params = ctx.RoConfig ()->params ();
  pxd::MoneySupply ms(db);

  Amount total = 0;
  Json::Value entries(Json::objectValue);
  for (const auto& key : ms.GetValidKeys ())
    {
      if (key == "gifted" && !params.god_mode ())
        {
          CHECK_EQ (ms.Get (key), 0);
          continue;
        }

      const Amount value = ms.Get (key);
      entries[key] = IntToJson (value);
      total += value;
    }

  Json::Value burnsale(Json::arrayValue);
  Amount burnsaleAmount = ms.Get ("burnsale");
  for (int i = 0; i < params.burnsale_stages_size (); ++i)
    {
      const auto& data = params.burnsale_stages (i);
      const Amount alreadySold = std::min<Amount> (burnsaleAmount,
                                                   data.amount_sold ());
      burnsaleAmount -= alreadySold;

      Json::Value stage(Json::objectValue);
      stage["stage"] = IntToJson (i + 1);
      stage["price"] = static_cast<double> (data.price_sat ()) / COIN;
      stage["total"] = IntToJson (data.amount_sold ());
      stage["sold"] = IntToJson (alreadySold);
      stage["available"] = IntToJson (data.amount_sold () - alreadySold);

      burnsale.append (stage);
    }
  CHECK_EQ (burnsaleAmount, 0);

  Json::Value res(Json::objectValue);
  res["total"] = IntToJson (total);
  res["entries"] = entries;
  res["burnsale"] = burnsale;

  return res;
}

Json::Value
GameStateJson::XayaPlayers()
{
  XayaPlayersTable tbl(db);
  Json::Value res = ResultsAsArray (tbl, tbl.QueryAll ());
  return res;
}

Json::Value
GameStateJson::Activities()
{
  ActivityTable tbl(db);
  Json::Value res = ResultsAsArray (tbl, tbl.QueryAll ());

  return res;
}

Json::Value
GameStateJson::Rewards()
{
  RewardsTable tbl(db);
  Json::Value res = ResultsAsArray (tbl, tbl.QueryAll ());

  return res;
}

Json::Value
GameStateJson::Fighters()
{
  FighterTable tbl(db);
  Json::Value res = ResultsAsArray (tbl, tbl.QueryAll ());

  return res;
}

Json::Value
GameStateJson::Recepies()
{
  RecipeInstanceTable tbl(db);
  Json::Value res = ResultsAsArray (tbl, tbl.QueryAll ());

  return res;
}

Json::Value
GameStateJson::Tournaments()
{
  TournamentTable tbl(db);
  Json::Value res = ResultsAsArray (tbl, tbl.QueryAll ());

  return res;
}

Json::Value
GameStateJson::SpecialTournaments()
{
  SpecialTournamentTable tbl(db);
  Json::Value res = ResultsAsArray (tbl, tbl.QueryAll ());

  return res;
}

Json::Value
GameStateJson::CrystalBundles()
{
  const auto& bundles = ctx.RoConfig ()->crystalbundles();
  
  /*For testing purpose, we want to have constant hash of full state 
   string, hence its imporant to keep sorting order here deterministic*/
  
  std::vector<std::pair<std::string, pxd::proto::CrystalBundle>> sortedCrystalBundle;
  for (auto itr = bundles.begin(); itr != bundles.end(); ++itr)
      sortedCrystalBundle.push_back(*itr);

  sort(sortedCrystalBundle.begin(), sortedCrystalBundle.end(), [=](std::pair<std::string, pxd::proto::CrystalBundle>& a, std::pair<std::string, pxd::proto::CrystalBundle>& b)
  {
      return a.first < b.first;
  } 
  );    
  
  Json::Value res(Json::arrayValue);
  
  for (auto& bundle: sortedCrystalBundle)
  {
      res.append(bundle.first);
  }

  return res;
}
 
Json::Value
GameStateJson::User(const std::string& userName)
{
  Json::Value res(Json::objectValue);
  
  XayaPlayersTable xayaplayers(db);
  Json::Value res5 = ResultsAsArray (xayaplayers, xayaplayers.QueryForOwner (userName));
  res["xayaplayers"] = res5;
    
  FighterTable tbl2(db);
  Json::Value res2 = ResultsAsArray (tbl2, tbl2.QueryForOwner (userName)); 
  res["fighters"] = res2;  

  RecipeInstanceTable tbl3(db);
  Json::Value res3 = ResultsAsArray (tbl3, tbl3.QueryForOwner (userName)); 
  res["recepies"] = res3;
  
  RewardsTable tbl4(db);
  Json::Value res4 = ResultsAsArray (tbl4, tbl4.QueryForOwner (userName));  
  res["rewards"] = res4;
  
  res["statehex"] = latestKnownStateHash; 
  res["stateblock"] = latestKnownStateBlock;  
  
  // Additionally, we want to attach all the player ratings
  
  auto res2x = xayaplayers.QueryAll ();
  
  Json::Value ratings(Json::arrayValue);
  
  while (res2x.Step ())
  {
      auto pl = xayaplayers.GetFromResult (res2x, ctx.RoConfig ());
	  
	  Json::Value ratingData(Json::objectValue);	  
	  ratingData["name"] = pl->GetName();
	  ratingData["prestige"] = IntToJson (pl->GetPresitge());	  
      ratings.append(ratingData);  
  }

  res["ratings"] = ratings;

  return res;
} 

Json::Value
GameStateJson::UserTournaments(const std::string& userName)
{
  Json::Value res(Json::objectValue);
  
  res["specialtournaments"] = SpecialTournaments();
  
  // We do not want all the tournaments here
  // We want only open tournaments, then tournaments which are running
  // Lastly we want complete, but only if we have any rewards in them
  
  TournamentTable tbl(db);
  FighterTable ftbl(db); 
  RewardsTable rtbl(db);
  
  Json::Value arr(Json::arrayValue);
  Json::Value fghtrarr(Json::arrayValue);
  
  std::vector<int32_t> collectedFightersIds;
  
  auto res2 = tbl.QueryAll ();
  
  while (res2.Step ())
  {
      auto h = tbl.GetFromResult (res2, ctx.RoConfig ());
	  
	  if(h->GetInstance().state() == 0)
	  {
		arr.append (Convert<TournamentInstance> (*h));
		
		  // Additionally, we want to supply all the relevant fighters
		  const auto ftrs = h->GetInstance().fighters();
		  h.reset();
          for(const auto ft: ftrs)
          { 
              auto resDD = ftbl.GetById (ft, ctx.RoConfig ());
              if(resDD != nullptr) // was destroyed at some point of the game?
			  {
				  // We ignore user fighters as we get those from User() anyway, also
				  // lets make sure we are filtering for duplictaes, we there is possibility
				  // of duplicating, and we really don't want this to happen at all costs
				  int32_t fID = resDD->GetId();
				  if(resDD->GetOwner() != userName)
				  {
					if (std::find(collectedFightersIds.begin(), collectedFightersIds.end(), fID) != collectedFightersIds.end()) 
					{
						// ignore duplicate
					}					
					else
					{
					 fghtrarr.append (Convert<FighterInstance> (*resDD));  
					 collectedFightersIds.push_back(fID);
					}
				  }	
				  
				  resDD.reset();
			  }
		  }			  
	  }
	  else if(h->GetInstance().state() == 1 || h->GetInstance().state() == 2)
	  {
		  bool hasAnyOfOurFighters = false;
		  
		  for(const auto ft: h->GetInstance().fighters())
          { 
              auto resDD = ftbl.GetById (ft, ctx.RoConfig ());

              if(resDD->GetOwner() == userName)
			  {
				  hasAnyOfOurFighters = true;
			  }

              resDD.reset();			  
			  
			  if(hasAnyOfOurFighters) break;
	      }
		  
		  if(hasAnyOfOurFighters)
		  {
              arr.append (Convert<TournamentInstance> (*h));	

			  // Additionally, we want to supply all the relevant fighters
			  const auto ftrs = h->GetInstance().fighters();
			  h.reset();
			  for(const auto ft: ftrs)
			  { 
				  auto resDD = ftbl.GetById (ft, ctx.RoConfig ());
                  if(resDD != nullptr) // was destroyed at some point of the game?
			      {
					  // We ignore user fighters as we get those from User() anyway, also
					  // lets make sure we are filtering for duplictaes, we there is possibility
					  // of duplicating, and we really don't want this to happen at all costs
					  int32_t fID = resDD->GetId();
					  if(resDD->GetOwner() != userName)
					  {
						if (std::find(collectedFightersIds.begin(), collectedFightersIds.end(), fID) != collectedFightersIds.end()) 
						{
							// ignore duplicate
						}					
						else
						{
						 fghtrarr.append (Convert<FighterInstance> (*resDD));  
						 collectedFightersIds.push_back(fID);
						}
					  }	
					  
					  resDD.reset();
				  }
			  }			 
		  }
		  else
		  {
			   h.reset();
		  }
		  
	  }		
	  else if(h->GetInstance().state() == 3)
	  {
		  bool hasAnyOfOurFighters = false;
		  
		  // Additionally, we want to supply all the relevant fighters
		  const auto ftrs = h->GetInstance().fighters();
		  auto converted = Convert<TournamentInstance> (*h);
		  int64_t idT = h->GetId();
		  h.reset();
          for(const auto ft: ftrs)
          { 	  
              auto resDD = ftbl.GetById (ft, ctx.RoConfig ());

			  if(resDD != nullptr) // was destroyed at some point of the game?
			  {
				  if(resDD->GetOwner() == userName)
				  {
					  // Lets additionally check, we did claim all the tournament rewards already?
					  bool allRewardsAreClaimed = true;
								  
					  auto ourRewards = rtbl.QueryForOwner(userName);
					  bool stepLoop = ourRewards.Step ();
					  while (stepLoop)
					  {
						 auto h2 = rtbl.GetFromResult (ourRewards);	
						 
						 if(h2->GetProto().tournamentid() == idT)
						 {
							 allRewardsAreClaimed = false;
						 }
						 
						 h2.reset();	
						 stepLoop = ourRewards.Step ();
					  }		

					  if(allRewardsAreClaimed == false) hasAnyOfOurFighters = true; 				  
				  }

				  resDD.reset();			 
			  }			  
			  
			  if(hasAnyOfOurFighters) break;
	      }
		  
		  if(hasAnyOfOurFighters)
		  {
              arr.append (converted);		
			  // Additionally, we want to supply all the relevant fighters
			  for(const auto ft: ftrs)
			  { 
				  auto resDD = ftbl.GetById (ft, ctx.RoConfig ());
                  if(resDD != nullptr) // was destroyed at some point of the game?
			      {
					  // We ignore user fighters as we get those from User() anyway, also
					  // lets make sure we are filtering for duplictaes, we there is possibility
					  // of duplicating, and we really don't want this to happen at all costs
					  int32_t fID = resDD->GetId();
					  if(resDD->GetOwner() != userName)
					  {
						if (std::find(collectedFightersIds.begin(), collectedFightersIds.end(), fID) != collectedFightersIds.end()) 
						{
							// ignore duplicate
						}					
						else
						{
						 fghtrarr.append (Convert<FighterInstance> (*resDD));  
						 collectedFightersIds.push_back(fID);
						}
					  }	
					  
					  resDD.reset();
				  }
		      }			 
		  }		  
	  }	
  };  
  
  // Additionally, lets feed all the special tournament defenders
  
  auto resDDFT = ftbl.QueryAll();
  while (resDDFT.Step ())
  {
      auto c = ftbl.GetFromResult (resDDFT, ctx.RoConfig ());
      int32_t fIDD = c->GetId();
	  
      if(c->GetProto().specialtournamentinstanceid() > 0)
      {
        if (std::find(collectedFightersIds.begin(), collectedFightersIds.end(), fIDD) != collectedFightersIds.end()) 
		{
			// ignore duplicate
		}					
		else
		{		  
         fghtrarr.append(Convert<FighterInstance> (*c));  
		 collectedFightersIds.push_back(fIDD);
		}
      }
	  
	  c.reset();
  } 
      
  res["tournaments"] = arr;
  res["fighters"] = fghtrarr;

  return res;
} 

Json::Value
GameStateJson::Exchange()
{
  Json::Value res(Json::objectValue);
  Json::Value fghtrarr(Json::arrayValue);
  // We identify fighters listed on exchange based on their status
  
  FighterTable tbl2(db);
  
  auto res2 = tbl2.QueryAll ();
  
  while (res2.Step ())
  {
     auto h = tbl2.GetFromResult (res2, ctx.RoConfig ()); 

     if(h->GetStatus() == FighterStatus::Exchange)
	 {
          fghtrarr.append (Convert<FighterInstance> (*h));				 
	 }
	 
	 h.reset();
  }  
   
  res["fighters"] = fghtrarr;     
  return res;
}
 
Json::Value
GameStateJson::FullState()
{
  Json::Value res(Json::objectValue);

  res["xayaplayers"] = XayaPlayers();
  res["activities"] = Activities();
  res["crystalbundles"] = CrystalBundles();
  res["fighters"] = Fighters();
  res["rewards"] = Rewards();
  res["recepies"] = Recepies();
  res["tournaments"] = Tournaments();
  res["specialtournaments"] = SpecialTournaments();
  
  res["statehex"] = latestKnownStateHash; 
  res["stateblock"] = latestKnownStateBlock;
  
  return res;
}  

} // namespace pxd

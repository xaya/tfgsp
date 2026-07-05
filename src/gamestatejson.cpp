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
#include "database/reward.hpp"
#include "database/ongoings.hpp"

#include "jsonutils.hpp"


#include <algorithm>

namespace pxd
{

namespace
{

/* Returns the OngoinOperation proto from either a bare proto or a
   (height, proto) pair, so AppendInProgressCookRecipeIds can be shared by the
   two ongoing-collection shapes used in this file. */
inline const proto::OngoinOperation&
OngoingProtoOf (const proto::OngoinOperation& op)
{
  return op;
}
inline const proto::OngoinOperation&
OngoingProtoOf (const std::pair<unsigned, proto::OngoinOperation>& op)
{
  return op.second;
}

/* Appends the in-progress cook recipe ids of the given ongoing operations to
   arr: first every COOK_RECIPE id, then every COOK_SWEETENER id, preserving
   the source order the front-end roster relies on. */
template <typename Ongoings>
  void
  AppendInProgressCookRecipeIds (Json::Value& arr, const Ongoings& ongoings)
{
  for (const auto& ong : ongoings)
    if ((pxd::OngoingType) OngoingProtoOf (ong).type () == pxd::OngoingType::COOK_RECIPE)
      arr.append (OngoingProtoOf (ong).recipeid ());
  for (const auto& ong : ongoings)
    if ((pxd::OngoingType) OngoingProtoOf (ong).type () == pxd::OngoingType::COOK_SWEETENER)
      arr.append (OngoingProtoOf (ong).recipeid ());
}

} // anonymous namespace
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
	
	res["firstnamerarity"] = IntToJson (pb.firstnamerarity());
    res["secondnamerarity"] = IntToJson (pb.secondnamerarity());
    
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

  res["firstnamerarity"] = IntToJson (pb.firstnamerarity());
  res["secondnamerarity"] = IntToJson (pb.secondnamerarity());
  
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
  
  /* H3: blocksleft is now DERIVED from the ongoing_operations row's absolute
     resolve height minus the current height. */
  {
    OngoingsTable ongoings(db);
    auto ores = ongoings.QueryForOwner (fighter.GetOwner ());
    while (ores.Step ())
    {
      auto op = ongoings.GetFromResult (ores);
      if(op->GetProto().fighterdatabaseid() == fighter.GetId())
      {
        /* The height-less full-state dump (getcurrentstate) has no current
           height to subtract; emit 0 there rather than tripping the NO_HEIGHT
           guard.  Every height-bearing path reports the real remaining blocks. */
        res["blocksleft"] = IntToJson (ctx.HasHeight ()
            ? (int) op->GetHeight () - (int) ctx.Height () : 0);
      }
    }
  }

  res["exchangeexpire"] = IntToJson (pb.exchangeexpire());
  res["exchangeprice"] = IntToJson (pb.exchangeprice());

  if(pb.tournamentinstanceid() > 0)
  {
    TournamentTable tournamentsDatabase(db);
    auto tnm = tournamentsDatabase.GetById (pb.tournamentinstanceid(), ctx.RoConfig ());
    /* Guard the row lookup: a serialization read must never crash the daemon if a
       fighter references a tournament row that has already been pruned. */
    if(tnm != nullptr)
    {
      res["blocksleft"] = IntToJson (tnm->GetInstance().blocksleft());
      tnm.reset();
    }
  }

  /* Deconstruction candy now credits straight into inventory at resolve (no
     reward row, no held deconstruction data), so there is nothing per-fighter to
     serialize here -- the old rewards-table scan is dead and has been removed. */

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

  Json::Value bal(Json::objectValue);
  bal["available"] = IntToJson (a.GetBalance ());
  res["balance"] = bal;

  /* Append-only reward counter: the client watches this advance to know new
     activity rewards were auto-credited (no claim tx / unclaimed rows to read). */
  res["rewardsserial"] = IntToJson (pb.rewards_serial ());

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
  
  /* H3: ongoings now live in the height-keyed ongoing_operations table; fetch
     this player's rows once (QueryForOwner is ORDER BY id == insertion order, so
     the JSON ordering and the FullState hash stay stable). */
  std::vector<std::pair<unsigned, proto::OngoinOperation>> playerOngoings;
  {
    OngoingsTable ongoingsTbl(db);
    auto ores = ongoingsTbl.QueryForOwner (a.GetName ());
    while (ores.Step ())
    {
      auto op = ongoingsTbl.GetFromResult (ores);
      playerOngoings.push_back ({op->GetHeight (), op->GetProto ()});
    }
  }

  AppendInProgressCookRecipeIds (recJsonObj, playerOngoings);

  res["recepies"] = recJsonObj;

  Json::Value ongoingOps(Json::arrayValue);

  for(const auto& ong: playerOngoings)
  {
      const auto& ongoing = ong.second;
      Json::Value ongOB(Json::objectValue);
      ongOB["type"] = ongoing.type();
      ongOB["blocksleft"] = IntToJson (ctx.HasHeight ()
          ? (int) ong.first - (int) ctx.Height () : 0);

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
  res["address"] = pb.address ();
  

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
  
  // Becase for ONGOING_COOKING we set recepie owner to "",
  // here we need to add those additionally, as else they
  // will be missing from the front-end roster until fighter
  // is cooked
  
  // Also we want to recipes in rewards, because we want to
  // fetch their info as well for rewards screens
  
  std::vector<int> ourRecipesInRewards;
  
  RewardsTable tblRewards(db);
  auto ourRewards = tblRewards.QueryForOwner(userName);
  bool stepLoopRewards = ourRewards.Step ();
  while (stepLoopRewards)
  {
      auto hReward = tblRewards.GetFromResult (ourRewards); 
	  
	  if(hReward->GetProto().generatedrecipeid() > 0)
	  {
		  ourRecipesInRewards.push_back(hReward->GetProto().generatedrecipeid());
	  }
	  
	  hReward.reset();
	  stepLoopRewards = ourRewards.Step ();
  }
  
  
  auto a = xayaplayers.GetByName (userName, ctx.RoConfig ());
  
  Json::Value arrAllPotentialRecipes(Json::arrayValue);
  if(a != nullptr)
  {
	  /* H3: collect in-progress cook recipe ids from the ongoing_operations table. */
	  Json::Value recJsonObj(Json::arrayValue);
	  {
	    OngoingsTable ongoingsTbl(db);
	    auto ores = ongoingsTbl.QueryForOwner (a->GetName ());
	    std::vector<proto::OngoinOperation> ops;
	    while (ores.Step ())
	    {
	      ops.push_back (ongoingsTbl.GetFromResult (ores)->GetProto ());
	    }
	    AppendInProgressCookRecipeIds (recJsonObj, ops);
	  }

	  a.reset();
	  
	  RecipeInstanceTable tblRecipe(db);
	  auto recipeAllResults =  tblRecipe.QueryAll ();
	  while (recipeAllResults.Step ())
	  {
		  auto h = tblRecipe.GetFromResult (recipeAllResults, ctx.RoConfig ());
		  
		  if(h->GetOwner() == userName)
		  {
			 arrAllPotentialRecipes.append (Convert (*h));
		  }
		  else
		  {
			  int ID = h->GetId();
			  bool found = false;
			  for (Json::ArrayIndex i = 0; i < recJsonObj.size(); i++) {
				  if (recJsonObj[i].isInt() && recJsonObj[i].asInt() == ID) {
					found = true;
					break;
				  }
			  }		  
			  
			  if(found == false)
			  {
				   if (std::count(ourRecipesInRewards.begin(), ourRecipesInRewards.end(), ID))
				   {
					  found = true;
				   }
			  }
			  
			  if(found)
			  {
				 arrAllPotentialRecipes.append (Convert (*h)); 
			  }
		  }
		  
		  h.reset();
	  }
  }

  res["recepies"] = arrAllPotentialRecipes;
  
  RewardsTable tbl4(db);
  Json::Value res4 = ResultsAsArray (tbl4, tbl4.QueryForOwner (userName));  
  res["rewards"] = res4;

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

  GlobalData gd(db);
  res["version"] = gd.GetVersion();
  res["vanillaurl"] = gd.GetUrl();
  res["multiplier"] = IntToJson(gd.GetChiMultiplier());

  return res;
} 

Json::Value
GameStateJson::UserTournaments(const std::string& userName)
{
  Json::Value res(Json::objectValue);
  
  
  // We do not want all the tournaments here
  // We want only open tournaments, then tournaments which are running
  // Lastly we want complete, but only if we have any rewards in them
  
  TournamentTable tbl(db);
  FighterTable ftbl(db); 
  RewardsTable rtbl(db);
  
  Json::Value arr(Json::arrayValue);
  Json::Value fghtrarr(Json::arrayValue);
  
  std::vector<int32_t> collectedFightersIds;
  
  /* Collects a tournament's non-owned (foreign) fighters into fghtrarr,
     de-duplicating by id against collectedFightersIds; the querying user's
     own fighters are skipped (they come from User()).  Shared verbatim by
     all three tournament-state branches below. */
  const auto appendForeignFighters = [&] (const auto& ftrs)
  {
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
  };

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
          appendForeignFighters (ftrs);
	  }
	  else if(h->GetInstance().state() == 1 || h->GetInstance().state() == 2)
	  {
		  bool hasAnyOfOurFighters = false;
		  
		  for(const auto ft: h->GetInstance().fighters())
          { 
              auto resDD = ftbl.GetById (ft, ctx.RoConfig ());
              if(resDD == nullptr) continue; // fighter may have been destroyed

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
			  appendForeignFighters (ftrs);
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
			  appendForeignFighters (ftrs);
		  }		  
	  }	
  };  
  
  res["tournaments"] = arr;
  res["fighters"] = fghtrarr;

  return res;
} 

namespace
{

ExchangeSort
ParseExchangeSort (const Json::Value& v)
{
  const std::string s = v.isString () ? v.asString () : "";
  if (s == "quality")   return ExchangeSort::Quality;
  if (s == "sweetness") return ExchangeSort::Sweetness;
  if (s == "expire")    return ExchangeSort::Expire;
  return ExchangeSort::Price;   // default + unknown
}

int64_t
OptInt (const Json::Value& r, const char* key, int64_t dflt)
{
  return (r.isMember (key) && r[key].isInt ()) ? r[key].asInt64 () : dflt;
}

} // anonymous namespace

Json::Value
GameStateJson::Exchange (const Json::Value& request)
{
  const Json::Value r = request.isObject () ? request : Json::Value (Json::objectValue);

  ExchangeQuery q;
  q.sort = ParseExchangeSort (r["sort"]);
  q.ascending = !(r.isMember ("order") && r["order"].isString ()
                  && r["order"].asString () == "desc");
  q.minQuality    = OptInt (r, "minquality", -1);
  q.maxQuality    = OptInt (r, "maxquality", -1);
  q.minPrice      = OptInt (r, "minprice", -1);
  q.maxPrice      = OptInt (r, "maxprice", -1);
  q.maxAffordable = OptInt (r, "affordablefor", -1);
  if (r.isMember ("excludeowner") && r["excludeowner"].isString ())
    q.excludeOwner = r["excludeowner"].asString ();

  int64_t limit = OptInt (r, "limit", 50);
  q.limit = limit < 1 ? 1 : (limit > 100 ? 100 : limit);   // server-side hard cap
  const int64_t offset = OptInt (r, "offset", 0);
  q.offset = offset < 0 ? 0 : offset;

  FighterTable tbl (db);
  Json::Value fghtrarr (Json::arrayValue);
  {
    auto res = tbl.QueryExchange (q);
    while (res.Step ())
      {
        auto h = tbl.GetFromResult (res, ctx.RoConfig ());
        fghtrarr.append (Convert<FighterInstance> (*h));
        h.reset ();
      }
  } // close the page cursor before the COUNT statement

  Json::Value out (Json::objectValue);
  out["fighters"] = fghtrarr;
  out["total"]    = IntToJson (tbl.CountExchange (q));
  return out;
}
 
Json::Value
GameStateJson::FullState()
{
  Json::Value res(Json::objectValue);

  res["xayaplayers"] = XayaPlayers();
  res["crystalbundles"] = CrystalBundles();
  res["fighters"] = Fighters();
  res["rewards"] = Rewards();
  res["recepies"] = Recepies();
  res["tournaments"] = Tournaments();

  GlobalData gd(db);
  res["version"] = gd.GetVersion();
  res["vanillaurl"] = gd.GetUrl();
  res["multiplier"] = IntToJson(gd.GetChiMultiplier());
  
  return res;
}  

} // namespace pxd

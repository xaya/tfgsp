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
  GameStateJson::Convert<FighterInstance>(const FighterInstance& fighter) const
{
  const auto& pb = fighter.GetProto ();
  Json::Value res(Json::objectValue);
  
  res["owner"] = fighter.GetOwner ();
  res["recipeid"] = pb.recipeid();
  res["tournamentinstanceid"] = IntToJson (pb.tournamentinstanceid());
  res["fightertypeid"] = pb.fightertypeid();
  res["quality"] = IntToJson (pb.quality());
  res["rating"] = IntToJson (pb.rating());
  res["sweetness"] = IntToJson (pb.sweetness());
  res["highestappliedsweetener"] = IntToJson (pb.highestappliedsweetener());
  
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
      armor["rewardsourceid"] = IntToJson (data2.rewardsourceid());
      
      armorPieces.append (armor);
  }  
  
  res["armorpieces"] = armorPieces;
  res["animationid"] = pb.animationid();
  
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

  if (a.IsInitialised ())
  {
      res["role"] = PlayerRoleToString (a.GetRole ());
  }

  res["ftuestate"] = FTUEStateToString (a.GetFTUEState ());
  
  res["inventory"] = Convert (a.GetInventory ());

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
GameStateJson::Fighters()
{
  FighterTable tbl(db);
  Json::Value res = ResultsAsArray (tbl, tbl.QueryAll ());

  return res;
}

Json::Value
GameStateJson::CrystalBundles()
{
  const auto& bundles = ctx.RoConfig ()->crystalbundles();
  Json::Value res(Json::arrayValue);
  
  for (auto& bundle: bundles)
  {
      res.append(bundle.first);
  }

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
  
  return res;
}  

} // namespace pxd

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
#include "database/moneysupply.hpp"

#include "jsonutils.hpp"


#include <algorithm>

namespace pxd
{
    
template <typename T, typename R>
  Json::Value
  GameStateJson::ResultsAsArray (T& tbl, Database::Result<R> res) const
{
  Json::Value arr(Json::arrayValue);

  while (res.Step ())
    {
      const auto h = tbl.GetFromResult (res);
      arr.append (Convert (*h));
    }

  return arr;
}    
 
template <>
  Json::Value
  GameStateJson::Convert<XayaPlayer> (const XayaPlayer& a) const
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

  return res;
}

Json::Value
GameStateJson::MoneySupply ()
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
GameStateJson::XayaPlayers ()
{
  XayaPlayersTable tbl(db);
  Json::Value res = ResultsAsArray (tbl, tbl.QueryAll ());

  /* Add in also the Cubit balances reserved in open bids.  */
  //const int reserved = 0;
  
  for (auto& entry : res)
    {
      const auto& nmVal = entry["name"];
      CHECK (nmVal.isString ());
      
      //const auto mit = reserved.find (nmVal.asString ());
      Amount cur;
     // if (mit == reserved.end ())
        cur = 0;
     // else
        //cur = mit->second;

      auto& bal = entry["balance"];
      CHECK (bal.isObject ());
      bal["reserved"] = IntToJson (cur);
      bal["total"] = IntToJson (cur + bal["available"].asInt64 ());
    }

  return res;
}
 
Json::Value
GameStateJson::FullState ()
{
  Json::Value res(Json::objectValue);

  res["xayaplayers"] = XayaPlayers();

  return res;
}  

} // namespace pxd

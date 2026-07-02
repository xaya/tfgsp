/*
    GSP for the tf blockchain game
    Copyright (C) 2019-2020  Autonomous Worlds Ltd

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

#include "pxrpcserver.hpp"

#include "config.h"


#include "jsonutils.hpp"
#include "moveprocessor.hpp"


#include "proto/config.pb.h"

#include <xayagame/gamerpcserver.hpp>
#include <xayagame/sqliteintro.hpp>

#include <xayautil/hash.hpp>
#include <xayautil/uint256.hpp>

#include <glog/logging.h>
#include <jsonrpccpp/common/exception.h>

#include <limits>
#include <sstream>
#include <string>

namespace pxd
{

/* ************************************************************************** */

void
PXRpcServer::ThrowJsonError (ErrorCode code, const std::string& msg)
{
  throw jsonrpc::JsonRpcException (static_cast<int> (code), msg);
}

void
PXRpcServer::EnsureUnsafeAllowed (const std::string& method)
{
  if (!unsafeMethods)
    {
      LOG (WARNING) << "Blocked unsafe '" << method << "' call";
      ThrowJsonError (ErrorCode::UNSAFE_METHOD,
                      "unsafe RPC methods are disabled in the server");
    }
}

void
PXRpcServer::EnableUnsafeMethods ()
{
  LOG (WARNING) << "Enabling unsafe RPC methods";
  unsafeMethods = true;
}

void
PXRpcServer::stop ()
{
  LOG (INFO) << "RPC method called: stop";
  EnsureUnsafeAllowed ("stop");
  game.RequestStop ();
}

Json::Value
PXRpcServer::getcurrentstate ()
{
  LOG (INFO) << "RPC method called: getcurrentstate";
  return game.GetCurrentJsonState ();
}

Json::Value
PXRpcServer::getnullstate ()
{
  LOG (INFO) << "RPC method called: getnullstate";
  return game.GetNullJsonState ();
}

Json::Value
PXRpcServer::getpendingstate ()
{
  LOG (INFO) << "RPC method called: getpendingstate";
  return game.GetPendingJsonState ();
}

Json::Value
PXRpcServer::getxayaplayers ()
{
  LOG (INFO) << "RPC method called: getxayaplayers";
  return logic.GetCustomStateData (game,
    [] (GameStateJson& gsj)
      {
		  
		Json::Value res(Json::objectValue);
        res["xayaplayers"] = gsj.XayaPlayers();  
		  
        return res;
      });
}

Json::Value
PXRpcServer::getuser (const std::string& userName)
{
  LOG (INFO) << "RPC method called: getuser";
  return logic.GetCustomStateData (game,
    [userName] (GameStateJson& gsj)
      {
        return gsj.User (userName);
      });
}

Json::Value
PXRpcServer::gettournaments (const std::string& userName)
{
  LOG (INFO) << "RPC method called: gettournaments";
  return logic.GetCustomStateData (game,
    [userName] (GameStateJson& gsj)
      {
        return gsj.UserTournaments (userName);
      });
}

Json::Value
PXRpcServer::getexchange ()
{
  LOG (INFO) << "RPC method called: getexchange";
  return logic.GetCustomStateData (game,
    [] (GameStateJson& gsj)
      {
        return gsj.Exchange ();
      });
}

Json::Value
PXRpcServer::waitforpendingchange (const int oldVersion)
{
  LOG (INFO) << "RPC method called: waitforpendingchange " << oldVersion;
  return game.WaitForPendingChange (oldVersion);
}

std::string
PXRpcServer::waitforchange (const std::string& knownBlock)
{
  LOG (INFO) << "RPC method called: waitforchange " << knownBlock;
  return xaya::GameRpcServer::DefaultWaitForChange (game, knownBlock);
}

Json::Value
PXRpcServer::getfueldata (const Json::Value& candiesNew, const Json::Value& candiesSubmited, const Json::Value& candylist, const Json::Value& fighterData, const Json::Value& fightersNew, const Json::Value& fightersSubmited, const Json::Value& recipeData, const Json::Value& recipesNew, const Json::Value& recipesSubmited)
{
  /* getfueldata is a pure client-side estimator fed entirely by untrusted RPC
     input.  EvaluateFuelList/CalculateFuelPower index those arrays with
     operator[]/asInt/asString without per-element type checks, so a malformed
     element (e.g. a non-object candy or a string fighter id) makes jsoncpp throw
     Json::LogicError.  jsonrpccpp only catches JsonRpcException, so an unguarded
     throw would unwind through the libmicrohttpd C callback and terminate the
     whole daemon.  Convert any such throw into a normal invalid-params error. */
  try
    {
      return BaseMoveProcessor::EvaluateFuelList(fightersSubmited, recipesSubmited, candiesSubmited, fightersNew, recipesNew, candiesNew, fighterData, recipeData, candylist);
    }
  catch (const Json::Exception& exc)
    {
      throw jsonrpc::JsonRpcException (
          jsonrpc::Errors::ERROR_RPC_INVALID_PARAMS,
          std::string ("malformed getfueldata input: ") + exc.what ());
    }
}


Json::Value
PXRpcServer::hashcurrentstate ()
{
  LOG (INFO) << "RPC method called: hashcurrentstate";
  EnsureUnsafeAllowed ("hashcurrentstate");
  return logic.GetCustomStateData (game, "data",
    [] (const xaya::SQLiteDatabase& db)
      {
        xaya::SHA256 h;
        xaya::WriteTables (h, db, xaya::GetSqliteTables (db));
        return h.Finalise ().ToHex ();
      });
}

Json::Value
PXRpcServer::getstatehash (const std::string& block)
{
  LOG (INFO) << "RPC method called: getstatehash " << block;
  if (hasher == nullptr)
    throw jsonrpc::JsonRpcException (
        jsonrpc::Errors::ERROR_RPC_METHOD_NOT_FOUND,
        "state hashing is not enabled (start with --statehash_interval=N)");

  xaya::uint256 blockHash;
  if (!blockHash.FromHex (block))
    throw jsonrpc::JsonRpcException (
        jsonrpc::Errors::ERROR_RPC_INVALID_PARAMS, "invalid block hash");

  return logic.GetCustomStateData (game, "data",
    [this, &blockHash] (const xaya::SQLiteDatabase& db) -> Json::Value
      {
        xaya::uint256 value;
        if (!hasher->GetHash (db, blockHash, value))
          return Json::Value ();
        return value.ToHex ();
      });
}

/* ************************************************************************** */

} // namespace pxd

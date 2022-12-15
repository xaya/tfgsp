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

#include "proto/config.pb.h"

#include <xayagame/gamerpcserver.hpp>

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
PXRpcServer::getmoneysupply ()
{
  LOG (INFO) << "RPC method called: getmoneysupply";
  return logic.GetCustomStateData (game,
    [] (GameStateJson& gsj)
      {
        return gsj.MoneySupply ();
      });
}

Json::Value
PXRpcServer::getxayaplayers ()
{
  LOG (INFO) << "RPC method called: getxayaplayers";
  return logic.GetCustomStateData (game,
    [] (GameStateJson& gsj)
      {
        return gsj.XayaPlayers ();
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



/* ************************************************************************** */

} // namespace pxd

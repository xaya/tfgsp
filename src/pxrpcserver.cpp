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

#include <limits>
#include <sstream>
#include <string>

namespace pxd
{

/* ************************************************************************** */

namespace
{


enum class ErrorCode
{

  /* Invalid values for arguments (e.g. passing a malformed JSON value for
     a HexCoord or an out-of-range integer.  */
  INVALID_ARGUMENT = -1,

  /* Non-existing account passed as associated name for some RPC.  */
  INVALID_ACCOUNT = -2,

  /* Specific errors with findpath.  */
  FINDPATH_NO_CONNECTION = 1,
  FINDPATH_ENCODE_FAILED = 4,

  /* Specific errors with getregionat.  */
  REGIONAT_OUT_OF_MAP = 2,

  /* Specific errors with getregions.  */
  GETREGIONS_FROM_TOO_LOW = 3,

};



} // anonymous namespace


void
PXRpcServer::stop ()
{
  LOG (INFO) << "RPC method called: stop";
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

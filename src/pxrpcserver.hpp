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

#ifndef PXD_PXRPCSERVER_HPP
#define PXD_PXRPCSERVER_HPP

#include "rpc-stubs/pxrpcserverstub.h"

#include "logic.hpp"

#include <xayagame/game.hpp>

#include <json/json.h>
#include <jsonrpccpp/server.h>

#include <map>
#include <mutex>

namespace pxd
{
    
/**
 * Fake JSON-RPC server connector that just does nothing.  By using this class,
 * we can construct instances of the libjson-rpc-cpp servers without needing to
 * really process requests with them (we'll just use it to call the actual
 * methods on it directly).
 */
class NullServerConnector : public jsonrpc::AbstractServerConnector
{

public:

  NullServerConnector () = default;

  bool
  StartListening () override
  {
    return false;
  }

  bool
  StopListening () override
  {
    return false;
  }

};    

/**
 * Implementation of the JSON-RPC interface to the game daemon.  This mostly
 * contains methods that query the game-state database in some way as needed
 * by the UI process.
 */
class PXRpcServer : public PXRpcServerStub
{

private:

  /** The underlying Game instance that manages everything.  */
  xaya::Game& game;

  /** The PX game logic implementation.  */
  PXLogic& logic;


public:

explicit PXRpcServer (xaya::Game& g, PXLogic& l,
                        jsonrpc::AbstractServerConnector& conn)
    : PXRpcServerStub(conn), game(g), logic(l)
  {}

  void stop () override;
  Json::Value getcurrentstate () override;
  Json::Value getnullstate () override;
  Json::Value getpendingstate () override;
  std::string waitforchange (const std::string& knownBlock) override;
  Json::Value waitforpendingchange (int oldVersion) override;
  
  Json::Value getxayaplayers () override;
  Json::Value getmoneysupply () override;

};

} // namespace pxd

#endif // PXD_PXRPCSERVER_HPP

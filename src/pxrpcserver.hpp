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
  /* This method is considered unsafe and not enabled in the server.  */
  UNSAFE_METHOD = -4,  

}; 
 
 
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
  
 /**
   * Whether or not to allow "unsafe" RPC methods (like stop, that should
   * not be publicly exposed).
   */
  bool unsafeMethods = false;

  /**
   * Checks if unsafe methods are allowed.  If not, throws a JSON-RPC
   * exception to the caller.
   */
  void EnsureUnsafeAllowed (const std::string& method);  

  /**
   * Throws a JSON-RPC error from the current method.  This throws an exception,
   * so does not return to the caller in a normal way.
   */
  void ThrowJsonError (ErrorCode code, const std::string& msg);

public:

explicit PXRpcServer (xaya::Game& g, PXLogic& l,
                        jsonrpc::AbstractServerConnector& conn)
    : PXRpcServerStub(conn), game(g), logic(l)
  {}

  /**
   * Turns on support for unsafe methods, which should not be publicly
   * exposed.
   */
  void EnableUnsafeMethods ();

  void stop () override;
  Json::Value getcurrentstate () override;
  Json::Value getnullstate () override;
  Json::Value getpendingstate () override;
  std::string waitforchange (const std::string& knownBlock) override;
  Json::Value waitforpendingchange (int oldVersion) override;
  
  Json::Value getxayaplayers () override;
  Json::Value getmoneysupply () override;
  
  Json::Value getuser (const std::string& userName) override;
  Json::Value gettournaments (const std::string& userName) override;
  Json::Value getfueldata (const Json::Value& candiesNew, const Json::Value& candiesSubmited, const Json::Value& candylist, const Json::Value& fighterData, const Json::Value& fightersNew, const Json::Value& fightersSubmited, const Json::Value& recipeData, const Json::Value& recipesNew, const Json::Value& recipesSubmited) override;
  Json::Value getexchange () override;

};

} // namespace pxd

#endif // PXD_PXRPCSERVER_HPP

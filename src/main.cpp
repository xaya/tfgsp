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

#include "config.h"

#include "logic.hpp"
#include "pending.hpp"
#include "pxrpcserver.hpp"

#include <xayagame/defaultmain.hpp>
#include <xayagame/game.hpp>
#include <xayagame/sqliteproc.hpp>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <jsonrpccpp/server/connectors/httpserver.h>

#include <google/protobuf/stubs/common.h>

#include <cstdlib>
#include <iostream>
#include <memory>

#include <string>

namespace
{

DEFINE_string (xaya_rpc_url, "",
               "URL at which the Xaya X (Polygon) JSON-RPC interface is"
               " available");
DEFINE_int32 (xaya_rpc_protocol, 1,
              "JSON-RPC version for connecting to Xaya (use 2 for XayaX)");
DEFINE_int32 (game_rpc_port, 0,
              "the port at which the game's JSON-RPC server will be started"
              " (if non-zero)");
DEFINE_bool (game_rpc_listen_locally, true,
             "whether the game's JSON-RPC server should listen locally");

DEFINE_int32 (enable_pruning, -1,
              "if non-negative (including zero), old undo data will be pruned"
              " and only as many blocks as specified will be kept");

DEFINE_int32 (statehash_interval, 0,
              "if set to a positive number N, hash the game state every N"
              " blocks (best-effort fork-detection via libxayagame SQLiteHasher;"
              " 0 = disabled)");

DEFINE_string (datadir, "",
               "base data directory for game data (will be extended by game ID"
               " and the chain)");

DEFINE_bool (pending_moves, true,
             "whether or not pending moves should be tracked");
             
DEFINE_bool (dump_state, false,
             "whether to dump state into file or not");           

DEFINE_bool (unsafe_rpc, true,
             "whether or not to allow 'unsafe' RPC methods like stop");             

class PXInstanceFactory : public xaya::CustomisedInstanceFactory
{

private:

  /**
   * Reference to the PXLogic instance.  This is needed to construct the
   * RPC server.
   */
  pxd::PXLogic& rules;

  /** The game-state hasher, instantiated only if --statehash_interval > 0.  */
  std::unique_ptr<xaya::SQLiteHasher> hasher;

public:

  explicit PXInstanceFactory (pxd::PXLogic& r)
    : rules(r)
  {
    if (FLAGS_statehash_interval > 0)
      {
        hasher = std::make_unique<xaya::SQLiteHasher> ();
        hasher->SetInterval (FLAGS_statehash_interval);
        rules.AddProcessor (*hasher);
        LOG (INFO) << "Enabled state hashing every "
                   << FLAGS_statehash_interval << " block(s)";
      }
  }

  std::unique_ptr<xaya::RpcServerInterface>
  BuildRpcServer (xaya::Game& game,
                  jsonrpc::AbstractServerConnector& conn) override
  {
    using WrappedRpc = xaya::WrappedRpcServer<pxd::PXRpcServer>;
    auto* h = hasher.get ();
    auto rpc = std::make_unique<WrappedRpc> (game, rules, h, conn);
                                                             
    if (FLAGS_unsafe_rpc)
    {
       rpc->Get ().EnableUnsafeMethods ();
    }
    
    return rpc;
  }


};

} // anonymous namespace


int
main (int argc, char** argv)
{
    
  google::InitGoogleLogging (argv[0]);
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  LOG (INFO)
      << "Running tf version " << PACKAGE_VERSION;
      
  gflags::SetUsageMessage ("Run tf game daemon");
  gflags::SetVersionString (PACKAGE_VERSION);
  gflags::ParseCommandLineFlags (&argc, &argv, false);

#ifdef ENABLE_SLOW_ASSERTS
  LOG (WARNING)
      << "Slow assertions are enabled.  This is fine for testing, but will"
         " slow down syncing";
#endif // ENABLE_SLOW_ASSERTS

  if (FLAGS_xaya_rpc_url.empty ())
    {
      std::cerr << "Error: --xaya_rpc_url must be set" << std::endl;
      return EXIT_FAILURE;
    }
  if (FLAGS_datadir.empty ())
    {
      std::cerr << "Error: --datadir must be specified" << std::endl;
      return EXIT_FAILURE;
    }

  xaya::GameDaemonConfiguration config;
  config.XayaRpcUrl = FLAGS_xaya_rpc_url;
  config.XayaJsonRpcProtocol = FLAGS_xaya_rpc_protocol;
  if (FLAGS_game_rpc_port != 0)
    {
      config.GameRpcServer = xaya::RpcServerType::HTTP;
      config.GameRpcPort = FLAGS_game_rpc_port;
      config.GameRpcListenLocally = FLAGS_game_rpc_listen_locally;
    }
    
    
  config.EnablePruning = FLAGS_enable_pruning;
  config.DataDirectory = FLAGS_datadir;
  
  for (auto& c: config.DataDirectory )
  {
    if (static_cast<unsigned char>(c) > 127) 
    {
       c = 'n';
    }
  }       

  /* We run on Xaya X Eth (Polygon), which reports its version as 1.0.0.0.  */
  config.MinXayaVersion = 1'00'00'00;

  pxd::PXLogic rules;
  
  if (FLAGS_dump_state)
  {
      rules.dumpStateToFile = true;
  }
  else
  {
      rules.dumpStateToFile = false;
  }  
  
  PXInstanceFactory instanceFact(rules);
  config.InstanceFactory = &instanceFact;

  pxd::PendingMoves pending(rules);
  if (FLAGS_pending_moves)
    config.PendingMoves = &pending;

  const int rc = xaya::SQLiteMain (config, "tf", rules);

  google::protobuf::ShutdownProtobufLibrary ();
  return rc;
}

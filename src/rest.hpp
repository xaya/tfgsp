// Copyright (C) 2020 The Xaya developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PXD_REST_HPP
#define PXD_REST_HPP

#include "logic.hpp"

#include <xayagame/game.hpp>
#include <xayagame/rest.hpp>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

namespace pxd
{

/**
 * HTTP server providing a REST API for tfd.
 */
class RestApi : public xaya::RestApi
{

private:

  /** The underlying Game instance that manages everything.  */
  xaya::Game& game;

  /** The game logic implementation.  */
  PXLogic& logic;

  /** Lock for the bootstrap data cache.  */
  std::mutex mutBootstrap;

  /** Set to true if we should stop.  */
  bool shouldStop;

  /** Condition variable that is signaled if shouldStop is set.  */
  std::condition_variable cvStop;

  /** Mutex for the stop flag and condition variable.  */
  std::mutex mutStop;

protected:

  SuccessResult Process (const std::string& url) override;

public:

  explicit RestApi (xaya::Game& g, PXLogic& l, const int p)
    : xaya::RestApi(p), game(g), logic(l)
  {}

  void Start () override;
  void Stop () override;

};

/**
 * REST client for the tf API.
 */
class RestClient : public xaya::RestClient
{

public:

  using xaya::RestClient::RestClient;

};

} // namespace pxd

#endif // PXD_REST_HPP

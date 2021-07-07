// Copyright (C) 2020 The Xaya developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rest.hpp"

#include "gamestatejson.hpp"

#include <microhttpd.h>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <chrono>

namespace pxd
{

namespace
{

DEFINE_int32 (rest_bootstrap_refresh_seconds, 60 * 60,
              "the refresh interval for bootstrap data in seconds");

} // anonymous namespace

RestApi::SuccessResult
RestApi::Process (const std::string& url)
{


  throw HttpError (MHD_HTTP_NOT_FOUND, "invalid API endpoint");
}

void
RestApi::Start ()
{
  xaya::RestApi::Start ();

  std::lock_guard<std::mutex> lock(mutStop);
  shouldStop = false;
}

void
RestApi::Stop ()
{
  {
    std::lock_guard<std::mutex> lock(mutStop);
    shouldStop = true;
    cvStop.notify_all ();
  }

  xaya::RestApi::Stop ();
}

} // namespace pxd

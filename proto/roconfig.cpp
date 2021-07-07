/*
    GSP for the Taurion blockchain game
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

#include "roconfig.hpp"

#include <glog/logging.h>

#include <map>
#include <memory>
#include <mutex>

extern "C"
{

/* Binary blob for the roconfig protocol buffer data.  */
extern const unsigned char blob_roconfig_start;
extern const unsigned char blob_roconfig_end;

} // extern C

namespace pxd
{

/* ************************************************************************** */

namespace
{

/** Lock for constructing and accessing the global singletons.  */
std::mutex mutInstances;

} // anonymous namespace

/**
 * Data for the singleton instance of the proto with all associated
 * extra stuff (like constructed items).
 */
struct RoConfig::Data
{

  /** Mutex for the mutable state.  */
  mutable std::recursive_mutex mut;

  /** The protocol buffer instance itself.  */
  proto::ConfigData proto;

};

RoConfig::Data* RoConfig::mainnet = nullptr;
RoConfig::Data* RoConfig::testnet = nullptr;
RoConfig::Data* RoConfig::regtest = nullptr;

RoConfig::RoConfig (const xaya::Chain chain)
{
  std::lock_guard<std::mutex> lock(mutInstances);

  Data** instancePtr = nullptr;
  bool mergeTestnet, mergeRegtest;
  switch (chain)
    {
    case xaya::Chain::MAIN:
      instancePtr = &mainnet;
      mergeTestnet = false;
      mergeRegtest = false;
      break;
    case xaya::Chain::TEST:
      instancePtr = &testnet;
      mergeTestnet = true;
      mergeRegtest = false;
      break;
    case xaya::Chain::REGTEST:
      instancePtr = &regtest;
      mergeTestnet = true;
      mergeRegtest = true;
      break;
    default:
      LOG (FATAL) << "Unexpected chain: " << static_cast<int> (chain);
    }
  CHECK (instancePtr != nullptr);

  if (*instancePtr == nullptr)
    {
      LOG (INFO) << "Initialising hard-coded ConfigData proto instance...";

      *instancePtr = new Data ();
      auto& pb = (*instancePtr)->proto;

      const auto* begin = &blob_roconfig_start;
      const auto* end = &blob_roconfig_end;
      CHECK (pb.ParseFromArray (begin, end - begin));

      CHECK (!pb.testnet_merge ().has_testnet_merge ());
      CHECK (!pb.testnet_merge ().has_regtest_merge ());

      CHECK (!pb.regtest_merge ().has_testnet_merge ());
      CHECK (!pb.regtest_merge ().has_regtest_merge ());

      if (mergeTestnet)
      {
        pb.MergeFrom (pb.testnet_merge ());
      }
      if (mergeRegtest)
      {
          pb.MergeFrom (pb.regtest_merge ());
      }
      pb.clear_testnet_merge ();
      pb.clear_regtest_merge ();
    }

  data = *instancePtr;
  CHECK (data != nullptr);
}

const proto::ConfigData&
RoConfig::operator* () const
{
  return data->proto;
}

const proto::ConfigData*
RoConfig::operator-> () const
{
  return &(operator* ());
}



/* ************************************************************************** */

} // namespace pxd

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

#ifndef PROTO_ROCONFIG_HPP
#define PROTO_ROCONFIG_HPP

#include "config.pb.h"

#include <xayagame/gamelogic.hpp>

namespace pxd
{

/**
 * A light wrapper class around the read-only ConfigData proto.  It allows
 * access to the proto data itself as well as provides some helper methods
 * for accessing the data on a higher level (e.g. specifically for items
 * or buildings).
 */
class RoConfig
{

private:

  class Data;

  /**
   * A reference to the singleton instance that actually holds all the
   * global state wrapped by this instance.
   */
  const Data* data;

  /**
   * The global singleton data instance for mainnet or null when it is not yet
   * initialised.  This is never destructed.
   */
  static Data* mainnet;

  /** The singleton instance for testnet.  */
  static Data* testnet;

  /** The singleton instance for regtest.  */
  static Data* regtest;

public:

  /**
   * Constructs a fresh instance of the wrapper class, which will give
   * access to the underlying data.
   *
   * On the first call, this will also instantiate and set up the underlying
   * singleton instance with the real data.
   */
  explicit RoConfig (xaya::Chain chain);

  RoConfig (const RoConfig&) = delete;
  void operator= (const RoConfig&) = delete;

  /**
   * Exposes the actual protocol buffer.
   */
  const proto::ConfigData& operator* () const;

  /**
   * Exposes the actual protocol buffer's fields directly.
   */
  const proto::ConfigData* operator-> () const;

};

} // namespace pxd

#endif // PROTO_ROCONFIG_HPP

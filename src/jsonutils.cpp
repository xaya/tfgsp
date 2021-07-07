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

#include "jsonutils.hpp"

#include <xayautil/jsonutils.hpp>

#include <glog/logging.h>

#include <limits>

namespace pxd
{

namespace
{

/**
 * The maximum amount of vCHI in a move.  This is consensus relevant.
 * The value here is actually the total cap on vCHI (although that's not
 * relevant in this context).
 */

constexpr Database::IdT MAX_ID = 999999999;
constexpr Amount MAX_COIN_AMOUNT = 100'000'000'000;


} // anonymous namespace

bool
CoinAmountFromJson (const Json::Value& val, Amount& amount)
{
  if (!val.isInt64 () || !xaya::IsIntegerValue (val))
    return false;

  amount = val.asInt64 ();
  return amount >= 0 && amount <= MAX_COIN_AMOUNT;
}

bool
IdFromJson (const Json::Value& val, Database::IdT& id)
{
  if (!val.isUInt64 () || !xaya::IsIntegerValue (val))
    return false;

  id = val.asUInt64 ();
  return id > 0 && id < MAX_ID;
}

template <>
  Json::Value
  IntToJson<int32_t> (const int32_t val)
{
  return static_cast<Json::Int> (val);
}

template <>
  Json::Value
  IntToJson<uint32_t> (const uint32_t val)
{
  return static_cast<Json::UInt> (val);
}

template <>
  Json::Value
  IntToJson<int64_t> (const int64_t val)
{
  return static_cast<Json::Int64> (val);
}

template <>
  Json::Value
  IntToJson<uint64_t> (const uint64_t val)
{
  return static_cast<Json::UInt64> (val);
}

} // namespace pxd

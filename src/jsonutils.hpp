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

#ifndef PXD_JSONUTILS_HPP
#define PXD_JSONUTILS_HPP

#include "database/database.hpp"
#include "database/amount.hpp"

#include <json/json.h>

#include <string>
#include <vector>

namespace pxd
{

/**
 * Converts an integer value to the proper JSON representation.
 */
template <typename T>
  Json::Value IntToJson (T val);
  
/**
 * Parses a Cubit amount from JSON, and verifies that it is roughly in
 * range, i.e. within [0, MAX_COIN_AMOUNT].
 */
bool CoinAmountFromJson (const Json::Value& val, Amount& amount);  

} // namespace pxd

#endif // PXD_JSONUTILS_HPP

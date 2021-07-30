/*
    GSP for the TF blockchain game
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

/* Template implementation code for xayaplayers.hpp.  */

#include <glog/logging.h>

#include <type_traits>

namespace pxd
{

template <typename T>
  PlayerRole
  GetNullablePlayerRoleFromColumn (const Database::Result<T>& res)
{
  static_assert (std::is_base_of<ResultWithRole, T>::value,
                 "GetNullablePlayerRoleFromColumn needs a ResultWithRole");
  
  if (res.template IsNull<typename T::role> ())
    return PlayerRole::INVALID;

  const auto val = res.template Get<typename T::role> ();
  CHECK (val >= 1 && val <= 6)
      << "Invalid role value from database: " << val;

  return static_cast<PlayerRole> (val);
}

template <typename T>
  FTUEState
  GetFTUEStateFromColumn (const Database::Result<T>& res)
{
  if (res.template IsNull<typename T::ftuestate> ())
    return FTUEState::Intro;

  const auto val = res.template Get<typename T::ftuestate> ();
  return static_cast<FTUEState> (val);
}

template <typename T>
  PlayerRole
  GetPlayerRoleFromColumn (const Database::Result<T>& res)
{
  const PlayerRole f = GetNullablePlayerRoleFromColumn (res);
  CHECK (f != PlayerRole::INVALID) << "Unexpected NULL role";
  return f;
}

} // namespace pxd

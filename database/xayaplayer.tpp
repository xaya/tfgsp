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

  /* Whitelist the exact PlayerRole enum values (symmetric with
     BindPlayerRoleParameter).  A range check like `val <= 6` both rejected the
     valid high roles EXCHANGEADMIN(8)/ADMINISTRATOR(15) -- crashing every node
     that reloaded such a row -- and wrongly admitted the non-existent 5/6. */
  const auto val = res.template Get<typename T::role> ();
  switch (val)
    {
    case 1:  return PlayerRole::PLAYER;
    case 2:  return PlayerRole::ROLEADMIN;
    case 3:  return PlayerRole::CONENTADMIN;
    case 4:  return PlayerRole::CONFIGADMIN;
    case 8:  return PlayerRole::EXCHANGEADMIN;
    case 15: return PlayerRole::ADMINISTRATOR;
    default:
      LOG (FATAL) << "Invalid role value from database: " << val;
    }
  return PlayerRole::INVALID; // unreachable; silences -Wreturn-type
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

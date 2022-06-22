/*
    GSP for the Taurion blockchain game
    Copyright (C) 2020  Autonomous Worlds Ltd

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

#ifndef DATABASE_GLOBALDATA_HPP
#define DATABASE_GLOBALDATA_HPP

#include "amount.hpp"
#include "database.hpp"

#include <set>
#include <string>

namespace pxd
{

/**
 * Wrapper class around the database table holding data about
 * the money supply.
 */
class GlobalData
{

private:

  /** The underlying database handle.  */
  Database& db;

public:

  explicit GlobalData (Database& d)
    : db(d)
  {}

  GlobalData () = delete;
  GlobalData (const GlobalData&) = delete;
  void operator= (const GlobalData&) = delete;

  /**
   * Returns the timestamp of the last special tournament data held
   */
  int64_t GetLastTournamentTime();
  
  /**
   * Returns Crystal prices in CHI multiplier
   */
  int64_t GetChiMultiplier();  



  /**
   * Initialises the database, putting in all entries that are valid
   * with initial amounts.
   */
  void InitialiseDatabase ();
};

} // namespace pxd

#endif // DATABASE_GLOBALDATA_HPP

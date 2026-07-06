/*
    GSP for the tf blockchain game
    Copyright (C) 2026  Autonomous Worlds Ltd

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

#ifndef DATABASE_PARAMS_HPP
#define DATABASE_PARAMS_HPP

#include "database.hpp"

#include <cstdint>
#include <string>

namespace pxd
{

/**
 * Runtime-tunable game parameters, held in the `parameters` KV table
 * (name TEXT PRIMARY KEY -> value INTEGER).  Mirrors Soccerverse's params table
 * so the balance knobs (rarest_recipe_drop_divisor / tournament_loss_kills_enabled
 * / tournament_capture_pct / tournament_max_captures) can be retuned live via a g/tf admin `param` move
 * with no rebuild.  Every knob is seeded at genesis (InitialiseDatabase), so
 * GetParam CHECK-fails on an unset name -- the authoritative value is always the
 * on-chain seeded/admin value, never a drifting C++ literal (no silent hard fork).
 */
class GameParams
{

private:

  /** The underlying database handle.  */
  Database& db;

public:

  explicit GameParams (Database& d) : db(d) {}

  GameParams () = delete;
  GameParams (const GameParams&) = delete;
  void operator= (const GameParams&) = delete;

  /** Returns the value for `name`; CHECK-fails if the name was never set.  */
  int64_t GetParam (const std::string& name);

  /** Returns true iff `name` has a row.  */
  bool HasParam (const std::string& name);

  /** Inserts or updates `name` -> `value`.  */
  void SetParam (const std::string& name, int64_t value);

  /** Deletes `name` (no-op if absent).  */
  void RemoveParam (const std::string& name);

  /** Seeds the launch defaults; called once at genesis and in test-DB setup.  */
  void InitialiseDatabase ();

};

} // namespace pxd

#endif // DATABASE_PARAMS_HPP

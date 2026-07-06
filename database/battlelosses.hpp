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

#ifndef DATABASE_BATTLELOSSES_HPP
#define DATABASE_BATTLELOSSES_HPP

#include "database.hpp"

#include <cstdint>
#include <string>

namespace pxd
{

/** A row of the battle_losses table.  */
struct BattleLossResult : public Database::ResultType
{
  RESULT_COLUMN (int64_t, id, 1);
  RESULT_COLUMN (std::string, owner, 2);
  RESULT_COLUMN (int64_t, fighterid, 3);
  RESULT_COLUMN (std::string, name, 4);
  RESULT_COLUMN (int64_t, outcome, 5);
  RESULT_COLUMN (std::string, opponent, 6);
  RESULT_COLUMN (int64_t, tournamentid, 7);
};

/**
 * Access to the battle_losses table (Change C): append a client-visible battle
 * record when a fighter is destroyed/captured in a tournament, and read a
 * player's records for getuser.  Rows are per-perspective, keyed by `owner` = the
 * player who should SEE the record; `opponent` is the other party.
 */
class BattleLossesTable
{

private:

  Database& db;

public:

  explicit BattleLossesTable (Database& d) : db(d) {}

  /** Appends one record (id auto-assigned).  `outcome` is perspective-tagged:
   *    LOSER's view (owner = the losing player):
   *      0 = your fighter was destroyed,
   *      1 = your fighter was captured by `opponent`;
   *    WINNER's view (owner = the tournament winner):
   *      2 = you captured `opponent`'s fighter,
   *      3 = you defeated it but your roster was full -> it was destroyed,
   *      4 = you defeated it but hit the capture cap -> it was destroyed.
   *  When a capture is blocked by BOTH a full roster and the cap, code 4 (cap) is
   *  written -- the cap is the binding per-tournament limit.  */
  void Add (const std::string& owner, uint32_t fighterId, const std::string& name,
            uint32_t outcome, const std::string& opponent, uint32_t tournamentId);

  /** All losses for `owner`, ordered by id (oldest first).  */
  Database::Result<BattleLossResult> QueryForOwner (const std::string& owner);

};

} // namespace pxd

#endif // DATABASE_BATTLELOSSES_HPP

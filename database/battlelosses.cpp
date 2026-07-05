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

#include "battlelosses.hpp"

#include <glog/logging.h>

namespace pxd
{

void
BattleLossesTable::Add (const std::string& owner, const uint32_t fighterId,
                        const std::string& name, const uint32_t outcome,
                        const std::string& opponent, const uint32_t tournamentId)
{
  auto stmt = db.Prepare (R"(
    INSERT INTO `battle_losses`
      (`id`, `owner`, `fighterid`, `name`, `outcome`, `opponent`, `tournamentid`)
      VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7)
  )");
  stmt.Bind (1, db.GetNextId ());
  stmt.Bind (2, owner);
  stmt.Bind (3, (int64_t) fighterId);
  stmt.Bind (4, name);
  stmt.Bind (5, (int64_t) outcome);
  stmt.Bind (6, opponent);
  stmt.Bind (7, (int64_t) tournamentId);
  stmt.Execute ();
}

Database::Result<BattleLossResult>
BattleLossesTable::QueryForOwner (const std::string& owner)
{
  auto stmt = db.Prepare (R"(
    SELECT * FROM `battle_losses` WHERE `owner` = ?1 ORDER BY `id`
  )");
  stmt.Bind (1, owner);
  return stmt.Query<BattleLossResult> ();
}

} // namespace pxd

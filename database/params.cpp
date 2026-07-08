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

#include "params.hpp"

#include <glog/logging.h>

namespace pxd
{

namespace
{

struct ParamResult : public Database::ResultType
{
  RESULT_COLUMN (std::string, name, 1);
  RESULT_COLUMN (int64_t, value, 2);
};

/* Fixed geometric shape, 256 == 1.0x: DURATION_MULT[sw] = round (256 * 20^((sw-1)/9)).
   Index 0 is unused (sweetness is 1..10). */
constexpr int64_t DURATION_MULT[11]
    = {0, 256, 357, 498, 695, 969, 1352, 1886, 2630, 3669, 5120};

/* Cook Quality (None=0,Common=1,Uncommon=2,Rare=3,Epic=4) -> sweetness tier. */
constexpr uint32_t COOK_Q2S[5] = {1, 1, 4, 7, 10};

} // anonymous namespace

int64_t
GameParams::GetParam (const std::string& name)
{
  auto stmt = db.Prepare (R"(
    SELECT `name`, `value` FROM `parameters` WHERE `name` = ?1
  )");
  stmt.Bind (1, name);

  auto res = stmt.Query<ParamResult> ();
  CHECK (res.Step ()) << "GameParams::GetParam: unset parameter '" << name << "'";
  const int64_t value = res.Get<ParamResult::value> ();
  CHECK (!res.Step ());

  return value;
}

bool
GameParams::HasParam (const std::string& name)
{
  auto stmt = db.Prepare (R"(
    SELECT `name`, `value` FROM `parameters` WHERE `name` = ?1
  )");
  stmt.Bind (1, name);

  auto res = stmt.Query<ParamResult> ();
  return res.Step ();
}

void
GameParams::SetParam (const std::string& name, const int64_t value)
{
  auto stmt = db.Prepare (R"(
    INSERT OR REPLACE INTO `parameters` (`name`, `value`) VALUES (?1, ?2)
  )");
  stmt.Bind (1, name);
  stmt.Bind (2, value);
  stmt.Execute ();
}

void
GameParams::RemoveParam (const std::string& name)
{
  auto stmt = db.Prepare (R"(
    DELETE FROM `parameters` WHERE `name` = ?1
  )");
  stmt.Bind (1, name);
  stmt.Execute ();
}

void
GameParams::InitialiseDatabase ()
{
  SetParam ("rarest_recipe_drop_divisor", 4);
  SetParam ("tournament_loss_kills_enabled", 1);
  SetParam ("tournament_capture_pct", 128);
  SetParam ("tournament_max_captures", 3);
}

int32_t
ScaleDuration (int32_t base, uint32_t sweetness, int64_t pct)
{
  if (base <= 0)
    return base;
  uint32_t s = sweetness;
  if (s < 1) s = 1;
  if (s > 10) s = 10;
  const uint64_t scaled = (uint64_t) base * (uint64_t) DURATION_MULT[s]
                          * (uint64_t) pct / 25600ULL;
  return scaled < 1 ? 1 : (int32_t) scaled;
}

uint32_t
CookQualityToSweetness (uint32_t quality)
{
  if (quality > 4)
    quality = 4;
  return COOK_Q2S[quality];
}

} // namespace pxd

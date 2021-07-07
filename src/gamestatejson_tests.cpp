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

#include "gamestatejson.hpp"
#include "database/dbtest.hpp"
#include "testutils.hpp"

#include "database/xayaplayer.hpp"

#include <gtest/gtest.h>

#include <json/json.h>

#include <string>

namespace pxd
{
namespace
{

/* ************************************************************************** */

class GameStateJsonTests : public DBTestWithSchema
{

protected:

  ContextForTesting ctx;

  /** GameStateJson instance used in testing.  */
  GameStateJson converter;

  GameStateJsonTests ()
    : converter(db, ctx)
  {}

  /**
   * Expects that the current state matches the given one, after parsing
   * the expected state's string as JSON.  Furthermore, the expected value
   * is assumed to be *partial* -- keys that are not present in the expected
   * value may be present with any value in the actual object.  If a key is
   * present in expected but has value null, then it must not be present
   * in the actual data, though.
   */
  void
  ExpectStateJson (const std::string& expectedStr)
  {
    const Json::Value actual = converter.FullState ();
    VLOG (1) << "Actual JSON for the game state:\n" << actual;
    ASSERT_TRUE (PartialJsonEqual (actual, ParseJson (expectedStr)));
  }

};

/* ************************************************************************** */

class XayaPlayersJsonTests : public GameStateJsonTests
{

protected:

  XayaPlayersTable tbl;

  XayaPlayersJsonTests ()
    : tbl(db)
  {}

};

TEST_F (XayaPlayersJsonTests, KillsAndFame)
{
  auto a = tbl.CreateNew ("foo");
  a->SetRole (PlayerRole::PLAYER);
  a.reset ();

  a = tbl.CreateNew ("bar");
  a->SetRole (PlayerRole::ROLEADMIN);
  a.reset ();

  ExpectStateJson (R"({
    "xayaplayers":
      [
        {"name": "bar", "role": "r"},
        {"name": "foo", "role": "p"}
      ]
  })");
}

TEST_F (XayaPlayersJsonTests, UninitialisedBalance)
{
  tbl.CreateNew ("foo")->SetRole (PlayerRole::PLAYER);

  auto a = tbl.CreateNew ("bar");
  a->MutableProto ().set_burnsale_balance (10);
  a->AddBalance (42);
  a.reset ();

  ExpectStateJson (R"({
	"xayaplayers" : 
	[
		{
			"balance" : 
			{
				"available" : 42,
				"reserved" : 0,
				"total" : 42
			},
			"minted" : 10,
			"name" : "bar"
		},
		{
			"balance" : 
			{
				"available" : 0,
				"reserved" : 0,
				"total" : 0
			},
			"minted" : 0,
			"name" : "foo",
			"role" : "p"
		}
	]
  })");
}

/* ************************************************************************** */

/* ************************************************************************** */

} // anonymous namespace
} // namespace pxd

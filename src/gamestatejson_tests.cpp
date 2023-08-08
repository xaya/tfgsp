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
#include "database/fighter.hpp"
#include "database/recipe.hpp"
#include "database/reward.hpp"
#include "testutils.hpp"


#include <xayautil/random.hpp>

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
    //LOG (WARNING) << "Actual JSON for the game state:\n" << actual;
    //LOG (WARNING) << "EXPECTED JSON for the game state:\n" << expectedStr;
    ASSERT_TRUE (PartialJsonEqual (actual, ParseJson (expectedStr)));
  }
  
  void
  ExpectStateTournamentsOnlyJson (const std::string& expectedStr, const std::string& userName)
  {
    const Json::Value actual = converter.UserTournaments (userName);
    //LOG (WARNING) << "Actual tournament JSON for the game state:\n" << actual;
    //LOG (WARNING) << "EXPECTED tournament JSON for the game state:\n" << expectedStr;
    ASSERT_TRUE (PartialJsonEqual (actual, ParseJson (expectedStr)));
  }  
  
  void
  ExpectStateUserOnlyJson (const std::string& expectedStr, const std::string& userName)
  {
    const Json::Value actual = converter.User(userName);
    LOG (WARNING) << "Actual user JSON for the game state:\n" << actual;
    LOG (WARNING) << "EXPECTED user JSON for the game state:\n" << expectedStr;
    ASSERT_TRUE (PartialJsonEqual (actual, ParseJson (expectedStr)));
  }   

};

/* ************************************************************************** */

class XayaPlayersJsonTests : public GameStateJsonTests
{

protected:

  TestRandom rnd;
  
  XayaPlayersTable tbl;
  FighterTable tbl2;
  RecipeInstanceTable tbl3;
  RewardsTable tbl4;
  TournamentTable tbl5;
   
  std::unique_ptr<pxd::RoConfig> cfg;

  XayaPlayersJsonTests ()
    : tbl(db), tbl2(db), tbl3(db), tbl4(db), tbl5(db)
  {
      cfg = std::make_unique<pxd::RoConfig> (xaya::Chain::REGTEST);
  }

};

TEST_F (XayaPlayersJsonTests, KillsAndFame)
{
  auto a = tbl.CreateNew ("foo", ctx.RoConfig(), rnd, true);
  a->SetRole (PlayerRole::PLAYER);
  a.reset ();

  a = tbl.CreateNew ("bar", ctx.RoConfig(), rnd, true);
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
  tbl.CreateNew ("foo", ctx.RoConfig(), rnd, true)->SetRole (PlayerRole::PLAYER);

  auto a = tbl.CreateNew ("bar", ctx.RoConfig(), rnd, true);
  a->AddBalance (42);
  a.reset ();

  ExpectStateJson (R"({
        "xayaplayers" :
        [
                {
                        "balance" :
                        {
                                "available" : 42
                        },
                        "inventory" :
                        {
                                "fungible" :
                                {
                                        "Common_Candy Cane" : 9,
                                        "Common_Chocolate Chip" : 20,
                                        "Common_Fizzy Powder" : 9,
                                        "Common_Icing" : 20,
                                        "Common_Nonpareil" : 10
                                }
                        },
                        "minted" : 0,
                        "name" : "bar",
                        "ongoings" : [],
                        "prestige" : 1586,
                        "recepies" :
                        [
                                10
                        ],
                        "role" : "i"
                },
                {
                        "balance" :
                        {
                                "available" : 0
                        },
                        "inventory" :
                        {
                                "fungible" :
                                {
                                        "Common_Candy Cane" : 9,
                                        "Common_Chocolate Chip" : 20,
                                        "Common_Fizzy Powder" : 9,
                                        "Common_Icing" : 20,
                                        "Common_Nonpareil" : 10
                                }
                        },
                        "minted" : 0,
                        "name" : "foo",
                        "ongoings" : [],
                        "prestige" : 1586,
                        "recepies" :
                        [
                                5
                        ],
                        "role" : "p"
                }
        ]


  })");
}

TEST_F (XayaPlayersJsonTests, FighterInstance)
{
  auto a = tbl.CreateNew ("foo", ctx.RoConfig(), rnd, true);
  a->SetRole (PlayerRole::PLAYER);
  a.reset ();

  auto r0 = tbl3.CreateNew("foo", "5864a19b-c8c0-2d34-eaef-9455af0baf2c", *cfg);
  const auto id0 = r0->GetId ();
  r0.reset();  
  
  auto f1 = tbl2.CreateNew ("foo", id0, *cfg, rnd);
  f1.reset();
  
  ExpectStateJson (R"({
    "xayaplayers":
      [
        {"name": "foo", "role": "p"}
      ],
   "fighters" :
        [
                {
                        "armorpieces" :
                        [
                                {
                                        "armortype" : 3,
                                        "candy" : "934dae05-689f-30e4-5804-d497aee0b47c",
                                        "rewardsource" : 0,
                                        "rewardsourceid" : ""
                                },
                                {
                                        "armortype" : 14,
                                        "candy" : "c2774273-0cd4-2494-fa02-76cffe1ef1ef",
                                        "rewardsource" : 0,
                                        "rewardsourceid" : ""
                                }
                        ],
                        "blocksleft" : 0,
                        "expeditioninstanceid" : "",
                        "fightertypeid" : "c160f7ad-c775-8614-abe2-8ef74e54401f",
                        "highestappliedsweetener" : 1,
                        "id" : 3,
                        "moves" :
                        [
                                {
                                        "authoredid" : "86b323c2-b2fd-2494-ab5e-bc3514bc92d8"
                                },
                                {
                                        "authoredid" : "0090580c-04ef-9d84-e883-32f52c977b98"
                                }
                        ],
                        "name" : "Sour Gummi Brawler",
                        "owner" : "foo",
                        "quality" : 1,
                        "rating" : 1000,
                        "recipeid" : 1,
                        "status" : 0,
                        "sweetness" : 1,
                        "tournamentinstanceid" : 0
                },
                {
                        "armorpieces" :
                        [
                                {
                                        "armortype" : 13,
                                        "candy" : "4e343f68-1aaa-0e84-7bab-eae458b68264",
                                        "rewardsource" : 0,
                                        "rewardsourceid" : ""
                                },
                                {
                                        "armortype" : 14,
                                        "candy" : "c2774273-0cd4-2494-fa02-76cffe1ef1ef",
                                        "rewardsource" : 0,
                                        "rewardsourceid" : ""
                                }
                        ],
                        "blocksleft" : 0,
                        "expeditioninstanceid" : "",
                        "fightertypeid" : "85f361b8-e55d-0244-1a98-26bffa0a18a2",
                        "highestappliedsweetener" : 1,
                        "id" : 4,
                        "moves" :
                        [
                                {
                                        "authoredid" : "2c555752-8a84-58f4-395e-6460b7864069"
                                },
                                {
                                        "authoredid" : "0090580c-04ef-9d84-e883-32f52c977b98"
                                }
                        ],
                        "name" : "Caramel Kicker",
                        "owner" : "foo",
                        "quality" : 1,
                        "rating" : 1000,
                        "recipeid" : 2,
                        "status" : 0,
                        "sweetness" : 1,
                        "tournamentinstanceid" : 0
                },
                {
                        "armorpieces" :
                        [
                                {
                                        "armortype" : 3,
                                        "candy" : "934dae05-689f-30e4-5804-d497aee0b47c",
                                        "rewardsource" : 0,
                                        "rewardsourceid" : ""
                                },
                                {
                                        "armortype" : 17,
                                        "candy" : "c2774273-0cd4-2494-fa02-76cffe1ef1ef",
                                        "rewardsource" : 0,
                                        "rewardsourceid" : ""
                                }
                        ],
                        "blocksleft" : 0,
                        "expeditioninstanceid" : "",
                        "fightertypeid" : "c160f7ad-c775-8614-abe2-8ef74e54401f",
                        "highestappliedsweetener" : 1,
                        "id" : 7,
                        "moves" :
                        [
                                {
                                        "authoredid" : "86b323c2-b2fd-2494-ab5e-bc3514bc92d8"
                                },
                                {
                                        "authoredid" : "0090580c-04ef-9d84-e883-32f52c977b98"
                                }
                        ],
                        "name" : "Sour Gummi Brawler",
                        "owner" : "foo",
                        "quality" : 1,
                        "rating" : 1000,
                        "recipeid" : 6,
                        "status" : 0,
                        "sweetness" : 1,
                        "tournamentinstanceid" : 0
                }
        ]

  })");
}

TEST_F (XayaPlayersJsonTests, RecipeInstance)
{
  auto a = tbl.CreateNew ("foo", ctx.RoConfig(), rnd, true);
  a->SetRole (PlayerRole::PLAYER);
  a.reset ();

  auto r0 = tbl3.CreateNew("foo", "5864a19b-c8c0-2d34-eaef-9455af0baf2c", *cfg);
  r0->MutableProto().set_authoredid("generated");
  r0.reset();  

  ExpectStateJson (R"({
    "xayaplayers":
      [
        {"name": "foo", "role": "p"}
      ],
     "recepies" :
        [
                {
                        "authoredid" : "5864a19b-c8c0-2d34-eaef-9455af0baf2c",
                        "did" : 1,
                        "owner" : ""
                },
                {
                        "authoredid" : "ba0121ba-e8a6-7e64-9bc1-71dfeca27daa",
                        "did" : 2,
                        "owner" : ""
                },
                {
                        "authoredid" : "1bbc7d99-7fce-24a4-c9a3-dfaf4b744efa",
                        "did" : 5,
                        "owner" : "foo"
                },
                {
                        "animationid" : "938e1ab9-a85b-5444-7bd6-f8862ace8b16",
                        "armor" :
                        [
                                {
                                        "armortype" : 8,
                                        "candytype" : "f9c0a4d2-f049-29a4-eb1e-2269de35798e"
                                },
                                {
                                        "armortype" : 9,
                                        "candytype" : "f9c0a4d2-f049-29a4-eb1e-2269de35798e"
                                }
                        ],
                        "authoredid" : "generated",
                        "did" : 6,
                        "duration" : 1,
                        "fightername" : "Sour Gummi Brawler",
                        "fightertype" : "c160f7ad-c775-8614-abe2-8ef74e54401f",
                        "isaccountbound" : true,
                        "moves" :
                        [
                                "86b323c2-b2fd-2494-ab5e-bc3514bc92d8",
                                "0090580c-04ef-9d84-e883-32f52c977b98"
                        ],
                        "name" : "First Recipe",
                        "owner" : "foo",
                        "quality" : 1,
                        "requiredcandy" :
                        [
                                {
                                        "amount" : 1,
                                        "candytype" : "c2774273-0cd4-2494-fa02-76cffe1ef1ef"
                                },
                                {
                                        "amount" : 1,
                                        "candytype" : "f9c0a4d2-f049-29a4-eb1e-2269de35798e"
                                }
                        ],
                        "requiredfighterquality" : 0
                }
        ]

  })");
}

TEST_F (XayaPlayersJsonTests, ExpeditionInstance)
{
  auto a = tbl.CreateNew ("foo", ctx.RoConfig(), rnd, true);
  a->SetRole (PlayerRole::PLAYER);
  a.reset ();

  auto rw = tbl4.CreateNew ("foo");
  rw->MutableProto().set_expeditionid("myexpid");
  rw->MutableProto().set_generatedrecipeid(2);
  rw.reset();

  ExpectStateJson (R"({
        "rewards" :
        [
          {
            "expeditionid" : "myexpid",
            "generatedrecipeid" : 2,
            "owner" : "foo",
            "sweetenerid" : "",
            "tournamentid" : 0
          }
        ]
        }
        )");
		
  ExpectStateUserOnlyJson (R"({
        "fighters" :
        [
                {
                        "animationid" : "5fce85e9-51f9-d7b4-db81-ef141439db00",
                        "armorpieces" :
                        [
                                {
                                        "armortype" : 3,
                                        "candy" : "934dae05-689f-30e4-5804-d497aee0b47c",
                                        "rewardsource" : 0,
                                        "rewardsourceid" : ""
                                },
                                {
                                        "armortype" : 14,
                                        "candy" : "c2774273-0cd4-2494-fa02-76cffe1ef1ef",
                                        "rewardsource" : 0,
                                        "rewardsourceid" : ""
                                }
                        ],
                        "blocksleft" : 0,
                        "deconstructions" : [],
                        "exchangeexpire" : 0,
                        "exchangeprice" : 0,
                        "expeditioninstanceid" : "",
                        "fightertypeid" : "c160f7ad-c775-8614-abe2-8ef74e54401f",
                        "highestappliedsweetener" : 1,
                        "id" : 3,
                        "isaccountbound" : true,
                        "lasttournamenttime" : 0,
                        "moves" :
                        [
                                {
                                        "authoredid" : "86b323c2-b2fd-2494-ab5e-bc3514bc92d8"
                                },
                                {
                                        "authoredid" : "0090580c-04ef-9d84-e883-32f52c977b98"
                                }
                        ],
                        "name" : "Sour Gummi Brawler",
                        "owner" : "foo",
                        "quality" : 1,
                        "rating" : 1000,
                        "recipeid" : 1,
                        "saleshistory" : [],
                        "specialtournamentinstanceid" : 0,
                        "specialtournamentstatus" : 0,
                        "status" : 0,
                        "sweetness" : 1,
                        "tournamentinstanceid" : 0,
                        "tournamentpoints" : 0
                },
                {
                        "animationid" : "05633498-ace9-de14-c939-9435a6343d0f",
                        "armorpieces" :
                        [
                                {
                                        "armortype" : 13,
                                        "candy" : "4e343f68-1aaa-0e84-7bab-eae458b68264",
                                        "rewardsource" : 0,
                                        "rewardsourceid" : ""
                                },
                                {
                                        "armortype" : 14,
                                        "candy" : "c2774273-0cd4-2494-fa02-76cffe1ef1ef",
                                        "rewardsource" : 0,
                                        "rewardsourceid" : ""
                                }
                        ],
                        "blocksleft" : 0,
                        "deconstructions" : [],
                        "exchangeexpire" : 0,
                        "exchangeprice" : 0,
                        "expeditioninstanceid" : "",
                        "fightertypeid" : "85f361b8-e55d-0244-1a98-26bffa0a18a2",
                        "highestappliedsweetener" : 1,
                        "id" : 4,
                        "isaccountbound" : true,
                        "lasttournamenttime" : 0,
                        "moves" :
                        [
                                {
                                        "authoredid" : "2c555752-8a84-58f4-395e-6460b7864069"
                                },
                                {
                                        "authoredid" : "0090580c-04ef-9d84-e883-32f52c977b98"
                                }
                        ],
                        "name" : "Caramel Kicker",
                        "owner" : "foo",
                        "quality" : 1,
                        "rating" : 1000,
                        "recipeid" : 2,
                        "saleshistory" : [],
                        "specialtournamentinstanceid" : 0,
                        "specialtournamentstatus" : 0,
                        "status" : 0,
                        "sweetness" : 1,
                        "tournamentinstanceid" : 0,
                        "tournamentpoints" : 0
                }
        ],
        "multiplier" : 1000,
        "ratings" :
        [
                {
                        "name" : "foo",
                        "prestige" : 1586
                }
        ],
        "recepies" :
        [
                {
                        "authoredid" : "ba0121ba-e8a6-7e64-9bc1-71dfeca27daa",
                        "did" : 2,
                        "owner" : ""
                },
                {
                        "authoredid" : "1bbc7d99-7fce-24a4-c9a3-dfaf4b744efa",
                        "did" : 5,
                        "owner" : "foo"
                }
        ],
        "rewards" :
        [
                {
                        "expeditionid" : "myexpid",
                        "fighterid" : 0,
                        "generatedrecipeid" : 2,
                        "owner" : "foo",
                        "positionintable" : 0,
                        "rewardid" : "",
                        "rid" : 6,
                        "sweetenerid" : "",
                        "tournamentid" : 0
                }
        ],
        "stateblock" : 0,
        "statehex" : "",
        "vanillaurl" : "xaya.io",
        "version" : "1.1.3",
        "xayaplayers" :
        [
                {
                        "address" : "",
                        "balance" :
                        {
                                "available" : 0
                        },
                        "fighteraverage" : 476,
                        "inventory" :
                        {
                                "fungible" :
                                {
                                        "Common_Candy Cane" : 9,
                                        "Common_Chocolate Chip" : 20,
                                        "Common_Fizzy Powder" : 9,
                                        "Common_Icing" : 20,
                                        "Common_Nonpareil" : 10
                                }
                        },
                        "minted" : 0,
                        "name" : "foo",
                        "ongoings" : [],
                        "prestige" : 1586,
                        "recepies" :
                        [
                                5
                        ],
                        "role" : "p",
                        "specialtournamentprestigetier" : 1,
                        "tournamentperformance" : 1,
                        "valuecommon" : 476,
                        "valueepic" : 0,
                        "valuerare" : 0,
                        "valueuncommon" : 0
                }
        ]
})", "foo");			
  
}

TEST_F (XayaPlayersJsonTests, TournamentInstance)
{
  auto tnm = tbl5.CreateNew ("99258908-ce4f-50e4-2880-99f0027b8d2b", ctx.RoConfig());
  int IDsr = tnm->GetId();
  tnm.reset();

  ExpectStateJson (R"({
        "tournaments" :
        [
          {
            "blocksleft" : 60,
            "blueprint" : "99258908-ce4f-50e4-2880-99f0027b8d2b",
            "state" : 0,
            "winnerid" : "",
            "results" : []
          }
        ]
        }
        )");
		
	
  ExpectStateTournamentsOnlyJson (R"({
	"fighters" : [],
	"specialtournaments" : [],
	"tournaments" : 
	[
		{
			"blocksleft" : 60,
			"blueprint" : "99258908-ce4f-50e4-2880-99f0027b8d2b",
			"fighters" : [],
			"results" : [],
			"state" : 0,
			"teamsjoined" : 0,
			"tid" : 1,
			"winnerid" : ""
		}
	]
})", "domob");	
    
}

/* ************************************************************************** */

/* ************************************************************************** */

} // anonymous namespace
} // namespace pxd

/*
    GSP for the tf blockchain game
    Copyright (C) 2019-2021  Autonomous Worlds Ltd

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

#include "pending.hpp"

#include "jsonutils.hpp"
#include "protoutils.hpp"
#include "testutils.hpp"

#include "database/xayaplayers.hpp"

#include <xayautil/jsonutils.hpp>

#include <gtest/gtest.h>

namespace pxd
{
namespace
{

/* ************************************************************************** */

class PendingStateTests : public DBTestWithSchema
{

protected:

  PendingState state;

  ContextForTesting ctx;

  XayaPlayersTable xayaplayers;

  PendingStateTests () : xayaplayers(db)
  {}

  /**
   * Expects that the current state matches the given one, after parsing
   * the expected state's string as JSON.  The comparison is done in the
   * "partial" sense.
   */
  void
  ExpectStateJson (const std::string& expectedStr)
  {
    const Json::Value actual = state.ToJson ();
    VLOG (1) << "Actual JSON for the pending state:\n" << actual;
    ASSERT_TRUE (PartialJsonEqual (actual, ParseJson (expectedStr)));
  }

};

TEST_F (PendingStateTests, Empty)
{
  ExpectStateJson (R"(
    {
      "xayaplayers": []
    }
  )");
}

TEST_F (PendingStateTests, Clear)
{
  auto a = xayaplayers.CreateNew ("domob");
  a->SetPlayerRole (PlayerRole::PLAYER);
  CoinTransferBurn coinOp;
  coinOp.minted = 5;
  coinOp.burnt = 10;
  coinOp.transfers["andy"] = 20;
  state.AddCoinTransferBurn (*a, coinOp);
  a.reset ();

  ExpectStateJson (R"(
    {
      "xayaplayers": [{}]
    }
  )");

  state.Clear ();
  ExpectStateJson (R"(
    {
      "xayaplayers": []
    }
  )");
}

TEST_F (PendingStateTests, CoinTransferBurn)
{
  auto a = xayaplayers.CreateNew ("domob");

  CoinTransferBurn coinOp;
  coinOp.minted = 32;
  coinOp.burnt = 5;
  coinOp.transfers["andy"] = 20;
  state.AddCoinTransferBurn (*a, coinOp);

  coinOp.minted = 10;
  coinOp.burnt = 2;
  coinOp.transfers["andy"] = 1;
  coinOp.transfers["daniel"] = 10;
  state.AddCoinTransferBurn (*a, coinOp);

  a.reset ();

  ExpectStateJson (R"(
    {
      "xayaplayers":
        [
          {
            "name": "domob",
            "coinops":
              {
                "minted": 42,
                "burnt": 7,
                "transfers":
                  {
                    "andy": 21,
                    "daniel": 10
                  }
              }
          }
        ]
    }
  )");
}

}


/* ************************************************************************** */

class PendingStateUpdaterTests : public PendingStateTests
{

protected:

  ContextForTesting ctx;

  /** Cost of one character.  */
  const Amount characterCost;

  PendingStateUpdaterTests ()
    : characterCost(ctx.Config ()->params ().character_cost () * COIN)
  {}

  /**
   * Processes a move for the given name and with the given move data, parsed
   * from JSON string.  If paidToDev is non-zero, then add an "out" entry
   * paying the given amount to the dev address.  If burntChi is non-zero,
   * then also a CHI burn is added to the JSON data.
   */
  void
  ProcessWithDevPaymentAndBurn (const std::string& name,
                                const Amount paidToDev, const Amount burntChi,
                                const std::string& mvStr)
  {
    Json::Value moveObj(Json::objectValue);
    moveObj["name"] = name;
    moveObj["move"] = ParseJson (mvStr);

    if (paidToDev != 0)
      moveObj["out"][ctx.Config ()->params ().dev_addr ()]
          = xaya::ChiAmountToJson (paidToDev);
    if (burntChi != 0)
      moveObj["burnt"] = xaya::ChiAmountToJson (burntChi);

    DynObstacles dyn(db, ctx);
    PendingStateUpdater updater(db, dyn, state, ctx);
    updater.ProcessMove (moveObj);
  }

  /**
   * Processes a move for the given name, data and dev payment, without burn.
   */
  void
  ProcessWithDevPayment (const std::string& name, const Amount paidToDev,
                         const std::string& mvStr)
  {
    ProcessWithDevPaymentAndBurn (name, paidToDev, 0, mvStr);
  }

  /**
   * Processes a move for the given name, data and burn.
   */
  void
  ProcessWithBurn (const std::string& name, const Amount burntChi,
                   const std::string& mvStr)
  {
    ProcessWithDevPaymentAndBurn (name, 0, burntChi, mvStr);
  }

  /**
   * Processes a move for the given name and with the given move data
   * as JSON string, without developer payment or burn.
   */
  void
  Process (const std::string& name, const std::string& mvStr)
  {
    ProcessWithDevPaymentAndBurn (name, 0, 0, mvStr);
  }
};

TEST_F (PendingStateUpdaterTests, UninitialisedAndNonExistantAccount)
{
  /* This verifies that the pending processor is fine with (i.e. does not
     crash or something like that) non-existant and uninitialised accounts.
     The pending moves (even though they might be valid in some of these cases)
     are not tracked.  */

  Process ("domob", R"({
    "vc": {"x": 5}
  })");
  Process ("domob", R"({
    "a": {"init": {"faction": "r"}}
  })");

  ASSERT_EQ (accounts.GetByName ("domob"), nullptr);
  accounts.CreateNew ("domob");

  ProcessWithDevPayment ("domob", characterCost, R"({
    "a": {"init": {"faction": "r"}},
    "nc": [{}]
  })");

  ExpectStateJson (R"(
    {
      "accounts": [],
      "characters": [],
      "newcharacters": []
    }
  )");
}

TEST_F (PendingStateUpdaterTests, CoinTransferBurn)
{
  xayaplayers.CreateNew ("domob")->AddBalance (100);
  xayaplayers.CreateNew ("andy")->AddBalance (100);

  Process ("domob", R"({
    "abc": "foo",
    "vc": {"x": 5, "b": 10}
  })");
  Process ("andy", R"({
    "vc": {"b": -10, "t": {"domob": 5, "invalid": 2}}
  })");
  Process ("invalid", R"({
    "vc": {"b": 1}
  })");
  Process ("domob", R"({
    "abc": "foo",
    "vc": {"b": 2, "t": {"domob": 1, "andy": 5}}
  })");
  Process ("andy", R"({
    "vc": {"b": 101, "t": {"domob": 101}}
  })");

  ExpectStateJson (R"(
    {
      "xayaplayers":
        [
          {
            "name": "andy",
            "coinops":
              {
                "burnt": 0,
                "transfers": {"domob": 5}
              }
          },
          {
            "name": "domob",
            "coinops":
              {
                "burnt": 12,
                "transfers": {"andy": 5}
              }
          }
        ]
    }
  )");
}

TEST_F (PendingStateUpdaterTests, Minting)
{
  xayaplayers.CreateNew ("domob");

  ProcessWithBurn ("domob", COIN, R"({
    "vc": {"m": {}}
  })");
  ProcessWithBurn ("domob", 2 * COIN, R"({
    "vc": {"m": {}}
  })");

  ExpectStateJson (R"(
    {
      "xayaplayers":
        [
          {
            "name": "domob",
            "coinops":
              {
                "minted": 30000
              }
          }
        ]
    }
  )");
}

} // namespace pxd

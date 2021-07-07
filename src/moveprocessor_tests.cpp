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

#include "moveprocessor.hpp"

#include "jsonutils.hpp"
#include "testutils.hpp"
#include "forks.hpp"

#include "database/dbtest.hpp"

#include <xayautil/jsonutils.hpp>

#include <gflags/gflags.h>
#include <gtest/gtest.h>

#include <json/json.h>

#include <string>

namespace pxd
{

namespace
{

class MoveProcessorTests : public DBTestWithSchema
{

private:

  TestRandom rnd;

protected:

  ContextForTesting ctx;
  XayaPlayersTable xayaplayers;
  
  

  explicit MoveProcessorTests ()
    : xayaplayers(db)
  {}

  /**
   * Processes an array of admin commands given as JSON string.
   */
  void
  ProcessAdmin (const std::string& str)
  {
    MoveProcessor mvProc(db, rnd, ctx);
    mvProc.ProcessAdmin (ParseJson (str));
  }

  /**
   * Processes the given data (which is passed as string and converted to
   * JSON before processing it).
   */
  void
  Process (const std::string& str)
  {
    MoveProcessor mvProc(db, rnd, ctx);
    mvProc.ProcessAll (ParseJson (str));
  }

  /**
   * Processes the given data as string, adding the given amount as payment
   * to the dev address for each entry.  This is a utility method to avoid the
   * need of pasting in the long "out" and dev-address parts.
   */
  void
  ProcessWithDevPayment (const std::string& str, const Amount amount)
  {
    Json::Value val = ParseJson (str);
    for (auto& entry : val)
      entry["out"][ctx.RoConfig()->params ().dev_addr ()]
          = xaya::ChiAmountToJson (amount);

    MoveProcessor mvProc(db, rnd, ctx);
    mvProc.ProcessAll (val);
  }

};

/* ************************************************************************** */

TEST_F (MoveProcessorTests, InvalidDataFromXaya)
{
  EXPECT_DEATH (Process ("{}"), "isArray");

  EXPECT_DEATH (Process (R"(
    [{"name": "domob"}]
  )"), "isMember.*move");

  EXPECT_DEATH (Process (R"(
    [{"move": {}}]
  )"), "nameVal.isString");
  EXPECT_DEATH (Process (R"(
    [{"name": 5, "move": {}}]
  )"), "nameVal.isString");

  EXPECT_DEATH (Process (R"([{
    "name": "domob", "move": {},
    "out": {")" + ctx.RoConfig()->params ().dev_addr () + R"(": false}
  }])"), "JSON value for amount is not double");
}

TEST_F (MoveProcessorTests, InvalidAdminFromXaya)
{
  EXPECT_DEATH (ProcessAdmin ("42"), "isArray");
  EXPECT_DEATH (ProcessAdmin ("false"), "isArray");
  EXPECT_DEATH (ProcessAdmin ("null"), "isArray");
  EXPECT_DEATH (ProcessAdmin ("{}"), "isArray");
  EXPECT_DEATH (ProcessAdmin ("[5]"), "isObject");
}

TEST_F (MoveProcessorTests, AllMoveDataAccepted)
{
  for (const auto& mvStr : {"5", "false", "\"foo\"", "{}"})
    {
      LOG (INFO) << "Testing move data (in valid array): " << mvStr;

      std::ostringstream fullMoves;
      fullMoves
          << R"([{"name": "test", "move": )"
          << mvStr
          << "}]";

      Process (fullMoves.str ());
    }
}

TEST_F (MoveProcessorTests, AllAdminDataAccepted)
{
  for (const auto& admStr : {"42", "[5]", "\"foo\"", "null"})
    {
      LOG (INFO) << "Testing admin data (in valid array): " << admStr;

      std::ostringstream fullAdm;
      fullAdm << R"([{"cmd": )" << admStr << "}]";

      ProcessAdmin (fullAdm.str ());
    }
}

/* ************************************************************************** */

using XayaPlayersUpdateTests = MoveProcessorTests;

TEST_F (XayaPlayersUpdateTests, Initialisation)
{
  Process (R"([
    {"name": "domob", "move": {"a": {"x": 42, "init": {"role": "p"}}}}
  ])");

  auto a = xayaplayers.GetByName ("domob");
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetRole (), PlayerRole::PLAYER);
}

TEST_F (XayaPlayersUpdateTests, InvalidInitialisation)
{
  EXPECT_DEATH (Process (R"([
    {"name": "domob", "move": {"a": {"init": {"x": 1, "role": "b"}}}},
    {"name": "domob", "move": {"a": {"init": {"role": "x"}}}},
    {"name": "domob", "move": {"a": {"init": {"role": "a"}}}},
    {"name": "domob", "move": {"a": {"init": {"y": 5}}}},
    {"name": "domob", "move": {"a": {"init": false}}},
    {"name": "domob", "move": {"a": 42}}
  ])"), "Invalid role value from database");
}

TEST_F (XayaPlayersUpdateTests, InitialisationOfExistingAccount)
{
  xayaplayers.CreateNew ("domob")->SetRole(PlayerRole::PLAYER);

  Process (R"([
    {"name": "domob", "move": {"a": {"init": {"role": "f"}}}}
  ])");

  auto a = xayaplayers.GetByName ("domob");
  ASSERT_TRUE (a != nullptr);
  EXPECT_EQ (a->GetRole (), PlayerRole::PLAYER);
}

/* ************************************************************************** */

class CoinOperationTests : public MoveProcessorTests
{

protected:

  /**
   * Expects that the balances of the given accounts are as stated.
   */
  void
  ExpectBalances (const std::map<std::string, Amount>& expected)
  {
    for (const auto& entry : expected)
      {
        auto a = xayaplayers.GetByName (entry.first);
        if (a == nullptr)
          ASSERT_EQ (entry.second, 0);
        else
          ASSERT_EQ (a->GetBalance (), entry.second);
      }
  }

};

TEST_F (CoinOperationTests, Invalid)
{
  xayaplayers.CreateNew ("domob")->AddBalance (100);

  Process (R"([
    {"name": "domob", "move": {"vc": 42}},
    {"name": "domob", "move": {"vc": []}},
    {"name": "domob", "move": {"vc": {}}},
    {"name": "domob", "move": {"vc": {"x": 10}}},
    {"name": "domob", "move": {"vc": {"b": -1}}},
    {"name": "domob", "move": {"vc": {"b": 1000000001}}},
    {"name": "domob", "move": {"vc": {"b": 999999999999999999}}},
    {"name": "domob", "move": {"vc": {"t": 42}}},
    {"name": "domob", "move": {"vc": {"t": "other"}}},
    {"name": "domob", "move": {"vc": {"t": {"other": -1}}}},
    {"name": "domob", "move": {"vc": {"t": {"other": 1000000001}}}},
    {"name": "domob", "move": {"vc": {"t": {"other": 1.999999}}}},
    {"name": "domob", "move": {"vc": {"t": {"other": 101}}}},
    {"name": "domob", "move": {"vc": {"m": {"x": "foo"}}}, "burnt": 1.0},
    {"name": "domob", "move": {"vc": {"m": null}}, "burnt": 1.0},
    {"name": "domob", "move": {"vc": {"m": []}}, "burnt": 1.0},
    {"name": "domob", "move": {"vc": {}}, "burnt": 1.0}
  ])");

  ExpectBalances ({{"domob", 100}, {"other", 0}});
}

TEST_F (CoinOperationTests, ExtraFieldsAreFine)
{
  xayaplayers.CreateNew ("domob")->AddBalance (100);
  Process (R"([
    {"name": "domob", "move": {"vc": {"b": 10, "x": "foo"}}}
  ])");
  ExpectBalances ({{"domob", 90}});
}

TEST_F (CoinOperationTests, BurnAndTransfer)
{
  xayaplayers.CreateNew ("domob")->AddBalance (100);

  /* Some of the burns and transfers are invalid.  They should just be ignored,
     without affecting the rest of the operation (valid parts).  */
  Process (R"([
    {"name": "domob", "move": {"vc":
      {
        "b": 10,
        "t": {"a": "invalid", "b": -5, "c": 1000, "second": 5, "third": 3}
      }}
    },
    {"name": "domob", "move": {"vc":
      {
        "b": 1000,
        "t": {"third": 2}
      }}
    },
    {"name": "second", "move": {"vc": {"b": 1}}}
  ])");

  ExpectBalances ({
    {"domob", 80},
    {"second", 4},
    {"third", 5},
  });
}

TEST_F (CoinOperationTests, BurnAll)
{
  xayaplayers.CreateNew ("domob")->AddBalance (100);
  Process (R"([
    {"name": "domob", "move": {"vc": {"b": 100}}}
  ])");
  ExpectBalances ({{"domob", 0}});
}

TEST_F (CoinOperationTests, TransferAll)
{
  xayaplayers.CreateNew ("domob")->AddBalance (100);
  Process (R"([
    {"name": "domob", "move": {"vc": {"t": {"other": 100}}}}
  ])");
  ExpectBalances ({{"domob", 0}, {"other", 100}});
}

TEST_F (CoinOperationTests, SelfTransfer)
{
  xayaplayers.CreateNew ("domob")->AddBalance (100);
  Process (R"([
    {"name": "domob", "move": {"vc": {"t": {"domob": 90, "other": 20}}}}
  ])");
  ExpectBalances ({{"domob", 80}, {"other", 20}});
}

TEST_F (CoinOperationTests, Minting)
{
  xayaplayers.CreateNew ("andy")->SetRole(PlayerRole::PLAYER);

  Process (R"([
    {"name": "domob", "move": {"vc": {"m": {}}}, "burnt": 1000000},
    {"name": "andy", "move": {"vc": {"m": {}}}, "burnt": 2.00019999}
  ])");

  ExpectBalances ({{"domob", 10'000'000'000}});

  MoneySupply ms(db);
  EXPECT_EQ (ms.Get ("burnsale"), 10'000'010'000);
}

TEST_F (CoinOperationTests, BurnsaleBalance)
{
  Process (R"([
    {"name": "domob", "move": {"vc": {"m": {}}}, "burnt": 0.1},
    {"name": "domob", "move": {"vc": {"b": 10}}, "burnt": 1}
  ])");

  ExpectBalances ({{"domob", 990}});
  EXPECT_EQ (xayaplayers.GetByName ("domob")->GetProto ().burnsale_balance (),
             1'000);
}

TEST_F (CoinOperationTests, MintBeforeBurnBeforeTransfer)
{
  Process (R"([
    {"name": "domob", "burnt": 0.01, "move": {"vc":
      {
        "m": {},
        "b": 90,
        "t": {"other": 20}
      }}
    }
  ])");

  ExpectBalances ({
    {"domob", 10},
    {"other", 0},
  });

  Process (R"([
    {"name": "domob", "burnt": 0.01, "move": {"vc":
      {
        "m": {},
        "t": {"other": 20}
      }}
    }
  ])");

  ExpectBalances ({
    {"domob", 10 + 80},
    {"other", 20},
  });

  MoneySupply ms(db);
  EXPECT_EQ (ms.Get ("burnsale"), 200);
}

TEST_F (CoinOperationTests, TransferOrder)
{
  xayaplayers.CreateNew ("domob")->AddBalance (100);

  Process (R"([
    {"name": "domob", "move": {"vc":
      {
        "t": {"z": 10, "a": 101, "middle": 99}
      }}
    }
  ])");

  ExpectBalances ({
    {"domob", 1},
    {"a", 0},
    {"middle", 99},
    {"z", 0},
  });
}

/* ************************************************************************** */

class GameStartTests : public CoinOperationTests
{

protected:

  GameStartTests ()
  {
    FLAGS_fork_height_gamestart = 100;
  }

  ~GameStartTests ()
  {
    FLAGS_fork_height_gamestart = -1;
  }

};

TEST_F (GameStartTests, Before)
{
  ctx.SetHeight (99);
  xayaplayers.CreateNew ("domob")->AddBalance (100);

  Process (R"([
    {"name": "andy", "move": {"vc": {"m": {}}}, "burnt": 1},
    {"name": "domob", "move": {"vc": {"b": 10}}},
    {"name": "domob", "move": {"vc": {"t": {"daniel": 5}}}},
    {"name": "domob", "move": {"a": {"init": {"role": "p"}}}}
  ])");

  ExpectBalances ({
    {"domob", 85},
    {"daniel", 5},
    {"andy", 10'000},
  });

  EXPECT_FALSE (xayaplayers.GetByName ("domob")->IsInitialised ());
}

TEST_F (GameStartTests, After)
{
  ctx.SetHeight (100);
  Process (R"([
    {"name": "domob", "move": {"a": {"init": {"role": "p"}}}}
  ])");

  auto a = xayaplayers.GetByName ("domob");
  EXPECT_TRUE (a->IsInitialised ());
  EXPECT_EQ (a->GetRole(), PlayerRole::PLAYER);
}

/* ************************************************************************** */

} // anonymous namespace
} // namespace pxd

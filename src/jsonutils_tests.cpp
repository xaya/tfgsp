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

#include "jsonutils.hpp"

#include "testutils.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <glog/logging.h>

#include <limits>

namespace pxd
{
namespace
{

using testing::ElementsAre;

using JsonCoordTests = testing::Test;

using CoinAmountJsonTests = testing::Test;

using QuantityJsonTests = testing::Test;


using IdFromJsonTests = testing::Test;


using IntToJsonTests = testing::Test;

TEST_F (IntToJsonTests, UInt)
{
  const Json::Value res = IntToJson (std::numeric_limits<uint32_t>::max ());
  ASSERT_TRUE (res.isUInt ());
  EXPECT_FALSE (res.isInt ());
  EXPECT_EQ (res.asUInt (), std::numeric_limits<uint32_t>::max ());
}

TEST_F (IntToJsonTests, Int)
{
  const Json::Value res = IntToJson (std::numeric_limits<int32_t>::min ());
  ASSERT_TRUE (res.isInt ());
  EXPECT_FALSE (res.isUInt ());
  EXPECT_EQ (res.asInt (), std::numeric_limits<int32_t>::min ());
}

TEST_F (IntToJsonTests, UInt64)
{
  const Json::Value res = IntToJson (std::numeric_limits<uint64_t>::max ());
  ASSERT_TRUE (res.isUInt64 ());
  EXPECT_FALSE (res.isInt64 ());
  EXPECT_FALSE (res.isUInt ());
  EXPECT_EQ (res.asUInt64 (), std::numeric_limits<uint64_t>::max ());
}

TEST_F (IntToJsonTests, Int64)
{
  const Json::Value res = IntToJson (std::numeric_limits<int64_t>::min ());
  ASSERT_TRUE (res.isInt64 ());
  EXPECT_FALSE (res.isUInt64 ());
  EXPECT_FALSE (res.isInt ());
  EXPECT_EQ (res.asInt64 (), std::numeric_limits<int64_t>::min ());
}


} // anonymous namespace
} // namespace pxd

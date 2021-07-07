/*
    GSP for the Taurion blockchain game
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

#include "roconfig.hpp"

#include <glog/logging.h>
#include <gtest/gtest.h>

namespace pxd
{
namespace
{

/* ************************************************************************** */

TEST (RoConfigTests, ConstructionWorks)
{
  *RoConfig (xaya::Chain::MAIN);
  *RoConfig (xaya::Chain::TEST);
  *RoConfig (xaya::Chain::REGTEST);
}

TEST (RoConfigTests, ProtoIsSingleton)
{
  const auto* ptr1 = &(*RoConfig (xaya::Chain::MAIN));
  const auto* ptr2 = &(*RoConfig (xaya::Chain::MAIN));
  const auto* ptr3 = &(*RoConfig (xaya::Chain::REGTEST));
  EXPECT_EQ (ptr1, ptr2);
  EXPECT_NE (ptr1, ptr3);
}

TEST (RoConfigTests, ChainDependence)
{
  const RoConfig main(xaya::Chain::MAIN);
  const RoConfig test(xaya::Chain::TEST);
  const RoConfig regtest(xaya::Chain::REGTEST);
  
  EXPECT_EQ (main->params ().dev_addr (), "DHy2615XKevE23LVRVZVxGeqxadRGyiFW4");
  EXPECT_EQ (test->params ().dev_addr (), "dSFDAWxovUio63hgtfYd3nz3ir61sJRsXn");
  EXPECT_EQ (regtest->params ().dev_addr (), "dHNvNaqcD7XPDnoRjAoyfcMpHRi5upJD7p");

  EXPECT_FALSE (main->params ().god_mode ());
  EXPECT_FALSE (test->params ().god_mode ());
  EXPECT_TRUE (regtest->params ().god_mode ());
}

} // anonymous namespace
} // namespace pxd

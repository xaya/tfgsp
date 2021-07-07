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

#include "schema.hpp"

#include "dbtest.hpp"

#include <gtest/gtest.h>

namespace pxd
{
namespace
{

using SchemaTests = DBTestFixture;

TEST_F (SchemaTests, Works)
{
  SetupDatabaseSchema (*db);
}

TEST_F (SchemaTests, TwiceIsOk)
{
  SetupDatabaseSchema (*db);
  SetupDatabaseSchema (*db);
}

} // anonymous namespace
} // namespace pxd

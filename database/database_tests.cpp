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

#include "database.hpp"

#include "dbtest.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace pxd
{
namespace
{

/**
 * Basic struct that holds data corresponding to what we store in the test
 * database table.
 */
struct RowData
{
  int64_t id;
  bool flag;
  bool nameNull;
  std::string name;
  int coordX;
  int coordY;
};

/**
 * Database result type for our test table.
 */
struct TestResult : public Database::ResultType
{
  RESULT_COLUMN (int64_t, id, 1);
  RESULT_COLUMN (bool, flag, 2);
  RESULT_COLUMN (std::string, name, 3);
};

class DatabaseTests : public DBTestFixture
{

protected:

  DatabaseTests ()
  {
    auto stmt = db.Prepare (R"(
      CREATE TABLE `test` (
        `id` INTEGER PRIMARY KEY,
        `flag` INTEGER NULL,
        `name` TEXT NULL,
        `proto` BLOB NULL
      )
    )");
    stmt.Execute ();
  }

  /**
   * Queries the data to verify that it matches the given golden values.
   */
  void
  ExpectData (const std::vector<RowData>& golden)
  {
    auto stmt = db.Prepare (R"(
      SELECT * FROM `test` ORDER BY `id` ASC
    )");
    auto res = stmt.Query<TestResult> ();
    for (const auto& val : golden)
      {
        LOG (INFO) << "Verifying golden data with ID " << val.id << "...";

        ASSERT_TRUE (res.Step ());
        EXPECT_EQ (res.Get<TestResult::id> (), val.id);
        EXPECT_EQ (res.Get<TestResult::flag> (), val.flag);
        if (res.IsNull<TestResult::name> ())
          EXPECT_TRUE (val.nameNull);
        else
          EXPECT_EQ (res.Get<TestResult::name> (), val.name);

      }
    ASSERT_FALSE (res.Step ());
  }

};



TEST_F (DatabaseTests, StatementReset)
{
  auto stmt = db.Prepare ("INSERT INTO `test` (`id`, `flag`) VALUES (?1, ?2)");

  stmt.Bind (1, 42);
  stmt.Bind (2, true);
  stmt.Execute ();

  stmt.Reset ();
  stmt.Bind (1, 50);
  /* Do not bind parameter 2, so it is NULL.  This verifies that the parameter
     bindings are reset completely.  */
  stmt.Execute ();

  stmt = db.Prepare ("SELECT `id`, `flag` FROM `test` ORDER BY `id`");
  auto res = stmt.Query<TestResult> ();

  ASSERT_TRUE (res.Step ());
  EXPECT_EQ (res.Get<TestResult::id> (), 42);
  EXPECT_EQ (res.Get<TestResult::flag> (), true);

  ASSERT_TRUE (res.Step ());
  EXPECT_EQ (res.Get<TestResult::id> (), 50);
  EXPECT_EQ (res.Get<TestResult::flag> (), false);

  ASSERT_FALSE (res.Step ());
}



TEST_F (DatabaseTests, ResultProperties)
{
  auto stmt = db.Prepare ("SELECT * FROM `test`");
  auto res = stmt.Query<TestResult> ();
  EXPECT_EQ (&res.GetDatabase (), &db);
}

} // anonymous namespace
} // namespace pxd

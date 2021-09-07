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

#include "inventory.hpp"

#include "dbtest.hpp"

#include <google/protobuf/text_format.h>

#include <gtest/gtest.h>

#include <limits>
#include <map>

namespace pxd
{
namespace
{

using google::protobuf::TextFormat;

/* ************************************************************************** */

TEST (QuantityProductTests, Initialisation)
{
  EXPECT_EQ (QuantityProduct ().Extract (), 0);
  EXPECT_EQ (QuantityProduct (1, 0).Extract (), 0);
  EXPECT_EQ (QuantityProduct (6, 7).Extract (), 42);
  EXPECT_EQ (QuantityProduct (-5, 4).Extract (), -20);
  EXPECT_EQ (QuantityProduct (1'000'000'000'000ll, -2).Extract (),
             -2'000'000'000'000ll);
}

TEST (QuantityProductTests, AddProduct)
{
  QuantityProduct total;
  total.AddProduct (6, 7);
  total.AddProduct (-2, 5);
  total.AddProduct (1'000'000'000, 1'000'000'000);
  EXPECT_EQ (total.Extract (), 42 - 10 + 1'000'000'000'000'000'000ll);
}

TEST (QuantityProductTests, Comparison)
{
  QuantityProduct pos(10, 10);
  EXPECT_TRUE (pos <= 100);
  EXPECT_TRUE (pos <= 1'000'000'000'000);
  EXPECT_FALSE (pos <= 99);
  EXPECT_FALSE (pos > 100);
  EXPECT_TRUE (pos > 99);

  QuantityProduct zero;
  EXPECT_TRUE (zero <= 0);
  EXPECT_TRUE (zero <= 100);

  QuantityProduct neg(-1, 1);
  EXPECT_TRUE (neg <= 0);
  EXPECT_TRUE (neg <= 100);
}

TEST (QuantityProductTests, Overflows)
{
  QuantityProduct pos(std::numeric_limits<int64_t>::max (),
                      std::numeric_limits<int64_t>::max ());
  EXPECT_FALSE (pos <= std::numeric_limits<int64_t>::max ());
  EXPECT_DEATH (pos.Extract (), "is too large");

  QuantityProduct neg(std::numeric_limits<int64_t>::max (),
                      -std::numeric_limits<int64_t>::max ());
  EXPECT_TRUE (neg <= 0);
  EXPECT_TRUE (neg <= std::numeric_limits<int64_t>::max ());
  EXPECT_DEATH (neg.Extract (), "is too large");
}

/* ************************************************************************** */

class InventoryTests : public testing::Test
{

protected:

  Inventory inv;

  /**
   * Expects that the elements in the fungible "map" match the given
   * set of expected elements.
   */
  void
  ExpectFungibleElements (const std::map<std::string, uint64_t>& expected)
  {
    const auto& fungible = inv.GetFungible ();
    ASSERT_EQ (expected.size (), fungible.size ());
    for (const auto& entry : expected)
      {
        const auto mit = fungible.find (entry.first);
        ASSERT_TRUE (mit != fungible.end ())
            << "Entry " << entry.first << " not found in actual data";
        ASSERT_EQ (mit->second, entry.second);
      }
  }

};

TEST_F (InventoryTests, DefaultData)
{
  ExpectFungibleElements ({});
  EXPECT_EQ (inv.GetFungibleCount ("foo"), 0);
  EXPECT_FALSE (inv.IsDirty ());
  EXPECT_EQ (inv.GetProtoForBinding ().GetSerialised (), "");
}

TEST_F (InventoryTests, FromProto)
{
  LazyProto<proto::Inventory> pb;
  pb.SetToDefault ();
  CHECK (TextFormat::ParseFromString (R"(
    fungible: { key: "foo" value: 10 }
    fungible: { key: "bar" value: 5 }
  )", &pb.Mutable ()));

  inv = std::move (pb);

  ExpectFungibleElements ({{"foo", 10}, {"bar", 5}});
  EXPECT_FALSE (inv.IsEmpty ());
}

TEST_F (InventoryTests, Modification)
{
  inv.SetFungibleCount ("foo", 10);
  inv.SetFungibleCount ("bar", 5);

  ExpectFungibleElements ({{"foo", 10}, {"bar", 5}});
  EXPECT_EQ (inv.GetFungibleCount ("foo"), 10);
  EXPECT_EQ (inv.GetFungibleCount ("bar"), 5);
  EXPECT_EQ (inv.GetFungibleCount ("baz"), 0);
  EXPECT_FALSE (inv.IsEmpty ());
  EXPECT_TRUE (inv.IsDirty ());

  inv.AddFungibleCount ("bar", 3);
  ExpectFungibleElements ({{"foo", 10}, {"bar", 8}});

  inv.SetFungibleCount ("foo", 0);
  ExpectFungibleElements ({{"bar", 8}});
  EXPECT_FALSE (inv.IsEmpty ());

  inv.AddFungibleCount ("bar", -8);
  EXPECT_TRUE (inv.IsEmpty ());
  EXPECT_TRUE (inv.IsDirty ());
}

TEST_F (InventoryTests, Equality)
{
  Inventory a, b, c;

  a.SetFungibleCount ("foo", 10);
  a.SetFungibleCount ("bar", 5);

  b.SetFungibleCount ("bar", 5);
  b.SetFungibleCount ("foo", 10);

  c.SetFungibleCount ("foo", 10);
  c.SetFungibleCount ("bar", 5);
  c.SetFungibleCount ("zerospace", 1);

  EXPECT_EQ (a, a);
  EXPECT_EQ (a, b);
  EXPECT_NE (a, c);
}

TEST_F (InventoryTests, Clear)
{
  inv.SetFungibleCount ("foo", 10);
  inv.SetFungibleCount ("bar", 5);

  /* Clearing twice should be fine (and just not have any effect).  */
  inv.Clear ();
  inv.Clear ();

  EXPECT_TRUE (inv.IsEmpty ());
  EXPECT_EQ (inv.GetFungibleCount ("foo"), 0);
}

TEST_F (InventoryTests, AdditionOfOtherInventory)
{
  inv.SetFungibleCount ("foo", 10);

  Inventory other;
  other.SetFungibleCount ("foo", 2);
  other.SetFungibleCount ("bar", 3);

  inv += other;

  EXPECT_EQ (inv.GetFungibleCount ("foo"), 12);
  EXPECT_EQ (inv.GetFungibleCount ("bar"), 3);
}

TEST_F (InventoryTests, ProtoRef)
{
  proto::Inventory pb;
  pb.mutable_fungible ()->insert ({"foo", 5});

  Inventory other(pb);
  EXPECT_EQ (other.GetFungibleCount ("foo"), 5);
  other.AddFungibleCount ("foo", -5);
  other.AddFungibleCount ("bar", 10);

  EXPECT_EQ (pb.fungible ().size (), 1);
  EXPECT_EQ (pb.fungible ().at ("bar"), 10);

  const proto::Inventory& constRef(pb);
  Inventory ro(constRef);
  EXPECT_EQ (ro.GetFungibleCount ("bar"), 10);
  EXPECT_DEATH (ro.AddFungibleCount ("foo", 1), "non-mutable");
}



} // anonymous namespace
} // namespace pxd

/*
    GSP for the Taurion blockchain game
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

#ifndef DATABASE_INVENTORY_HPP
#define DATABASE_INVENTORY_HPP

#include "database.hpp"
#include "lazyproto.hpp"

#include "proto/inventory.pb.h"

#include <google/protobuf/map.h>

#include <gmp.h>

#include <cstdint>
#include <memory>
#include <string>

namespace pxd
{

/* ************************************************************************** */

/**
 * The maximum valid value for an item quantity or dual value (such as e.g. the
 * per-unit price in a market order).  If a move contains a number
 * larger than this, it is considered invalid.  This is consensus relevant.
 *
 * But this is not only applied to moves, but checked in general for any
 * item quantity.  So it should really be the total supply limit of anything
 * in the game.
 *
 * The value chosen here should be large enough for any practical need.  It is
 * still significantly below full 64 bits, though, to give us some extra
 * headway against overflows just in case.
 */
static constexpr int64_t MAX_QUANTITY = (1ll << 50);

/** Type for the quantity of an item.  */
using Quantity = int64_t;

/**
 * Helper class to compute the inner product of vectors of quantities
 * (e.g. total weight of an inventory, or price of some order).  It uses
 * GMP bignum's internally, so that we do not run into any overflows while
 * multiplying two Quantity values.  (In the end all such products should
 * fit into 64 bits anyway, but this way we can enforce it.)
 *
 * All products of Quantity values should be computed with this class
 * rather than direct integer math.
 */
class QuantityProduct
{

private:

  /** Underlying GMP value for the running sum.  */
  mpz_t total;

public:

  /**
   * Starts with a zero value.
   */
  QuantityProduct ();

  /**
   * Initialises the value to the product of both numbers.
   */
  explicit QuantityProduct (Quantity a, Quantity b);

  ~QuantityProduct ();

  /**
   * Adds a product of two values to the running total.
   */
  void AddProduct (Quantity a, Quantity b);

  /**
   * Checks that the value is less-or-equal a given int value, e.g. to
   * compare to the total cargo space or available funds.
   */
  bool operator<= (uint64_t limit) const;

  inline bool
  operator> (uint64_t limit) const
  {
    return !(*this <= limit);
  }

  /**
   * Extracts the value as 64-bit integer.  CHECK-fails if it does not
   * fit (so only use this when it is guaranteed to fit, e.g. because
   * the inputs are known to fit always or because <= has been used already
   * to check the size).
   */
  int64_t Extract () const;

};

/* ************************************************************************** */

/**
 * Wrapper class around the state of an inventory.  This is what game-logic
 * code should use rather than plain Inventory protos.
 */
class Inventory
{

private:

  /** The underlying data if it comes from a database column.  */
  std::unique_ptr<LazyProto<proto::Inventory>> data;

  /**
   * The underlying data if it just references a proto directly, e.g. when
   * it is part of another proto.
   */
  proto::Inventory* ref;

  /** If this is a reference, whether it is mutable.  */
  bool mutableRef;

  /**
   * Returns the underlying proto as read-only data.
   */
  const proto::Inventory& Get () const;

  /**
   * Returns the underlying data for mutation.
   */
  proto::Inventory& Mutable ();

public:

  /**
   * Constructs an instance representing an empty inventory (that can then
   * be modified, for instance).
   */
  Inventory ();

  Inventory (Inventory&&) = default;

  /**
   * Constructs an instance based on the given explicit proto.  This is
   * used to wrap a raw proto not coming from a database column (e.g.
   * already part of another proto) so that it can interface with
   * code that expects an Inventory instance.
   */
  explicit Inventory (proto::Inventory& p);
  explicit Inventory (const proto::Inventory& p);

  /**
   * Constructs an instance wrapping the given proto data.
   */
  explicit Inventory (LazyProto<proto::Inventory>&& d);

  /**
   * Sets the contained inventory from the given proto.
   */
  Inventory& operator= (LazyProto<proto::Inventory>&& d);

  Inventory (const Inventory&) = delete;
  Inventory (const LazyProto<proto::Inventory>&) = delete;
  void operator= (const Inventory&) = delete;
  void operator= (const LazyProto<proto::Inventory>&) = delete;

  friend bool operator== (const Inventory& a, const Inventory& b);

  friend bool
  operator != (const Inventory& a, const Inventory& b)
  {
    return !(a == b);
  }

  /**
   * Clears the inventory completely.  This is mostly useful for testing.
   */
  void Clear ();

  /**
   * Returns the fungible inventory items as protobuf maps.  This can be
   * used to iterate over all non-zero fungible items (e.g. to construct
   * the JSON state for it).
   */
  const google::protobuf::Map<std::string, google::protobuf::uint64>&
  GetFungible () const
  {
    return Get ().fungible ();
  }

  /**
   * Returns the number of fungible items with the given key in the inventory.
   * Returns zero for non-existant items.
   */
  Quantity GetFungibleCount (const std::string& type) const;

  /**
   * Sets the number of fungible items with the given key in the inventory.
   */
  void SetFungibleCount (const std::string& type, Quantity count);

  /**
   * Updates the number of fungible items with the given key by adding
   * the given (positive or negative) amount.
   */
  void AddFungibleCount (const std::string& type, Quantity count);

  /**
   * Adds in all items from a given second inventory.
   */
  Inventory& operator+= (const Inventory& other);

  /**
   * Returns true if the inventory data has been modified (and thus needs to
   * be saved back to the database).
   */
  bool IsDirty () const;

  /**
   * Returns true if the inventory is empty.  Note that this forces the
   * proto to get parsed if it hasn't yet been.
   */
  bool IsEmpty () const;

  /**
   * Gives access to the underlying lazy proto for binding purposes.
   */
  const LazyProto<proto::Inventory>& GetProtoForBinding () const;

};

} // namespace pxd

#endif // DATABASE_INVENTORY_HPP

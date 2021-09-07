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

#include "inventory.hpp"

#include <glog/logging.h>
#include <google/protobuf/util/message_differencer.h>

#include <sstream>

using google::protobuf::util::MessageDifferencer;

namespace pxd
{

/* ************************************************************************** */

namespace
{

/**
 * Sets an mpz value from a Quantity.  The predefined conversion functions
 * work on longs and not the types like int64_t.
 */
void
MpzFromQuantity (mpz_t& out, Quantity q)
{
  const bool neg = (q < 0);
  if (neg)
    q = -q;
  CHECK_GE (q, 0);

  mpz_import (out, 1, 1, sizeof (q), 0, 0, &q);
  if (neg)
    mpz_neg (out, out);
}

} // anonymous namespace

QuantityProduct::QuantityProduct ()
{
  mpz_init (total);
}

QuantityProduct::QuantityProduct (const Quantity a, const Quantity b)
{
  mpz_init (total);
  AddProduct (a, b);
}

QuantityProduct::~QuantityProduct ()
{
  mpz_clear (total);
}

void
QuantityProduct::AddProduct (const Quantity a, const Quantity b)
{
  mpz_t aa, bb;
  mpz_inits (aa, bb, nullptr);

  MpzFromQuantity (aa, a);
  MpzFromQuantity (bb, b);

  mpz_addmul (total, aa, bb);

  mpz_clears (aa, bb, nullptr);
}

bool
QuantityProduct::operator<= (const uint64_t lim) const
{
  mpz_t ll;
  mpz_init (ll);

  mpz_import (ll, 1, 1, sizeof (lim), 0, 0, &lim);
  const int cmp = mpz_cmp (total, ll);

  mpz_clear (ll);

  return cmp <= 0;
}

int64_t
QuantityProduct::Extract () const
{
  const int64_t sgn = mpz_sgn (total);
  if (sgn == 0)
    return 0;

  /* Make sure the value actually fits, and leave some bits open just in case
     (and so there is no issue with the sign).  We won't reach the limit in
     practice anyway, so it is fine if we are a bit more strict here.  */
  CHECK_LE (mpz_sizeinbase (total, 2), 60) << "QuantityProduct is too large";

  uint64_t abs;
  size_t count;
  mpz_export (&abs, &count, 1, sizeof (abs), 0, 0, total);
  CHECK_EQ (count, 1);

  return sgn * abs;
}

/* ************************************************************************** */

Inventory::Inventory ()
  : data(std::make_unique<LazyProto<proto::Inventory>> ()), ref(nullptr)
{
  data->SetToDefault ();
}

Inventory::Inventory (proto::Inventory& p)
  : data(nullptr), ref(&p), mutableRef(true)
{}

Inventory::Inventory (const proto::Inventory& p)
  : data(nullptr), ref(const_cast<proto::Inventory*> (&p)), mutableRef(false)
{}

Inventory::Inventory (LazyProto<proto::Inventory>&& d)
  : data(std::make_unique<LazyProto<proto::Inventory>> ()), ref(nullptr)
{
  *this = std::move (d);
}

Inventory&
Inventory::operator= (LazyProto<proto::Inventory>&& d)
{
  CHECK (data != nullptr);
  *data = std::move (d);
  return *this;
}

bool
operator== (const Inventory& a, const Inventory& b)
{
  return MessageDifferencer::Equals (a.Get (), b.Get ());
}

const proto::Inventory&
Inventory::Get () const
{
  if (data != nullptr)
    return data->Get ();

  CHECK (ref != nullptr);
  return *ref;
}

proto::Inventory&
Inventory::Mutable ()
{
  if (data != nullptr)
    return data->Mutable ();

  CHECK (mutableRef) << "Inventory is a non-mutable proto reference";
  CHECK (ref != nullptr);
  return *ref;
}

void
Inventory::Clear ()
{
  Mutable ().clear_fungible ();
}

bool
Inventory::IsDirty () const
{
  CHECK (data != nullptr);
  return data->IsDirty ();
}

bool
Inventory::IsEmpty () const
{
  return Get ().fungible ().empty ();
}

Quantity
Inventory::GetFungibleCount (const std::string& type) const
{
  const auto& fungible = Get ().fungible ();
  const auto mit = fungible.find (type);
  if (mit == fungible.end ())
    return 0;
  return mit->second;
}

const LazyProto<proto::Inventory>&
Inventory::GetProtoForBinding () const
{
  CHECK (data != nullptr);
  return *data;
}

void
Inventory::SetFungibleCount (const std::string& type, const Quantity count)
{

  CHECK_GE (count, 0);
  CHECK_LE (count, MAX_QUANTITY);

  auto& fungible = *Mutable ().mutable_fungible ();

  if (count == 0)
    fungible.erase (type);
  else
    fungible[type] = count;
}

void
Inventory::AddFungibleCount (const std::string& type, const Quantity count)
{
  CHECK_GE (count, -MAX_QUANTITY);
  CHECK_LE (count, MAX_QUANTITY);

  /* Instead of getting and then setting the value using the existing methods,
     we could just query the map once and update directly.  But doing so would
     require us to duplicate some of the logic (or refactor the code), so it
     seems not worth it unless this becomes an actual bottleneck.  */

  const auto previous = GetFungibleCount (type);
  SetFungibleCount (type, previous + count);
}

Inventory&
Inventory::operator+= (const Inventory& other)
{
  for (const auto& entry : other.GetFungible ())
    AddFungibleCount (entry.first, entry.second);

  return *this;
}

} // namespace pxd

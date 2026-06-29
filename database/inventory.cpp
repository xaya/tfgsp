/*
    GSP for the TF blockchain game
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

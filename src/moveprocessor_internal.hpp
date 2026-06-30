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

#ifndef PXD_MOVEPROCESSOR_INTERNAL_HPP
#define PXD_MOVEPROCESSOR_INTERNAL_HPP

/* Small file-local helpers shared by the moveprocessor translation units
   (moveprocessor.cpp and its per-domain splits). Header-inline so every TU
   builds the deterministic dedup / key-sort / goody-lookup one way. */

#include "database/inventory.hpp"
#include "database/reward.hpp"
#include "proto/goody.pb.h"

#include <fpm/fixed.hpp>

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace pxd
{

/** DoS caps on attacker-controlled move arrays (Pass B: H1, H2, H6).
    Legitimate moves are far below these; the caps only bound malicious inputs
    so a single gas-bounded move cannot impose super-linear per-block work. */
inline constexpr unsigned MAX_MOVE_ARRAY = 1000;
inline constexpr unsigned MAX_SUBMOVES = 100;

/** OVF-01: upper bound on a single transfigure candy amount (the move's
    {"ic":[{"a":N,...}]}).  N is fed into fpm::fixed_24_8, whose int32 backing
    store computes raw = N*256 and overflows (signed-overflow UB) at N == 2^23
    (8'388'608).  Cap well below that (256M raw, ~8x margin) so neither the
    per-candy construction nor the deduplicated-per-type candyRarityAccumulated
    sum can overflow.  Legitimate transfigures use tens of candy; this only
    rejects absurd attacker inputs.  Mirrors the EXCH-1 seller-payout fix. */
inline constexpr int32_t MAX_TRANSFIGURE_CANDY = 1'000'000;

/* Dedups a vector in place (the same algorithm the move parsers used inline)
   and reports whether any duplicate was removed, so duplicate-id moves are
   rejected identically across every call site.  */
template <typename T>
bool
HasDuplicates (std::vector<T>& vec)
{
  const int32_t sizeBefore = vec.size ();
  auto end = vec.end ();
  for (auto it = vec.begin (); it != end; ++it)
    end = std::remove (it + 1, end, *it);
  vec.erase (end, vec.end ());
  const int32_t sizeAfter = vec.size ();
  return sizeBefore != sizeAfter;
}

/* Copies a protobuf map<string, T> into a vector<pair> sorted by key, so the
   deterministic key ordering the consensus paths rely on is built one way.  */
template <typename MapT>
std::vector<std::pair<std::string, typename MapT::mapped_type>>
SortPairsByKey (const MapT& m)
{
  std::vector<std::pair<std::string, typename MapT::mapped_type>> sorted;
  for (auto itr = m.begin (); itr != m.end (); ++itr)
    sorted.push_back (*itr);
  std::sort (sorted.begin (), sorted.end (),
             [] (const std::pair<std::string, typename MapT::mapped_type>& a,
                 const std::pair<std::string, typename MapT::mapped_type>& b)
             {
               return a.first < b.first;
             });
  return sorted;
}

/* Finds the first owned goody of the given type in key-sorted order and, when
   present, records its inventory key name and reduction percent.  */
inline void
FindApplicableGoody (const Inventory& inventory, const pxd::GoodyType goodyType,
                     const std::vector<std::pair<std::string, pxd::proto::Goody>>& sortedGoodies,
                     std::string& weHaveApplibeGoodyName,
                     fpm::fixed_24_8& reductionPercent)
{
  for (const auto& goodie : sortedGoodies)
  {
    if (inventory.GetFungibleCount (goodie.first) > 0)
    {
      if (goodie.second.goodytype () == (int8_t) goodyType)
      {
        weHaveApplibeGoodyName = goodie.first;
        reductionPercent = fpm::fixed_24_8::from_raw_value (goodie.second.reductionpercent ());
        break;
      }
    }
  }
}

} // namespace pxd

#endif // PXD_MOVEPROCESSOR_INTERNAL_HPP

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

#ifndef DATABASE_SPECIALTOURNAMENT_HPP
#define DATABASE_SPECIALTOURNAMENT_HPP


#include "database.hpp"
#include "inventory.hpp"
#include "lazyproto.hpp"

#include "proto/specialtournament_blueprint.pb.h"
#include "proto/roconfig.hpp"

#include <functional>
#include <memory>
#include <string>

namespace pxd
{
    
/**
 * Current special tournament state
 */
enum class SpecialTournamentState : int8_t
{
  Listed = 0,
  Calculating = 1
};

/**
 * Database result type for rows from the tournament table.
 */
struct SpecialTournamentResult : public Database::ResultType
{
  RESULT_COLUMN (int64_t, id, 1);
  RESULT_COLUMN (int32_t, tier, 2);
  RESULT_COLUMN (pxd::proto::SpecialTournamentBlueprint, proto, 3);
};

/**
 * Wrapper class for the state of one special tournament tiers.  This connects the actual game
 * logic (reading the state and doing modifications to it) from the database.
 * All interpretation of database results and upates to the database are done
 * through this class.
 *
 * This class should not be instantiated directly by users.  Instead, the
 * methods from SpecialTournamentTable should be used.  Furthermore, variables should
 * be of type SpecialTournamentTable::Handle (or using auto) to get move semantics.
 */
class SpecialTournamentInstance
{

private:

  /** The underlying integer ID in the database.  */
  Database::IdT id;

  /** The UniqueHandles tracker for this instance.  */
  Database::HandleTracker tracker;

  /** All other data in the protocol buffer.  */
  LazyProto<proto::SpecialTournamentBlueprint> data;

  /**
   * Set to true if any modification to the non-proto columns was made that
   * needs to be synced back to the database in the destructor.
   */
  bool dirtyFields;
    
  /** Whether or not this is a new instance.  */
  bool isNew;

  /** Tier for easi unit-tests queries.  */
  int32_t tier;

  /** Database reference this belongs to.  */
  Database& db; 

  /**
   * Constructs a new tournament with an auto-generated ID meant to be inserted
   * into the database.
   */
  explicit SpecialTournamentInstance (Database& d, const int32_t& _tier, const RoConfig& cfg);

  /**
   * Constructs a tournament instance based on the given query result.  This
   * represents the data from the result row but can then be modified.  The
   * result should come from a query made through TournamentTable.
   */
  explicit SpecialTournamentInstance (Database& d, const Database::Result<SpecialTournamentResult>& res);

  /**
   * Binds parameters in a statement to the mutable non-proto fields.  This is
   * to share code between the proto and non-proto updates.  The ID is always
   * bound to parameter ?1, and other fields to following integer IDs.
   *
   * The immutable non-proto field faction is also not bound
   * here, since it is only present in the INSERT OR REPLACE statement
   * (with proto update) and not the UPDATE one.
   */
  void BindFieldValues (Database::Statement& stmt) const;

  friend class SpecialTournamentTable;

protected:

  void Validate () const;

public:

  /**
   * In the destructor, the underlying database is updated if there are any
   * modifications to send.
   */
  ~SpecialTournamentInstance ();

  SpecialTournamentInstance () = delete;
  SpecialTournamentInstance (const SpecialTournamentInstance&) = delete;
  void operator= (const SpecialTournamentInstance&) = delete;

  /* Accessor methods.  */

  Database::IdT
  GetId () const
  {
    return id;
  }

  const proto::SpecialTournamentBlueprint&
  GetProto () const
  {
    return data.Get ();
  }
  
  proto::SpecialTournamentBlueprint&
  MutableProto ()
  {
    return data.Mutable ();
  }
};

/**
 * Utility class that handles querying the tournament table in the database and
 * should be used to obtain Special Tournament instances (or rather, the underlying
 * Database::Result's for them).
 */
class SpecialTournamentTable
{

private:

  /** The Database reference for creating queries.  */
  Database& db;

public:

  /** Movable handle to a tournament instance.  */
  using Handle = std::unique_ptr<SpecialTournamentInstance>;

  explicit SpecialTournamentTable (Database& d)
    : db(d)
  {}

  SpecialTournamentTable () = delete;
  SpecialTournamentTable (const SpecialTournamentTable&) = delete;
  void operator= (const SpecialTournamentTable&) = delete;

  /**
   * Returns a SpecialTournamentInstance handle for a fresh instance corresponding to a new
   * tournament that will be created.
   */
  Handle CreateNew (const int32_t& _tier, const RoConfig& cfg);

  /**
   * Returns a handle for the instance based on a Database::Result.
   */
  Handle GetFromResult (const Database::Result<SpecialTournamentResult>& res, const RoConfig& cfg);

  /**
   * Returns the special tournament with the given tier level
   */  
  Handle GetByTier (const int32_t _tier, const RoConfig& cfg);  

  /**
   * Returns the tournament with the given ID or a null handle if there is
   * none with that ID.
   */
  Handle GetById (Database::IdT id, const RoConfig& cfg);
  
  /**
   * Queries for all tournaments in the database table. The tournaments are
   * ordered by ID to make the result deterministic.
   */
  Database::Result<SpecialTournamentResult> QueryAll ();
};

} // namespace pxd

#endif // DATABASE_SPECIALTOURNAMENT_HPP
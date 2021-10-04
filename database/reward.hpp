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

#ifndef DATABASE_REWARD_HPP
#define DATABASE_REWARD_HPP


#include "database.hpp"
#include "inventory.hpp"
#include "lazyproto.hpp"

#include "proto/activity_reward_instance.pb.h"
#include "proto/roconfig.hpp"

#include <functional>
#include <memory>
#include <string>

namespace pxd
{

/**
 * Type of the reward issuer
 */
enum class RewardSource : int8_t
{
  None = 0,
  Tournament = 1,
  Expedition = 2,
  Sweetener
};    

/**
 * Type of goody applied to ungoing operation
 */
enum class GoodyType : int8_t
{
  None = 0,
  PressureCooker = 1,
  Espresso = 2
};

enum class RewardType : int8_t
{
  Candy = 0,
  GeneratedRecipe = 1,
  CraftedRecipe = 2,
  Move = 3,
  Armor = 4,
  Animation = 5,
  List = 6,
  None = 7
};

/**
 * Database result type for rows from the rewards table.
 */
struct RewardInstanceResult : public Database::ResultType
{
  RESULT_COLUMN (int64_t, id, 1);
  RESULT_COLUMN (std::string, owner, 2);
  RESULT_COLUMN (pxd::proto::ActivityRewardInstance, proto, 3);
};

/**
 * Wrapper class for the state of one rewards instance.  This connects the actual game
 * logic (reading the state and doing modifications to it) from the database.
 * All interpretation of database results and upates to the database are done
 * through this class.
 *
 * This class should not be instantiated directly by users.  Instead, the
 * methods from RewardsTable should be used.  Furthermore, variables should
 * be of type RewardsTable::Handle (or using auto) to get move semantics.
 */
class RewardInstance
{

private:

  /** The underlying integer ID in the database.  */
  Database::IdT id;

  /** The UniqueHandles tracker for this instance.  */
  Database::HandleTracker tracker;

  /** All other data in the protocol buffer.  */
  LazyProto<proto::ActivityRewardInstance> data;

  /**
   * Set to true if any modification to the non-proto columns was made that
   * needs to be synced back to the database in the destructor.
   */
  bool dirtyFields;
  
  /** The owner string.  */
  std::string owner;  
  
  /** Whether or not this is a new instance.  */
  bool isNew;

  /** Database reference this belongs to.  */
  Database& db;  
  
  /**
   * Constructs a new reward with an auto-generated ID meant to be inserted
   * into the database.
   */
  explicit RewardInstance (Database& d, const std::string& o);

  /**
   * Constructs a reward instance based on the given query result.  This
   * represents the data from the result row but can then be modified.  The
   * result should come from a query made through RewardsTable.
   */
  explicit RewardInstance (Database& d,
                      const Database::Result<RewardInstanceResult>& res);

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

  friend class RewardsTable;

protected:

  void Validate () const;

  bool
  IsDirtyCombatData () const
  {
    return data.IsDirty ();
  }

public:

  /**
   * In the destructor, the underlying database is updated if there are any
   * modifications to send.
   */
  ~RewardInstance ();

  RewardInstance () = delete;
  RewardInstance (const RewardInstance&) = delete;
  void operator= (const RewardInstance&) = delete;

  /* Accessor methods.  */

  Database::IdT
  GetId () const
  {
    return id;
  }

  const proto::ActivityRewardInstance&
  GetProto () const
  {
    return data.Get ();
  }
  
  const std::string&
  GetOwner () const
  {
    return owner;
  }

  void
  SetOwner (const std::string& o)
  {
    dirtyFields = true;
    owner = o;
  }  

  proto::ActivityRewardInstance&
  MutableProto ()
  {
    return data.Mutable ();
  }
};

/**
 * Utility class that handles querying the rewards table in the database and
 * should be used to obtain RewardInstance instances (or rather, the underlying
 * Database::Result's for them).
 */
class RewardsTable
{

private:

  /** The Database reference for creating queries.  */
  Database& db;

public:

  /** Movable handle to a reward instance.  */
  using Handle = std::unique_ptr<RewardInstance>;

  explicit RewardsTable (Database& d)
    : db(d)
  {}

  RewardsTable () = delete;
  RewardsTable (const RewardsTable&) = delete;
  void operator= (const RewardsTable&) = delete;

  /**
   * Returns a RewardInstance handle for a fresh instance corresponding to a new
   * reward that will be created.
   */
  Handle CreateNew (const std::string& owner);

  /**
   * Returns a handle for the instance based on a Database::Result.
   */
  Handle GetFromResult (const Database::Result<RewardInstanceResult>& res);

  /**
   * Returns a handle for the instance based on a Database::Result, with config to match with template 
     function in gamestate.cpp
   */
  Handle GetFromResult (const Database::Result<RewardInstanceResult>& res, const RoConfig& cfg);

  /**
   * Returns the reward with the given ID or a null handle if there is
   * none with that ID.
   */
  Handle GetById (Database::IdT id);

  /**
   * Queries for all rewards in the database table.  The rewards are
   * ordered by ID to make the result deterministic.
   */
  Database::Result<RewardInstanceResult> QueryAll ();
  
  /**
   * Queries for all characters with a given owner, ordered by ID.
   */
  Database::Result<RewardInstanceResult> QueryForOwner (const std::string& owner);  
  
  /**
   * Counts for all rewards with a given owner
   */  
  unsigned CountForOwner (const std::string& owner);

  /**
   * Deletes the reward with the given ID.
   */
  void DeleteById (Database::IdT id);
};

} // namespace pxd

#endif // DATABASE_REWARD_HPP
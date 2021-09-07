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

#ifndef DATABASE_ACTIVITY_HPP
#define DATABASE_ACTIVITY_HPP


#include "database.hpp"
#include "inventory.hpp"
#include "lazyproto.hpp"

#include "proto/activity.pb.h"
#include "proto/roconfig.hpp"

#include <functional>
#include <memory>
#include <string>

namespace pxd
{

/**
 * Database result type for rows from the activity table.
 */
struct ActivityResult : public Database::ResultType
{
  RESULT_COLUMN (int64_t, id, 1);
  RESULT_COLUMN (std::string, owner, 2);
  RESULT_COLUMN (pxd::proto::Activity, proto, 3);

};

/**
 * Wrapper class for the state of one activity.  This connects the actual game
 * logic (reading the state and doing modifications to it) from the database.
 * All interpretation of database results and upates to the database are done
 * through this class.
 *
 * This class should not be instantiated directly by users.  Instead, the
 * methods from ActivityTable should be used.  Furthermore, variables should
 * be of type ActivityTable::Handle (or using auto) to get move semantics.
 */
class ActivityInstance
{

private:

  /** The underlying integer ID in the database.  */
  Database::IdT id;

  /** The UniqueHandles tracker for this instance.  */
  Database::HandleTracker tracker;

  /** All other data in the protocol buffer.  */
  LazyProto<proto::Activity> data;

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
   * Constructs a new activity with an auto-generated ID meant to be inserted
   * into the database.
   */
  explicit ActivityInstance (Database& d, const std::string& o, const uint32_t start_block, const uint32_t duration, const std::string& name, const std::string& related_item_guid, Database::IdT related_item_or_class_id);

  /**
   * Constructs a activity instance based on the given query result.  This
   * represents the data from the result row but can then be modified.  The
   * result should come from a query made through ActivityTable.
   */
  explicit ActivityInstance (Database& d,
                      const Database::Result<ActivityResult>& res);

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

  friend class ActivityTable;

protected:

  void Validate () const;

  bool
  IsDirtyActivityData () const
  {
    return data.IsDirty ();
  }

public:

  /**
   * In the destructor, the underlying database is updated if there are any
   * modifications to send.
   */
  ~ActivityInstance ();

  ActivityInstance () = delete;
  ActivityInstance (const ActivityInstance&) = delete;
  void operator= (const ActivityInstance&) = delete;

  /* Accessor methods.  */

  Database::IdT
  GetId () const
  {
    return id;
  }

  const proto::Activity&
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

  proto::Activity&
  MutableProto ()
  {
    return data.Mutable ();
  }
};

/**
 * Utility class that handles querying the activity table in the database and
 * should be used to obtain Activity instances (or rather, the underlying
 * Database::Result's for them).
 */
class ActivityTable
{

private:

  /** The Database reference for creating queries.  */
  Database& db;

public:

  /** Movable handle to a activity instance.  */
  using Handle = std::unique_ptr<ActivityInstance>;

  explicit ActivityTable (Database& d)
    : db(d)
  {}

  ActivityTable () = delete;
  ActivityTable (const ActivityTable&) = delete;
  void operator= (const ActivityTable&) = delete;

  /**
   * Returns a Activity handle for a fresh instance corresponding to a new
   * activity that will be created.
   */
  Handle CreateNew (const std::string& owner, const uint32_t start_block, const uint32_t duration, const std::string& name, const std::string& related_item_guid, Database::IdT related_item_or_class_id);

  /**
   * Returns a handle for the instance based on a Database::Result.
   */
  Handle GetFromResult (const Database::Result<ActivityResult>& res, const RoConfig& cfg);

  /**
   * Returns the activity with the given ID or a null handle if there is
   * none with that ID.
   */
  Handle GetById (Database::IdT id, const RoConfig& cfg);

  /**
   * Queries for all activities in the database table.  The activities are
   * ordered by ID to make the result deterministic.
   */
  Database::Result<ActivityResult> QueryAll ();
  
  /**
   * Queries for all activities with a given owner, ordered by ID.
   */
  Database::Result<ActivityResult> QueryForOwner (const std::string& owner);  

  /**
   * Deletes the activity with the given ID.
   */
  void DeleteById (Database::IdT id);
};

} // namespace pxd

#endif // DATABASE_ACTIVITY_HPP
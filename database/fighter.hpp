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

#ifndef DATABASE_FIGHTER_HPP
#define DATABASE_FIGHTER_HPP


#include "database.hpp"
#include "inventory.hpp"
#include "lazyproto.hpp"

#include "proto/fighter.pb.h"
#include "proto/roconfig.hpp"

#include <functional>
#include <memory>
#include <string>

namespace pxd
{
    
/**
 * Current sweetness of the fighter
 */
enum class Sweetness : int8_t
{
  Any = 0,
  Not_Too_Sweet = 1,
  Bittersweet = 2,
  Semi_Sweet = 3,
  Sweet_and_Sour = 4,
  Salty_Sweet = 5,
  Artificially_Sweet = 6,
  Sticky_Sweet = 7,
  Sugary_Sweet = 8,
  Pretty_Sweet = 9,
  Super_Sweet = 10
};

/**
 * Current status of the fighter
 */
enum class SpecialTournamentStatus : int8_t
{
  Listed = 0,
  Lost = 1,
  Won = 2
};

/**
 * Current status of the fighter
 */
enum class FighterStatus : int8_t
{
  Available = 0,
  Expedition = 1,
  Tournament = 2,
  Exchange = 3,
  Cooking = 4,
  Deconstructing = 5,
  SpecialTournament = 6,
  Cooked = 7
};

/**
 * A type of fighters move 
 */
enum class MoveType : int8_t
{
  Heavy = 0,
  Speedy = 1,
  Tricky = 2,
  Distance = 3,
  Blocking = 4
};

/**
 * A type of fighters armor piece
 */
enum class ArmorType : int8_t
{
  None = 0,
  Head = 1,
  RightShoulder = 2,
  LeftShoulder = 3,
  UpperLeftArm = 4,
  UpperRightArm = 5,
  LowerLeftArm = 6,
  LowerRightArm = 7,
  RightHand = 8,
  LeftHand = 9,
  Torso = 10,
  Waist = 11,
  UpperRightLeg = 12,
  UpperLeftLeg = 13,
  LowerRightLeg = 14,
  LowerLeftLeg = 15,
  RightFoot = 16,
  LeftFoot = 17,
  Back = 18
};

/**
 * Database result type for rows from the fighter table.
 */
struct FighterResult : public Database::ResultType
{
  RESULT_COLUMN (int64_t, id, 1);
  RESULT_COLUMN (std::string, owner, 2);
  RESULT_COLUMN (pxd::proto::Fighter, proto, 3);
  RESULT_COLUMN (int64_t, status, 4);
};

/**
 * Retrieves a fighter status from a database column.  This function verifies that
 * the database value represents a valid faction.  Otherwise it crashes the
 * process (data corruption).
 *
 * This is templated, so that it can accept different database result types.
 */
template <typename T>
  FighterStatus GetStatusFromColumn (const Database::Result<T>& res);


/**
 * Binds a status value to a status parameter.
 */
void BindStatusParameter (Database::Statement& stmt, unsigned ind, FighterStatus f);

/**
 * Wrapper class for the state of one fighter.  This connects the actual game
 * logic (reading the state and doing modifications to it) from the database.
 * All interpretation of database results and upates to the database are done
 * through this class.
 *
 * This class should not be instantiated directly by users.  Instead, the
 * methods from FighterTable should be used.  Furthermore, variables should
 * be of type FighterTable::Handle (or using auto) to get move semantics.
 */
class FighterInstance
{

private:

  /** The underlying integer ID in the database.  */
  Database::IdT id;

  /** The UniqueHandles tracker for this instance.  */
  Database::HandleTracker tracker;

  /** All other data in the protocol buffer.  */
  LazyProto<proto::Fighter> data;

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
  
  /** Current fighter status */
  FighterStatus status;

  /**
   * Constructs a new fighter with an auto-generated ID meant to be inserted
   * into the database.
   */
  explicit FighterInstance (Database& d, const std::string& o, uint32_t r, const RoConfig& cfg, xaya::Random& rnd);

  /**
   * Constructs a fighter instance based on the given query result.  This
   * represents the data from the result row but can then be modified.  The
   * result should come from a query made through FighterTable.
   */
  explicit FighterInstance (Database& d,
                      const Database::Result<FighterResult>& res, const RoConfig& cfg);

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

  friend class FighterTable;

protected:

  void Validate () const;

  bool
  IsDirtyCombatData () const
  {
    return data.IsDirty ();
  }

public:

  /**
   * Helper function for fighter calculation during 
   */
  uint32_t GetRatingFromQuality (uint32_t quality);
  
  /**
   * Calculates new sweetness based on the rating
   */  
  void UpdateSweetness();

  /**
   * In the destructor, the underlying database is updated if there are any
   * modifications to send.
   */
  ~FighterInstance ();

  FighterInstance () = delete;
  FighterInstance (const FighterInstance&) = delete;
  void operator= (const FighterInstance&) = delete;

  /* Accessor methods.  */

  Database::IdT
  GetId () const
  {
    return id;
  }

  const FighterStatus
  GetStatus () const
  {
    return status;
  }

  void
  SetStatus (const FighterStatus newStatus)
  {
    dirtyFields = true;
    status = newStatus;
  } 

  const proto::Fighter&
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

  proto::Fighter&
  MutableProto ()
  {
    return data.Mutable ();
  }
  
private:

  /**
   * Returns list of armor type pieces for the certain move type
   */  
  std::vector<pxd::ArmorType> ArmorTypeFromMoveType(pxd::MoveType moveType);
};

/**
 * Utility class that handles querying the fighter table in the database and
 * should be used to obtain Fighter instances (or rather, the underlying
 * Database::Result's for them).
 */
class FighterTable
{

private:

  /** The Database reference for creating queries.  */
  Database& db;

public:

  /** Movable handle to a fighter instance.  */
  using Handle = std::unique_ptr<FighterInstance>;

  explicit FighterTable (Database& d)
    : db(d)
  {}

  FighterTable () = delete;
  FighterTable (const FighterTable&) = delete;
  void operator= (const FighterTable&) = delete;

  /**
   * Returns a Fighter handle for a fresh instance corresponding to a new
   * fighter that will be created.
   */
  Handle CreateNew (const std::string& owner, uint32_t recipe, const RoConfig& cfg, xaya::Random& rnd);

  /**
   * Returns a handle for the instance based on a Database::Result.
   */
  Handle GetFromResult (const Database::Result<FighterResult>& res, const RoConfig& cfg);

  /**
   * Returns the fighter with the given ID or a null handle if there is
   * none with that ID.
   */
  Handle GetById (Database::IdT id, const RoConfig& cfg);

  /**
   * Queries for all fighters in the database table.  The fighters are
   * ordered by ID to make the result deterministic.
   */
  Database::Result<FighterResult> QueryAll ();
  
  /**
   * Queries for all characters with a given owner, ordered by ID.
   */
  Database::Result<FighterResult> QueryForOwner (const std::string& owner);  
  
  /**
   * Counts for all fighters with a given owner
   */  
  unsigned CountForOwner (const std::string& owner);

  /**
   * Deletes the fighter with the given ID.
   */
  void DeleteById (Database::IdT id);
};

} // namespace pxd

#include "fighter.tpp"

#endif // DATABASE_FIGHTER_HPP
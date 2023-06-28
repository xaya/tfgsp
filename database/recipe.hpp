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

#ifndef DATABASE_RECIPE_HPP
#define DATABASE_RECIPE_HPP


#include "database.hpp"
#include "inventory.hpp"
#include "lazyproto.hpp"

#include "proto/crafted_recipe.pb.h"
#include "proto/roconfig.hpp"

#include <functional>
#include <memory>
#include <string>

namespace pxd
{
    
/**
 * Current quality of the recepie we are going to generate
 */
enum class Quality : int8_t
{
  None = 0,
  Common = 1,
  Uncommon = 2,
  Rare = 3,
  Epic = 4
};

/**
 * Database result type for rows from the recipes table.
 */
struct RecipeInstanceResult : public Database::ResultType
{
  RESULT_COLUMN (int64_t, id, 1);
  RESULT_COLUMN (std::string, owner, 2);
  RESULT_COLUMN (pxd::proto::CraftedRecipe, proto, 3);
};

/**
 * Wrapper class for the state of one recepies.  This connects the actual game
 * logic (reading the state and doing modifications to it) from the database.
 * All interpretation of database results and upates to the database are done
 * through this class.
 *
 * This class should not be instantiated directly by users.  Instead, the
 * methods from RecipeTable should be used.  Furthermore, variables should
 * be of type RecipeTable::Handle (or using auto) to get move semantics.
 */
class RecipeInstance
{

private:

  /** The underlying integer ID in the database.  */
  Database::IdT id;

  /** The UniqueHandles tracker for this instance.  */
  Database::HandleTracker tracker;

  /** All other data in the protocol buffer.  */
  LazyProto<proto::CraftedRecipe> data;

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
   * Constructs a new recipe with an auto-generated ID meant to be inserted
   * into the database.
   */
  explicit RecipeInstance (Database& d, const std::string& o, const std::string& cr, const RoConfig& cfg);
  
  /**
   * Constructs a new recipe with an auto-generated ID meant to be inserted
   * into the database based on the generated recipe instance
   */
  explicit RecipeInstance (Database& d, const std::string& o, const pxd::proto::CraftedRecipe cr, const RoConfig& cfg);  

  /**
   * Constructs a recepie instance instance based on the given query result. This
   * represents the data from the result row but can then be modified.  The
   * result should come from a query made through RecipeInstanceTable.
   */
  explicit RecipeInstance (Database& d, const Database::Result<RecipeInstanceResult>& res);

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

  friend class RecipeInstanceTable;

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
  ~RecipeInstance ();

  RecipeInstance () = delete;
  RecipeInstance (const RecipeInstance&) = delete;
  void operator= (const RecipeInstance&) = delete;

  /* Accessor methods.  */

  Database::IdT
  GetId () const
  {
    return id;
  }

  const proto::CraftedRecipe&
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

  proto::CraftedRecipe&
  MutableProto ()
  {
    return data.Mutable ();
  }
  
  static uint32_t Generate(pxd::Quality quality, const RoConfig& cfg,  xaya::Random& rnd, Database& db, std::string owner, bool fork);
};

/**
 * Utility class that handles querying the recipe table in the database and
 * should be used to obtain Recipe instances (or rather, the underlying
 * Database::Result's for them).
 */
class RecipeInstanceTable
{

private:

  /** The Database reference for creating queries.  */
  Database& db;

public:

  /** Movable handle to a recipe instance.  */
  using Handle = std::unique_ptr<RecipeInstance>;

  explicit RecipeInstanceTable (Database& d)
    : db(d)
  {}

  RecipeInstanceTable () = delete;
  RecipeInstanceTable (const RecipeInstanceTable&) = delete;
  void operator= (const RecipeInstanceTable&) = delete;

  /**
   * Returns a RecipeInstance handle for a fresh instance corresponding to a new
   * recipe that will be created.
   */
  Handle CreateNew (const std::string& owner, const std::string& cr, const RoConfig& cfg);
  
  /**
   * Returns a RecipeInstance handle for a fresh instance corresponding to a new
   * recipe that will be created.
   */
  Handle CreateNew (const std::string& owner, const pxd::proto::CraftedRecipe cr, const RoConfig& cfg);  

  /**
   * Returns a handle for the instance based on a Database::Result.
   */
  Handle GetFromResult (const Database::Result<RecipeInstanceResult>& res);
  
  /**
   * Returns a handle for the instance based on a Database::Result. Version with config, to make it
     compatible with gamestate.cpp template function
   */
  Handle GetFromResult (const Database::Result<RecipeInstanceResult>& res, const RoConfig& cfg);
  

  /**
   * Returns the recepie with the given ID or a null handle if there is
   * none with that ID.
   */
  Handle GetById (Database::IdT id);

  /**
   * Queries for all recepies in the database table.  The recepies are
   * ordered by ID to make the result deterministic.
   */
  Database::Result<RecipeInstanceResult> QueryAll ();
  
  /**
   * Queries for all characters with a given owner, ordered by ID.
   */
  Database::Result<RecipeInstanceResult> QueryForOwner (const std::string& owner);  

  /**
   * Deletes the recipe with the given ID.
   */
  void DeleteById (Database::IdT id);
  
  
  /**
   * Counts for all recepies with a given owner
   */  
  unsigned CountForOwner (const std::string& owner);  
  
  /**
   * Counts for all recepies in database table
   */  
  unsigned CountForAll ();    
};

} // namespace pxd

#endif // DATABASE_RECIPE_HPP
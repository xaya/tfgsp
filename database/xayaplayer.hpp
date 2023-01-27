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

#ifndef DATABASE_XAYAPLAYER_HPP
#define DATABASE_XAYAPLAYER_HPP

#include "database.hpp"
#include "amount.hpp"
#include "fighter.hpp"
#include "recipe.hpp"
#include "tournament.hpp"
#include "inventory.hpp"
#include "lazyproto.hpp"
#include "proto/roconfig.hpp"

#include "proto/xaya_player.pb.h"

#include <memory>
#include <string>

namespace pxd
{
/**
 * Ongoing operation type which we resolve once its blockcount hits zero
 */
enum class OngoingType : int8_t
{
  INVALID = 0,
  COOK_RECIPE = 1,
  EXPEDITION = 2,
  DECONSTRUCTION = 3,
  COOK_SWEETENER = 4
};

/**
 * A player role in the game
 */
enum class PlayerRole : int8_t
{
  INVALID = 0,
  PLAYER = 1,
  ROLEADMIN = 2,
  CONENTADMIN = 3,
  CONFIGADMIN = 4,
  EXCHANGEADMIN = 8,
  ADMINISTRATOR = 15
};

/**
 * A database result that includes a "role" column.
 */
struct ResultWithRole : public Database::ResultType
{
  RESULT_COLUMN (int64_t, role, 50);
};


/**
 * Retrieves a faction from a database column.  This function verifies that
 * the database value represents a valid faction.  Otherwise it crashes the
 * process (data corruption).
 *
 * This is templated, so that it can accept different database result types.
 * They should all be derived from ResultWithFaction, though.
 */
template <typename T>
  PlayerRole GetPlayerRoleFromColumn (const Database::Result<T>& res);

/**
 * Retrieves a faction from a database column, which can also be NULL.
 * In the case of NULL, Faction::INVALID is returned.  Any other
 * value (i.e. non-matching integer values) will CHECK-fail.
 */
template <typename T>
  PlayerRole GetNullablePlayerRoleFromColumn (const Database::Result<T>& res);

/**
 * Database result type for rows from the accounts table.
 */
struct XayaPlayerResult : public ResultWithRole
{
  RESULT_COLUMN (std::string, name, 1);
  RESULT_COLUMN (pxd::proto::XayaPlayer, proto, 2);
  RESULT_COLUMN (pxd::proto::Inventory, inventory, 3);
  RESULT_COLUMN (int64_t, prestige, 4);
};


/**
 * Converts the faction to a string.  This is used for logging and other
 * messages, as well as in the JSON format of game states.
 */
std::string PlayerRoleToString (PlayerRole f);

/**
 * Parses a faction value from a string.  Returns INVALID if the string does
 * not represent any of the real factions.
 */
PlayerRole PlayerRoleFromString (const std::string& str);

/**
 * Binds a role value to a role parameter. 
 */
void BindPlayerRoleParameter (Database::Statement& stmt, unsigned ind, PlayerRole f);

/**
 * Wrapper class around the state of one Xaya account (name) in the database.
 * Instantiations of this class should be made through the AccountsTable.
 */
class XayaPlayer
{

private:

  /** Database reference this belongs to.  */
  Database& db;

  /** The Xaya name of this account.  */
  std::string name;

  /** UniqueHandles tracker for this instance.  */
  Database::HandleTracker tracker;

  /** The role of this account.  May be INVALID if not yet initialised.  */
  PlayerRole role;
   
  /** General proto data.  */
  LazyProto<proto::XayaPlayer> data;
  
  /** The character's inventory.  */
  Inventory inv;  

  /** Total prestige of this account*/
  int64_t prestige;

  /** Whether or not this is dirty in the fields (like faction).  */
  bool dirtyFields;

  /**
   * Constructs an instance with "default / empty" data for the given name
   * and not-yet-set faction.
   */
  explicit XayaPlayer (Database& d, const std::string& n, const RoConfig& cfg, xaya::Random& rnd);

  /**
   * Constructs an instance based on the given DB result set.  The result
   * set should be constructed by an AccountsTable.
   */
  explicit XayaPlayer (Database& d, const Database::Result<XayaPlayerResult>& res, const RoConfig& cfg);

  friend class XayaPlayersTable;       

public:

  /**
   * In the destructor, the underlying database is updated if there are any
   * modifications to send.
   */
  ~XayaPlayer ();

  XayaPlayer () = delete;
  XayaPlayer (const XayaPlayer&) = delete;
  void operator= (const XayaPlayer&) = delete;

  void SetRole (PlayerRole f);

  const std::string&
  GetName () const
  {
    return name;
  }

  int64_t
  GetPresitge () const
  {
    return prestige;
  }

  PlayerRole
  GetRole () const
  {
    return role;
  }

  const bool
  GetIsMine ();

  std::string
  SendCHI (std::string address, Amount amount);
  
  const bool
  HasRole (PlayerRole role); 
  
  const bool
  GrantRole (PlayerRole role);  

  const bool
  RevokeRole (PlayerRole role);   

  const proto::XayaPlayer&
  GetProto () const
  {
    return data.Get ();
  }

  proto::XayaPlayer&
  MutableProto ()
  {
    return data.Mutable ();
  }
  
  uint32_t GetOngoingsSize()
  {
      return GetProto().ongoings().size();
  }

  /**
   * Updates the account balance by the given (signed) amount.  This should
   * be used instead of manually editing the proto, so that there is a single
   * place that controls all balance updates.
   */
  void AddBalance (Amount val);

  Amount
  GetBalance () const
  {
    return data.Get ().balance ();
  }
  
  const Inventory&
  GetInventory () const
  {
    return inv;
  }

  Inventory&
  GetInventory ()
  {
    return inv;
  }

  /*below are functions for ineracting with PROTO static data and player inventory to
  fetch all kind of differnt intems in posession information*/
  
  std::vector<FighterTable::Handle> CollectInventoryFighters(const RoConfig& cfg);
  std::vector<FighterTable::Handle> CollectInventoryFightersFromSpecialTournament(const RoConfig& cfg, uint32_t specialTournamentTier);
  std::vector<TournamentTable::Handle> CollectTournaments(const RoConfig& cfg);
  
  /*This is calculated every block based on all the different assets 
  player contains globally for the player name*/
  
  void CalculatePrestige(const RoConfig& cfg);

private:
   
  /*Helper utility function used in perstige calculations*/
  double GetFighterPercentageFromQuality(uint32_t quality, std::vector<FighterTable::Handle>& fighters);
  
  /*We are not writing those in the database, so we need to create them on each 
  time we load account or create fresh*/
    
  uint32_t recipe_slots;
  uint32_t roster_slots;
};

/**
 * Utility class that handles querying the accounts table in the database and
 * should be used to obtain XayaPlayer instances.
 */
class XayaPlayersTable
{

private:

  /** The Database reference for creating queries.  */
  Database& db;

public:

  /** Movable handle to an account instance.  */
  using Handle = std::unique_ptr<XayaPlayer>;

  explicit XayaPlayersTable (Database& d)
    : db(d)
  {}

  XayaPlayersTable () = delete;
  XayaPlayersTable (const XayaPlayersTable&) = delete;
  void operator= (const XayaPlayersTable&) = delete;

  /**
   * Creates a new entry in the database for the given name.
   * Calling this method for a name that already has an account is an error.
   */
  Handle CreateNew (const std::string& name, const RoConfig& cfg, xaya::Random& rnd);

  /**
   * Returns a handle for the instance based on a Database::Result.
   */
  Handle GetFromResult (const Database::Result<XayaPlayerResult>& res, const RoConfig& cfg);

  /**
   * Returns the account with the given name.
   */
  Handle GetByName (const std::string& name, const RoConfig& cfg);
  
  /**
   * Queries the database for particular account
   */
  Database::Result<XayaPlayerResult> QueryForOwner (const std::string& owner);

  /**
   * Queries the database for all accounts, including uninitialised ones.
   */
  Database::Result<XayaPlayerResult> QueryAll ();

  /**
   * Queries the database for all accounts which have been initialised yet
   * with a faction.  Returns a result set that can be used together with
   * GetFromResult.
   */
  Database::Result<XayaPlayerResult> QueryInitialised ();

};

} // namespace pxd

#include "xayaplayer.tpp"

#endif // DATABASE_XAYAPLAYER_HPP

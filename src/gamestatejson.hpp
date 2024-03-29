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

#ifndef PXD_GAMESTATEJSON_HPP
#define PXD_GAMESTATEJSON_HPP

#include "context.hpp"
#include "database/database.hpp"
#include <json/json.h>

namespace pxd
{

/**
 * Utility class that handles construction of game-state JSON.
 */
class GameStateJson
{

private:

  /** Database to read from.  */
  Database& db;

  /** Current parameter context.  */
  const Context& ctx;

  /**
   * Extracts all results from the Database::Result instance, converts them
   * to JSON, and returns a JSON array.
   */
  template <typename T, typename R>
    Json::Value ResultsAsArray(T& tbl, Database::Result<R> res) const;
	

public:

  explicit GameStateJson(Database& d, const Context& c)
    : db(d), ctx(c)
  {}

  GameStateJson () = delete;
  GameStateJson (const GameStateJson&) = delete;
  void operator= (const GameStateJson&) = delete;
  
  /** To track against the forks, lets send those to 
  front-end to compare **/
  
  inline static std::string latestKnownStateHash = "";
  inline static uint64_t latestKnownStateBlock = 0;     
  
  /**
   * Returns the JSON data representing all accounts in the game state.
   */
  Json::Value XayaPlayers();  
  
  /**
   * Returns the JSON data representing all activities in the game state.
   */
  Json::Value Activities();   
  
  /**
   * Returns the JSON data representing all crystal bundle keys available for the game in the game proto
   */  
  
  Json::Value CrystalBundles(); 
  
  /**
   * Returns the JSON data about money supply and burnsale stats.
   */
  Json::Value MoneySupply();  
  
  /**
   * Returns the JSON data about fighters.
   */
  Json::Value Fighters();   

  /**
   * Returns the JSON data about rewards.
   */
  Json::Value Rewards(); 
  
  /**
   * Returns the JSON data about recepies.
   */
  Json::Value Recepies(); 

  /**
   * Returns the JSON data about tournaments.
   */
  Json::Value Tournaments();   

  /**
   * Returns the JSON data about special tournaments.
   */
  Json::Value SpecialTournaments();  
  
  /**
   * All user relevant data, excluding tournaments
   */
  Json::Value User(const std::string& userName);
  
  /**
   * Tournaments and special tournaments data exluding user data treats and irrelevant treats
   */
  Json::Value UserTournaments(const std::string& userName);  
  
  /**
   * Fighters exclusive to any exchange operations
   */
  Json::Value Exchange();    
  
  /**
   * Returns the full game state JSON for the given Database handle.  The full
   * game state as JSON should mainly be used for debugging and testing, not
   * in production.  For that, more targeted RPC results should be used.
   */
  Json::Value FullState();

  /**
   * Converts a state instance (like a Character or Region) to the corresponding
   * JSON value in the game state.
   */
  template <typename T>
    Json::Value Convert(const T& val) const;
    

};

} // namespace pxd

#endif // PXD_GAMESTATEJSON_HPP

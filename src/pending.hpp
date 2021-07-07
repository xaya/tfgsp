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

#ifndef PXD_PENDING_HPP
#define PXD_PENDING_HPP

#include "context.hpp"
#include "moveprocessor.hpp"

#include <xayagame/sqlitegame.hpp>

#include <json/json.h>

#include <memory>
#include <string>
#include <vector>

namespace pxd
{

class PXLogic;

/**
 * The state of pending moves for a tf game.  (This holds just the
 * state and manages updates as well as JSON conversion, without being
 * the PendingMoveProcessor itself.)
 */
class PendingState
{

public:

  PendingState () = default;

  PendingState (const PendingState&) = delete;
  void operator= (const PendingState&) = delete;

  /**
   * Clears all pending state and resets it to "empty" (corresponding to a
   * situation without any pending moves).
   */
  void Clear ();
  
   /**
   * Updates the state for a new coin transfer / burn.
   */
  void AddCoinTransferBurn (const XayaPlayer& a, const CoinTransferBurn& op);  
  
  /**
   * Pending state updates associated to an account.
   */
  struct XayaPlayerState
  {

    /** The combined coin transfer / burn for this account.  */
    std::unique_ptr<CoinTransferBurn> coinOps;


    /**
     * Returns the JSON representation of the pending state.
     */
    Json::Value ToJson () const;

  };  
  
  /** Pending updates by account name.  */
  std::map<std::string, XayaPlayerState> xayaplayers;  
  
  /**
   * Returns the pending state for the given account instance, creating a new
   * (empty) one if there is not already one.
   */
  XayaPlayerState& GetXayaPlayerState (const XayaPlayer& a);  

  /**
   * Returns the JSON representation of the pending state.
   */
  Json::Value ToJson () const;

};

/**
 * BaseMoveProcessor class that updates the pending state.  This contains the
 * main logic for PendingMoves::AddPendingMove, and is also accessible from
 * the unit tests independently of SQLiteGame.
 *
 * Instances of this class are light-weight and just contain the logic.  They
 * are created on-the-fly for processing a single move.
 */
class PendingStateUpdater : public BaseMoveProcessor
{

private:

  /** The PendingState instance that is updated.  */
  PendingState& state;

public:

  explicit PendingStateUpdater (Database& d,
                                PendingState& s, const Context& c)
    : BaseMoveProcessor(d, c), state(s)
  {}

  /**
   * Processes the given move.
   */
  void ProcessMove (const Json::Value& moveObj);

};

/**
 * Processor for pending moves in tf.  This keeps track of some information
 * that we use in the frontend, like the modified waypoints of characters
 * and creation of new characters.
 */
class PendingMoves : public xaya::SQLiteGame::PendingMoves
{

private:

  /** The current state of pending moves.  */
  PendingState state;

protected:

  void Clear () override;
  void AddPendingMove (const Json::Value& mv) override;

public:

  explicit PendingMoves (PXLogic& rules);

  Json::Value ToJson () const override;

};

} // namespace pxd

#endif // PXD_PENDING_HPP

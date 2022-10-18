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
   * Updates the state for a new crystal bundle purchase.
   */  
  void AddCrystalPurchase (const XayaPlayer& a, std::string crystalBundleKey, Amount crystalAmount);
  
   /**
   * Updates the state for a new recepie cooking action
   */    
  void AddRecepieCookingInstance (const XayaPlayer& a, int32_t duration, int32_t recepieID, Amount cookingCost, std::map<std::string, pxd::Quantity> fungibleItemAmountForDeduction); 
  
   /**
   * Updates the state for a new recepie destroy action
   */    
  void AddRecepieDestroyInstance (const XayaPlayer& a, int32_t duration, std::vector<uint32_t>& recepieIDS); 
    
  /**
   * Updates the state for a new sweetener cooking action
   */    
  void AddSweetenerCookingInstance (const XayaPlayer& a, const std::string sweetenerKeyName, int32_t duration, int32_t fighterID, Amount cookingCost, std::map<std::string, pxd::Quantity> fungibleItemAmountForDeduction); 
  
  /**
   * Updates the state for a cooked sweetener being claimed back on fighter
   */  
  void AddClaimingSweetenerReward (const XayaPlayer& a, const std::string sweetenerAuthId, int32_t fighterID);
  
   /**
   * Updates the state for a new recepie instance bundle
   */    
  void AddExpeditionInstance (const XayaPlayer& a, int32_t duration, std::string expeditionID, std::vector<int> fighterID);  

   /**
   * Updates the state for a collecting cooked fighter recepie
   */    
  void AddCookedRecepieCollectInstance(const XayaPlayer& a, int32_t fighterToCollectID);     
  
   /**
   * Updates the state for a new pending rewards
   */      
  void AddRewardIDs (const XayaPlayer& a, std::vector<std::string> expeditionIDArray, std::vector<uint32_t> rewardDatabaseIds);
  
   /**
   * Updates the state for a new pending rewards
   */      
  void AddTournamentRewardIDs (const XayaPlayer& a, uint32_t tournamentID, std::vector<uint32_t> rewardDatabaseIds);  
  
   /**
   * Updates the state for a new pending tournament entries
   */      
  void AddTournamentEntries (const XayaPlayer& a, uint32_t tournamentID, std::vector<uint32_t> fighterIDS);
  
   /**
   * Updates the state for a new pending special tournament entries
   */      
  void AddSpecialTournamentEntries (const XayaPlayer& a, uint32_t tournamentID, std::vector<uint32_t> fighterIDS);  
  
   /**
   * Updates the state for a new pending tournament leaves
   */      
  void AddTournamentLeaves (const XayaPlayer& a, uint32_t tournamentID, std::vector<uint32_t> fighterIDS);

   /**
   * Updates the state for a new pending special tournament leaves
   */      
  void AddSpecialTournamentLeaves (const XayaPlayer& a, uint32_t tournamentID, std::vector<uint32_t> fighterIDS);    
  
    /**
   * Updates the state for a new pending purchases of fungible items
   */      
  void AddPurchasing (const XayaPlayer& a, std::string authID, Amount purchaseCost);
  
   /**
   * Updates the state for a new fighter deconstruction
   */      
  void AddDeconstructionData (const XayaPlayer& a, uint32_t fighterID);   
  
   /**
   * Updates the state for a deconstructed fighter reward claiming
   */     
  void AddDeconstructionRewardData (const XayaPlayer& a, uint32_t fighterID);
  
   /**
   * Updates the state for a fighter being bought
   */      
  void AddFighterForBuyData (const XayaPlayer& a, uint32_t fighterID, Amount exchangeprice);

   /**
   * Updates the state for a fighter being sold
   */      
  void AddFighterForSaleData (const XayaPlayer& a, uint32_t fighterID, Amount listingfee);

   /**
   * Updates the state for a fighter being removed from sale
   */      
  void RemoveFromSaleData (const XayaPlayer& a, uint32_t fighterID); 
  
  /**
   * Pending state updates associated to an account.
   */
  struct XayaPlayerState
  {

    /** The combined coin transfer / burn for this account.  */
    std::unique_ptr<CoinTransferBurn> coinOps;

    /** List of crystal purchases, pending to be bought*/
    std::vector<std::string> crystalpurchace;
    
    /** Current crystal balance on the player */
    Amount balance;  

    /** Current crystal balance not of the pending, but one that comes from a.GetBalance() */
    Amount onChainBalance;      
    
    /** Current fungibles in the player inventory*/
    std::map<std::string, uint64_t> currentFungibleSet;
    
    /** Fungibles in the original player inventory*/
    std::map<std::string, uint64_t> onChainFungibleSet;

    /** List of currently pending ongoing operations, which are going
    to take more then 1 block to finish*/
    
    std::vector<proto::OngoinOperation> ongoings;

    /**
     * Returns the JSON representation of the pending state.
     */
    Json::Value ToJson () const;
    
    /** IDs of rewards currently pending for claiming */
    std::map<std::string, std::vector<uint32_t>> rewardDatabaseIds;
    
    /** IDs of tournaments rewards currently pending for claiming */
    std::map<uint32_t, std::vector<uint32_t>> rewardDatabaseIdsTournaments;    
    
    /** List of the tournament entries which are pending for the
        not yet started tournaments to enter
    */
    std::map<uint32_t, std::vector<uint32_t>> tournamentEntries;
    
    /** List of the special tournament entries which are pending for the
        not yet started tournaments to enter
    */
    std::map<uint32_t, std::vector<uint32_t>> specialTournamentEntries;    
    
    /** Pending ids of tournaments player currently is trying
        to leave out
    */    
    std::map<uint32_t, std::vector<uint32_t>> tournamentLeaves;
    
    /** Pending ids of special tournaments player currently is trying
        to leave out
    */    
    std::map<uint32_t, std::vector<uint32_t>> specialTournamentLeaves;    
    
    /** Pending ids of fighters player currently  being deconstructed
    */    
    std::vector<uint32_t> deconstructionData;    
    
    /** Pending ids of fighters claiming deonstruction data
    */    
    std::vector<uint32_t> deconstructionDataClaiming; 

    /** Pending ids of fighters being putted for sale
    */    
    std::vector<uint32_t> fightingForSale;

    /** Pending ids of fighters being putted for buying
    */    
    std::vector<uint32_t> fightingForBuy;  
    
    /** Pending ids of fighters being putted for removed from sale
    */    
    std::vector<uint32_t> fightingRemoveSale;     
    
    /** Pending authids of currently items purchasing
    */    
    std::vector<std::string> purchasing;   
 
    /** IAuth ids of sweeteners being claimed */
    std::vector<std::string> sweetenerClaimingAuthIds;
    
    /** IAuth ids of sweetener fighters attached when being claimed */
    std::vector<std::uint32_t> sweetenerClaimingFightersIds;    
    
    /** Pending ids of fighters ready to collect
    */    
    std::vector<int32_t> cookedFightersToCollect;    
    
    /** Pending ids of recepies ready to destroy
    */    
    std::vector<int32_t> destroyrecipe;      
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

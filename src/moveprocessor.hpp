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

#ifndef PXD_MOVEPROCESSOR_HPP
#define PXD_MOVEPROCESSOR_HPP

#include "context.hpp"
#include "proto/config.pb.h"

#include <fpm/fixed.hpp>
#include <fpm/math.hpp>

#include "database/database.hpp"
#include "database/xayaplayer.hpp"
#include "database/fighter.hpp"
#include "database/tournament.hpp"
#include "database/recipe.hpp"
#include "database/reward.hpp"
#include "database/ongoings.hpp"
#include "database/amount.hpp"
#include "database/globaldata.hpp"
#include "database/params.hpp"

#include <xayautil/random.hpp>

#include <json/json.h>

#include <map>
#include <string>
#include <vector>

namespace pxd
{

/**
 * Base class for MoveProcessor (handling confirmed moves) and PendingProcessor
 * (for processing pending moves).  It holds some common stuff for both
 * as well as some basic logic for processing some moves.
 */
class BaseMoveProcessor
{

protected:

  /** Processing context data.  */
  const Context& ctx;

  /**
   * The Database handle we use for making any changes (and looking up the
   * current state while validating moves).
   */
  Database& db;

  /**
   * When true, this processor runs in read-only mode against the confirmed
   * state (the pending-move path).  Move-validation side effects that would
   * write to the confirmed database are then suppressed.  The confirmed-move
   * path leaves this false, so its state transitions are unchanged.
   */
  const bool readOnly;
  
  /** Access handle for the accounts database table.  */
  XayaPlayersTable xayaplayers;  
  
  /** Access handle for the recepies database table.  */
  RecipeInstanceTable recipeTbl;    
   
  /** Access handle for the fighters database table.  */
  FighterTable fighters;    
  
  /** Access handle for the rewards database table.  */
  RewardsTable rewards;

  /** Access handle for the height-keyed ongoing-operations table.  */
  OngoingsTable ongoings;

  /** Access handle tournaments database instances*/
  TournamentTable tournamentsTbl;     
  
  /** Access handle for global data*/
  GlobalData globalData;       
  
  explicit BaseMoveProcessor (Database& d, const Context& c, bool ro);

  /**
   * Parses some basic stuff from a move JSON object.  This extracts the
   * actual move JSON value, the name and the dev payment.  The function
   * returns true if the extraction went well so far and the move
   * may be processed further.
   */
  bool ExtractMoveBasics (const Json::Value& moveObj,
                          std::string& name, Json::Value& mv,
                          Amount& paidToDev);
                          
                       
                          
  /**
   * Tries to handle a move that purchases crystals
   */                       
  void TryCrystalPurchase (const std::string& name, const Json::Value& mv, Amount& paidToDev);

  /**
   * Tries to purchase a name reroll for the treat
   */
  void TryNameRerollPurchase (const std::string& name, const Json::Value& mv, Amount& paidToDev, const RoConfig& cfg, xaya::Random& rnd);

  /**
   * Tries to handle a move that purchases goody
   */                       
  void TryGoodyPurchase (const std::string& name, const Json::Value& mv); 

  /**
   * Tries to handle a move that purchases sweetener
   */                       
  void TrySweetenerPurchase (const std::string& name, const Json::Value& mv);     

  /**
   * Tries to handle a move that purchases goody bundle
   */                       
  void TryGoodyBundlePurchase (const std::string& name, const Json::Value& mv);      

  /**
   * Tries to parse a move that purchases crystals up to the point where actual handling happens
   * The idea that we can safely use this inside pending parser too to validate everything properly
   */      
  bool ParseCrystalPurchase(const Json::Value& mv, std::string& bundleKeyName, Amount& cost, Amount& crystalAmount, const std::string& name, const Amount& paidToDev);
  
  /**
   * Tries to parse a move that rerolls name with a certain probability once per treat
   */      
  bool ParseNameRerollPurchase(const Json::Value& mv, int64_t& treatId, Amount& cost, const std::string& name, const Amount& paidToDev);
    
  /**
   * Tries to parse a move that purchases goody
   */       
   
  bool ParseGoodyPurchase(const Json::Value& mv, Amount& cost, const std::string& name, std::string& fungibleName, Amount balance, uint64_t& uses);
  
  /**
   * Tries to parse a move that purchases sweetener
   */       
   
  bool ParseSweetenerPurchase(const Json::Value& mv, Amount& cost, const std::string& name, std::string& fungibleName, Amount balance, Amount& total);  
    
  /**
   * Tries to parse a move that puts fighter on the auction
   */       
    
  bool ParseFighterForSaleData(const XayaPlayer& a, const Json::Value& fighter, uint32_t& fighterID, 
  uint32_t& duration, Amount& price, Amount& listingfee);
      
  /**
   * Tries to parse a move that purchases goody bundle
   */       
   
  bool ParseGoodyBundlePurchase(const Json::Value& mv, Amount& cost, const std::string& name, std::map<std::string, uint64_t>& fungibles, Amount balance);    
  
  /**
   * Parses move that claims cooked sweetener rewards
   */    
  
      
  /**
   * Tries to parse a move that sets recepie for cooking
   */       
   
  bool ParseCookRecepie(const XayaPlayer& a, const std::string& name, const Json::Value& recepie, std::map<std::string, pxd::Quantity>& fungibleItemAmountForDeduction, int32_t& cookCost, int32_t& duration, std::string& weHaveApplibeGoodyName);
  
  /**
   * Tries to parse a move that sets recepie for destroying
   */       
   
  bool ParseDestroyRecepie(const XayaPlayer& a, const std::string& name, const Json::Value& recepie, std::vector<uint32_t>& recepieIDS);
  
  /**
   * Tries to parse a move that transfiures fighter
   */       
   
  bool ParseTransfigureData(const XayaPlayer& a, const Json::Value& fighter);

   /**
   * Tries to parse a move that send fighter on the expedition
   */

  bool ParseExpeditionData(const XayaPlayer& a, const Json::Value& expedition, pxd::proto::ExpeditionBlueprint& expeditionBlueprint, std::vector<int32_t>& fightersIds, int32_t& duration, std::string& weHaveApplibeGoodyName);
 
   /**
   * Tries to parse a move that send fighters on the tournament
   */ 
 
  bool ParseTournamentEntryData(const XayaPlayer& a, const Json::Value& tournament, uint32_t& tournamentID, std::vector<uint32_t>& fighterIDS);
 
  
 
  /**
   * Tries to parse a move that deconstructs fighter
   */ 
 
  bool ParseDeconstructData(const XayaPlayer& a, const Json::Value& fighter, uint32_t& fighterID);
  
  /**
   * Tries to parse a move sweetener fungible that upgrades fighter
   */ 
   
  bool ParseSweetener(const XayaPlayer& a, const std::string& name, const Json::Value& sweetener, 
  std::map<std::string, pxd::Quantity>& fungibleItemAmountForDeduction, int32_t& cookCost, uint32_t& fighterID, 
  int32_t& duration, std::string& sweetenerKeyName);
 
  /**
   * Tries to parse a move that buys fighter from the exchange 
   */ 
 
  bool ParseBuyData(const XayaPlayer& a, const Json::Value& fighter, uint32_t& fighterID, Amount& exchangeprice);

  /**
   * Tries to parse a move that removes fighter from the exchange 
   */ 
 
  bool ParseRemoveBuyData(const XayaPlayer& a, const Json::Value& fighter, uint32_t& fighterID);    
  
  /**
   * Tries to parse a move that claims deconstruction rewards
   */ 
 
    
   /**
   * Tries to parse a move that withdraws fighters from the tournament
   */ 
 
  bool ParseTournamentLeaveData(const XayaPlayer& a, const std::string& name, const Json::Value& tournament, uint32_t& tournamentID, std::vector<uint32_t>& fighterIDS);
     
          
   /**
   */    
   
  
   /**
   */    
   
  
  /**
   * Function checks if fungible item is inside players inventory
   */  
  bool InventoryHasItem(const std::string& itemKeyName, const Inventory& inventory, const google::protobuf::uint64 amount);  
  
  /* Thin wrapper over pxd::ArmorTypesForMoveType (database/fighter.hpp); helps transfigure resolve
     which armor slots a move type governs. */
  std::vector<pxd::ArmorType> ArmorTypeFromMoveType(pxd::MoveType moveType);
  
public:

  /** Utility functions to help converting between authID and keyNames.
   *  Ultimately, we need to get rid of authID, as its seems redundant at
   *  this point, but need to keep for now to be more consistant with the
   *  original source */     
  pxd::RecipeInstanceTable::Handle GetRecepieObjectFromID(const uint32_t& ID);
  static std::string GetCandyKeyNameFromID(const std::string& authID, const Context& ctx);
  /* Crystal cost to cook a recipe of the given quality (1..4), or -1 if the quality is out of range.
     One source of truth for both the charge (ParseCookRecepie) and the refund (ResolveCookingRecepie)
     so the two can never drift into an over/under-refund. */
  static Amount RecipeCookCostForQuality(int32_t quality, const RoConfig& cfg);
  static Json::Value EvaluateFuelList(const Json::Value& fightersSubmited, const Json::Value& recipesSubmited, const Json::Value& candiesSubmited, const Json::Value& fightersNew, const Json::Value& recipesNew, const Json::Value& candiesNew, const Json::Value wholeFightersData,  const Json::Value wholeRecipeData, const Json::Value& candylist);
  /* P1-01 RPC DoS guard: "" when the getfueldata request is within the
     MAX_FUEL_* element caps (moveprocessor_internal.hpp), else a description
     of the violated cap.  pxrpcserver rejects with invalid-params BEFORE the
     O(candidates x submitted-items) EvaluateFuelList work; parameters mirror
     the RPC method's order.  RPC-only, non-consensus. */
  static std::string FuelRequestCapError(const Json::Value& candiesNew, const Json::Value& candiesSubmited, const Json::Value& candylist, const Json::Value& fighterData, const Json::Value& fightersNew, const Json::Value& fightersSubmited, const Json::Value& recipeData, const Json::Value& recipesNew, const Json::Value& recipesSubmited);

  virtual ~BaseMoveProcessor () = default;

  BaseMoveProcessor () = delete;
  BaseMoveProcessor (const BaseMoveProcessor&) = delete;
  void operator= (const BaseMoveProcessor&) = delete;

};

/**
 * Class that handles processing of all moves made in a block.
 */
class MoveProcessor : public BaseMoveProcessor
{

private:

  /** Handle for random numbers.  */
  xaya::Random& rnd;

  /**
   * Processes the move corresponding to one transaction.
   */
  void ProcessOne (const Json::Value& moveObj);

  /**
   * Processes one admin command.
   */
  void ProcessOneAdmin (const Json::Value& cmd);

  /**
   * Handles a god-mode admin command, if any.  These are used only for
   * integration testing, so that this will only be done on regtest.
   */
  void HandleGodMode (const Json::Value& cmd);

  /**
   * Handles a runtime-parameter admin command (the `param` verb).  Unlike
   * god-mode this is NOT gated on REGTEST -- it is live on POLYGON, authorised
   * solely by ownership of the g/tf admin name (libxayagame only delivers admin
   * moves from that name).  Parses an array of {n:string, v:int|null} ops and
   * applies range-guarded SetParam; unknown names and out-of-range values are
   * ignored (soft-fail, never CHECK), and removal of the required knobs is
   * refused (GetParam CHECK-fails on unset).
   */
  void HandleSetParam (const Json::Value& cmd);

  /*Global data command to set up using regtest commands on in production using xaya owned game name*/
  void MaybeSetNewCostMultiplier (const Json::Value& cmd);
  void MaybeSetNewVersionIdentifier (const Json::Value& cmd);
  void MaybeSetNewVanillaDownloadUrl (const Json::Value& cmd);

  /**
   * FORK/DEV ONLY: instantly grant a fighter of an arbitrary quality to a player,
   * so duels can be tested with verb-carrying Uncommon/Rare/Epic treats that would
   * otherwise take the full sweetness climb to earn.  Dispatched from
   * ProcessOneAdmin ONLY when the env var TF_ENABLE_GRANT is set -- never in the
   * production image -- so it is unreachable on POLYGON and inert-and-deterministic
   * there (every node ignores it identically; the goldenreplay build gate runs with
   * the var unset, so golden is byte-unchanged).  Command shape:
   *   {"cmd":{"god":{"grantfighter":{"to":"<name>","quality":1-4,"count":1-50}}}}
   * Reuses the EXACT production generation path (RecipeInstance::Generate ->
   * FighterTable::CreateNew rolls quality-appropriate moves), then consumes the
   * scaffolding recipe so no phantom is left.  Soft-fails (LOG + skip), never
   * CHECK -- an admin move must not be able to crash the daemon.
   */
  void MaybeGrantFighter (const Json::Value& cmd);
  
  /* Copy from logic.cpp, to make sure we recalc after user buys from exchange */
  
  
   /**
   * Tries to deconstruct the given fighter from the move.
   */
  void MaybeDeconstructFighter(const std::string& name, const Json::Value& fighter);  
  
   /**
   * Actual claiming logic, as this is called from multiple places, we
   * seperate into own function
   */  
  
   /**
   */
  
   /**
   * Tries to handle an account initialisation (setting the payout address)
   * from the given move.
   */
  void MaybeInitXayaPlayer (const std::string& name, const Json::Value& init);
  
  
   /**
   * Tries to put the fighter for sale on the exchange
   */  
  void MaybePutFighterForSale (const std::string& name, const Json::Value& fighter);
     
     
   /**
   * Tries to put the fighter for sale on the exchange
   */  
  void MaybeBuyFighterFromExchange(const std::string& name, const Json::Value& fighter);  

   /**
   * Tries to remove the fighter from the exchange sale
   */  
  void MaybeRemoveFighterFromExchange(const std::string& name, const Json::Value& fighter);     

   /**
   * Tries to transifure fighter in necrotorium
   */  
  void MaybeTransfigureFighter (const std::string& name, const Json::Value& fighter);   
  
  /**
  * Tries to cook recepie instance, optionally with the fighter attached
  */  
  void MaybeCookRecepie (const std::string& name, const Json::Value& recepie);
  
  /**
  * Tries to destroy recepie instance
  */  
  void MaybeDestroyRecepie (const std::string& name, const Json::Value& recepie);

  /**
  */

  /**
  * Tries to cook sweetener instance
  */
  void MaybeCookSweetener (const std::string& name, const Json::Value& sweetener);
  
  /**
  * Tries to send the fighter for the expedition
  */    
  void MaybeGoForExpedition (const std::string& name, const Json::Value& expedition);
  
  /**
  */    
  
  /**
  */    
  
  /**
  * Tries to send the fighters for the tournament
  */    
  void MaybeEnterTournament (const std::string& name, const Json::Value& tournament);  
  
  /**
  * Tries to withdraw the fighters for the tournament
  */    
  void MaybeLeaveTournament (const std::string& name, const Json::Value& tournament);    
  
  /**
   * Tries to handle a move that updates an account.
   */
  void TryXayaPlayerUpdate (const std::string& name, const Json::Value& upd);
  
  /**
   * Tries to process all kind of cooking actions
   */
  void TryCookingAction (const std::string& name, const Json::Value& upd);
  
  /**
   * Tries to process all kind of actions related to expeditions
   */  
  void TryExpeditionAction (const std::string& name, const Json::Value& upd);
  
  /**
   * Tries to process all kind of actions related to tournaments
   */  
  void TryTournamentAction (const std::string& name, const Json::Value& upd);

  /**
   * Tries to process all kind of actions related to fighters
   */  
  void TryFighterAction (const std::string& name, const Json::Value& upd);    
  
  /**
   * Destroys all submited resources
   */		
  void DestroyUsedElements(std::unique_ptr<pxd::XayaPlayer>& player, const Json::Value& fighter);	  
                
public:

  /**
   * Evaluates all submited resources and computes
   * final fuel power out of them
   */		
  static fpm::fixed_24_8 CalculateFuelPower(const Json::Value& fighter, const Json::Value& wholeFighterData, const Json::Value& wholeRecipeData);

  explicit MoveProcessor (Database& d, xaya::Random& r,
                          const Context& c)
    : BaseMoveProcessor(d, c, false), rnd(r)
  {}

  /**
   * Processes all moves from the given JSON array.
   */
  void ProcessAll (const Json::Value& moveArray);

  /**
   * Processes all admin commands sent in a block.
   */
  void ProcessAdmin (const Json::Value& arr);

};

} // namespace pxd

#endif // PXD_MOVEPROCESSOR_HPP

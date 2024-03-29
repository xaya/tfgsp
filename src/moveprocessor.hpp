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
#include "database/specialtournament.hpp"
#include "database/recipe.hpp"
#include "database/reward.hpp"
#include "database/moneysupply.hpp"
#include "database/amount.hpp"
#include "database/globaldata.hpp"

#include <xayautil/random.hpp>

#include <json/json.h>

#include <map>
#include <string>
#include <vector>

namespace pxd
{

/**
 * Raw data for a coin transfer and burn command.
 */
struct CoinTransferBurn
{

  /** Amount of coins transferred to each account.  */
  std::map<std::string, Amount> transfers;

  /** Amount of coins burnt.  */
  Amount burnt = 0;

  /** Amount of coins bought for CHI in the burnsale.  */
  Amount minted = 0;

  CoinTransferBurn () = default;
  CoinTransferBurn (CoinTransferBurn&&) = default;
  CoinTransferBurn (const CoinTransferBurn&) = default;

  void operator= (const CoinTransferBurn&) = delete;

};

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
  
  /** Access handle for the accounts database table.  */
  XayaPlayersTable xayaplayers;  
  
  /** Access handle for the recepies database table.  */
  RecipeInstanceTable recipeTbl;    
   
  /** Access handle for the fighters database table.  */
  FighterTable fighters;    
  
  /** MoneySupply database table.  */
  MoneySupply moneySupply;  
    
  /** Access handle for the rewards database table.  */
  RewardsTable rewards;    
  
  /** Access handle tournaments database instances*/
  TournamentTable tournamentsTbl;     
  
  /** Access handle special tournaments database instances*/
  SpecialTournamentTable specialTournamentsTbl;     

  /** Access handle for global data*/
  GlobalData globalData;       
  
  explicit BaseMoveProcessor (Database& d, const Context& c);

  /**
   * Parses some basic stuff from a move JSON object.  This extracts the
   * actual move JSON value, the name and the dev payment.  The function
   * returns true if the extraction went well so far and the move
   * may be processed further.
   */
  bool ExtractMoveBasics (const Json::Value& moveObj,
                          std::string& name, Json::Value& mv,
                          std::map<std::string, Amount>& paidToCrownHolders,
                          Amount& burnt);
                          
                       
                          
 /**
   * Parses and validates a move to transfer and burn coins (vCHI), as well
   * as to mint coins through the burnsale.
   *
   * Returns true if at least one part of the transfer/burn was parsed
   * successfully and needs to be executed.  The amount of burnt CHI
   * is updated accordingly if all or some is used up.
   */
  bool ParseCoinTransferBurn (const XayaPlayer& a, const Json::Value& moveObj,
                              CoinTransferBurn& op,
                              Amount& burntChi);   

  /**
   * Tries to handle a move that purchases crystals
   */                       
  void TryCrystalPurchase (const std::string& name, const Json::Value& mv, std::map<std::string, Amount>& paidToCrownHolders); 
  
  /**
   * Tries to purchase a name reroll for the treat
   */                       
  void TryNameRerollPurchase (const std::string& name, const Json::Value& mv, std::map<std::string, Amount>& paidToCrownHolders, const RoConfig& cfg, xaya::Random& rnd);   

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
    
  bool ParseFighterForSaleData(const XayaPlayer& a, const std::string& name, const Json::Value& fighter, uint32_t& fighterID, 
  uint32_t& duration, Amount& price, Amount& listingfee);
      
  /**
   * Tries to parse a move that purchases goody bundle
   */       
   
  bool ParseGoodyBundlePurchase(const Json::Value& mv, Amount& cost, const std::string& name, std::map<std::string, uint64_t>& fungibles, Amount balance);    
  
  /**
   * Parses move that claims cooked sweetener rewards
   */    
  
  bool ParseClaimSweetener(const XayaPlayer& a, const std::string& name, const Json::Value& sweetener, uint32_t& fighterID, 
  std::vector<uint32_t>& rewardDatabaseIds, std::string& sweetenerAuthId);
      
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
   
  bool ParseTransfigureData(const XayaPlayer& a, const std::string& name, const Json::Value& fighter);
    
  /**
   * Tries to parse a move that collects cooked fighter
   */       
   
  bool ParseCollectCookRecepie(const XayaPlayer& a, const std::string& name, const Json::Value& recepie, int32_t& fighterID);
    
   /**
   * Tries to parse a move that send fighter on the expedition
   */    
 
  bool ParseExpeditionData(const XayaPlayer& a, const std::string& name, const Json::Value& expedition, pxd::proto::ExpeditionBlueprint& expeditionBlueprint, std::vector<int32_t>& fightersIds, int32_t& duration, std::string& weHaveApplibeGoodyName);
 
   /**
   * Tries to parse a move that send fighters on the tournament
   */ 
 
  bool ParseTournamentEntryData(const XayaPlayer& a, const std::string& name, const Json::Value& tournament, uint32_t& tournamentID, std::vector<uint32_t>& fighterIDS);
 
   /**
   * Tries to parse a move that send fighter on the special tournament
   */ 
 
  bool ParseSpecialTournamentEntryData(const XayaPlayer& a, const std::string& name, const Json::Value& tournament, uint32_t& tournamentID, std::vector<uint32_t>& fighterIDS);
  
 
  /**
   * Tries to parse a move that deconstructs fighter
   */ 
 
  bool ParseDeconstructData(const XayaPlayer& a, const std::string& name, const Json::Value& fighter, uint32_t& fighterID);
  
  /**
   * Tries to parse a move sweetener fungible that upgrades fighter
   */ 
   
  bool ParseSweetener(const XayaPlayer& a, const std::string& name, const Json::Value& sweetener, 
  std::map<std::string, pxd::Quantity>& fungibleItemAmountForDeduction, int32_t& cookCost, uint32_t& fighterID, 
  int32_t& duration, std::string& sweetenerKeyName);
 
  /**
   * Tries to parse a move that buys fighter from the exchange 
   */ 
 
  bool ParseBuyData(const XayaPlayer& a, const std::string& name, const Json::Value& fighter, uint32_t& fighterID, Amount& exchangeprice);

  /**
   * Tries to parse a move that removes fighter from the exchange 
   */ 
 
  bool ParseRemoveBuyData(const XayaPlayer& a, const std::string& name, const Json::Value& fighter, uint32_t& fighterID);    
  
  /**
   * Tries to parse a move that claims deconstruction rewards
   */ 
 
  bool ParseDeconstructRewardData(const XayaPlayer& a, const std::string& name, const Json::Value& fighter, uint32_t& fighterID);  
    
   /**
   * Tries to parse a move that withdraws fighters from the tournament
   */ 
 
  bool ParseTournamentLeaveData(const XayaPlayer& a, const std::string& name, const Json::Value& tournament, uint32_t& tournamentID, std::vector<uint32_t>& fighterIDS);
     
   /**
   * Tries to parse a move that withdraws fighters from the special tournament
   */ 
 
  bool ParseSpecialTournamentLeaveData(const XayaPlayer& a, const std::string& name, const Json::Value& tournament, uint32_t& tournamentID, std::vector<uint32_t>& fighterIDS);
          
   /**
   * Tries to parse a move that collects reward data
   */    
   
  bool ParseRewardData(const XayaPlayer& a, const std::string& name, const Json::Value& expedition, std::vector<uint32_t>& rewardDatabaseIds, std::vector<std::string>& expeditionIDArray);
  
   /**
   * Tries to parse a move that collects tournament reward data
   */    
   
  bool ParseTournamentRewardData(const XayaPlayer& a, const std::string& name, const Json::Value& tournament, std::vector<uint32_t>& rewardDatabaseIds, uint32_t& tournamentID);  
  
  /**
   * Function checks if fungible item is inside players inventory
   */  
  bool InventoryHasItem(const std::string& itemKeyName, const Inventory& inventory, const google::protobuf::uint64 amount);  
  
  /*Function copypasted from hifghter.hpp, helps in transifuration to resolve armor type*/
  std::vector<pxd::ArmorType> ArmorTypeFromMoveType(pxd::MoveType moveType);
  
public:

  /** Utility functions to help converting between authID and keyNames.
   *  Ultimately, we need to get rid of authID, as its seems redundant at
   *  this point, but need to keep for now to be more consistant with the
   *  original source */     
  pxd::RecipeInstanceTable::Handle GetRecepieObjectFromID(const uint32_t& ID, const Context& ctx);
  static std::string GetCandyKeyNameFromID(const std::string& authID, const Context& ctx); 
  static Json::Value EvaluateFuelList(const Json::Value& fightersSubmited, const Json::Value& recipesSubmited, const Json::Value& candiesSubmited, const Json::Value& fightersNew, const Json::Value& recipesNew, const Json::Value& candiesNew, const Json::Value wholeFightersData,  const Json::Value wholeRecipeData, const Json::Value& candylist);

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
  
  /*Global data command to set up using regtest commands on in production using xaya owned game name*/
  void MaybeSetNewCostMultiplier (const Json::Value& cmd);
  void MaybeSetNewVersionIdentifier (const Json::Value& cmd);
  void MaybeSetNewVanillaDownloadUrl (const Json::Value& cmd);
  
  /* Copy from logic.cpp, to make sure we recalc after user buys from exchange */
  
  void RecalculatePlayerTiers(Database& db, const Context& ctx);
  
   /**
   * Tries to handle an account initialisation (choosing faction) from
   * the given move.
   */
  void MaybeDeconstructFighter(const std::string& name, const Json::Value& fighter);  
  
   /**
   * Actual claiming logic, as this is called from multiple places, we
   * seperate into own function
   */  
  void ClaimRewardsInnerLogic(std::string name, std::vector<uint32_t> rewardDatabaseIds);  
  
   /**
   * Tries to get the rewards from the deconstructed fighter
   */
  void MaybeClaimDeconstructionReward(const std::string& name, const Json::Value& fighter);    
  
   /**
   * Tries to handle an account initialisation (choosing faction) from
   * the given move.
   */
  void MaybeInitXayaPlayer (const std::string& name, const Json::Value& init);
  
  
   /**
   * Tries to put the fighter for sale on the exchange
   */  
  void MaybePutFighterForSale (const std::string& name, const Json::Value& fighter);
     
  /**
  * Tries to run special command from pythin unittest
  */   
  void MaybeSQLTestInjection(const std::string& name, const Json::Value& injection);  
  void MaybeSQLTestInjection2(const std::string& name, const Json::Value& injection);  
     
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
  * Tries to colled cooked fighter
  */  
  void MaybeCollectCookedRecepie (const std::string& name, const Json::Value& recepie);  
  
  /**
  * Tries to claim sweetener rewards if that is possible
  */   
  void MaybeClaimSweetenerReward (const std::string& name, const Json::Value& sweetener);
  
  /**
  * Tries to cook sweetener instance
  */  
  void MaybeCookSweetener (const std::string& name, const Json::Value& sweetener);  
  
  /**
  * Tries to send the fighter for the expedition
  */    
  void MaybeGoForExpedition (const std::string& name, const Json::Value& expedition);
  
  /**
  * Tries to claim all the expedition rewards
  */    
  void MaybeClaimReward (const std::string& name, const Json::Value& expedition);  
  
  /**
  * Tries to claim all the tournament rewards
  */    
  void MaybeClaimTournamentReward (const std::string& name, const Json::Value& tournament);    
  
  /**
  * Tries to send the fighters for the tournament
  */    
  void MaybeEnterTournament (const std::string& name, const Json::Value& tournament);  
  
  /**
  * Tries to withdraw the fighters for the tournament
  */    
  void MaybeLeaveTournament (const std::string& name, const Json::Value& tournament);    
  
  /**
  * Tries to send the fighters for the special tournament
  */    
  void MaybeEnterSpecialTournament (const std::string& name, const Json::Value& tournament);  
  
  /**
  * Tries to withdraw the fighters from the special tournament
  */    
  void MaybeLeaveSpecialTournament (const std::string& name, const Json::Value& tournament);    
  
   /**
   * Tries to update tutorial state even further on.
   */
  void MaybeUpdateFTUEState (const std::string& name, const Json::Value& init); 

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
   * Tries to process all kind of actions related to special tournaments
   */  
  void TrySpecialTournamentAction (const std::string& name, const Json::Value& upd, const Context& ctx);    
  
  /**
   * Tries to process all kind of actions related to fighters
   */  
  void TryFighterAction (const std::string& name, const Json::Value& upd);    
  
  /**
   * Tries to handle a coin (Crystal) transfer / burn operation.  The amount
   * of burnt CHI in the move is updated if any is used for minting vCHI.
   */
  void TryCoinOperation (const std::string& name, const Json::Value& mv,
                         Amount& burntChi);   
  /**
   * Destroys all submited resources
   */		
  void DestroyUsedElements(std::unique_ptr<pxd::XayaPlayer>& player, const Json::Value& fighter);	  
                
public:

  /**
   * Evaluates all submited resources and computes
   * final fuel power out of them
   */		
  static fpm::fixed_24_8 CalculateFuelPower(const Json::Value& fighter, const Json::Value& wholeFighterData, const Json::Value& wholeRecipeData, const Json::Value& candylist, bool outputDebug);	

  explicit MoveProcessor (Database& d, xaya::Random& r,
                          const Context& c)
    : BaseMoveProcessor(d, c), rnd(r)
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

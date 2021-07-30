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

#include "database/database.hpp"
#include "database/xayaplayer.hpp"
#include "database/moneysupply.hpp"
#include "database/amount.hpp"

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
  
  /** MoneySupply database table.  */
  MoneySupply moneySupply;  

  explicit BaseMoveProcessor (Database& d, const Context& c);

  /**
   * Parses some basic stuff from a move JSON object.  This extracts the
   * actual move JSON value, the name and the dev payment.  The function
   * returns true if the extraction went well so far and the move
   * may be processed further.
   */
  bool ExtractMoveBasics (const Json::Value& moveObj,
                          std::string& name, Json::Value& mv,
                          Amount& paidToDev,
                          Amount& burnt) const;
                          
                       
                          
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


public:

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
   * Tries to handle an account initialisation (choosing faction) from
   * the given move.
   */
  void MaybeInitXayaPlayer (const std::string& name, const Json::Value& init);
  
   /**
   * Tries to update tutorial state even further on.
   */
  void MaybeUpdateFTUEState (const std::string& name, const Json::Value& init); 

  /**
   * Tries to handle a move that updates an account.
   */
  void TryXayaPlayerUpdate (const std::string& name, const Json::Value& upd);
  
  /**
   * Tries to set new ftuestate variable for the tutorial
   */
  void TryTFTutorialUpdate (const std::string& name, const Json::Value& upd);  
  
  /**
   * Tries to handle a coin (vCHI) transfer / burn operation.  The amount
   * of burnt CHI in the move is updated if any is used for minting vCHI.
   */
  void TryCoinOperation (const std::string& name, const Json::Value& mv,
                         Amount& burntChi);   

public:

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

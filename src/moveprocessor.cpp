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

#include "moveprocessor.hpp"
#include "moveprocessor_internal.hpp"

#include "jsonutils.hpp"
#include "proto/config.pb.h"

#include <xayautil/jsonutils.hpp>

#include <xayagame/sqlitestorage.hpp>

#include <json/json.h>

#include <sstream>
#include <iostream>
#include <vector>
#include <string>

#include <chrono>
#include <thread>

namespace
{

/** Clock used for timing the callbacks.  */
using PerformanceTimer = std::chrono::high_resolution_clock;

/** Duration type used for reporting callback timings.  */
using CallbackDuration = std::chrono::microseconds;
/** Unit (as string) for the callback timings.  */
constexpr const char* CALLBACK_DURATION_UNIT = "us";

/**
 * Lower-cases an ASCII string deterministically.  Used to compare the
 * payment-receiver keys of a move's "out" map (which XayaX reports as
 * EIP-55 checksummed addresses) against the configured dev address
 * case-insensitively, since Ethereum addresses are case-insensitive.
 */
std::string
AsciiToLower (const std::string& s)
{
  std::string res;
  res.reserve (s.size ());
  for (const char c : s)
    res.push_back ((c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c);
  return res;
}

} // anonymous namespace


namespace pxd
{


/*************************************************************************** */

BaseMoveProcessor::BaseMoveProcessor (Database& d,
                                      const Context& c, bool ro)
  : ctx(c), db(d), readOnly(ro), xayaplayers(db), recipeTbl(db), fighters(db), rewards(db), ongoings(db), tournamentsTbl(db), globalData(db)
{}


bool
BaseMoveProcessor::ExtractMoveBasics (const Json::Value& moveObj,
                                      std::string& name, Json::Value& mv,
                                      Amount& paidToDev)
{
  VLOG (1) << "Processing move:\n" << moveObj;
  CHECK (moveObj.isObject ());

  CHECK (moveObj.isMember ("move"));
  mv = moveObj["move"];

  if (!mv.isObject () && !mv.isArray ())
  {
    LOG (WARNING) << "Move is not an object and not array: " << mv;
    return false;
  }

  const auto& nameVal = moveObj["name"];
  CHECK (nameVal.isString ());
  name = nameVal.asString ();

  /* Sum the WCHI paid to the foundation/dev address.  A paid move arrives
     with moveObj["out"] mapping each on-chain payment receiver to its amount;
     XayaX keys those by the EIP-55 checksummed receiver address.  We credit
     only what was paid to the configured dev_address (matched
     case-insensitively, since Ethereum addresses are), and the per-purchase
     handlers reject the move unless that total covers the price. */
  paidToDev = 0;

  const std::string devAddr
      = AsciiToLower (ctx.RoConfig ()->params ().dev_address ());
  const auto& outVal = moveObj["out"];
  if (!devAddr.empty () && outVal.isObject ())
    for (const auto& key : outVal.getMemberNames ())
      {
        if (AsciiToLower (key) != devAddr)
          continue;
        const auto& amountVal = outVal[key];
        if (!amountVal.isDouble ())
          continue;
        Amount amount;
        if (!xaya::ChiAmountFromJson (amountVal, amount))
          continue;
        paidToDev += amount;
      }

  return true;
}


void
MoveProcessor::ProcessAll (const Json::Value& moveArray)
{
  const auto start = PerformanceTimer::now ();
  CHECK (moveArray.isArray ());
  LOG (INFO) << "Processing " << moveArray.size () << " moves...";

  for (const auto& m : moveArray)
  {
    ProcessOne (m);
  }
  
  const auto end = PerformanceTimer::now ();
  LOG (INFO) << "Processing all moves took " << std::chrono::duration_cast<CallbackDuration> (end - start).count (); 
}

void
MoveProcessor::ProcessAdmin (const Json::Value& admArray)
{
  CHECK (admArray.isArray ());
  LOG (INFO) << "Processing " << admArray.size () << " admin commands...";

  for (const auto& cmd : admArray)
    {
      CHECK (cmd.isObject ());
      ProcessOneAdmin (cmd["cmd"]);
    }
}

void
MoveProcessor::ProcessOneAdmin (const Json::Value& cmd)
{
  if (!cmd.isObject ())
    return;

  HandleGodMode (cmd["god"]);
}

void
MoveProcessor::HandleGodMode (const Json::Value& cmd)
{
    if (!cmd.isObject ())
    return;

    if (!ctx.RoConfig()->params ().god_mode ())
    {
      LOG (WARNING) << "God mode command ignored: " << cmd;
      return;
    }
	
	MaybeSetNewCostMultiplier (cmd["cost"]);
	MaybeSetNewVersionIdentifier (cmd["version"]);
	MaybeSetNewVanillaDownloadUrl (cmd["vanillaurl"]);
}

void
MoveProcessor::MaybeSetNewCostMultiplier (const Json::Value& cmd)
{
  if (!cmd.isInt ())
    return;

  int64_t mt = cmd.asInt();

  /* CRYS-6 hardening: the multiplier feeds 'chiPRICE_sats * multiplier / 1000'.
     A non-positive value would make crystal cost zero/negative (free or inverted
     mint) and an absurdly large value could overflow that product.  Reject
     out-of-range values rather than persist them.  (This setter is god-mode only
     and unreachable on POLYGON, but the guard keeps it safe under REGTEST/admin
     use and is a one-fat-finger-from-free-crystals foot-gun otherwise.)  */
  constexpr int64_t MAX_CHI_MULTIPLIER = 100'000'000;  // up to 100000x, overflow-safe
  if (mt <= 0 || mt > MAX_CHI_MULTIPLIER)
    {
      LOG (WARNING) << "Ignoring out-of-range chi cost multiplier: " << mt;
      return;
    }

  globalData.SetChiMultiplier(mt);
}

void
MoveProcessor::MaybeSetNewVersionIdentifier (const Json::Value& cmd)
{
  if (!cmd.isString ())
    return;

  std::string dd = cmd.asString();
  globalData.SetVersion(dd);
}

void
MoveProcessor::MaybeSetNewVanillaDownloadUrl (const Json::Value& cmd)
{
  if (!cmd.isString ())
    return;

  std::string dd = cmd.asString();
  globalData.SetUrl(dd);
}


void
MoveProcessor::ProcessOne (const Json::Value& moveObj)
{
  std::string name;
  Json::Value mv;
  Amount paidToDev = 0;

  if (!ExtractMoveBasics (moveObj, name, mv, paidToDev))
  {
    return;
  }
  
  std::vector<Json::Value> moves;
  
  if(mv.isObject())
  {
    moves.push_back(mv);
  }
  else
  {
      for(auto& mvx: mv)
      {
          /* DoS cap (H6): bound sub-moves per move; each runs ~12 handlers. */
          if(moves.size() >= MAX_SUBMOVES) break;
          moves.push_back(mvx);
      }
  }
  
  /* Ensure that the account database entry exists.  In other words, we

     have accounts (although perhaps uninitialised) for everyone who
     ever sent a TF move.  */
  if (xayaplayers.GetByName (name, ctx.RoConfig ()) == nullptr)
  {
    LOG (INFO) << "Creating uninitialised account for " << name;
    xayaplayers.CreateNew (name, ctx.RoConfig (), rnd);
  }

  /* Run the sub-move handlers in a fixed order.  The account is auto-created
     above if needed, so handlers are valid even for a freshly seen name.  The
     ordering also ensures that if funds run out, earlier handlers take priority
     over later operations that may require coins implicitly.  */
     
  for(auto& mrl: moves)
  {
    /* A top-level move array may carry scalar/array elements; calling
       mrl["..."] on a non-object throws Json::LogicError.  Skip them. */
    if (!mrl.isObject ()) continue;

    /* Backstop: any unanticipated jsoncpp type-exception inside a handler
       deterministically skips only this sub-move instead of halting the
       chain.  Per-element guards in the Parse* helpers are the primary fix;
       this catches anything missed. */
    try
    {
	/* We perform account updates first.  That ensures that it is possible to
    e.g. initialise an account and act on it in a single move.  */
    TryXayaPlayerUpdate (name, mrl["a"]);
	
	 /* Launch-date gate dropped (legacy fork baggage): Polygon genesis is post-launch; REGTEST never gated. */
	
    /* Handle crystal purchace now */
    TryCrystalPurchase (name, mrl, paidToDev);
	
    /* Handle name reroll purchase*/
    TryNameRerollPurchase (name, mrl, paidToDev, ctx.RoConfig (), rnd);
    
    /* Handle sweetener purchace now */
    TrySweetenerPurchase (name, mrl);  
    
    /* Handle goody purchace now */
    TryGoodyPurchase (name, mrl);   
    
    /* Handle goody bundle purchace now */
    TryGoodyBundlePurchase (name, mrl);    
    
    /* We are trying all kind of cooking actions*/
    TryCookingAction (name, mrl["ca"]);
    
    /* We are trying all kind of expedition actions*/
    TryExpeditionAction (name, mrl["exp"]);  
    
    /* We are trying all kind of tournament related actions*/
    TryTournamentAction (name, mrl["tm"]);  

    /* We are trying all kind of fighter related actions*/
    TryFighterAction (name, mrl["f"]);     
    }
    catch (const std::exception& exc)
    {
      LOG (WARNING) << "Skipping malformed sub-move from " << name
                    << " due to exception: " << exc.what ();
      continue;
    }
  }

  /* If any dev payment is left unconsumed, the user probably overpaid due to
     a frontend bug (every purchase handler consumes the whole payment).  */
  LOG_IF (WARNING, paidToDev > 0)
      << "At the end of the move, " << name
      << " has " << paidToDev << " unconsumed paid-to-dev WCHI satoshi left";
}

  
  void
  MoveProcessor::MaybeInitXayaPlayer (const std::string& name,
                                   const Json::Value& init)
  {
    if (!init.isObject ())
      return;

    auto a = xayaplayers.GetByName (name, ctx.RoConfig ());
    CHECK (a != nullptr);
	
    const auto& address = init["address"];
    if (!address.isString ())
    {
        LOG (WARNING)
            << "Account initialisation does not specify address: " << address;
        a.reset();    
        return;
    }
	
	if(address == "")
	{
		a.reset();
		return;
	}

    a->MutableProto().set_address(address.asString());
    a.reset();
    LOG (INFO)
        << "Initialised account " << name << " to address " << address;
  }
  
  void
  MoveProcessor::TryCookingAction (const std::string& name,
                                   const Json::Value& upd)
  {
    if (!upd.isObject ())
      return;

    /*Trying to cook recepie, optionally with the fighter */
	
	const auto& ref = upd["r"];
    if (!ref.isNull ())
	{
		MaybeCookRecepie (name, upd["r"]);
	}
    
    /*Trying to destroy recepie*/
	const auto& ref2 = upd["d"];
    if (!ref2.isNull ())
	{	
		MaybeDestroyRecepie (name, upd["d"]); 
	}	
    
    /*Trying to collect cooked recepie*/
	const auto& ref3 = upd["cl"];
    if (!ref3.isNull ())
	{
		MaybeCollectCookedRecepie (name, upd["cl"]); 
	}	
    
    /*Trying to cook sweetener */
	const auto& ref4 = upd["s"];
    if (!ref4.isNull ())
	{
		MaybeCookSweetener (name, upd["s"]);  
	}

    /* Sweetener rewards auto-credit at resolve -- no claim verb. */
  }
  
  void
  MoveProcessor::TryExpeditionAction (const std::string& name,
                                   const Json::Value& upd)
  {
    if (!upd.isObject ())
      return;

    /*Trying to send own fighter to expedition*/
	const auto& ref = upd["f"];
    if (!ref.isNull ())
	{	
		MaybeGoForExpedition (name, upd["f"]);
	}
    
    /* Expedition rewards auto-credit at resolve -- no claim verb. */
  }
  

  void
  MoveProcessor::TryTournamentAction (const std::string& name,
                                   const Json::Value& upd)
  {
    if (!upd.isObject ())
      return;
    
    /*Trying to enter the tournament*/
	const auto& ref = upd["e"];
    if (!ref.isNull ())
	{	
		MaybeEnterTournament (name, upd["e"]); 
	}
    
    /*Trying to leave the tournament*/
	const auto& ref2 = upd["l"];
    if (!ref2.isNull ())
	{	
		MaybeLeaveTournament (name, upd["l"]);     
	}

    /* Tournament rewards auto-credit at resolve -- no claim verb. */
  }
  
  void
  MoveProcessor::TryFighterAction (const std::string& name,
                                   const Json::Value& upd)
  {
    if (!upd.isObject ())
      return;
    
    /*Trying to deconstruct the fighter*/
	const auto& ref = upd["d"];
    if (!ref.isNull ())
	{	
		MaybeDeconstructFighter (name, upd["d"]); 
	}
    
    /* Deconstruction candy auto-credits at resolve -- no claim verb. */

    /*Trying to auction the fighter*/
	const auto& ref3 = upd["s"];
    if (!ref3.isNull ())
	{	
		MaybePutFighterForSale(name, upd["s"]); 
	}

    /*Trying to buy the fighter*/
	const auto& ref4 = upd["b"];
    if (!ref4.isNull ())
	{
		MaybeBuyFighterFromExchange(name, upd["b"]);  
	}

    /*Trying to remove the listed fighter*/
	const auto& ref5 = upd["r"];
    if (!ref5.isNull ())
	{	
		MaybeRemoveFighterFromExchange(name, upd["r"]);     
	}

    /*Maybe transfigure fighter*/
	const auto& ref6 = upd["t"];
    if (!ref6.isNull ())
	{	
		MaybeTransfigureFighter (name, upd["t"]); 	
	}
      
  }    

  void
  MoveProcessor::TryXayaPlayerUpdate (const std::string& name,
                                   const Json::Value& upd)
  {
    if (!upd.isObject ())
      return;

	const auto& ref = upd["init"];
    if (!ref.isNull ())
	{
		MaybeInitXayaPlayer (name, upd["init"]);
	}
  }    


/* ************************************************************************** */

} // namespace pxd

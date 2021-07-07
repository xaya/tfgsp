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

#include "burnsale.hpp"

#include "forks.hpp"
#include "jsonutils.hpp"
#include "proto/config.pb.h"

#include <xayautil/jsonutils.hpp>

#include <sstream>

namespace pxd
{


/* ************************************************************************** */

BaseMoveProcessor::BaseMoveProcessor (Database& d,
                                      const Context& c)
  : ctx(c), db(d), xayaplayers(db), moneySupply(db)
{}

bool
BaseMoveProcessor::ExtractMoveBasics (const Json::Value& moveObj,
                                      std::string& name, Json::Value& mv,
                                      Amount& paidToDev,
                                      Amount& burnt) const
{
  VLOG (1) << "Processing move:\n" << moveObj;
  CHECK (moveObj.isObject ());

  CHECK (moveObj.isMember ("move"));
  mv = moveObj["move"];
  if (!mv.isObject ())
    {
      LOG (WARNING) << "Move is not an object: " << mv;
      return false;
    }

  const auto& nameVal = moveObj["name"];
  CHECK (nameVal.isString ());
  name = nameVal.asString ();

  paidToDev = 0;
  const auto& outVal = moveObj["out"];
  const auto& devAddr = ctx.RoConfig()->params ().dev_addr ();
  if (outVal.isObject () && outVal.isMember (devAddr))
    CHECK (xaya::ChiAmountFromJson (outVal[devAddr], paidToDev));

  if (moveObj.isMember ("burnt"))
    CHECK (xaya::ChiAmountFromJson (moveObj["burnt"], burnt));
  else
    burnt = 0;

  return true;
}

bool
BaseMoveProcessor::ParseCoinTransferBurn (const XayaPlayer& a,
                                          const Json::Value& moveObj,
                                          CoinTransferBurn& op,
                                          Amount& burntChi)
{
  CHECK (moveObj.isObject ());
  const auto& cmd = moveObj["vc"];
  if (!cmd.isObject ())
    return false;

  Amount balance = a.GetBalance ();
  Amount total = 0;

  const auto& mint = cmd["m"];
  if (mint.isObject () && mint.empty ())
    {
      const Amount soldBefore = moneySupply.Get ("burnsale");
      op.minted = ComputeBurnsaleAmount (burntChi, soldBefore, ctx);
      balance += op.minted;
    }
  else
    {
      op.minted = 0;
      LOG_IF (WARNING, !mint.isNull ()) << "Invalid mint command: " << mint;
    }

  const auto& burn = cmd["b"];
  if (CoinAmountFromJson (burn, op.burnt))
    {
      if (total + op.burnt <= balance)
        total += op.burnt;
      else
        {
          LOG (WARNING)
              << a.GetName () << " has only a balance of " << balance
              << ", can't burn " << op.burnt << " coins";
          op.burnt = 0;
        }
    }
  else
    op.burnt = 0;

  const auto& transfers = cmd["t"];
  if (transfers.isObject ())
    {
      op.transfers.clear ();
      for (auto it = transfers.begin (); it != transfers.end (); ++it)
        {
          CHECK (it.key ().isString ());
          const std::string to = it.key ().asString ();

          Amount amount;
          if (!CoinAmountFromJson (*it, amount))
            {
              LOG (WARNING)
                  << "Invalid coin transfer from " << a.GetName ()
                  << " to " << to << ": " << *it;
              continue;
            }

          if (total + amount > balance)
            {
              LOG (WARNING)
                  << "Transfer of " << amount << " from " << a.GetName ()
                  << " to " << to << " would exceed the balance";
              continue;
            }

          /* Self transfers are a no-op, so we just ignore them (and do not
             update the total sent, so the balance is still available for
             future transfers).  */
          if (to == a.GetName ())
            continue;

          total += amount;
          CHECK (op.transfers.emplace (to, amount).second);
        }
    }

  CHECK_LE (total, balance);
  return total > 0 || op.minted > 0;
}

void
MoveProcessor::ProcessAll (const Json::Value& moveArray)
{
  CHECK (moveArray.isArray ());
  LOG (INFO) << "Processing " << moveArray.size () << " moves...";

  for (const auto& m : moveArray)
    ProcessOne (m);
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
}

void
MoveProcessor::TryCoinOperation (const std::string& name,
                                 const Json::Value& mv,
                                 Amount& burntChi)
{
  auto a = xayaplayers.GetByName (name);
  CHECK (a != nullptr);

  CoinTransferBurn op;
  if (!ParseCoinTransferBurn (*a, mv, op, burntChi))
    return;

  if (op.minted > 0)
    {
      LOG (INFO) << name << " minted " << op.minted << " coins in the burnsale";
      a->AddBalance (op.minted);
      const Amount oldBurnsale = a->GetProto ().burnsale_balance ();
      a->MutableProto ().set_burnsale_balance (oldBurnsale + op.minted);
      moneySupply.Increment ("burnsale", op.minted);
    }

  if (op.burnt > 0)
    {
      LOG (INFO) << name << " is burning " << op.burnt << " coins";
      a->AddBalance (-op.burnt);
    }

  for (const auto& entry : op.transfers)
    {
      /* Transfers to self are a no-op, but we have to explicitly handle
         them here.  Else we would run into troubles by having a second
         active Account handle for the same account.  */
      if (entry.first == name)
        continue;

      LOG (INFO)
          << name << " is sending " << entry.second
          << " coins to " << entry.first;
      a->AddBalance (-entry.second);

      auto to = xayaplayers.GetByName (entry.first);
      if (to == nullptr)
        {
          LOG (INFO)
              << "Creating uninitialised account for coin recipient "
              << entry.first;
          to = xayaplayers.CreateNew (entry.first);
        }
      to->AddBalance (entry.second);
    }
}

void
MoveProcessor::ProcessOne (const Json::Value& moveObj)
{
  std::string name;
  Json::Value mv;
  Amount paidToDev, burnt;
  if (!ExtractMoveBasics (moveObj, name, mv, paidToDev, burnt))
    return;

  /* Ensure that the account database entry exists.  In other words, we
     have accounts (although perhaps uninitialised) for everyone who
     ever sent a Taurion move.  */
  if (xayaplayers.GetByName (name) == nullptr)
    {
      LOG (INFO) << "Creating uninitialised account for " << name;
      xayaplayers.CreateNew (name);
    }

  /* Handle coin transfers before other game operations.  They are even
     valid without a properly initialised account (so that vCHI works as
     a real cryptocurrency, not necessarily tied to the game).
     This also ensures that if funds run out, then the explicit transfers
     are done with priority over the other operations that may require coins
     implicitly.  */
  TryCoinOperation (name, mv, burnt);

  /* At this point, we terminate if the game-play itself has not started.
     This is more or less when the "game world is created", except that we
     do allow Cubit operations already from the start of the burnsale.  */
  if (!ctx.Forks ().IsActive (Fork::GameStart))
    return;

  /* We perform account updates first.  That ensures that it is possible to
     e.g. choose one's faction and create characters in a single move.  */
  TryXayaPlayerUpdate (name, mv["a"]);


  /* If there is no account (after potentially updating/initialising it),
     then let's not try to process any more updates.  This explicitly
     enforces that accounts have to be initialised before doing anything
     else, even if perhaps some changes wouldn't actually require access
     to an account in their processing.  */
  if (!xayaplayers.GetByName (name)->IsInitialised ())
    {
      VLOG (1)
          << "Account " << name << " is not yet initialised,"
          << " ignoring parts of the move " << moveObj;
      return;
    }

  /* If any burnt or paid-to-dev coins are left, it means probably something
     has gone wrong and the user overpaid due to a frontend bug.  */
  LOG_IF (WARNING, paidToDev > 0 || burnt > 0)
      << "At the end of the move, " << name
      << " has " << paidToDev << " paid-to-dev and "
      << burnt << " burnt CHI satoshi left";
}

  void
  MoveProcessor::MaybeInitXayaPlayer (const std::string& name,
                                   const Json::Value& init)
  {
    if (!init.isObject ())
      return;

    auto a = xayaplayers.GetByName (name);
    CHECK (a != nullptr);
    if (a->IsInitialised ())
      {
        LOG (WARNING) << "Account " << name << " is already initialised";
        return;
      }

    const auto& roleVal = init["role"];
    if (!roleVal.isString ())
      {
        LOG (WARNING)
            << "Account initialisation does not specify faction: " << init;
        return;
      }
    const PlayerRole role = PlayerRoleFromString (roleVal.asString ());
    switch (role)
      {
      case PlayerRole::INVALID:
        LOG (WARNING) << "Invalid faction specified for account: " << init;
        return;

      default:
        break;
      }

    if (init.size () != 1)
      {
        LOG (WARNING) << "Account initialisation has extra fields: " << init;
        return;
      }

    a->SetRole (role);
    LOG (INFO)
        << "Initialised account " << name << " to faction "
        << PlayerRoleToString (role);
  }

  void
  MoveProcessor::TryXayaPlayerUpdate (const std::string& name,
                                   const Json::Value& upd)
  {
    if (!upd.isObject ())
      return;

    MaybeInitXayaPlayer (name, upd["init"]);
  }    


/* ************************************************************************** */

} // namespace pxd

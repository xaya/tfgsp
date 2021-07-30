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

#include "xayaplayer.hpp"

namespace pxd
{

XayaPlayer::XayaPlayer (Database& d, const std::string& n)
  : db(d), name(n), tracker(db.TrackHandle ("xayaplayer", n)),
    role(PlayerRole::INVALID), ftuestate(FTUEState::Intro), dirtyFields(true)
{
  VLOG (1) << "Created instance for newly initialised account " << name;
  data.SetToDefault ();
}

XayaPlayer::XayaPlayer (Database& d, const Database::Result<XayaPlayerResult>& res)
  : db(d), dirtyFields(false)
{
  name = res.Get<XayaPlayerResult::name> ();
  tracker = d.TrackHandle ("xayaplayer", name);

  role = GetNullablePlayerRoleFromColumn (res);
  data = res.GetProto<XayaPlayerResult::proto> ();
  ftuestate = GetFTUEStateFromColumn (res);

  VLOG (1) << "Created account instance for " << name << " from database";
}

XayaPlayer::~XayaPlayer ()
{
  if (!dirtyFields && !data.IsDirty ())
    {
      VLOG (1) << "Account instance " << name << " is not dirty";
      return;
    }

  VLOG (1) << "Updating account " << name << " in the database";
  CHECK_GE (GetBalance (), 0);

  auto stmt = db.Prepare (R"(
    INSERT OR REPLACE INTO `xayaplayers`
      (`name`, `role`, `proto`, `ftuestate`)
      VALUES (?1, ?2, ?3, ?4)
  )");

  stmt.Bind (1, name);
  BindPlayerRoleParameter (stmt, 2, role);
  stmt.BindProto (3, data);
  BindFTUEStateParameter (stmt, 4, ftuestate);

  stmt.Execute ();
}

const bool XayaPlayer::GetIsMine ()
{
    bool alwaystrue = true;
    return alwaystrue;
}

std::string XayaPlayer::SendCHI (std::string address, Amount amount)
{
    return "";
}

void
BindFTUEStateParameter (Database::Statement& stmt, const unsigned ind,
                      const FTUEState f)
{
  stmt.Bind (ind, static_cast<int64_t> (f));
  return;
}

void
BindPlayerRoleParameter (Database::Statement& stmt, const unsigned ind,
                      const PlayerRole f)
{
  switch (f)
    {
    case PlayerRole::PLAYER:
    case PlayerRole::ROLEADMIN:
    case PlayerRole::CONENTADMIN:
    case PlayerRole::CONFIGADMIN:
    case PlayerRole::EXCHANGEADMIN:
    case PlayerRole::ADMINISTRATOR:
      stmt.Bind (ind, static_cast<int64_t> (f));
      return;
    case PlayerRole::INVALID:
      stmt.BindNull (ind);
      return;
    default:
      LOG (FATAL)
          << "Binding invalid faction to parameter: " << static_cast<int> (f);
    }
}

std::string
FTUEStateToString (const FTUEState f)
{
  switch (f)
    {
    case FTUEState::Intro:
      return "t0";
    case FTUEState::FirstRecipe:
      return "t1";
    case FTUEState::CookFirstRecipe:
      return "t3";
    case FTUEState::CookingFirstRecipe:
      return "t4";
    case FTUEState::FirstExpedition:
      return "t5";
    case FTUEState::JoinFirstExpedition:
      return "t6";  
    case FTUEState::FirstExpeditionPending:
      return "t7";  
    case FTUEState::ResolveFirstExpedition:
      return "t8";  
    case FTUEState::SecondRecipe:
      return "t9";  
    case FTUEState::CookSecondRecipe:
      return "t10";  
    case FTUEState::CookingSecondRecipe:
      return "t11";  
    case FTUEState::FirstTournament:
      return "t12";  
    case FTUEState::GoToFirstTournament:
      return "t13";  
    case FTUEState::JoinFirstTournament:
      return "t14";  
    case FTUEState::FirstTournamentPending:
      return "t15";  
    case FTUEState::ResolveFirstTournament:
      return "t16";  
    case FTUEState::Completed:
      return "t17";        
    default:
      LOG (FATAL) << "Invalid FTUEState: " << static_cast<int> (f);
    }
}

std::string
PlayerRoleToString (const PlayerRole f)
{
  switch (f)
    {
    case PlayerRole::PLAYER:
      return "p";
    case PlayerRole::ROLEADMIN:
      return "r";
    case PlayerRole::CONENTADMIN:
      return "c";
    case PlayerRole::CONFIGADMIN:
      return "f";
    case PlayerRole::EXCHANGEADMIN:
      return "e";
    case PlayerRole::ADMINISTRATOR:
      return "a";      
    default:
      LOG (FATAL) << "Invalid faction: " << static_cast<int> (f);
    }
}

void
XayaPlayer::SetRole (const PlayerRole f)
{
  CHECK (role == PlayerRole::INVALID)
      << "Account " << name << " already has a role";
  CHECK (f != PlayerRole::INVALID)
      << "Setting account " << name << " to NULL role";
  role = f;
  dirtyFields = true;
}

void
XayaPlayer::SetFTUEState(const FTUEState f)
{
  ftuestate = f;
  dirtyFields = true;
}

FTUEState
FTUEStateFromString (const std::string& str)
{
  if (str == "t0")
    return FTUEState::Intro;
  if (str == "t1")
    return FTUEState::FirstRecipe;
  if (str == "t3")
    return FTUEState::CookFirstRecipe;
  if (str == "t4")
    return FTUEState::CookingFirstRecipe;
  if (str == "t5")
    return FTUEState::FirstExpedition;
  if (str == "t6")
    return FTUEState::JoinFirstExpedition;
  if (str == "t7")
    return FTUEState::FirstExpeditionPending;
  if (str == "t8")
    return FTUEState::ResolveFirstExpedition;
  if (str == "t9")
    return FTUEState::SecondRecipe;
  if (str == "t10")
    return FTUEState::CookSecondRecipe;
  if (str == "t11")
    return FTUEState::CookingSecondRecipe;
  if (str == "t12")
    return FTUEState::FirstTournament;
  if (str == "t13")
    return FTUEState::GoToFirstTournament;
  if (str == "t14")
    return FTUEState::JoinFirstTournament;
  if (str == "t15")
    return FTUEState::FirstTournamentPending;
  if (str == "t16")
    return FTUEState::ResolveFirstTournament;
  if (str == "t17")
    return FTUEState::Completed;

  LOG (WARNING) << "String is not a valid ftuestate: " << str;
  return FTUEState::Invalid;
}

PlayerRole
PlayerRoleFromString (const std::string& str)
{
  if (str == "p")
    return PlayerRole::PLAYER;
  if (str == "r")
    return PlayerRole::ROLEADMIN;
  if (str == "c")
    return PlayerRole::CONENTADMIN;
  if (str == "f")
    return PlayerRole::CONFIGADMIN;
  if (str == "e")
    return PlayerRole::EXCHANGEADMIN;
  if (str == "a")
    return PlayerRole::ADMINISTRATOR;


  LOG (WARNING) << "String is not a valid faction: " << str;
  return PlayerRole::INVALID;
}
 
const bool XayaPlayer::HasRole (PlayerRole role)
{
    return true;
} 
 
const bool XayaPlayer::GrantRole (PlayerRole role)
{
  return true;
}
    
const bool XayaPlayer::RevokeRole (PlayerRole role)
{
  return true;
}    

void
XayaPlayer::AddBalance (const Amount val)
{
  Amount balance = data.Get ().balance ();
  balance += val;
  CHECK_GE (balance, 0);
  data.Mutable ().set_balance (balance);
}

XayaPlayersTable::Handle
XayaPlayersTable::CreateNew (const std::string& name)
{
  CHECK (GetByName (name) == nullptr)
      << "Account for " << name << " exists already";
  return Handle (new XayaPlayer (db, name));
}

XayaPlayersTable::Handle
XayaPlayersTable::GetFromResult (const Database::Result<XayaPlayerResult>& res)
{
  return Handle (new XayaPlayer (db, res));
}

XayaPlayersTable::Handle
XayaPlayersTable::GetByName (const std::string& name)
{
  auto stmt = db.Prepare ("SELECT * FROM `xayaplayers` WHERE `name` = ?1");
  stmt.Bind (1, name);
  auto res = stmt.Query<XayaPlayerResult> ();

  if (!res.Step ())
    return nullptr;

  auto r = GetFromResult (res);
  CHECK (!res.Step ());
  return r;
}

Database::Result<XayaPlayerResult>
XayaPlayersTable::QueryAll ()
{
  auto stmt = db.Prepare (R"(
    SELECT *
      FROM `xayaplayers`
      ORDER BY `name`
  )");
  return stmt.Query<XayaPlayerResult> ();
}

Database::Result<XayaPlayerResult>
XayaPlayersTable::QueryInitialised ()
{
  auto stmt = db.Prepare (R"(
    SELECT *
      FROM `xayaplayers`
      WHERE `role` IS NOT NULL
      ORDER BY `name`
  )");
  return stmt.Query<XayaPlayerResult> ();
}

} // namespace pxd

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
#include <math.h>

namespace pxd
{

XayaPlayer::XayaPlayer (Database& d, const std::string& n, const RoConfig& cfg)
  : db(d), name(n), tracker(db.TrackHandle ("xayaplayer", n)),
    role(PlayerRole::INVALID), ftuestate(FTUEState::Intro), dirtyFields(true)
{
  VLOG (1) << "Created instance for newly initialised account " << name;
  data.SetToDefault ();
  
  /*Probably not needed and is just leftover from original source,
  but lets transfer administrator name role just in case*/
  
  if(name == "tftr")
  {
      role = PlayerRole::ADMINISTRATOR;
  }
  
  /*Load configuration values*/ 
  recipe_slots = cfg->params().max_recipe_inventory_amount();
  roster_slots = cfg->params().max_fighter_inventory_amount();
  prestige = cfg->params().base_prestige();
  
  /*For the new account, we are supplying initial set of items*/    
  std::string starting_recepie_guid = cfg->params().starting_recipes(); 
  
  for (const auto& recepie : cfg->recepies())
  {
    if(recepie.second.authoredid() == starting_recepie_guid)
    {

        RecipeInstanceTable recipesTbl(d);          
        recipesTbl.CreateNew (GetName(), starting_recepie_guid, cfg);
    
        for (const auto& candy : recepie.second.requiredcandy())
        {
            for (const auto& candyTemplate : cfg->candies())
            {
               if(candyTemplate.second.authoredid() == candy.candytype())
               {
                  inv.SetFungibleCount(candyTemplate.first, candy.amount());
               }
            }
        }
    }
  }
  
  AddBalance(cfg->params().starting_crystals());  
  
  CalculatePrestige(cfg);
}

XayaPlayer::XayaPlayer (Database& d, const Database::Result<XayaPlayerResult>& res, const RoConfig& cfg)
  : db(d), dirtyFields(false)
{
  name = res.Get<XayaPlayerResult::name> ();
  tracker = d.TrackHandle ("xayaplayer", name);

  role = GetNullablePlayerRoleFromColumn (res);
  data = res.GetProto<XayaPlayerResult::proto> ();
  ftuestate = GetFTUEStateFromColumn (res);
  
  inv = res.GetProto<XayaPlayerResult::inventory> ();
  
  /*Load configuration values*/ 
  recipe_slots = cfg->params().max_recipe_inventory_amount();
  roster_slots = cfg->params().max_fighter_inventory_amount();
  
  prestige = res.Get<XayaPlayerResult::prestige> ();

  VLOG (1) << "Created account instance for " << name << " from database";
}

XayaPlayer::~XayaPlayer ()
{
  if (!dirtyFields && !data.IsDirty () && !inv.IsDirty())
  {
      VLOG (1) << "Account instance " << name << " is not dirty";
      return;
  }

  VLOG (1) << "Updating account " << name << " in the database";
  CHECK_GE (GetBalance (), 0);

  auto stmt = db.Prepare (R"(
    INSERT OR REPLACE INTO `xayaplayers`
      (`name`, `role`, `proto`, `ftuestate`, `inventory`, `prestige`)
      VALUES (?1, ?2, ?3, ?4, ?108, ?5)
  )");

  stmt.Bind (1, name);
  BindPlayerRoleParameter (stmt, 2, role);
  stmt.BindProto (3, data);
  BindFTUEStateParameter (stmt, 4, ftuestate);
  stmt.Bind (5, prestige);
  stmt.BindProto (108, inv.GetProtoForBinding ());

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

std::vector<FighterTable::Handle> XayaPlayer::CollectInventoryFighters(const RoConfig& cfg)
{
  std::vector<FighterTable::Handle> fighters;
  
  FighterTable fightersTable(db);
  auto res = fightersTable.QueryForOwner (GetName());
  while (res.Step ())
  {
      auto c = fightersTable.GetFromResult (res, cfg);
      fighters.push_back(std::move(c));
  }
  return fighters;
}

std::vector<TournamentTable::Handle> XayaPlayer::CollectTournaments(const RoConfig& cfg)
{
  std::vector<TournamentTable::Handle> tournaments;
  
  TournamentTable tournamentsTable(db);
  auto res = tournamentsTable.QueryAll();
  while (res.Step ())
  {
      auto c = tournamentsTable.GetFromResult (res);
      tournaments.push_back(std::move(c));
  }
  return tournaments;
}

float XayaPlayer::GetFighterPercentageFromQuality(uint32_t quality, std::vector<FighterTable::Handle>& fighters)
{
  float totalFighters = 0;
  for(const auto& fighter: fighters)
  {
     if(fighter.get()->GetProto().quality() == quality)
     {
         totalFighters++;
     }
  }  
  
  return totalFighters / fighters.size();
}

void
XayaPlayer::CalculatePrestige(const RoConfig& cfg)
{
    std::vector<FighterTable::Handle> fighters = CollectInventoryFighters(cfg);
    uint32_t totalFighters = fighters.size();
    
    if(totalFighters == 0)
    {
      prestige = cfg->params().base_prestige();
      return;
    }
    
    float getPercentageEpic  = GetFighterPercentageFromQuality(4, fighters) * cfg->params().prestige_epic_mod() * log10(totalFighters+1);
    float getPercentageRare  = GetFighterPercentageFromQuality(3, fighters) * cfg->params().prestige_rare_mod() * log10(totalFighters+1);
    float getPercentageUncommom  = GetFighterPercentageFromQuality(2, fighters) * cfg->params().prestige_uncommon_mod() * log10(totalFighters+1);
    float getPercentageCommon  = GetFighterPercentageFromQuality(1, fighters) * cfg->params().prestige_common_mod() * log10(totalFighters+1);

    float fighterAverage = 0;
    
    for(const auto& fighter: fighters)
    {
        if(fighterAverage == 0)
        {
            fighterAverage = fighter.get()->GetProto().rating();
        }
        else
        {
            fighterAverage = (fighterAverage + fighter.get()->GetProto().rating()) / 2;
        }
    }
    
    float averageRatingScore =  fighterAverage * cfg->params().prestige_avg_rating_mod() * log10(totalFighters+1);
    
    std::vector<TournamentTable::Handle> tournaments = CollectTournaments(cfg);
    float winCount = 0;
    
    //for(const auto& tournament: tournaments)
    //{
        //winCount++; //todo
    //}
    
    float totalTournaments = tournaments.size();
    float winPercent = 0.0f;
    
    if(totalTournaments > 0)
    {
        winPercent = winCount / totalTournaments * log10(totalTournaments+1); 
        winPercent *= cfg->params().prestige_tournament_performance_mod();
    }
    
    float winPercentPrestigeMod = winPercent;
    prestige = (int64_t)(getPercentageEpic + getPercentageRare + getPercentageUncommom + getPercentageCommon + averageRatingScore) * (1 + winPercentPrestigeMod);
    
    dirtyFields = true;
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
XayaPlayersTable::CreateNew (const std::string& name, const RoConfig& cfg)
{
  CHECK (GetByName (name, cfg) == nullptr)
      << "Account for " << name << " exists already";
  return Handle (new XayaPlayer (db, name, cfg));
}

XayaPlayersTable::Handle
XayaPlayersTable::GetFromResult (const Database::Result<XayaPlayerResult>& res, const RoConfig& cfg)
{
  return Handle (new XayaPlayer (db, res, cfg));
}

XayaPlayersTable::Handle
XayaPlayersTable::GetByName (const std::string& name, const RoConfig& cfg)
{
  auto stmt = db.Prepare ("SELECT * FROM `xayaplayers` WHERE `name` = ?1");
  stmt.Bind (1, name);
  auto res = stmt.Query<XayaPlayerResult> ();

  if (!res.Step ())
    return nullptr;

  auto r = GetFromResult (res, cfg);
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

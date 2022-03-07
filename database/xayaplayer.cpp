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

#include <xayautil/random.hpp>

#include <math.h>

namespace pxd
{

XayaPlayer::XayaPlayer (Database& d, const std::string& n, const RoConfig& cfg, xaya::Random& rnd)
  : db(d), name(n), tracker(db.TrackHandle ("xayaplayer", n)),
    role(PlayerRole::INVALID), dirtyFields(true)
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
  
  RecipeInstanceTable recipesTbl(d);
  
  auto rcp1 = recipesTbl.CreateNew (GetName(), "5864a19b-c8c0-2d34-eaef-9455af0baf2c", cfg);
  auto rcp2 = recipesTbl.CreateNew (GetName(), "ba0121ba-e8a6-7e64-9bc1-71dfeca27daa", cfg);
  
  int rcp1Id = rcp1->GetId();
  rcp1.reset();
  
  int rcp2Id = rcp2->GetId();
  rcp2.reset();
  
  // This also should be consisten with items we have at the end of front-end tutorial
  
  inv.SetFungibleCount("Common_Nonpareil", 10);
  inv.SetFungibleCount("Common_Icing", 20);
  inv.SetFungibleCount("Common_Fizzy Powder", 9);
  inv.SetFungibleCount("Common_Chocolate Chip", 20);
  inv.SetFungibleCount("Common_Candy Cane", 9);
  
  // This also includes having 2 tutorial fighters cooked, and that includes preowning 2 starting recepies
  
  FighterTable fighterTbl(d);
  
  fighterTbl.CreateNew (GetName(), rcp1Id, cfg, rnd);
  fighterTbl.CreateNew (GetName(), rcp2Id, cfg, rnd);
  
  // And finally tutorial reward recepies      
  recipesTbl.CreateNew (GetName(), starting_recepie_guid, cfg);
  
  AddBalance(cfg->params().starting_crystals());  
  CalculatePrestige(cfg);
  
  rcp1 = recipesTbl.GetById(rcp1Id);
  rcp2 = recipesTbl.GetById(rcp2Id);
  
  rcp1->SetOwner("");
  rcp2->SetOwner("");
  
  rcp1.reset();
  rcp2.reset();
  
  
  //For ryans special test, lets give him such juice fighters and recepies
  
  AddBalance(100000);
  
  inv.SetFungibleCount("Common_Gumdrop", 500);
  inv.SetFungibleCount("Common_Candy Button", 500);
  inv.SetFungibleCount("Uncommon_Marshmallow", 500);
  inv.SetFungibleCount("Epic_Bronze Mint Patty", 500);
  inv.SetFungibleCount("Epic_Silver Icing", 500);
  inv.SetFungibleCount("Epic_Silver Fruit Slices", 500);
  inv.SetFungibleCount("Epic_Silver Jelly Beans", 500);
  inv.SetFungibleCount("Epic_Silver Peppermint", 500);
  inv.SetFungibleCount("Uncommon_Jelly Bean", 500);
  inv.SetFungibleCount("Common_Fizzy Powder", 500);
  inv.SetFungibleCount("Epic_Gold Candy Ribbon", 500);
  inv.SetFungibleCount("Epic_Bronze Icing", 500);
  inv.SetFungibleCount("Epic_Bronze Gummy Bear", 500);
  inv.SetFungibleCount("Rare_Mint Patty", 500);
  inv.SetFungibleCount("Rare_Gummi Bear", 500);
  inv.SetFungibleCount("Rare_Mint Stick", 500);
  inv.SetFungibleCount("Epic_Gold Circus Peanuts", 500);
  inv.SetFungibleCount("Epic_Gold Toffee Chunk", 500);
  inv.SetFungibleCount("Common_Fruit Slice", 500);
  inv.SetFungibleCount("Epic_Bronze Gum Drop", 500);
  inv.SetFungibleCount("Epic_Gold Gum Ball", 500);
  inv.SetFungibleCount("Common_Gumball", 500);
  inv.SetFungibleCount("Rare_Circus Peanuts", 500);
  inv.SetFungibleCount("Rare_Licorice Piece", 500);
  inv.SetFungibleCount("Epic_Silver Hard Candy", 500);
  inv.SetFungibleCount("Uncommon_Chocolate Nut", 500);
  inv.SetFungibleCount("Uncommon_Peppermint", 500);
  inv.SetFungibleCount("Epic_Gold Non Pareil", 500);
  inv.SetFungibleCount("Rare_Candy Ribbon", 500);
  inv.SetFungibleCount("Epic_Bronze Candy Corn", 500);
  inv.SetFungibleCount("Epic_Silver Candy Cane", 500);
  inv.SetFungibleCount("Common_Icing", 500);
  inv.SetFungibleCount("Rare_Peanut Butter Cup", 500);
  inv.SetFungibleCount("Epic_Silver Licorice Piece", 500);
  inv.SetFungibleCount("Common_Chocolate Chip", 500);
  inv.SetFungibleCount("Common_Sugar Glaze", 500);
  inv.SetFungibleCount("Common_Ring Pop", 500);
  inv.SetFungibleCount("Rare_Jawbreaker", 500);
  inv.SetFungibleCount("Rare_Toffee Chunk", 500);
  inv.SetFungibleCount("Epic_Silver MInt Stick", 500);
  inv.SetFungibleCount("Rare_Hard Candy", 500);
  inv.SetFungibleCount("Uncommon_Cinnamon Ball", 500);
  inv.SetFungibleCount("Epic_Gold Hard Candy", 500);
  inv.SetFungibleCount("Common_Nonpareil", 500);
  inv.SetFungibleCount("Rare_Candy Corn", 500);
  inv.SetFungibleCount("Epic_Gold Icing", 500);
  inv.SetFungibleCount("Rare_Candy Loops", 500);
  inv.SetFungibleCount("Uncommon_Banana Chew", 500);
  inv.SetFungibleCount("Epic_Bronze Jelly Bean", 500);
  inv.SetFungibleCount("Epic_Bronze Jawbreaker", 500);
  inv.SetFungibleCount("Epic_Silver Gum Drop", 500);
  inv.SetFungibleCount("Uncommon_Cotton Candy", 500);
  inv.SetFungibleCount("Epic_Gold Jelly Bean", 500);
  inv.SetFungibleCount("Epic_Silver Ring Pop", 500);
  inv.SetFungibleCount("Rare_Giant Chocolate Chip", 500);
  inv.SetFungibleCount("Common_Licorice Lace", 500);
  inv.SetFungibleCount("Epic_Gold Chocolate Bar", 500);
  inv.SetFungibleCount("Epic_Bronze Gum Ball", 500);
  inv.SetFungibleCount("Common_Candy Cane", 500);
  inv.SetFungibleCount("Uncommon_Rock Candy", 500);
  
  recipesTbl.CreateNew (GetName(), "60f3f548-20ca-7a64-6ad3-054d654e24a2", cfg);
  recipesTbl.CreateNew (GetName(), "a889e1d9-c337-dff4-3aae-7701b6e48aeb", cfg);
  recipesTbl.CreateNew (GetName(), "dfd860da-2127-60e4-8b46-39bfffb57245", cfg);
  recipesTbl.CreateNew (GetName(), "8f8425e7-cab0-0b14-1874-5072afe7d0b5", cfg);
  recipesTbl.CreateNew (GetName(), "1bbc7d99-7fce-24a4-c9a3-dfaf4b744efa", cfg);
  recipesTbl.CreateNew (GetName(), "db1606dd-112d-7a34-db0b-8923900c8ac7", cfg);
  recipesTbl.CreateNew (GetName(), "c481aeee-d1a1-01c4-7aca-92d0edcddf18", cfg);
  recipesTbl.CreateNew (GetName(), "7b7d8898-7f58-0334-0bad-825dc87a6638", cfg);
  recipesTbl.CreateNew (GetName(), "7938f8c7-2d20-19c4-9be9-b05fd663f44f", cfg);
  recipesTbl.CreateNew (GetName(), "175cd684-0079-c1b4-89b7-6bca7288f50d", cfg);
  
  
}

XayaPlayer::XayaPlayer (Database& d, const Database::Result<XayaPlayerResult>& res, const RoConfig& cfg)
  : db(d), dirtyFields(false)
{
  name = res.Get<XayaPlayerResult::name> ();
  tracker = d.TrackHandle ("xayaplayer", name);

  role = GetNullablePlayerRoleFromColumn (res);
  data = res.GetProto<XayaPlayerResult::proto> ();

  inv = res.GetProto<XayaPlayerResult::inventory> ();
  
  /*Load configuration values*/ 
  recipe_slots = cfg->params().max_recipe_inventory_amount();
  roster_slots = cfg->params().max_fighter_inventory_amount();
  
  prestige = res.Get<XayaPlayerResult::prestige> ();
}

XayaPlayer::~XayaPlayer ()
{
  if (!dirtyFields && !data.IsDirty () && !inv.IsDirty())
  {
      return;
  }

  CHECK_GE (GetBalance (), 0);

  auto stmt = db.Prepare (R"(
    INSERT OR REPLACE INTO `xayaplayers`
      (`name`, `role`, `proto`, `inventory`, `prestige`)
      VALUES (?1, ?2, ?3, ?108, ?5)
  )");

  stmt.Bind (1, name);
  BindPlayerRoleParameter (stmt, 2, role);
  stmt.BindProto (3, data);
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
      auto c = tournamentsTable.GetFromResult (res, cfg);
      tournaments.push_back(std::move(c));
  }
  return tournaments;
}

double XayaPlayer::GetFighterPercentageFromQuality(uint32_t quality, std::vector<FighterTable::Handle>& fighters)
{
  double totalFighters = 0;
  for(const auto& fighter: fighters)
  {
     if(fighter.get()->GetProto().quality() == quality)
     {
         totalFighters++;
     }
  }  
  
  return (totalFighters) / fighters.size();
}

void
XayaPlayer::CalculatePrestige(const RoConfig& cfg)
{
    std::vector<FighterTable::Handle> fighters = CollectInventoryFighters(cfg);
    double totalFighters = fighters.size();
    
    if(totalFighters == 0)
    {
      prestige = cfg->params().base_prestige();
      return;
    }
    
    double getPercentageEpic  = GetFighterPercentageFromQuality(4, fighters) * cfg->params().prestige_epic_mod() * log10(totalFighters+1);
    double getPercentageRare  = GetFighterPercentageFromQuality(3, fighters) * cfg->params().prestige_rare_mod() * log10(totalFighters+1);
    double getPercentageUncommom  = GetFighterPercentageFromQuality(2, fighters) * cfg->params().prestige_uncommon_mod() * log10(totalFighters+1);
    double getPercentageCommon  = GetFighterPercentageFromQuality(1, fighters) * cfg->params().prestige_common_mod() * log10(totalFighters+1);

    uint32_t fighterAverage = 0;
    
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
    
    double averageRatingScore =  (fighterAverage) * cfg->params().prestige_avg_rating_mod() * log10(totalFighters+1);
    
    double winCount = GetProto().tournamentswon();
    double totalTournaments = GetProto().tournamentscompleted();
    double winPercent = 0;
    
    if(totalTournaments > 0)
    {
        winPercent = winCount / totalTournaments * log10(totalTournaments+1); 
        winPercent *= cfg->params().prestige_tournament_performance_mod();
    }
    
    double winPercentPrestigeMod = winPercent;
    prestige = (int64_t)(getPercentageEpic + getPercentageRare + getPercentageUncommom + getPercentageCommon + averageRatingScore) * (1 + winPercentPrestigeMod);
    dirtyFields = true;
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
    case PlayerRole::INVALID:
      return "i";        
    default:
      LOG (FATAL) << "Invalid faction: " << static_cast<int> (f);
    }
}

void
XayaPlayer::SetRole (const PlayerRole f)
{  
  role = f;
  dirtyFields = true;
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
XayaPlayersTable::CreateNew (const std::string& name, const RoConfig& cfg, xaya::Random& rnd)
{
  CHECK (GetByName (name, cfg) == nullptr)
      << "Account for " << name << " exists already";
  return Handle (new XayaPlayer (db, name, cfg, rnd));
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

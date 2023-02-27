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

#include "recipe.hpp"

#include <glog/logging.h>

namespace pxd
{

RecipeInstance::RecipeInstance (Database& d, const std::string& o, const std::string& cr, const RoConfig& cfg)
  : id(d.GetNextId ()),
    tracker(d.TrackHandle ("recipe", id)),  
	dirtyFields(true),
    owner(o),	
	isNew(true),
	db(d)
{
  VLOG (1)
      << "Created new recipe instance with ID " << id << ": "
      << "crafter recipe=" << cr;
 
  data.SetToDefault ();

  for (const auto& recepie : cfg->recepies())
  {
    if(recepie.second.authoredid() == cr)
    {
      MutableProto().set_authoredid(recepie.second.authoredid());
      MutableProto().set_animationid(recepie.second.animationid());

      for(auto& armor: recepie.second.armor())
      {
          pxd::proto::AuthoredArmor* newArmor = MutableProto().add_armor();
          newArmor->set_candytype(armor.candytype());
          newArmor->set_armortype(armor.armortype());
      }
      
      MutableProto().set_name(recepie.second.name());
      MutableProto().set_duration(recepie.second.duration());
      MutableProto().set_fightername(recepie.second.fightername());
      MutableProto().set_fightertype(recepie.second.fightertype());
      
      for(std::string move : recepie.second.moves())
      {
          std::string* newMove = MutableProto().add_moves();
          newMove->assign(move);
      }
      
      MutableProto().set_quality(recepie.second.quality());
      MutableProto().set_isaccountbound(recepie.second.isaccountbound());
      
      for(auto& candy : recepie.second.requiredcandy())
      {
          pxd::proto::CandyAmount* newCandy = MutableProto().add_requiredcandy();
          newCandy->set_candytype(candy.candytype());
          newCandy->set_amount(candy.amount());
      }      

      MutableProto().set_requiredfighterquality(recepie.second.requiredfighterquality());
    }
  }
  
  Validate ();
}

RecipeInstance::RecipeInstance (Database& d, const std::string& o, const pxd::proto::CraftedRecipe cr, const RoConfig& cfg)
  : id(d.GetNextId ()),
    tracker(d.TrackHandle ("recipe", id)),  
	dirtyFields(true),
    owner(o),	
	isNew(true),
	db(d)
{
  VLOG (1)  << "Created new recipe instance with generated recipe";
  
  data.SetToDefault ();
  
  MutableProto().set_authoredid("generated");
  MutableProto().set_animationid(cr.animationid());

  for(auto& armor: cr.armor())
  {
      pxd::proto::AuthoredArmor* newArmor = MutableProto().add_armor();
      newArmor->set_candytype(armor.candytype());
      newArmor->set_armortype(armor.armortype());
  }
  
  MutableProto().set_name(cr.name());
  MutableProto().set_duration(cr.duration());
  MutableProto().set_fightername(cr.fightername());
  MutableProto().set_fightertype(cr.fightertype());
  
  for(std::string move : cr.moves())
  {
      std::string* newMove = MutableProto().add_moves();
      newMove->assign(move);
  }
  
  MutableProto().set_quality(cr.quality());
  MutableProto().set_isaccountbound(cr.isaccountbound());
  
  for(auto& candy : cr.requiredcandy())
  {
      pxd::proto::CandyAmount* newCandy = MutableProto().add_requiredcandy();
      newCandy->set_candytype(candy.candytype());
      newCandy->set_amount(candy.amount());
  }      

  MutableProto().set_requiredfighterquality(cr.requiredfighterquality());

  Validate ();
}

RecipeInstance::RecipeInstance (Database& d, const Database::Result<RecipeInstanceResult>& res)
  : dirtyFields(false), isNew(false), db(d)
{
  id = res.Get<RecipeInstanceResult::id> ();
  tracker = d.TrackHandle ("recepie", id);
  owner = res.Get<RecipeInstanceResult::owner> ();

  data = res.GetProto<RecipeInstanceResult::proto> ();

  VLOG (2) << "Fetched recipe instance with ID " << id << " from database result";
  Validate ();
}

RecipeInstance::~RecipeInstance ()
{
  Validate ();

  if (isNew || data.IsDirty () || dirtyFields)
  {
      VLOG (2)
          << "Recipe Instance " << id
          << " has been modified including proto data, updating DB";
      auto stmt = db.Prepare (R"(
        INSERT OR REPLACE INTO `recepies`
          (`id`,`owner`, `proto`)
          VALUES
          (?1,
           ?2,
		   ?3)
      )");

      BindFieldValues (stmt);
      stmt.BindProto (3, data);
      stmt.Execute ();

      return;
  }
	
  VLOG (2) << "Recipe Instance " << id << " is not dirty, no update";
}

void
RecipeInstance::Validate () const
{
  CHECK_NE (id, Database::EMPTY_ID);
}

void
RecipeInstance::BindFieldValues (Database::Statement& stmt) const
{
  stmt.Bind (1, id);
  stmt.Bind (2, owner);
}

uint32_t RecipeInstance::Generate(pxd::Quality quality, const RoConfig& cfg,  xaya::Random& rnd, Database& db, std::string owner)
{
    std::vector<pxd::proto::FighterName> potentialNames;
    const auto& fighterNames = cfg->fighternames();
    
    std::vector<std::pair<std::string, pxd::proto::FighterName>> sortedNamesTypesmap;
    for (auto itr = fighterNames.begin(); itr != fighterNames.end(); ++itr)
        sortedNamesTypesmap.push_back(*itr);

    sort(sortedNamesTypesmap.begin(), sortedNamesTypesmap.end(), [=](std::pair<std::string, pxd::proto::FighterName>& a, std::pair<std::string, pxd::proto::FighterName>& b)
    {
        return a.first < b.first;
    } 
    );        
    
    for (const auto& fighter : sortedNamesTypesmap)
    {
        if((Quality)(int)fighter.second.quality() == quality)
        {
            potentialNames.push_back(fighter.second);
        }
    }
    
    std::vector<pxd::proto::FighterName> position0names;
    std::vector<pxd::proto::FighterName> position1names;
    
    for(auto& name: potentialNames)
    {
        if(name.position() == 0)
        {
            position0names.push_back(name);
        }
        
        if(name.position() == 1)
        {
            position1names.push_back(name);
        }        
    }
     
    std::string fname = "";
    std::string lname = "";
    
    if(position0names.size() == 0)
    {
        LOG (ERROR) << "psnm0 The script would and in infinite loop for quality" << (int)quality;
        return 0;
    }
    
    if(position1names.size() == 0)
    {
        LOG (ERROR) << "psnm1 The script would and in infinite loop for quality" << (int)quality;
        return 0;
    }    
    
    std::vector<std::string> candidates0collected;
	std::vector<std::string> candidates1collected;
	
	int32_t biggetRollSoFar = 0;
	while(candidates0collected.size() == 0)
	{
		for(long long unsigned int e =0; e < position0names.size(); e++)
		{
		  int32_t probabilityTreshhold = position0names[e].probability();
		  int32_t rolCurNum = rnd.NextInt(1001);
		  
		  if(probabilityTreshhold < rolCurNum)
		  {			
            if(biggetRollSoFar < probabilityTreshhold)
			{
			   candidates0collected.clear();
               candidates0collected.push_back(position0names[e].name());
               biggetRollSoFar = probabilityTreshhold;			   
			}
            if(biggetRollSoFar == probabilityTreshhold)
			{
               candidates0collected.push_back(position0names[e].name());		   
			}				
		  }
		}
		
		if(candidates0collected.size() == 1)
		{
			fname = candidates0collected[0];
		}
		else
		{
			fname = candidates0collected[rnd.NextInt(candidates0collected.size())];			
		}	
	}
    
    biggetRollSoFar = 0;
	while(candidates1collected.size() == 0)
	{
		for(long long unsigned int e =0; e < position1names.size(); e++)
		{
		  int32_t probabilityTreshhold = position1names[e].probability();
		  int32_t rolCurNum = rnd.NextInt(1001);
		  
		  if(probabilityTreshhold < rolCurNum)
		  {			
            if(biggetRollSoFar < probabilityTreshhold)
			{
			   candidates1collected.clear();
               candidates1collected.push_back(position1names[e].name());
               biggetRollSoFar = probabilityTreshhold;			   
			}
            if(biggetRollSoFar == probabilityTreshhold)
			{
               candidates1collected.push_back(position1names[e].name());			   
			}							
		  }
		}
		
		if(candidates1collected.size() == 1)
		{
			fname = candidates1collected[0];
		}
		else
		{
			fname = candidates1collected[rnd.NextInt(candidates1collected.size())];			
		}	
	}

    pxd::proto::FighterType fighterType;
    const auto& fighterTypes = cfg->fightertype();
    
    std::vector<std::pair<std::string, pxd::proto::FighterType>> sortedfighterTypesmap;
    for (auto itr = fighterTypes.begin(); itr != fighterTypes.end(); ++itr)
        sortedfighterTypesmap.push_back(*itr);

    sort(sortedfighterTypesmap.begin(), sortedfighterTypesmap.end(), [=](std::pair<std::string, pxd::proto::FighterType>& a, std::pair<std::string, pxd::proto::FighterType>& b)
    {
        return a.first < b.first;
    } 
    );    
        
    int probabilityTreshholdMaximum = 0;    
     
    for (const auto& fighter : sortedfighterTypesmap)
    {
        if(probabilityTreshholdMaximum <  fighter.second.probability() * 1000)
        {
            probabilityTreshholdMaximum = fighter.second.probability() * 1000;
        }
    }
        
    std::vector<pxd::proto::FighterType> candidates;
    while(true)
    {
      for (const auto& fighter : sortedfighterTypesmap)
      {
        int probabilityTreshhold = fighter.second.probability() * 1000;
        int rolCurNum = rnd.NextInt(probabilityTreshholdMaximum * 2);

        if(rolCurNum < probabilityTreshhold)
        {
            candidates.push_back(fighter.second);
        }
      }
      
      if(candidates.size() == 1)
      {
          break;
      }
      
      candidates.clear();
    }

    fighterType = candidates[0];

    pxd::proto::CraftedRecipe generatedRecipe;
    
    if(quality == Quality::Common)
    {
      generatedRecipe.set_duration(cfg->params().common_recipe_cook_cost());
    }
    
    if(quality == Quality::Uncommon)
    {
      generatedRecipe.set_duration(cfg->params().uncommon_recipe_cook_cost());
    }

    if(quality == Quality::Rare)
    {
      generatedRecipe.set_duration(cfg->params().rare_recipe_cook_cost());
    }    
    
    if(quality == Quality::Epic)
    {
      generatedRecipe.set_duration(cfg->params().epic_recipe_cook_cost());
    }     
    
    generatedRecipe.set_fightername(fname + " " + lname);
    generatedRecipe.set_name(fname + " " + lname);
    generatedRecipe.set_fightertype(fighterType.authoredid());
    generatedRecipe.set_quality((int)quality);
    generatedRecipe.set_requiredfighterquality((int)Quality::None);
    
    
    if(quality == Quality::Rare)
    {
      generatedRecipe.set_requiredfighterquality((int)Quality::Common);
    }
    
    if(quality == Quality::Epic)
    {
      generatedRecipe.set_requiredfighterquality((int)Quality::Uncommon);
    }
    
    //Generate moves now
    const auto& moveBlueprints = cfg->fightermoveblueprints();
    int numberOfMoves  = cfg->params().common_move_count();
    
    if(quality == Quality::Uncommon)
    {
        numberOfMoves  = cfg->params().uncommon_move_count();
    }
    
    if(quality == Quality::Rare)
    {
        numberOfMoves  = cfg->params().rare_move_count();
    }

    if(quality == Quality::Epic)
    {
        numberOfMoves = cfg->params().epic_move_count();
    }    
        
    std::vector<pxd::proto::FighterMoveBlueprint> generatedMoveblueprints;
    
    std::vector<std::pair<std::string, pxd::proto::FighterMoveBlueprint>> sortedMoveBlueprintTypesmap;
    for (auto itr = moveBlueprints.begin(); itr != moveBlueprints.end(); ++itr)
        sortedMoveBlueprintTypesmap.push_back(*itr);

    sort(sortedMoveBlueprintTypesmap.begin(), sortedMoveBlueprintTypesmap.end(), [=](std::pair<std::string, pxd::proto::FighterMoveBlueprint>& a, std::pair<std::string, pxd::proto::FighterMoveBlueprint>& b)
    {
        return a.first < b.first;
    } 
    );     
    
    probabilityTreshholdMaximum = 0;    
     
    for(auto& probableMove: fighterType.moveprobabilities())
    {
        if(probabilityTreshholdMaximum <  probableMove.probability() * 1000)
        {
            probabilityTreshholdMaximum = probableMove.probability() * 1000;
        }
    }    
    
    for(int g = 0; g < numberOfMoves; g++)
    {
        while(true)
        {
            std::vector<pxd::proto::FighterTypeMoveProbability> validMoves;
            
            for(auto& probableMove: fighterType.moveprobabilities())
            {
                int probabilityTreshhold = probableMove.probability() * 1000;
                
                int rolCurNum = rnd.NextInt(probabilityTreshholdMaximum * 2);  
                if(rolCurNum < probabilityTreshhold)
                {    
                  validMoves.push_back(probableMove);
                } 
            }
            
            if(validMoves.size() == 1)
            {
                  std::vector<std::pair<std::string, pxd::proto::FighterMoveBlueprint>> validCandidates;
                  
                  for(auto& moveBlp: sortedMoveBlueprintTypesmap)
                  {       
                    if( (Quality)(uint32_t)moveBlp.second.quality() == quality)
                    {                        
                      if( moveBlp.second.movetype() == validMoves[0].movetype())
                      {
                        validCandidates.push_back(moveBlp);
                      }
                    }
                  }  
                          
                  generatedMoveblueprints.push_back(validCandidates[rnd.NextInt(validCandidates.size())].second);                          
                  break;
            }
            
            validMoves.clear();
        }
    }
    
    for(long long unsigned int d =0; d < generatedMoveblueprints.size(); d++)
    { 
        std::string* newMove = generatedRecipe.add_moves();
        newMove->assign(generatedMoveblueprints[d].authoredid());
    }

    // We generated candy needed amount based on the moves
    
    uint64_t candyPerMoveAmount = cfg->params().required_candy_per_vove();
    
    std::map<std::string, uint64_t> rcCandyData;
    
    for(const auto& move: generatedRecipe.moves())
    {
        for(auto& moveBlp: sortedMoveBlueprintTypesmap)
        {
          if(moveBlp.second.authoredid() == move)
          {
             std::string candyAuthId = moveBlp.second.candytype();
             if (rcCandyData.find(candyAuthId) == rcCandyData.end())
             {
                rcCandyData.insert(std::pair<std::string, uint64_t>(candyAuthId, candyPerMoveAmount));
             }
             else
             {
                rcCandyData[candyAuthId] += (candyPerMoveAmount);
             }
             
             break;
          }
        }
    }
    
    for(const auto& rcData: rcCandyData)
    {
      pxd::proto::CandyAmount* rcNew = generatedRecipe.add_requiredcandy();
      rcNew->set_candytype(rcData.first);
      rcNew->set_amount(rcData.second);           
    }

    RecipeInstanceTable rt(db);
    
    auto handle = rt.CreateNew(owner, generatedRecipe, cfg); 
    int hdata = handle->GetId();  
    handle.reset();   
    
    return hdata;
}

namespace
{

struct CountResult : public Database::ResultType
{
  RESULT_COLUMN (int64_t, cnt, 1);
};

} // anonymous namespace

unsigned
RecipeInstanceTable::CountForOwner (const std::string& owner)
{
  auto stmt = db.Prepare (R"(
    SELECT COUNT(*) AS `cnt`
      FROM `recepies`
      WHERE `owner` = ?1
      ORDER BY `id`
  )");
  stmt.Bind (1, owner);

  auto res = stmt.Query<CountResult> ();
  CHECK (res.Step ());
  const unsigned count = res.Get<CountResult::cnt> ();
  CHECK (!res.Step ());

  return count;
}

unsigned
RecipeInstanceTable::CountForAll ()
{
  auto stmt = db.Prepare (R"(
    SELECT COUNT(*) AS `cnt`
      FROM `recepies`
  )");

  auto res = stmt.Query<CountResult> ();
  CHECK (res.Step ());
  const unsigned count = res.Get<CountResult::cnt> ();
  CHECK (!res.Step ());
  
  return count;
}


RecipeInstanceTable::Handle
RecipeInstanceTable::CreateNew (const std::string& owner, const pxd::proto::CraftedRecipe cr, const RoConfig& cfg)
{
  return Handle (new RecipeInstance (db, owner, cr, cfg));
}

RecipeInstanceTable::Handle
RecipeInstanceTable::CreateNew (const std::string& owner, const std::string& cr, const RoConfig& cfg)
{
  return Handle (new RecipeInstance (db, owner, cr, cfg));
}

RecipeInstanceTable::Handle
RecipeInstanceTable::GetFromResult (const Database::Result<RecipeInstanceResult>& res)
{
  return Handle (new RecipeInstance (db, res));
}

RecipeInstanceTable::Handle
RecipeInstanceTable::GetFromResult (const Database::Result<RecipeInstanceResult>& res, const RoConfig& cfg)
{
  return Handle (new RecipeInstance (db, res));
}

RecipeInstanceTable::Handle
RecipeInstanceTable::GetById (const Database::IdT id)
{
  auto stmt = db.Prepare ("SELECT * FROM `recepies` WHERE `id` = ?1");
  stmt.Bind (1, id);
  auto res = stmt.Query<RecipeInstanceResult> ();
  if (!res.Step ())
    return nullptr;

  auto c = GetFromResult (res);
  CHECK (!res.Step ());
  return c;
}

Database::Result<RecipeInstanceResult>
RecipeInstanceTable::QueryAll ()
{
  auto stmt = db.Prepare ("SELECT * FROM `recepies` ORDER BY `id`");
  return stmt.Query<RecipeInstanceResult> ();
}

Database::Result<RecipeInstanceResult>
RecipeInstanceTable::QueryForOwner (const std::string& owner)
{
  auto stmt = db.Prepare (R"(
    SELECT * FROM `recepies` WHERE `owner` = ?1 ORDER BY `id`
  )");
  stmt.Bind (1, owner);
  return stmt.Query<RecipeInstanceResult> ();
}

void
RecipeInstanceTable::DeleteById (const Database::IdT id)
{
  VLOG (1) << "Deleting recepie with ID " << id;

  auto stmt = db.Prepare (R"(
    DELETE FROM `recepies` WHERE `id` = ?1
  )");
  stmt.Bind (1, id);
  stmt.Execute ();
}


} // namespace pxd
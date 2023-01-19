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

#include "fighter.hpp"
#include "recipe.hpp"

#include <xayautil/random.hpp>

#include <glog/logging.h>

namespace pxd
{

FighterInstance::FighterInstance (Database& d, const std::string& o, uint32_t r, const RoConfig& cfg, xaya::Random& rnd)
  : id(d.GetNextId ()),
    tracker(d.TrackHandle ("fighter", id)),
    dirtyFields(true),
    owner(o),    
    isNew(true),
    db(d)
{
  VLOG (1)
      << "Created new fighter with ID " << id << ": "
      << "owner=" << o;
 
  data.SetToDefault ();
  
  MutableProto().set_recipeid(r);
  MutableProto().set_tournamentinstanceid(0);
  MutableProto().set_specialtournamentinstanceid(0);
  
  SetStatus(FighterStatus::Available);

  const auto& fighterMoveBlueprintList = cfg->fightermoveblueprints();
  const auto& animations = cfg->animations();
        
  RecipeInstanceTable recipeTbl(db);    
  auto recepie = recipeTbl.GetById(r);
  
  if(recepie == nullptr)
  {
      LOG (ERROR) << "fatal error, could not resolve fighter's recepie with the ID " << r;
      return;
  }
              
  MutableProto().set_fightertypeid(recepie->GetProto().fightertype());  
  MutableProto().set_quality(recepie->GetProto().quality());
  MutableProto().set_rating(GetRatingFromQuality(recepie->GetProto().quality()));
  MutableProto().set_sweetness(0 + recepie->GetProto().quality());
  MutableProto().set_name(recepie->GetProto().fightername());
  MutableProto().set_highestappliedsweetener(0 + recepie->GetProto().quality());
  MutableProto().set_isaccountbound(recepie->GetProto().isaccountbound());
  
  std::map<pxd::ArmorType, std::string> slotsUsed;
  
  std::vector<std::pair<std::string, pxd::proto::FighterMoveBlueprint>> sortedMoveBlueprintTypesmap;
  for (auto itr = fighterMoveBlueprintList.begin(); itr != fighterMoveBlueprintList.end(); ++itr)
      sortedMoveBlueprintTypesmap.push_back(*itr);

  sort(sortedMoveBlueprintTypesmap.begin(), sortedMoveBlueprintTypesmap.end(), [=](std::pair<std::string, pxd::proto::FighterMoveBlueprint>& a, std::pair<std::string, pxd::proto::FighterMoveBlueprint>& b)
  {
      return a.first < b.first;
  } 
  );   
    
  for(auto& move: recepie->GetProto().moves())
  {
       std::string* newmove = MutableProto().add_moves();
       newmove->assign(move);
       
       for(auto& moveBlueprint: sortedMoveBlueprintTypesmap)
       {
           if(moveBlueprint.second.authoredid() == move)
           {   
             std::vector<pxd::ArmorType> aType = ArmorTypeFromMoveType((pxd::MoveType)moveBlueprint.second.movetype());
             pxd::ArmorType randomElement = aType[rnd.NextInt(aType.size())];
             
             if(slotsUsed.find(randomElement) == slotsUsed.end())
             {
                slotsUsed.insert(std::pair<pxd::ArmorType, std::string>(randomElement, moveBlueprint.second.candytype()));
               
                proto::ArmorPiece* newArmorPiece = MutableProto().add_armorpieces();
                newArmorPiece->set_armortype((uint32_t)randomElement);
                newArmorPiece->set_candy(moveBlueprint.second.candytype());
                newArmorPiece->set_rewardsource(0);
                newArmorPiece->set_rewardsourceid("");
             }
           }
       }
       
       std::vector<std::string> animationsToChoiceFrom;
       
        std::vector<std::pair<std::string, pxd::proto::Animation>> sortedAnimations;
        for (auto itr = animations.begin(); itr != animations.end(); ++itr)
            sortedAnimations.push_back(*itr);

        sort(sortedAnimations.begin(), sortedAnimations.end(), [=](std::pair<std::string, pxd::proto::Animation>& a, std::pair<std::string, pxd::proto::Animation>& b)
        {
            return a.first < b.first;
        } 
        );        
       
       for(auto& animation: sortedAnimations)
       {
           if(animation.second.quality() == recepie->GetProto().quality())
           {
               animationsToChoiceFrom.push_back(animation.second.authoredid());
           }
       }
       
       std::string randomAnimationElement = animationsToChoiceFrom[rnd.NextInt(animationsToChoiceFrom.size())];
       MutableProto().set_animationid(randomAnimationElement);            
  }

  recepie.reset();
  Validate ();
}

FighterInstance::FighterInstance (Database& d, const Database::Result<FighterResult>& res, const RoConfig& cfg)
  : dirtyFields(false), isNew(false), db(d)
{
  id = res.Get<FighterResult::id> ();
  tracker = db.TrackHandle ("fighter", id);
  owner = res.Get<FighterResult::owner> ();
  status = GetStatusFromColumn (res);

  data = res.GetProto<FighterResult::proto> ();

  VLOG (2) << "Fetched fighter with ID " << id << " from database result";
  Validate ();
}

std::vector<pxd::ArmorType> FighterInstance::ArmorTypeFromMoveType(pxd::MoveType moveType)
{
  std::vector<pxd::ArmorType> pieceList;
    
  switch(moveType) 
  {
     case pxd::MoveType::Heavy:
        pieceList.push_back(pxd::ArmorType::Head);
        pieceList.push_back(pxd::ArmorType::RightShoulder);
        pieceList.push_back(pxd::ArmorType::LeftShoulder);
        break;
     case pxd::MoveType::Speedy:
        pieceList.push_back(pxd::ArmorType::UpperRightArm);
        pieceList.push_back(pxd::ArmorType::LowerRightArm);
        pieceList.push_back(pxd::ArmorType::UpperLeftArm);
        pieceList.push_back(pxd::ArmorType::LowerLeftArm);
        break;
     case pxd::MoveType::Tricky:
        pieceList.push_back(pxd::ArmorType::RightHand);
        pieceList.push_back(pxd::ArmorType::Torso);
        pieceList.push_back(pxd::ArmorType::LeftHand);
        break;
     case pxd::MoveType::Distance:
        pieceList.push_back(pxd::ArmorType::Waist);
        pieceList.push_back(pxd::ArmorType::UpperRightLeg);
        pieceList.push_back(pxd::ArmorType::UpperLeftLeg);
        break;
     case pxd::MoveType::Blocking:
        pieceList.push_back(pxd::ArmorType::LowerRightLeg);
        pieceList.push_back(pxd::ArmorType::RightFoot);
        pieceList.push_back(pxd::ArmorType::LeftFoot);
        break;        
    
     // you can have any number of case statements.
     default : //Optional
        LOG (WARNING) << "Default should not be triggered for the moveType of type " << (uint32_t)moveType;
  }

  return pieceList;  
}

void FighterInstance::UpdateSweetness()
{
    if(GetProto().rating() > 2000)
    {
        MutableProto().set_sweetness((int)pxd::Sweetness::Super_Sweet);
        return;
    }
    
    double srate = (double)(GetProto().sweetness() * 100.0 + 1000);
    
    if(GetProto().rating() >= srate)
    {
        pxd::Sweetness newSW = (pxd::Sweetness)(((GetProto().rating() - 1000) / 100.0) + 1);
        MutableProto().set_sweetness((int)newSW);
    } 
}

uint32_t
FighterInstance::GetRatingFromQuality (uint32_t quality)
{
    return 1000 + (quality-1) * 100;
}

FighterInstance::~FighterInstance ()
{
  Validate ();

  if (isNew || data.IsDirty () || dirtyFields)
  {
      VLOG (2)
          << "Fighter " << id
          << " has been modified including proto data, updating DB";
      auto stmt = db.Prepare (R"(
        INSERT OR REPLACE INTO `fighters`
          (`id`,`owner`, `proto`, `status`)
          VALUES
          (?1,
           ?2,
           ?3,
           ?4)
      )");

      BindFieldValues (stmt);
      stmt.BindProto (3, data);
      BindStatusParameter (stmt, 4, status);
      stmt.Execute ();

      return;
  }
	
  VLOG (2) << "Fighter " << id << " is not dirty, no update";
}

void
BindStatusParameter (Database::Statement& stmt, const unsigned ind,
                      const FighterStatus f)
{
  stmt.Bind (ind, static_cast<int64_t> (f));
  return;
}

void
FighterInstance::Validate () const
{
  CHECK_NE (id, Database::EMPTY_ID);
}

void
FighterInstance::BindFieldValues (Database::Statement& stmt) const
{
  stmt.Bind (1, id);
  stmt.Bind (2, owner);
}

FighterTable::Handle
FighterTable::CreateNew (const std::string& owner, uint32_t recipe, const RoConfig& cfg, xaya::Random& rnd)
{
  return Handle (new FighterInstance (db, owner, recipe, cfg, rnd));
}

FighterTable::Handle
FighterTable::GetFromResult (const Database::Result<FighterResult>& res, const RoConfig& cfg)
{
  return Handle (new FighterInstance (db, res, cfg));
}

FighterTable::Handle
FighterTable::GetById (const Database::IdT id, const RoConfig& cfg)
{
  auto stmt = db.Prepare ("SELECT * FROM `fighters` WHERE `id` = ?1");
  stmt.Bind (1, id);
  auto res = stmt.Query<FighterResult> ();
  if (!res.Step ())
    return nullptr;

  auto c = GetFromResult (res, cfg);
  CHECK (!res.Step ());
  return c;
}

Database::Result<FighterResult>
FighterTable::QueryAll ()
{
  auto stmt = db.Prepare ("SELECT * FROM `fighters` ORDER BY `id`");
  return stmt.Query<FighterResult> ();
}

Database::Result<FighterResult>
FighterTable::QueryForOwner (const std::string& owner)
{
  auto stmt = db.Prepare (R"(
    SELECT * FROM `fighters` WHERE `owner` = ?1 ORDER BY `id`
  )");
  stmt.Bind (1, owner);
  return stmt.Query<FighterResult> ();
}

namespace
{

struct CountResult : public Database::ResultType
{
  RESULT_COLUMN (int64_t, cnt, 1);
};

} // anonymous namespace

unsigned
FighterTable::CountForOwner (const std::string& owner)
{
  auto stmt = db.Prepare (R"(
    SELECT COUNT(*) AS `cnt`
      FROM `fighters`
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

void
FighterTable::DeleteById (const Database::IdT id)
{
  VLOG (1) << "Deleting fighter with ID " << id;

  auto stmt = db.Prepare (R"(
    DELETE FROM `fighters` WHERE `id` = ?1
  )");
  stmt.Bind (1, id);
  stmt.Execute ();
}


} // namespace pxd
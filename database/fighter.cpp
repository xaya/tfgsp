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
#include "amount.hpp"

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
  
  MutableProto().set_firstnamerarity(recepie->GetProto().firstnamerarity());  
  MutableProto().set_secondnamerarity(recepie->GetProto().secondnamerarity());  
  MutableProto().set_firstname(recepie->GetProto().firstname());  
  MutableProto().set_secondname(recepie->GetProto().secondname());  
  
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

void FighterInstance::RerollName(Amount cost, const RoConfig& cfg,  xaya::Random& rnd, pxd::Quality ql)
{
	MutableProto().set_isnamererolled(true);
	
    std::vector<pxd::proto::FighterName> potentialNames;
    const auto& fighterNames = cfg->fighternames();
    
    std::vector<std::pair<std::string, pxd::proto::FighterName>> sortedNamesTypesmap;
    for (auto itr = fighterNames.begin(); itr != fighterNames.end(); ++itr)
	{
		if((int32_t)itr->second.quality() <= (int32_t)ql)
		{
			sortedNamesTypesmap.push_back(*itr);
		}
	}

    sort(sortedNamesTypesmap.begin(), sortedNamesTypesmap.end(), [=](std::pair<std::string, pxd::proto::FighterName>& a, std::pair<std::string, pxd::proto::FighterName>& b)
    {
        return a.first < b.first;
    } 
    );        
    
    for (const auto& fighter : sortedNamesTypesmap)
    {
        if((Quality)(int32_t)fighter.second.quality() == (Quality)(int32_t)GetProto().quality())
        {
            potentialNames.push_back(fighter.second);
        }
    }
    
    std::vector<pxd::proto::FighterName> position0names;
    std::vector<pxd::proto::FighterName> position1names;
    std::vector<int32_t> candidates0collectedR;
	std::vector<int32_t> candidates1collectedR;		
    
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
    int32_t fnamer = 1000;
    int32_t lnamer = 1000;		
    
    if(position0names.size() == 0)
    {
        LOG (ERROR) << "psnm0 The script would and in infinite loop for quality" << (int32_t)GetProto().quality();
        return;
    }
    
    if(position1names.size() == 0)
    {
        LOG (ERROR) << "psnm1 The script would and in infinite loop for quality" << (int32_t)GetProto().quality();
        return;
    }    
    
    std::vector<std::string> candidates0collected;
	std::vector<std::string> candidates1collected;
	
	int32_t increasedProbability = 0;
	
	std::vector<int32_t> lookUpProbability =  {100,150,200,225,250,275,300,320,340,360,380,400,410,420,430,440,450,460,470,480,490,500,505,510,515,520,525,530,535,540,545,550,555,560,565,570,575,580,585,590,595,600,604,608,612,616,620,624,628,632,636,640,644,648,652,656,660,664,668,672,676,680,684,688,692,696,700,702,704,706,708,710,712,714,716,718,720,722,724,726,728,730,732,734,736,738,740,742,744,746,748,750,752,754,756,758,760,762,764,766,768,770,772,774,776,778,780,782,784,786,788,790,792,794,796,798,800,801,802,803,804,805,806,807,808,809,810,811,812,813,814,815,816,817,818,819,820,821,822,823,824,825,826,827,828,829,830,831,832,833,834,835,836,837,838,839,840,841,842,843,844,845,846,847,848,849,850,851,852,853,854,855,856,857,858,859,860,861,862,863,864,865,866,867,868,869,870,871,872,873,874,875,876,877,878,879,880,881,882,883,884,885,886,887,888,889,890,891,892,893,894,895,896,897,898,899,900,901,902,903,904,905,906,907,908,909,910,911,912,913,914,915,916,917,918,919,920,921,922,923,924,925,926,927,928,929,930,931,932,933,934,935,936,937,938,939,940,941,942,943,944,945,946,947,948,949,950,951,952,953,954,955,956,957,958,959,960,961,962,963,964,965,966,967,968,969,970,971,972,973,974,975,976,977,978,979,980,981,982,983,984,985,986,987,988,989,990,991,992,993,994,995,996,997,998,999};
	std::vector<Amount> lookUpPrices =  {84000000,252000000,672000000,924000000,1344000000,1764000000,2268000000,2772000000,3276000000,3948000000,4620000000,5376000000,5796000000,6216000000,6720000000,7140000000,7644000000,8148000000,8736000000,9324000000,9912000000,10500000000,10836000000,11172000000,11508000000,11844000000,12180000000,12516000000,12852000000,13188000000,13608000000,13944000000,14364000000,14784000000,15120000000,15540000000,15960000000,16380000000,16800000000,17220000000,17724000000,18144000000,18480000000,18900000000,19236000000,19656000000,19992000000,20412000000,20832000000,21168000000,21588000000,22008000000,22428000000,22848000000,23268000000,23688000000,24108000000,24612000000,25032000000,25452000000,25956000000,26376000000,26880000000,27384000000,27804000000,28308000000,28812000000,29064000000,29316000000,29568000000,29820000000,30072000000,30324000000,30576000000,30828000000,31080000000,31332000000,31584000000,31920000000,32172000000,32424000000,32676000000,32928000000,33180000000,33516000000,33768000000,34020000000,34356000000,34608000000,34860000000,35196000000,35448000000,35700000000,36036000000,36288000000,36624000000,36876000000,37128000000,37464000000,37716000000,38052000000,38388000000,38640000000,38976000000,39228000000,39564000000,39900000000,40152000000,40488000000,40824000000,41076000000,41412000000,41748000000,42084000000,42336000000,42672000000,43008000000,43176000000,43344000000,43512000000,43680000000,43848000000,44016000000,44184000000,44352000000,44436000000,44604000000,44772000000,44940000000,45108000000,45276000000,45444000000,45612000000,45780000000,45948000000,46116000000,46284000000,46452000000,46620000000,46788000000,46956000000,47208000000,47376000000,47544000000,47712000000,47880000000,48048000000,48216000000,48384000000,48552000000,48720000000,48888000000,49056000000,49224000000,49392000000,49644000000,49812000000,49980000000,50148000000,50316000000,50484000000,50652000000,50820000000,51072000000,51240000000,51408000000,51576000000,51744000000,51912000000,52164000000,52332000000,52500000000,52668000000,52836000000,53088000000,53256000000,53424000000,53592000000,53844000000,54012000000,54180000000,54348000000,54516000000,54768000000,54936000000,55104000000,55356000000,55524000000,55692000000,55860000000,56112000000,56280000000,56448000000,56700000000,56868000000,57036000000,57204000000,57456000000,57624000000,57792000000,58044000000,58212000000,58464000000,58632000000,58800000000,59052000000,59220000000,59388000000,59640000000,59808000000,60060000000,60228000000,60396000000,60648000000,60816000000,61068000000,61236000000,61404000000,61656000000,61824000000,62076000000,62244000000,62496000000,62664000000,62916000000,63084000000,63336000000,63504000000,63756000000,63924000000,64176000000,64344000000,64596000000,64764000000,65016000000,65184000000,65436000000,65604000000,65856000000,66024000000,66276000000,66444000000,66696000000,66948000000,67116000000,67368000000,67536000000,67788000000,68040000000,68208000000,68460000000,68628000000,68880000000,69132000000,69300000000,69552000000,69804000000,69972000000,70224000000,70476000000,70644000000,70896000000,71148000000,71316000000,71568000000,71820000000,71988000000,72240000000,72492000000,72744000000,72912000000,73164000000,73416000000,73584000000,73836000000,74088000000,74340000000,74592000000,74760000000,75012000000,75264000000,75516000000,75684000000,75936000000,76188000000,76440000000,76692000000,76860000000,77112000000,77364000000,77616000000,77868000000,78120000000,78372000000,78540000000,78792000000,79044000000,79296000000,79548000000,79800000000,80052000000,80304000000,80556000000,80808000000,80976000000,81228000000,81480000000,81732000000,81984000000,82236000000,82488000000,82740000000,82992000000,83244000000,83496000000,83748000000};
	
	int32_t probabilityIndex = 0;
	
	if(cost < lookUpPrices.front())
	{
		increasedProbability = 0;
	}
	else
	{
		for (size_t i = 0; i < lookUpPrices.size(); ++i) 
		{
			if(cost < lookUpPrices[i])
			{
				probabilityIndex = i - 1;
				break;
			}
		}
		
		increasedProbability = lookUpProbability[probabilityIndex];
		
		if(cost > lookUpPrices.back())
	    {
			Amount excessive = cost - lookUpPrices.back();
			increasedProbability += excessive / COIN;
		}
	}
	
	while(candidates0collected.size() == 0)
	{
		for(int32_t e =0; e < (int32_t)position0names.size(); e++)
		{
		  int32_t probabilityTreshhold = position0names[e].probability();
		  int32_t rolCurNum = rnd.NextInt(1001 - increasedProbability);
		  
		  if(probabilityTreshhold > rolCurNum)
		  {			
	         candidates0collectedR.push_back(rolCurNum);
             candidates0collected.push_back(position0names[e].name());			
		  }
		}
		
		if(candidates0collected.size() == 1)
		{
			fname = candidates0collected[0];
			fnamer = candidates0collectedR[0];
		}
		else
		{
			int32_t rDex = rnd.NextInt(candidates0collected.size());
			fname = candidates0collected[rDex];
            fnamer = candidates0collectedR[rDex];			
		}			
	}
    
	while(candidates1collected.size() == 0)
	{
		for(int32_t e =0; e < (int32_t)position1names.size(); e++)
		{
		  int32_t probabilityTreshhold = position1names[e].probability();
		  int32_t rolCurNum = rnd.NextInt(1001 - increasedProbability);
		  
		  if(probabilityTreshhold > rolCurNum)
		  {			
	        candidates1collectedR.push_back(rolCurNum);
            candidates1collected.push_back(position1names[e].name());				
		  }
		}
		
		if(candidates1collected.size() == 1)
		{
			lname = candidates1collected[0];
			lnamer = candidates1collectedR[0];
		}
		else
		{
			int32_t rDex = rnd.NextInt(candidates1collected.size());
			lname = candidates1collected[rDex];	
            lnamer = candidates1collectedR[rDex];			
		}	
	}

	MutableProto().set_firstnamerarity(fnamer);
	MutableProto().set_secondnamerarity(lnamer);	  
	MutableProto().set_firstname(fname);
	MutableProto().set_secondname(lname);	
    MutableProto().set_name(fname + " " + lname);   	
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
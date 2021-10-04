/*
    GSP for the tf blockchain game
    Copyright (C) 2019-2020  Autonomous Worlds Ltd

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

#include "logic.hpp"
#include "database/reward.hpp"

#include "moveprocessor.hpp"

#include "database/schema.hpp"

#include <glog/logging.h>

namespace pxd
{

SQLiteGameDatabase::SQLiteGameDatabase (xaya::SQLiteDatabase& d, PXLogic& g)
  : game(g)
{
  SetDatabase (d);
}

Database::IdT
SQLiteGameDatabase::GetNextId ()
{
  return game.Ids ("pxd").GetNext ();
}

Database::IdT
SQLiteGameDatabase::GetLogId ()
{
  return game.Ids ("log").GetNext ();
}

void
PXLogic::UpdateState (Database& db, xaya::Random& rnd,
                      const xaya::Chain chain,
                      const Json::Value& blockData)
{
  const auto& blockMeta = blockData["block"];
  CHECK (blockMeta.isObject ());
  const auto& heightVal = blockMeta["height"];
  CHECK (heightVal.isUInt64 ());
  const unsigned height = heightVal.asUInt64 ();
  const auto& timestampVal = blockMeta["timestamp"];
  CHECK (timestampVal.isInt64 ());
  const int64_t timestamp = timestampVal.asInt64 ();

  Context ctx(chain, height, timestamp);

  UpdateState (db, rnd, ctx, blockData);
}

std::vector<uint32_t> PXLogic::GenerateActivityReward(const uint32_t fighterID, const std::string blueprintAuthID, const pxd::proto::AuthoredActivityReward rw, const Context& ctx, Database& db, std::unique_ptr<XayaPlayer>& a, xaya::Random& rnd, const uint32_t posInTableList, const std::string basedRewardsTableAuthId)
{
  RecipeInstanceTable recipeTbl(db);
  RewardsTable rewardsTbl(db);
    
  std::vector<uint32_t> iDS;
    
  if((int8_t)rw.type() != (int8_t)RewardType::None)
  {
      auto newReward = rewardsTbl.CreateNew(a->GetName());              
      newReward->MutableProto().set_expeditionid(blueprintAuthID);
      newReward->MutableProto().set_rewardid(basedRewardsTableAuthId);
      newReward->MutableProto().set_positionintable(posInTableList);
      newReward->MutableProto().set_fighterid(fighterID);
      
      LOG (WARNING) << "Processing reward type:" << rw.type();
      
      if((RewardType)(int)rw.type() == RewardType::CraftedRecipe)
      {
          auto newRecipe = recipeTbl.CreateNew("", rw.craftedrecipeid(), ctx.RoConfig());          
          newReward->MutableProto().set_generatedrecipeid(newRecipe->GetId());
          iDS.push_back(newReward->GetId());
          
          LOG (WARNING) << "CraftedRecipe reward generated";
      }
      
      if((RewardType)(int)rw.type() == RewardType::GeneratedRecipe)
      {
          newReward->MutableProto().set_generatedrecipeid(pxd::RecipeInstance::Generate((pxd::Quality)(int)rw.generatedrecipequality(), ctx.RoConfig(), rnd, db));                      
          iDS.push_back(newReward->GetId());
          
          LOG (WARNING) << "GeneratedRecipe reward generated";
      }
      
      if((RewardType)(int)rw.type() == RewardType::List)
      {
          const auto& rewardsList = ctx.RoConfig()->activityrewards();
          for(const auto& rewardsTable: rewardsList)
          {   
            
            if(rewardsTable.second.authoredid() == rw.listtableid())
            {            
              uint32_t posInTableList2 = 0;
              
              for(auto& rw2: rewardsTable.second.rewards())
              {
                  std::vector<uint32_t> newIDS = GenerateActivityReward(fighterID, blueprintAuthID, rw2, ctx, db, a, rnd, posInTableList2, rw.listtableid());
                  
                  for(long long unsigned int j = 0; j < newIDS.size(); j++)
                  {
                    iDS.push_back(newIDS[j]);
                  }
                  
                  posInTableList2++;
              }
            }
            
            
          }     
      }
  }

  return iDS;
}

void PXLogic::ResolveExpedition(std::unique_ptr<XayaPlayer>& a, const std::string blueprintAuthID, const uint32_t fighterID, Database& db, const Context& ctx, xaya::Random& rnd)
{
    const auto& expeditionList = ctx.RoConfig()->expeditionblueprints();
    bool blueprintSolved = false;
    int rollCount = 0;
    
    const auto chain = ctx.Chain ();
    if (blueprintAuthID == "00000000-0000-0000-zzzz-zzzzzzzzzzzz" && chain == xaya::Chain::MAIN)
    {
        LOG (WARNING) << "Unit test blueprint is not allowed on the mainnet " << blueprintAuthID;
        return;
    }
    
    std::string basedRewardsTableAuthId = "";
    
    for(const auto& expedition: expeditionList)
    {
        if(expedition.second.authoredid() == blueprintAuthID)
        {
            rollCount = expedition.second.baserollcount();
            basedRewardsTableAuthId = expedition.second.baserewardstableid();
            blueprintSolved = true;
            break;
        }
    }   
    
    if(blueprintSolved == false)
    {
        LOG (WARNING) << "Could not resolve expedition in logic blueprint with authID: " << blueprintAuthID;
        return;              
    }
    
    pxd::proto::ActivityReward rewardTableDb;
    
    const auto& rewardsList = ctx.RoConfig()->activityrewards();
    bool rewardsSolved = false;
    
    for(const auto& rewardsTable: rewardsList)
    {
        if(rewardsTable.second.authoredid() == basedRewardsTableAuthId)
        {
            rewardTableDb = rewardsTable.second;
            rewardsSolved = true;
            break;
        }
    }     

    if(rewardsSolved == false)
    {
        LOG (WARNING) << "Could not resolve expedition rewards in logic  with authID: " << blueprintAuthID;
        return;             
    }
    
    FighterTable fighters(db);
    FighterTable::Handle fighter;
    fighter = fighters.GetById (fighterID, ctx.RoConfig ());
    
    if(fighter == nullptr)
    {
        LOG (WARNING) << "Could not resolve fighter with id: " << fighterID;
        return;           
    }

    
    fighter->SetStatus(FighterStatus::Available);
    
    uint32_t totalWeight = 0;
    for(auto& rw: rewardTableDb.rewards())
    {
       totalWeight += (uint32_t)rw.weight();
    }

    LOG (WARNING) << "Rolling for total possible awards: " << rollCount;
    
    for(int roll = 0; roll < rollCount; ++roll)
    {
        int rolCurNum = 0;
        
        if(totalWeight != 0)
        {
          rnd.NextInt(totalWeight);
        }
        
        int accumulatedWeight = 0;
        int posInTableList = 0;
        for(auto& rw: rewardTableDb.rewards())
        {
            accumulatedWeight += rw.weight();
            
            if(rolCurNum <= accumulatedWeight)
            {
               GenerateActivityReward(fighterID, blueprintAuthID, rw, ctx, db, a, rnd, posInTableList, basedRewardsTableAuthId);
            }
            
             posInTableList++;
        }
    }

    //If we are in tutorial yet, lets advance tutorial counter
    
    if(a->GetFTUEState() == FTUEState::FirstExpeditionPending)
    {
        a->SetFTUEState(FTUEState::ResolveFirstExpedition);
    }    
}

void PXLogic::ResolveCookingRecepie(std::unique_ptr<XayaPlayer>& a, const uint32_t recepieID, Database& db, const Context& ctx, xaya::Random& rnd)
{
    /* We must check against treat slots once more,
    because at this point player could have more fighters cooked
    from the previously running instances*/
    FighterTable fighters(db);
    RecipeInstanceTable recipeTbl(db);
    
    uint32_t slots = fighters.CountForOwner(a->GetName());   
  
    if(slots >= ctx.RoConfig()->params().max_fighter_inventory_amount())
    {
        LOG (WARNING) << "Need to revert, not enough slots to host a new fighter for recepie with authid " << recepieID;   
        
        std::map<std::string, pxd::Quantity> fungibleItemAmountForDeduction;
        int32_t cookCost = -1;        
        
        pxd::RecipeInstanceTable::Handle recepie = recipeTbl.GetById(recepieID);
        
        if(recepie == nullptr)
        {
            LOG (ERROR) << "Fatal error, could not find recepie with id: " << recepieID;
            return;
        }
        
        if(recepie->GetProto().quality() == 0)
        {
            cookCost = ctx.RoConfig()->params().common_recipe_cook_cost();
        }
        
        if(recepie->GetProto().quality() == 1)
        {
            cookCost = ctx.RoConfig()->params().uncommon_recipe_cook_cost();
        }

        if(recepie->GetProto().quality() == 2)
        {
            cookCost = ctx.RoConfig()->params().rare_recipe_cook_cost();
        }

        if(recepie->GetProto().quality() == 3)
        {
            cookCost = ctx.RoConfig()->params().epic_recipe_cook_cost();
        } 
        
        for(const auto& candyNeeds: recepie->GetProto().requiredcandy())
        {
          std::string candyInventoryName = BaseMoveProcessor::GetCandyKeyNameFromID(candyNeeds.candytype(), ctx);
          pxd::Quantity quantity = candyNeeds.amount();
          fungibleItemAmountForDeduction.insert(std::pair<std::string, pxd::Quantity>(candyInventoryName, quantity));              
        }
        
        auto& playerInventory = a->GetInventory();
        for(const auto& itemToDeduct: fungibleItemAmountForDeduction)
        {
          playerInventory.AddFungibleCount(itemToDeduct.first, itemToDeduct.second);
        }
    
        a->AddBalance(cookCost); 
        
        recepie->SetOwner(a->GetName());
    }
    else
    {
        auto newFighter = fighters.CreateNew (a->GetName(), recepieID, ctx.RoConfig(), rnd);
        a->CalculatePrestige(ctx.RoConfig());
        
        //If we are in tutorial yet, lets advance tutorial counter
        
        if(a->GetFTUEState() == FTUEState::CookingFirstRecipe)
        {
            a->SetFTUEState(FTUEState::FirstExpedition);
        }
        
        if(a->GetFTUEState() == FTUEState::CookingSecondRecipe)
        {
            a->SetFTUEState(FTUEState::FirstTournament);
        }        
        
        /**
        recipeTbl.DeleteById(recepieID);  
        
        now, jere ideally we want to erase it. but, for example, fighter object stores recepie ID,
        and I am not sure will it be ever used somewhere else again? so, FOR NOW, lets not delete,
        just keep empty owner so it marks this as used this way
        */
    }        
}

void PXLogic::TickAndResolveOngoings(Database& db, const Context& ctx, xaya::Random& rnd)
{
    LOG (INFO) << "Ticking ongoing operations.";
    
    XayaPlayersTable xayaplayers(db);
    auto res = xayaplayers.QueryAll ();

    bool tryAndStep = res.Step();
    while (tryAndStep)
    {
      auto a = xayaplayers.GetFromResult (res, ctx.RoConfig ());
      auto ongoings = a->MutableProto().mutable_ongoings();
      
      std::vector<google::protobuf::internal::RepeatedPtrIterator<pxd::proto::OngoinOperation>> forErasing;
      for (auto it = ongoings->begin(); it != ongoings->end(); it++)
      {
          it->set_blocksleft(it->blocksleft() - 1);

          if(it->blocksleft() == 0)
          {
              if((pxd::OngoingType)it->type() == pxd::OngoingType::COOK_RECIPE)
              {
                LOG (INFO) << "Resolving oingoing operation for pxd::OngoingType::COOK_RECIPE";
                
                ResolveCookingRecepie(a, (uint32_t)it->recipeid(), db, ctx, rnd);
                forErasing.push_back(it);
              }
              
              if((pxd::OngoingType)it->type() == pxd::OngoingType::EXPEDITION)
              {
                LOG (INFO) << "Resolving oingoing operation for pxd::OngoingType::EXPEDITION";
                
                ResolveExpedition(a, it->expeditionblueprintid(), (uint32_t)it->fighterdatabaseid(), db, ctx, rnd);
                forErasing.push_back(it);
              }              
          }          
      }
      
      for(uint32_t t = 0; t < forErasing.size(); t++)
      {
          ongoings->erase(forErasing[t]);
      }
      
       tryAndStep = res.Step ();
    }    
}

void
PXLogic::UpdateState (Database& db, xaya::Random& rnd,
                      const Context& ctx, const Json::Value& blockData)
{
  MoveProcessor mvProc(db, rnd, ctx);
  mvProc.ProcessAdmin (blockData["admin"]);
  mvProc.ProcessAll (blockData["moves"]);
  
  /*The first thing we do, is try and resolve all pending ongoing operations
  from the last block. So if there count is minimal 1, they will guarantee
  to get solved before any new moves submitted by player */    
  TickAndResolveOngoings(db, ctx, rnd);

#ifdef ENABLE_SLOW_ASSERTS
  ValidateStateSlow (db, ctx);
#endif // ENABLE_SLOW_ASSERTS
}

void
PXLogic::SetupSchema (xaya::SQLiteDatabase& db)
{
  SetupDatabaseSchema (db);
}

void
PXLogic::GetInitialStateBlock (unsigned& height,
                               std::string& hashHex) const
{
  const xaya::Chain chain = GetChain ();
  switch (chain)
    {
    case xaya::Chain::MAIN:
      height = 3'011'108;
      hashHex
          = "a4c0b18c23f21c15df284710209f089925eb5582b0945cbc317f51899ceeaea4";
      break;

    case xaya::Chain::TEST:
      height = 112'000;
      hashHex
          = "9c5b83a5caaf7f4ce17cc1f38fdb1ed3e3e3e98e43d23d19a4810767d7df38b9";
      break;

    case xaya::Chain::REGTEST:
      height = 0;
      hashHex
          = "6f750b36d22f1dc3d0a6e483af45301022646dfc3b3ba2187865f5a7d6d83ab1";
      break;

    default:
      LOG (FATAL) << "Unexpected chain: " << xaya::ChainToString (chain);
    }
}

void
PXLogic::InitialiseState (xaya::SQLiteDatabase& db)
{
  SQLiteGameDatabase dbObj(db, *this);

  MoneySupply ms(dbObj);
  ms.InitialiseDatabase ();

  /* The initialisation uses up some auto IDs, namely for placed buildings.
     We start "regular" IDs at a later value to avoid shifting them always
     when we tweak initialisation, and thus having to potentially update test
     data and other stuff.  */
  Ids ("pxd").ReserveUpTo (1'000);
}

void
PXLogic::UpdateState (xaya::SQLiteDatabase& db, const Json::Value& blockData)
{
  SQLiteGameDatabase dbObj(db, *this);
  UpdateState (dbObj, GetContext ().GetRandom (),
               GetChain (), blockData);
}

Json::Value
PXLogic::GetStateAsJson (const xaya::SQLiteDatabase& db)
{
  SQLiteGameDatabase dbObj(const_cast<xaya::SQLiteDatabase&> (db), *this);
  const Context ctx(GetChain (),
                    Context::NO_HEIGHT, Context::NO_TIMESTAMP);
  
  GameStateJson gsj(dbObj, ctx);

  return gsj.FullState ();
}

Json::Value
PXLogic::GetCustomStateData (xaya::Game& game, const JsonStateFromRawDb& cb)
{
  return SQLiteGame::GetCustomStateData (game, "data",
      [this, &cb] (const xaya::SQLiteDatabase& db, const xaya::uint256& hash,
                   const unsigned height)
        {
          SQLiteGameDatabase dbObj(const_cast<xaya::SQLiteDatabase&> (db),
                                   *this);
          return cb (dbObj, hash, height);
        });
}

Json::Value
PXLogic::GetCustomStateData (xaya::Game& game,
                             const JsonStateFromDatabaseWithBlock& cb)
{
  return GetCustomStateData (game,
    [this, &cb] (Database& db, const xaya::uint256& hash, const unsigned height)
        {
          const Context ctx(GetChain (),
                            Context::NO_HEIGHT, Context::NO_TIMESTAMP);
          GameStateJson gsj(db, ctx);
          return cb (gsj, hash, height);
        });
}

Json::Value
PXLogic::GetCustomStateData (xaya::Game& game, const JsonStateFromDatabase& cb)
{
  return GetCustomStateData (game,
    [&cb] (GameStateJson& gsj, const xaya::uint256& hash, const unsigned height)
    {
      return cb (gsj);
    });
}


void
PXLogic::ValidateStateSlow (Database& db, const Context& ctx)
{
  LOG (INFO) << "Performing slow validation of the game-state database...";
}

} // namespace pxd

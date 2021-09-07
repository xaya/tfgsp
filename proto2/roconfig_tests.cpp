/*
    GSP for the Taurion blockchain game
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

#include "../proto/roconfig.hpp"

#include <glog/logging.h>
#include <gtest/gtest.h>

namespace pxd
{
namespace
{

/* ************************************************************************** */

TEST (RoConfigTests, ConstructionWorks)
{
  *RoConfig (xaya::Chain::MAIN);
  *RoConfig (xaya::Chain::TEST);
  *RoConfig (xaya::Chain::REGTEST);
}

TEST (RoConfigTests, ProtoIsSingleton)
{
  const auto* ptr1 = &(*RoConfig (xaya::Chain::MAIN));
  const auto* ptr2 = &(*RoConfig (xaya::Chain::MAIN));
  const auto* ptr3 = &(*RoConfig (xaya::Chain::REGTEST));
  EXPECT_EQ (ptr1, ptr2);
  EXPECT_NE (ptr1, ptr3);
}

TEST (RoConfigTests, ChainDependence)
{
  const RoConfig main(xaya::Chain::MAIN);
  const RoConfig test(xaya::Chain::TEST);
  const RoConfig regtest(xaya::Chain::REGTEST);
  
  EXPECT_EQ (main->params ().dev_addr (), "CGUpAcjsb6MDktSYg8yRDxDutr7FhWtdWC");
  EXPECT_EQ (test->params ().dev_addr (), "dSFDAWxovUio63hgtfYd3nz3ir61sJRsXn");
  EXPECT_EQ (regtest->params ().dev_addr (), "dHNvNaqcD7XPDnoRjAoyfcMpHRi5upJD7p");

  EXPECT_FALSE (main->params ().god_mode ());
  EXPECT_FALSE (test->params ().god_mode ());
  EXPECT_TRUE (regtest->params ().god_mode ());
}

/* ************************************************************************** */

class RoConfigSanityTests : public testing::Test
{

protected:

  /**
   * Checks if the given config instance is valid.  This verifies things
   * like that item types referenced from other item configs (e.g. what
   * something refines to) are actually valid.
   */
  static bool IsConfigValid (const RoConfig& cfg);

};

bool
RoConfigSanityTests::IsConfigValid (const RoConfig& cfg)
{
  if (cfg->has_testnet_merge () || cfg->has_regtest_merge ())
  {
      LOG (WARNING) << "Merge data still present";
      return false;
  }
  
  for (const auto& entry : cfg->activityrewards ())
  {
      const auto& i = entry.second;
      
      if (!i.has_authoredid ())
      {
          LOG (WARNING) << "No has_authoredid defined for activityrewards: " << entry.first;
          return false;
      }   

      if (!i.has_name ())
      {
          LOG (WARNING) << "No Name defined for activityrewards: " << entry.first;
          return false;
      } 

      if (!(i.rewards_size() > 0))
      {
          LOG (WARNING) << "No rewards defined for activityrewards: " << entry.first;
          return false;
      }       

      for (const auto& entry2 : i.rewards())
      {
         if (!entry2.has_type ())
         {
            LOG (WARNING) << "No type defined for activityrewards: " << entry.first;
            return false;
         }   

         if (!entry2.has_generatedrecipequality())
         {
            LOG (WARNING) << "No has_generatedrecipequality defined for activityrewards: " << entry.first;
            return false;
         } 

         if (!entry2.has_craftedrecipeid())
         {
            LOG (WARNING) << "No has_craftedrecipeid defined for activityrewards: " << entry.first;
            return false;
         }  
         
         if (!entry2.has_moveid())
         {
            LOG (WARNING) << "No MoveId defined for activityrewards: " << entry.first;
            return false;
         }   

         if (!entry2.has_armortype())
         {
            LOG (WARNING) << "No ArmorType defined for activityrewards: " << entry.first;
            return false;
         }   

         if (!entry2.has_animationid())
         {
            LOG (WARNING) << "No AnimationId defined for activityrewards: " << entry.first;
            return false;
         }   

         if (!entry2.has_candytype())
         {
            LOG (WARNING) << "No candytype defined for activityrewards: " << entry.first;
            return false;
         }  

         if (!entry2.has_listtableid())
         {
            LOG (WARNING) << "No has_listtableid defined for activityrewards: " << entry.first;
            return false;
         }     
 
         if (!entry2.has_quantity())
         {
            LOG (WARNING) << "No Quantity defined for activityrewards: " << entry.first;
            return false;
         }   

         if (!entry2.has_weight())
         {
            LOG (WARNING) << "No Weight defined for activityrewards: " << entry.first;
            return false;
         }       

         /*If GUID is not zero, we must cross check it. There are also ZERO ones, dunno if its bug or not*/
         bool GUIDCrossCheckIsValid = false;
         bool GUIDNeedsCrossCheck = false;
         if(entry2.craftedrecipeid() != "00000000-0000-0000-0000-000000000000")
         {
            GUIDNeedsCrossCheck = true;
            if(GUIDCrossCheckIsValid == false)
            {
              for (const auto& entry3 : cfg->recepies ())
              {
                const auto& i3 = entry3.second;

                if (i3.authoredid () == entry2.craftedrecipeid())
                {        
                      GUIDCrossCheckIsValid = true;
                      break;
                }
              }
            }
         }
         
         if(entry2.moveid() != "00000000-0000-0000-0000-000000000000")
         {     
            GUIDNeedsCrossCheck = true;
            if(GUIDCrossCheckIsValid == false)
            {
              for (const auto& entry3 : cfg->fightermoveblueprints ())
              {
                const auto& i3 = entry3.second;

                if (i3.authoredid () == entry2.moveid())
                {        
                      GUIDCrossCheckIsValid = true;
                      break;
                }
              }
            }
         }
          
         if(entry2.animationid() != "00000000-0000-0000-0000-000000000000")
         {       
            GUIDNeedsCrossCheck = true;     
            if(GUIDCrossCheckIsValid == false)
            {
              for (const auto& entry3 : cfg->animations ())
              {
                const auto& i3 = entry3.second;

                if (i3.authoredid () == entry2.animationid())
                {        
                      GUIDCrossCheckIsValid = true;
                      break;
                }
              }
            }  
         }

         if(entry2.candytype() != "00000000-0000-0000-0000-000000000000")
         { 
            GUIDNeedsCrossCheck = true;
            if(GUIDCrossCheckIsValid == false)
            {
              for (const auto& entry3 : cfg->candies())
              {
                const auto& i3 = entry3.second;

                if (i3.authoredid () == entry2.candytype())
                {        
                      GUIDCrossCheckIsValid = true;
                      break;
                }
              }
            }                
         }
      
         if(entry2.listtableid() != "00000000-0000-0000-0000-000000000000")
         { 
            GUIDNeedsCrossCheck = true;
            if(GUIDCrossCheckIsValid == false)
            {
              for (const auto& entry3 : cfg->activityrewards())
              {
                const auto& i3 = entry3.second;

                if (i3.authoredid () == entry2.listtableid())
                {        
                      GUIDCrossCheckIsValid = true;
                      break;
                }
              }
            }                
         }
         
         if(GUIDNeedsCrossCheck == true && GUIDCrossCheckIsValid == false)
         {
            LOG (WARNING) << "GUID cross-check failed for activityrewards: " << entry.first;
            return false;            
         }         
      }          
  }
  
  for (const auto& entry : cfg->animations ())
  {
      const auto& i = entry.second;
      
      if (!i.has_authoredid())
      {
          LOG (WARNING) << "No has_authoredid defined for animations: " << entry.first;
          return false;
      }

      if (!i.has_name())
      {
          LOG (WARNING) << "No has_name defined for animations: " << entry.first;
          return false;
      } 

      if (!i.has_quality())
      {
          LOG (WARNING) << "No has_quality defined for animations: " << entry.first;
          return false;
      }   

      if (!i.has_clip())
      {
          LOG (WARNING) << "No has_clip defined for animations: " << entry.first;
          return false;
      }       
  }

  for (const auto& entry : cfg->candies ())
  {
      const auto& i = entry.second;
      
      if (!i.has_authoredid())
      {
          LOG (WARNING) << "No has_authoredid defined for candies: " << entry.first;
          return false;
      }

      if (!i.has_name())
      {
          LOG (WARNING) << "No has_name defined for candies: " << entry.first;
          return false;
      }     
  }

  for (const auto& entry : cfg->crystalbundles ())
  {
      const auto& i = entry.second;
      
      if (!i.has_authoredid())
      {
          LOG (WARNING) << "No has_authoredid defined for crystalbundles: " << entry.first;
          return false;
      }  

      if (!i.has_quantity())
      {
          LOG (WARNING) << "No has_quantity defined for crystalbundles: " << entry.first;
          return false;
      }    

      if (!i.has_chicost())
      {
          LOG (WARNING) << "No has_chicost defined for crystalbundles: " << entry.first;
          return false;
      }          
  }

  for (const auto& entry : cfg->expeditionblueprints ())
  {
      const auto& i = entry.second;
      
      if (!i.has_authoredid())
      {
          LOG (WARNING) << "No has_authoredid defined for expeditionblueprints: " << entry.first;
          return false;
      }

      if (!i.has_name())
      {
          LOG (WARNING) << "No has_name defined for expeditionblueprints: " << entry.first;
          return false;
      }    

      if (!i.has_name())
      {
          LOG (WARNING) << "No has_name defined for expeditionblueprints: " << entry.first;
          return false;
      }  

      if (!i.has_duration())
      {
          LOG (WARNING) << "No has_duration defined for expeditionblueprints: " << entry.first;
          return false;
      }  

      if (!i.has_minsweetness())
      {
          LOG (WARNING) << "No has_minsweetness defined for expeditionblueprints: " << entry.first;
          return false;
      }

      if (!i.has_maxsweetness())
      {
          LOG (WARNING) << "No has_maxsweetness defined for expeditionblueprints: " << entry.first;
          return false;
      }

      if (!i.has_maxrewardquality())
      {
          LOG (WARNING) << "No has_maxrewardquality defined for expeditionblueprints: " << entry.first;
          return false;
      }     

      if (!i.has_baserewardstableid())
      {
          LOG (WARNING) << "No BaseRewardsTableId defined for expeditionblueprints: " << entry.first;
          return false;
      }  

      if (!i.has_baserollcount())
      {
          LOG (WARNING) << "No BaseRollCount defined for expeditionblueprints: " << entry.first;
          return false;
      }
      
      if(i.baserewardstableid() == "00000000-0000-0000-0000-000000000000")
      {
          LOG (WARNING) << "baserewardstableid GUID is null/zero for for expeditionblueprints: " << entry.first;
          return false;          
      }

      bool GUIDCrossCheckIsValid = false;

      for (const auto& entry3 : cfg->activityrewards ())
      {
        const auto& i3 = entry3.second;

        if (i3.authoredid () == i.baserewardstableid())
        {        
                GUIDCrossCheckIsValid = true;
                break;
        }
      }
      
      if(GUIDCrossCheckIsValid == false)
      {
        LOG (WARNING) << "GUID cross-check failed for expeditionblueprints: " << entry.first;
        return false;            
      }             
  }

  for (const auto& entry : cfg->fightermoveblueprints ())
  {
      const auto& i = entry.second;
      
      if (!i.has_authoredid())
      {
          LOG (WARNING) << "No has_authoredid defined for fightermoveblueprints: " << entry.first;
          return false;
      }

      if (!i.has_name())
      {
          LOG (WARNING) << "No has_name defined for fightermoveblueprints: " << entry.first;
          return false;
      }     

      if (!i.has_candytype())
      {
          LOG (WARNING) << "No has_candytype defined for fightermoveblueprints: " << entry.first;
          return false;
      }    

      if (!i.has_quality())
      {
          LOG (WARNING) << "No has_quality defined for fightermoveblueprints: " << entry.first;
          return false;
      }    

      if (!i.has_movetype())
      {
          LOG (WARNING) << "No has_movetype defined for fightermoveblueprints: " << entry.first;
          return false;
      }       

      if(i.candytype() == "00000000-0000-0000-0000-000000000000")
      {
          LOG (WARNING) << "baserewardstableid GUID is null/zero for for fightermoveblueprints: " << entry.first;
          return false;          
      }

      bool GUIDCrossCheckIsValid = false;

      for (const auto& entry3 : cfg->candies())
      {
        const auto& i3 = entry3.second;

        if (i3.authoredid () == i.candytype())
        {        
                GUIDCrossCheckIsValid = true;
                break;
        }
      }
      
      if(GUIDCrossCheckIsValid == false)
      {
        LOG (WARNING) << "GUID cross-check failed for fightermoveblueprints: " << entry.first;
        return false;            
      }       
  }

  for (const auto& entry : cfg->fighternames ())
  {
      const auto& i = entry.second;
      
      if (!i.has_authoredid())
      {
          LOG (WARNING) << "No has_authoredid defined for fighternames: " << entry.first;
          return false;
      }

      if (!i.has_name())
      {
          LOG (WARNING) << "No has_name defined for fighternames: " << entry.first;
          return false;
      }   

      if (!i.has_position())
      {
          LOG (WARNING) << "No has_position defined for fighternames: " << entry.first;
          return false;
      }  

      if (!i.has_quality())
      {
          LOG (WARNING) << "No has_quality defined for fighternames: " << entry.first;
          return false;
      }  

      if (!i.has_probability())
      {
          LOG (WARNING) << "No has_probability defined for fighternames: " << entry.first;
          return false;
      }       
  }

  for (const auto& entry : cfg->fightertype ())
  {
      const auto& i = entry.second;
      
      if (!i.has_authoredid())
      {
          LOG (WARNING) << "No has_authoredid defined for fightertype: " << entry.first;
          return false;
      }

      if (!i.has_name())
      {
          LOG (WARNING) << "No has_name defined for fightertype: " << entry.first;
          return false;
      }  

      if (!i.has_probability())
      {
          LOG (WARNING) << "No has_probability defined for fightertype: " << entry.first;
          return false;
      }   

      if (!(i.moveprobabilities_size() > 0))
      {
          LOG (WARNING) << "No moveprobabilities_size defined for fightertype: " << entry.first;
          return false;
      }       

      for (const auto& entry2 : i.moveprobabilities())
      {
         if (!entry2.has_movetype ())
         {
            LOG (WARNING) << "No has_movetype defined for fightertype: " << entry.first;
            return false;
         }

         if (!entry2.has_probability ())
         {
            LOG (WARNING) << "No has_probability defined for fightertype: " << entry.first;
            return false;
         }  

         if (!entry2.has_authoredid ())
         {
            LOG (WARNING) << "No has_authoredid defined for fightertype: " << entry.first;
            return false;
         }          
      }         
  }

  for (const auto& entry : cfg->goodies ())
  {
      const auto& i = entry.second;
      
      if (!i.has_authoredid())
      {
          LOG (WARNING) << "No has_authoredid defined for goodies: " << entry.first;
          return false;
      }

      if (!i.has_name())
      {
          LOG (WARNING) << "No has_name defined for goodies: " << entry.first;
          return false;
      } 

      if (!i.has_description())
      {
          LOG (WARNING) << "No has_description defined for goodies: " << entry.first;
          return false;
      } 

      if (!i.has_price())
      {
          LOG (WARNING) << "No has_price defined for goodies: " << entry.first;
          return false;
      }   

      if (!i.has_goodytype())
      {
          LOG (WARNING) << "No has_goodytype defined for goodies: " << entry.first;
          return false;
      } 

      if (!i.has_uses())
      {
          LOG (WARNING) << "No has_uses defined for goodies: " << entry.first;
          return false;
      }  

      if (!i.has_reductionpercent())
      {
          LOG (WARNING) << "No has_reductionpercent defined for goodies: " << entry.first;
          return false;
      }       
  }

  for (const auto& entry : cfg->goodybundles ())
  {
      const auto& i = entry.second;
      
      if (!i.has_authoredid())
      {
          LOG (WARNING) << "No has_authoredid defined for goodybundles: " << entry.first;
          return false;
      }

      if (!i.has_name())
      {
          LOG (WARNING) << "No has_name defined for goodybundles: " << entry.first;
          return false;
      }       
      
      if (!i.has_price())
      {
          LOG (WARNING) << "No has_price defined for goodybundles: " << entry.first;
          return false;
      }     

      if (!(i.bundledgoodies_size() > 0))
      {
          LOG (WARNING) << "No bundledgoodies_size defined for goodybundles: " << entry.first;
          return false;
      }       

      for (const auto& entry2 : i.bundledgoodies())
      {
         if (!entry2.has_goodyid ())
         {
            LOG (WARNING) << "No has_goodyid defined for goodybundles: " << entry.first;
            return false;
         }

         if (!entry2.has_quantity ())
         {
            LOG (WARNING) << "No has_quantity defined for goodybundles: " << entry.first;
            return false;
         }       

         if(entry2.goodyid() == "00000000-0000-0000-0000-000000000000")
         {
              LOG (WARNING) << "goodyid GUID is null/zero for for goodybundles: " << entry.first;
              return false;          
         }

         bool GUIDCrossCheckIsValid = false;

         for (const auto& entry3 : cfg->goodies())
         {
            const auto& i3 = entry3.second;

            if (i3.authoredid () == entry2.goodyid())
            {        
                    GUIDCrossCheckIsValid = true;
                    break;
            }
         }
          
         if(GUIDCrossCheckIsValid == false)
         {
            LOG (WARNING) << "GUID cross-check failed for goodybundles: " << entry.first;
            return false;            
         }                
      }         
  }

  for (const auto& entry : cfg->sweetenerblueprints ())
  {
      const auto& i = entry.second;
      
      if (!i.has_authoredid())
      {
          LOG (WARNING) << "No has_authoredid defined for sweetenerblueprints: " << entry.first;
          return false;
      }

      if (!i.has_name())
      {
          LOG (WARNING) << "No has_name defined for sweetenerblueprints: " << entry.first;
          return false;
      }

      if (!i.has_description())
      {
          LOG (WARNING) << "No has_description defined for sweetenerblueprints: " << entry.first;
          return false;
      }    

      if (!i.has_price())
      {
          LOG (WARNING) << "No has_price defined for sweetenerblueprints: " << entry.first;
          return false;
      }    

      if (!i.has_duration())
      {
          LOG (WARNING) << "No has_duration defined for sweetenerblueprints: " << entry.first;
          return false;
      } 

      if (!i.has_cookcost())
      {
          LOG (WARNING) << "No has_cookcost defined for sweetenerblueprints: " << entry.first;
          return false;
      }   

      if (!i.has_requiredsweetness())
      {
          LOG (WARNING) << "No has_requiredsweetness defined for sweetenerblueprints: " << entry.first;
          return false;
      }   

      if (!(i.rewardchoices_size() > 0))
      {
          LOG (WARNING) << "No rewardchoices_size defined for sweetenerblueprints: " << entry.first;
          return false;
      }       

      for (const auto& entry2 : i.rewardchoices())
      {
         if (!entry2.has_description())
         {
            LOG (WARNING) << "No has_description defined for rewardchoices: " << entry.first;
            return false;
         }

         if (!entry2.has_rewardstableid())
         {
            LOG (WARNING) << "No has_rewardstableid defined for rewardchoices: " << entry.first;
            return false;
         } 

         if (!entry2.has_baserollcount())
         {
            LOG (WARNING) << "No has_baserollcount defined for rewardchoices: " << entry.first;
            return false;
         }  

         if (!entry2.has_armorrewardstableid())
         {
            LOG (WARNING) << "No has_armorrewardstableid defined for rewardchoices: " << entry.first;
            return false;
         }       

         if (!entry2.has_armorrollcount())
         {
            LOG (WARNING) << "No has_armorrollcount defined for rewardchoices: " << entry.first;
            return false;
         }   

         if (!entry2.has_moverewardstableid())
         {
            LOG (WARNING) << "No has_moverewardstableid defined for rewardchoices: " << entry.first;
            return false;
         }  

         if (!entry2.has_moverollcount())
         {
            LOG (WARNING) << "No has_moverollcount defined for rewardchoices: " << entry.first;
            return false;
         }   

         if (!(entry2.requiredcandy_size() > 0))
         {
            LOG (WARNING) << "No has_requiredcandy defined for rewardchoices: " << entry.first;
            return false;
         }        

         if(entry2.rewardstableid() == "00000000-0000-0000-0000-000000000000")
         {
              LOG (WARNING) << "goodyid GUID is null/zero for for rewardchoices: " << entry.first;
              return false;          
         }

         bool GUIDCrossCheckIsValid = false;

         for (const auto& entry3 : cfg->activityrewards())
         {
            const auto& i3 = entry3.second;

            if (i3.authoredid () == entry2.rewardstableid())
            {        
                    GUIDCrossCheckIsValid = true;
                    break;
            }
         }
          
         if(GUIDCrossCheckIsValid == false)
         {
            LOG (WARNING) << "GUID cross-check failed for rewardchoices: " << entry.first;
            return false;            
         } 
         
         if(entry2.armorrewardstableid() == "00000000-0000-0000-0000-000000000000")
         {
              LOG (WARNING) << "goodyid GUID is null/zero for for rewardchoices: " << entry.first;
              return false;          
         }

        GUIDCrossCheckIsValid = false;

         for (const auto& entry3 : cfg->activityrewards())
         {
            const auto& i3 = entry3.second;

            if (i3.authoredid () == entry2.armorrewardstableid())
            {        
                    GUIDCrossCheckIsValid = true;
                    break;
            }
         }
          
         if(GUIDCrossCheckIsValid == false)
         {
            LOG (WARNING) << "GUID cross-check failed for rewardchoices: " << entry.first;
            return false;            
         }          
         
         if(entry2.moverewardstableid() == "00000000-0000-0000-0000-000000000000")
         {
              LOG (WARNING) << "goodyid GUID is null/zero for for rewardchoices: " << entry.first;
              return false;          
         }

        GUIDCrossCheckIsValid = false;

         for (const auto& entry3 : cfg->activityrewards())
         {
            const auto& i3 = entry3.second;

            if (i3.authoredid () == entry2.moverewardstableid())
            {        
                    GUIDCrossCheckIsValid = true;
                    break;
            }
         }
          
         if(GUIDCrossCheckIsValid == false)
         {
            LOG (WARNING) << "GUID cross-check failed for rewardchoices: " << entry.first;
            return false;            
         }        

         for (const auto& entry4 : entry2.requiredcandy())
         {
           GUIDCrossCheckIsValid = false;
           
           if(entry4.first == "00000000-0000-0000-0000-000000000000")
           {
                LOG (WARNING) << "goodyid GUID is null/zero for for rewardchoices: " << entry.first;
                return false;          
           }  
           
           for (const auto& entry3 : cfg->candies())
           {
              const auto& i3 = entry3.second;

              if (i3.authoredid () == entry4.first)
              {        
                      GUIDCrossCheckIsValid = true;
                      break;
              }
           }  

           if(GUIDCrossCheckIsValid == false)
           {
              LOG (WARNING) << "GUID cross-check failed for rewardchoices: " << entry.first;
              return false;            
           }           
         }           
      }      
  }

  for (const auto& entry : cfg->tournamentbluprints ())
  {
      const auto& i = entry.second;
      
      if (!i.has_authoredid())
      {
          LOG (WARNING) << "No has_authoredid defined for tournamentbluprints: " << entry.first;
          return false;
      }

      if (!i.has_name())
      {
          LOG (WARNING) << "No has_name defined for tournamentbluprints: " << entry.first;
          return false;
      }  

      if (!i.has_duration())
      {
          LOG (WARNING) << "No has_duration defined for tournamentbluprints: " << entry.first;
          return false;
      }  

      if (!i.has_teamcount())
      {
          LOG (WARNING) << "No has_teamcount defined for tournamentbluprints: " << entry.first;
          return false;
      }     

      if (!i.has_teamsize())
      {
          LOG (WARNING) << "No has_teamsize defined for tournamentbluprints: " << entry.first;
          return false;
      }  

      if (!i.has_minsweetness())
      {
          LOG (WARNING) << "No has_minsweetness defined for tournamentbluprints: " << entry.first;
          return false;
      }     

      if (!i.has_maxsweetness())
      {
          LOG (WARNING) << "No has_maxsweetness defined for tournamentbluprints: " << entry.first;
          return false;
      }   

      if (!i.has_maxrewardquality())
      {
          LOG (WARNING) << "No has_maxrewardquality defined for tournamentbluprints: " << entry.first;
          return false;
      }      

      if (!i.has_baserewardstableid())
      {
          LOG (WARNING) << "No has_baserewardstableid defined for tournamentbluprints: " << entry.first;
          return false;
      }    

      if (!i.has_baserollcount())
      {
          LOG (WARNING) << "No has_baserollcount defined for tournamentbluprints: " << entry.first;
          return false;
      } 

      if (!i.has_winnerrewardstableid())
      {
          LOG (WARNING) << "No has_winnerrewardstableid defined for tournamentbluprints: " << entry.first;
          return false;
      }   

      if (!i.has_winnerrollcount())
      {
          LOG (WARNING) << "No has_winnerrollcount defined for tournamentbluprints: " << entry.first;
          return false;
      }  

      if (!i.has_joincost())
      {
          LOG (WARNING) << "No has_joincost defined for tournamentbluprints: " << entry.first;
          return false;
      }   
      
      if(i.baserewardstableid() == "00000000-0000-0000-0000-000000000000")
      {
          LOG (WARNING) << "baserewardstableid GUID is null/zero for for tournamentbluprints: " << entry.first;
          return false;          
      }      

      bool GUIDCrossCheckIsValid = false;

      for (const auto& entry3 : cfg->activityrewards())
      {
        const auto& i3 = entry3.second;

        if (i3.authoredid () == i.baserewardstableid())
        {        
                GUIDCrossCheckIsValid = true;
                break;
        }
      }
      
      if(GUIDCrossCheckIsValid == false)
      {
        LOG (WARNING) << "GUID cross-check failed for tournamentbluprints: " << entry.first;
        return false;            
      }   

      if(i.winnerrewardstableid() == "00000000-0000-0000-0000-000000000000")
      {
          LOG (WARNING) << "winnerrewardstableid GUID is null/zero for for tournamentbluprints: " << entry.first;
          return false;          
      }      

      GUIDCrossCheckIsValid = false;

      for (const auto& entry3 : cfg->activityrewards())
      {
        const auto& i3 = entry3.second;

        if (i3.authoredid () == i.winnerrewardstableid())
        {        
                GUIDCrossCheckIsValid = true;
                break;
        }
      }
      
      if(GUIDCrossCheckIsValid == false)
      {
        LOG (WARNING) << "GUID cross-check failed for tournamentbluprints: " << entry.first;
        return false;            
      }          
  }  


  for (const auto& entry : cfg->recepies ())
  {
      const auto& i = entry.second;

      if (!i.has_authoredid ())
      {
          LOG (WARNING) << "No AuthoredId defined for recepies: " << entry.first;
          return false;
      }

      if (!i.has_animationid ())
      {
          LOG (WARNING) << "No AnimationId defined for recepies: " << entry.first;
          return false;
      } 

      if (!(i.armor_size() > 0))
      {
          LOG (WARNING) << "No Armor defined for recepies: " << entry.first;
          return false;
      }

      if (!i.has_name ())
      {
          LOG (WARNING) << "No Name defined for recepies: " << entry.first;
          return false;
      }

      if (!i.has_duration ())
      {
          LOG (WARNING) << "No Duration defined for recepies: " << entry.first;
          return false;
      }
      
      if (!i.has_fightername ())
      {
          LOG (WARNING) << "No FighterName defined for recepies: " << entry.first;
          return false;
      }      

      if (!i.has_fightertype ())
      {
          LOG (WARNING) << "No FighterType defined for recepies: " << entry.first;
          return false;
      }

      if (!(i.moves_size() > 0))
      {
          LOG (WARNING) << "No Moves defined for recepies: " << entry.first;
          return false;
      }

      if (!i.has_quality ())
      {
          LOG (WARNING) << "No Quality defined for recepies: " << entry.first;
          return false;
      }

      if (!i.has_isaccountbound ())
      {
          LOG (WARNING) << "No IsAccountBound defined for recepies: " << entry.first;
          return false;
      }

      if (!(i.requiredcandy_size() > 0))
      {
          /*this might happen only for InvestorReward recepies*/
          
          if(entry.first.find("InvestorReward") == std::string::npos)
          {
          LOG (WARNING) << "No RequiredCandy defined for recepies: " << entry.first;
          return false;
          }
      }   

      if (!i.has_requiredfighterquality ())
      {
          LOG (WARNING) << "No RequiredFighterQuality defined for recepies: " << entry.first;
          return false;
      }       
      
      if(i.animationid() == "00000000-0000-0000-0000-000000000000")
      {
          LOG (WARNING) << "animationid GUID is null/zero for for recepies: " << entry.first;
          return false;          
      }      

      bool GUIDCrossCheckIsValid = false;

      for (const auto& entry3 : cfg->animations())
      {
        const auto& i3 = entry3.second;

        if (i3.authoredid () == i.animationid())
        {        
                GUIDCrossCheckIsValid = true;
                break;
        }
      }
      
      if(GUIDCrossCheckIsValid == false)
      {
        LOG (WARNING) << "GUID cross-check failed for recepies: " << entry.first;
        return false;            
      }  

      if(i.fightertype() == "00000000-0000-0000-0000-000000000000")
      {
          LOG (WARNING) << "fightertype GUID is null/zero for for recepies: " << entry.first;
          return false;          
      }      

      GUIDCrossCheckIsValid = false;

      for (const auto& entry3 : cfg->fightertype())
      {
        const auto& i3 = entry3.second;

        if (i3.authoredid () == i.fightertype())
        {        
                GUIDCrossCheckIsValid = true;
                break;
        }
      }
      
      if(GUIDCrossCheckIsValid == false)
      {
        LOG (WARNING) << "GUID cross-check failed for recepies: " << entry.first;
        return false;            
      }    

      for (const auto& entry5 : i.moves())
      {
        if(entry5 == "00000000-0000-0000-0000-000000000000")
        {
            LOG (WARNING) << "moves GUID is null/zero for for recepies: " << entry.first;
            return false;          
        }

        GUIDCrossCheckIsValid = false;

        for (const auto& entry3 : cfg->fightermoveblueprints())
        {
          const auto& i3 = entry3.second;

          if (i3.authoredid () == entry5)
          {        
                  GUIDCrossCheckIsValid = true;
                  break;
          }
        }
        
        if(GUIDCrossCheckIsValid == false)
        {
          LOG (WARNING) << "GUID cross-check failed for recepies: " << entry.first;
          return false;            
        }      
      }   
  }

  return true;
}

TEST_F (RoConfigSanityTests, Valid)
{
  EXPECT_TRUE (IsConfigValid (RoConfig (xaya::Chain::MAIN)));
  EXPECT_TRUE (IsConfigValid (RoConfig (xaya::Chain::TEST)));
  EXPECT_TRUE (IsConfigValid (RoConfig (xaya::Chain::REGTEST)));
}

} // anonymous namespace
} // namespace pxd

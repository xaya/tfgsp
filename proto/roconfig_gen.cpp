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

#include "config.h"

#include "config.pb.h"

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <cstdlib>
#include <fstream>
#include <iostream>

namespace
{

using google::protobuf::TextFormat;

constexpr const char* ROCONFIG_PROTO_TEXT = R"(
params:
{
    # The address configured here (for mainnet) is the premine address of Xaya
    # controlled by the Xaya team.  See also Xaya Core's src/chainparams.cpp.
    dev_addr: "CGUpAcjsb6MDktSYg8yRDxDutr7FhWtdWC"

    god_mode: false

    burnsale_stages: { amount_sold: 10000000000 price_sat: 10000 }
    burnsale_stages: { amount_sold: 10000000000 price_sat: 20000 }
    burnsale_stages: { amount_sold: 10000000000 price_sat: 50000 }
    burnsale_stages: { amount_sold: 20000000000 price_sat: 100000 }
    
    starting_recipes: "5864a19b-c8c0-2d34-eaef-9455af0baf2c"
  
    elok_factor: 50
  
    alms: 0.15
  
    starting_crystals: 0
  
    required_candy_per_vove: 20
  
    exchange_listing_fee: 10
  
    exchange_sale_percentage: 0.04
  
    common_recipe_cook_cost: 15
  
    uncommon_recipe_cook_cost: 25
  
    rare_recipe_cook_cost: 30
  
    epic_recipe_cook_cost: 45
  
    common_cook_duration: 10
  
    uncommon_cook_duration: 20
  
    rare_cook_duration: 50
  
    epic_cook_duration: 100
  
    common_move_count: 3
  
    uncommon_move_count: 4
  
    rare_move_count: 6
  
    epic_move_count: 8
  
    max_recipe_inventory_amount: 48
  
    max_fighter_inventory_amount: 48
  
    deconstruction_blocks: 10
  
    deconstruction_return_percent: 0.5  
    
    base_prestige: 0
  
    prestige_epic_mod: 1300
  
    prestige_rare_mod: 1200
  
    prestige_uncommon_mod: 1100
  
    prestige_common_mod: 1000
  
    prestige_total_treats_mod: 100
  
    prestige_avg_rating_mod: 1
  
    prestige_tournament_performance_mod: 0.5   
}recepies:
{
  key: "Recipe_Rare_CommanderLourdesofLimon"
  value:
  {
    AuthoredId: "1082e384-a607-f834-9ab3-d089c215d31a"
    AnimationId: "f33c74e6-f1b4-6ca4-bb5a-cf6edc77cab5"
    Armor: {candyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" armorType: 4}
    Armor: {candyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" armorType: 5}
    Armor: {candyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" armorType: 6}
    Armor: {candyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" armorType: 7}
    Name: "Commander Lourdes of Limon"
    Duration: 50
    FighterName: "Lady Lourdes of Limon"
    FighterType: "2522657f-c73f-23c4-da7d-3934d135265b"
    Moves: "55e05ece-a3f6-a924-3833-2222a0eee6a3"
    Moves: "55e05ece-a3f6-a924-3833-2222a0eee6a3"
    Moves: "55e05ece-a3f6-a924-3833-2222a0eee6a3"
    Moves: "55e05ece-a3f6-a924-3833-2222a0eee6a3"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Moves: "b1bd32de-88b2-1204-7b2e-0680677d78c4"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "9c64b7e4-0458-65c4-9b79-5cc35e847f9c" amount: 50}
    RequiredCandy: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" amount: 30}
    RequiredCandy: {candyType: "9b70e722-0442-2924-092c-93efc1d92975" amount: 50}
    RequiredCandy: {candyType: "7ea89b0d-7070-d3e4-a86f-c0e75c0e4ff5" amount: 30}
    RequiredCandy: {candyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Rare_CommanderBettyTheBlueberry"
  value:
  {
    AuthoredId: "7b7d8898-7f58-0334-0bad-825dc87a6638"
    AnimationId: "938e1ab9-a85b-5444-7bd6-f8862ace8b16"
    Armor: {candyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" armorType: 11}
    Armor: {candyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" armorType: 12}
    Armor: {candyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" armorType: 13}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 8}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 9}
    Name: "Commander Betty The Blueberry"
    Duration: 50
    FighterName: "Lady Betty The Blueberry"
    FighterType: "85f361b8-e55d-0244-1a98-26bffa0a18a2"
    Moves: "55e05ece-a3f6-a924-3833-2222a0eee6a3"
    Moves: "55e05ece-a3f6-a924-3833-2222a0eee6a3"
    Moves: "a9d69fc0-a1b8-ae24-cbd7-249760200211"
    Moves: "2c555752-8a84-58f4-395e-6460b7864069"
    Moves: "b1bd32de-88b2-1204-7b2e-0680677d78c4"
    Moves: "aa7d8565-99e1-84f4-49d2-8925c2602a7b"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" amount: 50}
    RequiredCandy: {candyType: "af509c01-933e-ce14-49c3-106081e16d75" amount: 30}
    RequiredCandy: {candyType: "9b70e722-0442-2924-092c-93efc1d92975" amount: 50}
    RequiredCandy: {candyType: "7ea89b0d-7070-d3e4-a86f-c0e75c0e4ff5" amount: 30}
    RequiredCandy: {candyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_InvestorReward_Silver"
  value:
  {
    AuthoredId: "546b51df-fb58-8c84-ebd3-43efb2d13334"
    AnimationId: "3afdf3ed-bea4-1c14-faac-159412578c0f"
    Armor: {candyType: "321c2a65-64b1-8344-2915-85562e2ad7ec" armorType: 1}
    Armor: {candyType: "321c2a65-64b1-8344-2915-85562e2ad7ec" armorType: 2}
    Armor: {candyType: "321c2a65-64b1-8344-2915-85562e2ad7ec" armorType: 3}
    Armor: {candyType: "bbeb0e31-3775-a864-1952-082233cf7b89" armorType: 4}
    Armor: {candyType: "bbeb0e31-3775-a864-1952-082233cf7b89" armorType: 5}
    Armor: {candyType: "bbeb0e31-3775-a864-1952-082233cf7b89" armorType: 6}
    Armor: {candyType: "bbeb0e31-3775-a864-1952-082233cf7b89" armorType: 7}
    Armor: {candyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" armorType: 8}
    Armor: {candyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" armorType: 9}
    Armor: {candyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" armorType: 10}
    Armor: {candyType: "eeeca33e-5c80-9ec4-eb83-46e4b7daf04b" armorType: 11}
    Armor: {candyType: "eeeca33e-5c80-9ec4-eb83-46e4b7daf04b" armorType: 12}
    Armor: {candyType: "eeeca33e-5c80-9ec4-eb83-46e4b7daf04b" armorType: 13}
    Armor: {candyType: "08847311-c629-a154-5958-13e8f1d77f88" armorType: 14}
    Armor: {candyType: "08847311-c629-a154-5958-13e8f1d77f88" armorType: 16}
    Armor: {candyType: "08847311-c629-a154-5958-13e8f1d77f88" armorType: 15}
    Armor: {candyType: "08847311-c629-a154-5958-13e8f1d77f88" armorType: 17}
    Armor: {candyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" armorType: 18}
    Name: "Recipe_InvestorReward_Silver"
    Duration: 20
    FighterName: "Daniel Gum the Silver"
    FighterType: "a672e1da-a87d-2a54-4918-cafb72b02aec"
    Moves: "4dbb297f-6a80-ca44-a94f-3ea9ed3311e8"
    Moves: "e55a3d69-9fb4-5194-4812-c2f85e887aca"
    Moves: "1a67cc79-b97a-4624-ca67-36c9fcbee970"
    Moves: "bbf71f26-78f5-5014-5b4b-a1c3866cb75b"
    Moves: "8e4753b5-c739-aaf4-ab79-4d333a69cae6"
    Moves: "55cafe03-4a58-0f44-6a27-f9fc6fce881a"
    Moves: "3e76f2fa-072c-5654-dac9-c9b0e4e1643a"
    Moves: "b96f1ff8-66d5-f244-79fd-260735d4c3d7"
    Moves: "61d8b3ec-9a01-0e64-fb45-27d2b858e332"
    Quality: 4
    IsAccountBound: true
    RequiredFighterQuality: 0
  }
}
recepies:
{
  key: "Recipe_Epic_BiteTyson"
  value:
  {
    AuthoredId: "c481aeee-d1a1-01c4-7aca-92d0edcddf18"
    AnimationId: "a7629ff3-c1ef-e664-3a9c-bd27cb332abd"
    Armor: {candyType: "321c2a65-64b1-8344-2915-85562e2ad7ec" armorType: 1}
    Armor: {candyType: "321c2a65-64b1-8344-2915-85562e2ad7ec" armorType: 2}
    Armor: {candyType: "321c2a65-64b1-8344-2915-85562e2ad7ec" armorType: 3}
    Armor: {candyType: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c" armorType: 4}
    Armor: {candyType: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c" armorType: 5}
    Armor: {candyType: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c" armorType: 6}
    Armor: {candyType: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c" armorType: 7}
    Armor: {candyType: "9b70e722-0442-2924-092c-93efc1d92975" armorType: 8}
    Armor: {candyType: "9b70e722-0442-2924-092c-93efc1d92975" armorType: 9}
    Armor: {candyType: "9b70e722-0442-2924-092c-93efc1d92975" armorType: 10}
    Armor: {candyType: "9b70e722-0442-2924-092c-93efc1d92975" armorType: 18}
    Armor: {candyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" armorType: 11}
    Armor: {candyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" armorType: 12}
    Armor: {candyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" armorType: 13}
    Armor: {candyType: "8aac4645-29cf-fe04-59c0-415508b0432e" armorType: 14}
    Armor: {candyType: "8aac4645-29cf-fe04-59c0-415508b0432e" armorType: 15}
    Armor: {candyType: "8aac4645-29cf-fe04-59c0-415508b0432e" armorType: 16}
    Armor: {candyType: "8aac4645-29cf-fe04-59c0-415508b0432e" armorType: 17}
    Name: "Bite Tyson"
    Duration: 100
    FighterName: "Bite Tyson"
    FighterType: "2522657f-c73f-23c4-da7d-3934d135265b"
    Moves: "92f7b745-c1ff-3cd4-f9cf-0fe0dafb5536"
    Moves: "92f7b745-c1ff-3cd4-f9cf-0fe0dafb5536"
    Moves: "e16531a7-33fd-a3e4-69fd-ff10eba65cb7"
    Moves: "aa7d8565-99e1-84f4-49d2-8925c2602a7b"
    Moves: "fa6144f9-ad49-8124-386f-351a7f1ab546"
    Moves: "fa6144f9-ad49-8124-386f-351a7f1ab546"
    Moves: "c28e13b4-cb1f-6ea4-680f-91010a400e10"
    Moves: "86b323c2-b2fd-2494-ab5e-bc3514bc92d8"
    Moves: "01b34a91-37d8-29d4-09cb-2d515f51e315"
    Quality: 4
    IsAccountBound: false
    RequiredCandy: {candyType: "321c2a65-64b1-8344-2915-85562e2ad7ec" amount: 50}
    RequiredCandy: {candyType: "08847311-c629-a154-5958-13e8f1d77f88" amount: 30}
    RequiredCandy: {candyType: "52ea4044-e203-16d4-c90f-8b685ed5e9c9" amount: 25}
    RequiredCandy: {candyType: "9ff884ab-efcb-0af4-5acd-976fea43b3a3" amount: 30}
    RequiredCandy: {candyType: "934dae05-689f-30e4-5804-d497aee0b47c" amount: 75}
    RequiredCandy: {candyType: "16e0b307-2b40-4584-294c-dee505e47827" amount: 50}
    RequiredCandy: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" amount: 25}
    RequiredCandy: {candyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" amount: 50}
    RequiredFighterQuality: 2
  }
}
recepies:
{
  key: "Recipe_Rare_CommanderBlancheofFluff"
  value:
  {
    AuthoredId: "4aba4145-0370-3484-1b2b-d416df6138ee"
    AnimationId: "05633498-ace9-de14-c939-9435a6343d0f"
    Armor: {candyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" armorType: 10}
    Armor: {candyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" armorType: 18}
    Armor: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" armorType: 14}
    Armor: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" armorType: 15}
    Name: "Commander Blanche of Fluff"
    Duration: 50
    FighterName: "Lady Blanche of Fluff"
    FighterType: "a672e1da-a87d-2a54-4918-cafb72b02aec"
    Moves: "55e05ece-a3f6-a924-3833-2222a0eee6a3"
    Moves: "2c555752-8a84-58f4-395e-6460b7864069"
    Moves: "a9d69fc0-a1b8-ae24-cbd7-249760200211"
    Moves: "a9d69fc0-a1b8-ae24-cbd7-249760200211"
    Moves: "9f0df0ed-13a6-84c4-781d-9dbf9741c66e"
    Moves: "9fd0b9ef-2e7d-d344-8926-69f1fab99f65"
    Moves: "1e47a8e0-acfd-1244-0b7d-539d62429060"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" amount: 50}
    RequiredCandy: {candyType: "af509c01-933e-ce14-49c3-106081e16d75" amount: 30}
    RequiredCandy: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" amount: 50}
    RequiredCandy: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" amount: 30}
    RequiredCandy: {candyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Rare_CaptainSammyTheChewy"
  value:
  {
    AuthoredId: "8c4b6099-b736-16a4-eb8b-25eaaac1a0c3"
    AnimationId: "938e1ab9-a85b-5444-7bd6-f8862ace8b16"
    Armor: {candyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" armorType: 4}
    Armor: {candyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" armorType: 5}
    Armor: {candyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" armorType: 6}
    Armor: {candyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" armorType: 7}
    Name: "Captain Sammy The Chewy"
    Duration: 50
    FighterName: "Lord Sammy The Chewy"
    FighterType: "a672e1da-a87d-2a54-4918-cafb72b02aec"
    Moves: "e16531a7-33fd-a3e4-69fd-ff10eba65cb7"
    Moves: "e16531a7-33fd-a3e4-69fd-ff10eba65cb7"
    Moves: "aa7d8565-99e1-84f4-49d2-8925c2602a7b"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Moves: "431900a2-54d4-df34-d9f7-6a4608fecfaa"
    Moves: "fa6144f9-ad49-8124-386f-351a7f1ab546"
    Moves: "c28e13b4-cb1f-6ea4-680f-91010a400e10"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" amount: 50}
    RequiredCandy: {candyType: "af509c01-933e-ce14-49c3-106081e16d75" amount: 30}
    RequiredCandy: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" amount: 50}
    RequiredCandy: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" amount: 30}
    RequiredCandy: {candyType: "934dae05-689f-30e4-5804-d497aee0b47c" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Epic_HoneyBunny"
  value:
  {
    AuthoredId: "f0d9f776-2d8e-1e04-da62-fb62623b7d2e"
    AnimationId: "02ddcd6e-e441-73f4-e874-399531dc4708"
    Armor: {candyType: "934dae05-689f-30e4-5804-d497aee0b47c" armorType: 1}
    Armor: {candyType: "934dae05-689f-30e4-5804-d497aee0b47c" armorType: 2}
    Armor: {candyType: "934dae05-689f-30e4-5804-d497aee0b47c" armorType: 3}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 4}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 5}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 6}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 7}
    Armor: {candyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" armorType: 8}
    Armor: {candyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" armorType: 9}
    Armor: {candyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" armorType: 10}
    Armor: {candyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" armorType: 18}
    Armor: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" armorType: 11}
    Armor: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" armorType: 12}
    Armor: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" armorType: 13}
    Armor: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" armorType: 14}
    Armor: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" armorType: 15}
    Armor: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" armorType: 16}
    Armor: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" armorType: 17}
    Name: "Honey Bunny"
    Duration: 100
    FighterName: "Honey Bunny"
    FighterType: "a672e1da-a87d-2a54-4918-cafb72b02aec"
    Moves: "bbf71f26-78f5-5014-5b4b-a1c3866cb75b"
    Moves: "bbf71f26-78f5-5014-5b4b-a1c3866cb75b"
    Moves: "b96f1ff8-66d5-f244-79fd-260735d4c3d7"
    Moves: "29d559bb-41a7-9f14-5bf8-605da8166943"
    Moves: "34a115db-3153-aff4-9a46-97700634f1fb"
    Moves: "1d1dec74-ca35-0cc4-d930-164786a9dd81"
    Moves: "1e47a8e0-acfd-1244-0b7d-539d62429060"
    Moves: "a9d69fc0-a1b8-ae24-cbd7-249760200211"
    Moves: "55e05ece-a3f6-a924-3833-2222a0eee6a3"
    Quality: 4
    IsAccountBound: false
    RequiredCandy: {candyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" amount: 50}
    RequiredCandy: {candyType: "1f089286-ff92-dcc4-b85c-7ebf28d2cc4f" amount: 30}
    RequiredCandy: {candyType: "8aa0e6a7-0b45-3404-9b39-dc4a4300a87f" amount: 25}
    RequiredCandy: {candyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" amount: 30}
    RequiredCandy: {candyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" amount: 75}
    RequiredCandy: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" amount: 50}
    RequiredCandy: {candyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" amount: 25}
    RequiredCandy: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" amount: 50}
    RequiredFighterQuality: 2
  }
}
recepies:
{
  key: "Recipe_Epic_FloydFruitleather"
  value:
  {
    AuthoredId: "b192f393-c1ac-3364-a8a0-dd5582933e66"
    AnimationId: "a170f9f3-f97b-8eb4-49c0-c99a916e55d7"
    Armor: {candyType: "f6c3b895-6488-1b74-a9b0-664027a683e2" armorType: 1}
    Armor: {candyType: "f6c3b895-6488-1b74-a9b0-664027a683e2" armorType: 2}
    Armor: {candyType: "f6c3b895-6488-1b74-a9b0-664027a683e2" armorType: 3}
    Armor: {candyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" armorType: 4}
    Armor: {candyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" armorType: 5}
    Armor: {candyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" armorType: 6}
    Armor: {candyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" armorType: 7}
    Armor: {candyType: "8898194e-ac52-f624-a830-176655bb43cd" armorType: 8}
    Armor: {candyType: "8898194e-ac52-f624-a830-176655bb43cd" armorType: 9}
    Armor: {candyType: "8898194e-ac52-f624-a830-176655bb43cd" armorType: 10}
    Armor: {candyType: "8898194e-ac52-f624-a830-176655bb43cd" armorType: 18}
    Armor: {candyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" armorType: 11}
    Armor: {candyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" armorType: 12}
    Armor: {candyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" armorType: 13}
    Armor: {candyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" armorType: 14}
    Armor: {candyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" armorType: 15}
    Armor: {candyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" armorType: 16}
    Armor: {candyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" armorType: 17}
    Name: "Floyd Fruitleather"
    Duration: 100
    FighterName: "Floyd Fruitleather"
    FighterType: "2522657f-c73f-23c4-da7d-3934d135265b"
    Moves: "34a115db-3153-aff4-9a46-97700634f1fb"
    Moves: "34a115db-3153-aff4-9a46-97700634f1fb"
    Moves: "b96f1ff8-66d5-f244-79fd-260735d4c3d7"
    Moves: "61d8b3ec-9a01-0e64-fb45-27d2b858e332"
    Moves: "29d559bb-41a7-9f14-5bf8-605da8166943"
    Moves: "b1bd32de-88b2-1204-7b2e-0680677d78c4"
    Moves: "a9d69fc0-a1b8-ae24-cbd7-249760200211"
    Moves: "0090580c-04ef-9d84-e883-32f52c977b98"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Quality: 4
    IsAccountBound: false
    RequiredCandy: {candyType: "8898194e-ac52-f624-a830-176655bb43cd" amount: 50}
    RequiredCandy: {candyType: "bbeb0e31-3775-a864-1952-082233cf7b89" amount: 30}
    RequiredCandy: {candyType: "48ea4c49-79dd-6514-6b8b-f87fd6d8a330" amount: 25}
    RequiredCandy: {candyType: "9c64b7e4-0458-65c4-9b79-5cc35e847f9c" amount: 30}
    RequiredCandy: {candyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" amount: 75}
    RequiredCandy: {candyType: "9b70e722-0442-2924-092c-93efc1d92975" amount: 50}
    RequiredCandy: {candyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" amount: 25}
    RequiredCandy: {candyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" amount: 50}
    RequiredFighterQuality: 2
  }
}
recepies:
{
  key: "Recipe_Rare_CaptainScottLemonpie"
  value:
  {
    AuthoredId: "fda69e34-c1cb-2664-4ba1-d943713218c5"
    AnimationId: "27448f34-ca08-99b4-1b63-fd4b227e6af4"
    Armor: {candyType: "16e0b307-2b40-4584-294c-dee505e47827" armorType: 1}
    Armor: {candyType: "16e0b307-2b40-4584-294c-dee505e47827" armorType: 2}
    Armor: {candyType: "16e0b307-2b40-4584-294c-dee505e47827" armorType: 3}
    Name: "Captain Scott Lemonpie"
    Duration: 50
    FighterName: "Lord Scott Lemonpie"
    FighterType: "4c2d03be-70bf-9294-c8d5-0a8e2d8d0677"
    Moves: "aa7d8565-99e1-84f4-49d2-8925c2602a7b"
    Moves: "9f0df0ed-13a6-84c4-781d-9dbf9741c66e"
    Moves: "9f0df0ed-13a6-84c4-781d-9dbf9741c66e"
    Moves: "55e05ece-a3f6-a924-3833-2222a0eee6a3"
    Moves: "a9d69fc0-a1b8-ae24-cbd7-249760200211"
    Moves: "86b323c2-b2fd-2494-ab5e-bc3514bc92d8"
    Moves: "9fd0b9ef-2e7d-d344-8926-69f1fab99f65"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" amount: 50}
    RequiredCandy: {candyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" amount: 30}
    RequiredCandy: {candyType: "9b70e722-0442-2924-092c-93efc1d92975" amount: 50}
    RequiredCandy: {candyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" amount: 30}
    RequiredCandy: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" amount: 35}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Epic_ClariceGummi"
  value:
  {
    AuthoredId: "30a597d1-31e7-3474-4b15-d7ff87f384f8"
    AnimationId: "c6e3c59b-4415-52b4-8b34-1593bbc26634"
    Armor: {candyType: "f6c3b895-6488-1b74-a9b0-664027a683e2" armorType: 1}
    Armor: {candyType: "f6c3b895-6488-1b74-a9b0-664027a683e2" armorType: 2}
    Armor: {candyType: "f6c3b895-6488-1b74-a9b0-664027a683e2" armorType: 3}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 4}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 5}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 6}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 7}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 8}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 9}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 10}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 18}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 11}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 12}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 13}
    Armor: {candyType: "47b8a853-791a-1424-1ba9-4adaf4a3f4a5" armorType: 14}
    Armor: {candyType: "47b8a853-791a-1424-1ba9-4adaf4a3f4a5" armorType: 15}
    Armor: {candyType: "47b8a853-791a-1424-1ba9-4adaf4a3f4a5" armorType: 16}
    Armor: {candyType: "47b8a853-791a-1424-1ba9-4adaf4a3f4a5" armorType: 17}
    Name: "Clarice Gummi"
    Duration: 100
    FighterName: "Clarice Gummi"
    FighterType: "c160f7ad-c775-8614-abe2-8ef74e54401f"
    Moves: "c6770789-4a1f-7f24-f921-c4dc66c2697a"
    Moves: "c6770789-4a1f-7f24-f921-c4dc66c2697a"
    Moves: "55e05ece-a3f6-a924-3833-2222a0eee6a3"
    Moves: "2c555752-8a84-58f4-395e-6460b7864069"
    Moves: "29d559bb-41a7-9f14-5bf8-605da8166943"
    Moves: "61d8b3ec-9a01-0e64-fb45-27d2b858e332"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Moves: "a9d69fc0-a1b8-ae24-cbd7-249760200211"
    Moves: "e201f964-8949-9d74-fbee-4ee8786284ca"
    Quality: 4
    IsAccountBound: false
    RequiredCandy: {candyType: "47b8a853-791a-1424-1ba9-4adaf4a3f4a5" amount: 50}
    RequiredCandy: {candyType: "8f896243-a6de-1b54-68a3-0df72e8688e4" amount: 30}
    RequiredCandy: {candyType: "a2c2697c-d738-c004-4b32-d800921bab06" amount: 25}
    RequiredCandy: {candyType: "af509c01-933e-ce14-49c3-106081e16d75" amount: 30}
    RequiredCandy: {candyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" amount: 75}
    RequiredCandy: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" amount: 50}
    RequiredCandy: {candyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" amount: 25}
    RequiredCandy: {candyType: "16e0b307-2b40-4584-294c-dee505e47827" amount: 50}
    RequiredFighterQuality: 2
  }
}
recepies:
{
  key: "Recipe_Rare_CommanderTabithaThePowdered"
  value:
  {
    AuthoredId: "ef33f366-41a4-5924-19bf-af4ea20ca7df"
    AnimationId: "d35a9d1c-2223-c404-c840-c0218f9b147d"
    Armor: {candyType: "16e0b307-2b40-4584-294c-dee505e47827" armorType: 2}
    Armor: {candyType: "16e0b307-2b40-4584-294c-dee505e47827" armorType: 3}
    Armor: {candyType: "16e0b307-2b40-4584-294c-dee505e47827" armorType: 13}
    Armor: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" armorType: 12}
    Name: "Commander Tabitha The Powdered"
    Duration: 50
    FighterName: "Lady Tabitha The Powdered"
    FighterType: "4c2d03be-70bf-9294-c8d5-0a8e2d8d0677"
    Moves: "ad6d5d28-08b4-4304-6b49-11f8f5f9ff1c"
    Moves: "ad6d5d28-08b4-4304-6b49-11f8f5f9ff1c"
    Moves: "ad6d5d28-08b4-4304-6b49-11f8f5f9ff1c"
    Moves: "628ebc0b-bb19-ee64-68b8-2f66f4b92720"
    Moves: "fa6144f9-ad49-8124-386f-351a7f1ab546"
    Moves: "fa6144f9-ad49-8124-386f-351a7f1ab546"
    Moves: "c28e13b4-cb1f-6ea4-680f-91010a400e10"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "9ff884ab-efcb-0af4-5acd-976fea43b3a3" amount: 50}
    RequiredCandy: {candyType: "86b5fac0-4ec1-09e4-6ab7-96d3fd13dd04" amount: 30}
    RequiredCandy: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" amount: 50}
    RequiredCandy: {candyType: "7ea89b0d-7070-d3e4-a86f-c0e75c0e4ff5" amount: 30}
    RequiredCandy: {candyType: "16e0b307-2b40-4584-294c-dee505e47827" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Epic_KitKatCarter"
  value:
  {
    AuthoredId: "9d7d627c-8627-b934-6b2a-f645d96447ea"
    AnimationId: "f33c74e6-f1b4-6ca4-bb5a-cf6edc77cab5"
    Armor: {candyType: "321c2a65-64b1-8344-2915-85562e2ad7ec" armorType: 1}
    Armor: {candyType: "321c2a65-64b1-8344-2915-85562e2ad7ec" armorType: 2}
    Armor: {candyType: "321c2a65-64b1-8344-2915-85562e2ad7ec" armorType: 3}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 4}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 5}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 6}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 7}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 8}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 9}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 10}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 18}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 11}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 12}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 13}
    Armor: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" armorType: 14}
    Armor: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" armorType: 15}
    Armor: {candyType: "08847311-c629-a154-5958-13e8f1d77f88" armorType: 16}
    Armor: {candyType: "08847311-c629-a154-5958-13e8f1d77f88" armorType: 17}
    Name: "Kit Kat Carter"
    Duration: 100
    FighterName: "Kit Kat Carter"
    FighterType: "a672e1da-a87d-2a54-4918-cafb72b02aec"
    Moves: "e55a3d69-9fb4-5194-4812-c2f85e887aca"
    Moves: "e55a3d69-9fb4-5194-4812-c2f85e887aca"
    Moves: "aa7d8565-99e1-84f4-49d2-8925c2602a7b"
    Moves: "9f0df0ed-13a6-84c4-781d-9dbf9741c66e"
    Moves: "9fd0b9ef-2e7d-d344-8926-69f1fab99f65"
    Moves: "fa6144f9-ad49-8124-386f-351a7f1ab546"
    Moves: "c28e13b4-cb1f-6ea4-680f-91010a400e10"
    Moves: "431900a2-54d4-df34-d9f7-6a4608fecfaa"
    Moves: "01b34a91-37d8-29d4-09cb-2d515f51e315"
    Quality: 4
    IsAccountBound: false
    RequiredCandy: {candyType: "321c2a65-64b1-8344-2915-85562e2ad7ec" amount: 50}
    RequiredCandy: {candyType: "8898cfa3-1512-22e4-0b9f-c6e220f315b5" amount: 30}
    RequiredCandy: {candyType: "8932caea-da2e-6384-58ec-749624e213c9" amount: 25}
    RequiredCandy: {candyType: "28089a53-62ac-2044-1964-29f2f31d8a33" amount: 30}
    RequiredCandy: {candyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" amount: 75}
    RequiredCandy: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" amount: 50}
    RequiredCandy: {candyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" amount: 25}
    RequiredCandy: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" amount: 50}
    RequiredFighterQuality: 2
  }
}
recepies:
{
  key: "Recipe_Epic_RoloRousey"
  value:
  {
    AuthoredId: "b4efa30e-0d76-5a04-b8fb-0a5c7b02d4a3"
    AnimationId: "938e1ab9-a85b-5444-7bd6-f8862ace8b16"
    Armor: {candyType: "aa0cb18d-5681-5504-09d7-4db06bf7a4d8" armorType: 1}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 2}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 3}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 4}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 5}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 6}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 7}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 8}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 9}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 10}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 18}
    Armor: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" armorType: 11}
    Armor: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" armorType: 12}
    Armor: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" armorType: 13}
    Armor: {candyType: "a2c2697c-d738-c004-4b32-d800921bab06" armorType: 14}
    Armor: {candyType: "a2c2697c-d738-c004-4b32-d800921bab06" armorType: 15}
    Armor: {candyType: "a2c2697c-d738-c004-4b32-d800921bab06" armorType: 16}
    Armor: {candyType: "a2c2697c-d738-c004-4b32-d800921bab06" armorType: 17}
    Name: "Rolo Rousey"
    Duration: 100
    FighterName: "Rolo Rousey"
    FighterType: "c160f7ad-c775-8614-abe2-8ef74e54401f"
    Moves: "8e4753b5-c739-aaf4-ab79-4d333a69cae6"
    Moves: "8e4753b5-c739-aaf4-ab79-4d333a69cae6"
    Moves: "55cafe03-4a58-0f44-6a27-f9fc6fce881a"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Moves: "431900a2-54d4-df34-d9f7-6a4608fecfaa"
    Moves: "bd27fe1c-ce9a-f524-c887-61f9e4d5088b"
    Moves: "c28e13b4-cb1f-6ea4-680f-91010a400e10"
    Moves: "86bd4f81-e96b-9864-78b5-5fc51fbff6a4"
    Moves: "e55a3d69-9fb4-5194-4812-c2f85e887aca"
    Quality: 4
    IsAccountBound: false
    RequiredCandy: {candyType: "a2c2697c-d738-c004-4b32-d800921bab06" amount: 50}
    RequiredCandy: {candyType: "48ea4c49-79dd-6514-6b8b-f87fd6d8a330" amount: 30}
    RequiredCandy: {candyType: "aa0cb18d-5681-5504-09d7-4db06bf7a4d8" amount: 25}
    RequiredCandy: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" amount: 30}
    RequiredCandy: {candyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" amount: 75}
    RequiredCandy: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" amount: 50}
    RequiredCandy: {candyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" amount: 25}
    RequiredCandy: {candyType: "934dae05-689f-30e4-5804-d497aee0b47c" amount: 50}
    RequiredFighterQuality: 2
  }
}
recepies:
{
  key: "Recipe_Rare_CaptainJasonTheButtery"
  value:
  {
    AuthoredId: "502693b2-3dd5-a204-197d-39031869f194"
    AnimationId: "a170f9f3-f97b-8eb4-49c0-c99a916e55d7"
    Armor: {candyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" armorType: 1}
    Armor: {candyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" armorType: 2}
    Armor: {candyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" armorType: 3}
    Name: "Captain Jason The Buttery"
    Duration: 50
    FighterName: "Lord Jason The Buttery"
    FighterType: "4c2d03be-70bf-9294-c8d5-0a8e2d8d0677"
    Moves: "2569d967-c676-8b44-cbdf-d514e4c5c73d"
    Moves: "2569d967-c676-8b44-cbdf-d514e4c5c73d"
    Moves: "2569d967-c676-8b44-cbdf-d514e4c5c73d"
    Moves: "2569d967-c676-8b44-cbdf-d514e4c5c73d"
    Moves: "fa6144f9-ad49-8124-386f-351a7f1ab546"
    Moves: "fa6144f9-ad49-8124-386f-351a7f1ab546"
    Moves: "c28e13b4-cb1f-6ea4-680f-91010a400e10"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" amount: 45}
    RequiredCandy: {candyType: "86b5fac0-4ec1-09e4-6ab7-96d3fd13dd04" amount: 30}
    RequiredCandy: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" amount: 50}
    RequiredCandy: {candyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" amount: 30}
    RequiredCandy: {candyType: "524c8f4f-cb50-ea04-dafd-e192f30c7656" amount: 35}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Rare_CommanderAubreyTheBrulee-Brawler"
  value:
  {
    AuthoredId: "60f3f548-20ca-7a64-6ad3-054d654e24a2"
    AnimationId: "27448f34-ca08-99b4-1b63-fd4b227e6af4"
    Armor: {candyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" armorType: 8}
    Armor: {candyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" armorType: 9}
    Armor: {candyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" armorType: 14}
    Armor: {candyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" armorType: 15}
    Name: "Commander Aubrey The Brulee-Brawler"
    Duration: 50
    FighterName: "Lady Aubrey The Brulee-Brawler"
    FighterType: "2522657f-c73f-23c4-da7d-3934d135265b"
    Moves: "29d559bb-41a7-9f14-5bf8-605da8166943"
    Moves: "29d559bb-41a7-9f14-5bf8-605da8166943"
    Moves: "29d559bb-41a7-9f14-5bf8-605da8166943"
    Moves: "b889835e-1cd6-d3d4-f9b4-dd1989f49dac"
    Moves: "fa6144f9-ad49-8124-386f-351a7f1ab546"
    Moves: "1d1dec74-ca35-0cc4-d930-164786a9dd81"
    Moves: "bd27fe1c-ce9a-f524-c887-61f9e4d5088b"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" amount: 50}
    RequiredCandy: {candyType: "28089a53-62ac-2044-1964-29f2f31d8a33" amount: 30}
    RequiredCandy: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" amount: 50}
    RequiredCandy: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" amount: 30}
    RequiredCandy: {candyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" amount: 44}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Rare_CaptainChrisGummiBear"
  value:
  {
    AuthoredId: "db1606dd-112d-7a34-db0b-8923900c8ac7"
    AnimationId: "a91a62fa-8d65-6234-d91b-a0ad0d9388e3"
    Armor: {candyType: "af509c01-933e-ce14-49c3-106081e16d75" armorType: 18}
    Armor: {candyType: "af509c01-933e-ce14-49c3-106081e16d75" armorType: 10}
    Armor: {candyType: "af509c01-933e-ce14-49c3-106081e16d75" armorType: 16}
    Armor: {candyType: "af509c01-933e-ce14-49c3-106081e16d75" armorType: 17}
    Name: "Captain Chris Gummi Bear"
    Duration: 50
    FighterName: "Lord Chris Gummi Bear"
    FighterType: "c160f7ad-c775-8614-abe2-8ef74e54401f"
    Moves: "e16531a7-33fd-a3e4-69fd-ff10eba65cb7"
    Moves: "e16531a7-33fd-a3e4-69fd-ff10eba65cb7"
    Moves: "e16531a7-33fd-a3e4-69fd-ff10eba65cb7"
    Moves: "9f0df0ed-13a6-84c4-781d-9dbf9741c66e"
    Moves: "29d559bb-41a7-9f14-5bf8-605da8166943"
    Moves: "2c555752-8a84-58f4-395e-6460b7864069"
    Moves: "431900a2-54d4-df34-d9f7-6a4608fecfaa"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "af509c01-933e-ce14-49c3-106081e16d75" amount: 50}
    RequiredCandy: {candyType: "9c64b7e4-0458-65c4-9b79-5cc35e847f9c" amount: 30}
    RequiredCandy: {candyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" amount: 50}
    RequiredCandy: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" amount: 30}
    RequiredCandy: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Epic_JellyHolm"
  value:
  {
    AuthoredId: "390c2aa8-6b27-9a04-c820-835a0d178686"
    AnimationId: "5fce85e9-51f9-d7b4-db81-ef141439db00"
    Armor: {candyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" armorType: 1}
    Armor: {candyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" armorType: 2}
    Armor: {candyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" armorType: 3}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 4}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 5}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 6}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 7}
    Armor: {candyType: "48ea4c49-79dd-6514-6b8b-f87fd6d8a330" armorType: 8}
    Armor: {candyType: "48ea4c49-79dd-6514-6b8b-f87fd6d8a330" armorType: 9}
    Armor: {candyType: "48ea4c49-79dd-6514-6b8b-f87fd6d8a330" armorType: 10}
    Armor: {candyType: "48ea4c49-79dd-6514-6b8b-f87fd6d8a330" armorType: 18}
    Armor: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" armorType: 11}
    Armor: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" armorType: 12}
    Armor: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" armorType: 13}
    Armor: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" armorType: 14}
    Armor: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" armorType: 15}
    Armor: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" armorType: 16}
    Armor: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" armorType: 17}
    Name: "Jelly Holm"
    Duration: 100
    FighterName: "Jelly Holm"
    FighterType: "a672e1da-a87d-2a54-4918-cafb72b02aec"
    Moves: "b96f1ff8-66d5-f244-79fd-260735d4c3d7"
    Moves: "b96f1ff8-66d5-f244-79fd-260735d4c3d7"
    Moves: "61d8b3ec-9a01-0e64-fb45-27d2b858e332"
    Moves: "29d559bb-41a7-9f14-5bf8-605da8166943"
    Moves: "bbf71f26-78f5-5014-5b4b-a1c3866cb75b"
    Moves: "86bd4f81-e96b-9864-78b5-5fc51fbff6a4"
    Moves: "e201f964-8949-9d74-fbee-4ee8786284ca"
    Moves: "bd27fe1c-ce9a-f524-c887-61f9e4d5088b"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Quality: 4
    IsAccountBound: false
    RequiredCandy: {candyType: "48ea4c49-79dd-6514-6b8b-f87fd6d8a330" amount: 50}
    RequiredCandy: {candyType: "dd29bd35-bb51-8d94-4913-8ca943d062f1" amount: 30}
    RequiredCandy: {candyType: "92954ab6-fe54-4a64-d942-feca64e9e931" amount: 25}
    RequiredCandy: {candyType: "af509c01-933e-ce14-49c3-106081e16d75" amount: 30}
    RequiredCandy: {candyType: "9b70e722-0442-2924-092c-93efc1d92975" amount: 75}
    RequiredCandy: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" amount: 50}
    RequiredCandy: {candyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" amount: 25}
    RequiredCandy: {candyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" amount: 50}
    RequiredFighterQuality: 2
  }
}
recepies:
{
  key: "Recipe_Rare_CaptainTommyPandan"
  value:
  {
    AuthoredId: "275d1b18-22e6-fcf4-2919-b28bc1da9060"
    AnimationId: "c6e3c59b-4415-52b4-8b34-1593bbc26634"
    Armor: {candyType: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c" armorType: 4}
    Armor: {candyType: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c" armorType: 5}
    Armor: {candyType: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c" armorType: 6}
    Armor: {candyType: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c" armorType: 7}
    Name: "Captain Tommy Pandan"
    Duration: 50
    FighterName: "Lord Tommy Pandan"
    FighterType: "c160f7ad-c775-8614-abe2-8ef74e54401f"
    Moves: "2c94f949-a10e-2524-2827-6588982ce0b1"
    Moves: "2c94f949-a10e-2524-2827-6588982ce0b1"
    Moves: "0090580c-04ef-9d84-e883-32f52c977b98"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Moves: "86b323c2-b2fd-2494-ab5e-bc3514bc92d8"
    Moves: "e201f964-8949-9d74-fbee-4ee8786284ca"
    Moves: "c28e13b4-cb1f-6ea4-680f-91010a400e10"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c" amount: 50}
    RequiredCandy: {candyType: "9c64b7e4-0458-65c4-9b79-5cc35e847f9c" amount: 30}
    RequiredCandy: {candyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" amount: 50}
    RequiredCandy: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" amount: 30}
    RequiredCandy: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" amount: 38}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Rare_CaptainLeifTheLemony"
  value:
  {
    AuthoredId: "a889e1d9-c337-dff4-3aae-7701b6e48aeb"
    AnimationId: "97c2c3da-93c7-a0d4-1967-eeba13396cce"
    Armor: {candyType: "524c8f4f-cb50-ea04-dafd-e192f30c7656" armorType: 18}
    Armor: {candyType: "524c8f4f-cb50-ea04-dafd-e192f30c7656" armorType: 10}
    Armor: {candyType: "524c8f4f-cb50-ea04-dafd-e192f30c7656" armorType: 8}
    Armor: {candyType: "524c8f4f-cb50-ea04-dafd-e192f30c7656" armorType: 9}
    Name: "Captain Leif The Lemony"
    Duration: 50
    FighterName: "Lord Leif The Lemony"
    FighterType: "2522657f-c73f-23c4-da7d-3934d135265b"
    Moves: "2c94f949-a10e-2524-2827-6588982ce0b1"
    Moves: "2c94f949-a10e-2524-2827-6588982ce0b1"
    Moves: "431900a2-54d4-df34-d9f7-6a4608fecfaa"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Moves: "a9d69fc0-a1b8-ae24-cbd7-249760200211"
    Moves: "29d559bb-41a7-9f14-5bf8-605da8166943"
    Moves: "9fd0b9ef-2e7d-d344-8926-69f1fab99f65"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "af509c01-933e-ce14-49c3-106081e16d75" amount: 52}
    RequiredCandy: {candyType: "28089a53-62ac-2044-1964-29f2f31d8a33" amount: 30}
    RequiredCandy: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" amount: 45}
    RequiredCandy: {candyType: "7ea89b0d-7070-d3e4-a86f-c0e75c0e4ff5" amount: 30}
    RequiredCandy: {candyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Epic_LicoriceDuran"
  value:
  {
    AuthoredId: "c4c807ca-431a-9184-da8e-2c9993d1bba0"
    AnimationId: "b0877d2a-71db-7374-8a8b-8044f7a8a52a"
    Armor: {candyType: "16e0b307-2b40-4584-294c-dee505e47827" armorType: 1}
    Armor: {candyType: "16e0b307-2b40-4584-294c-dee505e47827" armorType: 2}
    Armor: {candyType: "16e0b307-2b40-4584-294c-dee505e47827" armorType: 3}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 4}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 5}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 6}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 7}
    Armor: {candyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" armorType: 8}
    Armor: {candyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" armorType: 9}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 10}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 18}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 11}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 12}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 13}
    Armor: {candyType: "08847311-c629-a154-5958-13e8f1d77f88" armorType: 14}
    Armor: {candyType: "08847311-c629-a154-5958-13e8f1d77f88" armorType: 15}
    Armor: {candyType: "08847311-c629-a154-5958-13e8f1d77f88" armorType: 16}
    Armor: {candyType: "08847311-c629-a154-5958-13e8f1d77f88" armorType: 17}
    Name: "Licorice Duran"
    Duration: 100
    FighterName: "Licorice Duran"
    FighterType: "a672e1da-a87d-2a54-4918-cafb72b02aec"
    Moves: "4dbb297f-6a80-ca44-a94f-3ea9ed3311e8"
    Moves: "4dbb297f-6a80-ca44-a94f-3ea9ed3311e8"
    Moves: "55cafe03-4a58-0f44-6a27-f9fc6fce881a"
    Moves: "8e4753b5-c739-aaf4-ab79-4d333a69cae6"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Moves: "9f0df0ed-13a6-84c4-781d-9dbf9741c66e"
    Moves: "86b323c2-b2fd-2494-ab5e-bc3514bc92d8"
    Moves: "c28e13b4-cb1f-6ea4-680f-91010a400e10"
    Moves: "fa6144f9-ad49-8124-386f-351a7f1ab546"
    Quality: 4
    IsAccountBound: false
    RequiredCandy: {candyType: "08847311-c629-a154-5958-13e8f1d77f88" amount: 50}
    RequiredCandy: {candyType: "8aa0e6a7-0b45-3404-9b39-dc4a4300a87f" amount: 30}
    RequiredCandy: {candyType: "eeeca33e-5c80-9ec4-eb83-46e4b7daf04b" amount: 25}
    RequiredCandy: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" amount: 30}
    RequiredCandy: {candyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" amount: 75}
    RequiredCandy: {candyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" amount: 50}
    RequiredCandy: {candyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" amount: 25}
    RequiredCandy: {candyType: "524c8f4f-cb50-ea04-dafd-e192f30c7656" amount: 50}
    RequiredFighterQuality: 2
  }
}
recepies:
{
  key: "Recipe_Rare_CaptainPaulCandydish"
  value:
  {
    AuthoredId: "7938f8c7-2d20-19c4-9be9-b05fd663f44f"
    AnimationId: "5fce85e9-51f9-d7b4-db81-ef141439db00"
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 10}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 18}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 8}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 9}
    Name: "Captain Paul Candydish"
    Duration: 50
    FighterName: "Lord Paul Candydish"
    FighterType: "2522657f-c73f-23c4-da7d-3934d135265b"
    Moves: "55e05ece-a3f6-a924-3833-2222a0eee6a3"
    Moves: "86bd4f81-e96b-9864-78b5-5fc51fbff6a4"
    Moves: "86bd4f81-e96b-9864-78b5-5fc51fbff6a4"
    Moves: "86bd4f81-e96b-9864-78b5-5fc51fbff6a4"
    Moves: "431900a2-54d4-df34-d9f7-6a4608fecfaa"
    Moves: "34a115db-3153-aff4-9a46-97700634f1fb"
    Moves: "c28e13b4-cb1f-6ea4-680f-91010a400e10"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" amount: 50}
    RequiredCandy: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" amount: 30}
    RequiredCandy: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" amount: 50}
    RequiredCandy: {candyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" amount: 30}
    RequiredCandy: {candyType: "16e0b307-2b40-4584-294c-dee505e47827" amount: 45}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Epic_SugarRayChocolaton"
  value:
  {
    AuthoredId: "3f5314d7-0755-ab64-8a9c-589d3158de7a"
    AnimationId: "05633498-ace9-de14-c939-9435a6343d0f"
    Armor: {candyType: "16e0b307-2b40-4584-294c-dee505e47827" armorType: 1}
    Armor: {candyType: "16e0b307-2b40-4584-294c-dee505e47827" armorType: 2}
    Armor: {candyType: "16e0b307-2b40-4584-294c-dee505e47827" armorType: 3}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 4}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 5}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 6}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 7}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 8}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 9}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 10}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 18}
    Armor: {candyType: "2f765969-c607-d4a4-0b9c-72d3cce7fbec" armorType: 11}
    Armor: {candyType: "2f765969-c607-d4a4-0b9c-72d3cce7fbec" armorType: 12}
    Armor: {candyType: "2f765969-c607-d4a4-0b9c-72d3cce7fbec" armorType: 13}
    Armor: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" armorType: 14}
    Armor: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" armorType: 15}
    Armor: {candyType: "47b8a853-791a-1424-1ba9-4adaf4a3f4a5" armorType: 16}
    Armor: {candyType: "47b8a853-791a-1424-1ba9-4adaf4a3f4a5" armorType: 17}
    Name: "Sugar Ray Chocolaton"
    Duration: 100
    FighterName: "Sugar Ray Chocolaton"
    FighterType: "85f361b8-e55d-0244-1a98-26bffa0a18a2"
    Moves: "1d1dec74-ca35-0cc4-d930-164786a9dd81"
    Moves: "1d1dec74-ca35-0cc4-d930-164786a9dd81"
    Moves: "1d1dec74-ca35-0cc4-d930-164786a9dd81"
    Moves: "3e76f2fa-072c-5654-dac9-c9b0e4e1643a"
    Moves: "ad6d5d28-08b4-4304-6b49-11f8f5f9ff1c"
    Moves: "e201f964-8949-9d74-fbee-4ee8786284ca"
    Moves: "86bd4f81-e96b-9864-78b5-5fc51fbff6a4"
    Moves: "431900a2-54d4-df34-d9f7-6a4608fecfaa"
    Moves: "9fd0b9ef-2e7d-d344-8926-69f1fab99f65"
    Quality: 4
    IsAccountBound: false
    RequiredCandy: {candyType: "2f765969-c607-d4a4-0b9c-72d3cce7fbec" amount: 50}
    RequiredCandy: {candyType: "499a8021-eaea-c7d4-aa1c-bb6e55cd300c" amount: 30}
    RequiredCandy: {candyType: "8aa0e6a7-0b45-3404-9b39-dc4a4300a87f" amount: 25}
    RequiredCandy: {candyType: "1525c955-d3c0-9f84-69fb-cf98837f4f85" amount: 30}
    RequiredCandy: {candyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" amount: 75}
    RequiredCandy: {candyType: "9b70e722-0442-2924-092c-93efc1d92975" amount: 50}
    RequiredCandy: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" amount: 25}
    RequiredCandy: {candyType: "934dae05-689f-30e4-5804-d497aee0b47c" amount: 50}
    RequiredFighterQuality: 2
  }
}
recepies:
{
  key: "Recipe_Rare_CaptainChristianTheFrosted"
  value:
  {
    AuthoredId: "b68f5a1f-8bfd-36d4-6ac4-f9ccc5f3acf6"
    AnimationId: "a2a185e5-4543-6814-d88d-d078a0e38521"
    Armor: {candyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" armorType: 10}
    Armor: {candyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" armorType: 18}
    Armor: {candyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" armorType: 8}
    Armor: {candyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" armorType: 9}
    Name: "Captain Christian The Frosted"
    Duration: 50
    FighterName: "Lord Christian The Frosted"
    FighterType: "85f361b8-e55d-0244-1a98-26bffa0a18a2"
    Moves: "41416afb-0d72-f9b4-ba30-f089b713500f"
    Moves: "41416afb-0d72-f9b4-ba30-f089b713500f"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Moves: "86bd4f81-e96b-9864-78b5-5fc51fbff6a4"
    Moves: "1d1dec74-ca35-0cc4-d930-164786a9dd81"
    Moves: "628ebc0b-bb19-ee64-68b8-2f66f4b92720"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "33d499a1-9dee-dfd4-7843-c743a6131dcf" amount: 50}
    RequiredCandy: {candyType: "af509c01-933e-ce14-49c3-106081e16d75" amount: 30}
    RequiredCandy: {candyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" amount: 50}
    RequiredCandy: {candyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" amount: 30}
    RequiredCandy: {candyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Rare_CommanderEricaofFruitville"
  value:
  {
    AuthoredId: "80ede00d-10fd-10c4-baaa-045acaeedc22"
    AnimationId: "c6e3c59b-4415-52b4-8b34-1593bbc26634"
    Armor: {candyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" armorType: 4}
    Armor: {candyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" armorType: 5}
    Armor: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" armorType: 16}
    Armor: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" armorType: 17}
    Name: "Commander Erica of Fruitville"
    Duration: 50
    FighterName: "Lady Erica of Fruitville"
    FighterType: "2522657f-c73f-23c4-da7d-3934d135265b"
    Moves: "e16531a7-33fd-a3e4-69fd-ff10eba65cb7"
    Moves: "aa7d8565-99e1-84f4-49d2-8925c2602a7b"
    Moves: "e16531a7-33fd-a3e4-69fd-ff10eba65cb7"
    Moves: "aa7d8565-99e1-84f4-49d2-8925c2602a7b"
    Moves: "86b323c2-b2fd-2494-ab5e-bc3514bc92d8"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Moves: "0090580c-04ef-9d84-e883-32f52c977b98"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" amount: 50}
    RequiredCandy: {candyType: "9c64b7e4-0458-65c4-9b79-5cc35e847f9c" amount: 30}
    RequiredCandy: {candyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" amount: 50}
    RequiredCandy: {candyType: "9b70e722-0442-2924-092c-93efc1d92975" amount: 30}
    RequiredCandy: {candyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Rare_CommanderKristyTheCrusted"
  value:
  {
    AuthoredId: "6d88afa9-a489-d6b4-b84c-5ae9b5762055"
    AnimationId: "0c2cdea6-ff22-b124-da21-882987b1707a"
    Armor: {candyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" armorType: 18}
    Armor: {candyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" armorType: 10}
    Armor: {candyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" armorType: 8}
    Armor: {candyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" armorType: 9}
    Name: "Commander Kristy The Crusted"
    Duration: 50
    FighterName: "Lady Kristy The Crusted"
    FighterType: "a672e1da-a87d-2a54-4918-cafb72b02aec"
    Moves: "e16531a7-33fd-a3e4-69fd-ff10eba65cb7"
    Moves: "e16531a7-33fd-a3e4-69fd-ff10eba65cb7"
    Moves: "e16531a7-33fd-a3e4-69fd-ff10eba65cb7"
    Moves: "aa7d8565-99e1-84f4-49d2-8925c2602a7b"
    Moves: "fa6144f9-ad49-8124-386f-351a7f1ab546"
    Moves: "fa6144f9-ad49-8124-386f-351a7f1ab546"
    Moves: "c28e13b4-cb1f-6ea4-680f-91010a400e10"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "1525c955-d3c0-9f84-69fb-cf98837f4f85" amount: 50}
    RequiredCandy: {candyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" amount: 30}
    RequiredCandy: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" amount: 50}
    RequiredCandy: {candyType: "9b70e722-0442-2924-092c-93efc1d92975" amount: 30}
    RequiredCandy: {candyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Rare_CaptainGrahamTheSnacktastic"
  value:
  {
    AuthoredId: "28e2a7ea-e606-37d4-4b77-33069ba87bc8"
    AnimationId: "c6e3c59b-4415-52b4-8b34-1593bbc26634"
    Armor: {candyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" armorType: 1}
    Armor: {candyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" armorType: 2}
    Armor: {candyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" armorType: 3}
    Name: "Captain Graham the Snacktastic"
    Duration: 50
    FighterName: "Lord Graham the Snacktastic"
    FighterType: "85f361b8-e55d-0244-1a98-26bffa0a18a2"
    Moves: "55e05ece-a3f6-a924-3833-2222a0eee6a3"
    Moves: "86bd4f81-e96b-9864-78b5-5fc51fbff6a4"
    Moves: "e201f964-8949-9d74-fbee-4ee8786284ca"
    Moves: "e201f964-8949-9d74-fbee-4ee8786284ca"
    Moves: "fa6144f9-ad49-8124-386f-351a7f1ab546"
    Moves: "b1bd32de-88b2-1204-7b2e-0680677d78c4"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" amount: 50}
    RequiredCandy: {candyType: "9c64b7e4-0458-65c4-9b79-5cc35e847f9c" amount: 30}
    RequiredCandy: {candyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" amount: 50}
    RequiredCandy: {candyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" amount: 30}
    RequiredCandy: {candyType: "4e343f68-1aaa-0e84-7bab-eae458b68264" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Rare_CommanderEmmaTheJawbreaker"
  value:
  {
    AuthoredId: "fae112f6-dba3-e154-78bf-154ec2f3f94b"
    AnimationId: "a170f9f3-f97b-8eb4-49c0-c99a916e55d7"
    Armor: {candyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" armorType: 1}
    Armor: {candyType: "934dae05-689f-30e4-5804-d497aee0b47c" armorType: 2}
    Armor: {candyType: "934dae05-689f-30e4-5804-d497aee0b47c" armorType: 3}
    Armor: {candyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" armorType: 11}
    Name: "Commander Emma The Jawbreaker"
    Duration: 50
    FighterName: "Lady Emma The Jawbreaker"
    FighterType: "4c2d03be-70bf-9294-c8d5-0a8e2d8d0677"
    Moves: "aa7d8565-99e1-84f4-49d2-8925c2602a7b"
    Moves: "aa7d8565-99e1-84f4-49d2-8925c2602a7b"
    Moves: "86b323c2-b2fd-2494-ab5e-bc3514bc92d8"
    Moves: "9fd0b9ef-2e7d-d344-8926-69f1fab99f65"
    Moves: "9f0df0ed-13a6-84c4-781d-9dbf9741c66e"
    Moves: "9f0df0ed-13a6-84c4-781d-9dbf9741c66e"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" amount: 50}
    RequiredCandy: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" amount: 30}
    RequiredCandy: {candyType: "7ea89b0d-7070-d3e4-a86f-c0e75c0e4ff5" amount: 50}
    RequiredCandy: {candyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" amount: 30}
    RequiredCandy: {candyType: "4e343f68-1aaa-0e84-7bab-eae458b68264" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Rare_CaptainEricTheBitter"
  value:
  {
    AuthoredId: "8bc478cc-d85e-2d84-28f3-1781f582319f"
    AnimationId: "0c2cdea6-ff22-b124-da21-882987b1707a"
    Armor: {candyType: "87040ddf-f0cb-f894-dbbd-a4f266cae4e7" armorType: 12}
    Armor: {candyType: "87040ddf-f0cb-f894-dbbd-a4f266cae4e7" armorType: 13}
    Armor: {candyType: "87040ddf-f0cb-f894-dbbd-a4f266cae4e7" armorType: 11}
    Name: "Captain Eric The Bitter"
    Duration: 50
    FighterName: "Lord Eric The Bitter"
    FighterType: "a672e1da-a87d-2a54-4918-cafb72b02aec"
    Moves: "0090580c-04ef-9d84-e883-32f52c977b98"
    Moves: "0090580c-04ef-9d84-e883-32f52c977b98"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Moves: "86bd4f81-e96b-9864-78b5-5fc51fbff6a4"
    Moves: "e201f964-8949-9d74-fbee-4ee8786284ca"
    Moves: "9f0df0ed-13a6-84c4-781d-9dbf9741c66e"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" amount: 50}
    RequiredCandy: {candyType: "1525c955-d3c0-9f84-69fb-cf98837f4f85" amount: 30}
    RequiredCandy: {candyType: "9b70e722-0442-2924-092c-93efc1d92975" amount: 50}
    RequiredCandy: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" amount: 30}
    RequiredCandy: {candyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_InvestorReward_Gold"
  value:
  {
    AuthoredId: "0ef5f6fd-4dbf-14a4-9b00-04f71e34feb7"
    AnimationId: "b0877d2a-71db-7374-8a8b-8044f7a8a52a"
    Armor: {candyType: "f6c3b895-6488-1b74-a9b0-664027a683e2" armorType: 1}
    Armor: {candyType: "f6c3b895-6488-1b74-a9b0-664027a683e2" armorType: 2}
    Armor: {candyType: "f6c3b895-6488-1b74-a9b0-664027a683e2" armorType: 3}
    Armor: {candyType: "52ea4044-e203-16d4-c90f-8b685ed5e9c9" armorType: 4}
    Armor: {candyType: "52ea4044-e203-16d4-c90f-8b685ed5e9c9" armorType: 5}
    Armor: {candyType: "52ea4044-e203-16d4-c90f-8b685ed5e9c9" armorType: 6}
    Armor: {candyType: "52ea4044-e203-16d4-c90f-8b685ed5e9c9" armorType: 7}
    Armor: {candyType: "8898194e-ac52-f624-a830-176655bb43cd" armorType: 8}
    Armor: {candyType: "8898194e-ac52-f624-a830-176655bb43cd" armorType: 9}
    Armor: {candyType: "8898194e-ac52-f624-a830-176655bb43cd" armorType: 10}
    Armor: {candyType: "2f765969-c607-d4a4-0b9c-72d3cce7fbec" armorType: 11}
    Armor: {candyType: "2f765969-c607-d4a4-0b9c-72d3cce7fbec" armorType: 12}
    Armor: {candyType: "2f765969-c607-d4a4-0b9c-72d3cce7fbec" armorType: 13}
    Armor: {candyType: "47b8a853-791a-1424-1ba9-4adaf4a3f4a5" armorType: 14}
    Armor: {candyType: "47b8a853-791a-1424-1ba9-4adaf4a3f4a5" armorType: 16}
    Armor: {candyType: "47b8a853-791a-1424-1ba9-4adaf4a3f4a5" armorType: 15}
    Armor: {candyType: "47b8a853-791a-1424-1ba9-4adaf4a3f4a5" armorType: 17}
    Armor: {candyType: "8898194e-ac52-f624-a830-176655bb43cd" armorType: 18}
    Name: "Recipe_InvestorReward_Gold"
    Duration: 20
    FighterName: "Sterling Golden Chip"
    FighterType: "2522657f-c73f-23c4-da7d-3934d135265b"
    Moves: "01b34a91-37d8-29d4-09cb-2d515f51e315"
    Moves: "c6770789-4a1f-7f24-f921-c4dc66c2697a"
    Moves: "92f7b745-c1ff-3cd4-f9cf-0fe0dafb5536"
    Moves: "34a115db-3153-aff4-9a46-97700634f1fb"
    Moves: "4dbb297f-6a80-ca44-a94f-3ea9ed3311e8"
    Moves: "e55a3d69-9fb4-5194-4812-c2f85e887aca"
    Moves: "1a67cc79-b97a-4624-ca67-36c9fcbee970"
    Moves: "bbf71f26-78f5-5014-5b4b-a1c3866cb75b"
    Moves: "61d8b3ec-9a01-0e64-fb45-27d2b858e332"
    Quality: 4
    IsAccountBound: true
    RequiredFighterQuality: 0
  }
}
recepies:
{
  key: "Recipe_Rare_CaptainRollieSnackwell"
  value:
  {
    AuthoredId: "dfd860da-2127-60e4-8b46-39bfffb57245"
    AnimationId: "05633498-ace9-de14-c939-9435a6343d0f"
    Armor: {candyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" armorType: 4}
    Armor: {candyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" armorType: 5}
    Armor: {candyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" armorType: 6}
    Armor: {candyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" armorType: 7}
    Name: "Captain Rollie Snackwell"
    Duration: 50
    FighterName: "Lord Rollie Snackwell"
    FighterType: "c160f7ad-c775-8614-abe2-8ef74e54401f"
    Moves: "ad6d5d28-08b4-4304-6b49-11f8f5f9ff1c"
    Moves: "aa7d8565-99e1-84f4-49d2-8925c2602a7b"
    Moves: "1a67cc79-b97a-4624-ca67-36c9fcbee970"
    Moves: "aa7d8565-99e1-84f4-49d2-8925c2602a7b"
    Moves: "1d1dec74-ca35-0cc4-d930-164786a9dd81"
    Moves: "86b323c2-b2fd-2494-ab5e-bc3514bc92d8"
    Moves: "2c94f949-a10e-2524-2827-6588982ce0b1"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "28089a53-62ac-2044-1964-29f2f31d8a33" amount: 50}
    RequiredCandy: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" amount: 30}
    RequiredCandy: {candyType: "9b70e722-0442-2924-092c-93efc1d92975" amount: 50}
    RequiredCandy: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" amount: 30}
    RequiredCandy: {candyType: "87040ddf-f0cb-f894-dbbd-a4f266cae4e7" amount: 35}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Rare_CommanderCharlotteTheHoney-Striker"
  value:
  {
    AuthoredId: "99f7eaf3-67c6-30d4-5a3d-4a2a18bb75f7"
    AnimationId: "5fce85e9-51f9-d7b4-db81-ef141439db00"
    Armor: {candyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" armorType: 11}
    Armor: {candyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" armorType: 12}
    Armor: {candyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" armorType: 13}
    Name: "Commander Charlotte The Honey-Striker"
    Duration: 50
    FighterName: "Lady Charlotte The Honey-Striker"
    FighterType: "c160f7ad-c775-8614-abe2-8ef74e54401f"
    Moves: "2569d967-c676-8b44-cbdf-d514e4c5c73d"
    Moves: "2569d967-c676-8b44-cbdf-d514e4c5c73d"
    Moves: "ad6d5d28-08b4-4304-6b49-11f8f5f9ff1c"
    Moves: "ad6d5d28-08b4-4304-6b49-11f8f5f9ff1c"
    Moves: "86bd4f81-e96b-9864-78b5-5fc51fbff6a4"
    Moves: "a9d69fc0-a1b8-ae24-cbd7-249760200211"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "28089a53-62ac-2044-1964-29f2f31d8a33" amount: 50}
    RequiredCandy: {candyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" amount: 30}
    RequiredCandy: {candyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" amount: 50}
    RequiredCandy: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" amount: 30}
    RequiredCandy: {candyType: "934dae05-689f-30e4-5804-d497aee0b47c" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Rare_CommanderDesireeDelight"
  value:
  {
    AuthoredId: "9a2c04f2-cd9c-e484-daca-d4e4a6b5b9b8"
    AnimationId: "97c2c3da-93c7-a0d4-1967-eeba13396cce"
    Armor: {candyType: "af509c01-933e-ce14-49c3-106081e16d75" armorType: 16}
    Armor: {candyType: "af509c01-933e-ce14-49c3-106081e16d75" armorType: 17}
    Armor: {candyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" armorType: 14}
    Armor: {candyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" armorType: 15}
    Name: "Commander Desiree Delight"
    Duration: 50
    FighterName: "Lady Desiree Delight"
    FighterType: "2522657f-c73f-23c4-da7d-3934d135265b"
    Moves: "e16531a7-33fd-a3e4-69fd-ff10eba65cb7"
    Moves: "aa7d8565-99e1-84f4-49d2-8925c2602a7b"
    Moves: "86b323c2-b2fd-2494-ab5e-bc3514bc92d8"
    Moves: "9fd0b9ef-2e7d-d344-8926-69f1fab99f65"
    Moves: "431900a2-54d4-df34-d9f7-6a4608fecfaa"
    Moves: "1d1dec74-ca35-0cc4-d930-164786a9dd81"
    Moves: "1e47a8e0-acfd-1244-0b7d-539d62429060"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "af509c01-933e-ce14-49c3-106081e16d75" amount: 50}
    RequiredCandy: {candyType: "9c64b7e4-0458-65c4-9b79-5cc35e847f9c" amount: 30}
    RequiredCandy: {candyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" amount: 50}
    RequiredCandy: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" amount: 30}
    RequiredCandy: {candyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Rare_CaptainFrankTheCrunchy"
  value:
  {
    AuthoredId: "c28bd52c-abf1-eff4-da81-6a47f7bd79a9"
    AnimationId: "a7629ff3-c1ef-e664-3a9c-bd27cb332abd"
    Armor: {candyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" armorType: 1}
    Armor: {candyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" armorType: 2}
    Armor: {candyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" armorType: 3}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 10}
    Name: "Captain Frank The Crunchy"
    Duration: 50
    FighterName: "Lord Frank The Crunchy"
    FighterType: "c160f7ad-c775-8614-abe2-8ef74e54401f"
    Moves: "ad6d5d28-08b4-4304-6b49-11f8f5f9ff1c"
    Moves: "ad6d5d28-08b4-4304-6b49-11f8f5f9ff1c"
    Moves: "1e47a8e0-acfd-1244-0b7d-539d62429060"
    Moves: "bd27fe1c-ce9a-f524-c887-61f9e4d5088b"
    Moves: "bd27fe1c-ce9a-f524-c887-61f9e4d5088b"
    Moves: "86bd4f81-e96b-9864-78b5-5fc51fbff6a4"
    Moves: "a9d69fc0-a1b8-ae24-cbd7-249760200211"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" amount: 50}
    RequiredCandy: {candyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" amount: 30}
    RequiredCandy: {candyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" amount: 50}
    RequiredCandy: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" amount: 30}
    RequiredCandy: {candyType: "87040ddf-f0cb-f894-dbbd-a4f266cae4e7" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Rare_CaptainScottyBiscotti"
  value:
  {
    AuthoredId: "4b90dbd5-e6d3-69c4-8b8d-1327bf8381af"
    AnimationId: "a2a185e5-4543-6814-d88d-d078a0e38521"
    Armor: {candyType: "af509c01-933e-ce14-49c3-106081e16d75" armorType: 16}
    Armor: {candyType: "af509c01-933e-ce14-49c3-106081e16d75" armorType: 15}
    Armor: {candyType: "af509c01-933e-ce14-49c3-106081e16d75" armorType: 15}
    Armor: {candyType: "af509c01-933e-ce14-49c3-106081e16d75" armorType: 14}
    Name: "Captain Scotty Biscotti"
    Duration: 50
    FighterName: "Lord Scotty Biscotti"
    FighterType: "4c2d03be-70bf-9294-c8d5-0a8e2d8d0677"
    Moves: "55e05ece-a3f6-a924-3833-2222a0eee6a3"
    Moves: "86bd4f81-e96b-9864-78b5-5fc51fbff6a4"
    Moves: "e201f964-8949-9d74-fbee-4ee8786284ca"
    Moves: "a9d69fc0-a1b8-ae24-cbd7-249760200211"
    Moves: "2c555752-8a84-58f4-395e-6460b7864069"
    Moves: "b889835e-1cd6-d3d4-f9b4-dd1989f49dac"
    Moves: "628ebc0b-bb19-ee64-68b8-2f66f4b92720"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "86b5fac0-4ec1-09e4-6ab7-96d3fd13dd04" amount: 50}
    RequiredCandy: {candyType: "1525c955-d3c0-9f84-69fb-cf98837f4f85" amount: 30}
    RequiredCandy: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" amount: 50}
    RequiredCandy: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" amount: 30}
    RequiredCandy: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" amount: 42}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Rare_CommanderJillianofGummiland"
  value:
  {
    AuthoredId: "63da609a-1938-3524-08ac-879fea5cce55"
    AnimationId: "a7629ff3-c1ef-e664-3a9c-bd27cb332abd"
    Armor: {candyType: "af509c01-933e-ce14-49c3-106081e16d75" armorType: 14}
    Armor: {candyType: "af509c01-933e-ce14-49c3-106081e16d75" armorType: 15}
    Armor: {candyType: "af509c01-933e-ce14-49c3-106081e16d75" armorType: 16}
    Armor: {candyType: "af509c01-933e-ce14-49c3-106081e16d75" armorType: 17}
    Name: "Lady Jillian of Gummiland"
    Duration: 50
    FighterName: "Lady Jillian of Gummiland"
    FighterType: "c160f7ad-c775-8614-abe2-8ef74e54401f"
    Moves: "ad6d5d28-08b4-4304-6b49-11f8f5f9ff1c"
    Moves: "2569d967-c676-8b44-cbdf-d514e4c5c73d"
    Moves: "ad6d5d28-08b4-4304-6b49-11f8f5f9ff1c"
    Moves: "2569d967-c676-8b44-cbdf-d514e4c5c73d"
    Moves: "86b323c2-b2fd-2494-ab5e-bc3514bc92d8"
    Moves: "9fd0b9ef-2e7d-d344-8926-69f1fab99f65"
    Moves: "9fd0b9ef-2e7d-d344-8926-69f1fab99f65"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "af509c01-933e-ce14-49c3-106081e16d75" amount: 50}
    RequiredCandy: {candyType: "9c64b7e4-0458-65c4-9b79-5cc35e847f9c" amount: 30}
    RequiredCandy: {candyType: "9b70e722-0442-2924-092c-93efc1d92975" amount: 50}
    RequiredCandy: {candyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" amount: 30}
    RequiredCandy: {candyType: "87040ddf-f0cb-f894-dbbd-a4f266cae4e7" amount: 50}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Epic_OscarDeLaCola"
  value:
  {
    AuthoredId: "ec86d547-e1d0-6324-bb20-e0a20e116fb2"
    AnimationId: "64732638-e355-d5c4-aa2d-e28c3db4aa72"
    Armor: {candyType: "321c2a65-64b1-8344-2915-85562e2ad7ec" armorType: 1}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 2}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 3}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 4}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 5}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 6}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 7}
    Armor: {candyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" armorType: 8}
    Armor: {candyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" armorType: 9}
    Armor: {candyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" armorType: 10}
    Armor: {candyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" armorType: 18}
    Armor: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" armorType: 11}
    Armor: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" armorType: 12}
    Armor: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" armorType: 13}
    Armor: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" armorType: 14}
    Armor: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" armorType: 15}
    Armor: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" armorType: 16}
    Armor: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" armorType: 17}
    Name: "Oscar De La Cola"
    Duration: 100
    FighterName: "Oscar De La Cola"
    FighterType: "4c2d03be-70bf-9294-c8d5-0a8e2d8d0677"
    Moves: "e201f964-8949-9d74-fbee-4ee8786284ca"
    Moves: "e201f964-8949-9d74-fbee-4ee8786284ca"
    Moves: "86bd4f81-e96b-9864-78b5-5fc51fbff6a4"
    Moves: "2c555752-8a84-58f4-395e-6460b7864069"
    Moves: "a9d69fc0-a1b8-ae24-cbd7-249760200211"
    Moves: "628ebc0b-bb19-ee64-68b8-2f66f4b92720"
    Moves: "c28e13b4-cb1f-6ea4-680f-91010a400e10"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Moves: "431900a2-54d4-df34-d9f7-6a4608fecfaa"
    Quality: 4
    IsAccountBound: false
    RequiredCandy: {candyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" amount: 50}
    RequiredCandy: {candyType: "48ea4c49-79dd-6514-6b8b-f87fd6d8a330" amount: 30}
    RequiredCandy: {candyType: "aa0cb18d-5681-5504-09d7-4db06bf7a4d8" amount: 25}
    RequiredCandy: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" amount: 30}
    RequiredCandy: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" amount: 75}
    RequiredCandy: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" amount: 50}
    RequiredCandy: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" amount: 25}
    RequiredCandy: {candyType: "934dae05-689f-30e4-5804-d497aee0b47c" amount: 50}
    RequiredFighterQuality: 2
  }
}
recepies:
{
  key: "Recipe_Rare_CommanderCortniSugarbowl"
  value:
  {
    AuthoredId: "8f8425e7-cab0-0b14-1874-5072afe7d0b5"
    AnimationId: "917e27ec-19ff-3d04-69f6-c596456816e6"
    Armor: {candyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" armorType: 10}
    Armor: {candyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" armorType: 18}
    Armor: {candyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" armorType: 8}
    Armor: {candyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" armorType: 9}
    Name: "Commander Cortni Sugarbowl"
    Duration: 50
    FighterName: "Lady Cortni Sugarbowl"
    FighterType: "a672e1da-a87d-2a54-4918-cafb72b02aec"
    Moves: "41416afb-0d72-f9b4-ba30-f089b713500f"
    Moves: "41416afb-0d72-f9b4-ba30-f089b713500f"
    Moves: "2c94f949-a10e-2524-2827-6588982ce0b1"
    Moves: "2c94f949-a10e-2524-2827-6588982ce0b1"
    Moves: "431900a2-54d4-df34-d9f7-6a4608fecfaa"
    Moves: "86b323c2-b2fd-2494-ab5e-bc3514bc92d8"
    Moves: "29d559bb-41a7-9f14-5bf8-605da8166943"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" amount: 50}
    RequiredCandy: {candyType: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c" amount: 30}
    RequiredCandy: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" amount: 50}
    RequiredCandy: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" amount: 30}
    RequiredCandy: {candyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Rare_CommanderErikaEverlasting"
  value:
  {
    AuthoredId: "83bdccb0-abc7-3264-8994-4d1700458a07"
    AnimationId: "2831df30-e1a2-dcf4-2a9a-0c91ac4ea310"
    Armor: {candyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" armorType: 1}
    Armor: {candyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" armorType: 2}
    Armor: {candyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" armorType: 3}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 11}
    Name: "Commander Erika Everlasting"
    Duration: 50
    FighterName: "Lady Erika Everlasting"
    FighterType: "c160f7ad-c775-8614-abe2-8ef74e54401f"
    Moves: "ad6d5d28-08b4-4304-6b49-11f8f5f9ff1c"
    Moves: "2569d967-c676-8b44-cbdf-d514e4c5c73d"
    Moves: "b1bd32de-88b2-1204-7b2e-0680677d78c4"
    Moves: "b889835e-1cd6-d3d4-f9b4-dd1989f49dac"
    Moves: "1d1dec74-ca35-0cc4-d930-164786a9dd81"
    Moves: "1e47a8e0-acfd-1244-0b7d-539d62429060"
    Moves: "bd27fe1c-ce9a-f524-c887-61f9e4d5088b"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" amount: 50}
    RequiredCandy: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" amount: 30}
    RequiredCandy: {candyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" amount: 50}
    RequiredCandy: {candyType: "7ea89b0d-7070-d3e4-a86f-c0e75c0e4ff5" amount: 30}
    RequiredCandy: {candyType: "4e343f68-1aaa-0e84-7bab-eae458b68264" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Rare_CaptainKrisofMarshmallow"
  value:
  {
    AuthoredId: "c229b9d2-8e0f-8644-7b09-1b6ba6247026"
    AnimationId: "64732638-e355-d5c4-aa2d-e28c3db4aa72"
    Armor: {candyType: "8aac4645-29cf-fe04-59c0-415508b0432e" armorType: 14}
    Armor: {candyType: "8aac4645-29cf-fe04-59c0-415508b0432e" armorType: 15}
    Armor: {candyType: "8aac4645-29cf-fe04-59c0-415508b0432e" armorType: 17}
    Armor: {candyType: "8aac4645-29cf-fe04-59c0-415508b0432e" armorType: 16}
    Name: "Captain Kris of Marshmallow"
    Duration: 50
    FighterName: "Lord Kris of Marshmallow"
    FighterType: "2522657f-c73f-23c4-da7d-3934d135265b"
    Moves: "55e05ece-a3f6-a924-3833-2222a0eee6a3"
    Moves: "55e05ece-a3f6-a924-3833-2222a0eee6a3"
    Moves: "e201f964-8949-9d74-fbee-4ee8786284ca"
    Moves: "e201f964-8949-9d74-fbee-4ee8786284ca"
    Moves: "a9d69fc0-a1b8-ae24-cbd7-249760200211"
    Moves: "2c555752-8a84-58f4-395e-6460b7864069"
    Moves: "c28e13b4-cb1f-6ea4-680f-91010a400e10"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" amount: 50}
    RequiredCandy: {candyType: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c" amount: 30}
    RequiredCandy: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" amount: 50}
    RequiredCandy: {candyType: "9b70e722-0442-2924-092c-93efc1d92975" amount: 25}
    RequiredCandy: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" amount: 45}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Rare_CaptainDaveCookieCrust"
  value:
  {
    AuthoredId: "27f47d35-c2d3-c004-9a36-5253005559c2"
    AnimationId: "f33c74e6-f1b4-6ca4-bb5a-cf6edc77cab5"
    Armor: {candyType: "4e343f68-1aaa-0e84-7bab-eae458b68264" armorType: 11}
    Armor: {candyType: "4e343f68-1aaa-0e84-7bab-eae458b68264" armorType: 12}
    Armor: {candyType: "4e343f68-1aaa-0e84-7bab-eae458b68264" armorType: 13}
    Name: "Captain Dave Cookie-Crust"
    Duration: 50
    FighterName: "Lord Dave Cookie-Crust"
    FighterType: "4c2d03be-70bf-9294-c8d5-0a8e2d8d0677"
    Moves: "2c94f949-a10e-2524-2827-6588982ce0b1"
    Moves: "2c94f949-a10e-2524-2827-6588982ce0b1"
    Moves: "431900a2-54d4-df34-d9f7-6a4608fecfaa"
    Moves: "431900a2-54d4-df34-d9f7-6a4608fecfaa"
    Moves: "0090580c-04ef-9d84-e883-32f52c977b98"
    Moves: "628ebc0b-bb19-ee64-68b8-2f66f4b92720"
    Moves: "b889835e-1cd6-d3d4-f9b4-dd1989f49dac"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "9ff884ab-efcb-0af4-5acd-976fea43b3a3" amount: 50}
    RequiredCandy: {candyType: "86b5fac0-4ec1-09e4-6ab7-96d3fd13dd04" amount: 30}
    RequiredCandy: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" amount: 50}
    RequiredCandy: {candyType: "7ea89b0d-7070-d3e4-a86f-c0e75c0e4ff5" amount: 30}
    RequiredCandy: {candyType: "8aac4645-29cf-fe04-59c0-415508b0432e" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Rare_CaptainGeordieTheSour"
  value:
  {
    AuthoredId: "fe9ec873-e4b8-0714-2be0-a3c2c547f9ad"
    AnimationId: "2831df30-e1a2-dcf4-2a9a-0c91ac4ea310"
    Armor: {candyType: "934dae05-689f-30e4-5804-d497aee0b47c" armorType: 1}
    Armor: {candyType: "16e0b307-2b40-4584-294c-dee505e47827" armorType: 2}
    Armor: {candyType: "16e0b307-2b40-4584-294c-dee505e47827" armorType: 3}
    Name: "Captain Geordie The Sour"
    Duration: 50
    FighterName: "Lord Geordie The Sour"
    FighterType: "a672e1da-a87d-2a54-4918-cafb72b02aec"
    Moves: "b96f1ff8-66d5-f244-79fd-260735d4c3d7"
    Moves: "61d8b3ec-9a01-0e64-fb45-27d2b858e332"
    Moves: "29d559bb-41a7-9f14-5bf8-605da8166943"
    Moves: "29d559bb-41a7-9f14-5bf8-605da8166943"
    Moves: "2569d967-c676-8b44-cbdf-d514e4c5c73d"
    Moves: "fa6144f9-ad49-8124-386f-351a7f1ab546"
    Moves: "a9d69fc0-a1b8-ae24-cbd7-249760200211"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "1525c955-d3c0-9f84-69fb-cf98837f4f85" amount: 50}
    RequiredCandy: {candyType: "af509c01-933e-ce14-49c3-106081e16d75" amount: 30}
    RequiredCandy: {candyType: "7ea89b0d-7070-d3e4-a86f-c0e75c0e4ff5" amount: 50}
    RequiredCandy: {candyType: "9b70e722-0442-2924-092c-93efc1d92975" amount: 30}
    RequiredCandy: {candyType: "16e0b307-2b40-4584-294c-dee505e47827" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_InvestorReward_Bronze"
  value:
  {
    AuthoredId: "b5850520-cb42-d804-d887-38e4577f62a6"
    AnimationId: "471eee2f-e6ad-a694-58de-bdee5a379096"
    Armor: {candyType: "aa0cb18d-5681-5504-09d7-4db06bf7a4d8" armorType: 1}
    Armor: {candyType: "aa0cb18d-5681-5504-09d7-4db06bf7a4d8" armorType: 2}
    Armor: {candyType: "aa0cb18d-5681-5504-09d7-4db06bf7a4d8" armorType: 3}
    Armor: {candyType: "8932caea-da2e-6384-58ec-749624e213c9" armorType: 4}
    Armor: {candyType: "8932caea-da2e-6384-58ec-749624e213c9" armorType: 5}
    Armor: {candyType: "8932caea-da2e-6384-58ec-749624e213c9" armorType: 6}
    Armor: {candyType: "8932caea-da2e-6384-58ec-749624e213c9" armorType: 7}
    Armor: {candyType: "48ea4c49-79dd-6514-6b8b-f87fd6d8a330" armorType: 8}
    Armor: {candyType: "48ea4c49-79dd-6514-6b8b-f87fd6d8a330" armorType: 9}
    Armor: {candyType: "48ea4c49-79dd-6514-6b8b-f87fd6d8a330" armorType: 10}
    Armor: {candyType: "8f896243-a6de-1b54-68a3-0df72e8688e4" armorType: 11}
    Armor: {candyType: "8f896243-a6de-1b54-68a3-0df72e8688e4" armorType: 12}
    Armor: {candyType: "8f896243-a6de-1b54-68a3-0df72e8688e4" armorType: 13}
    Armor: {candyType: "a2c2697c-d738-c004-4b32-d800921bab06" armorType: 14}
    Armor: {candyType: "a2c2697c-d738-c004-4b32-d800921bab06" armorType: 16}
    Armor: {candyType: "a2c2697c-d738-c004-4b32-d800921bab06" armorType: 15}
    Armor: {candyType: "a2c2697c-d738-c004-4b32-d800921bab06" armorType: 17}
    Armor: {candyType: "48ea4c49-79dd-6514-6b8b-f87fd6d8a330" armorType: 18}
    Name: "Recipe_InvestorReward_Bronze"
    Duration: 20
    FighterName: "Luke Saltwater the Bronzed"
    FighterType: "c160f7ad-c775-8614-abe2-8ef74e54401f"
    Moves: "8e4753b5-c739-aaf4-ab79-4d333a69cae6"
    Moves: "55cafe03-4a58-0f44-6a27-f9fc6fce881a"
    Moves: "3e76f2fa-072c-5654-dac9-c9b0e4e1643a"
    Moves: "b96f1ff8-66d5-f244-79fd-260735d4c3d7"
    Moves: "61d8b3ec-9a01-0e64-fb45-27d2b858e332"
    Moves: "e55a3d69-9fb4-5194-4812-c2f85e887aca"
    Moves: "1a67cc79-b97a-4624-ca67-36c9fcbee970"
    Moves: "bbf71f26-78f5-5014-5b4b-a1c3866cb75b"
    Moves: "92f7b745-c1ff-3cd4-f9cf-0fe0dafb5536"
    Quality: 4
    IsAccountBound: false
    RequiredFighterQuality: 0
  }
}
recepies:
{
  key: "Recipe_Rare_CaptainVicofBlowingBubbles"
  value:
  {
    AuthoredId: "223de3eb-e192-0854-d800-6f9f2fa8e28b"
    AnimationId: "ed324300-aa2c-0c04-4b75-2d0eae3780f4"
    Armor: {candyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" armorType: 17}
    Armor: {candyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" armorType: 10}
    Armor: {candyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" armorType: 8}
    Armor: {candyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" armorType: 9}
    Name: "Captain Vic of Blowing Bubbles"
    Duration: 50
    FighterName: "Lord Vic of Blowing Bubbles"
    FighterType: "c160f7ad-c775-8614-abe2-8ef74e54401f"
    Moves: "41416afb-0d72-f9b4-ba30-f089b713500f"
    Moves: "41416afb-0d72-f9b4-ba30-f089b713500f"
    Moves: "2c94f949-a10e-2524-2827-6588982ce0b1"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Moves: "fa6144f9-ad49-8124-386f-351a7f1ab546"
    Moves: "fa6144f9-ad49-8124-386f-351a7f1ab546"
    Moves: "c28e13b4-cb1f-6ea4-680f-91010a400e10"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" amount: 50}
    RequiredCandy: {candyType: "9c64b7e4-0458-65c4-9b79-5cc35e847f9c" amount: 30}
    RequiredCandy: {candyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" amount: 50}
    RequiredCandy: {candyType: "9b70e722-0442-2924-092c-93efc1d92975" amount: 30}
    RequiredCandy: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" amount: 42}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Epic_VladimirCrunchko"
  value:
  {
    AuthoredId: "c1e9a4e1-cf55-6084-8bf2-778678387353"
    AnimationId: "27448f34-ca08-99b4-1b63-fd4b227e6af4"
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 1}
    Armor: {candyType: "f6c3b895-6488-1b74-a9b0-664027a683e2" armorType: 2}
    Armor: {candyType: "f6c3b895-6488-1b74-a9b0-664027a683e2" armorType: 3}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 4}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 5}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 6}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 7}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 8}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 9}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 10}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 18}
    Armor: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" armorType: 11}
    Armor: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" armorType: 12}
    Armor: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" armorType: 13}
    Armor: {candyType: "47b8a853-791a-1424-1ba9-4adaf4a3f4a5" armorType: 14}
    Armor: {candyType: "47b8a853-791a-1424-1ba9-4adaf4a3f4a5" armorType: 15}
    Armor: {candyType: "47b8a853-791a-1424-1ba9-4adaf4a3f4a5" armorType: 16}
    Armor: {candyType: "47b8a853-791a-1424-1ba9-4adaf4a3f4a5" armorType: 17}
    Name: "Vladimir Crunchko"
    Duration: 100
    FighterName: "Vladimir Crunchko"
    FighterType: "4c2d03be-70bf-9294-c8d5-0a8e2d8d0677"
    Moves: "c6770789-4a1f-7f24-f921-c4dc66c2697a"
    Moves: "c6770789-4a1f-7f24-f921-c4dc66c2697a"
    Moves: "2c555752-8a84-58f4-395e-6460b7864069"
    Moves: "55e05ece-a3f6-a924-3833-2222a0eee6a3"
    Moves: "e201f964-8949-9d74-fbee-4ee8786284ca"
    Moves: "01b34a91-37d8-29d4-09cb-2d515f51e315"
    Moves: "431900a2-54d4-df34-d9f7-6a4608fecfaa"
    Moves: "c28e13b4-cb1f-6ea4-680f-91010a400e10"
    Moves: "aa7d8565-99e1-84f4-49d2-8925c2602a7b"
    Quality: 4
    IsAccountBound: false
    RequiredCandy: {candyType: "47b8a853-791a-1424-1ba9-4adaf4a3f4a5" amount: 50}
    RequiredCandy: {candyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" amount: 30}
    RequiredCandy: {candyType: "aa0cb18d-5681-5504-09d7-4db06bf7a4d8" amount: 25}
    RequiredCandy: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" amount: 30}
    RequiredCandy: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" amount: 75}
    RequiredCandy: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" amount: 50}
    RequiredCandy: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" amount: 25}
    RequiredCandy: {candyType: "934dae05-689f-30e4-5804-d497aee0b47c" amount: 50}
    RequiredFighterQuality: 2
  }
}
recepies:
{
  key: "Recipe_Rare_CommanderWhitneyofBrownie"
  value:
  {
    AuthoredId: "8f67d790-fc5b-0604-ab52-6292a829a6fc"
    AnimationId: "d35a9d1c-2223-c404-c840-c0218f9b147d"
    Armor: {candyType: "8aac4645-29cf-fe04-59c0-415508b0432e" armorType: 1}
    Armor: {candyType: "8aac4645-29cf-fe04-59c0-415508b0432e" armorType: 2}
    Armor: {candyType: "8aac4645-29cf-fe04-59c0-415508b0432e" armorType: 2}
    Armor: {candyType: "8aac4645-29cf-fe04-59c0-415508b0432e" armorType: 2}
    Name: "Commander Whitney of Brownie"
    Duration: 50
    FighterName: "Lady Whitney of Brownie"
    FighterType: "85f361b8-e55d-0244-1a98-26bffa0a18a2"
    Moves: "55e05ece-a3f6-a924-3833-2222a0eee6a3"
    Moves: "a9d69fc0-a1b8-ae24-cbd7-249760200211"
    Moves: "a9d69fc0-a1b8-ae24-cbd7-249760200211"
    Moves: "2c555752-8a84-58f4-395e-6460b7864069"
    Moves: "e201f964-8949-9d74-fbee-4ee8786284ca"
    Moves: "ad6d5d28-08b4-4304-6b49-11f8f5f9ff1c"
    Moves: "86b323c2-b2fd-2494-ab5e-bc3514bc92d8"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "33d499a1-9dee-dfd4-7843-c743a6131dcf" amount: 50}
    RequiredCandy: {candyType: "86b5fac0-4ec1-09e4-6ab7-96d3fd13dd04" amount: 30}
    RequiredCandy: {candyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" amount: 50}
    RequiredCandy: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" amount: 30}
    RequiredCandy: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Rare_CommanderSandyTheCandy"
  value:
  {
    AuthoredId: "ba49d448-4c02-1fe4-4b20-a43f623e0d8f"
    AnimationId: "a2a185e5-4543-6814-d88d-d078a0e38521"
    Armor: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" armorType: 14}
    Armor: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" armorType: 15}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 7}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 6}
    Name: "Commander Sandy The Candy"
    Duration: 50
    FighterName: "Lady Sandy The Candy"
    FighterType: "2522657f-c73f-23c4-da7d-3934d135265b"
    Moves: "29d559bb-41a7-9f14-5bf8-605da8166943"
    Moves: "29d559bb-41a7-9f14-5bf8-605da8166943"
    Moves: "ad6d5d28-08b4-4304-6b49-11f8f5f9ff1c"
    Moves: "ad6d5d28-08b4-4304-6b49-11f8f5f9ff1c"
    Moves: "aa7d8565-99e1-84f4-49d2-8925c2602a7b"
    Moves: "e16531a7-33fd-a3e4-69fd-ff10eba65cb7"
    Moves: "55e05ece-a3f6-a924-3833-2222a0eee6a3"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" amount: 50}
    RequiredCandy: {candyType: "9c64b7e4-0458-65c4-9b79-5cc35e847f9c" amount: 30}
    RequiredCandy: {candyType: "7ea89b0d-7070-d3e4-a86f-c0e75c0e4ff5" amount: 50}
    RequiredCandy: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" amount: 30}
    RequiredCandy: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Rare_CommanderHeatherTheCreamy"
  value:
  {
    AuthoredId: "9c89ab1e-b043-6134-a86f-89a986e4e8c0"
    AnimationId: "2831df30-e1a2-dcf4-2a9a-0c91ac4ea310"
    Armor: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" armorType: 14}
    Armor: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" armorType: 15}
    Armor: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" armorType: 17}
    Armor: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" armorType: 16}
    Name: "Commander Heather The Creamy"
    Duration: 50
    FighterName: "Lady Heather The Creamy"
    FighterType: "85f361b8-e55d-0244-1a98-26bffa0a18a2"
    Moves: "29d559bb-41a7-9f14-5bf8-605da8166943"
    Moves: "29d559bb-41a7-9f14-5bf8-605da8166943"
    Moves: "b96f1ff8-66d5-f244-79fd-260735d4c3d7"
    Moves: "bd27fe1c-ce9a-f524-c887-61f9e4d5088b"
    Moves: "b1bd32de-88b2-1204-7b2e-0680677d78c4"
    Moves: "2c94f949-a10e-2524-2827-6588982ce0b1"
    Moves: "41416afb-0d72-f9b4-ba30-f089b713500f"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "28089a53-62ac-2044-1964-29f2f31d8a33" amount: 50}
    RequiredCandy: {candyType: "1525c955-d3c0-9f84-69fb-cf98837f4f85" amount: 30}
    RequiredCandy: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" amount: 50}
    RequiredCandy: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" amount: 30}
    RequiredCandy: {candyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Rare_CaptainVoodooofCupcakes"
  value:
  {
    AuthoredId: "c6cabeb5-60f6-7c34-fb09-c7e24971ff3b"
    AnimationId: "05633498-ace9-de14-c939-9435a6343d0f"
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 2}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 3}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 4}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 5}
    Name: "Captain Voodoo of Cupcakes"
    Duration: 50
    FighterName: "Lord Voodoo of Cupcakes"
    FighterType: "85f361b8-e55d-0244-1a98-26bffa0a18a2"
    Moves: "61d8b3ec-9a01-0e64-fb45-27d2b858e332"
    Moves: "b96f1ff8-66d5-f244-79fd-260735d4c3d7"
    Moves: "b889835e-1cd6-d3d4-f9b4-dd1989f49dac"
    Moves: "b889835e-1cd6-d3d4-f9b4-dd1989f49dac"
    Moves: "29d559bb-41a7-9f14-5bf8-605da8166943"
    Moves: "29d559bb-41a7-9f14-5bf8-605da8166943"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" amount: 50}
    RequiredCandy: {candyType: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c" amount: 30}
    RequiredCandy: {candyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" amount: 50}
    RequiredCandy: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" amount: 30}
    RequiredCandy: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" amount: 38}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Epic_JoanofChocolateBark"
  value:
  {
    AuthoredId: "40f97352-9e9e-39d4-a830-6672d1ed46f6"
    AnimationId: "97c2c3da-93c7-a0d4-1967-eeba13396cce"
    Armor: {candyType: "aa0cb18d-5681-5504-09d7-4db06bf7a4d8" armorType: 1}
    Armor: {candyType: "aa0cb18d-5681-5504-09d7-4db06bf7a4d8" armorType: 2}
    Armor: {candyType: "aa0cb18d-5681-5504-09d7-4db06bf7a4d8" armorType: 3}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 4}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 5}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 6}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 7}
    Armor: {candyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" armorType: 8}
    Armor: {candyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" armorType: 9}
    Armor: {candyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" armorType: 10}
    Armor: {candyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" armorType: 18}
    Armor: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" armorType: 11}
    Armor: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" armorType: 12}
    Armor: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" armorType: 13}
    Armor: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" armorType: 14}
    Armor: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" armorType: 15}
    Armor: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" armorType: 16}
    Armor: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" armorType: 17}
    Name: "Joan of Chocolate Bark"
    Duration: 100
    FighterName: "Joan of Chocolate Bark"
    FighterType: "2522657f-c73f-23c4-da7d-3934d135265b"
    Moves: "61d8b3ec-9a01-0e64-fb45-27d2b858e332"
    Moves: "61d8b3ec-9a01-0e64-fb45-27d2b858e332"
    Moves: "b96f1ff8-66d5-f244-79fd-260735d4c3d7"
    Moves: "29d559bb-41a7-9f14-5bf8-605da8166943"
    Moves: "34a115db-3153-aff4-9a46-97700634f1fb"
    Moves: "b889835e-1cd6-d3d4-f9b4-dd1989f49dac"
    Moves: "b1bd32de-88b2-1204-7b2e-0680677d78c4"
    Moves: "0090580c-04ef-9d84-e883-32f52c977b98"
    Moves: "9f0df0ed-13a6-84c4-781d-9dbf9741c66e"
    Quality: 4
    IsAccountBound: false
    RequiredCandy: {candyType: "aa0cb18d-5681-5504-09d7-4db06bf7a4d8" amount: 50}
    RequiredCandy: {candyType: "f6c3b895-6488-1b74-a9b0-664027a683e2" amount: 30}
    RequiredCandy: {candyType: "1525c955-d3c0-9f84-69fb-cf98837f4f85" amount: 25}
    RequiredCandy: {candyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" amount: 30}
    RequiredCandy: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" amount: 75}
    RequiredCandy: {candyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" amount: 50}
    RequiredCandy: {candyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" amount: 25}
    RequiredCandy: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" amount: 50}
    RequiredFighterQuality: 2
  }
}
recepies:
{
  key: "Recipe_Rare_CaptainPatrickTheChef"
  value:
  {
    AuthoredId: "db8bf7c2-c569-8314-2a80-d9ffb6f1601b"
    AnimationId: "917e27ec-19ff-3d04-69f6-c596456816e6"
    Armor: {candyType: "9b70e722-0442-2924-092c-93efc1d92975" armorType: 18}
    Armor: {candyType: "9b70e722-0442-2924-092c-93efc1d92975" armorType: 10}
    Armor: {candyType: "9b70e722-0442-2924-092c-93efc1d92975" armorType: 9}
    Armor: {candyType: "9b70e722-0442-2924-092c-93efc1d92975" armorType: 8}
    Name: "Captain Patrick The Chef"
    Duration: 50
    FighterName: "Lord Patrick The Chef"
    FighterType: "a672e1da-a87d-2a54-4918-cafb72b02aec"
    Moves: "bbf71f26-78f5-5014-5b4b-a1c3866cb75b"
    Moves: "bbf71f26-78f5-5014-5b4b-a1c3866cb75b"
    Moves: "29d559bb-41a7-9f14-5bf8-605da8166943"
    Moves: "29d559bb-41a7-9f14-5bf8-605da8166943"
    Moves: "431900a2-54d4-df34-d9f7-6a4608fecfaa"
    Moves: "86bd4f81-e96b-9864-78b5-5fc51fbff6a4"
    Moves: "9f0df0ed-13a6-84c4-781d-9dbf9741c66e"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" amount: 50}
    RequiredCandy: {candyType: "33d499a1-9dee-dfd4-7843-c743a6131dcf" amount: 30}
    RequiredCandy: {candyType: "7ea89b0d-7070-d3e4-a86f-c0e75c0e4ff5" amount: 50}
    RequiredCandy: {candyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" amount: 30}
    RequiredCandy: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" amount: 42}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Rare_CommanderKrissyCocoCrunch"
  value:
  {
    AuthoredId: "2729a029-a53e-7b34-38c7-2c6ebe932c94"
    AnimationId: "02ddcd6e-e441-73f4-e874-399531dc4708"
    Armor: {candyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" armorType: 1}
    Armor: {candyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" armorType: 2}
    Armor: {candyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" armorType: 3}
    Armor: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" armorType: 11}
    Name: "Commander Krissy Coco Crunch"
    Duration: 50
    FighterName: "Lady Krissy Coco Crunch"
    FighterType: "85f361b8-e55d-0244-1a98-26bffa0a18a2"
    Moves: "aa7d8565-99e1-84f4-49d2-8925c2602a7b"
    Moves: "aa7d8565-99e1-84f4-49d2-8925c2602a7b"
    Moves: "9f0df0ed-13a6-84c4-781d-9dbf9741c66e"
    Moves: "9f0df0ed-13a6-84c4-781d-9dbf9741c66e"
    Moves: "9f0df0ed-13a6-84c4-781d-9dbf9741c66e"
    Moves: "1d1dec74-ca35-0cc4-d930-164786a9dd81"
    Moves: "1d1dec74-ca35-0cc4-d930-164786a9dd81"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "86b5fac0-4ec1-09e4-6ab7-96d3fd13dd04" amount: 50}
    RequiredCandy: {candyType: "28089a53-62ac-2044-1964-29f2f31d8a33" amount: 30}
    RequiredCandy: {candyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" amount: 50}
    RequiredCandy: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" amount: 30}
    RequiredCandy: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Rare_CommanderSuziTheGlazed"
  value:
  {
    AuthoredId: "175cd684-0079-c1b4-89b7-6bca7288f50d"
    AnimationId: "ed324300-aa2c-0c04-4b75-2d0eae3780f4"
    Armor: {candyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" armorType: 10}
    Armor: {candyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" armorType: 18}
    Armor: {candyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" armorType: 8}
    Armor: {candyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" armorType: 9}
    Name: "Commander Suzi the Glazed"
    Duration: 50
    FighterName: "Lady Suzi the Glazed"
    FighterType: "85f361b8-e55d-0244-1a98-26bffa0a18a2"
    Moves: "92f7b745-c1ff-3cd4-f9cf-0fe0dafb5536"
    Moves: "92f7b745-c1ff-3cd4-f9cf-0fe0dafb5536"
    Moves: "e16531a7-33fd-a3e4-69fd-ff10eba65cb7"
    Moves: "aa7d8565-99e1-84f4-49d2-8925c2602a7b"
    Moves: "fa6144f9-ad49-8124-386f-351a7f1ab546"
    Moves: "fa6144f9-ad49-8124-386f-351a7f1ab546"
    Moves: "c28e13b4-cb1f-6ea4-680f-91010a400e10"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" amount: 50}
    RequiredCandy: {candyType: "33d499a1-9dee-dfd4-7843-c743a6131dcf" amount: 30}
    RequiredCandy: {candyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" amount: 50}
    RequiredCandy: {candyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" amount: 30}
    RequiredCandy: {candyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Rare_CaptainAlexTheSweet"
  value:
  {
    AuthoredId: "34026945-900a-8d14-98b6-0af2354f22ac"
    AnimationId: "ed324300-aa2c-0c04-4b75-2d0eae3780f4"
    Armor: {candyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" armorType: 11}
    Armor: {candyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" armorType: 12}
    Armor: {candyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" armorType: 13}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 1}
    Name: "Captain Alex The Sweet"
    Duration: 50
    FighterName: "Lord Alex The Sweet"
    FighterType: "85f361b8-e55d-0244-1a98-26bffa0a18a2"
    Moves: "ad6d5d28-08b4-4304-6b49-11f8f5f9ff1c"
    Moves: "ad6d5d28-08b4-4304-6b49-11f8f5f9ff1c"
    Moves: "2569d967-c676-8b44-cbdf-d514e4c5c73d"
    Moves: "2569d967-c676-8b44-cbdf-d514e4c5c73d"
    Moves: "1d1dec74-ca35-0cc4-d930-164786a9dd81"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" amount: 50}
    RequiredCandy: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" amount: 30}
    RequiredCandy: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" amount: 50}
    RequiredCandy: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" amount: 30}
    RequiredCandy: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Epic_BazookaLee"
  value:
  {
    AuthoredId: "edc99c4e-6e95-5ae4-ea8c-e6071cd8635a"
    AnimationId: "7294e11a-f8b2-9d34-5854-fee4535f1aaa"
    Armor: {candyType: "16e0b307-2b40-4584-294c-dee505e47827" armorType: 1}
    Armor: {candyType: "16e0b307-2b40-4584-294c-dee505e47827" armorType: 2}
    Armor: {candyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" armorType: 3}
    Armor: {candyType: "bbeb0e31-3775-a864-1952-082233cf7b89" armorType: 4}
    Armor: {candyType: "bbeb0e31-3775-a864-1952-082233cf7b89" armorType: 5}
    Armor: {candyType: "bbeb0e31-3775-a864-1952-082233cf7b89" armorType: 6}
    Armor: {candyType: "bbeb0e31-3775-a864-1952-082233cf7b89" armorType: 7}
    Armor: {candyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" armorType: 8}
    Armor: {candyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" armorType: 9}
    Armor: {candyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" armorType: 10}
    Armor: {candyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" armorType: 18}
    Armor: {candyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" armorType: 11}
    Armor: {candyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" armorType: 12}
    Armor: {candyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" armorType: 13}
    Armor: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" armorType: 14}
    Armor: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" armorType: 15}
    Armor: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" armorType: 16}
    Armor: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" armorType: 17}
    Name: "Bazooka Lee"
    Duration: 100
    FighterName: "Bazooka Lee"
    FighterType: "85f361b8-e55d-0244-1a98-26bffa0a18a2"
    Moves: "1a67cc79-b97a-4624-ca67-36c9fcbee970"
    Moves: "1a67cc79-b97a-4624-ca67-36c9fcbee970"
    Moves: "ad6d5d28-08b4-4304-6b49-11f8f5f9ff1c"
    Moves: "2569d967-c676-8b44-cbdf-d514e4c5c73d"
    Moves: "e201f964-8949-9d74-fbee-4ee8786284ca"
    Moves: "86bd4f81-e96b-9864-78b5-5fc51fbff6a4"
    Moves: "9fd0b9ef-2e7d-d344-8926-69f1fab99f65"
    Moves: "628ebc0b-bb19-ee64-68b8-2f66f4b92720"
    Moves: "bd27fe1c-ce9a-f524-c887-61f9e4d5088b"
    Quality: 4
    IsAccountBound: false
    RequiredCandy: {candyType: "bbeb0e31-3775-a864-1952-082233cf7b89" amount: 50}
    RequiredCandy: {candyType: "a1af5095-bfbc-1a84-7862-9b246dc29d81" amount: 30}
    RequiredCandy: {candyType: "8932caea-da2e-6384-58ec-749624e213c9" amount: 25}
    RequiredCandy: {candyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" amount: 30}
    RequiredCandy: {candyType: "524c8f4f-cb50-ea04-dafd-e192f30c7656" amount: 75}
    RequiredCandy: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" amount: 50}
    RequiredCandy: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" amount: 25}
    RequiredCandy: {candyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" amount: 50}
    RequiredFighterQuality: 2
  }
}
recepies:
{
  key: "Recipe_Rare_CommanderSophiaTheSalted"
  value:
  {
    AuthoredId: "83acee4a-712c-b714-9887-36e6da3b8579"
    AnimationId: "a91a62fa-8d65-6234-d91b-a0ad0d9388e3"
    Armor: {candyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" armorType: 11}
    Armor: {candyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" armorType: 12}
    Armor: {candyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" armorType: 13}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 1}
    Name: "Commander Sophia The Salted"
    Duration: 50
    FighterName: "Lady Sophia The Salted"
    FighterType: "2522657f-c73f-23c4-da7d-3934d135265b"
    Moves: "41416afb-0d72-f9b4-ba30-f089b713500f"
    Moves: "41416afb-0d72-f9b4-ba30-f089b713500f"
    Moves: "431900a2-54d4-df34-d9f7-6a4608fecfaa"
    Moves: "431900a2-54d4-df34-d9f7-6a4608fecfaa"
    Moves: "0090580c-04ef-9d84-e883-32f52c977b98"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "9c64b7e4-0458-65c4-9b79-5cc35e847f9c" amount: 50}
    RequiredCandy: {candyType: "28089a53-62ac-2044-1964-29f2f31d8a33" amount: 30}
    RequiredCandy: {candyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" amount: 50}
    RequiredCandy: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" amount: 30}
    RequiredCandy: {candyType: "524c8f4f-cb50-ea04-dafd-e192f30c7656" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Common_FirstRecipe"
  value:
  {
    AuthoredId: "5864a19b-c8c0-2d34-eaef-9455af0baf2c"
    AnimationId: "938e1ab9-a85b-5444-7bd6-f8862ace8b16"
    Armor: {candyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" armorType: 8}
    Armor: {candyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" armorType: 9}
    Name: "First Recipe"
    Duration: 1
    FighterName: "Sour Gummi Brawler"
    FighterType: "c160f7ad-c775-8614-abe2-8ef74e54401f"
    Moves: "86b323c2-b2fd-2494-ab5e-bc3514bc92d8"
    Moves: "0090580c-04ef-9d84-e883-32f52c977b98"
    Quality: 1
    IsAccountBound: true
    RequiredCandy: {candyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" amount: 1}
    RequiredCandy: {candyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" amount: 1}
    RequiredFighterQuality: 0
  }
}
recepies:
{
  key: "Recipe_Rare_CommanderDonnaTheChocolatey"
  value:
  {
    AuthoredId: "fb87ff21-f4d6-d564-2956-e250d2cb2409"
    AnimationId: "64732638-e355-d5c4-aa2d-e28c3db4aa72"
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 2}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 3}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 4}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 5}
    Name: "Commander Donna The Chocolatey"
    Duration: 50
    FighterName: "Lady Donna The Chocolatey"
    FighterType: "a672e1da-a87d-2a54-4918-cafb72b02aec"
    Moves: "41416afb-0d72-f9b4-ba30-f089b713500f"
    Moves: "41416afb-0d72-f9b4-ba30-f089b713500f"
    Moves: "431900a2-54d4-df34-d9f7-6a4608fecfaa"
    Moves: "431900a2-54d4-df34-d9f7-6a4608fecfaa"
    Moves: "0090580c-04ef-9d84-e883-32f52c977b98"
    Moves: "86b323c2-b2fd-2494-ab5e-bc3514bc92d8"
    Moves: "9fd0b9ef-2e7d-d344-8926-69f1fab99f65"
    Quality: 3
    IsAccountBound: false
    RequiredCandy: {candyType: "86b5fac0-4ec1-09e4-6ab7-96d3fd13dd04" amount: 50}
    RequiredCandy: {candyType: "28089a53-62ac-2044-1964-29f2f31d8a33" amount: 30}
    RequiredCandy: {candyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" amount: 50}
    RequiredCandy: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" amount: 30}
    RequiredCandy: {candyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" amount: 40}
    RequiredFighterQuality: 1
  }
}
recepies:
{
  key: "Recipe_Epic_AbbaCazaba"
  value:
  {
    AuthoredId: "f13487ec-4bf1-7ec4-e9d6-fd043f9842d8"
    AnimationId: "4fca9718-5e62-8924-7a58-790c249fbc7f"
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 1}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 2}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 3}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 4}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 5}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 6}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 7}
    Armor: {candyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" armorType: 8}
    Armor: {candyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" armorType: 9}
    Armor: {candyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" armorType: 10}
    Armor: {candyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" armorType: 18}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 11}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 12}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 13}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 14}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 15}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 16}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 17}
    Name: "Abba Cazaba"
    Duration: 100
    FighterName: "Abba Cazaba"
    FighterType: "a672e1da-a87d-2a54-4918-cafb72b02aec"
    Moves: "bbf71f26-78f5-5014-5b4b-a1c3866cb75b"
    Moves: "bbf71f26-78f5-5014-5b4b-a1c3866cb75b"
    Moves: "e16531a7-33fd-a3e4-69fd-ff10eba65cb7"
    Moves: "aa7d8565-99e1-84f4-49d2-8925c2602a7b"
    Moves: "b96f1ff8-66d5-f244-79fd-260735d4c3d7"
    Moves: "b1bd32de-88b2-1204-7b2e-0680677d78c4"
    Moves: "86bd4f81-e96b-9864-78b5-5fc51fbff6a4"
    Moves: "29d559bb-41a7-9f14-5bf8-605da8166943"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Quality: 4
    IsAccountBound: false
    RequiredCandy: {candyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" amount: 50}
    RequiredCandy: {candyType: "48ea4c49-79dd-6514-6b8b-f87fd6d8a330" amount: 30}
    RequiredCandy: {candyType: "a2c2697c-d738-c004-4b32-d800921bab06" amount: 25}
    RequiredCandy: {candyType: "1525c955-d3c0-9f84-69fb-cf98837f4f85" amount: 30}
    RequiredCandy: {candyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" amount: 75}
    RequiredCandy: {candyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" amount: 50}
    RequiredCandy: {candyType: "9b70e722-0442-2924-092c-93efc1d92975" amount: 25}
    RequiredCandy: {candyType: "8aac4645-29cf-fe04-59c0-415508b0432e" amount: 50}
    RequiredFighterQuality: 2
  }
}
recepies:
{
  key: "Recipe_Epic_MuhammadCandi"
  value:
  {
    AuthoredId: "b5bb8cce-20bf-f5a4-3a51-694c37b8f9b9"
    AnimationId: "917e27ec-19ff-3d04-69f6-c596456816e6"
    Armor: {candyType: "f6c3b895-6488-1b74-a9b0-664027a683e2" armorType: 1}
    Armor: {candyType: "f6c3b895-6488-1b74-a9b0-664027a683e2" armorType: 2}
    Armor: {candyType: "f6c3b895-6488-1b74-a9b0-664027a683e2" armorType: 3}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 4}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 5}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 6}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 7}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 8}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 9}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 10}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 18}
    Armor: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" armorType: 11}
    Armor: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" armorType: 12}
    Armor: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" armorType: 13}
    Armor: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" armorType: 14}
    Armor: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" armorType: 15}
    Armor: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" armorType: 16}
    Armor: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" armorType: 17}
    Name: "Muhammad Candi"
    Duration: 100
    FighterName: "Muhammad Candi"
    FighterType: "2522657f-c73f-23c4-da7d-3934d135265b"
    Moves: "92f7b745-c1ff-3cd4-f9cf-0fe0dafb5536"
    Moves: "92f7b745-c1ff-3cd4-f9cf-0fe0dafb5536"
    Moves: "e16531a7-33fd-a3e4-69fd-ff10eba65cb7"
    Moves: "aa7d8565-99e1-84f4-49d2-8925c2602a7b"
    Moves: "fa6144f9-ad49-8124-386f-351a7f1ab546"
    Moves: "fa6144f9-ad49-8124-386f-351a7f1ab546"
    Moves: "c28e13b4-cb1f-6ea4-680f-91010a400e10"
    Moves: "86b323c2-b2fd-2494-ab5e-bc3514bc92d8"
    Moves: "01b34a91-37d8-29d4-09cb-2d515f51e315"
    Quality: 4
    IsAccountBound: false
    RequiredCandy: {candyType: "f6c3b895-6488-1b74-a9b0-664027a683e2" amount: 50}
    RequiredCandy: {candyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" amount: 30}
    RequiredCandy: {candyType: "aa0cb18d-5681-5504-09d7-4db06bf7a4d8" amount: 25}
    RequiredCandy: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" amount: 30}
    RequiredCandy: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" amount: 75}
    RequiredCandy: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" amount: 50}
    RequiredCandy: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" amount: 25}
    RequiredCandy: {candyType: "934dae05-689f-30e4-5804-d497aee0b47c" amount: 50}
    RequiredFighterQuality: 2
  }
}
recepies:
{
  key: "Recipe_Epic_CoffyCoffee"
  value:
  {
    AuthoredId: "7c922dce-d72d-2544-fbc3-7abb2a12c92e"
    AnimationId: "0c2cdea6-ff22-b124-da21-882987b1707a"
    Armor: {candyType: "321c2a65-64b1-8344-2915-85562e2ad7ec" armorType: 1}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 2}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 3}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 4}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 5}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 6}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 7}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 8}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 9}
    Armor: {candyType: "48ea4c49-79dd-6514-6b8b-f87fd6d8a330" armorType: 10}
    Armor: {candyType: "aa0cb18d-5681-5504-09d7-4db06bf7a4d8" armorType: 18}
    Armor: {candyType: "8f896243-a6de-1b54-68a3-0df72e8688e4" armorType: 11}
    Armor: {candyType: "8f896243-a6de-1b54-68a3-0df72e8688e4" armorType: 12}
    Armor: {candyType: "8f896243-a6de-1b54-68a3-0df72e8688e4" armorType: 13}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 14}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 15}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 16}
    Armor: {candyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" armorType: 17}
    Name: "Coffy Coffee"
    Duration: 100
    FighterName: "Coffy Coffee"
    FighterType: "85f361b8-e55d-0244-1a98-26bffa0a18a2"
    Moves: "55cafe03-4a58-0f44-6a27-f9fc6fce881a"
    Moves: "55cafe03-4a58-0f44-6a27-f9fc6fce881a"
    Moves: "2c94f949-a10e-2524-2827-6588982ce0b1"
    Moves: "41416afb-0d72-f9b4-ba30-f089b713500f"
    Moves: "431900a2-54d4-df34-d9f7-6a4608fecfaa"
    Moves: "9f0df0ed-13a6-84c4-781d-9dbf9741c66e"
    Moves: "86b323c2-b2fd-2494-ab5e-bc3514bc92d8"
    Moves: "9fd0b9ef-2e7d-d344-8926-69f1fab99f65"
    Moves: "2569d967-c676-8b44-cbdf-d514e4c5c73d"
    Quality: 4
    IsAccountBound: false
    RequiredCandy: {candyType: "8f896243-a6de-1b54-68a3-0df72e8688e4" amount: 50}
    RequiredCandy: {candyType: "8898194e-ac52-f624-a830-176655bb43cd" amount: 30}
    RequiredCandy: {candyType: "f7668430-bfc1-71f4-29b8-f36c7d6905d5" amount: 25}
    RequiredCandy: {candyType: "28089a53-62ac-2044-1964-29f2f31d8a33" amount: 30}
    RequiredCandy: {candyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" amount: 75}
    RequiredCandy: {candyType: "9b70e722-0442-2924-092c-93efc1d92975" amount: 50}
    RequiredCandy: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" amount: 25}
    RequiredCandy: {candyType: "8aac4645-29cf-fe04-59c0-415508b0432e" amount: 50}
    RequiredFighterQuality: 2
  }
}
recepies:
{
  key: "Recipe_Common_SecondRecipe"
  value:
  {
    AuthoredId: "ba0121ba-e8a6-7e64-9bc1-71dfeca27daa"
    AnimationId: "97c2c3da-93c7-a0d4-1967-eeba13396cce"
    Armor: {candyType: "16e0b307-2b40-4584-294c-dee505e47827" armorType: 1}
    Armor: {candyType: "4e343f68-1aaa-0e84-7bab-eae458b68264" armorType: 11}
    Name: "Second Recipe"
    Duration: 1
    FighterName: "Caramel Kicker"
    FighterType: "85f361b8-e55d-0244-1a98-26bffa0a18a2"
    Moves: "2c555752-8a84-58f4-395e-6460b7864069"
    Moves: "0090580c-04ef-9d84-e883-32f52c977b98"
    Quality: 1
    IsAccountBound: true
    RequiredCandy: {candyType: "4e343f68-1aaa-0e84-7bab-eae458b68264" amount: 1}
    RequiredCandy: {candyType: "16e0b307-2b40-4584-294c-dee505e47827" amount: 1}
    RequiredFighterQuality: 0
  }
}
recepies:
{
  key: "Recipe_Epic_MieshaTaste"
  value:
  {
    AuthoredId: "5124cf4a-b08e-2dd4-6809-ed15f73ae685"
    AnimationId: "d35a9d1c-2223-c404-c840-c0218f9b147d"
    Armor: {candyType: "499a8021-eaea-c7d4-aa1c-bb6e55cd300c" armorType: 1}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 2}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 3}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 4}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 5}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 6}
    Armor: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" armorType: 7}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 8}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 9}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 10}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 18}
    Armor: {candyType: "eeeca33e-5c80-9ec4-eb83-46e4b7daf04b" armorType: 11}
    Armor: {candyType: "eeeca33e-5c80-9ec4-eb83-46e4b7daf04b" armorType: 12}
    Armor: {candyType: "eeeca33e-5c80-9ec4-eb83-46e4b7daf04b" armorType: 13}
    Armor: {candyType: "08847311-c629-a154-5958-13e8f1d77f88" armorType: 14}
    Armor: {candyType: "08847311-c629-a154-5958-13e8f1d77f88" armorType: 15}
    Armor: {candyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" armorType: 16}
    Armor: {candyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" armorType: 17}
    Name: "Miesha Taste"
    Duration: 100
    FighterName: "Miesha Taste"
    FighterType: "4c2d03be-70bf-9294-c8d5-0a8e2d8d0677"
    Moves: "55e05ece-a3f6-a924-3833-2222a0eee6a3"
    Moves: "55e05ece-a3f6-a924-3833-2222a0eee6a3"
    Moves: "86bd4f81-e96b-9864-78b5-5fc51fbff6a4"
    Moves: "e201f964-8949-9d74-fbee-4ee8786284ca"
    Moves: "c6770789-4a1f-7f24-f921-c4dc66c2697a"
    Moves: "29d559bb-41a7-9f14-5bf8-605da8166943"
    Moves: "b1bd32de-88b2-1204-7b2e-0680677d78c4"
    Moves: "41416afb-0d72-f9b4-ba30-f089b713500f"
    Moves: "29d559bb-41a7-9f14-5bf8-605da8166943"
    Quality: 4
    IsAccountBound: false
    RequiredCandy: {candyType: "eeeca33e-5c80-9ec4-eb83-46e4b7daf04b" amount: 50}
    RequiredCandy: {candyType: "8932caea-da2e-6384-58ec-749624e213c9" amount: 30}
    RequiredCandy: {candyType: "b4e362bb-2634-7df4-39a6-71b358368a6d" amount: 25}
    RequiredCandy: {candyType: "9ff884ab-efcb-0af4-5acd-976fea43b3a3" amount: 30}
    RequiredCandy: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" amount: 75}
    RequiredCandy: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" amount: 50}
    RequiredCandy: {candyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" amount: 25}
    RequiredCandy: {candyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" amount: 50}
    RequiredFighterQuality: 2
  }
}
recepies:
{
  key: "Recipe_Epic_ZenobiaNabbout"
  value:
  {
    AuthoredId: "df921272-743b-5f14-1a81-710a1b97d72e"
    AnimationId: "471eee2f-e6ad-a694-58de-bdee5a379096"
    Armor: {candyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" armorType: 1}
    Armor: {candyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" armorType: 2}
    Armor: {candyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" armorType: 3}
    Armor: {candyType: "8932caea-da2e-6384-58ec-749624e213c9" armorType: 4}
    Armor: {candyType: "8932caea-da2e-6384-58ec-749624e213c9" armorType: 5}
    Armor: {candyType: "8932caea-da2e-6384-58ec-749624e213c9" armorType: 6}
    Armor: {candyType: "8932caea-da2e-6384-58ec-749624e213c9" armorType: 7}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 8}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 9}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 10}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 18}
    Armor: {candyType: "8f896243-a6de-1b54-68a3-0df72e8688e4" armorType: 11}
    Armor: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" armorType: 12}
    Armor: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" armorType: 13}
    Armor: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" armorType: 14}
    Armor: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" armorType: 15}
    Armor: {candyType: "a2c2697c-d738-c004-4b32-d800921bab06" armorType: 16}
    Armor: {candyType: "a2c2697c-d738-c004-4b32-d800921bab06" armorType: 17}
    Name: "Zenobia Nabbout"
    Duration: 100
    FighterName: "Zenobia Nabbout"
    FighterType: "85f361b8-e55d-0244-1a98-26bffa0a18a2"
    Moves: "92f7b745-c1ff-3cd4-f9cf-0fe0dafb5536"
    Moves: "92f7b745-c1ff-3cd4-f9cf-0fe0dafb5536"
    Moves: "e16531a7-33fd-a3e4-69fd-ff10eba65cb7"
    Moves: "aa7d8565-99e1-84f4-49d2-8925c2602a7b"
    Moves: "fa6144f9-ad49-8124-386f-351a7f1ab546"
    Moves: "fa6144f9-ad49-8124-386f-351a7f1ab546"
    Moves: "c28e13b4-cb1f-6ea4-680f-91010a400e10"
    Moves: "86b323c2-b2fd-2494-ab5e-bc3514bc92d8"
    Moves: "01b34a91-37d8-29d4-09cb-2d515f51e315"
    Quality: 4
    IsAccountBound: false
    RequiredCandy: {candyType: "f6c3b895-6488-1b74-a9b0-664027a683e2" amount: 50}
    RequiredCandy: {candyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" amount: 30}
    RequiredCandy: {candyType: "aa0cb18d-5681-5504-09d7-4db06bf7a4d8" amount: 25}
    RequiredCandy: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" amount: 30}
    RequiredCandy: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" amount: 75}
    RequiredCandy: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" amount: 50}
    RequiredCandy: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" amount: 25}
    RequiredCandy: {candyType: "934dae05-689f-30e4-5804-d497aee0b47c" amount: 50}
    RequiredFighterQuality: 2
  }
}
recepies:
{
  key: "Recipe_Epic_PockyMarciano"
  value:
  {
    AuthoredId: "cf363926-b8c5-8294-099a-5fd770749258"
    AnimationId: "a91a62fa-8d65-6234-d91b-a0ad0d9388e3"
    Armor: {candyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" armorType: 1}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 2}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 3}
    Armor: {candyType: "52ea4044-e203-16d4-c90f-8b685ed5e9c9" armorType: 4}
    Armor: {candyType: "52ea4044-e203-16d4-c90f-8b685ed5e9c9" armorType: 5}
    Armor: {candyType: "52ea4044-e203-16d4-c90f-8b685ed5e9c9" armorType: 6}
    Armor: {candyType: "52ea4044-e203-16d4-c90f-8b685ed5e9c9" armorType: 7}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 8}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 9}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 10}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 18}
    Armor: {candyType: "2f765969-c607-d4a4-0b9c-72d3cce7fbec" armorType: 11}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 12}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 13}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 14}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 15}
    Armor: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" armorType: 16}
    Armor: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" armorType: 17}
    Name: "Pocky Marciano"
    Duration: 100
    FighterName: "Pocky Marciano"
    FighterType: "c160f7ad-c775-8614-abe2-8ef74e54401f"
    Moves: "01b34a91-37d8-29d4-09cb-2d515f51e315"
    Moves: "01b34a91-37d8-29d4-09cb-2d515f51e315"
    Moves: "0090580c-04ef-9d84-e883-32f52c977b98"
    Moves: "8e4753b5-c739-aaf4-ab79-4d333a69cae6"
    Moves: "431900a2-54d4-df34-d9f7-6a4608fecfaa"
    Moves: "9fd0b9ef-2e7d-d344-8926-69f1fab99f65"
    Moves: "86b323c2-b2fd-2494-ab5e-bc3514bc92d8"
    Moves: "bd27fe1c-ce9a-f524-c887-61f9e4d5088b"
    Moves: "29d559bb-41a7-9f14-5bf8-605da8166943"
    Quality: 4
    IsAccountBound: false
    RequiredCandy: {candyType: "52ea4044-e203-16d4-c90f-8b685ed5e9c9" amount: 50}
    RequiredCandy: {candyType: "bbeb0e31-3775-a864-1952-082233cf7b89" amount: 30}
    RequiredCandy: {candyType: "a2c2697c-d738-c004-4b32-d800921bab06" amount: 25}
    RequiredCandy: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" amount: 30}
    RequiredCandy: {candyType: "87040ddf-f0cb-f894-dbbd-a4f266cae4e7" amount: 75}
    RequiredCandy: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" amount: 50}
    RequiredCandy: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" amount: 25}
    RequiredCandy: {candyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" amount: 50}
    RequiredFighterQuality: 2
  }
}
recepies:
{
  key: "Recipe_Epic_QueenBoudiccaNofork"
  value:
  {
    AuthoredId: "71a69f03-1e74-0ed4-6994-54b900eb0cc9"
    AnimationId: "3afdf3ed-bea4-1c14-faac-159412578c0f"
    Armor: {candyType: "f6c3b895-6488-1b74-a9b0-664027a683e2" armorType: 1}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 2}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 3}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 4}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 5}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 6}
    Armor: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" armorType: 7}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 8}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 9}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 10}
    Armor: {candyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" armorType: 18}
    Armor: {candyType: "2f765969-c607-d4a4-0b9c-72d3cce7fbec" armorType: 11}
    Armor: {candyType: "2f765969-c607-d4a4-0b9c-72d3cce7fbec" armorType: 12}
    Armor: {candyType: "2f765969-c607-d4a4-0b9c-72d3cce7fbec" armorType: 13}
    Armor: {candyType: "47b8a853-791a-1424-1ba9-4adaf4a3f4a5" armorType: 14}
    Armor: {candyType: "47b8a853-791a-1424-1ba9-4adaf4a3f4a5" armorType: 15}
    Armor: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" armorType: 16}
    Armor: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" armorType: 17}
    Name: "Queen Boudicca Nofork"
    Duration: 100
    FighterName: "Queen Boudicca Nofork"
    FighterType: "85f361b8-e55d-0244-1a98-26bffa0a18a2"
    Moves: "3e76f2fa-072c-5654-dac9-c9b0e4e1643a"
    Moves: "3e76f2fa-072c-5654-dac9-c9b0e4e1643a"
    Moves: "1d1dec74-ca35-0cc4-d930-164786a9dd81"
    Moves: "1e47a8e0-acfd-1244-0b7d-539d62429060"
    Moves: "b889835e-1cd6-d3d4-f9b4-dd1989f49dac"
    Moves: "b96f1ff8-66d5-f244-79fd-260735d4c3d7"
    Moves: "2c555752-8a84-58f4-395e-6460b7864069"
    Moves: "a9d69fc0-a1b8-ae24-cbd7-249760200211"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Quality: 4
    IsAccountBound: false
    RequiredCandy: {candyType: "2f765969-c607-d4a4-0b9c-72d3cce7fbec" amount: 50}
    RequiredCandy: {candyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" amount: 30}
    RequiredCandy: {candyType: "a2c2697c-d738-c004-4b32-d800921bab06" amount: 25}
    RequiredCandy: {candyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" amount: 30}
    RequiredCandy: {candyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" amount: 75}
    RequiredCandy: {candyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" amount: 50}
    RequiredCandy: {candyType: "63a8ea74-65ee-c024-b8f0-87e173258257" amount: 25}
    RequiredCandy: {candyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" amount: 50}
    RequiredFighterQuality: 2
  }
}
recepies:
{
  key: "Recipe_Uncommon_ThirdRecipe"
  value:
  {
    AuthoredId: "1bbc7d99-7fce-24a4-c9a3-dfaf4b744efa"
    AnimationId: "ed324300-aa2c-0c04-4b75-2d0eae3780f4"
    Armor: {candyType: "8aac4645-29cf-fe04-59c0-415508b0432e" armorType: 16}
    Armor: {candyType: "8aac4645-29cf-fe04-59c0-415508b0432e" armorType: 17}
    Armor: {candyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" armorType: 10}
    Name: "Chocolate Cutie Pie"
    Duration: 5
    FighterName: "Chocolate Cutie Pie"
    FighterType: "a672e1da-a87d-2a54-4918-cafb72b02aec"
    Moves: "a9d69fc0-a1b8-ae24-cbd7-249760200211"
    Moves: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Moves: "1e47a8e0-acfd-1244-0b7d-539d62429060"
    Moves: "c28e13b4-cb1f-6ea4-680f-91010a400e10"
    Quality: 2
    IsAccountBound: false
    RequiredCandy: {candyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" amount: 20}
    RequiredCandy: {candyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" amount: 25}
    RequiredCandy: {candyType: "8aac4645-29cf-fe04-59c0-415508b0432e" amount: 15}
    RequiredCandy: {candyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" amount: 20}
    RequiredFighterQuality: 0
  }
}
activityrewards:
{
  key: "RewardsTable_1stExpedition"
  value:
  {
    AuthoredId: "c1f4ec37-bb53-fed4-09e0-e19bb5b346bb"
    Name: "Rewards Table 1st Expedition"
    Rewards: {ListTableId: "e72d7dc0-2b81-d114-3803-652de993041f" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Rare_GummiBear_Legs"
  value:
  {
    AuthoredId: "50379c2b-2422-8104-ea69-bb2882e9cac0"
    Name: "SweetenerReward_Armor_Rare_GummiBear_Legs"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 17 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "af509c01-933e-ce14-49c3-106081e16d75" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 15 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "af509c01-933e-ce14-49c3-106081e16d75" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 14 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "af509c01-933e-ce14-49c3-106081e16d75" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 16 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "af509c01-933e-ce14-49c3-106081e16d75" Quantity: 1 Weight: 25}
  }
}
activityrewards:
{
  key: "SweetenerRewardsTable_Animations_Uncommon"
  value:
  {
    AuthoredId: "808db761-05dc-a2f4-6a31-210551e2cb5a"
    Name: "SweetenerRewardsTable_Animations_Uncommon"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "27448f34-ca08-99b4-1b63-fd4b227e6af4" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "938e1ab9-a85b-5444-7bd6-f8862ace8b16" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "05633498-ace9-de14-c939-9435a6343d0f" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "5fce85e9-51f9-d7b4-db81-ef141439db00" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "917e27ec-19ff-3d04-69f6-c596456816e6" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "97c2c3da-93c7-a0d4-1967-eeba13396cce" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "64732638-e355-d5c4-aa2d-e28c3db4aa72" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "a170f9f3-f97b-8eb4-49c0-c99a916e55d7" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "f33c74e6-f1b4-6ca4-bb5a-cf6edc77cab5" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "a2a185e5-4543-6814-d88d-d078a0e38521" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "a91a62fa-8d65-6234-d91b-a0ad0d9388e3" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "ed324300-aa2c-0c04-4b75-2d0eae3780f4" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "d35a9d1c-2223-c404-c840-c0218f9b147d" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 7 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
  }
}
activityrewards:
{
  key: "RewardsTable_1stTourneyWinner"
  value:
  {
    AuthoredId: "4c95fe23-e5e4-15d4-2a99-dbac2f3daeed"
    Name: "RewardsTable_1stTourneyWinner"
    Rewards: {ListTableId: "d8710632-c648-8814-9bc6-89bfcda03385" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Uncommon_CottonCandy_Torso"
  value:
  {
    AuthoredId: "b96d908f-3a89-1564-5815-d88b8edf4602"
    Name: "SweetenerReward_Armor_Uncommon_CottonCandy_Torso"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 18 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 9 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 8 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 10 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" Quantity: 1 Weight: 100}
  }
}
activityrewards:
{
  key: "RewardsTable_Tournament_T3Rare1"
  value:
  {
    AuthoredId: "d73e36c9-4533-0ff4-cb72-2137ae467bd8"
    Name: "RewardsTable_Tournament_T3Rare1"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4e343f68-1aaa-0e84-7bab-eae458b68264" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "16e0b307-2b40-4584-294c-dee505e47827" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "87040ddf-f0cb-f894-dbbd-a4f266cae4e7" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "524c8f4f-cb50-ea04-dafd-e192f30c7656" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8aac4645-29cf-fe04-59c0-415508b0432e" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "934dae05-689f-30e4-5804-d497aee0b47c" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9b70e722-0442-2924-092c-93efc1d92975" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "63a8ea74-65ee-c024-b8f0-87e173258257" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ea89b0d-7070-d3e4-a86f-c0e75c0e4ff5" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9ff884ab-efcb-0af4-5acd-976fea43b3a3" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "33d499a1-9dee-dfd4-7843-c743a6131dcf" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "28089a53-62ac-2044-1964-29f2f31d8a33" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 3 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "8f67d790-fc5b-0604-ab52-6292a829a6fc" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "ef33f366-41a4-5924-19bf-af4ea20ca7df" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "175cd684-0079-c1b4-89b7-6bca7288f50d" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "83acee4a-712c-b714-9887-36e6da3b8579" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "ba49d448-4c02-1fe4-4b20-a43f623e0d8f" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "1082e384-a607-f834-9ab3-d089c215d31a" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "6d88afa9-a489-d6b4-b84c-5ae9b5762055" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "2729a029-a53e-7b34-38c7-2c6ebe932c94" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "63da609a-1938-3524-08ac-879fea5cce55" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "9c89ab1e-b043-6134-a86f-89a986e4e8c0" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "83bdccb0-abc7-3264-8994-4d1700458a07" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "80ede00d-10fd-10c4-baaa-045acaeedc22" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "fae112f6-dba3-e154-78bf-154ec2f3f94b" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
  }
}
activityrewards:
{
  key: "RewardsTable_Tournament_T3Rare2"
  value:
  {
    AuthoredId: "6ca739e1-18c3-9b04-4bb6-cb38d82462ea"
    Name: "RewardsTable_Tournament_T3Rare2"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4e343f68-1aaa-0e84-7bab-eae458b68264" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "16e0b307-2b40-4584-294c-dee505e47827" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "87040ddf-f0cb-f894-dbbd-a4f266cae4e7" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "524c8f4f-cb50-ea04-dafd-e192f30c7656" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8aac4645-29cf-fe04-59c0-415508b0432e" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "934dae05-689f-30e4-5804-d497aee0b47c" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9b70e722-0442-2924-092c-93efc1d92975" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "63a8ea74-65ee-c024-b8f0-87e173258257" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ea89b0d-7070-d3e4-a86f-c0e75c0e4ff5" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "af509c01-933e-ce14-49c3-106081e16d75" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9c64b7e4-0458-65c4-9b79-5cc35e847f9c" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "86b5fac0-4ec1-09e4-6ab7-96d3fd13dd04" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "33d499a1-9dee-dfd4-7843-c743a6131dcf" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "28089a53-62ac-2044-1964-29f2f31d8a33" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 3 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "c6cabeb5-60f6-7c34-fb09-c7e24971ff3b" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "223de3eb-e192-0854-d800-6f9f2fa8e28b" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "275d1b18-22e6-fcf4-2919-b28bc1da9060" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "4b90dbd5-e6d3-69c4-8b8d-1327bf8381af" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "fda69e34-c1cb-2664-4ba1-d943713218c5" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "8c4b6099-b736-16a4-eb8b-25eaaac1a0c3" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "dfd860da-2127-60e4-8b46-39bfffb57245" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "7938f8c7-2d20-19c4-9be9-b05fd663f44f" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "db8bf7c2-c569-8314-2a80-d9ffb6f1601b" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "a889e1d9-c337-dff4-3aae-7701b6e48aeb" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "99f7eaf3-67c6-30d4-5a3d-4a2a18bb75f7" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "4aba4145-0370-3484-1b2b-d416df6138ee" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "7b7d8898-7f58-0334-0bad-825dc87a6638" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "60f3f548-20ca-7a64-6ad3-054d654e24a2" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
  }
}
activityrewards:
{
  key: "RewardsTable_Tournament_T3Rare3"
  value:
  {
    AuthoredId: "6c280e9c-2326-ce44-895c-b350dd01543a"
    Name: "RewardsTable_Tournament_T3Rare3"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4e343f68-1aaa-0e84-7bab-eae458b68264" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "16e0b307-2b40-4584-294c-dee505e47827" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "87040ddf-f0cb-f894-dbbd-a4f266cae4e7" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "524c8f4f-cb50-ea04-dafd-e192f30c7656" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8aac4645-29cf-fe04-59c0-415508b0432e" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "934dae05-689f-30e4-5804-d497aee0b47c" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9b70e722-0442-2924-092c-93efc1d92975" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "63a8ea74-65ee-c024-b8f0-87e173258257" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ea89b0d-7070-d3e4-a86f-c0e75c0e4ff5" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9c64b7e4-0458-65c4-9b79-5cc35e847f9c" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "1525c955-d3c0-9f84-69fb-cf98837f4f85" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "86b5fac0-4ec1-09e4-6ab7-96d3fd13dd04" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 3 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "c229b9d2-8e0f-8644-7b09-1b6ba6247026" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "502693b2-3dd5-a204-197d-39031869f194" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "28e2a7ea-e606-37d4-4b77-33069ba87bc8" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "fe9ec873-e4b8-0714-2be0-a3c2c547f9ad" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "c28bd52c-abf1-eff4-da81-6a47f7bd79a9" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "8bc478cc-d85e-2d84-28f3-1781f582319f" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "27f47d35-c2d3-c004-9a36-5253005559c2" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "b68f5a1f-8bfd-36d4-6ac4-f9ccc5f3acf6" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "db1606dd-112d-7a34-db0b-8923900c8ac7" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "34026945-900a-8d14-98b6-0af2354f22ac" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "fb87ff21-f4d6-d564-2956-e250d2cb2409" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "9a2c04f2-cd9c-e484-daca-d4e4a6b5b9b8" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "8f8425e7-cab0-0b14-1874-5072afe7d0b5" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
  }
}
activityrewards:
{
  key: "RewardsTable_Tournament_T2Uncommon1"
  value:
  {
    AuthoredId: "f222e20f-3157-f314-6803-692d4ea1c9f2"
    Name: "RewardsTable_Tournament_T2Uncommon1"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4e343f68-1aaa-0e84-7bab-eae458b68264" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "16e0b307-2b40-4584-294c-dee505e47827" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "87040ddf-f0cb-f894-dbbd-a4f266cae4e7" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "524c8f4f-cb50-ea04-dafd-e192f30c7656" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8aac4645-29cf-fe04-59c0-415508b0432e" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "934dae05-689f-30e4-5804-d497aee0b47c" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9b70e722-0442-2924-092c-93efc1d92975" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "63a8ea74-65ee-c024-b8f0-87e173258257" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ea89b0d-7070-d3e4-a86f-c0e75c0e4ff5" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 20}
  }
}
activityrewards:
{
  key: "RewardsTable_Tournament_T2Uncommon2"
  value:
  {
    AuthoredId: "facf013d-6328-82a4-8b61-32db1c3c5282"
    Name: "RewardsTable_Tournament_T2Uncommon2"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4e343f68-1aaa-0e84-7bab-eae458b68264" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "16e0b307-2b40-4584-294c-dee505e47827" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "87040ddf-f0cb-f894-dbbd-a4f266cae4e7" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "524c8f4f-cb50-ea04-dafd-e192f30c7656" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8aac4645-29cf-fe04-59c0-415508b0432e" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "934dae05-689f-30e4-5804-d497aee0b47c" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 20}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Epic_Bronze_Torso"
  value:
  {
    AuthoredId: "128d71f0-60c9-4eb4-2b6e-02b31d973959"
    Name: "SweetenerReward_Armor_Epic_Bronze_Torso"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 18 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "48ea4c49-79dd-6514-6b8b-f87fd6d8a330" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 10 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "48ea4c49-79dd-6514-6b8b-f87fd6d8a330" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 9 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "48ea4c49-79dd-6514-6b8b-f87fd6d8a330" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 8 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "48ea4c49-79dd-6514-6b8b-f87fd6d8a330" Quantity: 1 Weight: 25}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Epic_Gold_Torso"
  value:
  {
    AuthoredId: "f0d10fe6-0161-55e4-7973-f49726d5fdae"
    Name: "SweetenerReward_Armor_Epic_Gold_Torso"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 18 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8898194e-ac52-f624-a830-176655bb43cd" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 10 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8898194e-ac52-f624-a830-176655bb43cd" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 9 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8898194e-ac52-f624-a830-176655bb43cd" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 8 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8898194e-ac52-f624-a830-176655bb43cd" Quantity: 1 Weight: 25}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Rare_Jawbreaker_Bust"
  value:
  {
    AuthoredId: "69fe65f1-2d6a-82b4-5ada-5c48536e357a"
    Name: "SweetenerReward_Armor_Rare_Jawbreaker_Bust"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 1 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 3 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 2 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" Quantity: 1 Weight: 100}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Epic_Silver_Arms"
  value:
  {
    AuthoredId: "26a149ff-3a0f-1b54-9ac6-f5c30b44912f"
    Name: "SweetenerReward_Armor_Epic_Silver_Arms"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 4 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "bbeb0e31-3775-a864-1952-082233cf7b89" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 5 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "bbeb0e31-3775-a864-1952-082233cf7b89" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 6 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "bbeb0e31-3775-a864-1952-082233cf7b89" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 7 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "bbeb0e31-3775-a864-1952-082233cf7b89" Quantity: 1 Weight: 25}
  }
}
activityrewards:
{
  key: "GenericRewardTable_NUll"
  value:
  {
    AuthoredId: "1290674d-4fa7-07c4-3987-14262876f209"
    Name: "null table"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 7 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Uncommon_Banana_Arms"
  value:
  {
    AuthoredId: "2436f49f-2eb5-23e4-cbae-c0985d81c6cf"
    Name: "SweetenerReward_Armor_Uncommon_Banana_Arms"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 6 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 7 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 4 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 5 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" Quantity: 1 Weight: 100}
  }
}
activityrewards:
{
  key: "SweetenerRewardsTable_Animations_Epic"
  value:
  {
    AuthoredId: "fc0c2504-a2ec-e364-093d-5fca84205e2d"
    Name: "SweetenerRewardsTable_Animations_Epic"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "27448f34-ca08-99b4-1b63-fd4b227e6af4" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "938e1ab9-a85b-5444-7bd6-f8862ace8b16" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "05633498-ace9-de14-c939-9435a6343d0f" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "5fce85e9-51f9-d7b4-db81-ef141439db00" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "917e27ec-19ff-3d04-69f6-c596456816e6" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "97c2c3da-93c7-a0d4-1967-eeba13396cce" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "64732638-e355-d5c4-aa2d-e28c3db4aa72" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "a170f9f3-f97b-8eb4-49c0-c99a916e55d7" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "7294e11a-f8b2-9d34-5854-fee4535f1aaa" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "3afdf3ed-bea4-1c14-faac-159412578c0f" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "471eee2f-e6ad-a694-58de-bdee5a379096" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "b0877d2a-71db-7374-8a8b-8044f7a8a52a" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "4fca9718-5e62-8924-7a58-790c249fbc7f" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "c6e3c59b-4415-52b4-8b34-1593bbc26634" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "2831df30-e1a2-dcf4-2a9a-0c91ac4ea310" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "a7629ff3-c1ef-e664-3a9c-bd27cb332abd" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "02ddcd6e-e441-73f4-e874-399531dc4708" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "0c2cdea6-ff22-b124-da21-882987b1707a" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "f33c74e6-f1b4-6ca4-bb5a-cf6edc77cab5" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "a2a185e5-4543-6814-d88d-d078a0e38521" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "a91a62fa-8d65-6234-d91b-a0ad0d9388e3" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "ed324300-aa2c-0c04-4b75-2d0eae3780f4" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "d35a9d1c-2223-c404-c840-c0218f9b147d" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 7 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
  }
}
activityrewards:
{
  key: "RewardTable_Expedition_T3Rare3"
  value:
  {
    AuthoredId: "a2d81a2a-60e6-f354-6943-40803787242f"
    Name: "RewardTable_Expedition_T3Rare3"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4e343f68-1aaa-0e84-7bab-eae458b68264" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "16e0b307-2b40-4584-294c-dee505e47827" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "87040ddf-f0cb-f894-dbbd-a4f266cae4e7" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "524c8f4f-cb50-ea04-dafd-e192f30c7656" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8aac4645-29cf-fe04-59c0-415508b0432e" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "934dae05-689f-30e4-5804-d497aee0b47c" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "af509c01-933e-ce14-49c3-106081e16d75" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9b70e722-0442-2924-092c-93efc1d92975" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "63a8ea74-65ee-c024-b8f0-87e173258257" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ea89b0d-7070-d3e4-a86f-c0e75c0e4ff5" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9c64b7e4-0458-65c4-9b79-5cc35e847f9c" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "28089a53-62ac-2044-1964-29f2f31d8a33" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "33d499a1-9dee-dfd4-7843-c743a6131dcf" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 3 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "a889e1d9-c337-dff4-3aae-7701b6e48aeb" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "c229b9d2-8e0f-8644-7b09-1b6ba6247026" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "502693b2-3dd5-a204-197d-39031869f194" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "28e2a7ea-e606-37d4-4b77-33069ba87bc8" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "60f3f548-20ca-7a64-6ad3-054d654e24a2" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "fe9ec873-e4b8-0714-2be0-a3c2c547f9ad" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "c28bd52c-abf1-eff4-da81-6a47f7bd79a9" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "8bc478cc-d85e-2d84-28f3-1781f582319f" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "27f47d35-c2d3-c004-9a36-5253005559c2" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "b68f5a1f-8bfd-36d4-6ac4-f9ccc5f3acf6" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "db1606dd-112d-7a34-db0b-8923900c8ac7" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "34026945-900a-8d14-98b6-0af2354f22ac" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Common_Icing_Torso"
  value:
  {
    AuthoredId: "47fe34c6-8617-5134-1b61-8de82611f625"
    Name: "SweetenerReward_Armor_Common_Icing_Torso"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 18 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 9 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 8 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 10 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" Quantity: 1 Weight: 200}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Common_ChocoChip_Arms"
  value:
  {
    AuthoredId: "c9a8641f-d10a-a2b4-d854-0a52cd9150f0"
    Name: "SweetenerReward_Armor_Common_ChocoChip_Arms"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 6 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 7 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 5 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 6 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" Quantity: 1 Weight: 200}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Epic_Gold_Bust"
  value:
  {
    AuthoredId: "11abb47b-9c72-a854-7b43-426d198904b7"
    Name: "SweetenerReward_Armor_Epic_Gold_Bust"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 1 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f6c3b895-6488-1b74-a9b0-664027a683e2" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 3 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f6c3b895-6488-1b74-a9b0-664027a683e2" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 2 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f6c3b895-6488-1b74-a9b0-664027a683e2" Quantity: 1 Weight: 25}
  }
}
activityrewards:
{
  key: "RewardsTable_Tournament_T4Epic2"
  value:
  {
    AuthoredId: "53f146c5-6d06-d7e4-e970-5dd72dbef229"
    Name: "RewardsTable_Tournament_T4Epic2"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9b70e722-0442-2924-092c-93efc1d92975" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "63a8ea74-65ee-c024-b8f0-87e173258257" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ea89b0d-7070-d3e4-a86f-c0e75c0e4ff5" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9c64b7e4-0458-65c4-9b79-5cc35e847f9c" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "1525c955-d3c0-9f84-69fb-cf98837f4f85" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "86b5fac0-4ec1-09e4-6ab7-96d3fd13dd04" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "af509c01-933e-ce14-49c3-106081e16d75" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9ff884ab-efcb-0af4-5acd-976fea43b3a3" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "33d499a1-9dee-dfd4-7843-c743a6131dcf" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "28089a53-62ac-2044-1964-29f2f31d8a33" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "499a8021-eaea-c7d4-aa1c-bb6e55cd300c" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "bbeb0e31-3775-a864-1952-082233cf7b89" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "08847311-c629-a154-5958-13e8f1d77f88" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "3b01bcce-f26c-66c4-68a5-80ab4d67f28c" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "92954ab6-fe54-4a64-d942-feca64e9e931" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "eeeca33e-5c80-9ec4-eb83-46e4b7daf04b" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f7668430-bfc1-71f4-29b8-f36c7d6905d5" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "5b187f8e-d5ff-36d4-7b56-8be9cce004c8" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "321c2a65-64b1-8344-2915-85562e2ad7ec" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 3 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 4 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "f0d9f776-2d8e-1e04-da62-fb62623b7d2e" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "b192f393-c1ac-3364-a8a0-dd5582933e66" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "7c922dce-d72d-2544-fbc3-7abb2a12c92e" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "30a597d1-31e7-3474-4b15-d7ff87f384f8" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "c481aeee-d1a1-01c4-7aca-92d0edcddf18" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "edc99c4e-6e95-5ae4-ea8c-e6071cd8635a" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
  }
}
activityrewards:
{
  key: "RewardsTable_Tournament_T4Epic3"
  value:
  {
    AuthoredId: "a8c278b1-2a20-db84-d9ca-977c84fe9cb8"
    Name: "RewardsTable_Tournament_T4Epic3"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9b70e722-0442-2924-092c-93efc1d92975" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "63a8ea74-65ee-c024-b8f0-87e173258257" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ea89b0d-7070-d3e4-a86f-c0e75c0e4ff5" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9c64b7e4-0458-65c4-9b79-5cc35e847f9c" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "1525c955-d3c0-9f84-69fb-cf98837f4f85" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "86b5fac0-4ec1-09e4-6ab7-96d3fd13dd04" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "af509c01-933e-ce14-49c3-106081e16d75" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9ff884ab-efcb-0af4-5acd-976fea43b3a3" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "33d499a1-9dee-dfd4-7843-c743a6131dcf" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "28089a53-62ac-2044-1964-29f2f31d8a33" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "47b8a853-791a-1424-1ba9-4adaf4a3f4a5" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8898cfa3-1512-22e4-0b9f-c6e220f315b5" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "0947de60-3149-c2d4-dad0-d4e67db681b6" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f6c3b895-6488-1b74-a9b0-664027a683e2" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "a1af5095-bfbc-1a84-7862-9b246dc29d81" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "52ea4044-e203-16d4-c90f-8b685ed5e9c9" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "57674941-fe8c-5b24-0b69-16fb1d22ee62" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "2f765969-c607-d4a4-0b9c-72d3cce7fbec" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "b4e362bb-2634-7df4-39a6-71b358368a6d" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8898194e-ac52-f624-a830-176655bb43cd" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 3 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 4 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "ec86d547-e1d0-6324-bb20-e0a20e116fb2" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "b5bb8cce-20bf-f5a4-3a51-694c37b8f9b9" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "5124cf4a-b08e-2dd4-6809-ed15f73ae685" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "c4c807ca-431a-9184-da8e-2c9993d1bba0" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "9d7d627c-8627-b934-6b2a-f645d96447ea" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "40f97352-9e9e-39d4-a830-6672d1ed46f6" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Common_Nonpareil_Legs"
  value:
  {
    AuthoredId: "77be7cb7-b88b-1a24-d97d-661e005c9150"
    Name: "SweetenerReward_Armor_Common_Nonpareil_Legs"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 17 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8aac4645-29cf-fe04-59c0-415508b0432e" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 15 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8aac4645-29cf-fe04-59c0-415508b0432e" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 14 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8aac4645-29cf-fe04-59c0-415508b0432e" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 16 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8aac4645-29cf-fe04-59c0-415508b0432e" Quantity: 1 Weight: 200}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Epic_Bronze_Waist"
  value:
  {
    AuthoredId: "80b5d4e4-653e-8ce4-7b52-ca4cfa989056"
    Name: "SweetenerReward_Armor_Epic_Bronze_Waist"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 11 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "a2c2697c-d738-c004-4b32-d800921bab06" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 13 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "a2c2697c-d738-c004-4b32-d800921bab06" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 12 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "a2c2697c-d738-c004-4b32-d800921bab06" Quantity: 1 Weight: 25}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Common_RingPop_Bust"
  value:
  {
    AuthoredId: "463e6097-84d8-0324-ab8d-006829dac706"
    Name: "SweetenerReward_Armor_Common_RingPop_Bust"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 1 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "934dae05-689f-30e4-5804-d497aee0b47c" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 3 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "934dae05-689f-30e4-5804-d497aee0b47c" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 2 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "934dae05-689f-30e4-5804-d497aee0b47c" Quantity: 1 Weight: 200}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Rare_CandyRibbon_Waist"
  value:
  {
    AuthoredId: "14142562-824c-d2f4-4b86-d317c6781be5"
    Name: "SweetenerReward_Armor_Rare_CandyRibbon_Waist"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 11 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 12 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 13 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" Quantity: 1 Weight: 50}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Epic_Silver_Bust"
  value:
  {
    AuthoredId: "a7bb42dd-1559-8b44-58a7-50ba6903de9f"
    Name: "SweetenerReward_Armor_Epic_Silver_Bust"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 1 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "321c2a65-64b1-8344-2915-85562e2ad7ec" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 3 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "321c2a65-64b1-8344-2915-85562e2ad7ec" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 2 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "321c2a65-64b1-8344-2915-85562e2ad7ec" Quantity: 1 Weight: 25}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Common_FizzyPowder_Bust"
  value:
  {
    AuthoredId: "c0ebb18f-d55e-13b4-58f1-2e2eafe35e19"
    Name: "SweetenerReward_Armor_Common_FizzyPowder_Bust"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 1 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "16e0b307-2b40-4584-294c-dee505e47827" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 3 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "16e0b307-2b40-4584-294c-dee505e47827" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 2 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "16e0b307-2b40-4584-294c-dee505e47827" Quantity: 1 Weight: 200}
  }
}
activityrewards:
{
  key: "RewardsTable_Tournament_T1Common1"
  value:
  {
    AuthoredId: "be507768-f89e-7114-3a40-655521f5ecad"
    Name: "RewardsTable_Tournament_T1Common1"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4e343f68-1aaa-0e84-7bab-eae458b68264" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "16e0b307-2b40-4584-294c-dee505e47827" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "87040ddf-f0cb-f894-dbbd-a4f266cae4e7" Quantity: 5 Weight: 30}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Epic_Gold_Legs"
  value:
  {
    AuthoredId: "ef9cc262-19a4-06b4-b8ac-653f6cdf0271"
    Name: "SweetenerReward_Armor_Epic_Gold_Legs"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 15 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "47b8a853-791a-1424-1ba9-4adaf4a3f4a5" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 14 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "47b8a853-791a-1424-1ba9-4adaf4a3f4a5" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 17 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "47b8a853-791a-1424-1ba9-4adaf4a3f4a5" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 8 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8898194e-ac52-f624-a830-176655bb43cd" Quantity: 1 Weight: 25}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Common_Gumball_Waist"
  value:
  {
    AuthoredId: "5a5cdf57-3085-6654-68a3-f5914d0ef72c"
    Name: "SweetenerReward_Armor_Common_Gumball_Waist"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 13 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "87040ddf-f0cb-f894-dbbd-a4f266cae4e7" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 12 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "87040ddf-f0cb-f894-dbbd-a4f266cae4e7" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 11 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "87040ddf-f0cb-f894-dbbd-a4f266cae4e7" Quantity: 1 Weight: 200}
  }
}
activityrewards:
{
  key: "RewardsTable_Tournament_T4Epic1"
  value:
  {
    AuthoredId: "ba99802e-6d0c-3da4-b9b1-99fc8105e54a"
    Name: "RewardsTable_Tournament_T4Epic1"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9b70e722-0442-2924-092c-93efc1d92975" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "63a8ea74-65ee-c024-b8f0-87e173258257" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ea89b0d-7070-d3e4-a86f-c0e75c0e4ff5" Quantity: 5 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9c64b7e4-0458-65c4-9b79-5cc35e847f9c" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "1525c955-d3c0-9f84-69fb-cf98837f4f85" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "86b5fac0-4ec1-09e4-6ab7-96d3fd13dd04" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "af509c01-933e-ce14-49c3-106081e16d75" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9ff884ab-efcb-0af4-5acd-976fea43b3a3" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "33d499a1-9dee-dfd4-7843-c743a6131dcf" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "28089a53-62ac-2044-1964-29f2f31d8a33" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" Quantity: 5 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8932caea-da2e-6384-58ec-749624e213c9" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "dd29bd35-bb51-8d94-4913-8ca943d062f1" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8f896243-a6de-1b54-68a3-0df72e8688e4" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "a2c2697c-d738-c004-4b32-d800921bab06" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8aa0e6a7-0b45-3404-9b39-dc4a4300a87f" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "aa0cb18d-5681-5504-09d7-4db06bf7a4d8" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "48ea4c49-79dd-6514-6b8b-f87fd6d8a330" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "1f089286-ff92-dcc4-b85c-7ebf28d2cc4f" Quantity: 5 Weight: 3}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 3 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 10}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "df921272-743b-5f14-1a81-710a1b97d72e" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "c1e9a4e1-cf55-6084-8bf2-778678387353" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "3f5314d7-0755-ab64-8a9c-589d3158de7a" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "b4efa30e-0d76-5a04-b8fb-0a5c7b02d4a3" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "71a69f03-1e74-0ed4-6994-54b900eb0cc9" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "cf363926-b8c5-8294-099a-5fd770749258" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "390c2aa8-6b27-9a04-c820-835a0d178686" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "f13487ec-4bf1-7ec4-e9d6-fd043f9842d8" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 4 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Epic_Gold_Arms"
  value:
  {
    AuthoredId: "075a859d-e113-17a4-6964-fb731d53271c"
    Name: "SweetenerReward_Armor_Epic_Gold_Arms"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 6 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "52ea4044-e203-16d4-c90f-8b685ed5e9c9" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 7 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "52ea4044-e203-16d4-c90f-8b685ed5e9c9" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 4 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "52ea4044-e203-16d4-c90f-8b685ed5e9c9" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 5 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "52ea4044-e203-16d4-c90f-8b685ed5e9c9" Quantity: 1 Weight: 25}
  }
}
activityrewards:
{
  key: "RewardsTable_Tournament_T1Common2"
  value:
  {
    AuthoredId: "bd3bfc25-5636-a704-a96e-59473e39a368"
    Name: "RewardsTable_Tournament_T1Common2"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "524c8f4f-cb50-ea04-dafd-e192f30c7656" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8aac4645-29cf-fe04-59c0-415508b0432e" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "934dae05-689f-30e4-5804-d497aee0b47c" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" Quantity: 5 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" Quantity: 5 Weight: 30}
  }
}
activityrewards:
{
  key: "TounamentRewardsList_1stTourneyWinner"
  value:
  {
    AuthoredId: "d8710632-c648-8814-9bc6-89bfcda03385"
    Name: "TounamentRewardsList_1stTourneyWinner"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "1bbc7d99-7fce-24a4-c9a3-dfaf4b744efa" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8aac4645-29cf-fe04-59c0-415508b0432e" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8aac4645-29cf-fe04-59c0-415508b0432e" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" Quantity: 10 Weight: 100}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Epic_Silver_Legs"
  value:
  {
    AuthoredId: "8d5e0675-7121-cc44-4abe-83dc88519760"
    Name: "SweetenerReward_Armor_Epic_Silver_Legs"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 15 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "08847311-c629-a154-5958-13e8f1d77f88" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 14 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "08847311-c629-a154-5958-13e8f1d77f88" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 17 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "08847311-c629-a154-5958-13e8f1d77f88" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 16 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "08847311-c629-a154-5958-13e8f1d77f88" Quantity: 1 Weight: 25}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Rare_Licorice_Bust"
  value:
  {
    AuthoredId: "6b914cc5-c73a-ed64-a871-5e9757771f9f"
    Name: "SweetenerReward_Armor_Rare_Licorice_Bust"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 1 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 3 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 2 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" Quantity: 1 Weight: 100}
  }
}
activityrewards:
{
  key: "SweetenersRewardsTable_Moves_Epic_a"
  value:
  {
    AuthoredId: "f6655962-a4bc-4f14-985b-79dd68f61103"
    Name: "SweetenersRewardsTable_Moves_Epic_a"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "8e4753b5-c739-aaf4-ab79-4d333a69cae6" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "01b34a91-37d8-29d4-09cb-2d515f51e315" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "55cafe03-4a58-0f44-6a27-f9fc6fce881a" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "4dbb297f-6a80-ca44-a94f-3ea9ed3311e8" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "c6770789-4a1f-7f24-f921-c4dc66c2697a" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "e16531a7-33fd-a3e4-69fd-ff10eba65cb7" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "aa7d8565-99e1-84f4-49d2-8925c2602a7b" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "ad6d5d28-08b4-4304-6b49-11f8f5f9ff1c" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "2569d967-c676-8b44-cbdf-d514e4c5c73d" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "9f0df0ed-13a6-84c4-781d-9dbf9741c66e" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "b1bd32de-88b2-1204-7b2e-0680677d78c4" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "fa6144f9-ad49-8124-386f-351a7f1ab546" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "b889835e-1cd6-d3d4-f9b4-dd1989f49dac" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "1d1dec74-ca35-0cc4-d930-164786a9dd81" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "1e47a8e0-acfd-1244-0b7d-539d62429060" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "628ebc0b-bb19-ee64-68b8-2f66f4b92720" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "c28e13b4-cb1f-6ea4-680f-91010a400e10" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "bd27fe1c-ce9a-f524-c887-61f9e4d5088b" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "29d559bb-41a7-9f14-5bf8-605da8166943" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
  }
}
activityrewards:
{
  key: "SweetenersRewardsTable_Moves_Epic_b"
  value:
  {
    AuthoredId: "5c6f7ec8-7032-73a4-28d9-ce67b51c9db6"
    Name: "SweetenersRewardsTable_Moves_Epic_b"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "e55a3d69-9fb4-5194-4812-c2f85e887aca" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "1a67cc79-b97a-4624-ca67-36c9fcbee970" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "3e76f2fa-072c-5654-dac9-c9b0e4e1643a" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "b96f1ff8-66d5-f244-79fd-260735d4c3d7" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "bbf71f26-78f5-5014-5b4b-a1c3866cb75b" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "61d8b3ec-9a01-0e64-fb45-27d2b858e332" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "2c94f949-a10e-2524-2827-6588982ce0b1" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "41416afb-0d72-f9b4-ba30-f089b713500f" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "55e05ece-a3f6-a924-3833-2222a0eee6a3" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "431900a2-54d4-df34-d9f7-6a4608fecfaa" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "86bd4f81-e96b-9864-78b5-5fc51fbff6a4" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "e201f964-8949-9d74-fbee-4ee8786284ca" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "9f0df0ed-13a6-84c4-781d-9dbf9741c66e" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "0090580c-04ef-9d84-e883-32f52c977b98" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "a9d69fc0-a1b8-ae24-cbd7-249760200211" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "2c555752-8a84-58f4-395e-6460b7864069" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "86b323c2-b2fd-2494-ab5e-bc3514bc92d8" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Rare_Toffee_Torso"
  value:
  {
    AuthoredId: "f931da3f-e979-ee94-eacc-ee40174cf84b"
    Name: "SweetenerReward_Armor_Rare_Toffee_Torso"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 18 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 9 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 8 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 10 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" Quantity: 1 Weight: 50}
  }
}
activityrewards:
{
  key: "SweetenerRewardsTable_Animations_Rare"
  value:
  {
    AuthoredId: "3577d41a-18bc-8f74-392c-579c129e9181"
    Name: "SweetenerRewardsTable_Animations_Rare"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "27448f34-ca08-99b4-1b63-fd4b227e6af4" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "938e1ab9-a85b-5444-7bd6-f8862ace8b16" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "05633498-ace9-de14-c939-9435a6343d0f" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "5fce85e9-51f9-d7b4-db81-ef141439db00" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "917e27ec-19ff-3d04-69f6-c596456816e6" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "97c2c3da-93c7-a0d4-1967-eeba13396cce" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "64732638-e355-d5c4-aa2d-e28c3db4aa72" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "a170f9f3-f97b-8eb4-49c0-c99a916e55d7" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "c6e3c59b-4415-52b4-8b34-1593bbc26634" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "2831df30-e1a2-dcf4-2a9a-0c91ac4ea310" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "a7629ff3-c1ef-e664-3a9c-bd27cb332abd" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "02ddcd6e-e441-73f4-e874-399531dc4708" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "0c2cdea6-ff22-b124-da21-882987b1707a" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "f33c74e6-f1b4-6ca4-bb5a-cf6edc77cab5" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "a2a185e5-4543-6814-d88d-d078a0e38521" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "a91a62fa-8d65-6234-d91b-a0ad0d9388e3" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "ed324300-aa2c-0c04-4b75-2d0eae3780f4" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "d35a9d1c-2223-c404-c840-c0218f9b147d" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 7 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Uncommon_Marshmallow_Legs"
  value:
  {
    AuthoredId: "0dfaecda-5ccb-faa4-ea9c-e6b71f040b9e"
    Name: "SweetenerReward_Armor_Uncommon_Marshmallow_Legs"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 17 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 15 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 14 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 16 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" Quantity: 1 Weight: 25}
  }
}
activityrewards:
{
  key: "SweetenersRewardsTable_Moves_Rare_a"
  value:
  {
    AuthoredId: "b1fc13b2-84e3-e3b4-a904-371d21b0bd2c"
    Name: "SweetenersRewardsTable_Moves_Rare_a"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "2c94f949-a10e-2524-2827-6588982ce0b1" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "41416afb-0d72-f9b4-ba30-f089b713500f" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "55e05ece-a3f6-a924-3833-2222a0eee6a3" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "431900a2-54d4-df34-d9f7-6a4608fecfaa" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "86bd4f81-e96b-9864-78b5-5fc51fbff6a4" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "e201f964-8949-9d74-fbee-4ee8786284ca" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "9f0df0ed-13a6-84c4-781d-9dbf9741c66e" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "0090580c-04ef-9d84-e883-32f52c977b98" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "a9d69fc0-a1b8-ae24-cbd7-249760200211" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "2c555752-8a84-58f4-395e-6460b7864069" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "86b323c2-b2fd-2494-ab5e-bc3514bc92d8" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
  }
}
activityrewards:
{
  key: "SweetenersRewardsTable_Moves_Rare_b"
  value:
  {
    AuthoredId: "e03a72e4-d6e6-e0b4-4936-a09848309399"
    Name: "SweetenersRewardsTable_Moves_Rare_b"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "e16531a7-33fd-a3e4-69fd-ff10eba65cb7" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "aa7d8565-99e1-84f4-49d2-8925c2602a7b" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "ad6d5d28-08b4-4304-6b49-11f8f5f9ff1c" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "9f0df0ed-13a6-84c4-781d-9dbf9741c66e" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "b1bd32de-88b2-1204-7b2e-0680677d78c4" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "fa6144f9-ad49-8124-386f-351a7f1ab546" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "b889835e-1cd6-d3d4-f9b4-dd1989f49dac" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "1d1dec74-ca35-0cc4-d930-164786a9dd81" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "1e47a8e0-acfd-1244-0b7d-539d62429060" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "628ebc0b-bb19-ee64-68b8-2f66f4b92720" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "c28e13b4-cb1f-6ea4-680f-91010a400e10" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "bd27fe1c-ce9a-f524-c887-61f9e4d5088b" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
  }
}
activityrewards:
{
  key: "SweetenerRewardsTable_Armor_Epic"
  value:
  {
    AuthoredId: "b057b0af-c4d9-0464-98cf-0303f0baf359"
    Name: "SweetenerRewardsTable_Armor_Epic"
    Rewards: {ListTableId: "183878d8-0100-5714-2a9e-1623bbd6e656" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "0bbb171d-73d0-9a94-296b-502ebc7a0ec9" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "0c12eefb-bd73-c5a4-a97d-5d07eb20c9d5" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "128d71f0-60c9-4eb4-2b6e-02b31d973959" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "80b5d4e4-653e-8ce4-7b52-ca4cfa989056" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "075a859d-e113-17a4-6964-fb731d53271c" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "11abb47b-9c72-a854-7b43-426d198904b7" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "ef9cc262-19a4-06b4-b8ac-653f6cdf0271" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "f0d10fe6-0161-55e4-7973-f49726d5fdae" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "68d828af-100a-8994-4912-9c19f61fc6d6" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "26a149ff-3a0f-1b54-9ac6-f5c30b44912f" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "a7bb42dd-1559-8b44-58a7-50ba6903de9f" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "8d5e0675-7121-cc44-4abe-83dc88519760" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "f02ddbe9-5931-0404-29ad-743ded7f6d0f" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "d53b3e22-0dbe-5114-592c-55cf652694c3" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 7 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Epic_Silver_Torso"
  value:
  {
    AuthoredId: "f02ddbe9-5931-0404-29ad-743ded7f6d0f"
    Name: "SweetenerReward_Armor_Epic_Silver_Torso"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 18 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 10 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 9 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 8 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" Quantity: 1 Weight: 25}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Epic_Silver_Waist"
  value:
  {
    AuthoredId: "d53b3e22-0dbe-5114-592c-55cf652694c3"
    Name: "SweetenerReward_Armor_Epic_Silver_Waist"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 11 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "5b187f8e-d5ff-36d4-7b56-8be9cce004c8" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 13 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "5b187f8e-d5ff-36d4-7b56-8be9cce004c8" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 12 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "5b187f8e-d5ff-36d4-7b56-8be9cce004c8" Quantity: 1 Weight: 25}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Rare_Icing_Waist"
  value:
  {
    AuthoredId: "6141fbcc-382a-1524-eab6-b9c0bc4033ce"
    Name: "SweetenerReward_Armor_Rare_Icing_Waist"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 13 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 12 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 11 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" Quantity: 1 Weight: 50}
  }
}
activityrewards:
{
  key: "SweetenerRewardsTable_Armor_Rare"
  value:
  {
    AuthoredId: "f2092146-706b-09f4-a80b-64aabe5ff4a6"
    Name: "SweetenerRewardsTable_Armor_Rare"
    Rewards: {ListTableId: "3fa2e317-0875-98a4-d9ca-7dc143c894d0" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "14142562-824c-d2f4-4b86-d317c6781be5" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "50379c2b-2422-8104-ea69-bb2882e9cac0" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "82741761-1136-4b94-c90e-22b41bbeb41f" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "6141fbcc-382a-1524-eab6-b9c0bc4033ce" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "69fe65f1-2d6a-82b4-5ada-5c48536e357a" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "6b914cc5-c73a-ed64-a871-5e9757771f9f" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 7 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "f931da3f-e979-ee94-eacc-ee40174cf84b" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Uncommon_CinnamonBall_Waist"
  value:
  {
    AuthoredId: "aeb02c5c-b10b-23b4-5bdb-3693ce7661ff"
    Name: "SweetenerReward_Armor_Uncommon_CinnamonBall_Waist"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 13 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 12 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 11 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" Quantity: 1 Weight: 100}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Uncommon_ChocoNut_Bust"
  value:
  {
    AuthoredId: "fbf39341-9036-def4-e9c6-f15feb175e83"
    Name: "SweetenerReward_Armor_Uncommon_ChocoNut_Bust"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 1 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 3 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 2 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" Quantity: 1 Weight: 50}
  }
}
activityrewards:
{
  key: "SweetenersRewardsTable_Moves_All"
  value:
  {
    AuthoredId: "d07c3481-691a-71e4-990e-9b5739afcff1"
    Name: "SweetenersRewardsTable_Moves_All"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "1e47a8e0-acfd-1244-0b7d-539d62429060" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "628ebc0b-bb19-ee64-68b8-2f66f4b92720" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "c28e13b4-cb1f-6ea4-680f-91010a400e10" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "bd27fe1c-ce9a-f524-c887-61f9e4d5088b" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "29d559bb-41a7-9f14-5bf8-605da8166943" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "1d1dec74-ca35-0cc4-d930-164786a9dd81" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "628ebc0b-bb19-ee64-68b8-2f66f4b92720" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "1e47a8e0-acfd-1244-0b7d-539d62429060" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "c28e13b4-cb1f-6ea4-680f-91010a400e10" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "bd27fe1c-ce9a-f524-c887-61f9e4d5088b" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "29d559bb-41a7-9f14-5bf8-605da8166943" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "8e4753b5-c739-aaf4-ab79-4d333a69cae6" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "01b34a91-37d8-29d4-09cb-2d515f51e315" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "55cafe03-4a58-0f44-6a27-f9fc6fce881a" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "4dbb297f-6a80-ca44-a94f-3ea9ed3311e8" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "c6770789-4a1f-7f24-f921-c4dc66c2697a" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "92f7b745-c1ff-3cd4-f9cf-0fe0dafb5536" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "e55a3d69-9fb4-5194-4812-c2f85e887aca" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "1a67cc79-b97a-4624-ca67-36c9fcbee970" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "1a67cc79-b97a-4624-ca67-36c9fcbee970" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "3e76f2fa-072c-5654-dac9-c9b0e4e1643a" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "b96f1ff8-66d5-f244-79fd-260735d4c3d7" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "bbf71f26-78f5-5014-5b4b-a1c3866cb75b" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "61d8b3ec-9a01-0e64-fb45-27d2b858e332" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "34a115db-3153-aff4-9a46-97700634f1fb" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "2c94f949-a10e-2524-2827-6588982ce0b1" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "41416afb-0d72-f9b4-ba30-f089b713500f" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "55e05ece-a3f6-a924-3833-2222a0eee6a3" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "e16531a7-33fd-a3e4-69fd-ff10eba65cb7" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "aa7d8565-99e1-84f4-49d2-8925c2602a7b" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "ad6d5d28-08b4-4304-6b49-11f8f5f9ff1c" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "2569d967-c676-8b44-cbdf-d514e4c5c73d" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "431900a2-54d4-df34-d9f7-6a4608fecfaa" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "86bd4f81-e96b-9864-78b5-5fc51fbff6a4" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "e201f964-8949-9d74-fbee-4ee8786284ca" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "9f0df0ed-13a6-84c4-781d-9dbf9741c66e" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "b1bd32de-88b2-1204-7b2e-0680677d78c4" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "fa6144f9-ad49-8124-386f-351a7f1ab546" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "b889835e-1cd6-d3d4-f9b4-dd1989f49dac" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Uncommon_JellyBean_Back"
  value:
  {
    AuthoredId: "f4188e48-3ef8-0694-8bdb-5796687013f9"
    Name: "SweetenerReward_Armor_Uncommon_JellyBean_torso"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 18 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9b70e722-0442-2924-092c-93efc1d92975" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 9 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9b70e722-0442-2924-092c-93efc1d92975" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 8 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9b70e722-0442-2924-092c-93efc1d92975" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 10 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9b70e722-0442-2924-092c-93efc1d92975" Quantity: 1 Weight: 100}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Common_CandyCane_Waist"
  value:
  {
    AuthoredId: "540deb44-a0b5-6074-daa2-c3ed7dd4c509"
    Name: "SweetenerReward_Armor_Common_CandyCane_Waist"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 13 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4e343f68-1aaa-0e84-7bab-eae458b68264" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 12 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4e343f68-1aaa-0e84-7bab-eae458b68264" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 11 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4e343f68-1aaa-0e84-7bab-eae458b68264" Quantity: 1 Weight: 200}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Epic_Bronze_Bust"
  value:
  {
    AuthoredId: "0bbb171d-73d0-9a94-296b-502ebc7a0ec9"
    Name: "SweetenerReward_Armor_Epic_Bronze_Bust"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 1 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "aa0cb18d-5681-5504-09d7-4db06bf7a4d8" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 3 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "aa0cb18d-5681-5504-09d7-4db06bf7a4d8" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 2 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "aa0cb18d-5681-5504-09d7-4db06bf7a4d8" Quantity: 1 Weight: 25}
  }
}
activityrewards:
{
  key: "RewardsList_1stExpedition"
  value:
  {
    AuthoredId: "e72d7dc0-2b81-d114-3803-652de993041f"
    Name: "Reward List 1st Expedition"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "ba0121ba-e8a6-7e64-9bc1-71dfeca27daa" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "16e0b307-2b40-4584-294c-dee505e47827" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4e343f68-1aaa-0e84-7bab-eae458b68264" Quantity: 10 Weight: 100}
  }
}
activityrewards:
{
  key: "RewardTable_Expedition_T3Rare1"
  value:
  {
    AuthoredId: "1f9dc820-9e7b-0f04-3944-a13ef7236ca0"
    Name: "RewardTable_Expedition_T3Rare1"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4e343f68-1aaa-0e84-7bab-eae458b68264" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "16e0b307-2b40-4584-294c-dee505e47827" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "87040ddf-f0cb-f894-dbbd-a4f266cae4e7" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "524c8f4f-cb50-ea04-dafd-e192f30c7656" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8aac4645-29cf-fe04-59c0-415508b0432e" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "934dae05-689f-30e4-5804-d497aee0b47c" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "af509c01-933e-ce14-49c3-106081e16d75" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9b70e722-0442-2924-092c-93efc1d92975" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "63a8ea74-65ee-c024-b8f0-87e173258257" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ea89b0d-7070-d3e4-a86f-c0e75c0e4ff5" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9c64b7e4-0458-65c4-9b79-5cc35e847f9c" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "1525c955-d3c0-9f84-69fb-cf98837f4f85" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "86b5fac0-4ec1-09e4-6ab7-96d3fd13dd04" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 3 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "c6cabeb5-60f6-7c34-fb09-c7e24971ff3b" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "223de3eb-e192-0854-d800-6f9f2fa8e28b" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "275d1b18-22e6-fcf4-2919-b28bc1da9060" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "4b90dbd5-e6d3-69c4-8b8d-1327bf8381af" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "8c4b6099-b736-16a4-eb8b-25eaaac1a0c3" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "dfd860da-2127-60e4-8b46-39bfffb57245" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "db8bf7c2-c569-8314-2a80-d9ffb6f1601b" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "7938f8c7-2d20-19c4-9be9-b05fd663f44f" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
  }
}
activityrewards:
{
  key: "RewardTable_Expedition_T3Rare2"
  value:
  {
    AuthoredId: "74c64b03-c246-7174-bb50-4834e2d2a010"
    Name: "RewardTable_Expedition_T3Rare2"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4e343f68-1aaa-0e84-7bab-eae458b68264" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "16e0b307-2b40-4584-294c-dee505e47827" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "87040ddf-f0cb-f894-dbbd-a4f266cae4e7" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "524c8f4f-cb50-ea04-dafd-e192f30c7656" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8aac4645-29cf-fe04-59c0-415508b0432e" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "934dae05-689f-30e4-5804-d497aee0b47c" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "af509c01-933e-ce14-49c3-106081e16d75" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9b70e722-0442-2924-092c-93efc1d92975" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "63a8ea74-65ee-c024-b8f0-87e173258257" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ea89b0d-7070-d3e4-a86f-c0e75c0e4ff5" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9ff884ab-efcb-0af4-5acd-976fea43b3a3" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "33d499a1-9dee-dfd4-7843-c743a6131dcf" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "28089a53-62ac-2044-1964-29f2f31d8a33" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 3 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "83bdccb0-abc7-3264-8994-4d1700458a07" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "80ede00d-10fd-10c4-baaa-045acaeedc22" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "fae112f6-dba3-e154-78bf-154ec2f3f94b" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "fb87ff21-f4d6-d564-2956-e250d2cb2409" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "9a2c04f2-cd9c-e484-daca-d4e4a6b5b9b8" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "8f8425e7-cab0-0b14-1874-5072afe7d0b5" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "99f7eaf3-67c6-30d4-5a3d-4a2a18bb75f7" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "4aba4145-0370-3484-1b2b-d416df6138ee" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "7b7d8898-7f58-0334-0bad-825dc87a6638" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 5}
  }
}
activityrewards:
{
  key: "RewardTable_Expedition_T1Common1"
  value:
  {
    AuthoredId: "7c40dee4-c1f9-c8f4-d923-910133846c78"
    Name: "RewardTable_Expedition_T1Common1"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4e343f68-1aaa-0e84-7bab-eae458b68264" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "16e0b307-2b40-4584-294c-dee505e47827" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "87040ddf-f0cb-f894-dbbd-a4f266cae4e7" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 30}
  }
}
activityrewards:
{
  key: "RewardTable_Expedition_T1Common2"
  value:
  {
    AuthoredId: "8ba8347d-fb20-adf4-ca16-93a8943ddd4f"
    Name: "RewardTable_Expedition_T1Common2"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "524c8f4f-cb50-ea04-dafd-e192f30c7656" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8aac4645-29cf-fe04-59c0-415508b0432e" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "934dae05-689f-30e4-5804-d497aee0b47c" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" Quantity: 10 Weight: 100}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Epic_Bronze_Arms"
  value:
  {
    AuthoredId: "183878d8-0100-5714-2a9e-1623bbd6e656"
    Name: "SweetenerReward_Armor_Epic_Bronze_Arms"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 6 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8932caea-da2e-6384-58ec-749624e213c9" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 7 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8932caea-da2e-6384-58ec-749624e213c9" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 4 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8932caea-da2e-6384-58ec-749624e213c9" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 5 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8932caea-da2e-6384-58ec-749624e213c9" Quantity: 1 Weight: 25}
  }
}
activityrewards:
{
  key: "RewardTable_Expedition_T2Uncommon1"
  value:
  {
    AuthoredId: "bf5e7bbe-2361-80b4-f862-acdfa491c07e"
    Name: "RewardTable_Expedition_T2Uncommon1"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4e343f68-1aaa-0e84-7bab-eae458b68264" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "16e0b307-2b40-4584-294c-dee505e47827" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "87040ddf-f0cb-f894-dbbd-a4f266cae4e7" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "524c8f4f-cb50-ea04-dafd-e192f30c7656" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8aac4645-29cf-fe04-59c0-415508b0432e" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "934dae05-689f-30e4-5804-d497aee0b47c" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 15}
  }
}
activityrewards:
{
  key: "RewardTable_Expedition_T2Uncommon2"
  value:
  {
    AuthoredId: "f6151176-251b-6de4-1aee-245999752b5f"
    Name: "RewardTable_Expedition_T2Uncommon2"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4e343f68-1aaa-0e84-7bab-eae458b68264" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "16e0b307-2b40-4584-294c-dee505e47827" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "87040ddf-f0cb-f894-dbbd-a4f266cae4e7" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "524c8f4f-cb50-ea04-dafd-e192f30c7656" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8aac4645-29cf-fe04-59c0-415508b0432e" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "934dae05-689f-30e4-5804-d497aee0b47c" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "af509c01-933e-ce14-49c3-106081e16d75" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9b70e722-0442-2924-092c-93efc1d92975" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "63a8ea74-65ee-c024-b8f0-87e173258257" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ea89b0d-7070-d3e4-a86f-c0e75c0e4ff5" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 15}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Rare_CandyCorn_Arms"
  value:
  {
    AuthoredId: "3fa2e317-0875-98a4-d9ca-7dc143c894d0"
    Name: "SweetenerReward_Armor_Rare_CandyCorn_Arms"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 6 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 7 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 4 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c" Quantity: 1 Weight: 50}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 5 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c" Quantity: 1 Weight: 50}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Epic_Gold_Waist"
  value:
  {
    AuthoredId: "68d828af-100a-8994-4912-9c19f61fc6d6"
    Name: "SweetenerReward_Armor_Epic_Gold_Waist"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 11 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "2f765969-c607-d4a4-0b9c-72d3cce7fbec" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 12 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "2f765969-c607-d4a4-0b9c-72d3cce7fbec" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 13 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "2f765969-c607-d4a4-0b9c-72d3cce7fbec" Quantity: 1 Weight: 25}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Common_Gumdrop_Legs"
  value:
  {
    AuthoredId: "6afb61f1-05e7-66d4-69cc-a0ea015d3f3d"
    Name: "SweetenerReward_Armor_Common_Gumdrop_Legs"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 17 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 15 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 14 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 16 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" Quantity: 1 Weight: 200}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Common_CandyButton_Arms"
  value:
  {
    AuthoredId: "30ec82b1-39b3-fe44-4997-840026b266d5"
    Name: "SweetenerReward_Armor_Common_CandyButton_Arms"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 6 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 7 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 4 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 5 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436" Quantity: 1 Weight: 200}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Common_FruitSlice_Arms"
  value:
  {
    AuthoredId: "36eda8a2-97bc-34d4-69fa-2daedc5043a8"
    Name: "SweetenerReward_Armor_Common_FruitSlice_Arms"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 6 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 7 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 4 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 5 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" Quantity: 1 Weight: 200}
  }
}
activityrewards:
{
  key: "SweetenerRewardsTable_Armor_Common"
  value:
  {
    AuthoredId: "f38319c2-1e6a-97d4-8976-5ecf1aee51ee"
    Name: "SweetenerRewardsTable_Armor_Common"
    Rewards: {ListTableId: "30ec82b1-39b3-fe44-4997-840026b266d5" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "c0ebb18f-d55e-13b4-58f1-2e2eafe35e19" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "36eda8a2-97bc-34d4-69fa-2daedc5043a8" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "5a5cdf57-3085-6654-68a3-f5914d0ef72c" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "6afb61f1-05e7-66d4-69cc-a0ea015d3f3d" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "47fe34c6-8617-5134-1b61-8de82611f625" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "9b95befc-dace-c414-1afb-590b3c18c075" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "77be7cb7-b88b-1a24-d97d-661e005c9150" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "463e6097-84d8-0324-ab8d-006829dac706" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 7 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Epic_Bronze_Legs"
  value:
  {
    AuthoredId: "0c12eefb-bd73-c5a4-a97d-5d07eb20c9d5"
    Name: "SweetenerReward_Armor_Epic_Bronze_Legs"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 15 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "a2c2697c-d738-c004-4b32-d800921bab06" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 14 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "a2c2697c-d738-c004-4b32-d800921bab06" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 17 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "a2c2697c-d738-c004-4b32-d800921bab06" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 16 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "a2c2697c-d738-c004-4b32-d800921bab06" Quantity: 1 Weight: 25}
  }
}
activityrewards:
{
  key: "SweetenerRewardsTable_Armor_Uncommon"
  value:
  {
    AuthoredId: "0f3cb80c-c70c-1234-8a1b-1d349037a0c7"
    Name: "SweetenerRewardsTable_Armor_Uncommon"
    Rewards: {ListTableId: "2436f49f-2eb5-23e4-cbae-c0985d81c6cf" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "fbf39341-9036-def4-e9c6-f15feb175e83" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "aeb02c5c-b10b-23b4-5bdb-3693ce7661ff" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "b96d908f-3a89-1564-5815-d88b8edf4602" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "f4188e48-3ef8-0694-8bdb-5796687013f9" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "0dfaecda-5ccb-faa4-ea9c-e6b71f040b9e" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 7 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
  }
}
activityrewards:
{
  key: "SweetenerRewardsTable_Animations_Common"
  value:
  {
    AuthoredId: "87d8fe8d-231a-6234-3af6-a9246f364e17"
    Name: "SweetenerRewardsTable_Animations_Common"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "27448f34-ca08-99b4-1b63-fd4b227e6af4" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "938e1ab9-a85b-5444-7bd6-f8862ace8b16" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "05633498-ace9-de14-c939-9435a6343d0f" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "5fce85e9-51f9-d7b4-db81-ef141439db00" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "917e27ec-19ff-3d04-69f6-c596456816e6" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "97c2c3da-93c7-a0d4-1967-eeba13396cce" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "a170f9f3-f97b-8eb4-49c0-c99a916e55d7" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 5 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "64732638-e355-d5c4-aa2d-e28c3db4aa72" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 7 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
  }
}
activityrewards:
{
  key: "RewardsTable_1stTourneyLoser"
  value:
  {
    AuthoredId: "e6d106ef-38e1-67d4-db02-fd1fde4d94a1"
    Name: "RewardsTable_1stTourneyLoser"
    Rewards: {ListTableId: "9c7afb2a-a12c-5534-e86d-73d74e1fe212" Type: 6 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
  }
}
activityrewards:
{
  key: "SweetenersRewardsTable_Moves_Common_a"
  value:
  {
    AuthoredId: "70c75ebc-7d57-f124-6bc4-d0b7b7ff3d37"
    Name: "SweetenersRewardsTable_Moves_Common_a"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "0090580c-04ef-9d84-e883-32f52c977b98" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "a9d69fc0-a1b8-ae24-cbd7-249760200211" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "2c555752-8a84-58f4-395e-6460b7864069" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "86b323c2-b2fd-2494-ab5e-bc3514bc92d8" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "9fd0b9ef-2e7d-d344-8926-69f1fab99f65" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
  }
}
activityrewards:
{
  key: "SweetenersRewardsTable_Moves_Common_b"
  value:
  {
    AuthoredId: "30d9967d-8618-7254-3bff-46e09e717719"
    Name: "SweetenersRewardsTable_Moves_Common_b"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "1e47a8e0-acfd-1244-0b7d-539d62429060" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "628ebc0b-bb19-ee64-68b8-2f66f4b92720" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "c28e13b4-cb1f-6ea4-680f-91010a400e10" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "bd27fe1c-ce9a-f524-c887-61f9e4d5088b" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "29d559bb-41a7-9f14-5bf8-605da8166943" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "1d1dec74-ca35-0cc4-d930-164786a9dd81" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
  }
}
activityrewards:
{
  key: "RewardTable_Expedition_T4Epic1"
  value:
  {
    AuthoredId: "42ffc503-545f-61f4-1b57-e65dee1749e0"
    Name: "RewardTable_Expedition_T4Epic1"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "af509c01-933e-ce14-49c3-106081e16d75" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9b70e722-0442-2924-092c-93efc1d92975" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "63a8ea74-65ee-c024-b8f0-87e173258257" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ea89b0d-7070-d3e4-a86f-c0e75c0e4ff5" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9c64b7e4-0458-65c4-9b79-5cc35e847f9c" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "1525c955-d3c0-9f84-69fb-cf98837f4f85" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "86b5fac0-4ec1-09e4-6ab7-96d3fd13dd04" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9ff884ab-efcb-0af4-5acd-976fea43b3a3" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "33d499a1-9dee-dfd4-7843-c743a6131dcf" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "28089a53-62ac-2044-1964-29f2f31d8a33" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8932caea-da2e-6384-58ec-749624e213c9" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "dd29bd35-bb51-8d94-4913-8ca943d062f1" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8f896243-a6de-1b54-68a3-0df72e8688e4" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "a2c2697c-d738-c004-4b32-d800921bab06" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8aa0e6a7-0b45-3404-9b39-dc4a4300a87f" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "aa0cb18d-5681-5504-09d7-4db06bf7a4d8" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "48ea4c49-79dd-6514-6b8b-f87fd6d8a330" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "1f089286-ff92-dcc4-b85c-7ebf28d2cc4f" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 3 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 4 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 2}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "df921272-743b-5f14-1a81-710a1b97d72e" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 2}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "c1e9a4e1-cf55-6084-8bf2-778678387353" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 2}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "3f5314d7-0755-ab64-8a9c-589d3158de7a" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 2}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "b4efa30e-0d76-5a04-b8fb-0a5c7b02d4a3" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 2}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "71a69f03-1e74-0ed4-6994-54b900eb0cc9" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 2}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "cf363926-b8c5-8294-099a-5fd770749258" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 2}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "f13487ec-4bf1-7ec4-e9d6-fd043f9842d8" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 2}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "390c2aa8-6b27-9a04-c820-835a0d178686" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 2}
  }
}
activityrewards:
{
  key: "RewardTable_Expedition_T4Epic2"
  value:
  {
    AuthoredId: "45b14a76-3b9c-2014-7bc5-816c7de2f947"
    Name: "RewardTable_Expedition_T4Epic2"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "af509c01-933e-ce14-49c3-106081e16d75" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9b70e722-0442-2924-092c-93efc1d92975" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "63a8ea74-65ee-c024-b8f0-87e173258257" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ea89b0d-7070-d3e4-a86f-c0e75c0e4ff5" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9c64b7e4-0458-65c4-9b79-5cc35e847f9c" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "1525c955-d3c0-9f84-69fb-cf98837f4f85" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "86b5fac0-4ec1-09e4-6ab7-96d3fd13dd04" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9ff884ab-efcb-0af4-5acd-976fea43b3a3" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "33d499a1-9dee-dfd4-7843-c743a6131dcf" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "28089a53-62ac-2044-1964-29f2f31d8a33" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "499a8021-eaea-c7d4-aa1c-bb6e55cd300c" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "bbeb0e31-3775-a864-1952-082233cf7b89" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "08847311-c629-a154-5958-13e8f1d77f88" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "3b01bcce-f26c-66c4-68a5-80ab4d67f28c" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "92954ab6-fe54-4a64-d942-feca64e9e931" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "eeeca33e-5c80-9ec4-eb83-46e4b7daf04b" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f7668430-bfc1-71f4-29b8-f36c7d6905d5" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "5b187f8e-d5ff-36d4-7b56-8be9cce004c8" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "321c2a65-64b1-8344-2915-85562e2ad7ec" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 3 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 4 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 2}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "ec86d547-e1d0-6324-bb20-e0a20e116fb2" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 2}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "b5bb8cce-20bf-f5a4-3a51-694c37b8f9b9" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 2}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "5124cf4a-b08e-2dd4-6809-ed15f73ae685" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 2}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "c4c807ca-431a-9184-da8e-2c9993d1bba0" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 2}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "9d7d627c-8627-b934-6b2a-f645d96447ea" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 2}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "40f97352-9e9e-39d4-a830-6672d1ed46f6" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 2}
  }
}
activityrewards:
{
  key: "RewardTable_Expedition_T4Epic3"
  value:
  {
    AuthoredId: "18bce352-f1f1-c894-5807-0cc4e10c0f1a"
    Name: "RewardTable_Expedition_T4Epic3"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "af509c01-933e-ce14-49c3-106081e16d75" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9b70e722-0442-2924-092c-93efc1d92975" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "63a8ea74-65ee-c024-b8f0-87e173258257" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ea89b0d-7070-d3e4-a86f-c0e75c0e4ff5" Quantity: 10 Weight: 70}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9c64b7e4-0458-65c4-9b79-5cc35e847f9c" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "e5afded6-705a-2034-7b1c-556ac877fb0f" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "1525c955-d3c0-9f84-69fb-cf98837f4f85" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "86b5fac0-4ec1-09e4-6ab7-96d3fd13dd04" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "9ff884ab-efcb-0af4-5acd-976fea43b3a3" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "33d499a1-9dee-dfd4-7843-c743a6131dcf" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "28089a53-62ac-2044-1964-29f2f31d8a33" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d" Quantity: 10 Weight: 40}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "47b8a853-791a-1424-1ba9-4adaf4a3f4a5" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8898cfa3-1512-22e4-0b9f-c6e220f315b5" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "0947de60-3149-c2d4-dad0-d4e67db681b6" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f6c3b895-6488-1b74-a9b0-664027a683e2" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "a1af5095-bfbc-1a84-7862-9b246dc29d81" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "52ea4044-e203-16d4-c90f-8b685ed5e9c9" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "57674941-fe8c-5b24-0b69-16fb1d22ee62" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "2f765969-c607-d4a4-0b9c-72d3cce7fbec" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "b4e362bb-2634-7df4-39a6-71b358368a6d" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8898194e-ac52-f624-a830-176655bb43cd" Quantity: 10 Weight: 20}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 3 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 7}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 4 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 2}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 2 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 15}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 1 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 30}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "f0d9f776-2d8e-1e04-da62-fb62623b7d2e" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 2}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "b192f393-c1ac-3364-a8a0-dd5582933e66" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 2}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "7c922dce-d72d-2544-fbc3-7abb2a12c92e" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 2}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "30a597d1-31e7-3474-4b15-d7ff87f384f8" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 2}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "c481aeee-d1a1-01c4-7aca-92d0edcddf18" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 2}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "edc99c4e-6e95-5ae4-ea8c-e6071cd8635a" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 2}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Rare_HardCandy_Legs"
  value:
  {
    AuthoredId: "82741761-1136-4b94-c90e-22b41bbeb41f"
    Name: "SweetenerReward_Armor_Rare_HardCandy_Legs"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 17 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 15 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 14 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" Quantity: 1 Weight: 25}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 16 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b" Quantity: 1 Weight: 25}
  }
}
activityrewards:
{
  key: "SweetenerRewardsList_ArmorSet_Common_Licorice_Torso"
  value:
  {
    AuthoredId: "9b95befc-dace-c414-1afb-590b3c18c075"
    Name: "SweetenerReward_Armor_Common_Licorice_Torso"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 18 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "524c8f4f-cb50-ea04-dafd-e192f30c7656" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 9 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "524c8f4f-cb50-ea04-dafd-e192f30c7656" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 8 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "524c8f4f-cb50-ea04-dafd-e192f30c7656" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 4 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 10 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "524c8f4f-cb50-ea04-dafd-e192f30c7656" Quantity: 1 Weight: 200}
  }
}
activityrewards:
{
  key: "SweetenersRewardsTable_Moves_Uncommon_a"
  value:
  {
    AuthoredId: "2913144c-50ad-ccf4-ebdc-fb5d2286b88c"
    Name: "SweetenersRewardsTable_Moves_Uncommon_a"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "431900a2-54d4-df34-d9f7-6a4608fecfaa" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "86bd4f81-e96b-9864-78b5-5fc51fbff6a4" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "e201f964-8949-9d74-fbee-4ee8786284ca" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "0090580c-04ef-9d84-e883-32f52c977b98" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "a9d69fc0-a1b8-ae24-cbd7-249760200211" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "2c555752-8a84-58f4-395e-6460b7864069" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "86b323c2-b2fd-2494-ab5e-bc3514bc92d8" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "9fd0b9ef-2e7d-d344-8926-69f1fab99f65" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
  }
}
activityrewards:
{
  key: "TounamentRewardsList_1stTourneyLoser"
  value:
  {
    AuthoredId: "9c7afb2a-a12c-5534-e86d-73d74e1fe212"
    Name: "TounamentRewardsList_1stTourneyLoser"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 2 GeneratedRecipeQuality: 0 CraftedRecipeId: "1bbc7d99-7fce-24a4-c9a3-dfaf4b744efa" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 1}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "7ddba6f2-b08a-f954-4a97-eae6af175005" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "8aac4645-29cf-fe04-59c0-415508b0432e" Quantity: 10 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 0 GeneratedRecipeQuality: 1 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "00000000-0000-0000-0000-000000000000" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" Quantity: 10 Weight: 100}
  }
}
activityrewards:
{
  key: "SweetenersRewardsTable_Moves_Uncommon_b"
  value:
  {
    AuthoredId: "94f80543-5661-ff84-8860-1e9b222d6c48"
    Name: "SweetenersRewardsTable_Moves_Uncommon_b"
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "9f0df0ed-13a6-84c4-781d-9dbf9741c66e" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "b1bd32de-88b2-1204-7b2e-0680677d78c4" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "fa6144f9-ad49-8124-386f-351a7f1ab546" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "b889835e-1cd6-d3d4-f9b4-dd1989f49dac" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 100}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "1d1dec74-ca35-0cc4-d930-164786a9dd81" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "1e47a8e0-acfd-1244-0b7d-539d62429060" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "628ebc0b-bb19-ee64-68b8-2f66f4b92720" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "c28e13b4-cb1f-6ea4-680f-91010a400e10" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "bd27fe1c-ce9a-f524-c887-61f9e4d5088b" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
    Rewards: {ListTableId: "00000000-0000-0000-0000-000000000000" Type: 3 GeneratedRecipeQuality: 0 CraftedRecipeId: "00000000-0000-0000-0000-000000000000" MoveId: "29d559bb-41a7-9f14-5bf8-605da8166943" ArmorType: 0 AnimationId: "00000000-0000-0000-0000-000000000000" CandyType: "00000000-0000-0000-0000-000000000000" Quantity: 1 Weight: 200}
  }
}
animations:
{
  key: "Animation_Common_Uppercut"
  value:
  {
    AuthoredId: "a170f9f3-f97b-8eb4-49c0-c99a916e55d7"
    Name: "Uppercut"
    Quality: 1
    Clip: "PlayUppercut"
  }
}
animations:
{
  key: "Animation_Common_HammerTime"
  value:
  {
    AuthoredId: "05633498-ace9-de14-c939-9435a6343d0f"
    Name: "Hammer Time"
    Quality: 1
    Clip: "PlayHammerTime"
  }
}
animations:
{
  key: "Animation_Common_RoundhouseKick"
  value:
  {
    AuthoredId: "97c2c3da-93c7-a0d4-1967-eeba13396cce"
    Name: "Roundhouse Kick"
    Quality: 1
    Clip: "PlayRoundhouseKick"
  }
}
animations:
{
  key: "Animation_Epic_DempseyRoll"
  value:
  {
    AuthoredId: "471eee2f-e6ad-a694-58de-bdee5a379096"
    Name: "Dempsey Roll"
    Quality: 4
    Clip: "PlayDempseyRoll"
  }
}
animations:
{
  key: "Animation_Common_ElbowStrike"
  value:
  {
    AuthoredId: "938e1ab9-a85b-5444-7bd6-f8862ace8b16"
    Name: "Elbow Strike"
    Quality: 1
    Clip: "PlayElbowStrike"
  }
}
animations:
{
  key: "Animation_Uncommon_ThreeQuickSpins"
  value:
  {
    AuthoredId: "ed324300-aa2c-0c04-4b75-2d0eae3780f4"
    Name: "Three Quick Spins"
    Quality: 2
    Clip: "PlayThreeQuickSpins"
  }
}
animations:
{
  key: "Animation_Common_RightCross"
  value:
  {
    AuthoredId: "917e27ec-19ff-3d04-69f6-c596456816e6"
    Name: "Right Cross"
    Quality: 1
    Clip: "PlayRightCross"
  }
}
animations:
{
  key: "Animation_Common_DoublePunch"
  value:
  {
    AuthoredId: "27448f34-ca08-99b4-1b63-fd4b227e6af4"
    Name: "Double Punch"
    Quality: 1
    Clip: "PlayDoublePunch"
  }
}
animations:
{
  key: "Animation_Common_JumpingSidekick"
  value:
  {
    AuthoredId: "5fce85e9-51f9-d7b4-db81-ef141439db00"
    Name: "Jumping Sidekick"
    Quality: 1
    Clip: "PlayJumpingSidekick"
  }
}
animations:
{
  key: "Animation_Uncommon_OneTwo"
  value:
  {
    AuthoredId: "a91a62fa-8d65-6234-d91b-a0ad0d9388e3"
    Name: "One Two"
    Quality: 2
    Clip: "PlayOneTwo"
  }
}
animations:
{
  key: "Animation_Epic_HulkHogan"
  value:
  {
    AuthoredId: "b0877d2a-71db-7374-8a8b-8044f7a8a52a"
    Name: "Hulk Poses"
    Quality: 4
    Clip: "PlayHulkHogan"
  }
}
animations:
{
  key: "Animation_Rare_DropKick"
  value:
  {
    AuthoredId: "2831df30-e1a2-dcf4-2a9a-0c91ac4ea310"
    Name: "Drop Kick"
    Quality: 3
    Clip: "PlayDropKick"
  }
}
animations:
{
  key: "Animation_Epic_CraneKick"
  value:
  {
    AuthoredId: "3afdf3ed-bea4-1c14-faac-159412578c0f"
    Name: "Crane Kick"
    Quality: 4
    Clip: "PlayCraneKick"
  }
}
animations:
{
  key: "Animation_Rare_BoxingTheEars"
  value:
  {
    AuthoredId: "c6e3c59b-4415-52b4-8b34-1593bbc26634"
    Name: "Boxing The Ears"
    Quality: 3
    Clip: "PlayBoxingYourEars"
  }
}
animations:
{
  key: "Animation_Common_ThreeQuickJabs"
  value:
  {
    AuthoredId: "64732638-e355-d5c4-aa2d-e28c3db4aa72"
    Name: "Three Quick Jabs"
    Quality: 1
    Clip: "PlayTrippleJab"
  }
}
animations:
{
  key: "Animation_Rare_SlicingChopping"
  value:
  {
    AuthoredId: "02ddcd6e-e441-73f4-e874-399531dc4708"
    Name: "Slicing Chopping"
    Quality: 3
    Clip: "PlaySlicingChopping"
  }
}
animations:
{
  key: "Animation_Epic_BruceLeeTaunt"
  value:
  {
    AuthoredId: "7294e11a-f8b2-9d34-5854-fee4535f1aaa"
    Name: "Bruce Lee Taunt"
    Quality: 4
    Clip: "PlayBruceLeeTaunt"
  }
}
animations:
{
  key: "Animation_Uncommon_Haymaker"
  value:
  {
    AuthoredId: "a2a185e5-4543-6814-d88d-d078a0e38521"
    Name: "Haymaker"
    Quality: 2
    Clip: "PlayHaymaker"
  }
}
animations:
{
  key: "Animation_Epic_Shoryuken"
  value:
  {
    AuthoredId: "4fca9718-5e62-8924-7a58-790c249fbc7f"
    Name: "Shoryuken"
    Quality: 4
    Clip: "PlsyShoryuken"
  }
}
animations:
{
  key: "Animation_Rare_SpinningBackhand"
  value:
  {
    AuthoredId: "0c2cdea6-ff22-b124-da21-882987b1707a"
    Name: "Spinning Backhand"
    Quality: 3
    Clip: "PlaySpinningBlow"
  }
}
animations:
{
  key: "Animation_Uncommon_WaxOnWaxOff"
  value:
  {
    AuthoredId: "d35a9d1c-2223-c404-c840-c0218f9b147d"
    Name: "Wax On Wax Off"
    Quality: 2
    Clip: "PlayWaxOnWaxOff"
  }
}
animations:
{
  key: "Animation_Uncommon_DownwardFistSmash"
  value:
  {
    AuthoredId: "f33c74e6-f1b4-6ca4-bb5a-cf6edc77cab5"
    Name: "Downward Fist Smash"
    Quality: 2
    Clip: "PlayDownward"
  }
}
animations:
{
  key: "Animation_Rare_RollingDodges"
  value:
  {
    AuthoredId: "a7629ff3-c1ef-e664-3a9c-bd27cb332abd"
    Name: "Rolling Dodges"
    Quality: 3
    Clip: "PlayRollingDodges"
  }
}
candies:
{
  key: "Common_Gumdrop"
  value:
  {
    AuthoredId: "c2774273-0cd4-2494-fa02-76cffe1ef1ef"
    Name: "Gumdrop"
  }
}
candies:
{
  key: "Common_Candy Button"
  value:
  {
    AuthoredId: "f6025c1b-55e8-da24-d96f-0ecebf74c436"
    Name: "Candy Button"
  }
}
candies:
{
  key: "Uncommon_Marshmallow"
  value:
  {
    AuthoredId: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af"
    Name: "Marshmallow"
  }
}
candies:
{
  key: "Epic_Bronze Mint Patty"
  value:
  {
    AuthoredId: "1f089286-ff92-dcc4-b85c-7ebf28d2cc4f"
    Name: "Bronzed Mint Patty"
  }
}
candies:
{
  key: "Epic_Silver Icing"
  value:
  {
    AuthoredId: "0ee1131a-b8e4-05c4-9896-3690ebf8563a"
    Name: "Silver Icing"
  }
}
candies:
{
  key: "Epic_Silver Fruit Slices"
  value:
  {
    AuthoredId: "bbeb0e31-3775-a864-1952-082233cf7b89"
    Name: "Silver Fruit Slice"
  }
}
candies:
{
  key: "Epic_Silver Jelly Beans"
  value:
  {
    AuthoredId: "92954ab6-fe54-4a64-d942-feca64e9e931"
    Name: "Silver Jelly Beans"
  }
}
candies:
{
  key: "Epic_Silver Peppermint"
  value:
  {
    AuthoredId: "5b187f8e-d5ff-36d4-7b56-8be9cce004c8"
    Name: "Silver Peppermint"
  }
}
candies:
{
  key: "Uncommon_Jelly Bean"
  value:
  {
    AuthoredId: "9b70e722-0442-2924-092c-93efc1d92975"
    Name: "Jelly Bean"
  }
}
candies:
{
  key: "Common_Fizzy Powder"
  value:
  {
    AuthoredId: "16e0b307-2b40-4584-294c-dee505e47827"
    Name: "Fizzy Powder"
  }
}
candies:
{
  key: "Epic_Gold Candy Ribbon"
  value:
  {
    AuthoredId: "47b8a853-791a-1424-1ba9-4adaf4a3f4a5"
    Name: "Golden Candy Ribbon"
  }
}
candies:
{
  key: "Epic_Bronze Icing"
  value:
  {
    AuthoredId: "8aa0e6a7-0b45-3404-9b39-dc4a4300a87f"
    Name: "Bronzed Icing"
  }
}
candies:
{
  key: "Epic_Bronze Gummy Bear"
  value:
  {
    AuthoredId: "a2c2697c-d738-c004-4b32-d800921bab06"
    Name: "Bronzed Gummy Bear"
  }
}
candies:
{
  key: "Epic_Gold Giant Chocolate Chip"
  value:
  {
    AuthoredId: "f6c3b895-6488-1b74-a9b0-664027a683e2"
    Name: "Golden Giant Chocolate Chip"
  }
}
candies:
{
  key: "Rare_Mint Patty"
  value:
  {
    AuthoredId: "9ff884ab-efcb-0af4-5acd-976fea43b3a3"
    Name: "Mint Patty"
  }
}
candies:
{
  key: "Rare_Gummi Bear"
  value:
  {
    AuthoredId: "af509c01-933e-ce14-49c3-106081e16d75"
    Name: "Gummi Bear"
  }
}
candies:
{
  key: "Rare_Mint Stick"
  value:
  {
    AuthoredId: "33d499a1-9dee-dfd4-7843-c743a6131dcf"
    Name: "Mint Stick"
  }
}
candies:
{
  key: "Epic_Gold Circus Peanuts"
  value:
  {
    AuthoredId: "0947de60-3149-c2d4-dad0-d4e67db681b6"
    Name: "Golden Circus Peanuts"
  }
}
candies:
{
  key: "Epic_Gold Toffee Chunk"
  value:
  {
    AuthoredId: "8898194e-ac52-f624-a830-176655bb43cd"
    Name: "Golden Toffee Chunk"
  }
}
candies:
{
  key: "Common_Fruit Slice"
  value:
  {
    AuthoredId: "12ea78d4-e0f3-e5b4-1882-04787fa86d15"
    Name: "Fruit Slice"
  }
}
candies:
{
  key: "Epic_Bronze Gum Drop"
  value:
  {
    AuthoredId: "8f896243-a6de-1b54-68a3-0df72e8688e4"
    Name: "Bronzed Gum Drop"
  }
}
candies:
{
  key: "Epic_Gold Gum Ball"
  value:
  {
    AuthoredId: "a1af5095-bfbc-1a84-7862-9b246dc29d81"
    Name: "Golden Gum Ball"
  }
}
candies:
{
  key: "Common_Gumball"
  value:
  {
    AuthoredId: "87040ddf-f0cb-f894-dbbd-a4f266cae4e7"
    Name: "Gumball"
  }
}
candies:
{
  key: "Rare_Circus Peanuts"
  value:
  {
    AuthoredId: "1525c955-d3c0-9f84-69fb-cf98837f4f85"
    Name: "Circus Peanuts"
  }
}
candies:
{
  key: "Rare_Licorice Piece"
  value:
  {
    AuthoredId: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c"
    Name: "Licorice Piece"
  }
}
candies:
{
  key: "Epic_Silver Hard Candy"
  value:
  {
    AuthoredId: "3b01bcce-f26c-66c4-68a5-80ab4d67f28c"
    Name: "Silver Hard Candy"
  }
}
candies:
{
  key: "Uncommon_Chocolate Nut"
  value:
  {
    AuthoredId: "40067d7d-6651-cbc4-c8ed-785d73e9ab57"
    Name: "Chocolate Nut"
  }
}
candies:
{
  key: "Uncommon_Peppermint"
  value:
  {
    AuthoredId: "63a8ea74-65ee-c024-b8f0-87e173258257"
    Name: "Peppermint"
  }
}
candies:
{
  key: "Epic_Gold Non Pareil"
  value:
  {
    AuthoredId: "b4e362bb-2634-7df4-39a6-71b358368a6d"
    Name: "Golden Non Pareil"
  }
}
candies:
{
  key: "Rare_Candy Ribbon"
  value:
  {
    AuthoredId: "e5afded6-705a-2034-7b1c-556ac877fb0f"
    Name: "Candy Ribbon"
  }
}
candies:
{
  key: "Epic_Bronze Candy Corn"
  value:
  {
    AuthoredId: "8932caea-da2e-6384-58ec-749624e213c9"
    Name: "Bronzed Candy Corn"
  }
}
candies:
{
  key: "Epic_Silver Candy Cane"
  value:
  {
    AuthoredId: "499a8021-eaea-c7d4-aa1c-bb6e55cd300c"
    Name: "Silver Candy Cane"
  }
}
candies:
{
  key: "Common_Icing"
  value:
  {
    AuthoredId: "f9c0a4d2-f049-29a4-eb1e-2269de35798e"
    Name: "Icing"
  }
}
candies:
{
  key: "Rare_Peanut Butter Cup"
  value:
  {
    AuthoredId: "28089a53-62ac-2044-1964-29f2f31d8a33"
    Name: "Peanut Butter Cup"
  }
}
candies:
{
  key: "Epic_Silver Licorice Piece"
  value:
  {
    AuthoredId: "eeeca33e-5c80-9ec4-eb83-46e4b7daf04b"
    Name: "Silver Licorice Piece"
  }
}
candies:
{
  key: "Common_Chocolate Chip"
  value:
  {
    AuthoredId: "7ddba6f2-b08a-f954-4a97-eae6af175005"
    Name: "Chocolate Chip"
  }
}
candies:
{
  key: "Common_Sugar Glaze"
  value:
  {
    AuthoredId: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7"
    Name: "Sugar Glaze"
  }
}
candies:
{
  key: "Common_Ring Pop"
  value:
  {
    AuthoredId: "934dae05-689f-30e4-5804-d497aee0b47c"
    Name: "Ring Pop"
  }
}
candies:
{
  key: "Rare_Jawbreaker"
  value:
  {
    AuthoredId: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9"
    Name: "Jaw Breaker"
  }
}
candies:
{
  key: "Rare_Toffee Chunk"
  value:
  {
    AuthoredId: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d"
    Name: "Toffee Chunk"
  }
}
candies:
{
  key: "Epic_Silver MInt Stick"
  value:
  {
    AuthoredId: "f7668430-bfc1-71f4-29b8-f36c7d6905d5"
    Name: "Silver Mint Stick"
  }
}
candies:
{
  key: "Rare_Hard Candy"
  value:
  {
    AuthoredId: "d6478df9-c705-98c4-ebb6-2ffab7dd798b"
    Name: "Hard Candy"
  }
}
candies:
{
  key: "Uncommon_Cinnamon Ball"
  value:
  {
    AuthoredId: "1983b3b7-4a9e-8f14-b981-c94c32d7648b"
    Name: "Cinnamon Ball"
  }
}
candies:
{
  key: "Epic_Gold Hard Candy"
  value:
  {
    AuthoredId: "52ea4044-e203-16d4-c90f-8b685ed5e9c9"
    Name: "Golden Hard Candy"
  }
}
candies:
{
  key: "Common_Nonpareil"
  value:
  {
    AuthoredId: "8aac4645-29cf-fe04-59c0-415508b0432e"
    Name: "Nonpareil"
  }
}
candies:
{
  key: "Rare_Candy Corn"
  value:
  {
    AuthoredId: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c"
    Name: "Candy Corn"
  }
}
candies:
{
  key: "Epic_Gold Icing"
  value:
  {
    AuthoredId: "57674941-fe8c-5b24-0b69-16fb1d22ee62"
    Name: "Golden Icing"
  }
}
candies:
{
  key: "Rare_Candy Loops"
  value:
  {
    AuthoredId: "9c64b7e4-0458-65c4-9b79-5cc35e847f9c"
    Name: "Fruity Candy Loops"
  }
}
candies:
{
  key: "Uncommon_Banana Chew"
  value:
  {
    AuthoredId: "5a087336-6c2d-a684-1b72-a86d6ff6931f"
    Name: "Banana Chew"
  }
}
candies:
{
  key: "Epic_Bronze Jelly Bean"
  value:
  {
    AuthoredId: "48ea4c49-79dd-6514-6b8b-f87fd6d8a330"
    Name: "Bronzed Jelly Bean"
  }
}
candies:
{
  key: "Epic_Bronze Jawbreaker"
  value:
  {
    AuthoredId: "aa0cb18d-5681-5504-09d7-4db06bf7a4d8"
    Name: "Bronzed Jawbreaker"
  }
}
candies:
{
  key: "Epic_Silver Gum Drop"
  value:
  {
    AuthoredId: "08847311-c629-a154-5958-13e8f1d77f88"
    Name: "Silver Gum Drop"
  }
}
candies:
{
  key: "Uncommon_Cotton Candy"
  value:
  {
    AuthoredId: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1"
    Name: "Cotton Candy"
  }
}
candies:
{
  key: "Epic_Gold Jelly Bean"
  value:
  {
    AuthoredId: "2f765969-c607-d4a4-0b9c-72d3cce7fbec"
    Name: "Golden Jelly Bean"
  }
}
candies:
{
  key: "Epic_Silver Ring Pop"
  value:
  {
    AuthoredId: "321c2a65-64b1-8344-2915-85562e2ad7ec"
    Name: "Silver Ring Pop"
  }
}
candies:
{
  key: "Rare_Giant Chocolate Chip"
  value:
  {
    AuthoredId: "86b5fac0-4ec1-09e4-6ab7-96d3fd13dd04"
    Name: "Giant Chocolate Chip"
  }
}
candies:
{
  key: "Common_Licorice Lace"
  value:
  {
    AuthoredId: "524c8f4f-cb50-ea04-dafd-e192f30c7656"
    Name: "Licorice Lace"
  }
}
candies:
{
  key: "Epic_Gold Chocolate Bar"
  value:
  {
    AuthoredId: "8898cfa3-1512-22e4-0b9f-c6e220f315b5"
    Name: "Golden Chocolate Bar"
  }
}
candies:
{
  key: "Epic_Bronze Gum Ball"
  value:
  {
    AuthoredId: "dd29bd35-bb51-8d94-4913-8ca943d062f1"
    Name: "Bronzed Gum Ball"
  }
}
candies:
{
  key: "Common_Candy Cane"
  value:
  {
    AuthoredId: "4e343f68-1aaa-0e84-7bab-eae458b68264"
    Name: "Candy Cane"
  }
}
candies:
{
  key: "Uncommon_Rock Candy"
  value:
  {
    AuthoredId: "7ea89b0d-7070-d3e4-a86f-c0e75c0e4ff5"
    Name: "Rock Candy"
  }
}
crystalbundles:
{
  key: "CrystalsPurchase_T1"
  value:
  {
    AuthoredId: "b73860f9-de7b-b1e4-a993-0f8384889746"
    Quantity: 100
    CHICost: 1
  }
}
crystalbundles:
{
  key: "CrystalsPurchase_T2"
  value:
  {
    AuthoredId: "8b94c00c-8f7e-aba4-b9f3-08587790c9f2"
    Quantity: 220
    CHICost: 2
  }
}
crystalbundles:
{
  key: "CrystalsPurchase_T3"
  value:
  {
    AuthoredId: "0cc63ecf-fe20-41f4-cb0b-990951d403e8"
    Quantity: 580
    CHICost: 3
  }
}
crystalbundles:
{
  key: "CrystalsPurchase_T4"
  value:
  {
    AuthoredId: "59a5e6c2-c212-6a14-79a2-d640c57cf21f"
    Quantity: 1350
    CHICost: 4
  }
}
crystalbundles:
{
  key: "CrystalsPurchase_T5"
  value:
  {
    AuthoredId: "de198dca-d270-cd64-19c2-14b7734896c8"
    Quantity: 3000
    CHICost: 5
  }
}
crystalbundles:
{
  key: "CrystalsPurchase_T6"
  value:
  {
    AuthoredId: "5cd0fe1e-1d73-4b44-da15-f6b2bb380593"
    Quantity: 11000
    CHICost: 6
  }
}
expeditionblueprints:
{
  key: "Expedition_Rank9_ E2"
  value:
  {
    AuthoredId: "5c5c9f0e-4eda-bbe4-ba92-ec3c400d081e"
    Name: "The Foot of Hard Candy Mountain: The Abandoned City"
    Duration: 1400
    MinSweetness: 9
    MaxSweetness: 10
    MaxRewardQuality: 4
    BaseRewardsTableId: "42ffc503-545f-61f4-1b57-e65dee1749e0"
    BaseRollCount: 10
  }
}
expeditionblueprints:
{
  key: "Expedition_Rank4_ E1"
  value:
  {
    AuthoredId: "5edaf5d1-ec6d-7094-a8dd-a344d0650e5c"
    Name: "The Licorice Forest Deep: Low Hanging Fruit"
    Duration: 125
    MinSweetness: 4
    MaxSweetness: 10
    MaxRewardQuality: 2
    BaseRewardsTableId: "bf5e7bbe-2361-80b4-f862-acdfa491c07e"
    BaseRollCount: 6
  }
}
expeditionblueprints:
{
  key: "Expedition_Rank10_ E1"
  value:
  {
    AuthoredId: "aece8a9f-30c0-3aa4-7a9b-5c33fc2e534a"
    Name: "Hard Candy Mountain: Slow but Steady Ascent"
    Duration: 2800
    MinSweetness: 10
    MaxSweetness: 10
    MaxRewardQuality: 4
    BaseRewardsTableId: "45b14a76-3b9c-2014-7bc5-816c7de2f947"
    BaseRollCount: 10
  }
}
expeditionblueprints:
{
  key: "Expedition_Rank7_ E1"
  value:
  {
    AuthoredId: "16457184-f0f1-73e4-183d-e7f8f2d412c2"
    Name: "Lake Fruity Punch: Scouring the Shore"
    Duration: 700
    MinSweetness: 7
    MaxSweetness: 10
    MaxRewardQuality: 3
    BaseRewardsTableId: "1f9dc820-9e7b-0f04-3944-a13ef7236ca0"
    BaseRollCount: 6
  }
}
expeditionblueprints:
{
  key: "Expedition_Rank1_ E2"
  value:
  {
    AuthoredId: "a2512eaa-028a-1f84-6879-eb240ac80a3e"
    Name: "The Graveyard of the Glazed: Excavation"
    Duration: 15
    MinSweetness: 1
    MaxSweetness: 10
    MaxRewardQuality: 1
    BaseRewardsTableId: "8ba8347d-fb20-adf4-ca16-93a8943ddd4f"
    BaseRollCount: 3
  }
}
expeditionblueprints:
{
  key: "Expedition_Rank6_ E1"
  value:
  {
    AuthoredId: "dc498167-f8b7-3164-da5b-776b731a1a6c"
    Name: "The Marshmallow Hills: Among the Ruins"
    Duration: 700
    MinSweetness: 6
    MaxSweetness: 10
    MaxRewardQuality: 3
    BaseRewardsTableId: "a2d81a2a-60e6-f354-6943-40803787242f"
    BaseRollCount: 5
  }
}
expeditionblueprints:
{
  key: "Expedition_Rank9_ E1"
  value:
  {
    AuthoredId: "8928eb04-3ee7-f834-9b5b-8b6f48d04ecf"
    Name: "The Foot of Hard Candy Mountain: Taking it Slow"
    Duration: 1400
    MinSweetness: 9
    MaxSweetness: 10
    MaxRewardQuality: 4
    BaseRewardsTableId: "18bce352-f1f1-c894-5807-0cc4e10c0f1a"
    BaseRollCount: 10
  }
}
expeditionblueprints:
{
  key: "Expedition_1stExpedition"
  value:
  {
    AuthoredId: "c064e7f7-acbf-4f74-fab8-cccd7b2d4004"
    Name: "A WARRIOR'S FIRST JOURNEY"
    Duration: 1
    MinSweetness: 1
    MaxSweetness: 1
    MaxRewardQuality: 1
    BaseRewardsTableId: "c1f4ec37-bb53-fed4-09e0-e19bb5b346bb"
    BaseRollCount: 1
  }
}
expeditionblueprints:
{
  key: "Expedition_Rank8_ E1"
  value:
  {
    AuthoredId: "09b9f6ba-cc74-6ad4-dbe9-86759590d0fc"
    Name: "The Caramelized Valley: Down into theValley"
    Duration: 700
    MinSweetness: 8
    MaxSweetness: 10
    MaxRewardQuality: 4
    BaseRewardsTableId: "42ffc503-545f-61f4-1b57-e65dee1749e0"
    BaseRollCount: 7
  }
}
expeditionblueprints:
{
  key: "Expedition_Rank2_ E2"
  value:
  {
    AuthoredId: "a1387c26-3e1c-4554-e94e-e857643e58ed"
    Name: "The Fizzy Swamp: Delving the Muck"
    Duration: 30
    MinSweetness: 2
    MaxSweetness: 10
    MaxRewardQuality: 1
    BaseRewardsTableId: "8ba8347d-fb20-adf4-ca16-93a8943ddd4f"
    BaseRollCount: 4
  }
}
expeditionblueprints:
{
  key: "Expedition_Rank5_ E1"
  value:
  {
    AuthoredId: "0d9af5ec-b3fd-3de4-2a38-0a954c9f9cba"
    Name: "The Great Coco River: Scouring the Shore"
    Duration: 350
    MinSweetness: 5
    MaxSweetness: 10
    MaxRewardQuality: 3
    BaseRewardsTableId: "1f9dc820-9e7b-0f04-3944-a13ef7236ca0"
    BaseRollCount: 4
  }
}
expeditionblueprints:
{
  key: "Expedition_Rank1_ E1"
  value:
  {
    AuthoredId: "93ad71bb-cd8f-dc24-7885-2c3fd0013245"
    Name: "The Graveyard of the Glazed: Recon"
    Duration: 15
    MinSweetness: 1
    MaxSweetness: 10
    MaxRewardQuality: 1
    BaseRewardsTableId: "7c40dee4-c1f9-c8f4-d923-910133846c78"
    BaseRollCount: 3
  }
}
expeditionblueprints:
{
  key: "Expedition_Rank2_ E1"
  value:
  {
    AuthoredId: "35052272-b4bc-75c4-0ad0-fe122f54b508"
    Name: "The Fizzy Swamp: Brave the Stench"
    Duration: 30
    MinSweetness: 2
    MaxSweetness: 10
    MaxRewardQuality: 1
    BaseRewardsTableId: "7c40dee4-c1f9-c8f4-d923-910133846c78"
    BaseRollCount: 4
  }
}
expeditionblueprints:
{
  key: "Expedition_Rank3_ E1"
  value:
  {
    AuthoredId: "6d7e16b9-1a8f-b134-59a6-fb9865615001"
    Name: "The Edge of the Licorice Forest: Scouting Ahead"
    Duration: 60
    MinSweetness: 3
    MaxSweetness: 10
    MaxRewardQuality: 2
    BaseRewardsTableId: "bf5e7bbe-2361-80b4-f862-acdfa491c07e"
    BaseRollCount: 4
  }
}
expeditionblueprints:
{
  key: "Expedition_Rank3_ E2"
  value:
  {
    AuthoredId: "fbb0edc6-92a9-dc04-5ad7-a19c36843074"
    Name: "The Edge of the Licorice Forest: Foraging for Glory"
    Duration: 60
    MinSweetness: 3
    MaxSweetness: 10
    MaxRewardQuality: 2
    BaseRewardsTableId: "f6151176-251b-6de4-1aee-245999752b5f"
    BaseRollCount: 4
  }
}
expeditionblueprints:
{
  key: "Expedition_Rank4_ E2"
  value:
  {
    AuthoredId: "768a4c69-c928-4a34-1bf2-044d3e5bac33"
    Name: "The Licorice Forest Deep: Digging for Roots"
    Duration: 125
    MinSweetness: 4
    MaxSweetness: 10
    MaxRewardQuality: 2
    BaseRewardsTableId: "f6151176-251b-6de4-1aee-245999752b5f"
    BaseRollCount: 6
  }
}
expeditionblueprints:
{
  key: "Expedition_Rank5_ E2"
  value:
  {
    AuthoredId: "19bbdaff-2ba7-12f4-3a13-989a490aaf86"
    Name: "The Great Coco River: Wading for Treasures"
    Duration: 350
    MinSweetness: 5
    MaxSweetness: 10
    MaxRewardQuality: 3
    BaseRewardsTableId: "74c64b03-c246-7174-bb50-4834e2d2a010"
    BaseRollCount: 4
  }
}
expeditionblueprints:
{
  key: "Expedition_Rank6_ E2"
  value:
  {
    AuthoredId: "f0e675df-3f16-e4b4-f9e2-6b5291a92b0e"
    Name: "The Marshmallow Hills: Delving the Caves"
    Duration: 700
    MinSweetness: 6
    MaxSweetness: 10
    MaxRewardQuality: 3
    BaseRewardsTableId: "74c64b03-c246-7174-bb50-4834e2d2a010"
    BaseRollCount: 5
  }
}
expeditionblueprints:
{
  key: "Expedition_Rank7_ E2"
  value:
  {
    AuthoredId: "c06ccaea-47e5-ec14-bb62-d824095a1bae"
    Name: "Lake Fruity Punch: The Sunken Town"
    Duration: 700
    MinSweetness: 7
    MaxSweetness: 10
    MaxRewardQuality: 3
    BaseRewardsTableId: "a2d81a2a-60e6-f354-6943-40803787242f"
    BaseRollCount: 6
  }
}
expeditionblueprints:
{
  key: "Expedition_Rank8_ E2"
  value:
  {
    AuthoredId: "c088ddf8-2b2e-b9c4-6bb2-52aa1af2584f"
    Name: "The Caramelized Valley: Under Every Rock"
    Duration: 700
    MinSweetness: 8
    MaxSweetness: 10
    MaxRewardQuality: 4
    BaseRewardsTableId: "45b14a76-3b9c-2014-7bc5-816c7de2f947"
    BaseRollCount: 7
  }
}
expeditionblueprints:
{
  key: "Expedition_Rank10_ E2"
  value:
  {
    AuthoredId: "15840d95-57de-86a4-da5e-9ab9dfb89299"
    Name: "Hard Candy Mountain: The Heart of the Volcano"
    Duration: 2800
    MinSweetness: 10
    MaxSweetness: 10
    MaxRewardQuality: 4
    BaseRewardsTableId: "18bce352-f1f1-c894-5807-0cc4e10c0f1a"
    BaseRollCount: 10
  }
}
fightermoveblueprints:
{
  key: "Move_Rare_Blocking_CandiedShell"
  value:
  {
    AuthoredId: "41416afb-0d72-f9b4-ba30-f089b713500f"
    Name: "Candied Shell"
    CandyType: "d6478df9-c705-98c4-ebb6-2ffab7dd798b"
    Quality: 3
    MoveType: 4
  }
}
fightermoveblueprints:
{
  key: "Move_Common_Tricky_PuckerSucker"
  value:
  {
    AuthoredId: "29d559bb-41a7-9f14-5bf8-605da8166943"
    Name: "Pucker Sucker"
    CandyType: "524c8f4f-cb50-ea04-dafd-e192f30c7656"
    Quality: 1
    MoveType: 2
  }
}
fightermoveblueprints:
{
  key: "Move_Common_Tricky_SugarySweep"
  value:
  {
    AuthoredId: "bd27fe1c-ce9a-f524-c887-61f9e4d5088b"
    Name: "Sugary Sweep"
    CandyType: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7"
    Quality: 1
    MoveType: 2
  }
}
fightermoveblueprints:
{
  key: "Move_Common_Blocking_SugarShield"
  value:
  {
    AuthoredId: "73eb61fc-70a9-25a4-ba01-0ecaa0d547c5"
    Name: "Sugar Shield"
    CandyType: "8aac4645-29cf-fe04-59c0-415508b0432e"
    Quality: 1
    MoveType: 4
  }
}
fightermoveblueprints:
{
  key: "Move_Silver_Epic_Blocking_TarnishingKneeDrop"
  value:
  {
    AuthoredId: "4dbb297f-6a80-ca44-a94f-3ea9ed3311e8"
    Name: "Tarnising Knee Drop"
    CandyType: "08847311-c629-a154-5958-13e8f1d77f88"
    Quality: 4
    MoveType: 4
  }
}
fightermoveblueprints:
{
  key: "Move_Silver_Epic_Heavy_SilverKnuckles"
  value:
  {
    AuthoredId: "e55a3d69-9fb4-5194-4812-c2f85e887aca"
    Name: "Silver Knuckles"
    CandyType: "321c2a65-64b1-8344-2915-85562e2ad7ec"
    Quality: 4
    MoveType: 0
  }
}
fightermoveblueprints:
{
  key: "Move_Bronze_Epic_Blocking_BouncingBarrage"
  value:
  {
    AuthoredId: "8e4753b5-c739-aaf4-ab79-4d333a69cae6"
    Name: "Bouncing Barrage"
    CandyType: "a2c2697c-d738-c004-4b32-d800921bab06"
    Quality: 4
    MoveType: 4
  }
}
fightermoveblueprints:
{
  key: "Move_Uncommon_Distance_ChillingBlow"
  value:
  {
    AuthoredId: "86bd4f81-e96b-9864-78b5-5fc51fbff6a4"
    Name: "Chilling Blow"
    CandyType: "63a8ea74-65ee-c024-b8f0-87e173258257"
    Quality: 2
    MoveType: 3
  }
}
fightermoveblueprints:
{
  key: "Move_Rare_Heavy_ViciousJawbreaker"
  value:
  {
    AuthoredId: "aa7d8565-99e1-84f4-49d2-8925c2602a7b"
    Name: "Vicious Jawbreaker"
    CandyType: "4d85b9b7-5d8b-db24-a85a-8df9fa935cb9"
    Quality: 3
    MoveType: 0
  }
}
fightermoveblueprints:
{
  key: "Move_Common_Speedy_One Thousand Tiny Slices"
  value:
  {
    AuthoredId: "628ebc0b-bb19-ee64-68b8-2f66f4b92720"
    Name: "One Thousand Tiny Slices"
    CandyType: "12ea78d4-e0f3-e5b4-1882-04787fa86d15"
    Quality: 1
    MoveType: 1
  }
}
fightermoveblueprints:
{
  key: "Move_Bronze_Epic_Tricky_ExplosiveJawbreaker"
  value:
  {
    AuthoredId: "61d8b3ec-9a01-0e64-fb45-27d2b858e332"
    Name: "Explosive Jawbreaker"
    CandyType: "aa0cb18d-5681-5504-09d7-4db06bf7a4d8"
    Quality: 4
    MoveType: 0
  }
}
fightermoveblueprints:
{
  key: "Move_Bronze_Epic_Tricky_BarfingBlow"
  value:
  {
    AuthoredId: "b96f1ff8-66d5-f244-79fd-260735d4c3d7"
    Name: "Barfing Blow"
    CandyType: "48ea4c49-79dd-6514-6b8b-f87fd6d8a330"
    Quality: 4
    MoveType: 2
  }
}
fightermoveblueprints:
{
  key: "Move_Rare_Heavy_BlackoutShot"
  value:
  {
    AuthoredId: "e16531a7-33fd-a3e4-69fd-ff10eba65cb7"
    Name: "Blackout Shot"
    CandyType: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c"
    Quality: 3
    MoveType: 0
  }
}
fightermoveblueprints:
{
  key: "Move_Uncommon_Distance_CinnamonBlast"
  value:
  {
    AuthoredId: "e201f964-8949-9d74-fbee-4ee8786284ca"
    Name: "Cinnamon Blast"
    CandyType: "1983b3b7-4a9e-8f14-b981-c94c32d7648b"
    Quality: 2
    MoveType: 3
  }
}
fightermoveblueprints:
{
  key: "Move_Rare_Blocking_BerryBounce"
  value:
  {
    AuthoredId: "2c94f949-a10e-2524-2827-6588982ce0b1"
    Name: "Berry Bounce"
    CandyType: "af509c01-933e-ce14-49c3-106081e16d75"
    Quality: 3
    MoveType: 4
  }
}
fightermoveblueprints:
{
  key: "Move_Uncommon_Tricky_FloatLikeAButterCream"
  value:
  {
    AuthoredId: "fa6144f9-ad49-8124-386f-351a7f1ab546"
    Name: "Float Like a Butter Cream"
    CandyType: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1"
    Quality: 2
    MoveType: 2
  }
}
fightermoveblueprints:
{
  key: "Move_Uncommon_Blocking_ChewyAbsorption"
  value:
  {
    AuthoredId: "431900a2-54d4-df34-d9f7-6a4608fecfaa"
    Name: "Chewy Absorption"
    CandyType: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af"
    Quality: 2
    MoveType: 4
  }
}
fightermoveblueprints:
{
  key: "Move_Common_Speedy_CocoChaos"
  value:
  {
    AuthoredId: "1d1dec74-ca35-0cc4-d930-164786a9dd81"
    Name: "Coco Chaos"
    CandyType: "7ddba6f2-b08a-f954-4a97-eae6af175005"
    Quality: 1
    MoveType: 1
  }
}
fightermoveblueprints:
{
  key: "Move_Common_Distance_GetOverHere"
  value:
  {
    AuthoredId: "2c555752-8a84-58f4-395e-6460b7864069"
    Name: "Get Over Here!"
    CandyType: "4e343f68-1aaa-0e84-7bab-eae458b68264"
    Quality: 1
    MoveType: 3
  }
}
fightermoveblueprints:
{
  key: "Move_Common_Distance_BubbleTrouble"
  value:
  {
    AuthoredId: "a9d69fc0-a1b8-ae24-cbd7-249760200211"
    Name: "Bubble Trouble"
    CandyType: "87040ddf-f0cb-f894-dbbd-a4f266cae4e7"
    Quality: 1
    MoveType: 3
  }
}
fightermoveblueprints:
{
  key: "Move_Gold_Epic_Tricky_GildedTripper"
  value:
  {
    AuthoredId: "34a115db-3153-aff4-9a46-97700634f1fb"
    Name: "Gilded Tripper"
    CandyType: "8898194e-ac52-f624-a830-176655bb43cd"
    Quality: 4
    MoveType: 2
  }
}
fightermoveblueprints:
{
  key: "Move_Common_Heavy_PopRockPop"
  value:
  {
    AuthoredId: "9fd0b9ef-2e7d-d344-8926-69f1fab99f65"
    Name: "Pop Rock Pop"
    CandyType: "16e0b307-2b40-4584-294c-dee505e47827"
    Quality: 1
    MoveType: 0
  }
}
fightermoveblueprints:
{
  key: "Move_Uncommon_Speedy_ComeOverHereAndSlipOnIt"
  value:
  {
    AuthoredId: "b1bd32de-88b2-1204-7b2e-0680677d78c4"
    Name: "Come Over Here and Slip on it"
    CandyType: "5a087336-6c2d-a684-1b72-a86d6ff6931f"
    Quality: 2
    MoveType: 1
  }
}
fightermoveblueprints:
{
  key: "Move_Rare_Tricky_ToffeeTripper"
  value:
  {
    AuthoredId: "2569d967-c676-8b44-cbdf-d514e4c5c73d"
    Name: "Toffee Tripper"
    CandyType: "230bc5ac-6ab7-a6d4-7a66-fc06a0e0e38d"
    Quality: 3
    MoveType: 2
  }
}
fightermoveblueprints:
{
  key: "Move_Common_Speedy_RightOnTheButton"
  value:
  {
    AuthoredId: "c28e13b4-cb1f-6ea4-680f-91010a400e10"
    Name: "Right on the Button"
    CandyType: "f6025c1b-55e8-da24-d96f-0ecebf74c436"
    Quality: 1
    MoveType: 1
  }
}
fightermoveblueprints:
{
  key: "Move_Gold_Epic_Distance_GoldRush"
  value:
  {
    AuthoredId: "c6770789-4a1f-7f24-f921-c4dc66c2697a"
    Name: "Gold Rush"
    CandyType: "47b8a853-791a-1424-1ba9-4adaf4a3f4a5"
    Quality: 4
    MoveType: 3
  }
}
fightermoveblueprints:
{
  key: "Move_Uncommon_Heavy_ChocoCrunch"
  value:
  {
    AuthoredId: "9f0df0ed-13a6-84c4-781d-9dbf9741c66e"
    Name: "Choco Crunch"
    CandyType: "40067d7d-6651-cbc4-c8ed-785d73e9ab57"
    Quality: 2
    MoveType: 0
  }
}
fightermoveblueprints:
{
  key: "Move_Silver_Epic_Speedy_QuicksilverSlice"
  value:
  {
    AuthoredId: "1a67cc79-b97a-4624-ca67-36c9fcbee970"
    Name: "Quicksilver Slice"
    CandyType: "bbeb0e31-3775-a864-1952-082233cf7b89"
    Quality: 4
    MoveType: 1
  }
}
fightermoveblueprints:
{
  key: "Move_Common_Blocking_GumDropKick"
  value:
  {
    AuthoredId: "0090580c-04ef-9d84-e883-32f52c977b98"
    Name: "Gum Drop Kick"
    CandyType: "c2774273-0cd4-2494-fa02-76cffe1ef1ef"
    Quality: 1
    MoveType: 4
  }
}
fightermoveblueprints:
{
  key: "Move_Gold_Epic_Heavy_GoldenShowerOfChips"
  value:
  {
    AuthoredId: "92f7b745-c1ff-3cd4-f9cf-0fe0dafb5536"
    Name: "Golden Shower of Chips"
    CandyType: "f6c3b895-6488-1b74-a9b0-664027a683e2"
    Quality: 4
    MoveType: 0
  }
}
fightermoveblueprints:
{
  key: "Move_Common_Heavy_CandiedKnuckles"
  value:
  {
    AuthoredId: "86b323c2-b2fd-2494-ab5e-bc3514bc92d8"
    Name: "Candied Knuckles"
    CandyType: "934dae05-689f-30e4-5804-d497aee0b47c"
    Quality: 1
    MoveType: 0
  }
}
fightermoveblueprints:
{
  key: "Move_Rare_Distance_LimonShuriken"
  value:
  {
    AuthoredId: "55e05ece-a3f6-a924-3833-2222a0eee6a3"
    Name: "Limon Shuriken"
    CandyType: "e5afded6-705a-2034-7b1c-556ac877fb0f"
    Quality: 3
    MoveType: 3
  }
}
fightermoveblueprints:
{
  key: "Move_Silver_Epic_Tricky_ConductiveCoating"
  value:
  {
    AuthoredId: "bbf71f26-78f5-5014-5b4b-a1c3866cb75b"
    Name: "Conductive Coat"
    CandyType: "0ee1131a-b8e4-05c4-9896-3690ebf8563a"
    Quality: 4
    MoveType: 2
  }
}
fightermoveblueprints:
{
  key: "Move_Common_Speedy_IcingOnTheCake"
  value:
  {
    AuthoredId: "1e47a8e0-acfd-1244-0b7d-539d62429060"
    Name: "Icing on the Cake"
    CandyType: "f9c0a4d2-f049-29a4-eb1e-2269de35798e"
    Quality: 1
    MoveType: 1
  }
}
fightermoveblueprints:
{
  key: "Move_Rare_Speedy_SugarRush"
  value:
  {
    AuthoredId: "ad6d5d28-08b4-4304-6b49-11f8f5f9ff1c"
    Name: "Sugar Rush"
    CandyType: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c"
    Quality: 3
    MoveType: 1
  }
}
fightermoveblueprints:
{
  key: "Move_Gold_Epic_Blocking_GildedBonds"
  value:
  {
    AuthoredId: "01b34a91-37d8-29d4-09cb-2d515f51e315"
    Name: "Gilded Bonds"
    CandyType: "52ea4044-e203-16d4-c90f-8b685ed5e9c9"
    Quality: 4
    MoveType: 4
  }
}
fightermoveblueprints:
{
  key: "Move_Bronze_Epic_Speedy_SugaryRush"
  value:
  {
    AuthoredId: "3e76f2fa-072c-5654-dac9-c9b0e4e1643a"
    Name: "Super Sugary Rush"
    CandyType: "8932caea-da2e-6384-58ec-749624e213c9"
    Quality: 4
    MoveType: 1
  }
}
fightermoveblueprints:
{
  key: "Move_Bronze_Epic_Blocking_HeavyGumdropKick"
  value:
  {
    AuthoredId: "55cafe03-4a58-0f44-6a27-f9fc6fce881a"
    Name: "Heavy Gumdrop Kick"
    CandyType: "8f896243-a6de-1b54-68a3-0df72e8688e4"
    Quality: 4
    MoveType: 4
  }
}
fightermoveblueprints:
{
  key: "Move_Uncommon_Tricky_KickInTheBeans"
  value:
  {
    AuthoredId: "b889835e-1cd6-d3d4-f9b4-dd1989f49dac"
    Name: "Kick in the Beans"
    CandyType: "9b70e722-0442-2924-092c-93efc1d92975"
    Quality: 2
    MoveType: 2
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Boxer"
  value:
  {
    AuthoredId: "0d66cfa3-128e-2904-48d4-73a55fc1cab7"
    Name: "Boxer"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionButterscotch"
  value:
  {
    AuthoredId: "0d0fe377-02ea-eca4-0810-264ce2f6df30"
    Name: "Centurion Butterscotch"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Of_Fudgeland"
  value:
  {
    AuthoredId: "da9ffa75-43d1-63e4-ead1-ecc4af5de720"
    Name: "Of Fudgeland"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterFrosted"
  value:
  {
    AuthoredId: "6c23082c-7765-d824-f801-ddecc339412c"
    Name: "Grand Master Frosted"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Moldy"
  value:
  {
    AuthoredId: "c2af0caa-787d-ecd4-89b8-1d6ac552dad1"
    Name: "The Moldy"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Sugary"
  value:
  {
    AuthoredId: "0accbfa9-9279-56c4-db81-0e96b38e83ce"
    Name: "Sugarie"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Of_Jellyton"
  value:
  {
    AuthoredId: "e76dca62-0d94-ba54-99fb-9707f7b9d54c"
    Name: "Of Jellyton"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Pudding"
  value:
  {
    AuthoredId: "5f081a07-09a1-f2c4-fa09-a3b7df0c625f"
    Name: "Pudding"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirRootBeer"
  value:
  {
    AuthoredId: "88d68be1-ae45-7f94-ead1-b0fd185f60b7"
    Name: "Sir Root Beer"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterFizzy"
  value:
  {
    AuthoredId: "7b374c83-39a0-d264-58ca-bb3dd3e1a96f"
    Name: "Grand Master Fizzy"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Nutty"
  value:
  {
    AuthoredId: "9e8b549f-7758-89d4-c883-bf3ee9907709"
    Name: "The Nutty"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Everlasting"
  value:
  {
    AuthoredId: "4adc8af6-c8c7-69f4-c93e-e2ebbd1400d7"
    Name: "Everlasting"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirGrumpy"
  value:
  {
    AuthoredId: "450d815b-10b8-71d4-a9e2-29559c205101"
    Name: "Sir Grumpy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Counterpuncher"
  value:
  {
    AuthoredId: "e8ebd649-5a51-cb64-d8b4-604265091540"
    Name: "Counterpuncher"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Checkhook"
  value:
  {
    AuthoredId: "16c9d5dc-7bc8-9504-a969-2f26d6383fe9"
    Name: "Checkhook"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Fresh"
  value:
  {
    AuthoredId: "8f0b3c0f-7421-9264-588c-83a5ea6879a7"
    Name: "The Fresh"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterLazy"
  value:
  {
    AuthoredId: "bb978b77-a5ee-9e24-897a-4f61a13ce5bf"
    Name: "Grand Master Lazy"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Moldy"
  value:
  {
    AuthoredId: "e56769ed-cc14-2224-3837-f0fd353c7bf6"
    Name: "The Moldy"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiCaramelized"
  value:
  {
    AuthoredId: "81d1a8bc-a727-ff14-3b28-dd5319a482b4"
    Name: "Senpai Caramelized"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterButtered"
  value:
  {
    AuthoredId: "c88206f6-5a76-3c04-d83a-3c36f2fb0d8c"
    Name: "Grand Master Buttered"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionMelty"
  value:
  {
    AuthoredId: "ad32099a-befe-a0e4-6948-aa6de144728e"
    Name: "Centurion Melty"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiPrechewed"
  value:
  {
    AuthoredId: "2232a3a3-f164-9754-688a-aa11aaedc6de"
    Name: "Senpai Prechewed"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirRollie"
  value:
  {
    AuthoredId: "b3d25af6-7166-38a4-588d-bbc34e72f774"
    Name: "Sir Rollie"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Pie"
  value:
  {
    AuthoredId: "69216dae-20b0-e0e4-eaae-cf5d560e6909"
    Name: "Pie"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryIcey"
  value:
  {
    AuthoredId: "cdfd5bba-322e-b9b4-7a4c-462a74f43444"
    Name: "Legionary Icey"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiCandied"
  value:
  {
    AuthoredId: "c355feaf-01ed-2364-1903-249b2f0789e5"
    Name: "Senpai Candied"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Fighter"
  value:
  {
    AuthoredId: "3d69920b-201c-c9a4-aaaf-cf8abbbf3d9c"
    Name: "Fighter"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressAngry"
  value:
  {
    AuthoredId: "9a1de7b4-c0b4-42a4-8af2-023fac1f2aa1"
    Name: "Mistress Angry"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionBadass"
  value:
  {
    AuthoredId: "1cfd5434-40de-84b4-38ef-e941c399e52b"
    Name: "Centurion Badass"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Coconut"
  value:
  {
    AuthoredId: "6cdc41b1-c364-0a84-bbe4-ceae58497bb8"
    Name: "Coconut"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Southpaw"
  value:
  {
    AuthoredId: "55ec6f01-2655-0d04-eab9-c89c974d7306"
    Name: "Southpaw"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterFruity"
  value:
  {
    AuthoredId: "b9c5c24f-0ee8-e2e4-e956-f0b571556384"
    Name: "Grand Master Fruity"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Crazy"
  value:
  {
    AuthoredId: "8cfa818d-97b6-e484-3839-da2979e2c39d"
    Name: "The Crazy"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Of_Puddingham"
  value:
  {
    AuthoredId: "d1a4f1aa-5843-b114-8879-e31a2f08ac61"
    Name: "Of Puddingham"
    Position: 1
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Fried"
  value:
  {
    AuthoredId: "ba94e5d7-aa87-e304-0839-7f79a7471d36"
    Name: "Fried"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Melty"
  value:
  {
    AuthoredId: "74705edf-a7ba-07b4-8925-27fd08fefd4e"
    Name: "The Melty"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Sucker"
  value:
  {
    AuthoredId: "ec0ac184-c662-0b54-6a11-5cec45127d4b"
    Name: "Sucker"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_OfCakerton"
  value:
  {
    AuthoredId: "7b00a1e9-d835-6e54-7b8d-8db7ad95516d"
    Name: "Of Cakerton"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionThornberry"
  value:
  {
    AuthoredId: "01273cc6-c240-4e64-59cd-072d6d27d66f"
    Name: "Centurion Thornbery"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirSneezy"
  value:
  {
    AuthoredId: "f17fdc33-efde-2db4-8897-26a38b8f4ac2"
    Name: "Sir Sneezy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryHotButtered"
  value:
  {
    AuthoredId: "2788bbe1-261e-09b4-7806-b083c1087e55"
    Name: "Legionary Hot Buttered"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Dusted"
  value:
  {
    AuthoredId: "1359296d-e943-0e24-a8d8-01a433286f83"
    Name: "Dusted"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiButtered"
  value:
  {
    AuthoredId: "0292481d-b153-bcb4-da26-b829d5ab929a"
    Name: "Senpai Buttered"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionPineapple"
  value:
  {
    AuthoredId: "23f8d7ed-0878-7234-b8b9-f1376ea99101"
    Name: "Centurion Pineapple"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Lemony"
  value:
  {
    AuthoredId: "7aa77b57-1a44-1be4-e9c0-d7acb87d6e6c"
    Name: "The Lemony"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Chewy"
  value:
  {
    AuthoredId: "314bdee2-b00d-9b64-895c-2eeae514cc61"
    Name: "Chewy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionCaramelized"
  value:
  {
    AuthoredId: "d223ffce-c202-8fc4-da14-d33929ca08ee"
    Name: "Centurion Caramelized"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirMinty"
  value:
  {
    AuthoredId: "aac99205-872a-2254-c805-de9e8e0e0709"
    Name: "Sir Minty"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Counterpuncher"
  value:
  {
    AuthoredId: "7f0b7a03-477c-b684-8b3b-f840cde7412d"
    Name: "Counterpuncher"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressFrenchVanilla"
  value:
  {
    AuthoredId: "007156ea-6b1e-ac14-2a57-23d798d798ee"
    Name: "Mistress French Vanilla"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterMelon"
  value:
  {
    AuthoredId: "2115aff4-90e4-e614-cb52-e724a8b72e48"
    Name: "Grand Master Melon"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressOrange"
  value:
  {
    AuthoredId: "67a87c3a-f037-b914-6a85-076d32432168"
    Name: "Mistress Orange"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Crusty"
  value:
  {
    AuthoredId: "4c52f9ab-8db2-7444-e9a9-f32f4689a959"
    Name: "The Crusty"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Thornberry"
  value:
  {
    AuthoredId: "e7e0a11a-045d-34c4-d9fc-20eb93f711af"
    Name: "Thornbery"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressFresh"
  value:
  {
    AuthoredId: "bf142567-e96b-94d4-fb2e-4ecf09db3ac2"
    Name: "Mistress Fresh"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Cake"
  value:
  {
    AuthoredId: "0f88abb7-2f31-1754-4b99-7941a7b4503d"
    Name: "Cake"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirDeepFried"
  value:
  {
    AuthoredId: "8daba0f5-c1b7-cab4-389d-453e2cdb7278"
    Name: "Sir Deep Fried"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirSquishy"
  value:
  {
    AuthoredId: "9b28a5a2-d6db-8414-5b11-8b0a7b07551b"
    Name: "Sir Squishy"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterEverlasting"
  value:
  {
    AuthoredId: "7473fee6-5aa2-6314-d9c4-9590c54686c2"
    Name: "Grand Master Everlasting"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressCrunchy"
  value:
  {
    AuthoredId: "74d49bf0-3f51-5554-4be8-cc083eef550e"
    Name: "Mistress Crunchy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressMelted"
  value:
  {
    AuthoredId: "3a7a68c4-afbb-7124-fab8-1ba0f7c8f54d"
    Name: "Mistress Melted"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Cake"
  value:
  {
    AuthoredId: "679aef91-11c6-13d4-da2a-2c9337f71a5e"
    Name: "Cake"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiBlueberry"
  value:
  {
    AuthoredId: "99cfca2a-bd21-9354-88ba-f2da7081f8a8"
    Name: "Senpai Blueberry"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterThornberry"
  value:
  {
    AuthoredId: "44563416-caf4-5d04-6888-6ce5f5a94d14"
    Name: "Grand Master Thornbery"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressMelon"
  value:
  {
    AuthoredId: "8c49243c-6bbd-d174-7813-193e8103af97"
    Name: "Mistress Melon"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Pickle"
  value:
  {
    AuthoredId: "ba5b6463-36b1-bf34-9a4b-cdd0b940b230"
    Name: "Pickle"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Raspberry"
  value:
  {
    AuthoredId: "f404bb91-9566-c184-8b07-3b1b5a99afe7"
    Name: "Rasperry"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Brandied"
  value:
  {
    AuthoredId: "44b52cdc-0dea-49c4-9956-f70db85a019b"
    Name: "Brandied"
    Position: 0
    Quality: 0
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Grumpy"
  value:
  {
    AuthoredId: "e5161799-7faf-c874-5831-155b48118d5d"
    Name: "Grumpy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressCrazy"
  value:
  {
    AuthoredId: "9ebb1137-91ac-9d14-8add-484db4991f3c"
    Name: "Mistress Crazy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirSnozzberry"
  value:
  {
    AuthoredId: "5e70012f-3b16-b3d4-6a5e-224e3d81bb5e"
    Name: "Sir Snozzberry"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Bitter"
  value:
  {
    AuthoredId: "12a77f50-2447-b344-bb0f-7b29d9187ced"
    Name: "The Bitter"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressFlying"
  value:
  {
    AuthoredId: "24d57323-3b53-7fd4-7982-a2157c207e52"
    Name: "Mistress Flying"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Very_Tasty"
  value:
  {
    AuthoredId: "8faf5746-6ba0-0a54-19e0-34048e56542c"
    Name: "The Very Tasty"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirMango"
  value:
  {
    AuthoredId: "05b7a38f-33fc-b074-6b4c-ab9d03f5a2a4"
    Name: "Sir Mango"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Noogie"
  value:
  {
    AuthoredId: "7f7f8f6d-73d0-e6d4-5b17-723fb2f44f5a"
    Name: "Noogie"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_TravelSized"
  value:
  {
    AuthoredId: "ec517e02-0445-2c04-685f-1af13604637f"
    Name: "Travel Sized"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Candy_Coated"
  value:
  {
    AuthoredId: "f64788bd-f745-c1c4-2b15-a2c0af618fe3"
    Name: "The Candy Coated"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiLifeSized"
  value:
  {
    AuthoredId: "e5acc6b8-424d-4fc4-a91f-367da5e8c4ab"
    Name: "Senpai Life Sized"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirBiteSized"
  value:
  {
    AuthoredId: "f74a0d94-d060-8bf4-3b04-ce85f3cda17f"
    Name: "Sir Bite Sized"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirLimon"
  value:
  {
    AuthoredId: "841ac00a-c513-7c14-79e6-34aed0ede0f6"
    Name: "Sir Limon"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiSnozzberry"
  value:
  {
    AuthoredId: "5a000301-3c4f-d834-1bb2-9d49261cebbc"
    Name: "Senpai Snozzberry"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Powdered"
  value:
  {
    AuthoredId: "3106fb8f-7b8c-38d4-0b16-285c1089a95d"
    Name: "Powdered"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterNutty"
  value:
  {
    AuthoredId: "ee1d80dd-66fb-3d14-f998-5c7e02afcd86"
    Name: "Grand Master Nutty"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Dusted"
  value:
  {
    AuthoredId: "b4915b5f-53f8-a154-1b20-778d05ce70e3"
    Name: "Dusted"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionVanilla"
  value:
  {
    AuthoredId: "ba2ea7ef-5af8-6304-ea4f-78b7f7c65192"
    Name: "Centurion Vanilla"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Kicker"
  value:
  {
    AuthoredId: "34b015db-9d72-55a4-3a39-5ca8238e7361"
    Name: "Kicker"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterFrenchVanilla"
  value:
  {
    AuthoredId: "6142da44-1d30-2c84-4927-d4be01c0d110"
    Name: "Grand Master French Vanilla"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Stale"
  value:
  {
    AuthoredId: "80ec0f53-7e94-e4b4-8b4e-bde083f1cacc"
    Name: "Stale"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionAngry"
  value:
  {
    AuthoredId: "cad3c4c3-6e08-d504-a969-0ff585681407"
    Name: "Centurion Angry"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Hardened"
  value:
  {
    AuthoredId: "31a05e98-7677-9c84-fb0f-ed95587adf8f"
    Name: "The Hardened"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryCrusty"
  value:
  {
    AuthoredId: "3976235f-8997-22c4-e9ab-9b424d553027"
    Name: "Legionary Crusty"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Pop"
  value:
  {
    AuthoredId: "65b11fde-60ab-10f4-2b96-c51164dd0760"
    Name: "Pop"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Drizzled"
  value:
  {
    AuthoredId: "8e15a5b9-3951-d454-7b2d-e3ea9617f4d8"
    Name: "Drizzled"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterCrunchy"
  value:
  {
    AuthoredId: "9bdb23c8-55ef-d9a4-59c3-90ef6ee6053d"
    Name: "Grand Master Crunchy"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiSingleServing"
  value:
  {
    AuthoredId: "ac2472b7-9e76-8214-d99a-419bd87da560"
    Name: "Senpai Single Serving"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiMelon"
  value:
  {
    AuthoredId: "2a6b1e46-e26a-d074-fad5-a08fe1ec902e"
    Name: "Senpai Melon"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionGreenApple"
  value:
  {
    AuthoredId: "4d677206-4e08-6d54-2b1d-a337f97f6d5d"
    Name: "Centurion Green Apple"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiThornberry"
  value:
  {
    AuthoredId: "34a848f8-8c45-07a4-5930-0591153e7051"
    Name: "Senpai Thornbery"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Sneezy"
  value:
  {
    AuthoredId: "1055650f-c3ef-96f4-19da-2d15e86ed278"
    Name: "Sneezy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Fightah"
  value:
  {
    AuthoredId: "f9b383ea-7e4d-2cc4-ebb5-1d98ea48eba9"
    Name: "Fightah"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressSpicy"
  value:
  {
    AuthoredId: "cb98879c-6a6b-f074-a992-735ff0cb26be"
    Name: "Mistress Spicy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterSquishy"
  value:
  {
    AuthoredId: "9fdfe4bf-c5db-0b24-680f-5f23cf8b4886"
    Name: "Grand Master Squishy"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_OfLicorice"
  value:
  {
    AuthoredId: "56e5889f-443b-dcf4-1825-bd1725f2a791"
    Name: "Of Licorice"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Clinch"
  value:
  {
    AuthoredId: "149aae2f-d2c3-d5e4-2acc-c96e639f69b7"
    Name: "Clinch"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryFrenchVanilla"
  value:
  {
    AuthoredId: "5adb6398-55cb-c844-6b92-f1622c89b492"
    Name: "Legionary French Vanilla"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterRolly"
  value:
  {
    AuthoredId: "d2f2f46f-b397-4a34-aa68-82fe34281432"
    Name: "Grand Master Rolly"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressBashful"
  value:
  {
    AuthoredId: "904e8c10-671c-41a4-7b1b-8b96d146151b"
    Name: "Mistress Vanilla"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirGrouchy"
  value:
  {
    AuthoredId: "8994cb35-70c1-4af4-1a37-4fb37dc8010c"
    Name: "Sir Grouchy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Angry"
  value:
  {
    AuthoredId: "958e07db-6269-1484-e8e1-0fb88ed9f513"
    Name: "The Angry"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Chewy"
  value:
  {
    AuthoredId: "4d69d7dc-2bd5-66a4-39b0-e8471fbd1ee8"
    Name: "The Chewy"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Frosted"
  value:
  {
    AuthoredId: "a64b86d7-0808-ed44-4b84-82b4caff085e"
    Name: "The Frosted"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionarySalted"
  value:
  {
    AuthoredId: "8b5051b4-0bc3-f734-5a7d-811b17bd4e81"
    Name: "Legionary Salted"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Everlasting"
  value:
  {
    AuthoredId: "6d171ecc-33a8-3774-386b-026d227f88e2"
    Name: "The Everlasting"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterMelty"
  value:
  {
    AuthoredId: "96527d39-290a-e224-a917-8d458e22f9fd"
    Name: "Grand Master Melty"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiCoconut"
  value:
  {
    AuthoredId: "418b3f78-25c8-da04-193f-521a24f9d885"
    Name: "Senpai Coconut"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Baked"
  value:
  {
    AuthoredId: "43961912-fa95-6174-1a0b-1341efd440de"
    Name: "Baked"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Glazed"
  value:
  {
    AuthoredId: "b50c8ab4-fb1a-e8d4-7a9d-9c90824cbf28"
    Name: "Glazed"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Crunchy"
  value:
  {
    AuthoredId: "f465a442-74ea-ca94-082f-e9b6c55f075a"
    Name: "Crunchy"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryBitter"
  value:
  {
    AuthoredId: "799ff409-5c76-9c04-1b46-cb2a34fcd368"
    Name: "Legionary Bitter"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionFresh"
  value:
  {
    AuthoredId: "2be06101-352f-1b04-2a94-6e44a22c5b49"
    Name: "Centurion Fresh"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionMoldy"
  value:
  {
    AuthoredId: "ef98f9f9-098c-2204-eb62-3cbaa8e4f961"
    Name: "Centurion Moldy"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionEverlasting"
  value:
  {
    AuthoredId: "775352e1-8065-06a4-1b5b-1bf1e11e4435"
    Name: "Centurion Everlasting"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_DeepFried"
  value:
  {
    AuthoredId: "3e7d36b1-f441-3164-0bea-86f267cbb7aa"
    Name: "Deep Fried"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionFrenchVanilla"
  value:
  {
    AuthoredId: "51164421-2a1e-d154-9ae7-9a5d3d290fa0"
    Name: "Centurion French Vanilla"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryTravelSized"
  value:
  {
    AuthoredId: "696e4baf-dac6-fa84-8a47-ffdb0621e469"
    Name: "Legionary Travel Sized"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Chocolate"
  value:
  {
    AuthoredId: "c1971eae-5758-a174-ebab-0f88fbd1bc50"
    Name: "The Chocolate"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Slugger"
  value:
  {
    AuthoredId: "c87f34ca-2d8e-9844-a841-5f5885556bf7"
    Name: "Slugger"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterDusted"
  value:
  {
    AuthoredId: "b5c73c2f-8e6b-bd14-0b05-b5bc0483aa4a"
    Name: "Grand Master Dusted"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiNutty"
  value:
  {
    AuthoredId: "67c32986-2332-be74-6ac9-2542461959e9"
    Name: "Senpai Nutty"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Strong"
  value:
  {
    AuthoredId: "a9e36886-7152-b714-0901-740cd65ac2e6"
    Name: "The Strong"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Biter"
  value:
  {
    AuthoredId: "9f6e9b68-13e5-2064-eaaf-fadf75c034aa"
    Name: "Biter"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Fried"
  value:
  {
    AuthoredId: "23957a84-130b-8e64-6a30-103a75a10af0"
    Name: "The Fried"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiCherry"
  value:
  {
    AuthoredId: "afe7095e-7243-8c14-6bef-fb8eeab1977f"
    Name: "Senpai Cherry"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Fresh"
  value:
  {
    AuthoredId: "4b54fcac-8dc2-cec4-1afa-db3938539922"
    Name: "The Fresh"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Brandied"
  value:
  {
    AuthoredId: "caae666f-3767-9c04-9ab9-65cd3dae47fc"
    Name: "The Brandied"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Moldy"
  value:
  {
    AuthoredId: "302b1e92-cdc0-ec84-39f6-6caa531181a9"
    Name: "The Moldy"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Pie"
  value:
  {
    AuthoredId: "6618d93a-3257-c484-3961-49272bb666f8"
    Name: "Pie"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Jelly"
  value:
  {
    AuthoredId: "946d02ab-7021-9144-9b31-9f2bcaa466e9"
    Name: "Jelly"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_HotButtered"
  value:
  {
    AuthoredId: "51cb6c27-568d-8874-b9f4-bfda37effb3d"
    Name: "Hot Buttered"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Superstar"
  value:
  {
    AuthoredId: "f3278580-b13b-f734-c8b5-e8b7487a8d3d"
    Name: "Superstar"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterBashful"
  value:
  {
    AuthoredId: "a4001999-a632-8064-393c-e608dc4713f2"
    Name: "Grand Master Vanilla"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryMinty"
  value:
  {
    AuthoredId: "b23339e9-eca7-ef94-cad1-09dbb151fdf6"
    Name: "Legionary Minty"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Fast"
  value:
  {
    AuthoredId: "c22b1ff4-a7ff-4cf4-6926-d94c4c59ca75"
    Name: "Fast"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirPineapple"
  value:
  {
    AuthoredId: "15f00b33-c08f-96f4-7871-be3c3e273b0e"
    Name: "Sir Pineapple"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryCreamy"
  value:
  {
    AuthoredId: "84fc719b-543a-bd74-3863-4d21c394a1f7"
    Name: "Legionary Creamy"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Fightah"
  value:
  {
    AuthoredId: "e00d4921-f6b6-f484-bb5e-a181b884014c"
    Name: "Fightah"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Checkhook"
  value:
  {
    AuthoredId: "91f468f2-1a30-bfe4-7a68-c128e81183cb"
    Name: "Checkhook"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Chocolate"
  value:
  {
    AuthoredId: "d2ae971e-81d9-3e14-3a1d-fc2899053ee4"
    Name: "The Chocolate"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressChewy"
  value:
  {
    AuthoredId: "92191c2c-2f2a-b354-c91b-32c82619f739"
    Name: "Mistress Chewy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Switch_Hitter"
  value:
  {
    AuthoredId: "a851b866-4a05-abf4-9927-6b95deefea88"
    Name: "Switch-Hitter"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_BiteSized"
  value:
  {
    AuthoredId: "890adc00-9389-a5f4-3ac3-89ffd2634b8a"
    Name: "Bite Sized"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Rolled"
  value:
  {
    AuthoredId: "00d445d7-4a7e-97f4-1a64-d2ad06964b90"
    Name: "Rolled"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Tasty"
  value:
  {
    AuthoredId: "c6a1d77b-5fd6-6474-6b0a-de10455ca247"
    Name: "Tasty"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressBrandied"
  value:
  {
    AuthoredId: "4e977d4e-2434-6eb4-ebdb-3c5f65b60063"
    Name: "Mistress Brandied"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Of_Sourway"
  value:
  {
    AuthoredId: "252a6834-8736-ee84-6b94-b7babe5400e0"
    Name: "Of Sourway"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Crazy"
  value:
  {
    AuthoredId: "4b503d25-3b0c-39b4-e906-75f2b2daab03"
    Name: "The Crazy"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirChopped"
  value:
  {
    AuthoredId: "839d85ac-c64c-1c44-085e-c805cecf4f3b"
    Name: "Sir Chopped"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirThornberry"
  value:
  {
    AuthoredId: "03717946-3c23-3fe4-7b83-83c77c03a2ff"
    Name: "Sir Thornbery"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Cheesy"
  value:
  {
    AuthoredId: "5b046405-fbfe-afd4-9bfc-5466ed3417d8"
    Name: "Cheesy"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiMelty"
  value:
  {
    AuthoredId: "75c4b0b6-82a7-bf34-686b-acefc24e62b8"
    Name: "Senpai Melty"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Snozzberry"
  value:
  {
    AuthoredId: "455ea0b5-4ac7-f994-b866-5923032a4544"
    Name: "Snozzberry"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Lazy"
  value:
  {
    AuthoredId: "17b203a8-25e8-5704-da3a-bce3411c4b8d"
    Name: "The Lazy"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Pop"
  value:
  {
    AuthoredId: "9e24f6ae-4105-c4e2-db2a-72ac6e32553f"
    Name: "Pop"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionSour"
  value:
  {
    AuthoredId: "bc772513-d7f8-9b44-59f0-c96597e2f5b7"
    Name: "Centurion Sour"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Boiled"
  value:
  {
    AuthoredId: "c24fd4e2-1891-98c4-9adb-1d9439b2ca95"
    Name: "The Boiled"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Dopey"
  value:
  {
    AuthoredId: "08add95b-abc6-9ac4-385e-3dbb5670642d"
    Name: "Bashful"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Pickle"
  value:
  {
    AuthoredId: "d33422cf-0d87-5454-ba5a-6e21e4599bdb"
    Name: "Pickle"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Malted"
  value:
  {
    AuthoredId: "70c054dc-2682-06b4-a912-df0ba81beb8b"
    Name: "Malted"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryHappy"
  value:
  {
    AuthoredId: "3af1dfee-6351-8144-59e8-886fec10dd3c"
    Name: "Legionary Happy"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionSpicy"
  value:
  {
    AuthoredId: "cb0461c2-3edc-fa04-eb5f-0e34885a6c30"
    Name: "Centurion Spicy"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Breaker"
  value:
  {
    AuthoredId: "547e506c-4e01-fbe4-f813-1cb54728c116"
    Name: "Breaker"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressPowdered"
  value:
  {
    AuthoredId: "a5ef2ac6-ccaa-c884-281c-c0bd150ec7ee"
    Name: "Mistress Powdered"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Caramelized"
  value:
  {
    AuthoredId: "714d28d7-74de-f004-99ca-2c64d11b786c"
    Name: "Caramelized"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Salty"
  value:
  {
    AuthoredId: "d2f5073c-ca5b-ab84-899d-93c1c34a1380"
    Name: "Salty"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Plump"
  value:
  {
    AuthoredId: "4d062f30-6675-8024-db7b-3a6cbce38dde"
    Name: "The Plump"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Prechewed"
  value:
  {
    AuthoredId: "07e19c8d-a935-e6d4-bb6d-9acb9c891993"
    Name: "The Prechewed"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Single_Serving"
  value:
  {
    AuthoredId: "da89da52-de3c-6b74-6a3f-2f3a04c7ccf0"
    Name: "The Single Serving"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryMango"
  value:
  {
    AuthoredId: "59540aa5-2370-53a4-f983-f7518183cf6f"
    Name: "Legionary Mango"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirLemon"
  value:
  {
    AuthoredId: "72360f56-3c96-08b4-1b91-36a572d643ac"
    Name: "Sir Lemony"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Sugary"
  value:
  {
    AuthoredId: "9b047a3e-a06f-9524-bbb8-7043d181fd25"
    Name: "The Sugary"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Biter"
  value:
  {
    AuthoredId: "4d08e759-5a5e-5414-dbd8-b47542bce3a7"
    Name: "Biter"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiSour"
  value:
  {
    AuthoredId: "df7a7826-c79d-ddd4-19e8-ae45761c6606"
    Name: "Senpai Sour"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryLimon"
  value:
  {
    AuthoredId: "3d5073ff-ef1e-a0c4-ab62-ab33506fabea"
    Name: "Legionary Limon"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Angry"
  value:
  {
    AuthoredId: "a743ed2a-07e7-6174-89a7-99fe14450eb0"
    Name: "The Angry"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Pie"
  value:
  {
    AuthoredId: "ed0ad4cf-e149-8ff4-daf7-470913e164b4"
    Name: "Pie"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressGrouchy"
  value:
  {
    AuthoredId: "c3b600ec-6f65-1754-7893-7915f4ca8b66"
    Name: "Mistress Grouchy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterAngry"
  value:
  {
    AuthoredId: "06fa50f5-3ff9-0a24-09f0-b0531110d297"
    Name: "Grand Master Angry"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Crunchy"
  value:
  {
    AuthoredId: "702b9682-1b20-5f64-89b1-e1618d95475e"
    Name: "The Crunchy"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Fried"
  value:
  {
    AuthoredId: "539be2c0-d837-db64-48e0-44de180b7a17"
    Name: "Fried"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Kicker"
  value:
  {
    AuthoredId: "27ead6fc-3970-0b24-3a61-6de0dae944a2"
    Name: "Kicker"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Sundae"
  value:
  {
    AuthoredId: "8c738b0c-3740-98c4-89d0-afc020cebdb9"
    Name: "Sundae"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Boxer"
  value:
  {
    AuthoredId: "4d1f8487-cbfb-d4a4-3bba-1afdf5f4369b"
    Name: "Boxer"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressToffee"
  value:
  {
    AuthoredId: "d8274d19-1cd7-1064-d926-54e853e81054"
    Name: "Mistress Toffee"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionLimon"
  value:
  {
    AuthoredId: "afe3a430-5a28-8364-4816-d5e635fffe68"
    Name: "Centurion Limon"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Swarmer"
  value:
  {
    AuthoredId: "38615891-5b20-4d04-08f1-78326bb08c6a"
    Name: "Swarmer"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressSweetAndSour"
  value:
  {
    AuthoredId: "2cdc510a-7e5d-a1e4-fa6c-297a75096938"
    Name: "Mistress Sweet and Sour"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirBlueberry"
  value:
  {
    AuthoredId: "61239c6d-f8b7-d894-aa22-8f5c3277286f"
    Name: "Sir Blueberry"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Sucker"
  value:
  {
    AuthoredId: "1e637829-09e6-0004-088e-ee921412a109"
    Name: "Sucker"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionRaspberry"
  value:
  {
    AuthoredId: "cc330116-0938-4cf4-db7a-b920c67881bc"
    Name: "Centurion Rasperry"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Everlasting"
  value:
  {
    AuthoredId: "2624782d-7c0f-3af4-b83f-e6c85f70fb70"
    Name: "The Everlasting"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Fighter"
  value:
  {
    AuthoredId: "65972c8e-af69-b824-2824-42c3da9b417f"
    Name: "Fighter"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Grumpy"
  value:
  {
    AuthoredId: "de7ff712-290c-ef84-ea17-814c7bd8e21f"
    Name: "The Grumpy"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Melted"
  value:
  {
    AuthoredId: "0c884c92-620a-36e4-5a10-324b9bb82b76"
    Name: "The Melted"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Candied"
  value:
  {
    AuthoredId: "3ba562b0-ed37-16b4-7aef-fd2426f367be"
    Name: "The Candied"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Clinch"
  value:
  {
    AuthoredId: "8dc56101-2057-7904-e84c-cc8d4f6ba616"
    Name: "Clinch"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiBashful"
  value:
  {
    AuthoredId: "7576872d-37e5-5cf4-d897-16a34bcf82fb"
    Name: "Senpai Bashful"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirSour"
  value:
  {
    AuthoredId: "3d2d537f-1faf-7d04-db38-7d49672c655d"
    Name: "Sir Sour"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Crunchy"
  value:
  {
    AuthoredId: "583dfad4-cb47-3874-6bc6-50c213aa845a"
    Name: "The Crunchy"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Icey"
  value:
  {
    AuthoredId: "ce4d88b6-f66c-aef4-8b17-d4d8683b95de"
    Name: "The Icey"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Frosted"
  value:
  {
    AuthoredId: "17b498aa-e23a-be54-082d-5e20a4d7c31e"
    Name: "The Frosted"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Cheesy"
  value:
  {
    AuthoredId: "9d3a60aa-f99d-fcd4-395b-52971c90ed8d"
    Name: "Cheesy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryTricky"
  value:
  {
    AuthoredId: "71bac394-69c3-b3c4-da79-b4e78171743e"
    Name: "Legionary Tricky"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterGlazed"
  value:
  {
    AuthoredId: "73ebc3ba-4eb7-8cc4-2acc-776ea96cee00"
    Name: "Grand Master Glazed"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressEverlasting"
  value:
  {
    AuthoredId: "162dfe1a-2a60-8ad4-a84d-0507bb41ced7"
    Name: "Mistress Everlasting"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Slugger"
  value:
  {
    AuthoredId: "5ca13528-1a6f-d874-1ac6-fc923a3a5d1b"
    Name: "Slugger"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirLifeSized"
  value:
  {
    AuthoredId: "a02c0515-83b2-d134-59a4-d0b1970fe7ae"
    Name: "Sir Life Sized"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressFlakey"
  value:
  {
    AuthoredId: "4e9f1065-cec8-8c54-5837-1453775598da"
    Name: "Mistress Flakey"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterFresh"
  value:
  {
    AuthoredId: "0332701c-ac69-1c44-788a-c41008f46e41"
    Name: "Grand Master Fresh"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Flying"
  value:
  {
    AuthoredId: "e872d8c3-4738-6584-da62-4c89b11ad2c4"
    Name: "The Flying"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionChewy"
  value:
  {
    AuthoredId: "99a8e8e0-f30b-45b4-da4f-37750d73eb76"
    Name: "Centurion Chewy"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Baked"
  value:
  {
    AuthoredId: "4b041e53-8746-d6d4-28ef-0cda9190f904"
    Name: "The Baked"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterMoldy"
  value:
  {
    AuthoredId: "c0a628a8-d4c4-c604-7aeb-68fdb0dcbcc7"
    Name: "Grand Master Moldy"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Grouchy"
  value:
  {
    AuthoredId: "18d201d4-25c3-b0a4-ba6a-41330f411266"
    Name: "Grouchy"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Prechewed"
  value:
  {
    AuthoredId: "1d78c94b-2cf8-00f4-1b3f-5f439ca2a5b6"
    Name: "Prechewed"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Pickle"
  value:
  {
    AuthoredId: "897d265a-25e5-8504-7981-7ad7f8d6d0ff"
    Name: "Pickle"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_OfLicorice"
  value:
  {
    AuthoredId: "74625e31-6b0e-9f44-c8c2-0b6a75db9238"
    Name: "Of Licorice"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionSpeedy"
  value:
  {
    AuthoredId: "608a914b-7581-eca4-7b58-607b7d6223be"
    Name: "Centurion Speedy"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressGreenApple"
  value:
  {
    AuthoredId: "4b2ba81d-8381-e0f4-4a51-0710be17d39e"
    Name: "Mistress Green Apple"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterSingleServing"
  value:
  {
    AuthoredId: "3785a246-b4e1-3074-a958-b670b142312d"
    Name: "Grand Master Single Serving"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterSugary"
  value:
  {
    AuthoredId: "6d4294c0-8003-0d14-393b-d2758bb3bf7d"
    Name: "Grand Master Sugary"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressLazy"
  value:
  {
    AuthoredId: "6f92870c-b2b2-bc74-9be9-38665cea4300"
    Name: "Mistress Lazy"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiTricky"
  value:
  {
    AuthoredId: "7cbd03fb-b649-58d4-8b0d-e191f706579f"
    Name: "Senpai Tricky"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionarySnozzberry"
  value:
  {
    AuthoredId: "ff261533-29a0-a4b4-894e-edb2da53d625"
    Name: "Legionary Snozzberry"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Blackberry"
  value:
  {
    AuthoredId: "6a4d45e2-c620-d474-59e9-cc2e4521658f"
    Name: "Blackberry"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterChocolatey"
  value:
  {
    AuthoredId: "8a7bd8f1-cf09-4bd4-f838-18ea87c08167"
    Name: "Grand Master Chocolatey"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Chewy"
  value:
  {
    AuthoredId: "d674f2db-7ec8-69e4-cbcb-6bd3a8949485"
    Name: "The Chewy"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressSpeedy"
  value:
  {
    AuthoredId: "b429d3d4-940c-9fb4-dacc-70f2e3970252"
    Name: "Mistress Speedy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiBadass"
  value:
  {
    AuthoredId: "e2786bd1-f780-4e14-a945-c3f12dfea6ab"
    Name: "Senpai Badass"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiAngry"
  value:
  {
    AuthoredId: "2b259744-76c6-fc14-fa5f-7e68ec3ca08a"
    Name: "Senpai Angry"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterCrazy"
  value:
  {
    AuthoredId: "6ca1b943-f196-f594-e9d1-14e3ef6a2d1c"
    Name: "Grand Master Crazy"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Grouchy"
  value:
  {
    AuthoredId: "06182793-2e49-22d4-18f8-f6b67a3902bc"
    Name: "Grouchy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryOrange"
  value:
  {
    AuthoredId: "02794e6b-15ff-5e34-7a9c-4ef394fa048b"
    Name: "Legionary Orange"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirStrawberry"
  value:
  {
    AuthoredId: "59d45749-313c-f234-ca3a-fd0b944989a3"
    Name: "Sir Stawberry"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Of_Sugarville"
  value:
  {
    AuthoredId: "dc1e1490-45be-3714-3b01-f5f5b3d81898"
    Name: "Of Sugarville"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirCandyCoated"
  value:
  {
    AuthoredId: "346ed4c0-08a1-0b54-ca90-d274be415d36"
    Name: "Sir Candy Coated"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Stale"
  value:
  {
    AuthoredId: "87c53ccb-2958-5a34-dad3-7edac6d7ff7e"
    Name: "Stale"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Fruity"
  value:
  {
    AuthoredId: "d20d50df-3a77-38b4-88d0-5dce2993dbe0"
    Name: "The Fruity"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionFruity"
  value:
  {
    AuthoredId: "f0f267d6-096d-4344-7b80-58fde14aed28"
    Name: "Centurion Fruity"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirCherryCola"
  value:
  {
    AuthoredId: "fde3a018-1cd7-4ea4-68db-d2a410cfb94f"
    Name: "Sir Cherry Cola"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Breaker"
  value:
  {
    AuthoredId: "cbfca0fd-cdf2-04e2-d85d-b959c255c6a9"
    Name: "Breaker"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiStrawberry"
  value:
  {
    AuthoredId: "4228fa69-c01b-a234-b9ac-082c0dc36e8d"
    Name: "Senpai Stawberry"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryMalted"
  value:
  {
    AuthoredId: "c27132bc-ebb6-af44-ca69-3e13de31e7f7"
    Name: "Legionary Malted"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Brandied"
  value:
  {
    AuthoredId: "3db7f30d-09fa-e624-0a2f-d007beee75d3"
    Name: "Brandied"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Sprinkled"
  value:
  {
    AuthoredId: "daf101d8-71f7-e4c4-eb2a-e9d27836346f"
    Name: "Sprinkled"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_RootBeer"
  value:
  {
    AuthoredId: "4f6271c6-2b5d-ba34-c8ba-4f8936981e92"
    Name: "Root Beer"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Bashful"
  value:
  {
    AuthoredId: "6a5659a4-03bd-2c84-2a9d-8d284c4c3056"
    Name: "The Bashful"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionDeepFried"
  value:
  {
    AuthoredId: "64a83d55-c2e8-6e14-494a-d823d4edfba0"
    Name: "Centurion Deep Fried"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterCheesy"
  value:
  {
    AuthoredId: "93066010-4c50-fb14-8a72-ac5405547550"
    Name: "Grand Master Cheesy"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Tasty"
  value:
  {
    AuthoredId: "8e79b224-f3f5-1104-3b8e-d798df8b5f01"
    Name: "The Tasty"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Bob"
  value:
  {
    AuthoredId: "9178c085-e0d8-83c4-9b2b-83e1c5f1477d"
    Name: "Bob"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiOrange"
  value:
  {
    AuthoredId: "15319476-99ef-9254-9a30-abdc53488f3b"
    Name: "Senpai Orange"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiSalted"
  value:
  {
    AuthoredId: "7a8cf3ea-455f-e3a4-6992-f89d8b032901"
    Name: "Senpai Salted"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiCherryCola"
  value:
  {
    AuthoredId: "85c7527d-df98-6104-8816-35cc55edae43"
    Name: "Senpai Cherry Cola"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Pop"
  value:
  {
    AuthoredId: "31784ee9-d5e2-e6c4-7875-b40f06a7e2c1"
    Name: "Pop"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Southpaw"
  value:
  {
    AuthoredId: "e3e37301-02d2-5854-59b5-f33e6510125d"
    Name: "Southpaw"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterMalted"
  value:
  {
    AuthoredId: "b7d17a71-3bc0-9714-8827-20aa647ad61b"
    Name: "Grand Master Malted"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Biter"
  value:
  {
    AuthoredId: "8a47a7fb-446d-bb94-3b8c-943acc333706"
    Name: "Biter"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryFlying"
  value:
  {
    AuthoredId: "d9998cb8-a8fa-d164-5b10-09a4c823a208"
    Name: "Legionary Flying"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterPineapple"
  value:
  {
    AuthoredId: "840e18a1-e3fd-a234-8b78-77f2f10954cd"
    Name: "Grand Master Pineapple"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterSpicy"
  value:
  {
    AuthoredId: "2901eb01-e50c-49a4-9ad2-d9b3cdf7a979"
    Name: "Grand Master Spicy"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiMelted"
  value:
  {
    AuthoredId: "26bc8a83-d428-1a64-b843-7de22bf10e1f"
    Name: "Senpai Melted"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirPlump"
  value:
  {
    AuthoredId: "c304922c-4fd0-3bb4-daef-fbad2a1709f8"
    Name: "Sir Plump"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Blueberry"
  value:
  {
    AuthoredId: "7b907dd9-1ee5-a864-5b25-00f5286b2f1f"
    Name: "Blueberry"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressSugary"
  value:
  {
    AuthoredId: "d5a11f91-5102-d464-dae4-240ad0007030"
    Name: "Mistress Sugary"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionarySweetAndSour"
  value:
  {
    AuthoredId: "d4b10b2b-d5dd-a014-09a6-d9ccea8a71e5"
    Name: "Legionary Sweet and Sour"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryCandied"
  value:
  {
    AuthoredId: "5b73a687-3660-6554-6b6b-86e86b784815"
    Name: "Legionary Candied"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Dopey"
  value:
  {
    AuthoredId: "35a0db2c-98ab-e224-ea5d-171e67900347"
    Name: "The Dopey"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Chopped"
  value:
  {
    AuthoredId: "76308cf0-8147-87f4-898f-510a91190a4b"
    Name: "Chopped"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Frosted"
  value:
  {
    AuthoredId: "f01a88d2-ec4c-3c64-982e-f27f68a8ae82"
    Name: "Frosted"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Sour"
  value:
  {
    AuthoredId: "ad7ae213-2552-0364-1aaf-e7c480c2709c"
    Name: "Sour"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Salty"
  value:
  {
    AuthoredId: "4659978d-0fd8-5f04-0bc9-c6017bca52a6"
    Name: "The Salty"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryLemon"
  value:
  {
    AuthoredId: "15cb54e5-e119-02e4-fa2a-df8b57ed5318"
    Name: "Legionary Lemony"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Swarmer"
  value:
  {
    AuthoredId: "bab347e1-4ebc-1094-3824-0e432a2ea1cd"
    Name: "Swarmer"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Boxer"
  value:
  {
    AuthoredId: "ffd1d814-8b82-f464-390a-3997a2de02b1"
    Name: "Boxer"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressCaramelized"
  value:
  {
    AuthoredId: "2391634a-45bd-ad54-db3f-2c6d2f7f2a63"
    Name: "Mistress Caramelized"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Southpaw"
  value:
  {
    AuthoredId: "ac33113a-735b-5ac4-5ac8-0abd98eb309d"
    Name: "Southpaw"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Of_Gummiway"
  value:
  {
    AuthoredId: "4c656185-cb56-4424-084d-e54f3f97783a"
    Name: "Of Gummiway"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiFlying"
  value:
  {
    AuthoredId: "4398308c-7236-8e04-9b49-004026cbd75e"
    Name: "Senpai Flying"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionBiteSized"
  value:
  {
    AuthoredId: "e28cbed9-3d45-04a4-3aad-f198d150c043"
    Name: "Centurion Bite Sized"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirSingleServing"
  value:
  {
    AuthoredId: "3458ff68-6ecb-cc24-daf2-68fc9bcede1d"
    Name: "Sir Single Serving"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirBitter"
  value:
  {
    AuthoredId: "6f8b939f-3b33-1604-384a-5b0b59012cac"
    Name: "Sir Bitter"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiHotButtered"
  value:
  {
    AuthoredId: "3fd64e11-cda5-8024-b92d-5036004163fb"
    Name: "Senpai Hot Buttered"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterCandyCoated"
  value:
  {
    AuthoredId: "f3c9b4c2-b446-f9f4-4ba1-adad7ecf7a58"
    Name: "Grand Master Candy Coated"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiCrazy"
  value:
  {
    AuthoredId: "4b112743-b217-f6e4-b9f2-0276076123eb"
    Name: "Senpai Crazy"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_LifeSized"
  value:
  {
    AuthoredId: "36fce2d5-996c-4754-da9f-230c99d6764b"
    Name: "Life Sized"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Butterscotch"
  value:
  {
    AuthoredId: "7473bbae-b818-66d4-2a17-6937facdc387"
    Name: "Butterscotch"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Chopped"
  value:
  {
    AuthoredId: "0496a89f-ef86-62d4-3b29-227c6f72d79c"
    Name: "Chopped"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionCandied"
  value:
  {
    AuthoredId: "cbf82684-ac9b-a5b4-48e3-f594de9188c8"
    Name: "Centurion Candied"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Puncher"
  value:
  {
    AuthoredId: "591bad65-e2db-d6a4-2b9f-44c028f48063"
    Name: "Puncher"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Brawler"
  value:
  {
    AuthoredId: "021a9537-e0ce-73a4-e883-5bccbf214eee"
    Name: "Brawler"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Cherry"
  value:
  {
    AuthoredId: "bee49c0f-a6b0-2c14-6ae7-6cb53a6c3ff9"
    Name: "The Cherry"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionLazy"
  value:
  {
    AuthoredId: "9a9ab9e1-6591-63b4-298c-644dea99493e"
    Name: "Centurion Lazy"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionBrandied"
  value:
  {
    AuthoredId: "86a888b0-e40c-9554-9bd6-012bcf6556fc"
    Name: "Centurion Brandied"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Jelly"
  value:
  {
    AuthoredId: "46dcd591-72b5-a534-8854-0c0f887035e2"
    Name: "Jelly"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressFried"
  value:
  {
    AuthoredId: "65320306-14ef-ea34-989b-2a13160e8d2e"
    Name: "Mistress Fried"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Bitter"
  value:
  {
    AuthoredId: "63e5ad21-f808-a2f4-19f4-53c200f8eeef"
    Name: "Bitter"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Breaker"
  value:
  {
    AuthoredId: "81cdfad1-ecf8-c474-4b01-062e935891d4"
    Name: "Breaker"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Baked"
  value:
  {
    AuthoredId: "a5d42908-ecba-46f4-2b14-b72e2beed813"
    Name: "Baked"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_RootBeer"
  value:
  {
    AuthoredId: "ce7630ec-ecec-7a94-4bc7-c2961930b529"
    Name: "Root Beer"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Candied"
  value:
  {
    AuthoredId: "84fbe349-52fa-4174-0b3e-cecca4722d88"
    Name: "The Candied"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirFruity"
  value:
  {
    AuthoredId: "6fc2a87a-776b-2424-397f-cb60e3be7e72"
    Name: "Sir Fruity"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiLazy"
  value:
  {
    AuthoredId: "aa62a724-ec49-0404-0add-1795066ff418"
    Name: "Senpai Lazy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterIcey"
  value:
  {
    AuthoredId: "f84ff86d-0ba0-4114-d8fa-e9c2220b7c30"
    Name: "Grand Master Icey"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Buttered"
  value:
  {
    AuthoredId: "2fc88e78-0ed0-61e4-cbeb-32a0d4314cc4"
    Name: "The Buttered"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryPineapple"
  value:
  {
    AuthoredId: "89a8f7d3-d2bd-9734-3ab8-797d8d6c0c84"
    Name: "Legionary Pineapple"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterButterscotch"
  value:
  {
    AuthoredId: "2bf8b981-b021-5bf4-8895-5d2ef1e0fa5e"
    Name: "Grand Master Butterscotch"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Single_Serving"
  value:
  {
    AuthoredId: "e2228468-6abb-4eb4-4899-e1ea98936349"
    Name: "The Single Serving"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Slugger"
  value:
  {
    AuthoredId: "20adb91e-e19d-1ff4-db27-b12cab36ed7e"
    Name: "Slugger"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Slip"
  value:
  {
    AuthoredId: "2efc1ec6-b258-b1b4-cb2b-b7ea0b60fa2e"
    Name: "Slip"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Prechewed"
  value:
  {
    AuthoredId: "e2ff9949-bf21-3cb4-8b3a-feac5fe18f73"
    Name: "The Prechewed"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Fried"
  value:
  {
    AuthoredId: "fdb37126-da0f-c024-3813-a67a4e95f29b"
    Name: "The Fried"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Candy_Coated"
  value:
  {
    AuthoredId: "af03f492-a72b-6fd4-4b7d-dfd9f667bd52"
    Name: "The Candy Coated"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionHotButtered"
  value:
  {
    AuthoredId: "27c6bf3b-5e60-ded4-5be2-2dc579f73f2b"
    Name: "Centurion Hot Buttered"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiSpicy"
  value:
  {
    AuthoredId: "169aa258-fc36-27d4-da90-f85cb1375ff7"
    Name: "Senpai Spicy"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressCherry"
  value:
  {
    AuthoredId: "201a0468-d77f-4224-2807-73d2bae5bab1"
    Name: "Mistress Cherry"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterChewy"
  value:
  {
    AuthoredId: "60799ff9-b2ce-af74-5a4f-431905a5373e"
    Name: "Grand Master Chewy"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryToffee"
  value:
  {
    AuthoredId: "55628be7-356f-6894-9898-ad1d7cc77472"
    Name: "Legionary Toffee"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryThornberry"
  value:
  {
    AuthoredId: "882742a8-b5b4-ea24-9851-5b5354e9c09a"
    Name: "Legionary Thornbery"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Dusted"
  value:
  {
    AuthoredId: "e8d47787-2723-7274-3a70-f3987ee89c5c"
    Name: "The Dusted"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Buttered"
  value:
  {
    AuthoredId: "03746f71-471a-2654-fa09-bb38f96d8b75"
    Name: "Buttered"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Crusty"
  value:
  {
    AuthoredId: "8dd9f4ed-63d1-f874-89c5-911e83f56817"
    Name: "Crusty"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Swarmer"
  value:
  {
    AuthoredId: "e4d14f3e-98b3-37e4-9985-633b8e1be688"
    Name: "Swarmer"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Of_Mintdom"
  value:
  {
    AuthoredId: "d1b75f93-b3f7-5834-c8ae-b515ad951185"
    Name: "Of Mintdom"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Noogie"
  value:
  {
    AuthoredId: "cfcdbb6c-e537-5ab4-9ae8-a5933c71ec76"
    Name: "Noogie"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Creamy"
  value:
  {
    AuthoredId: "42e7556b-35e6-1084-49b3-82b00eb0b840"
    Name: "Creamy"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Salty"
  value:
  {
    AuthoredId: "ac6a6871-f490-2e44-da49-66ec2fbebc61"
    Name: "Salty"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Strawberry"
  value:
  {
    AuthoredId: "ade65b6a-ed6d-ed24-1beb-b68c49a130a2"
    Name: "Stawberry"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Powdered"
  value:
  {
    AuthoredId: "abedf771-9e53-1a04-1b4c-26378da91796"
    Name: "The Powdered"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Minty"
  value:
  {
    AuthoredId: "edf2ce20-2019-1dd4-f8c6-cdc7c1dd1b8c"
    Name: "Minty"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirLazy"
  value:
  {
    AuthoredId: "7ae1de29-7d7f-5674-184e-93eb03311cf0"
    Name: "Sir Lazy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiToffee"
  value:
  {
    AuthoredId: "1a04fcc7-7d1c-3a74-da29-fc0437c33835"
    Name: "Senpai Toffee"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Snozzberry"
  value:
  {
    AuthoredId: "49740a14-d535-6d84-b9f7-def1a235d613"
    Name: "Snozzberry"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressIcey"
  value:
  {
    AuthoredId: "441414ca-7cdd-a3c4-38ac-004e426aaf09"
    Name: "Mistress Icey"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Baked"
  value:
  {
    AuthoredId: "8ebf6fd3-02ac-75f4-18df-022cae9f53ca"
    Name: "The Baked"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirFrosted"
  value:
  {
    AuthoredId: "5e098f61-2446-bb94-ba2c-c639b9e7a310"
    Name: "Sir Frosted"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryBashful"
  value:
  {
    AuthoredId: "a7c78bc2-c3ca-5f64-a95e-853b6032482b"
    Name: "Legionary Vanilla"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Jelly"
  value:
  {
    AuthoredId: "6d9fa111-5c23-29d4-0a19-f3ad02c62473"
    Name: "Jelly"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterTravelSized"
  value:
  {
    AuthoredId: "1bee3521-2a85-2884-3995-438fa9668bfb"
    Name: "Grand Master Travel Sized"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_CherryCola"
  value:
  {
    AuthoredId: "998571bb-3c1c-3f14-f807-aaedd01e55c7"
    Name: "Cherry Cola"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Fresh"
  value:
  {
    AuthoredId: "c1f0a879-ed3f-5dd4-4989-5c684b189fc5"
    Name: "The Fresh"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Slapper"
  value:
  {
    AuthoredId: "e37d6015-b118-3474-7ae2-9504cd41f85a"
    Name: "Slapper"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Crusty"
  value:
  {
    AuthoredId: "3784cd0a-c629-1df4-e938-f8e28adec79a"
    Name: "Crusty"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Bitter"
  value:
  {
    AuthoredId: "73c3d404-cdad-1ab4-6adb-f203b802691e"
    Name: "Bitter"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Breaker"
  value:
  {
    AuthoredId: "13cc6e1c-bedc-7054-38c7-8565019d290c"
    Name: "Breaker"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionSquishy"
  value:
  {
    AuthoredId: "22ba230b-6687-87c4-882d-d9588bd74406"
    Name: "Centurion Squishy"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Dusted"
  value:
  {
    AuthoredId: "21c36ea9-1b2c-32c4-78c8-692bc1dfd62f"
    Name: "The Dusted"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionDusted"
  value:
  {
    AuthoredId: "c7a7243b-6e86-12f4-f994-45e77d4c3720"
    Name: "Centurion Dusted"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Striker"
  value:
  {
    AuthoredId: "279744f9-cd5b-2e94-fb95-8a403885bc09"
    Name: "Striker"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressStale"
  value:
  {
    AuthoredId: "5937d22d-15be-3304-584d-d8c4324601d9"
    Name: "Mistress Stale"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Bashful"
  value:
  {
    AuthoredId: "fbe689e5-10f7-a2f4-9b1e-ca4531d204c8"
    Name: "The Bashful"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressStrawberry"
  value:
  {
    AuthoredId: "4f4111f4-de99-5434-b8e2-f72e4d959c6d"
    Name: "Mistress Stawberry"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Caramel"
  value:
  {
    AuthoredId: "0bc00a87-9f60-c794-29d7-8e48fdea0e31"
    Name: "Caramel"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Happy"
  value:
  {
    AuthoredId: "3e472012-1543-24e5-2820-450448c583ca"
    Name: "Happy"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Switch_Hitter"
  value:
  {
    AuthoredId: "5e3c369b-1aa7-f814-7b88-6acce2cc3fa8"
    Name: "Switch-Hitter"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Fast"
  value:
  {
    AuthoredId: "474abc49-94a0-0564-baa9-c86676bed539"
    Name: "Fast"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirFizzy"
  value:
  {
    AuthoredId: "08238347-5781-7a94-0ac2-8737cd3bd634"
    Name: "Sir Fizzy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryCoconut"
  value:
  {
    AuthoredId: "a5a90133-2bfc-10e4-5af1-ba997b62daac"
    Name: "Legionary Coconut"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiChewy"
  value:
  {
    AuthoredId: "e0004f3e-6494-6e84-a99b-758b19ba6793"
    Name: "Senpai Chewy"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Mango"
  value:
  {
    AuthoredId: "309373f1-fa28-ee74-784e-9db79e712528"
    Name: "Mango"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionToffee"
  value:
  {
    AuthoredId: "af422ec7-f704-4bc4-bbcc-9e0528c389d5"
    Name: "Centurion Toffee"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryPlump"
  value:
  {
    AuthoredId: "d93cc817-a507-ef24-3a9f-d2bb45d717c0"
    Name: "Legionary Plump"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Limon"
  value:
  {
    AuthoredId: "272fbdba-5f2e-ef14-5826-81d7fe233007"
    Name: "Limon"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterCrusty"
  value:
  {
    AuthoredId: "0c678bfe-dd89-9004-a9a2-06fb55cf1b5c"
    Name: "Grand Master Crusty"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Sundae"
  value:
  {
    AuthoredId: "653f0ddd-4e1a-16d4-f827-8fa44766c03f"
    Name: "Sundae"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Puncher"
  value:
  {
    AuthoredId: "067845dc-9bf4-24d4-dae7-b79240f1489a"
    Name: "Puncher"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Poker"
  value:
  {
    AuthoredId: "5f06b618-094d-9f54-198b-b36f6a648cbb"
    Name: "Poker"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Brawler"
  value:
  {
    AuthoredId: "50d28f64-f83d-ec74-999e-ab956088fa46"
    Name: "Brawler"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Of_Jellyton"
  value:
  {
    AuthoredId: "ba51c5a2-26bb-01b4-ab91-c89075e7e4ec"
    Name: "Of Jellyton"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Swarmer"
  value:
  {
    AuthoredId: "e4d6cc68-c7a3-f3d4-59b8-4ee229948719"
    Name: "Swarmer"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Tasty"
  value:
  {
    AuthoredId: "7a9e3932-9f2f-d094-0be6-e4457a95fd68"
    Name: "The Tasty"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Creamy"
  value:
  {
    AuthoredId: "e933f1bc-8601-6d64-2981-c7bf168695c6"
    Name: "Creamy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiTravelSized"
  value:
  {
    AuthoredId: "7948b54a-9ed6-9934-f8aa-2af200e8f17f"
    Name: "Senpai Travel Sized"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressBaked"
  value:
  {
    AuthoredId: "c44e1823-87b8-f9c4-e934-8c06606f961a"
    Name: "Mistress Baked"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Fried"
  value:
  {
    AuthoredId: "71000eb3-b8e5-58b4-188a-a59b449f5adf"
    Name: "The Fried"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Prechewed"
  value:
  {
    AuthoredId: "202c6ed4-d4b6-ebe4-2bd8-ac1692bc4231"
    Name: "The Prechewed"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Of_Gummiway"
  value:
  {
    AuthoredId: "87e279cb-fe19-6984-3b39-351f241c7b5a"
    Name: "Of Gummiway"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionCoconut"
  value:
  {
    AuthoredId: "94061318-376d-2304-197b-27fe2d62d3eb"
    Name: "Centurion Coconut"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiGrouchy"
  value:
  {
    AuthoredId: "49d04a83-80a2-62a4-7994-3842aff00ffc"
    Name: "Senpai Grouchy"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionSprinkled"
  value:
  {
    AuthoredId: "9aa9d254-1731-5ac4-9bb4-a2205ca8e0a9"
    Name: "Centurion Sprinkled"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionGrouchy"
  value:
  {
    AuthoredId: "7885c46a-2c8c-2ae4-3987-3ae5f20ce642"
    Name: "Centurion Grouchy"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirHappy"
  value:
  {
    AuthoredId: "1e812acd-2aa4-a904-c9dc-87b3bb4e217e"
    Name: "Sir Happy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Quick"
  value:
  {
    AuthoredId: "f3fe88bf-8781-8ce4-69f6-1721243fa712"
    Name: "Quick"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressDeepFried"
  value:
  {
    AuthoredId: "dd18e5a7-51fc-bb34-5bcb-b98ed73bbcb8"
    Name: "Mistress Deep Fried"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Dopey"
  value:
  {
    AuthoredId: "7ba78947-df04-4f94-cbdd-46e4a00493d9"
    Name: "The Dopey"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Tricky"
  value:
  {
    AuthoredId: "ef9fb786-b0a5-7462-0aad-bf74ca633b61"
    Name: "Tricky"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Cover-up"
  value:
  {
    AuthoredId: "0201c9d7-91f2-3374-0840-e63c9cdb122f"
    Name: "Cover-up"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiFresh"
  value:
  {
    AuthoredId: "de8b1cab-8eca-3314-abb5-6e0d8804fb12"
    Name: "Senpai Fresh"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Butterscotch"
  value:
  {
    AuthoredId: "f91f8317-6d26-03d4-baa7-075fbdf9c570"
    Name: "Butterscotch"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiPineapple"
  value:
  {
    AuthoredId: "5e60f84a-9816-cde4-786b-fccc8f44d626"
    Name: "Senpai Pineapple"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterBitter"
  value:
  {
    AuthoredId: "22af4fc6-67b3-4884-da5e-10c2b0328e66"
    Name: "Grand Master Bitter"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Salty"
  value:
  {
    AuthoredId: "0e38abd7-b717-e314-3872-3dbefcb46d2f"
    Name: "The Salty"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryBrandied"
  value:
  {
    AuthoredId: "686c5a3e-0f70-0644-49a0-a77726d92861"
    Name: "Legionary Brandied"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Fast"
  value:
  {
    AuthoredId: "e2f80dbd-36e2-4334-09d0-3567a6b8d8d1"
    Name: "Fast"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Grouchy"
  value:
  {
    AuthoredId: "e2ae1ad6-9137-7a64-f9b2-f26b8f13cb5c"
    Name: "The Grouchy"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Brandied"
  value:
  {
    AuthoredId: "b8ebbed0-c38f-45d4-197a-c69bfeb63e31"
    Name: "The Brandied"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryStrawberry"
  value:
  {
    AuthoredId: "830cce44-1755-7f74-289d-6ab307b046a6"
    Name: "Legionary Stawberry"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Fast"
  value:
  {
    AuthoredId: "b2912b36-c5bb-5490-69b0-3fdd9c58735d"
    Name: "Fast"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionBlueberry"
  value:
  {
    AuthoredId: "7406dd51-2fdf-b044-6ba3-2558359a9550"
    Name: "Centurion Blueberry"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirDukeCaramel"
  value:
  {
    AuthoredId: "af2082f8-17f5-ed54-b9b1-9f3f4b8acda4"
    Name: "Sir Legionary Caramel"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Sugary"
  value:
  {
    AuthoredId: "7e93d52f-b92a-e044-da61-064c98d404b9"
    Name: "The Sugary"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Lazy"
  value:
  {
    AuthoredId: "9760a57d-deea-f824-caab-ec0bd40e7ff0"
    Name: "Lazy"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryFlakey"
  value:
  {
    AuthoredId: "d5038c01-76c9-56c4-dbc2-3b20bba55443"
    Name: "Legionary Flakey"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_FrenchVanilla"
  value:
  {
    AuthoredId: "198da9a6-970e-08b4-eb75-39a61dbbd213"
    Name: "French Vanilla"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryAngry"
  value:
  {
    AuthoredId: "1dcec88b-0b78-f3e4-fab5-803115d18286"
    Name: "Legionary  Angry"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryCherryCola"
  value:
  {
    AuthoredId: "25d5c8bb-6576-e334-6a2e-d917dddd9365"
    Name: "Legionary Cherry Cola"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionTravelSized"
  value:
  {
    AuthoredId: "0dd0dbcc-3906-1794-fa5b-eeb81bd56ed3"
    Name: "Centurion Travel Sized"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirMelon"
  value:
  {
    AuthoredId: "db86de37-cf2e-70d4-4a6b-c394ae0163f1"
    Name: "Sir Melon"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressButtered"
  value:
  {
    AuthoredId: "2330f735-1807-d3e4-5b68-52f64f33569d"
    Name: "Mistress Buttered"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirDusted"
  value:
  {
    AuthoredId: "86c691b9-35fe-52d4-fadb-9669d60b685f"
    Name: "Sir Dusted"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryPowdered"
  value:
  {
    AuthoredId: "27a46dd0-7059-80e4-39af-99bfdf342289"
    Name: "Legionary Powdered"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Of_Mintdom"
  value:
  {
    AuthoredId: "de1254a2-8a80-bc24-8ba5-21b9923b1576"
    Name: "Of Mintdom"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Powdered"
  value:
  {
    AuthoredId: "08c0ba8b-6169-8ce4-a808-359c0eaf73cd"
    Name: "The Powdered"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressBadass"
  value:
  {
    AuthoredId: "c576adc5-80bf-b394-8a3f-cd0735ec4dd8"
    Name: "Mistress Badass"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Checkhook"
  value:
  {
    AuthoredId: "8ce14162-14b5-5874-cafa-b407ddc2e712"
    Name: "Checkhook"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Pickle"
  value:
  {
    AuthoredId: "ba512d07-7573-7b54-ca12-a8c29846a228"
    Name: "Pickle"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionLifeSized"
  value:
  {
    AuthoredId: "61479fb1-df75-15a4-4bed-0a4e40a7ebb9"
    Name: "Centurion Life Sized"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressSneezy"
  value:
  {
    AuthoredId: "c3bc930d-8a74-2f34-6b1d-409e6e1c022a"
    Name: "Mistress Sneezy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Biter"
  value:
  {
    AuthoredId: "556e1578-19db-7714-88b7-4df1b3151b04"
    Name: "Biter"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Chocolatey"
  value:
  {
    AuthoredId: "8926399c-e98f-30a4-d8c9-03bbb13b6ca1"
    Name: "Chocolatey"
    Position: 0
    Quality: 0
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Orange"
  value:
  {
    AuthoredId: "8e0c475c-c584-6864-db8e-befa1f477809"
    Name: "Orange"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Puncher"
  value:
  {
    AuthoredId: "bffd0236-a495-1884-18a1-45da85e52752"
    Name: "Puncher"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Brawler"
  value:
  {
    AuthoredId: "591d4c02-6881-ea64-a91b-91ff27d13a35"
    Name: "Brawler"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionStale"
  value:
  {
    AuthoredId: "4f876b51-301a-e054-bac4-028c59a29b8e"
    Name: "Centurion Stale"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Jelly"
  value:
  {
    AuthoredId: "db56f53b-fb7d-e234-f845-4d3de9fd3d81"
    Name: "Jelly"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Bob"
  value:
  {
    AuthoredId: "db665c78-3c98-b6e4-18eb-111d41f07dee"
    Name: "Bob"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiFlakey"
  value:
  {
    AuthoredId: "8dedc92a-4e94-1c64-ca4e-6dc548c848af"
    Name: "Senpai Flakey"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterRaspberry"
  value:
  {
    AuthoredId: "4d9a3106-a392-cac4-2bb3-962edd86a38a"
    Name: "Grand Master Rasperry"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionarySpeedy"
  value:
  {
    AuthoredId: "e59c81d2-e67a-d974-4a2b-94cebf2b0f15"
    Name: "Legionary Speedy"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryCaramel"
  value:
  {
    AuthoredId: "7ec99a86-428e-5cf4-5a0c-1c1b3ac8e0f3"
    Name: "Legionary Caramel"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Melted"
  value:
  {
    AuthoredId: "bcd51415-f64c-b6b4-e846-0fbc603b877d"
    Name: "Melted"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Glazed"
  value:
  {
    AuthoredId: "ab5bd0d7-df27-82b4-c818-08136a24a11c"
    Name: "The Glazed"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressDopey"
  value:
  {
    AuthoredId: "ae28c0f0-8dbb-1524-eb9f-1c415e4dfe24"
    Name: "Mistress Dopey"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Bite_Sized"
  value:
  {
    AuthoredId: "023ce678-513b-c5d4-68b1-3bc4a003cbb3"
    Name: "The Bite Sized"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiChopped"
  value:
  {
    AuthoredId: "f137cdc3-dcd5-61c4-99d6-373eb7f815e4"
    Name: "Senpai Chopped"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Striker"
  value:
  {
    AuthoredId: "59c9f245-497a-9f84-1bf7-9536422f6307"
    Name: "Striker"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Strong"
  value:
  {
    AuthoredId: "cb7c0ce5-d115-fc54-d9db-55741492e67a"
    Name: "The Strong"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressSalty"
  value:
  {
    AuthoredId: "e80780e9-1db8-d074-3963-67f3db99904b"
    Name: "Mistress Salty"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionTricky"
  value:
  {
    AuthoredId: "af99dc9c-b65c-6f34-9a2b-41483367c61c"
    Name: "Centurion Tricky"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirCrunchy"
  value:
  {
    AuthoredId: "52ee92b8-7dda-09c4-1b98-facb14938ca2"
    Name: "Sir Crunchy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Kicker"
  value:
  {
    AuthoredId: "52c135ad-855a-0334-3b10-f0cf5de00fe8"
    Name: "Kicker"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressCandied"
  value:
  {
    AuthoredId: "f23c5fac-4756-16c4-7a80-28bb27bb5a54"
    Name: "Mistress Candied"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterLifeSized"
  value:
  {
    AuthoredId: "cd2d9be8-ec9b-b1e4-5ba4-654cde12136a"
    Name: "Grand Master Life Sized"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Switch_Hitter"
  value:
  {
    AuthoredId: "6e2fa335-9ba5-4dc4-5ade-50a56c5cb7de"
    Name: "Switch-Hitter"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Flying"
  value:
  {
    AuthoredId: "69ed2fee-eb68-c334-d8ed-14b57f31ea35"
    Name: "Flying"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressPineapple"
  value:
  {
    AuthoredId: "630fa3bb-50e4-8574-9808-2a7aef54a47f"
    Name: "Mistress Pineapple"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiSpeedy"
  value:
  {
    AuthoredId: "8d45e01d-0a61-6204-4a39-8d8db695d77c"
    Name: "Senpai Speedy"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Lemony"
  value:
  {
    AuthoredId: "ddcb0293-8258-6c54-a92e-0da3b6081df1"
    Name: "The Lemony"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionBaked"
  value:
  {
    AuthoredId: "6edfe2d6-5c56-4044-3833-64a326d5a034"
    Name: "Centurion Baked"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterBrandied"
  value:
  {
    AuthoredId: "235bd0ad-afaa-cfc4-fb6c-8945ad3b5f6c"
    Name: "Grand Master Brandied"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiRootBeer"
  value:
  {
    AuthoredId: "afd2c059-da9a-3664-dbdf-0242643ef023"
    Name: "Senpai Root Beer"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirBrandied"
  value:
  {
    AuthoredId: "9a4a3b7b-b64a-b384-ba7c-237c3a48ac6e"
    Name: "Sir Brandied"
    Position: 0
    Quality: 0
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Clinch"
  value:
  {
    AuthoredId: "00c0bee7-1a5f-5474-58e5-3f1fc43d7f9c"
    Name: "Clinch"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Frosted"
  value:
  {
    AuthoredId: "79a1e481-2c89-4c24-583b-9a5d2d3e7fe6"
    Name: "Frosted"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_OverhandRight"
  value:
  {
    AuthoredId: "96d921c8-57cd-5814-0a24-10d07d7b3f7a"
    Name: "Overhand Right"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiDeepFried"
  value:
  {
    AuthoredId: "aa76e47d-b811-f244-8876-1f873b7198f7"
    Name: "Senpai Deep Fried"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Of_Sourway"
  value:
  {
    AuthoredId: "e9a7c2b2-cd80-1784-a983-c64c09ab089b"
    Name: "Of Sourway"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Icey"
  value:
  {
    AuthoredId: "6f192a6b-22f4-7574-0851-7c4d7012e444"
    Name: "The Icey"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionarySugary"
  value:
  {
    AuthoredId: "f36b5e93-963b-6144-f934-fcef45c3149a"
    Name: "Senpai Sugarie"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Cherry"
  value:
  {
    AuthoredId: "88ab98cd-4bd7-2774-2954-261112ef1488"
    Name: "The Cherry"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Happy"
  value:
  {
    AuthoredId: "882d694b-9ee0-b2e4-fab8-67d06f6a5c58"
    Name: "The Happy"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterFried"
  value:
  {
    AuthoredId: "85d4e96c-fd39-3de4-d825-2e7cff1d9694"
    Name: "Grand Master Fried"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_BoloPunch"
  value:
  {
    AuthoredId: "a008c134-9e2c-d3c4-a9c0-cd5d7c8f6d66"
    Name: "Bolo Punch"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Pudding"
  value:
  {
    AuthoredId: "eaf31d35-59ce-6483-98ac-b3ebdd8def73"
    Name: "Pudding"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Fighter"
  value:
  {
    AuthoredId: "a1021826-7db9-7394-ea98-3dc11464ea7d"
    Name: "Fighter"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Counterpuncher"
  value:
  {
    AuthoredId: "f270c19c-1559-3ac4-e84e-ea1ee57314cf"
    Name: "Counterpuncher"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Lemon"
  value:
  {
    AuthoredId: "da495b54-3b76-7468-9ba8-63923c2787b6"
    Name: "Lemony"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Cutie"
  value:
  {
    AuthoredId: "9c469beb-d69e-f194-2b47-f936521efdfa"
    Name: "Cutie"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Boiled"
  value:
  {
    AuthoredId: "2ac57db2-93ae-e2d4-3a21-de024243a02f"
    Name: "The Boiled"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionRolled"
  value:
  {
    AuthoredId: "82561caa-97ac-b5e4-bbcc-a2888b902b31"
    Name: "Centurion Rolled"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirPowdered"
  value:
  {
    AuthoredId: "3b212626-436d-e6f4-3ba8-73357e853d54"
    Name: "Sir Powdered"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Blackberry"
  value:
  {
    AuthoredId: "f13975f8-8fbb-edb4-aaa1-a77d6b55ecb6"
    Name: "Blackberry"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryRaspberry"
  value:
  {
    AuthoredId: "0b0b4698-833f-57c4-fae1-24195160c4cf"
    Name: "Legionary Rasperry"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Puncher"
  value:
  {
    AuthoredId: "8749bc04-7341-25e4-2b5a-4af06ea2e182"
    Name: "Puncher"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Brawler"
  value:
  {
    AuthoredId: "ed4313f5-92fb-5364-59fb-6007ef4a5c9e"
    Name: "Brawler"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiBrandied"
  value:
  {
    AuthoredId: "541d1ae5-547f-b6e4-fafb-55d4489c3129"
    Name: "Senpai Brandied"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirMelty"
  value:
  {
    AuthoredId: "6eb591f4-47e5-7644-7909-96690f3e4c14"
    Name: "Sir Melty"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_SweetAndSour"
  value:
  {
    AuthoredId: "0b99381c-6b3d-d394-6835-64f1c2b8739d"
    Name: "Sweet and Sour"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Of_Candycity"
  value:
  {
    AuthoredId: "41c635b1-c36b-44e4-2bc7-723741d2ae11"
    Name: "Of Candycity"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Cheesy"
  value:
  {
    AuthoredId: "760b4e23-9e47-7414-c9b5-4d962d6263fc"
    Name: "The Cheesy"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Salted"
  value:
  {
    AuthoredId: "456472f9-2ea7-0634-e9da-72c794c37e20"
    Name: "Salted"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionCheesy"
  value:
  {
    AuthoredId: "fe4de57d-ca84-2414-0b69-5fc9cb802a71"
    Name: "Centurion Cheesy"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Frosted"
  value:
  {
    AuthoredId: "09ab031b-8f58-c6d4-ea3c-7142a4c92520"
    Name: "The Frosted"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiSugary"
  value:
  {
    AuthoredId: "05b07d76-b453-c674-3bbd-0728e6f5bacb"
    Name: "Sugary"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiDrizzled"
  value:
  {
    AuthoredId: "57b65ff1-0f33-cbe4-6b5d-e8f55c377084"
    Name: "Senpai Drizzled"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirButterscotch"
  value:
  {
    AuthoredId: "b78f2c36-ef0a-8664-f86f-880bf81af52c"
    Name: "Sir Butterscotch"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterCandied"
  value:
  {
    AuthoredId: "3bfe577b-a6bf-0f04-285c-8c67823509f1"
    Name: "Grand Master Candied"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiEverlasting"
  value:
  {
    AuthoredId: "3801f648-f223-0934-29fc-791d54123ea4"
    Name: "Senpai Everlasting"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Malted"
  value:
  {
    AuthoredId: "3e54038b-4fbd-c8e4-5b2a-487badc088b9"
    Name: "The Malted"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionMalted"
  value:
  {
    AuthoredId: "179a0d3b-4669-3ea4-cb13-122958961ad1"
    Name: "Centurion Malted"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Striker"
  value:
  {
    AuthoredId: "50ad0c60-0643-faf4-faa3-366a064070aa"
    Name: "Striker"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Grumpy"
  value:
  {
    AuthoredId: "b9b75456-3de9-f624-fb6a-e76f0eadf38b"
    Name: "The Grumpy"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirBashful"
  value:
  {
    AuthoredId: "7c13593f-2ced-d0a4-cbb4-be6e09f47747"
    Name: "Sir Vanilla"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterTricky"
  value:
  {
    AuthoredId: "5b74fed0-acb8-20d4-3bd9-07057c8ec3a4"
    Name: "Grand Master Tricky"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryChocolatey"
  value:
  {
    AuthoredId: "74ed1c66-8d58-26f4-38f3-b52c4d07fb2e"
    Name: "Legionary Chocolatey"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterRootBeer"
  value:
  {
    AuthoredId: "20fa7c1a-c63e-2f44-8a89-faae209cd787"
    Name: "Grand Master Root Beer"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirGlazed"
  value:
  {
    AuthoredId: "09f6aab0-7001-5f24-98c9-8b7f1c66ffab"
    Name: "Sir Glazed"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Minty"
  value:
  {
    AuthoredId: "a3a2d291-2a91-f6f4-f90d-1562e4a94881"
    Name: "Minty"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_CandyCoated"
  value:
  {
    AuthoredId: "daaa9994-7d36-4e44-8b53-f684bc843a56"
    Name: "Candy Coated"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Flakey"
  value:
  {
    AuthoredId: "41c02115-b9db-1b74-dad7-42a66ab8b178"
    Name: "The Flakey"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Quick"
  value:
  {
    AuthoredId: "8f44d49b-0c70-eef4-fb85-2765265117d8"
    Name: "The Quick"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Flying"
  value:
  {
    AuthoredId: "67471cd8-de4b-1254-0ac4-df905ec8c673"
    Name: "Flying"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressLemon"
  value:
  {
    AuthoredId: "59c823eb-9847-f544-5b04-1744275c8f58"
    Name: "Mistress Lemony"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterBlackberry"
  value:
  {
    AuthoredId: "6b32796f-e1db-5c94-6848-96045c67e611"
    Name: "Grand Master Blackberry"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Soup"
  value:
  {
    AuthoredId: "46b81dd2-1530-b8b4-d878-b2682b0b176c"
    Name: "Soup"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Icey"
  value:
  {
    AuthoredId: "9b484b4f-bcb0-7624-a843-c5689c19f1f3"
    Name: "Icey"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Tasty"
  value:
  {
    AuthoredId: "3d762444-0b63-f7f4-29ba-1d793ea1e5c0"
    Name: "The Tasty"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionStrawberry"
  value:
  {
    AuthoredId: "4741b1fa-e1e2-8114-b9c1-9781e9bbde50"
    Name: "Centurion Strawberry"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Strawberry"
  value:
  {
    AuthoredId: "00c25d20-d259-cb54-188a-8fdb046a5eea"
    Name: "Stawberry"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionSalty"
  value:
  {
    AuthoredId: "40231710-a998-5334-a9b3-6f6c827a36b9"
    Name: "Centurion Salty"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryCherry"
  value:
  {
    AuthoredId: "778d3123-5463-22f4-6bd7-363fec02f44d"
    Name: "Legionary Cherry"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryMelon"
  value:
  {
    AuthoredId: "c97a13c7-bf64-3774-08df-e0b65b89bef3"
    Name: "Legionary Melon"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiFried"
  value:
  {
    AuthoredId: "f5476faa-272b-2ce4-1b34-167309712947"
    Name: "Senpai Fried"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiSweetAndSour"
  value:
  {
    AuthoredId: "fcce41fd-33f0-2914-89df-69c252595022"
    Name: "Senpai Sweet and Sour"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirSprinkled"
  value:
  {
    AuthoredId: "0e17ebdd-a6a2-52e4-ebd9-6239cabfae2d"
    Name: "Sir Sprinkled"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionCherryCola"
  value:
  {
    AuthoredId: "66c68e00-befe-0904-aa67-4031aa73c4c5"
    Name: "Centurion Cherry Cola"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_CherryCola"
  value:
  {
    AuthoredId: "3c80b9ee-3dec-57f4-8afa-829052d5e787"
    Name: "Cherry Cola"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterFudgey"
  value:
  {
    AuthoredId: "e21ce82d-cf41-f374-1b10-3fe8ea206c67"
    Name: "Grand Master Fudgey"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Happy"
  value:
  {
    AuthoredId: "9730b930-c2e4-07d4-38a5-c01b8c6032d9"
    Name: "Happy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterOrange"
  value:
  {
    AuthoredId: "5b2082b5-29d5-bcc4-9ab5-5ba8bcc11015"
    Name: "Grand Master Orange"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Dopey"
  value:
  {
    AuthoredId: "ad4561bd-8968-beb4-58c5-b2b7f8fd1c5c"
    Name: "The Dopey"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Slip"
  value:
  {
    AuthoredId: "ba8c5535-3141-f6f4-8943-7ee6e1a5d832"
    Name: "Slip"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterStale"
  value:
  {
    AuthoredId: "53c85640-d495-6d34-2bdd-3e491fff5f85"
    Name: "Grand Master Stale"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Salty"
  value:
  {
    AuthoredId: "6ac49d57-6de9-6004-4a77-321a3237b065"
    Name: "The Salty"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Mango"
  value:
  {
    AuthoredId: "66607cc2-0d5c-9384-d937-464388f40227"
    Name: "Mango"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterMelted"
  value:
  {
    AuthoredId: "ab378d79-958a-0374-1859-598b3c0f7f58"
    Name: "Grand Master Melted"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryDeepFried"
  value:
  {
    AuthoredId: "3fecc053-d569-4ab4-58fa-baee10e1534e"
    Name: "Legionary Deep Fried"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Slip"
  value:
  {
    AuthoredId: "917e27c3-f518-af14-c885-06ba5b5ac1e5"
    Name: "Slip"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Limon"
  value:
  {
    AuthoredId: "f1b233c5-8270-2514-f86b-da84d6f9066c"
    Name: "Limon"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Poker"
  value:
  {
    AuthoredId: "26754c8b-cc30-c394-b98e-beda180a41da"
    Name: "Poker"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryGrumpy"
  value:
  {
    AuthoredId: "cc360e2f-7027-aa44-0bf3-3447266ed2d1"
    Name: "Legionary Grumpy"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressSalted"
  value:
  {
    AuthoredId: "5e3beb8c-cc26-a324-ea15-f4d7c2fa464a"
    Name: "Mistress Salted"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterStrawberry"
  value:
  {
    AuthoredId: "94fd285f-970f-efd4-38ff-8489c15a6504"
    Name: "Grand Master Stawberry"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Striker"
  value:
  {
    AuthoredId: "3d603ae3-9ac4-f8e4-489e-50da3f1d0797"
    Name: "Striker"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirCheesy"
  value:
  {
    AuthoredId: "e2fa95c5-2cc4-1754-d80c-a8cb8d99d736"
    Name: "Sir Cheesy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SingleServing"
  value:
  {
    AuthoredId: "0a37b84c-fb9c-e0f4-1804-cd4197c2a1c3"
    Name: "Single Serving"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressCoconut"
  value:
  {
    AuthoredId: "df063609-bd6d-fc84-8a55-39cbed1b731a"
    Name: "Mistress Coconut"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiMalted"
  value:
  {
    AuthoredId: "f4b7dd1e-9dcb-e6d4-0904-1f95a97f15ea"
    Name: "Senpai Malted"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Drizzled"
  value:
  {
    AuthoredId: "c085e9ea-2379-51c4-3b8f-7cb6305c2adc"
    Name: "Drizzled"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterFlying"
  value:
  {
    AuthoredId: "69683e1c-e353-3914-5a84-de9f77b3f5a9"
    Name: "Grand Master Flying"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Bite_Sized"
  value:
  {
    AuthoredId: "3464c1c1-1ef4-22c4-580f-f5abc1b525ba"
    Name: "The Bite Sized"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirMalted"
  value:
  {
    AuthoredId: "79e82723-a78b-2be4-580e-3f281ae996aa"
    Name: "Sir Malted"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterCherryCola"
  value:
  {
    AuthoredId: "31690c60-ca43-1864-ebdd-e0bd38e124cf"
    Name: "Grand Master Cherry Cola"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Melty"
  value:
  {
    AuthoredId: "5a1a5c98-74b0-7584-7be9-e329212a18c5"
    Name: "The Melty"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryNutty"
  value:
  {
    AuthoredId: "29e825b3-89a1-1da4-0a36-5d8d0047891b"
    Name: "Legionary Nutty"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterBaked"
  value:
  {
    AuthoredId: "61eef531-9845-3144-ca64-60adfa71e020"
    Name: "Grand Master Baked"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Quick"
  value:
  {
    AuthoredId: "7582bdcb-6d44-ae94-99a7-59683db50ef5"
    Name: "Quick"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirAngry"
  value:
  {
    AuthoredId: "c0c14231-a864-72c4-2a05-8755a9a9fba9"
    Name: "Sir Angry"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiGrumpy"
  value:
  {
    AuthoredId: "f379f851-c02e-5cd4-a818-87a14339828b"
    Name: "Senpai Grumpy"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionBitter"
  value:
  {
    AuthoredId: "169f53dd-ce2e-9504-58e6-00cf724f9480"
    Name: "Centurion Bitter"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Fruity"
  value:
  {
    AuthoredId: "b9a06098-aea1-ff94-7ac4-173cf2e2a3b7"
    Name: "The Fruity"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Chocolatey"
  value:
  {
    AuthoredId: "4e555ec3-dbbe-8704-88c6-5c65ece275ba"
    Name: "The Chocolatey"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionButtered"
  value:
  {
    AuthoredId: "455768af-6e8b-2bb4-0829-64584d577468"
    Name: "Centurion Buttered"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Cover-up"
  value:
  {
    AuthoredId: "b85e0d45-ee5f-0404-6878-09d8f97ee308"
    Name: "Cover-up"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_OverhandRight"
  value:
  {
    AuthoredId: "f1bd3119-1d15-1304-3a6c-86d6c930999d"
    Name: "Overhand Right"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Fudgy"
  value:
  {
    AuthoredId: "9dfc3592-594a-5490-ab36-6f9b108954f9"
    Name: "Fudgy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiRaspberry"
  value:
  {
    AuthoredId: "1fefef33-2619-7bf4-1871-aa71cbe63788"
    Name: "Senpai Rasperry"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryBiteSized"
  value:
  {
    AuthoredId: "d8ad490d-6b9f-9a74-8b83-9cbab7e915fb"
    Name: "Legionary Bite Sized"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Of_Candycity"
  value:
  {
    AuthoredId: "ed90040f-bf24-1ea4-d930-f385b0773605"
    Name: "Of Candycity"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Plump"
  value:
  {
    AuthoredId: "9e85c7bf-234d-c164-9a4a-f169ddddd6c7"
    Name: "Plump"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Minty"
  value:
  {
    AuthoredId: "a1bd2dfe-6cb4-eed4-4bfc-16be53f1ac9e"
    Name: "The Minty"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_TravelSized"
  value:
  {
    AuthoredId: "2b96c872-5327-a124-8a07-e61dc32faf21"
    Name: "Travel Sized"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Squishy"
  value:
  {
    AuthoredId: "aded67c3-6d00-00f4-d83f-939c542fcdf2"
    Name: "Squishy"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressBoiled"
  value:
  {
    AuthoredId: "5499e932-0621-e854-0a0c-55b28543ed48"
    Name: "Mistress Boiled"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionarySneezy"
  value:
  {
    AuthoredId: "13056870-8174-5da4-6b36-afcbf9b12c0c"
    Name: "Legionary Sneezy"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Runner"
  value:
  {
    AuthoredId: "1f8dec75-0a40-5de4-5b06-11478bec98b3"
    Name: "Runner"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Slip"
  value:
  {
    AuthoredId: "6ec5b623-203c-fbe4-79e2-381046736983"
    Name: "Slip"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_Xaya"
  value:
  {
    AuthoredId: "19047438-1f52-1824-b848-c060840e67bf"
    Name: "Xaya"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Toffee"
  value:
  {
    AuthoredId: "3f874e81-8e8d-0074-aa03-3f6e29501a89"
    Name: "Toffee"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiStale"
  value:
  {
    AuthoredId: "989a5e2c-21c1-23f4-a9b4-a87d9a52f99d"
    Name: "Senpai Stale"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Crunchy"
  value:
  {
    AuthoredId: "55c5407a-2725-df64-dbb6-afa8e4b7a56e"
    Name: "Crunchy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterSalted"
  value:
  {
    AuthoredId: "31f76754-9689-5564-6bad-a05999ed638e"
    Name: "Grand Master Salted"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Poker"
  value:
  {
    AuthoredId: "13708387-2433-6474-9a3e-ac82b85ba41b"
    Name: "Poker"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Buttered"
  value:
  {
    AuthoredId: "4f2d7402-c08d-2e14-3952-7333be0a77a9"
    Name: "The Buttered"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryMelty"
  value:
  {
    AuthoredId: "39a129c0-875e-27d4-0bc6-1e094e971b3e"
    Name: "Legionary Melty"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Candied"
  value:
  {
    AuthoredId: "d6cd744f-b479-b824-9be2-1806904f3611"
    Name: "The Candied"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterCoconut"
  value:
  {
    AuthoredId: "2df30bd9-38dd-92c4-3a0f-e53d58638be7"
    Name: "Grand Master Coconut"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Minty"
  value:
  {
    AuthoredId: "b444519d-0681-f6f4-8ac7-27b18aff4577"
    Name: "The Minty"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Squishy"
  value:
  {
    AuthoredId: "ca27ca6d-5fc2-49a4-5beb-71822ee5f918"
    Name: "Squishy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_OfCakerton"
  value:
  {
    AuthoredId: "8d9eb4f9-15b1-3d54-f8f5-3a284d34a4cb"
    Name: "Of Cakerton"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionSweetAndSour"
  value:
  {
    AuthoredId: "2b799869-2168-b5d4-b9a1-4db949c635e6"
    Name: "Centurion Sweet and Sour"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirFresh"
  value:
  {
    AuthoredId: "2a68f03b-2650-4ba4-c92f-8cd74a9bf8a7"
    Name: "Sir Fresh"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiSneezy"
  value:
  {
    AuthoredId: "0a79e6aa-b7d6-6ff4-68f5-015521476769"
    Name: "Senpai Sneezy"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiLemon"
  value:
  {
    AuthoredId: "2323d6f2-6626-3294-4868-cbc03defe75e"
    Name: "Senpai Lemony"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirMoldy"
  value:
  {
    AuthoredId: "7edaf272-1d05-d374-8b96-f121bc47327a"
    Name: "Sir Moldy"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Happy"
  value:
  {
    AuthoredId: "bb13dd21-c58b-97c4-bb3a-30d5cc1eeaef"
    Name: "The Happy"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_BoloPunch"
  value:
  {
    AuthoredId: "6241762f-89fc-3964-6b72-c78847f8656c"
    Name: "Bolo Punch"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Switch_Hitter"
  value:
  {
    AuthoredId: "11ede1d2-0ded-0884-4b07-80c67961cd32"
    Name: "Switch-Hitter"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Glazed"
  value:
  {
    AuthoredId: "462c0286-52ac-8274-9bdd-561f8c77ec34"
    Name: "Glazed"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Of_Puddingham"
  value:
  {
    AuthoredId: "5b6351a1-5c42-2f94-cadd-49e2dfcc146f"
    Name: "Of Puddingham"
    Position: 1
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Everlasting"
  value:
  {
    AuthoredId: "6307f519-c15b-dd84-893a-8b369538000b"
    Name: "The Everlasting"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Flakey"
  value:
  {
    AuthoredId: "7645b448-4183-97a4-ba45-cb0bb2090ec4"
    Name: "Flakey"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Rolled"
  value:
  {
    AuthoredId: "404f4990-7dbb-5344-59aa-46787af008bb"
    Name: "Rolled"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionIcey"
  value:
  {
    AuthoredId: "ed5cac8d-3d91-1884-d9cc-711dd5461124"
    Name: "Centurion Icey"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Crusty"
  value:
  {
    AuthoredId: "a266d594-912e-4044-4967-b2d9d8428f4e"
    Name: "The Crusty"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterGreenApple"
  value:
  {
    AuthoredId: "4cbaf19e-7385-f364-4b61-4d1b604fe8e0"
    Name: "Grand Master Green Apple"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressSingleServing"
  value:
  {
    AuthoredId: "a870f140-059e-a544-39c8-7881cafb4712"
    Name: "Mistress Single Serving"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionChocolatey"
  value:
  {
    AuthoredId: "f7ec0fe1-60d5-8a54-fa45-0d560f2c31c5"
    Name: "Centurion Chocolatey"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Chocolatey"
  value:
  {
    AuthoredId: "7b2f4680-c3f7-cbd4-680d-34ac97a70825"
    Name: "Chocolatey"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirBlackberry"
  value:
  {
    AuthoredId: "0d12c3b0-3278-f8c4-8bbc-17981fa48fa7"
    Name: "Sir Blackberry"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiBaked"
  value:
  {
    AuthoredId: "734d92ae-cee3-0054-1828-34d15fe6bc7d"
    Name: "Senpai Baked"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirCrazy"
  value:
  {
    AuthoredId: "bfe1a848-097d-0d94-38ef-71786154a4d9"
    Name: "Sir Crazy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterDopey"
  value:
  {
    AuthoredId: "09c2092a-dacf-1dc4-a980-dfc2576b1474"
    Name: "Grand Master Dopey"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Bitter"
  value:
  {
    AuthoredId: "de41714a-87f3-cd24-39e7-67ba4cd79b64"
    Name: "The Bitter"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressMinty"
  value:
  {
    AuthoredId: "d4d0e319-444e-a5e4-3955-7c4953bd871f"
    Name: "Mistress Minty"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterSalty"
  value:
  {
    AuthoredId: "9482e6e3-6d98-ebe4-5be3-0feecdcdaa54"
    Name: "Grand Master Salty"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiIcey"
  value:
  {
    AuthoredId: "f7916dca-2f8f-9064-7a77-488da92ccb72"
    Name: "Senpai Icey"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressRaspberry"
  value:
  {
    AuthoredId: "0e654ad2-2397-de74-0bdc-db5b3cfbf20d"
    Name: "Mistress Rasperry"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Butterscotch"
  value:
  {
    AuthoredId: "aa8cf8f0-5a5d-90e4-18a1-ed9f0d255d2e"
    Name: "Butterscotch"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Malted"
  value:
  {
    AuthoredId: "5429290f-9ff3-17f4-793f-d82e6dddfb92"
    Name: "Malted"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiBlackberry"
  value:
  {
    AuthoredId: "6e8eae99-6fd8-8ba4-5be7-976762f91794"
    Name: "Senpai Blackberry"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryBadass"
  value:
  {
    AuthoredId: "0436b15b-fd84-14b4-dbc6-4d7214377cda"
    Name: "Legionary  Badass"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Speedy"
  value:
  {
    AuthoredId: "3bebcc05-a901-61f4-e876-685f748bddbd"
    Name: "Speedy"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Deep_Fried"
  value:
  {
    AuthoredId: "00c5f526-b687-42b4-9b82-8631d4c77cd0"
    Name: "The Deep Fried"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Angry"
  value:
  {
    AuthoredId: "9497b58e-fe66-d477-1b54-40b55383f6d2"
    Name: "Angry"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Quick"
  value:
  {
    AuthoredId: "5a9fe36c-4b42-f964-48ed-16011c21da3a"
    Name: "The Quick"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionBlackberry"
  value:
  {
    AuthoredId: "78da8f48-af25-be24-4a40-5adaddbadfb8"
    Name: "Centurion Blackberry"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressMoldy"
  value:
  {
    AuthoredId: "fb7c6ccc-7750-7fb4-684e-559a0f3d661f"
    Name: "Mistress Moldy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirHotButtered"
  value:
  {
    AuthoredId: "380a6daf-e82c-4794-cabb-c290325efb93"
    Name: "Sir Hot Buttered"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Caramelized"
  value:
  {
    AuthoredId: "b9736786-57d2-4164-2ac3-13d2625edf14"
    Name: "Caramelized"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Bashful"
  value:
  {
    AuthoredId: "4a009654-8a2a-e324-4a1e-90c7defffbf2"
    Name: "Vanilla"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Rocky"
  value:
  {
    AuthoredId: "66b64d77-fd6f-a48a-5932-935ca54c0b51"
    Name: "Rocky"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Slapper"
  value:
  {
    AuthoredId: "afe7b80b-2506-ed24-f93f-97d54b631688"
    Name: "Slapper"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Sour"
  value:
  {
    AuthoredId: "7eea43a1-79d2-fad4-389e-86dc26ad1669"
    Name: "Sour"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Bitter"
  value:
  {
    AuthoredId: "f5862cbe-6d14-32e4-3a96-05affb5e106c"
    Name: "The Bitter"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressHappy"
  value:
  {
    AuthoredId: "8451b2b5-e32d-6aa4-08bc-55b72b8f57bd"
    Name: "Mistress Happy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressDrizzled"
  value:
  {
    AuthoredId: "adc7b9b7-9b65-9894-bb98-f456ba011cc1"
    Name: "Mistress Drizzled"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Bashful"
  value:
  {
    AuthoredId: "189c44bf-bf98-8144-ba81-e8d3e7dd160e"
    Name: "The Bashful"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryPrechewed"
  value:
  {
    AuthoredId: "470fab04-cb65-88d4-2bda-50a8c5594a38"
    Name: "Legionary Prechewed"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirSpicy"
  value:
  {
    AuthoredId: "6b765551-c822-e3b4-7b1a-464b0ed8c862"
    Name: "Sir Spicy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterBlueberry"
  value:
  {
    AuthoredId: "6bca1765-b608-5764-3be0-23837581fd9d"
    Name: "Grand Master Blueberry"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionarySingleServing"
  value:
  {
    AuthoredId: "3bab7a60-797c-ec54-f863-222e7fd7657b"
    Name: "Senpai Single Serving"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionFried"
  value:
  {
    AuthoredId: "6f37ba8d-faf9-2524-e974-9106b7238e4f"
    Name: "Centurion Fried"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressMango"
  value:
  {
    AuthoredId: "3b5a23d3-8a66-10e4-0bed-cbd7ae7b68cb"
    Name: "Mistress Mango"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirIcey"
  value:
  {
    AuthoredId: "59f954eb-90fb-1b44-8a3b-d105766fe79a"
    Name: "Sir Icey"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryGrouchy"
  value:
  {
    AuthoredId: "60882bb1-8a47-d564-98c6-703f5ea03177"
    Name: "Legionary Grouchy"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Pie"
  value:
  {
    AuthoredId: "f3d2d4db-8273-2db4-49d3-22459c790832"
    Name: "Pie"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressLimon"
  value:
  {
    AuthoredId: "a1c6a2e3-8759-7594-a981-5537abbb8a08"
    Name: "Mistress Limon"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionarySalty"
  value:
  {
    AuthoredId: "b373356f-b696-67a4-b889-1e626281035c"
    Name: "Legionary Salty"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Flakey"
  value:
  {
    AuthoredId: "e8b9398b-5e3f-c834-6ba3-63d7e80e0f19"
    Name: "Flakey"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiBoiled"
  value:
  {
    AuthoredId: "894eee1a-e115-15d4-286f-f34bc0b238cc"
    Name: "Senpai Boiled"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryRootBeer"
  value:
  {
    AuthoredId: "c979ea38-2e5f-5074-382a-1bb051a3b7e4"
    Name: "Legionary Root Beer"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Salted"
  value:
  {
    AuthoredId: "1fa89c70-7ca5-3f34-d9d2-20c02109d418"
    Name: "The Salted"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Cake"
  value:
  {
    AuthoredId: "0a605b7f-0d1c-6d14-68ba-3ac6152e7b3b"
    Name: "Cake"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Sugary"
  value:
  {
    AuthoredId: "d6117426-3d2f-a434-6a70-d050c0d31796"
    Name: "Sugary"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressFruity"
  value:
  {
    AuthoredId: "c538b368-32c1-0124-697c-efc09abaedd5"
    Name: "Mistress Fruity"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Creamy"
  value:
  {
    AuthoredId: "9f4b3dfd-9ecb-b604-ba2e-8b7f98aa5cb2"
    Name: "The Creamy"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionCreamy"
  value:
  {
    AuthoredId: "a310d781-cff8-2a44-cb25-f7ebe3d1805e"
    Name: "Centurion Creamy"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiDopey"
  value:
  {
    AuthoredId: "ff07762d-7694-5184-4824-3a2b41895226"
    Name: "Senpai Dopey"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionPrechewed"
  value:
  {
    AuthoredId: "2703bf57-8112-bf44-7a97-41a8d307ced5"
    Name: "Centurion Prechewed"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Powdered"
  value:
  {
    AuthoredId: "83352d6b-bdc5-b3b4-b957-59d2632cf801"
    Name: "The Powdered"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirCrusty"
  value:
  {
    AuthoredId: "e01597e6-00b8-b6a4-3a3e-0befe707e3bf"
    Name: "Sir Crusty"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionSingleServing"
  value:
  {
    AuthoredId: "9e14733e-7923-6714-5812-b6ee4545f98d"
    Name: "Centurion Single Serving"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Of_Gummiway"
  value:
  {
    AuthoredId: "4d462c84-bd7d-a984-b951-fb6a73c4cefd"
    Name: "Of Gummiway"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiSalty"
  value:
  {
    AuthoredId: "a13d9ad9-b283-1d44-3b3b-b999bac35d5e"
    Name: "Senpai Salty"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiChocolatey"
  value:
  {
    AuthoredId: "b7cd9583-4306-77f4-6a8e-8f8dca83fdac"
    Name: "Senpai Chocolatey"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryButtered"
  value:
  {
    AuthoredId: "06d4a824-5f11-68e4-1bcb-a6716bc3a678"
    Name: "Legionary Buttered"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Fizzy"
  value:
  {
    AuthoredId: "e255a82f-bb9e-b418-f86e-4919360a95b5"
    Name: "Fizzy"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressQuick"
  value:
  {
    AuthoredId: "6fde9fa2-41ef-77d4-0a72-349c72fae8b7"
    Name: "Mistress Quick"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterCaramel"
  value:
  {
    AuthoredId: "63c6e658-ef2e-f684-49e5-de59fcd59508"
    Name: "Grand Master Caramel"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionarySprinkled"
  value:
  {
    AuthoredId: "0c09a546-1cc5-eeb4-3884-50506538e226"
    Name: "Legionary Sprinkled"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Chocolatey"
  value:
  {
    AuthoredId: "e9b0276f-65a7-e874-7a3d-2be22ca00288"
    Name: "The Chocolatey"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Deep_Fried"
  value:
  {
    AuthoredId: "cb210cd7-c255-a814-0aae-930eb13cb1cb"
    Name: "The Deep Fried"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Plump"
  value:
  {
    AuthoredId: "88c4240d-31b4-2f34-b8f8-b6decc1ceeb4"
    Name: "The Plump"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Runner"
  value:
  {
    AuthoredId: "3f54189c-805c-d554-d8ec-2ebbf10463a2"
    Name: "Runner"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Coco"
  value:
  {
    AuthoredId: "5de014c5-def5-1944-99e8-1c0c2089ae3d"
    Name: "Coco"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionQuick"
  value:
  {
    AuthoredId: "6a307ebb-c1d5-45d4-08fa-238203ad9705"
    Name: "Centurion Quick"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterCaramelized"
  value:
  {
    AuthoredId: "5f375e8f-9984-1744-8aa0-8d1808337f90"
    Name: "Grand Master Caramelized"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionMinty"
  value:
  {
    AuthoredId: "a6310264-fe6e-90b4-991e-a5790f3f4675"
    Name: "Centurion Minty"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Grouchy"
  value:
  {
    AuthoredId: "bdc9dead-21de-3614-780c-f35d693142b8"
    Name: "The Grouchy"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_CandyCoated"
  value:
  {
    AuthoredId: "64017d6d-d525-d994-0b0e-56f2228bde33"
    Name: "Candy Coated"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Deep_Fried"
  value:
  {
    AuthoredId: "88e4f91e-e4ab-2874-dbc0-7440fda9a99a"
    Name: "The Deep Fried"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionarySour"
  value:
  {
    AuthoredId: "975d75a0-e850-2794-ca59-d620819cd6bf"
    Name: "Legionary Sour"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirChewy"
  value:
  {
    AuthoredId: "32ff01dc-7c72-0394-487a-7249e1a29a5a"
    Name: "Sir Chewy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Of_Popland"
  value:
  {
    AuthoredId: "913df874-833e-df94-2adc-add1805dc8fc"
    Name: "Of Popland"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryFresh"
  value:
  {
    AuthoredId: "ab1940fd-673c-42e4-eaef-0d43aa288197"
    Name: "Legionary Fresh"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Single_Serving"
  value:
  {
    AuthoredId: "68772196-3b27-c774-ebad-978c55bd1264"
    Name: "The Single Serving"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryMoldy"
  value:
  {
    AuthoredId: "b8e6f3d5-6643-8fc4-494e-a8d4d9cc0ede"
    Name: "Legionary Moldy"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Of_Fudgeland"
  value:
  {
    AuthoredId: "accf6d11-e301-ad04-7abf-c8f02826cce0"
    Name: "Of Fudgeland"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterFlakey"
  value:
  {
    AuthoredId: "a1c5b412-6992-2e14-2bd8-be12933a2434"
    Name: "Grand Master Flakey"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Minty"
  value:
  {
    AuthoredId: "f136730a-1a62-c184-4b3d-26ce99aed3d2"
    Name: "The Minty"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryChopped"
  value:
  {
    AuthoredId: "02956951-bb2a-aa24-5854-7de0686817cb"
    Name: "Legionary Chopped"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Cherry"
  value:
  {
    AuthoredId: "94bc2e32-37ef-eb04-7ab9-38d9ebedaf0b"
    Name: "Cherry"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressSnozzberry"
  value:
  {
    AuthoredId: "4ecbbee6-6323-f734-9932-83ddf3ca34e2"
    Name: "Mistress Snozzberry"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryLifeSized"
  value:
  {
    AuthoredId: "b85f44fc-2380-8b14-8ae9-87a71c87785e"
    Name: "Legionary Life Sized"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressBiteSized"
  value:
  {
    AuthoredId: "ae353323-0108-57d4-dbba-f7b19ebfb6f8"
    Name: "Mistress Bite Sized"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Caramel"
  value:
  {
    AuthoredId: "3b892611-f180-6494-fab3-3c9ba3fa3c92"
    Name: "Caramel"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiCheesy"
  value:
  {
    AuthoredId: "86306d8f-78ff-9594-2aa8-f9635c0b5b51"
    Name: "Senpai Cheesy"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirCandied"
  value:
  {
    AuthoredId: "398333a6-ce2b-b0f4-ea05-a427ec64fb70"
    Name: "Sir Candied"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionHappy"
  value:
  {
    AuthoredId: "c7a85d66-f07f-bbc4-d9c7-e63a35086326"
    Name: "Centurion Happy"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Of_Sourway"
  value:
  {
    AuthoredId: "5a23f080-1486-1174-6b5e-e72ccc9664bc"
    Name: "Of Sourway"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionGlazed"
  value:
  {
    AuthoredId: "df2a8b54-c399-9c44-5a10-7f5ae6429a63"
    Name: "Centurion Glazed"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Peanut"
  value:
  {
    AuthoredId: "0ce77cbf-11d5-441e-6a92-7ed9cbf6220b"
    Name: "Peanut"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryCrazy"
  value:
  {
    AuthoredId: "0f5d8d8a-8ecf-86e4-0ab0-a59b6163b167"
    Name: "Legionary Crazy"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressButterscotch"
  value:
  {
    AuthoredId: "44f8bc85-737c-7aa4-7a3b-c318c5f18019"
    Name: "Mistress Butterscotch"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiMoldy"
  value:
  {
    AuthoredId: "e15d5c97-a503-7794-793c-9b00edf6a4d5"
    Name: "Senpai Moldy"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Melon"
  value:
  {
    AuthoredId: "119b045a-3457-3744-db3f-01e6dd68d9c4"
    Name: "Melon"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionMango"
  value:
  {
    AuthoredId: "48d6536a-8fd2-1894-7989-0c69e8f9681b"
    Name: "Centurion Mango"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Slapper"
  value:
  {
    AuthoredId: "58ada2fc-cfa0-bc34-f827-c76653a46b6e"
    Name: "Slapper"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiGreenApple"
  value:
  {
    AuthoredId: "ec9cdddc-0bc2-4bc4-0978-c4511a022388"
    Name: "Senpai Green Apple"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_MistressGrumpy"
  value:
  {
    AuthoredId: "64f87637-15f7-b654-a891-848a1ae2fe7a"
    Name: "Mistress Grumpy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_Of_Sugarville"
  value:
  {
    AuthoredId: "90a52f6c-e799-d7c4-ca49-77e709ca0094"
    Name: "Of Sugarville"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirCreamy"
  value:
  {
    AuthoredId: "7d3cee78-bae3-14c4-3b5d-838c08c93c81"
    Name: "Sir Creamy"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_GrandMasterSpeedy"
  value:
  {
    AuthoredId: "4d5a702b-5ab6-f4a4-f82a-3d484df59fb6"
    Name: "Grand Master Speedy"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionChopped"
  value:
  {
    AuthoredId: "dd4c4ac7-1fc5-e034-ea92-c8d60fc547f2"
    Name: "Centurion Chopped"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Boiled"
  value:
  {
    AuthoredId: "ae0d1cf1-60d1-af84-ebe0-af56bf8b28b8"
    Name: "The Boiled"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_Last_Soup"
  value:
  {
    AuthoredId: "a410ec8a-adf6-644b-bb9d-ed284285a5e9"
    Name: "Soup"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Candy_Coated"
  value:
  {
    AuthoredId: "0c6028c3-6e2f-5934-6be2-92fccdc36ff1"
    Name: "The Candy Coated"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Happy"
  value:
  {
    AuthoredId: "e86d910c-4451-a354-f940-7b4dfc6ddb9e"
    Name: "The Happy"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_BoloPunch"
  value:
  {
    AuthoredId: "20b17074-1bae-feb4-08e6-403d7a7661b9"
    Name: "Bolo Punch"
    Position: 1
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirButtered"
  value:
  {
    AuthoredId: "798a8f75-b667-d2c4-d9fe-bdf1b88b741c"
    Name: "Sir Buttered"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_SirChocolatey"
  value:
  {
    AuthoredId: "599f4c72-a29c-d774-bbaa-d821a4a6feb2"
    Name: "Sir Chocolatey"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Pop"
  value:
  {
    AuthoredId: "ca7a0077-7043-5034-5acd-594ab5589cf7"
    Name: "Pop"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Grumpy"
  value:
  {
    AuthoredId: "4ce33a69-21b5-ffd4-e86a-27454b7c73e8"
    Name: "Grumpy"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_MasterBaker"
  value:
  {
    AuthoredId: "8fd76f7c-88d3-7d24-0bf3-59e97b8f526e"
    Name: "Master Baker"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Soup"
  value:
  {
    AuthoredId: "a90bf804-19ef-2a64-58a1-f897280981ed"
    Name: "Soup"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Plump"
  value:
  {
    AuthoredId: "8021f1b8-4d99-0a74-6a77-aa1b7be90f8c"
    Name: "Plump"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Melted"
  value:
  {
    AuthoredId: "0abcbef5-b159-ee84-681e-9200c6dc5241"
    Name: "The Melted"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiCrunchy"
  value:
  {
    AuthoredId: "d57129a4-168f-6034-8b30-0e18bd941202"
    Name: "Senpai Crunchy"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Carameized"
  value:
  {
    AuthoredId: "280bfa3c-61f1-5ff4-5b18-4c0b51ddef32"
    Name: "The Carameized"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_Last_The_Chopped"
  value:
  {
    AuthoredId: "478e2d6e-4dce-c694-f8db-65274042ff73"
    Name: "The Chopped"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryQuick"
  value:
  {
    AuthoredId: "ab3d89a4-39b6-0ca4-2add-371d9d13e87a"
    Name: "Legionary Quick"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Runner"
  value:
  {
    AuthoredId: "4ad3c053-4679-bb04-fa0b-7fd8641e543d"
    Name: "Runner"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_BoxerPuncher"
  value:
  {
    AuthoredId: "f586b1e0-c1e8-5e74-3b0f-4e68b9d3632e"
    Name: "Boxer-Puncher"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiSquishy"
  value:
  {
    AuthoredId: "cf74a319-bd8a-0394-88e5-63b013113c18"
    Name: "Senpai Squishy"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionaryCandyCoated"
  value:
  {
    AuthoredId: "f8a68aae-8ecb-c0f4-a92f-f59ac5e28feb"
    Name: "Legionary Candy Coated"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionCrusty"
  value:
  {
    AuthoredId: "d1fc61b5-b43a-4b44-59e6-fb7756f6d7d7"
    Name: "Centurion Crusty"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_LegionarySpicy"
  value:
  {
    AuthoredId: "16352d20-761b-e734-a921-0d61c32fc912"
    Name: "Legionary Spicy"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_Block"
  value:
  {
    AuthoredId: "ea15e579-2cab-c0b4-ba31-37c2021d79f3"
    Name: "Block"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Flakey"
  value:
  {
    AuthoredId: "6dd2da17-9ad2-46d4-4a2f-241eebe56e2f"
    Name: "The Flakey"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Glazed"
  value:
  {
    AuthoredId: "1505c53b-0b4e-7f34-49c4-a3a51faaf035"
    Name: "The Glazed"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Nutty"
  value:
  {
    AuthoredId: "03790d5a-8824-643d-2b57-212ea737b6ec"
    Name: "Nutty"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_Slugger"
  value:
  {
    AuthoredId: "e60a25ac-6c8c-e8a4-c956-4aad56599a73"
    Name: "Slugger"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_OverhandRight"
  value:
  {
    AuthoredId: "6b752b26-3dd9-19b4-fa9c-ddfc8ca93f59"
    Name: "Overhand Right"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Common_First_Crazy"
  value:
  {
    AuthoredId: "5dbe53c5-408b-d49b-e89e-af1a20f92185"
    Name: "Crazy"
    Position: 0
    Quality: 1
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_First_Cherry"
  value:
  {
    AuthoredId: "220f07b1-3ec4-52e4-2ab7-4e7f0305358b"
    Name: "Cherry"
    Position: 0
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_First_CenturionOrange"
  value:
  {
    AuthoredId: "68ff1a5b-1c38-26f4-38ce-5f96b3b4f23c"
    Name: "Centurion Orange"
    Position: 0
    Quality: 4
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Epic_Last_The_Quick"
  value:
  {
    AuthoredId: "36e36d20-1f8d-b534-78c0-c9d3349dd8a7"
    Name: "The Quick"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Rare_First_SenpaiSprinkled"
  value:
  {
    AuthoredId: "f009bcd7-4d3f-4824-483e-22544c722aa9"
    Name: "Senpai Sprinkled"
    Position: 0
    Quality: 3
    Probability: 1
  }
}
fighternames:
{
  key: "FighterName_Uncommon_Last_The_Very_Tasty"
  value:
  {
    AuthoredId: "d3f98ead-69e5-4f64-ab89-4514825c9d63"
    Name: "The Very Tasty"
    Position: 1
    Quality: 2
    Probability: 1
  }
}
fightertype:
{
  key: "RockCandy"
  value:
  {
    AuthoredId: "2522657f-c73f-23c4-da7d-3934d135265b"
    Name: "Rock Candy"
    Probability: 20
    MoveProbabilities: {MoveType: 4 Probability: 30.0 AuthoredId: "22075a8a-a0c6-4400-a962-37a18954596f"}
    MoveProbabilities: {MoveType: 3 Probability: 20.0 AuthoredId: "5a465b87-c2f5-4430-3b15-66fdb4627c4a"}
    MoveProbabilities: {MoveType: 0 Probability: 20.0 AuthoredId: "18d0f469-cb7b-d4fc-da6b-dad08eb6f6c4"}
    MoveProbabilities: {MoveType: 1 Probability: 15.0 AuthoredId: "259e3cfe-fb58-4438-9a99-5aca77db2bad"}
    MoveProbabilities: {MoveType: 2 Probability: 15.0 AuthoredId: "163c6577-97a3-9490-ab17-4ad07a8be44f"}
  }
}
fightertype:
{
  key: "Chocolate"
  value:
  {
    AuthoredId: "a672e1da-a87d-2a54-4918-cafb72b02aec"
    Name: "Chocolate"
    Probability: 20
    MoveProbabilities: {MoveType: 4 Probability: 15.0 AuthoredId: "0f2afa35-971e-f42c-bafa-dbd651bc527b"}
    MoveProbabilities: {MoveType: 3 Probability: 15.0 AuthoredId: "6ce34d0c-70b1-f47c-d8cc-a81b7dccafa7"}
    MoveProbabilities: {MoveType: 0 Probability: 30.0 AuthoredId: "26292174-40ee-44c2-3992-a5533add6e42"}
    MoveProbabilities: {MoveType: 1 Probability: 20.0 AuthoredId: "9d4d5415-f757-648d-1823-881eec8a05ff"}
    MoveProbabilities: {MoveType: 2 Probability: 20.0 AuthoredId: "e5b829cf-12ab-34a5-9abd-d3038ea3d05d"}
  }
}
fightertype:
{
  key: "Gummi"
  value:
  {
    AuthoredId: "c160f7ad-c775-8614-abe2-8ef74e54401f"
    Name: "Gummi"
    Probability: 20
    MoveProbabilities: {MoveType: 4 Probability: 20.0 AuthoredId: "3413e590-74e1-f417-8b38-0f2c366ff092"}
    MoveProbabilities: {MoveType: 3 Probability: 20.0 AuthoredId: "95f4c3f4-a488-64a9-9a9e-27c77b86b5d1"}
    MoveProbabilities: {MoveType: 0 Probability: 15.0 AuthoredId: "7fa054ce-bcde-44fb-6b9a-9196fdad00a0"}
    MoveProbabilities: {MoveType: 1 Probability: 15.0 AuthoredId: "f496e3b5-9ba5-0463-4afe-304db5bb5538"}
    MoveProbabilities: {MoveType: 2 Probability: 30.0 AuthoredId: "249ad092-4951-a4a4-0bac-0b969fbe578a"}
  }
}
fightertype:
{
  key: "Caramel"
  value:
  {
    AuthoredId: "85f361b8-e55d-0244-1a98-26bffa0a18a2"
    Name: "Caramel"
    Probability: 20
    MoveProbabilities: {MoveType: 4 Probability: 15.0 AuthoredId: "6f4cf662-f90a-84c4-d9d7-45196088498d"}
    MoveProbabilities: {MoveType: 3 Probability: 30.0 AuthoredId: "280c135c-0944-f447-48d5-0c68b459fa89"}
    MoveProbabilities: {MoveType: 0 Probability: 20.0 AuthoredId: "81563e3d-2b53-9420-cb84-97372b0117d4"}
    MoveProbabilities: {MoveType: 1 Probability: 20.0 AuthoredId: "b6803b8f-83eb-f432-6962-3e80f3f4f0f5"}
    MoveProbabilities: {MoveType: 2 Probability: 15.0 AuthoredId: "a8f02adc-2b26-84f2-fb64-c1b1c61a37ee"}
  }
}
fightertype:
{
  key: "WhiteChocolate"
  value:
  {
    AuthoredId: "4c2d03be-70bf-9294-c8d5-0a8e2d8d0677"
    Name: "White Chocolate"
    Probability: 20
    MoveProbabilities: {MoveType: 4 Probability: 20.0 AuthoredId: "9d231f48-67ce-5438-4995-a66f61ce7ced"}
    MoveProbabilities: {MoveType: 3 Probability: 15.0 AuthoredId: "f0a6d240-79cf-b462-ca62-fdfcc9b07003"}
    MoveProbabilities: {MoveType: 0 Probability: 15.0 AuthoredId: "fd4a78a6-7676-5400-48d2-8b26af64bbb7"}
    MoveProbabilities: {MoveType: 1 Probability: 30.0 AuthoredId: "faf6cf4d-6926-c43f-c9e9-a0c73746b98e"}
    MoveProbabilities: {MoveType: 2 Probability: 20.0 AuthoredId: "c18f297a-0076-54e0-eaa7-71ffe38a1278"}
  }
}
goodies:
{
  key: "Goodie_PressureCooker_1"
  value:
  {
    AuthoredId: "ca3378db-cd54-e514-7ae8-23705781bb9d"
    Name: "Rusty Pressure Cooker"
    Description: "Speed up your next 3 Cooks!"
    Price: 100
    GoodyType: 1
    Uses: 3
    ReductionPercent: 0.8
  }
}
goodies:
{
  key: "Goodie_PressureCooker_2"
  value:
  {
    AuthoredId: "de8529bb-e9e4-f764-3b93-64032254f105"
    Name: "Slightly Used Pressure Coker"
    Description: "Speed up your next 6 Cooks!"
    Price: 175
    GoodyType: 1
    Uses: 6
    ReductionPercent: 0.65
  }
}
goodies:
{
  key: "Goodie_Espresso_1"
  value:
  {
    AuthoredId: "339fa37e-d9be-0944-d990-9daff7017d91"
    Name: "Single Shot Espresso"
    Description: "Speed up your next 3 Expeditions!"
    Price: 100
    GoodyType: 2
    Uses: 3
    ReductionPercent: 0.8
  }
}
goodies:
{
  key: "Goodie_Espresso_2"
  value:
  {
    AuthoredId: "086433a2-1899-3d74-5b96-4127d7114011"
    Name: "Double Shot Espresso"
    Description: "Speed up your next 6 Expeditions!"
    Price: 175
    GoodyType: 2
    Uses: 6
    ReductionPercent: 0.65
  }
}
goodies:
{
  key: "Goodie_Espresso_3"
  value:
  {
    AuthoredId: "05451c32-ba50-4154-39a2-e7e20fcc3786"
    Name: "Triple Shot Espresso"
    Description: "Speed up your next 12 Expeditions!"
    Price: 300
    GoodyType: 2
    Uses: 12
    ReductionPercent: 0.5
  }
}
goodies:
{
  key: "Goodie_PressureCooker_3"
  value:
  {
    AuthoredId: "695bdc7d-e01a-17d4-a8a1-10c714c7da67"
    Name: "Pristine Pressure Cooker"
    Description: "Speed up your next 12 Cooks!"
    Price: 300
    GoodyType: 1
    Uses: 12
    ReductionPercent: 0.5
  }
}
goodybundles:
{
  key: "GoodieBundle_1"
  value:
  {
    AuthoredId: "645a9411-d8f1-3e24-aa12-f3f79665dca2"
    Name: "A LITTLE PEP IN YOUR STEP"
    Price: 175
    BundledGoodies: {GoodyId: "339fa37e-d9be-0944-d990-9daff7017d91" Quantity: 1}
    BundledGoodies: {GoodyId: "ca3378db-cd54-e514-7ae8-23705781bb9d" Quantity: 1}
  }
}
goodybundles:
{
  key: "GoodieBundle_2"
  value:
  {
    AuthoredId: "59a77d42-c84a-5ac4-5aba-e8e06e525ed2"
    Name: "MIZER'S EFFICIENCY PACK"
    Price: 300
    BundledGoodies: {GoodyId: "086433a2-1899-3d74-5b96-4127d7114011" Quantity: 1}
    BundledGoodies: {GoodyId: "de8529bb-e9e4-f764-3b93-64032254f105" Quantity: 1}
  }
}
goodybundles:
{
  key: "GoodieBundle_3"
  value:
  {
    AuthoredId: "1ec26860-947c-7a24-8954-3e2db54b0af1"
    Name: "THE SHWAG BAG"
    Price: 500
    BundledGoodies: {GoodyId: "05451c32-ba50-4154-39a2-e7e20fcc3786" Quantity: 1}
    BundledGoodies: {GoodyId: "695bdc7d-e01a-17d4-a8a1-10c714c7da67" Quantity: 1}
  }
}
goodybundles:
{
  key: "GoodieBundle_4"
  value:
  {
    AuthoredId: "486bdddb-fbe9-0684-49e0-ba935103601b"
    Name: "BARGAIN HUNTER'S DREAM"
    Price: 800
    BundledGoodies: {GoodyId: "05451c32-ba50-4154-39a2-e7e20fcc3786" Quantity: 2}
    BundledGoodies: {GoodyId: "695bdc7d-e01a-17d4-a8a1-10c714c7da67" Quantity: 2}
  }
}
sweetenerblueprints:
{
  key: "Sweetener_R5"
  value:
  {
    AuthoredId: "a5d19aba-ba28-01d4-e8a7-77ba3481288e"
    Name: "Sweetener: Salty Sweet"
    Description: "Sweeten one of your Salty Sweet Treats! "
    Price: 50
    Duration: 100
    CookCost: 15
    RequiredSweetness: 5
    RewardChoices: {Description: "ThisRecipewillgranttheTreat1extraMoveofeithertheHeavy BlockingorDistanceTypes uptoUncommonQuality.Alsohasachancetoadd/replaceArmorandTrick uptoUncommonQuality." RewardsTableId: "808db761-05dc-a2f4-6a31-210551e2cb5a" BaseRollCount: 1 ArmorRewardsTableId: "0f3cb80c-c70c-1234-8a1b-1d349037a0c7" ArmorRollCount: 1 MoveRewardsTableId: "2913144c-50ad-ccf4-ebdc-fb5d2286b88c" MoveRollCount: 1 RequiredCandy: {key: "7ea89b0d-7070-d3e4-a86f-c0e75c0e4ff5" value: 6} RequiredCandy: {key: "63a8ea74-65ee-c024-b8f0-87e173258257" value: 4} RequiredCandy: {key: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" value: 8} RequiredCandy: {key: "f6025c1b-55e8-da24-d96f-0ecebf74c436" value: 6} RequiredCandy: {key: "4e343f68-1aaa-0e84-7bab-eae458b68264" value: 6}}
    RewardChoices: {Description: "ThisRecipewillgranttheTreat1extraMoveofeithertheHeavy SpeedyorTrickyTypes uptoUncommonQuality.Alsohasachancetoadd/replaceArmorandTrick uptoUncommonQuality." RewardsTableId: "808db761-05dc-a2f4-6a31-210551e2cb5a" BaseRollCount: 1 ArmorRewardsTableId: "0f3cb80c-c70c-1234-8a1b-1d349037a0c7" ArmorRollCount: 1 MoveRewardsTableId: "94f80543-5661-ff84-8860-1e9b222d6c48" MoveRollCount: 1 RequiredCandy: {key: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" value: 14} RequiredCandy: {key: "9b70e722-0442-2924-092c-93efc1d92975" value: 6} RequiredCandy: {key: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" value: 5} RequiredCandy: {key: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" value: 5}}
  }
}
sweetenerblueprints:
{
  key: "Sweetener_R6"
  value:
  {
    AuthoredId: "36adece2-8ed9-3114-db6e-24fa8c494fa5"
    Name: "Sweetener: Artificially Sweet"
    Description: "Sweeten one of your Artificially Sweet Treats! "
    Price: 50
    Duration: 100
    CookCost: 15
    RequiredSweetness: 6
    RewardChoices: {Description: "ThisRecipewillgranttheTreat1extraMoveofeithertheHeavy BlockingorDistanceTypes uptoUncommonQuality.Alsohasachancetoadd/replaceArmorandTrick uptoUncommonQuality." RewardsTableId: "808db761-05dc-a2f4-6a31-210551e2cb5a" BaseRollCount: 1 ArmorRewardsTableId: "0f3cb80c-c70c-1234-8a1b-1d349037a0c7" ArmorRollCount: 1 MoveRewardsTableId: "2913144c-50ad-ccf4-ebdc-fb5d2286b88c" MoveRollCount: 1 RequiredCandy: {key: "5a087336-6c2d-a684-1b72-a86d6ff6931f" value: 10} RequiredCandy: {key: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" value: 7} RequiredCandy: {key: "87040ddf-f0cb-f894-dbbd-a4f266cae4e7" value: 13}}
    RewardChoices: {Description: "ThisRecipewillgranttheTreat1extraMoveofeithertheHeavy SpeedyorTrickyTypes uptoUncommonQuality.Alsohasachancetoadd/replaceArmorandTrick uptoUncommonQuality." RewardsTableId: "808db761-05dc-a2f4-6a31-210551e2cb5a" BaseRollCount: 1 ArmorRewardsTableId: "0f3cb80c-c70c-1234-8a1b-1d349037a0c7" ArmorRollCount: 1 MoveRewardsTableId: "94f80543-5661-ff84-8860-1e9b222d6c48" MoveRollCount: 1 RequiredCandy: {key: "1983b3b7-4a9e-8f14-b981-c94c32d7648b" value: 12} RequiredCandy: {key: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" value: 8} RequiredCandy: {key: "9b70e722-0442-2924-092c-93efc1d92975" value: 4} RequiredCandy: {key: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" value: 6}}
  }
}
sweetenerblueprints:
{
  key: "Sweetener_R7"
  value:
  {
    AuthoredId: "c5b8cd6d-59ae-8f24-7891-d8948de941b0"
    Name: "Sweetener: Sticky Sweet"
    Description: "Sweeten one of your Sticky Sweet Treats!"
    Price: 50
    Duration: 100
    CookCost: 15
    RequiredSweetness: 7
    RewardChoices: {Description: "ThisRecipewillgranttheTreat1extraMoveofeithertheHeavy BlockingorDistanceTypes uptoRareQuality.Alsohasachancetoadd/replaceArmor uptoRareQuality andTrick uptoUncommonQuality." RewardsTableId: "808db761-05dc-a2f4-6a31-210551e2cb5a" BaseRollCount: 1 ArmorRewardsTableId: "f2092146-706b-09f4-a80b-64aabe5ff4a6" ArmorRollCount: 1 MoveRewardsTableId: "b1fc13b2-84e3-e3b4-a904-371d21b0bd2c" MoveRollCount: 1 RequiredCandy: {key: "8ac3d9f0-2a3e-b724-6a96-ce73b5ee215c" value: 5} RequiredCandy: {key: "9c64b7e4-0458-65c4-9b79-5cc35e847f9c" value: 10} RequiredCandy: {key: "4e343f68-1aaa-0e84-7bab-eae458b68264" value: 15} RequiredCandy: {key: "4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" value: 10}}
    RewardChoices: {Description: "ThisRecipewillgranttheTreat1extraMoveofeithertheHeavyorSpeedyTypes uptoRareQuality.Alsohasachancetoadd/replaceArmor uptoUncommonQuality andTrick uptoRareQuality." RewardsTableId: "3577d41a-18bc-8f74-392c-579c129e9181" BaseRollCount: 1 ArmorRewardsTableId: "0f3cb80c-c70c-1234-8a1b-1d349037a0c7" ArmorRollCount: 1 MoveRewardsTableId: "e03a72e4-d6e6-e0b4-4936-a09848309399" MoveRollCount: 1 RequiredCandy: {key: "e5afded6-705a-2034-7b1c-556ac877fb0f" value: 13} RequiredCandy: {key: "1525c955-d3c0-9f84-69fb-cf98837f4f85" value: 7} RequiredCandy: {key: "7ddba6f2-b08a-f954-4a97-eae6af175005" value: 12} RequiredCandy: {key: "16e0b307-2b40-4584-294c-dee505e47827" value: 4} RequiredCandy: {key: "9b70e722-0442-2924-092c-93efc1d92975" value: 4}}
  }
}
sweetenerblueprints:
{
  key: "Sweetener_R8"
  value:
  {
    AuthoredId: "633d5927-9d0e-d3a4-b954-3a8f146aa792"
    Name: "Sweetener: Sugary Sweet"
    Description: "Sweeten one of your Sugary Sweet Treats!"
    Price: 50
    Duration: 100
    CookCost: 15
    RequiredSweetness: 8
    RewardChoices: {Description: "ThisRecipewillgranttheTreat1extraMoveofeithertheHeavy BlockingorDistanceTypes uptoRareQuality.Alsohasachancetoadd/replaceArmor uptoRareQuality andTrick uptoRareQuality." RewardsTableId: "3577d41a-18bc-8f74-392c-579c129e9181" BaseRollCount: 1 ArmorRewardsTableId: "f2092146-706b-09f4-a80b-64aabe5ff4a6" ArmorRollCount: 1 MoveRewardsTableId: "b1fc13b2-84e3-e3b4-a904-371d21b0bd2c" MoveRollCount: 1 RequiredCandy: {key: "86b5fac0-4ec1-09e4-6ab7-96d3fd13dd04" value: 22} RequiredCandy: {key: "2aae625b-a8b1-8ef4-ba8f-32a45702f9af" value: 8} RequiredCandy: {key: "f6025c1b-55e8-da24-d96f-0ecebf74c436" value: 10}}
    RewardChoices: {Description: "ThisRecipewillgranttheTreat1extraMoveofeithertheHeavyorSpeedyTypes uptoRareQuality.Alsohasachancetoadd/replaceArmor uptoRareQuality andTrick uptoRareQuality." RewardsTableId: "3577d41a-18bc-8f74-392c-579c129e9181" BaseRollCount: 1 ArmorRewardsTableId: "f2092146-706b-09f4-a80b-64aabe5ff4a6" ArmorRollCount: 1 MoveRewardsTableId: "e03a72e4-d6e6-e0b4-4936-a09848309399" MoveRollCount: 1 RequiredCandy: {key: "af509c01-933e-ce14-49c3-106081e16d75" value: 6} RequiredCandy: {key: "87040ddf-f0cb-f894-dbbd-a4f266cae4e7" value: 14} RequiredCandy: {key: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" value: 7} RequiredCandy: {key: "63a8ea74-65ee-c024-b8f0-87e173258257" value: 13}}
  }
}
sweetenerblueprints:
{
  key: "Sweetener_R9"
  value:
  {
    AuthoredId: "d2ae2cf3-90d3-e6a4-383f-042d902e87b6"
    Name: "Sweetener: Pretty Sweet"
    Description: "Sweeten one of your Sweetie Pie Treats!"
    Price: 50
    Duration: 100
    CookCost: 15
    RequiredSweetness: 9
    RewardChoices: {Description: "ThisRecipewillgranttheTreat1extraMoveofanyMoveType uptoEpicQuality.Alsohasachancetoadd/replaceArmor uptoEpicQuality andTrick uptoRareQuality." RewardsTableId: "3577d41a-18bc-8f74-392c-579c129e9181" BaseRollCount: 1 ArmorRewardsTableId: "b057b0af-c4d9-0464-98cf-0303f0baf359" ArmorRollCount: 1 MoveRewardsTableId: "f6655962-a4bc-4f14-985b-79dd68f61103" MoveRollCount: 1 RequiredCandy: {key: "8932caea-da2e-6384-58ec-749624e213c9" value: 12} RequiredCandy: {key: "aa0cb18d-5681-5504-09d7-4db06bf7a4d8" value: 4} RequiredCandy: {key: "7ff63ec3-352c-6ea4-98ad-55c4f2fb375c" value: 4} RequiredCandy: {key: "7ea89b0d-7070-d3e4-a86f-c0e75c0e4ff5" value: 16} RequiredCandy: {key: "524c8f4f-cb50-ea04-dafd-e192f30c7656" value: 14}}
    RewardChoices: {Description: "ThisRecipewillgranttheTreat1extraMoveofanyMoveType uptoEpicQuality.Alsohasachancetoadd/replaceArmor uptoRareQuality andTrick uptoEpicQuality." RewardsTableId: "fc0c2504-a2ec-e364-093d-5fca84205e2d" BaseRollCount: 1 ArmorRewardsTableId: "f2092146-706b-09f4-a80b-64aabe5ff4a6" ArmorRollCount: 1 MoveRewardsTableId: "5c6f7ec8-7032-73a4-28d9-ce67b51c9db6" MoveRollCount: 1 RequiredCandy: {key: "08847311-c629-a154-5958-13e8f1d77f88" value: 6} RequiredCandy: {key: "f7668430-bfc1-71f4-29b8-f36c7d6905d5" value: 8} RequiredCandy: {key: "9ff884ab-efcb-0af4-5acd-976fea43b3a3" value: 21} RequiredCandy: {key: "5a087336-6c2d-a684-1b72-a86d6ff6931f" value: 12} RequiredCandy: {key: "8aac4645-29cf-fe04-59c0-415508b0432e" value: 3}}
  }
}
sweetenerblueprints:
{
  key: "Sweetener_R2"
  value:
  {
    AuthoredId: "d596403b-b76f-52c4-6956-4bfd55231de0"
    Name: "Sweetener: Bittersweet"
    Description: "Sweeten one of your Bittersweet Treats!"
    Price: 50
    Duration: 20
    CookCost: 15
    RequiredSweetness: 2
    RewardChoices: {Description: "ThisRecipewillgranttheTreat1extraMoveofeithertheBlocking Distance orHeavyTypes uptoCommonQuality." RewardsTableId: "1290674d-4fa7-07c4-3987-14262876f209" BaseRollCount: 1 ArmorRewardsTableId: "1290674d-4fa7-07c4-3987-14262876f209" ArmorRollCount: 1 MoveRewardsTableId: "70c75ebc-7d57-f124-6bc4-d0b7b7ff3d37" MoveRollCount: 1 RequiredCandy: {key: "f6025c1b-55e8-da24-d96f-0ecebf74c436"  value: 5} RequiredCandy: {key: "7ddba6f2-b08a-f954-4a97-eae6af175005" value: 5} RequiredCandy: {key: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" value: 5} RequiredCandy: {key: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" value: 5}}
    RewardChoices: {Description: "ThisRecipewillgranttheTreat1extraMoveofeithertheSpeedyorTrickyTypes uptoCommonQuality." RewardsTableId: "1290674d-4fa7-07c4-3987-14262876f209" BaseRollCount: 1 ArmorRewardsTableId: "1290674d-4fa7-07c4-3987-14262876f209" ArmorRollCount: 1 MoveRewardsTableId: "30d9967d-8618-7254-3bff-46e09e717719" MoveRollCount: 1 RequiredCandy: {key: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" value: 10} RequiredCandy: {key: "f9c0a4d2-f049-29a4-eb1e-2269de35798e" value: 10}}
  }
}
sweetenerblueprints:
{
  key: "Sweetener_R3"
  value:
  {
    AuthoredId: "7e48a714-d191-0884-abd5-ec350a925a63"
    Name: "Sweetener: Semi Sweet"
    Description: "Sweeten one of your Semi Sweet Treats!"
    Price: 50
    Duration: 50
    CookCost: 15
    RequiredSweetness: 3
    RewardChoices: {Description: "ThisRecipewillgranttheTreat1extraMoveofeithertheBlocking Distance orHeavyTypes uptoCommonQuality." RewardsTableId: "1290674d-4fa7-07c4-3987-14262876f209" BaseRollCount: 1 ArmorRewardsTableId: "1290674d-4fa7-07c4-3987-14262876f209" ArmorRollCount: 1 MoveRewardsTableId: "70c75ebc-7d57-f124-6bc4-d0b7b7ff3d37" MoveRollCount: 1 RequiredCandy: {key: "f6025c1b-55e8-da24-d96f-0ecebf74c436" value: 10} RequiredCandy: {key: "c2774273-0cd4-2494-fa02-76cffe1ef1ef" value: 5} RequiredCandy: {key: "4e343f68-1aaa-0e84-7bab-eae458b68264" value: 5}}
    RewardChoices: {Description: "ThisRecipewillgranttheTreat1extraMoveofeithertheSpeedyorTrickyTypes uptoCommonQuality." RewardsTableId: "1290674d-4fa7-07c4-3987-14262876f209" BaseRollCount: 1 ArmorRewardsTableId: "1290674d-4fa7-07c4-3987-14262876f209" ArmorRollCount: 1 MoveRewardsTableId: "30d9967d-8618-7254-3bff-46e09e717719" MoveRollCount: 1 RequiredCandy: {key: "934dae05-689f-30e4-5804-d497aee0b47c" value: 15} RequiredCandy: {key: "524c8f4f-cb50-ea04-dafd-e192f30c7656" value: 5}}
  }
}
sweetenerblueprints:
{
  key: "Sweetener_R4"
  value:
  {
    AuthoredId: "14915002-83dd-83d4-e987-6cdf4ca3673b"
    Name: "Sweetener: Sweet and Sour"
    Description: "Sweeten one of your Sweet and Sour Treats!"
    Price: 50
    Duration: 100
    CookCost: 15
    RequiredSweetness: 4
    RewardChoices: {Description: "ThisRecipewillgranttheTreat1extraMoveofeithertheHeavy BlockingorDistanceTypes uptoUncommonQuality.Alsohasachancetoadd/replaceArmorandTrick uptoCommonQuality." RewardsTableId: "87d8fe8d-231a-6234-3af6-a9246f364e17" BaseRollCount: 1 ArmorRewardsTableId: "f38319c2-1e6a-97d4-8976-5ecf1aee51ee" ArmorRollCount: 1 MoveRewardsTableId: "2913144c-50ad-ccf4-ebdc-fb5d2286b88c" MoveRollCount: 1 RequiredCandy: {key: "f6025c1b-55e8-da24-d96f-0ecebf74c436" value: 15} RequiredCandy: {key: "7ddba6f2-b08a-f954-4a97-eae6af175005" value: 10} RequiredCandy: {key:"4c8a2a05-5649-8234-ea6a-1a80d41cbeb1" value: 5}}
    RewardChoices: {Description: "ThisRecipewillgranttheTreat1extraMoveofeithertheSpeedyorTrickyTypes uptoUncommonQuality.Alsohasachancetoadd/replaceArmorandTrick uptoCommonQuality." RewardsTableId: "87d8fe8d-231a-6234-3af6-a9246f364e17" BaseRollCount: 1 ArmorRewardsTableId: "f38319c2-1e6a-97d4-8976-5ecf1aee51ee" ArmorRollCount: 1 MoveRewardsTableId: "94f80543-5661-ff84-8860-1e9b222d6c48" MoveRollCount: 1 RequiredCandy: {key: "5a087336-6c2d-a684-1b72-a86d6ff6931f" value: 2} RequiredCandy: {key: "934dae05-689f-30e4-5804-d497aee0b47c" value: 3} RequiredCandy: {key: "8aac4645-29cf-fe04-59c0-415508b0432e" value: 10} RequiredCandy: {key: "16e0b307-2b40-4584-294c-dee505e47827" value: 5}}
  }
}
sweetenerblueprints:
{
  key: "Sweetener_R10"
  value:
  {
    AuthoredId: "20334892-5075-d1f4-99da-602f74bd92b1"
    Name: "Sweetener: Super Sweet"
    Description: "Sweeten one of your Super Sweet Treats!"
    Price: 50
    Duration: 100
    CookCost: 15
    RequiredSweetness: 10
    RewardChoices: {Description: "ThisRecipewillgranttheTreat1extraMoveofanyMoveType uptoEpicQuality.Alsohasachancetoadd/replaceArmor uptoEpicQuality andTrick uptoEpicQuality." RewardsTableId: "fc0c2504-a2ec-e364-093d-5fca84205e2d" BaseRollCount: 1 ArmorRewardsTableId: "b057b0af-c4d9-0464-98cf-0303f0baf359" ArmorRollCount: 1 MoveRewardsTableId: "d07c3481-691a-71e4-990e-9b5739afcff1" MoveRollCount: 1 RequiredCandy: {key: "47b8a853-791a-1424-1ba9-4adaf4a3f4a5" value: 10}RequiredCandy: { key: "2f765969-c607-d4a4-0b9c-72d3cce7fbec" value: 6}RequiredCandy: { key: "8aa0e6a7-0b45-3404-9b39-dc4a4300a87f" value: 12}RequiredCandy: { key: "af509c01-933e-ce14-49c3-106081e16d75" value: 8}RequiredCandy: { key: "3d47dfc5-ce51-9c54-68f1-6819b251e6c7" value: 14}RequiredCandy: { key: "f6025c1b-55e8-da24-d96f-0ecebf74c436" value: 1}}
    RewardChoices: {Description: "ThisRecipewillgranttheTreat1extraMoveofanyMoveType uptoEpicQuality.Alsohasachancetoadd/replaceArmor uptoEpicQuality andTrick uptoEpicQuality." RewardsTableId: "fc0c2504-a2ec-e364-093d-5fca84205e2d" BaseRollCount: 1 ArmorRewardsTableId: "b057b0af-c4d9-0464-98cf-0303f0baf359" ArmorRollCount: 1 MoveRewardsTableId: "d07c3481-691a-71e4-990e-9b5739afcff1" MoveRollCount: 1 RequiredCandy: {key: "a2c2697c-d738-c004-4b32-d800921bab06" value: 6} RequiredCandy: {key: "0ee1131a-b8e4-05c4-9896-3690ebf8563a" value: 3} RequiredCandy: {key: "12ea78d4-e0f3-e5b4-1882-04787fa86d15" value: 21} RequiredCandy: {key: "28089a53-62ac-2044-1964-29f2f31d8a33" value: 4} RequiredCandy: {key: "40067d7d-6651-cbc4-c8ed-785d73e9ab57" value: 16}}
  }
}
tournamentbluprints:
{
  key: "Tournament_1stTourney"
  value:
  {
    AuthoredId: "cbd2e78a-37ce-b864-793d-8dd27788a774"
    Name: "An Overlord's First Challenge"
    Duration: 2
    TeamCount: 2
    TeamSize: 2
    MinSweetness: 1
    MaxSweetness: 1
    MaxRewardQuality: 2
    BaseRewardsTableId: "e6d106ef-38e1-67d4-db02-fd1fde4d94a1"
    BaseRollCount: 1
    WinnerRewardsTableId: "4c95fe23-e5e4-15d4-2a99-dbac2f3daeed"
    WinnerRollCount: 1
    JoinCost: 0
  }
}
tournamentbluprints:
{
  key: "Tournament_Rank4a"
  value:
  {
    AuthoredId: "99258908-ce4f-50e4-2880-99f0027b8d2b"
    Name: "The Cotton Candy Qualifiers"
    Duration: 60
    TeamCount: 3
    TeamSize: 3
    MinSweetness: 4
    MaxSweetness: 6
    MaxRewardQuality: 2
    BaseRewardsTableId: "facf013d-6328-82a4-8b61-32db1c3c5282"
    BaseRollCount: 2
    WinnerRewardsTableId: "facf013d-6328-82a4-8b61-32db1c3c5282"
    WinnerRollCount: 4
    JoinCost: 0
  }
}
tournamentbluprints:
{
  key: "Tournament_Rank4b"
  value:
  {
    AuthoredId: "e17e19da-139b-c484-2bc2-6eec8d407c8a"
    Name: "The Cotton Candy Cup"
    Duration: 125
    TeamCount: 4
    TeamSize: 4
    MinSweetness: 4
    MaxSweetness: 6
    MaxRewardQuality: 2
    BaseRewardsTableId: "facf013d-6328-82a4-8b61-32db1c3c5282"
    BaseRollCount: 3
    WinnerRewardsTableId: "facf013d-6328-82a4-8b61-32db1c3c5282"
    WinnerRollCount: 5
    JoinCost: 0
  }
}
tournamentbluprints:
{
  key: "Tournament_Rank3a"
  value:
  {
    AuthoredId: "eedb6522-2311-3ef4-c999-d3ec275ea496"
    Name: "The Jelly Bean Quickie"
    Duration: 60
    TeamCount: 2
    TeamSize: 2
    MinSweetness: 3
    MaxSweetness: 5
    MaxRewardQuality: 2
    BaseRewardsTableId: "f222e20f-3157-f314-6803-692d4ea1c9f2"
    BaseRollCount: 2
    WinnerRewardsTableId: "f222e20f-3157-f314-6803-692d4ea1c9f2"
    WinnerRollCount: 4
    JoinCost: 0
  }
}
tournamentbluprints:
{
  key: "Tournament_Rank3b"
  value:
  {
    AuthoredId: "f6cbb7e0-a2f3-3e14-2be5-477eeefe8963"
    Name: "The Jelly Bean Grand"
    Duration: 125
    TeamCount: 4
    TeamSize: 4
    MinSweetness: 3
    MaxSweetness: 5
    MaxRewardQuality: 2
    BaseRewardsTableId: "f222e20f-3157-f314-6803-692d4ea1c9f2"
    BaseRollCount: 3
    WinnerRewardsTableId: "f222e20f-3157-f314-6803-692d4ea1c9f2"
    WinnerRollCount: 5
    JoinCost: 0
  }
}
tournamentbluprints:
{
  key: "Tournament_Rank9b"
  value:
  {
    AuthoredId: "06819c56-b599-d864-dbed-b1df4513ca11"
    Name: "The Silver Candy Cane Final Round"
    Duration: 1400
    TeamCount: 10
    TeamSize: 10
    MinSweetness: 9
    MaxSweetness: 10
    MaxRewardQuality: 4
    BaseRewardsTableId: "53f146c5-6d06-d7e4-e970-5dd72dbef229"
    BaseRollCount: 3
    WinnerRewardsTableId: "53f146c5-6d06-d7e4-e970-5dd72dbef229"
    WinnerRollCount: 5
    JoinCost: 0
  }
}
tournamentbluprints:
{
  key: "Tournament_Rank2a"
  value:
  {
    AuthoredId: "1dbea2e9-1abf-8524-5bb9-4c27d7d4f631"
    Name: "The Gumdrop Open"
    Duration: 15
    TeamCount: 3
    TeamSize: 3
    MinSweetness: 2
    MaxSweetness: 4
    MaxRewardQuality: 1
    BaseRewardsTableId: "bd3bfc25-5636-a704-a96e-59473e39a368"
    BaseRollCount: 2
    WinnerRewardsTableId: "bd3bfc25-5636-a704-a96e-59473e39a368"
    WinnerRollCount: 4
    JoinCost: 0
  }
}
tournamentbluprints:
{
  key: "Tournament_Rank2b"
  value:
  {
    AuthoredId: "0c7385d1-d807-1634-4ae1-9eb4e9991b17"
    Name: "The Gumdrop Finals"
    Duration: 30
    TeamCount: 4
    TeamSize: 4
    MinSweetness: 2
    MaxSweetness: 4
    MaxRewardQuality: 1
    BaseRewardsTableId: "bd3bfc25-5636-a704-a96e-59473e39a368"
    BaseRollCount: 3
    WinnerRewardsTableId: "bd3bfc25-5636-a704-a96e-59473e39a368"
    WinnerRollCount: 5
    JoinCost: 0
  }
}
tournamentbluprints:
{
  key: "Tournament_Rank8b"
  value:
  {
    AuthoredId: "77a516a8-4824-fbf4-ca26-42d18e610a7a"
    Name: "The Bronze Jawbreaker Team Challenge"
    Duration: 1400
    TeamCount: 5
    TeamSize: 4
    MinSweetness: 8
    MaxSweetness: 10
    MaxRewardQuality: 4
    BaseRewardsTableId: "ba99802e-6d0c-3da4-b9b1-99fc8105e54a"
    BaseRollCount: 3
    WinnerRewardsTableId: "ba99802e-6d0c-3da4-b9b1-99fc8105e54a"
    WinnerRollCount: 5
    JoinCost: 0
  }
}
tournamentbluprints:
{
  key: "Tournament_Rank1a"
  value:
  {
    AuthoredId: "e694d5f8-e454-7774-ca76-fc2637a9407f"
    Name: "The Chocolate Chip Training Rounds"
    Duration: 15
    TeamCount: 2
    TeamSize: 2
    MinSweetness: 1
    MaxSweetness: 3
    MaxRewardQuality: 1
    BaseRewardsTableId: "be507768-f89e-7114-3a40-655521f5ecad"
    BaseRollCount: 2
    WinnerRewardsTableId: "be507768-f89e-7114-3a40-655521f5ecad"
    WinnerRollCount: 4
    JoinCost: 0
  }
}
tournamentbluprints:
{
  key: "Tournament_Rank7a"
  value:
  {
    AuthoredId: "ec723560-ccfa-c984-89a6-f578b5387ce9"
    Name: "The Buttercup Junior Bowl"
    Duration: 350
    TeamCount: 4
    TeamSize: 4
    MinSweetness: 7
    MaxSweetness: 9
    MaxRewardQuality: 3
    BaseRewardsTableId: "6c280e9c-2326-ce44-895c-b350dd01543a"
    BaseRollCount: 2
    WinnerRewardsTableId: "6c280e9c-2326-ce44-895c-b350dd01543a"
    WinnerRollCount: 4
    JoinCost: 0
  }
}
tournamentbluprints:
{
  key: "Tournament_Rank7b"
  value:
  {
    AuthoredId: "fbc32cb0-c5f4-4884-1a53-f38f1c57e357"
    Name: "The Buttercup Grand Melee"
    Duration: 700
    TeamCount: 5
    TeamSize: 4
    MinSweetness: 7
    MaxSweetness: 9
    MaxRewardQuality: 3
    BaseRewardsTableId: "6c280e9c-2326-ce44-895c-b350dd01543a"
    BaseRollCount: 3
    WinnerRewardsTableId: "6c280e9c-2326-ce44-895c-b350dd01543a"
    WinnerRollCount: 5
    JoinCost: 0
  }
}
tournamentbluprints:
{
  key: "Tournament_Rank9a"
  value:
  {
    AuthoredId: "8dc57404-ec23-94d4-3919-5ed9e5d4e37f"
    Name: "The Silver Candy Cane Series"
    Duration: 700
    TeamCount: 5
    TeamSize: 5
    MinSweetness: 9
    MaxSweetness: 10
    MaxRewardQuality: 4
    BaseRewardsTableId: "53f146c5-6d06-d7e4-e970-5dd72dbef229"
    BaseRollCount: 2
    WinnerRewardsTableId: "53f146c5-6d06-d7e4-e970-5dd72dbef229"
    WinnerRollCount: 4
    JoinCost: 0
  }
}
tournamentbluprints:
{
  key: "Tournament_Rank6a"
  value:
  {
    AuthoredId: "1af67bae-6ab2-29e4-9b62-805c73901881"
    Name: "The Gummi Bear Warm up"
    Duration: 350
    TeamCount: 3
    TeamSize: 3
    MinSweetness: 6
    MaxSweetness: 8
    MaxRewardQuality: 3
    BaseRewardsTableId: "6ca739e1-18c3-9b04-4bb6-cb38d82462ea"
    BaseRollCount: 2
    WinnerRewardsTableId: "6ca739e1-18c3-9b04-4bb6-cb38d82462ea"
    WinnerRollCount: 4
    JoinCost: 0
  }
}
tournamentbluprints:
{
  key: "Tournament_Rank6b"
  value:
  {
    AuthoredId: "b714d4e1-b463-dd14-c943-b3d8a3677a0e"
    Name: "The Gummi Bear Melee"
    Duration: 700
    TeamCount: 4
    TeamSize: 4
    MinSweetness: 6
    MaxSweetness: 8
    MaxRewardQuality: 3
    BaseRewardsTableId: "6ca739e1-18c3-9b04-4bb6-cb38d82462ea"
    BaseRollCount: 3
    WinnerRewardsTableId: "6ca739e1-18c3-9b04-4bb6-cb38d82462ea"
    WinnerRollCount: 5
    JoinCost: 0
  }
}
tournamentbluprints:
{
  key: "Tournament_Rank1b"
  value:
  {
    AuthoredId: "5569ff18-4504-0a54-8b54-518ad7501db8"
    Name: "The Chocolate Chip Pee Wee Cup "
    Duration: 30
    TeamCount: 3
    TeamSize: 4
    MinSweetness: 1
    MaxSweetness: 3
    MaxRewardQuality: 1
    BaseRewardsTableId: "be507768-f89e-7114-3a40-655521f5ecad"
    BaseRollCount: 3
    WinnerRewardsTableId: "be507768-f89e-7114-3a40-655521f5ecad"
    WinnerRollCount: 5
    JoinCost: 0
  }
}
tournamentbluprints:
{
  key: "Tournament_Rank8a"
  value:
  {
    AuthoredId: "9ab085a2-2247-adc4-1857-68c3a31b20c3"
    Name: "The Bronze Jawbreaker Deathmatch"
    Duration: 700
    TeamCount: 4
    TeamSize: 4
    MinSweetness: 8
    MaxSweetness: 10
    MaxRewardQuality: 4
    BaseRewardsTableId: "ba99802e-6d0c-3da4-b9b1-99fc8105e54a"
    BaseRollCount: 2
    WinnerRewardsTableId: "ba99802e-6d0c-3da4-b9b1-99fc8105e54a"
    WinnerRollCount: 4
    JoinCost: 0
  }
}
tournamentbluprints:
{
  key: "Tournament_Rank5a"
  value:
  {
    AuthoredId: "34c5d1a4-0245-3104-6a35-e765865124b1"
    Name: "The Peppermint Casual Cup"
    Duration: 350
    TeamCount: 3
    TeamSize: 3
    MinSweetness: 5
    MaxSweetness: 7
    MaxRewardQuality: 3
    BaseRewardsTableId: "d73e36c9-4533-0ff4-cb72-2137ae467bd8"
    BaseRollCount: 2
    WinnerRewardsTableId: "d73e36c9-4533-0ff4-cb72-2137ae467bd8"
    WinnerRollCount: 4
    JoinCost: 0
  }
}
tournamentbluprints:
{
  key: "Tournament_Rank5b"
  value:
  {
    AuthoredId: "06cb83c5-def7-cbc4-4956-b53f755c075a"
    Name: "The Peppermint Pro Bowl"
    Duration: 700
    TeamCount: 4
    TeamSize: 4
    MinSweetness: 5
    MaxSweetness: 7
    MaxRewardQuality: 3
    BaseRewardsTableId: "d73e36c9-4533-0ff4-cb72-2137ae467bd8"
    BaseRollCount: 3
    WinnerRewardsTableId: "d73e36c9-4533-0ff4-cb72-2137ae467bd8"
    WinnerRollCount: 5
    JoinCost: 0
  }
}
tournamentbluprints:
{
  key: "Tournament_Rank10a"
  value:
  {
    AuthoredId: "e1cb93b2-3ba6-9494-6bd6-9e81994480ee"
    Name: "The Golden Candy Ribbon Preliminaries"
    Duration: 1400
    TeamCount: 10
    TeamSize: 10
    MinSweetness: 10
    MaxSweetness: 10
    MaxRewardQuality: 4
    BaseRewardsTableId: "a8c278b1-2a20-db84-d9ca-977c84fe9cb8"
    BaseRollCount: 2
    WinnerRewardsTableId: "a8c278b1-2a20-db84-d9ca-977c84fe9cb8"
    WinnerRollCount: 4
    JoinCost: 0
  }
}
tournamentbluprints:
{
  key: "Tournament_Rank10b"
  value:
  {
    AuthoredId: "dd89aedb-06c1-1964-d90b-b5977ee61c4c"
    Name: "The Golden Candy Ribbon Champion's Royale"
    Duration: 2800
    TeamCount: 20
    TeamSize: 20
    MinSweetness: 10
    MaxSweetness: 10
    MaxRewardQuality: 4
    BaseRewardsTableId: "a8c278b1-2a20-db84-d9ca-977c84fe9cb8"
    BaseRollCount: 3
    WinnerRewardsTableId: "a8c278b1-2a20-db84-d9ca-977c84fe9cb8"
    WinnerRollCount: 5
    JoinCost: 0
  }
}
testnet_merge:
  {
    params:
      {
        dev_addr: "dSFDAWxovUio63hgtfYd3nz3ir61sJRsXn"
      }
  }
)";

constexpr const char* ROCONFIG_PROTO_TEXT_REGTEST = R"(
# This gets merged in with the mainnet configuration, so we only need
# to specify parameters that actually differ.

params:
  {
    dev_addr: "dHNvNaqcD7XPDnoRjAoyfcMpHRi5upJD7p"
    god_mode: true
  }
)";

DEFINE_string (out_binary, "", "Output file for the binary data");
DEFINE_string (out_text, "", "Output file for the text data");

} // anonymous namespace

int
main (int argc, char** argv)
{
  google::InitGoogleLogging (argv[0]);
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  gflags::SetUsageMessage ("Generate roconfig protocol buffer");
  gflags::SetVersionString (PACKAGE_VERSION);
  gflags::ParseCommandLineFlags (&argc, &argv, true);

  LOG (INFO) << "Parsing hard-coded text proto...";
  pxd::proto::ConfigData pb;
  CHECK (TextFormat::ParseFromString (ROCONFIG_PROTO_TEXT, &pb));
  CHECK (TextFormat::ParseFromString (ROCONFIG_PROTO_TEXT_REGTEST,
                                      pb.mutable_regtest_merge ()));

  if (!FLAGS_out_binary.empty ())
    {
      LOG (INFO) << "Writing binary proto to output file " << FLAGS_out_binary;
      std::ofstream out(FLAGS_out_binary, std::ios::binary);
      CHECK (pb.SerializeToOstream (&out));
    }

  if (!FLAGS_out_text.empty ())
    {
      LOG (INFO) << "Writing text proto to output file " << FLAGS_out_text;
      std::ofstream out(FLAGS_out_text);
      google::protobuf::io::OstreamOutputStream zcOut(&out);
      CHECK (TextFormat::Print (pb, &zcOut));
    }

  google::protobuf::ShutdownProtobufLibrary ();
  return EXIT_SUCCESS;
}

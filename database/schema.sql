--  GSP for the tf blockchain game
--  Copyright (C) 2019-2020  Autonomous Worlds Ltd
--
--  This program is free software: you can redistribute it and/or modify
--  it under the terms of the GNU General Public License as published by
--  the Free Software Foundation, either version 3 of the License, or
--  (at your option) any later version.
--
--  This program is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--  GNU General Public License for more details.
--
--  You should have received a copy of the GNU General Public License
--  along with this program.  If not, see <https://www.gnu.org/licenses/>.

-- =============================================================================

-- Data for the activity instances in the game.
CREATE TABLE IF NOT EXISTS `activities` (

  -- The activity ID, which is assigned based on libxayagame's AutoIds.
  `id` INTEGER PRIMARY KEY,
  
 -- The Xaya name that owns this activity (can be empty).
  `owner` TEXT NOT NULL,  

  -- Additional data encoded as a Activity protocol buffer, some entries are optional and cab be emptry string/zero/null
  `proto` BLOB NOT NULL
);

-- Data for the tournaments instances in the game.
CREATE TABLE IF NOT EXISTS `tournaments` (

  -- The recipe ID, which is assigned based on libxayagame's AutoIds.
  `id` INTEGER PRIMARY KEY,
  
  -- Tournament name, used for easy unit-tests query
  `name` TEXT NOT NULL,  
  
  -- Additional data encoded as a TournamentBlueprint protocol buffer.
  `proto` BLOB NOT NULL,
  
  -- Additional data encoded as a TournamentInstance protocol buffer.
  `instance` BLOB NOT NULL  
);

-- Right now this is used for tracking stuff like last tournament time and easy access to last CHI crystal prices
CREATE TABLE IF NOT EXISTS `globaldata` (

  -- Always 0, as we have only 1 entry
  `id` INTEGER PRIMARY KEY,

  -- Time when last special tournament calculation started
  `lasttournamenttime` TIMESTAMP NOT NULL,
  
  -- Crystal prices in CHI multiplier, by default is 1, updated via g/tfr
  `chimultiplier` INTEGER NOT NULL,
  
 -- Current latest recommended version
  `version` TEXT NOT NULL,  

 -- Vanilla recommended download URL
  `url` TEXT NOT NULL ,

  -- Additional data of std:map encoded as string
  `queuedata` TEXT NOT NULL
);

-- Data for the special tournaments instances in the game.
CREATE TABLE IF NOT EXISTS `specialtournaments` (

  -- The recipe ID, which is assigned based on libxayagame's AutoIds.
  `id` INTEGER PRIMARY KEY,
  
  -- Special Tournament tier, used for easy unit-tests query
  `tier` INTEGER NOT NULL,  
  
  -- Additional data encoded as a TournamentBlueprint protocol buffer.
  `proto` BLOB NOT NULL 
);

-- Data for the recipe instances in the game.
CREATE TABLE IF NOT EXISTS `recepies` (

  -- The recipe ID, which is assigned based on libxayagame's AutoIds.
  `id` INTEGER PRIMARY KEY,
  
 -- The Xaya name that owns this recepie (and is thus allowed to send
  -- moves for it).
  `owner` TEXT NOT NULL,  

  -- Additional data encoded as a CrafterRecipe protocol buffer.
  `proto` BLOB NOT NULL
);

-- Data for the rewards in the game.
CREATE TABLE IF NOT EXISTS `rewards` (

  -- The reward ID, which is assigned based on libxayagame's AutoIds.
  `id` INTEGER PRIMARY KEY,
  
 -- The Xaya name that owns this reward (and is thus allowed to send
  -- moves for it).
  `owner` TEXT NOT NULL,  

  -- Additional data encoded as a ActivityRewardInstance protocol buffer.
  `proto` BLOB NOT NULL  
);

-- Data for the fighters in the game.
CREATE TABLE IF NOT EXISTS `fighters` (

  -- The fighter ID, which is assigned based on libxayagame's AutoIds.
  `id` INTEGER PRIMARY KEY,
  
 -- The Xaya name that owns this fighter (and is thus allowed to send
  -- moves for it).
  `owner` TEXT NOT NULL,  

  -- Additional data encoded as a Fighter protocol buffer.
  `proto` BLOB NOT NULL,
  
  -- The status (as integer corresponding to the FighterStatus enum in C++).
  -- Is always Available on start, when fresh fighter is created.
  `status` INTEGER NULL    
);

-- Data stored for the Xaya players (names) themselves.
CREATE TABLE IF NOT EXISTS `xayaplayers` (

  -- The Xaya p/ name of this account.
  `name` TEXT PRIMARY KEY,

  -- The role (as integer corresponding to the role enum in C++).
  -- Can be null for accounts that are not yet initialised.
  `role` INTEGER NULL,
 
  -- Additional data for the account as a serialised Account proto.
  `proto` BLOB NOT NULL,
  
  -- Additional data for the inventory as a serialised Inventory proto.
  `inventory` BLOB NOT NULL, 
  
  -- has BASE PRESTIGE on start and later is acculated accoring to all assets player posseses
  `prestige` INTEGER NULL
);

-- Height-keyed deferred ongoing operations (cook / expedition / deconstruction /
-- sweetener).  Replaces the old per-player "repeated ongoings" proto field so the
-- per-block tick only touches the operations actually due, instead of scanning and
-- rewriting every player every block (P-E1 / P-E7).
CREATE TABLE IF NOT EXISTS `ongoing_operations` (

  -- The operation ID, assigned based on libxayagame's AutoIds.
  `id` INTEGER PRIMARY KEY,

  -- The ABSOLUTE block height at which this operation resolves.
  `height` INTEGER NOT NULL,

  -- The Xaya p/ name that owns this operation.
  `owner` TEXT NOT NULL,

  -- Additional data encoded as an OngoinOperation protocol buffer (the type and
  -- the operation-specific references: recipe / fighter / blueprint / item / ...).
  `proto` BLOB NOT NULL
);

-- "Which operations are due now" -- the per-block hot query.
CREATE INDEX IF NOT EXISTS `ongoing_operations_by_height`
  ON `ongoing_operations` (`height`);
-- "Does this player already have an ongoing" + per-player JSON.
CREATE INDEX IF NOT EXISTS `ongoing_operations_by_owner`
  ON `ongoing_operations` (`owner`);
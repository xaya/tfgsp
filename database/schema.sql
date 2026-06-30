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

-- Data for the tournaments instances in the game.
CREATE TABLE IF NOT EXISTS `tournaments` (

  -- The recipe ID, which is assigned based on libxayagame's AutoIds.
  `id` INTEGER PRIMARY KEY,
  
  -- Tournament name, used for easy unit-tests query
  `name` TEXT NOT NULL,  
  
  -- Additional data encoded as a TournamentBlueprint protocol buffer.
  `proto` BLOB NOT NULL,
  
  -- Additional data encoded as a TournamentInstance protocol buffer.
  `instance` BLOB NOT NULL,

  -- Mirror of the TournamentInstance proto's `state` field (the TournamentState
  -- enum as an integer).  Kept in sync on every row write so the per-block tick
  -- can filter active tournaments (and the retention GC can find old completed
  -- ones) without deserialising the `instance` BLOB (DEF3).  Added last so the
  -- RESULT_COLUMN indices 1..4 stay valid.
  `state` INTEGER NOT NULL
);

-- "Which tournaments are still active" (per-block hot filter) and the windowed
-- retention GC over Completed rows.
CREATE INDEX IF NOT EXISTS `tournaments_by_state`
  ON `tournaments` (`state`);

-- "Is there an open instance for THIS blueprint?" -- ReopenMissingTournaments asks
-- this once per blueprint every block.  `name` holds the blueprint authoredid, so
-- this composite lets that probe seek one blueprint's active rows (the trailing
-- `state` column also skips that blueprint's Completed backlog) instead of
-- rescanning every active tournament once per blueprint (DEF2).
CREATE INDEX IF NOT EXISTS `tournaments_by_name_state`
  ON `tournaments` (`name`, `state`);

-- Right now this is used for easy access to the last CHI crystal prices
CREATE TABLE IF NOT EXISTS `globaldata` (

  -- Always 0, as we have only 1 entry
  `id` INTEGER PRIMARY KEY,

  -- Crystal prices in CHI multiplier, by default is 1, updated via g/tfr
  `chimultiplier` INTEGER NOT NULL,
  
 -- Current latest recommended version
  `version` TEXT NOT NULL,  

 -- Vanilla recommended download URL
  `url` TEXT NOT NULL ,

  -- Additional data of std:map encoded as string
  `queuedata` TEXT NOT NULL
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

-- Per-owner reward lookups (CountForOwner / QueryForOwner) are hit on every
-- reward roll and every claim; without this index they full-scan the unbounded
-- `rewards` table, which dominates per-block cost under sustained reward
-- generation (mirrors the ongoing_operations_by_owner pattern).
CREATE INDEX IF NOT EXISTS `rewards_by_owner`
  ON `rewards` (`owner`);

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

-- "Which fighters need a per-block status flip" -- the two per-block maintenance
-- scans (free Transfiguring fighters back to Available; expire for-sale listings)
-- act only on Exchange/Transfiguring rows, so this index lets them seek just those
-- instead of scanning the whole (unbounded) fighters table every block (DEF2).
CREATE INDEX IF NOT EXISTS `fighters_by_status`
  ON `fighters` (`status`);

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
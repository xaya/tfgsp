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

-- Per-owner recipe lookups (CountForOwner / QueryForOwner) gate the recipe
-- slot-limit check on every cook and the reward-claim path; without this index
-- they full-scan the unbounded, never-pruned `recepies` table (mirrors
-- rewards_by_owner).
CREATE INDEX IF NOT EXISTS `recepies_by_owner`
  ON `recepies` (`owner`);

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
  `status` INTEGER NULL,

  -- Denormalised projections of proto fields, re-bound from the proto on EVERY
  -- INSERT OR REPLACE (database/fighter.cpp ~FighterInstance) so they can never
  -- drift from the blob. They exist purely to make the marketplace query
  -- (getexchange) and the per-block listing-expiry maintenance index-driven
  -- instead of full scans. (SQLite generated columns can't parse a protobuf, so
  -- these are explicit columns bound in C++.)
  `price` INTEGER NOT NULL DEFAULT 0,
  `expire` INTEGER NOT NULL DEFAULT 0,
  `quality` INTEGER NOT NULL DEFAULT 0,
  `sweetness` INTEGER NOT NULL DEFAULT 0
);

-- "Which fighters need a per-block status flip" -- the two per-block maintenance
-- scans (free Transfiguring fighters back to Available; expire for-sale listings)
-- act only on Exchange/Transfiguring rows, so this index lets them seek just those
-- instead of scanning the whole (unbounded) fighters table every block (DEF2).
CREATE INDEX IF NOT EXISTS `fighters_by_status`
  ON `fighters` (`status`);

-- Per-owner fighter lookups (CountForOwner / QueryForOwner) run on essentially
-- every fighter-affecting move: the inventory-cap gate on cook/buy/expedition
-- and CalculatePrestige's CollectInventoryFighters. Without this index they
-- full-scan the unbounded `fighters` table each time (mirrors rewards_by_owner).
CREATE INDEX IF NOT EXISTS `fighters_by_owner`
  ON `fighters` (`owner`);

-- Marketplace query + event-driven listing expiry: each sort/filter column is
-- indexed with a leading `status` so getexchange (WHERE status=Exchange ORDER BY
-- <col>) and CheckFightersForSale (WHERE status=Exchange AND expire<height) are
-- index-driven and O(page)/O(expiring), never O(all listings).
CREATE INDEX IF NOT EXISTS `fighters_exchange_price`
  ON `fighters` (`status`, `price`);
CREATE INDEX IF NOT EXISTS `fighters_exchange_quality`
  ON `fighters` (`status`, `quality`);
CREATE INDEX IF NOT EXISTS `fighters_exchange_sweetness`
  ON `fighters` (`status`, `sweetness`);
CREATE INDEX IF NOT EXISTS `fighters_exchange_expire`
  ON `fighters` (`status`, `expire`);

-- Data stored for the Xaya players (names) themselves.
CREATE TABLE IF NOT EXISTS `xayaplayers` (

  -- The Xaya p/ name of this account.
  `name` TEXT PRIMARY KEY,

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
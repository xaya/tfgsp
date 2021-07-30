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

-- Data stored for the Xaya players (names) themselves.
CREATE TABLE IF NOT EXISTS `xayaplayers` (

  -- The Xaya p/ name of this account.
  `name` TEXT PRIMARY KEY,

  -- The role (as integer corresponding to the role enum in C++).
  -- Can be null for accounts that are not yet initialised.
  `role` INTEGER NULL,
  
  -- The FTUEstate (as integer corresponding to the FTUEstate enum in C++).
  -- Is always Intor on start, as everyone starts from the tutorial.
  `ftuestate` INTEGER NULL,  

  -- Additional data for the account as a serialised Account proto.
  `proto` BLOB NOT NULL
);

-- =============================================================================

-- Data about the money supply of vCHI.  This is a table with just a few
-- defined entries (keyed by a string) and an associated vCHI amount.
--
-- Keys that we use:
--
--  * "burnsale": vCHI bought in the burnsale process (in exchange for
--                burnt CHI).  This total also defines how much more vCHI
--                can be minted in that way.
--
--  * "gifted": vCHI that have been gifted through god mode (only possible
--              on regtest for testing).
--
CREATE TABLE IF NOT EXISTS `money_supply` (

  -- The key of an entry.
  `key` TEXT PRIMARY KEY,

  -- The associated amount of vCHI.
  `amount` INTEGER NOT NULL

);
/*
    GSP for the TF blockchain game
    Copyright (C) 2024  Autonomous Worlds Ltd

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

#ifndef DATABASE_ONGOINGS_HPP
#define DATABASE_ONGOINGS_HPP

#include "database.hpp"
#include "lazyproto.hpp"

#include "proto/ongoing.pb.h"
#include "proto/roconfig.hpp"

#include <memory>
#include <string>

namespace pxd
{

/**
 * Database result type for rows from the ongoing_operations table.  This is the
 * height-keyed deferred-operations table (P-E1 / P-E7): instead of storing a
 * "repeated OngoinOperation ongoings" field inside every player's proto BLOB and
 * decrementing a per-block countdown, each ongoing is one row whose `height` is
 * the ABSOLUTE block at which it resolves.  The per-block tick then only has to
 * SELECT the rows due now, instead of scanning + rewriting every player.
 */
struct OngoingResult : public Database::ResultType
{
  RESULT_COLUMN (int64_t, id, 1);
  RESULT_COLUMN (int64_t, height, 2);
  RESULT_COLUMN (std::string, owner, 3);
  RESULT_COLUMN (pxd::proto::OngoinOperation, proto, 4);
};

/**
 * Wrapper class around one ongoing operation row.  Instances should be obtained
 * through OngoingsTable.  The owning player is the `owner` column; any fighter /
 * recipe / item the operation refers to is kept inside the OngoinOperation proto
 * (FighterDatabaseID, RecipeID, ...), exactly as before.
 */
class OngoingOperation
{

private:

  /** Database reference this belongs to.  */
  Database& db;

  /** The underlying integer ID in the database.  */
  Database::IdT id;

  /** The UniqueHandles tracker for this instance.  */
  Database::HandleTracker tracker;

  /** The block height at which this operation resolves (absolute).  */
  unsigned height;

  /** The Xaya player name that owns this operation.  */
  std::string owner;

  /** The OngoinOperation proto payload (type + operation-specific refs).  */
  LazyProto<proto::OngoinOperation> data;

  /** Whether a non-proto column (height / owner) needs syncing back.  */
  bool dirtyFields;

  /**
   * Constructs a new operation with an auto-generated ID meant to be inserted
   * into the database.  startHeight is the current block height (stored in the
   * proto for front-end progress display); the caller must still SetHeight() to
   * the absolute resolve height before the handle is released.
   */
  explicit OngoingOperation (Database& d, unsigned startHeight);

  /**
   * Constructs an instance based on the given DB result row.  The result should
   * come from a query made through OngoingsTable.
   */
  explicit OngoingOperation (Database& d,
                             const Database::Result<OngoingResult>& res);

  friend class OngoingsTable;

public:

  /**
   * In the destructor, the underlying database row is written if anything was
   * modified.
   */
  ~OngoingOperation ();

  OngoingOperation () = delete;
  OngoingOperation (const OngoingOperation&) = delete;
  void operator= (const OngoingOperation&) = delete;

  Database::IdT
  GetId () const
  {
    return id;
  }

  unsigned
  GetHeight () const
  {
    return height;
  }

  void
  SetHeight (const unsigned h)
  {
    height = h;
    dirtyFields = true;
  }

  const std::string&
  GetOwner () const
  {
    return owner;
  }

  void
  SetOwner (const std::string& o)
  {
    owner = o;
    dirtyFields = true;
  }

  const proto::OngoinOperation&
  GetProto () const
  {
    return data.Get ();
  }

  proto::OngoinOperation&
  MutableProto ()
  {
    return data.Mutable ();
  }

};

/**
 * Utility class that handles querying the ongoing_operations table and is used
 * to obtain OngoingOperation instances.
 */
class OngoingsTable
{

private:

  /** The Database reference for creating queries.  */
  Database& db;

public:

  /** Movable handle to an instance.  */
  using Handle = std::unique_ptr<OngoingOperation>;

  explicit OngoingsTable (Database& d)
    : db(d)
  {}

  OngoingsTable () = delete;
  OngoingsTable (const OngoingsTable&) = delete;
  void operator= (const OngoingsTable&) = delete;

  /**
   * Creates a new entry and returns the handle so it can be initialised.  The
   * current block height is passed as the start height (mandatory, used only for
   * front-end progress display).
   */
  Handle CreateNew (unsigned startHeight);

  /**
   * Returns a handle for the instance based on a Database::Result.
   */
  Handle GetFromResult (const Database::Result<OngoingResult>& res);

  /**
   * Config-taking overload, for compatibility with the gamestate JSON template
   * helper (which passes the RoConfig through).  The config is unused here.
   */
  Handle GetFromResult (const Database::Result<OngoingResult>& res,
                        const RoConfig& cfg);

  /**
   * Returns a handle for the given ID (or null if it doesn't exist).
   */
  Handle GetById (Database::IdT id);

  /**
   * Queries for all ongoing operations, ordered by ID (deterministic).
   */
  Database::Result<OngoingResult> QueryAll ();

  /**
   * Queries for all operations that need processing at the given (current)
   * block height -- i.e. those whose absolute height has been reached.  Uses
   * `height <= ?1` (not `=`) so any stray past-due row surfaces and can be
   * asserted on while processing, rather than being silently skipped.
   */
  Database::Result<OngoingResult> QueryForHeight (unsigned h);

  /**
   * Queries for all operations owned by the given player, ordered by ID.
   */
  Database::Result<OngoingResult> QueryForOwner (const std::string& owner);

  /**
   * Counts the operations owned by the given player (used for the
   * "already has an ongoing" move guards).
   */
  unsigned CountForOwner (const std::string& owner);

  /**
   * Deletes all operations with exactly the given height.  Used to clean up
   * the operations resolved this block (exact height so a rescheduled op, now
   * at a future height, survives).
   */
  void DeleteForHeight (unsigned h);

  /**
   * Deletes all operations owned by the given player.
   */
  void DeleteForOwner (const std::string& owner);

  /**
   * Deletes the operation with the given ID.
   */
  void DeleteById (Database::IdT id);

};

} // namespace pxd

#endif // DATABASE_ONGOINGS_HPP

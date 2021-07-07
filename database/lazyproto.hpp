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

#ifndef DATABASE_LAZYPROTO_HPP
#define DATABASE_LAZYPROTO_HPP

#include <google/protobuf/arena.h>

#include <string>

namespace pxd
{

/**
 * A class that wraps a protocol buffer and implements "lazy deserialisation".
 * Initially, it just keeps the raw data in a string, and only deserialises the
 * protocol buffer when actually needed.  This can help speed up database
 * accesses for cases where we don't actually need some proto data for certain
 * operations.
 *
 * The class also keeps track of when the protocol buffer was modified, so
 * we know if we need to update it in the database.
 */
template <typename Proto>
  class LazyProto
{

private:

  /**
   * Possible "states" of a lazy proto.
   */
  enum class State : uint8_t
  {

    /** There is no data yet (this instance is uninitialised).  */
    UNINITIALISED,

    /** We have not yet accessed/parsed the byte data.  */
    UNPARSED,

    /**
     * We have parsed the byte data but not modified the proto object.
     * In other words, the serialised data is still in sync with the
     * proto message.
     */
    UNMODIFIED,

    /** The proto message has been modified.  */
    MODIFIED,

  };

  /** The arena used to allocate the parsed message, if any.  */
  google::protobuf::Arena* arena = nullptr;

  /** The raw bytes of the protocol buffer.  */
  mutable std::string data;

  /**
   * The parsed protocol buffer.  If we have an arena set, it is allocated
   * on and owned by the arena.  If no arena is set, the instance is owned
   * by the LazyProto instance and will be destructed accordingly.
   */
  mutable Proto* msg = nullptr;

  /** Current state of this lazy proto.  */
  mutable State state = State::UNINITIALISED;

  /**
   * Ensures that we have an allocated message.
   */
  void EnsureAllocated () const;

  /**
   * Ensures that the protocol buffer is parsed.
   */
  void EnsureParsed () const;

  friend class LazyProtoTests;

public:

  LazyProto () = default;

  /**
   * Constructs a lazy proto instance based on the given byte data.
   */
  explicit LazyProto (std::string&& d);

  ~LazyProto ();

  /* A LazyProto can be moved but not copied.  */
  LazyProto (LazyProto&&);
  LazyProto& operator= (LazyProto&&);

  LazyProto (const LazyProto&) = delete;
  void operator= (const LazyProto&) = delete;

  /**
   * Enables an arena for this instance.  This must only be called if the
   * message is not yet allocated, i.e. the state is unparsed or uninitialised.
   */
  void SetArena (google::protobuf::Arena& a);

  /**
   * Initialises the protocol buffer value as "empty" (i.e. default-constructed
   * protocol buffer message, empty data string).
   */
  void SetToDefault ();

  /**
   * Accesses the message read-only.
   */
  const Proto& Get () const;

  /**
   * Accesses and modifies the proto message.
   */
  Proto& Mutable ();

  /**
   * Returns true if the protocol buffer was modified from the original
   * data (e.g. so we know that it needs updating in the database).
   */
  bool IsDirty () const;

  /**
   * Returns true if this is known to be the empty message.
   */
  bool IsEmpty () const;

  /**
   * Returns a serialised form of the potentially modified protocol buffer.
   */
  const std::string& GetSerialised () const;

};

} // namespace pxd

#include "lazyproto.tpp"

#endif // DATABASE_LAZYPROTO_HPP

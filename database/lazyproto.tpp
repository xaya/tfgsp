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

/* Template implementation code for lazyproto.hpp.  */

#include <glog/logging.h>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

namespace pxd
{

template <typename Proto>
  LazyProto<Proto>::LazyProto (std::string&& d)
    : data(std::move (d)), state(State::UNPARSED)
{}

template <typename Proto>
  LazyProto<Proto>::~LazyProto ()
{
  if (arena == nullptr)
    delete msg;
}

template <typename Proto>
  LazyProto<Proto>::LazyProto (LazyProto&& o)
{
  *this = std::move (o);
}

template <typename Proto>
  LazyProto<Proto>&
  LazyProto<Proto>::operator= (LazyProto&& o)
{
  if (arena == nullptr)
    delete msg;

  arena = o.arena;
  data = std::move (o.data);
  msg = o.msg;
  state = o.state;

  o.msg = nullptr;
  o.state = State::UNINITIALISED;

  return *this;
}

template <typename Proto>
  void
  LazyProto<Proto>::SetArena (google::protobuf::Arena& a)
{
  CHECK (state == State::UNINITIALISED || state == State::UNPARSED);
  CHECK (msg == nullptr);
  CHECK (arena == nullptr);
  arena = &a;
}

template <typename Proto>
  inline void
  LazyProto<Proto>::EnsureAllocated () const
{
  if (msg != nullptr)
    return;

  CHECK (state == State::UNINITIALISED || state == State::UNPARSED);
  msg = google::protobuf::Arena::CreateMessage<Proto> (arena);
}

template <typename Proto>
  inline void
  LazyProto<Proto>::EnsureParsed () const
{
  EnsureAllocated ();

  switch (state)
    {
    case State::UNPARSED:
      CHECK (msg->ParseFromString (data));
      state = State::UNMODIFIED;
      return;

    case State::UNMODIFIED:
    case State::MODIFIED:
      return;

    default:
      LOG (FATAL) << "Unexpected state: " << static_cast<int> (state);
    }
}

template <typename Proto>
  void
  LazyProto<Proto>::SetToDefault ()
{
  EnsureAllocated ();
  data.clear ();
  msg->Clear ();
  state = State::UNMODIFIED;
}

template <typename Proto>
  inline const Proto&
  LazyProto<Proto>::Get () const
{
  EnsureParsed ();
  return *msg;
}

template <typename Proto>
  inline Proto&
  LazyProto<Proto>::Mutable ()
{
  EnsureParsed ();
  state = State::MODIFIED;
  return *msg;
}

template <typename Proto>
  inline bool
  LazyProto<Proto>::IsDirty () const
{
  CHECK (state != State::UNINITIALISED);
  return state == State::MODIFIED;
}

template <typename Proto>
  inline bool
  LazyProto<Proto>::IsEmpty () const
{
  CHECK (state != State::UNINITIALISED);
  return state == State::UNMODIFIED && data.empty ();
}

template <typename Proto>
  const std::string&
  LazyProto<Proto>::GetSerialised () const
{
  switch (state)
    {
    case State::UNPARSED:
    case State::UNMODIFIED:
      return data;

    case State::MODIFIED:
      CHECK (msg != nullptr);
      /* DETERMINISTIC serialisation is consensus-critical.  protobuf map<>
         fields (e.g. the player Inventory's fungible items) otherwise serialise
         their entries in a per-message randomised order, so the SAME logical
         proto yields DIFFERENT bytes across nodes and across replays of the same
         block.  Because xaya::SQLiteGame hashes the stored bytes, that is a
         silent chain fork (and it breaks reorg replay).  SetSerializationDeter-
         ministic sorts map keys, making the blob a pure function of content.  */
      {
        data.clear ();
        google::protobuf::io::StringOutputStream zos (&data);
        google::protobuf::io::CodedOutputStream cos (&zos);
        cos.SetSerializationDeterministic (true);
        CHECK (msg->SerializeToCodedStream (&cos));
      }
      return data;

    default:
      LOG (FATAL) << "Unexpected state: " << static_cast<int> (state);
    }
}

} // namespace pxd

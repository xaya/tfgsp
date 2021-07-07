// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: proto/xaya_player.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_proto_2fxaya_5fplayer_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_proto_2fxaya_5fplayer_2eproto

#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3016000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3016000 < PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers. Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/port_undef.inc>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_table_driven.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata_lite.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_proto_2fxaya_5fplayer_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_proto_2fxaya_5fplayer_2eproto {
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTableField entries[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::AuxiliaryParseTableField aux[]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::ParseTable schema[1]
    PROTOBUF_SECTION_VARIABLE(protodesc_cold);
  static const ::PROTOBUF_NAMESPACE_ID::internal::FieldMetadata field_metadata[];
  static const ::PROTOBUF_NAMESPACE_ID::internal::SerializationTable serialization_table[];
  static const ::PROTOBUF_NAMESPACE_ID::uint32 offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_proto_2fxaya_5fplayer_2eproto;
namespace pxd {
namespace proto {
class XayaPlayer;
struct XayaPlayerDefaultTypeInternal;
extern XayaPlayerDefaultTypeInternal _XayaPlayer_default_instance_;
}  // namespace proto
}  // namespace pxd
PROTOBUF_NAMESPACE_OPEN
template<> ::pxd::proto::XayaPlayer* Arena::CreateMaybeMessage<::pxd::proto::XayaPlayer>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace pxd {
namespace proto {

// ===================================================================

class XayaPlayer PROTOBUF_FINAL :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:pxd.proto.XayaPlayer) */ {
 public:
  inline XayaPlayer() : XayaPlayer(nullptr) {}
  ~XayaPlayer() override;
  explicit constexpr XayaPlayer(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  XayaPlayer(const XayaPlayer& from);
  XayaPlayer(XayaPlayer&& from) noexcept
    : XayaPlayer() {
    *this = ::std::move(from);
  }

  inline XayaPlayer& operator=(const XayaPlayer& from) {
    CopyFrom(from);
    return *this;
  }
  inline XayaPlayer& operator=(XayaPlayer&& from) noexcept {
    if (GetArena() == from.GetArena()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  inline const ::PROTOBUF_NAMESPACE_ID::UnknownFieldSet& unknown_fields() const {
    return _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance);
  }
  inline ::PROTOBUF_NAMESPACE_ID::UnknownFieldSet* mutable_unknown_fields() {
    return _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const XayaPlayer& default_instance() {
    return *internal_default_instance();
  }
  static inline const XayaPlayer* internal_default_instance() {
    return reinterpret_cast<const XayaPlayer*>(
               &_XayaPlayer_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(XayaPlayer& a, XayaPlayer& b) {
    a.Swap(&b);
  }
  inline void Swap(XayaPlayer* other) {
    if (other == this) return;
    if (GetArena() == other->GetArena()) {
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(XayaPlayer* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetArena() == other->GetArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  inline XayaPlayer* New() const final {
    return CreateMaybeMessage<XayaPlayer>(nullptr);
  }

  XayaPlayer* New(::PROTOBUF_NAMESPACE_ID::Arena* arena) const final {
    return CreateMaybeMessage<XayaPlayer>(arena);
  }
  void CopyFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void MergeFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void CopyFrom(const XayaPlayer& from);
  void MergeFrom(const XayaPlayer& from);
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  ::PROTOBUF_NAMESPACE_ID::uint8* _InternalSerialize(
      ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  inline void SharedCtor();
  inline void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(XayaPlayer* other);
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "pxd.proto.XayaPlayer";
  }
  protected:
  explicit XayaPlayer(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  private:
  static void ArenaDtor(void* object);
  inline void RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  public:

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kXayanameFieldNumber = 5,
    kNotusedFieldNumber = 1,
    kRoleFieldNumber = 2,
    kBalanceFieldNumber = 3,
    kBurnsaleBalanceFieldNumber = 4,
  };
  // optional string xayaname = 5;
  bool has_xayaname() const;
  private:
  bool _internal_has_xayaname() const;
  public:
  void clear_xayaname();
  const std::string& xayaname() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_xayaname(ArgT0&& arg0, ArgT... args);
  std::string* mutable_xayaname();
  std::string* release_xayaname();
  void set_allocated_xayaname(std::string* xayaname);
  private:
  const std::string& _internal_xayaname() const;
  void _internal_set_xayaname(const std::string& value);
  std::string* _internal_mutable_xayaname();
  public:

  // optional uint32 notused = 1;
  bool has_notused() const;
  private:
  bool _internal_has_notused() const;
  public:
  void clear_notused();
  ::PROTOBUF_NAMESPACE_ID::uint32 notused() const;
  void set_notused(::PROTOBUF_NAMESPACE_ID::uint32 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint32 _internal_notused() const;
  void _internal_set_notused(::PROTOBUF_NAMESPACE_ID::uint32 value);
  public:

  // optional uint32 role = 2;
  bool has_role() const;
  private:
  bool _internal_has_role() const;
  public:
  void clear_role();
  ::PROTOBUF_NAMESPACE_ID::uint32 role() const;
  void set_role(::PROTOBUF_NAMESPACE_ID::uint32 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint32 _internal_role() const;
  void _internal_set_role(::PROTOBUF_NAMESPACE_ID::uint32 value);
  public:

  // optional uint64 balance = 3;
  bool has_balance() const;
  private:
  bool _internal_has_balance() const;
  public:
  void clear_balance();
  ::PROTOBUF_NAMESPACE_ID::uint64 balance() const;
  void set_balance(::PROTOBUF_NAMESPACE_ID::uint64 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint64 _internal_balance() const;
  void _internal_set_balance(::PROTOBUF_NAMESPACE_ID::uint64 value);
  public:

  // optional uint64 burnsale_balance = 4;
  bool has_burnsale_balance() const;
  private:
  bool _internal_has_burnsale_balance() const;
  public:
  void clear_burnsale_balance();
  ::PROTOBUF_NAMESPACE_ID::uint64 burnsale_balance() const;
  void set_burnsale_balance(::PROTOBUF_NAMESPACE_ID::uint64 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint64 _internal_burnsale_balance() const;
  void _internal_set_burnsale_balance(::PROTOBUF_NAMESPACE_ID::uint64 value);
  public:

  // @@protoc_insertion_point(class_scope:pxd.proto.XayaPlayer)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  ::PROTOBUF_NAMESPACE_ID::internal::HasBits<1> _has_bits_;
  mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr xayaname_;
  ::PROTOBUF_NAMESPACE_ID::uint32 notused_;
  ::PROTOBUF_NAMESPACE_ID::uint32 role_;
  ::PROTOBUF_NAMESPACE_ID::uint64 balance_;
  ::PROTOBUF_NAMESPACE_ID::uint64 burnsale_balance_;
  friend struct ::TableStruct_proto_2fxaya_5fplayer_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// XayaPlayer

// optional uint32 notused = 1;
inline bool XayaPlayer::_internal_has_notused() const {
  bool value = (_has_bits_[0] & 0x00000002u) != 0;
  return value;
}
inline bool XayaPlayer::has_notused() const {
  return _internal_has_notused();
}
inline void XayaPlayer::clear_notused() {
  notused_ = 0u;
  _has_bits_[0] &= ~0x00000002u;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 XayaPlayer::_internal_notused() const {
  return notused_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 XayaPlayer::notused() const {
  // @@protoc_insertion_point(field_get:pxd.proto.XayaPlayer.notused)
  return _internal_notused();
}
inline void XayaPlayer::_internal_set_notused(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _has_bits_[0] |= 0x00000002u;
  notused_ = value;
}
inline void XayaPlayer::set_notused(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _internal_set_notused(value);
  // @@protoc_insertion_point(field_set:pxd.proto.XayaPlayer.notused)
}

// optional uint32 role = 2;
inline bool XayaPlayer::_internal_has_role() const {
  bool value = (_has_bits_[0] & 0x00000004u) != 0;
  return value;
}
inline bool XayaPlayer::has_role() const {
  return _internal_has_role();
}
inline void XayaPlayer::clear_role() {
  role_ = 0u;
  _has_bits_[0] &= ~0x00000004u;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 XayaPlayer::_internal_role() const {
  return role_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 XayaPlayer::role() const {
  // @@protoc_insertion_point(field_get:pxd.proto.XayaPlayer.role)
  return _internal_role();
}
inline void XayaPlayer::_internal_set_role(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _has_bits_[0] |= 0x00000004u;
  role_ = value;
}
inline void XayaPlayer::set_role(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _internal_set_role(value);
  // @@protoc_insertion_point(field_set:pxd.proto.XayaPlayer.role)
}

// optional uint64 balance = 3;
inline bool XayaPlayer::_internal_has_balance() const {
  bool value = (_has_bits_[0] & 0x00000008u) != 0;
  return value;
}
inline bool XayaPlayer::has_balance() const {
  return _internal_has_balance();
}
inline void XayaPlayer::clear_balance() {
  balance_ = PROTOBUF_ULONGLONG(0);
  _has_bits_[0] &= ~0x00000008u;
}
inline ::PROTOBUF_NAMESPACE_ID::uint64 XayaPlayer::_internal_balance() const {
  return balance_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint64 XayaPlayer::balance() const {
  // @@protoc_insertion_point(field_get:pxd.proto.XayaPlayer.balance)
  return _internal_balance();
}
inline void XayaPlayer::_internal_set_balance(::PROTOBUF_NAMESPACE_ID::uint64 value) {
  _has_bits_[0] |= 0x00000008u;
  balance_ = value;
}
inline void XayaPlayer::set_balance(::PROTOBUF_NAMESPACE_ID::uint64 value) {
  _internal_set_balance(value);
  // @@protoc_insertion_point(field_set:pxd.proto.XayaPlayer.balance)
}

// optional uint64 burnsale_balance = 4;
inline bool XayaPlayer::_internal_has_burnsale_balance() const {
  bool value = (_has_bits_[0] & 0x00000010u) != 0;
  return value;
}
inline bool XayaPlayer::has_burnsale_balance() const {
  return _internal_has_burnsale_balance();
}
inline void XayaPlayer::clear_burnsale_balance() {
  burnsale_balance_ = PROTOBUF_ULONGLONG(0);
  _has_bits_[0] &= ~0x00000010u;
}
inline ::PROTOBUF_NAMESPACE_ID::uint64 XayaPlayer::_internal_burnsale_balance() const {
  return burnsale_balance_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint64 XayaPlayer::burnsale_balance() const {
  // @@protoc_insertion_point(field_get:pxd.proto.XayaPlayer.burnsale_balance)
  return _internal_burnsale_balance();
}
inline void XayaPlayer::_internal_set_burnsale_balance(::PROTOBUF_NAMESPACE_ID::uint64 value) {
  _has_bits_[0] |= 0x00000010u;
  burnsale_balance_ = value;
}
inline void XayaPlayer::set_burnsale_balance(::PROTOBUF_NAMESPACE_ID::uint64 value) {
  _internal_set_burnsale_balance(value);
  // @@protoc_insertion_point(field_set:pxd.proto.XayaPlayer.burnsale_balance)
}

// optional string xayaname = 5;
inline bool XayaPlayer::_internal_has_xayaname() const {
  bool value = (_has_bits_[0] & 0x00000001u) != 0;
  return value;
}
inline bool XayaPlayer::has_xayaname() const {
  return _internal_has_xayaname();
}
inline void XayaPlayer::clear_xayaname() {
  xayaname_.ClearToEmpty();
  _has_bits_[0] &= ~0x00000001u;
}
inline const std::string& XayaPlayer::xayaname() const {
  // @@protoc_insertion_point(field_get:pxd.proto.XayaPlayer.xayaname)
  return _internal_xayaname();
}
template <typename ArgT0, typename... ArgT>
PROTOBUF_ALWAYS_INLINE
inline void XayaPlayer::set_xayaname(ArgT0&& arg0, ArgT... args) {
 _has_bits_[0] |= 0x00000001u;
 xayaname_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, static_cast<ArgT0 &&>(arg0), args..., GetArena());
  // @@protoc_insertion_point(field_set:pxd.proto.XayaPlayer.xayaname)
}
inline std::string* XayaPlayer::mutable_xayaname() {
  // @@protoc_insertion_point(field_mutable:pxd.proto.XayaPlayer.xayaname)
  return _internal_mutable_xayaname();
}
inline const std::string& XayaPlayer::_internal_xayaname() const {
  return xayaname_.Get();
}
inline void XayaPlayer::_internal_set_xayaname(const std::string& value) {
  _has_bits_[0] |= 0x00000001u;
  xayaname_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, value, GetArena());
}
inline std::string* XayaPlayer::_internal_mutable_xayaname() {
  _has_bits_[0] |= 0x00000001u;
  return xayaname_.Mutable(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, GetArena());
}
inline std::string* XayaPlayer::release_xayaname() {
  // @@protoc_insertion_point(field_release:pxd.proto.XayaPlayer.xayaname)
  if (!_internal_has_xayaname()) {
    return nullptr;
  }
  _has_bits_[0] &= ~0x00000001u;
  return xayaname_.ReleaseNonDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
}
inline void XayaPlayer::set_allocated_xayaname(std::string* xayaname) {
  if (xayaname != nullptr) {
    _has_bits_[0] |= 0x00000001u;
  } else {
    _has_bits_[0] &= ~0x00000001u;
  }
  xayaname_.SetAllocated(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), xayaname,
      GetArena());
  // @@protoc_insertion_point(field_set_allocated:pxd.proto.XayaPlayer.xayaname)
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)

}  // namespace proto
}  // namespace pxd

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_proto_2fxaya_5fplayer_2eproto

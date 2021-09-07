// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: proto/candy.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_proto_2fcandy_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_proto_2fcandy_2eproto

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
#define PROTOBUF_INTERNAL_EXPORT_proto_2fcandy_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_proto_2fcandy_2eproto {
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
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_proto_2fcandy_2eproto;
namespace pxd {
namespace proto {
class Candy;
struct CandyDefaultTypeInternal;
extern CandyDefaultTypeInternal _Candy_default_instance_;
}  // namespace proto
}  // namespace pxd
PROTOBUF_NAMESPACE_OPEN
template<> ::pxd::proto::Candy* Arena::CreateMaybeMessage<::pxd::proto::Candy>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace pxd {
namespace proto {

// ===================================================================

class Candy PROTOBUF_FINAL :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:pxd.proto.Candy) */ {
 public:
  inline Candy() : Candy(nullptr) {}
  ~Candy() override;
  explicit constexpr Candy(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  Candy(const Candy& from);
  Candy(Candy&& from) noexcept
    : Candy() {
    *this = ::std::move(from);
  }

  inline Candy& operator=(const Candy& from) {
    CopyFrom(from);
    return *this;
  }
  inline Candy& operator=(Candy&& from) noexcept {
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
  static const Candy& default_instance() {
    return *internal_default_instance();
  }
  static inline const Candy* internal_default_instance() {
    return reinterpret_cast<const Candy*>(
               &_Candy_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(Candy& a, Candy& b) {
    a.Swap(&b);
  }
  inline void Swap(Candy* other) {
    if (other == this) return;
    if (GetArena() == other->GetArena()) {
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(Candy* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetArena() == other->GetArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  inline Candy* New() const final {
    return CreateMaybeMessage<Candy>(nullptr);
  }

  Candy* New(::PROTOBUF_NAMESPACE_ID::Arena* arena) const final {
    return CreateMaybeMessage<Candy>(arena);
  }
  void CopyFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void MergeFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void CopyFrom(const Candy& from);
  void MergeFrom(const Candy& from);
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
  void InternalSwap(Candy* other);
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "pxd.proto.Candy";
  }
  protected:
  explicit Candy(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  private:
  static void ArenaDtor(void* object);
  inline void RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena* arena);
  public:

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kAuthoredIdFieldNumber = 1,
    kNameFieldNumber = 2,
  };
  // optional string AuthoredId = 1;
  bool has_authoredid() const;
  private:
  bool _internal_has_authoredid() const;
  public:
  void clear_authoredid();
  const std::string& authoredid() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_authoredid(ArgT0&& arg0, ArgT... args);
  std::string* mutable_authoredid();
  std::string* release_authoredid();
  void set_allocated_authoredid(std::string* authoredid);
  private:
  const std::string& _internal_authoredid() const;
  void _internal_set_authoredid(const std::string& value);
  std::string* _internal_mutable_authoredid();
  public:

  // optional string Name = 2;
  bool has_name() const;
  private:
  bool _internal_has_name() const;
  public:
  void clear_name();
  const std::string& name() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_name(ArgT0&& arg0, ArgT... args);
  std::string* mutable_name();
  std::string* release_name();
  void set_allocated_name(std::string* name);
  private:
  const std::string& _internal_name() const;
  void _internal_set_name(const std::string& value);
  std::string* _internal_mutable_name();
  public:

  // @@protoc_insertion_point(class_scope:pxd.proto.Candy)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  ::PROTOBUF_NAMESPACE_ID::internal::HasBits<1> _has_bits_;
  mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr authoredid_;
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr name_;
  friend struct ::TableStruct_proto_2fcandy_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// Candy

// optional string AuthoredId = 1;
inline bool Candy::_internal_has_authoredid() const {
  bool value = (_has_bits_[0] & 0x00000001u) != 0;
  return value;
}
inline bool Candy::has_authoredid() const {
  return _internal_has_authoredid();
}
inline void Candy::clear_authoredid() {
  authoredid_.ClearToEmpty();
  _has_bits_[0] &= ~0x00000001u;
}
inline const std::string& Candy::authoredid() const {
  // @@protoc_insertion_point(field_get:pxd.proto.Candy.AuthoredId)
  return _internal_authoredid();
}
template <typename ArgT0, typename... ArgT>
PROTOBUF_ALWAYS_INLINE
inline void Candy::set_authoredid(ArgT0&& arg0, ArgT... args) {
 _has_bits_[0] |= 0x00000001u;
 authoredid_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, static_cast<ArgT0 &&>(arg0), args..., GetArena());
  // @@protoc_insertion_point(field_set:pxd.proto.Candy.AuthoredId)
}
inline std::string* Candy::mutable_authoredid() {
  // @@protoc_insertion_point(field_mutable:pxd.proto.Candy.AuthoredId)
  return _internal_mutable_authoredid();
}
inline const std::string& Candy::_internal_authoredid() const {
  return authoredid_.Get();
}
inline void Candy::_internal_set_authoredid(const std::string& value) {
  _has_bits_[0] |= 0x00000001u;
  authoredid_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, value, GetArena());
}
inline std::string* Candy::_internal_mutable_authoredid() {
  _has_bits_[0] |= 0x00000001u;
  return authoredid_.Mutable(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, GetArena());
}
inline std::string* Candy::release_authoredid() {
  // @@protoc_insertion_point(field_release:pxd.proto.Candy.AuthoredId)
  if (!_internal_has_authoredid()) {
    return nullptr;
  }
  _has_bits_[0] &= ~0x00000001u;
  return authoredid_.ReleaseNonDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
}
inline void Candy::set_allocated_authoredid(std::string* authoredid) {
  if (authoredid != nullptr) {
    _has_bits_[0] |= 0x00000001u;
  } else {
    _has_bits_[0] &= ~0x00000001u;
  }
  authoredid_.SetAllocated(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), authoredid,
      GetArena());
  // @@protoc_insertion_point(field_set_allocated:pxd.proto.Candy.AuthoredId)
}

// optional string Name = 2;
inline bool Candy::_internal_has_name() const {
  bool value = (_has_bits_[0] & 0x00000002u) != 0;
  return value;
}
inline bool Candy::has_name() const {
  return _internal_has_name();
}
inline void Candy::clear_name() {
  name_.ClearToEmpty();
  _has_bits_[0] &= ~0x00000002u;
}
inline const std::string& Candy::name() const {
  // @@protoc_insertion_point(field_get:pxd.proto.Candy.Name)
  return _internal_name();
}
template <typename ArgT0, typename... ArgT>
PROTOBUF_ALWAYS_INLINE
inline void Candy::set_name(ArgT0&& arg0, ArgT... args) {
 _has_bits_[0] |= 0x00000002u;
 name_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, static_cast<ArgT0 &&>(arg0), args..., GetArena());
  // @@protoc_insertion_point(field_set:pxd.proto.Candy.Name)
}
inline std::string* Candy::mutable_name() {
  // @@protoc_insertion_point(field_mutable:pxd.proto.Candy.Name)
  return _internal_mutable_name();
}
inline const std::string& Candy::_internal_name() const {
  return name_.Get();
}
inline void Candy::_internal_set_name(const std::string& value) {
  _has_bits_[0] |= 0x00000002u;
  name_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, value, GetArena());
}
inline std::string* Candy::_internal_mutable_name() {
  _has_bits_[0] |= 0x00000002u;
  return name_.Mutable(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, GetArena());
}
inline std::string* Candy::release_name() {
  // @@protoc_insertion_point(field_release:pxd.proto.Candy.Name)
  if (!_internal_has_name()) {
    return nullptr;
  }
  _has_bits_[0] &= ~0x00000002u;
  return name_.ReleaseNonDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
}
inline void Candy::set_allocated_name(std::string* name) {
  if (name != nullptr) {
    _has_bits_[0] |= 0x00000002u;
  } else {
    _has_bits_[0] &= ~0x00000002u;
  }
  name_.SetAllocated(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), name,
      GetArena());
  // @@protoc_insertion_point(field_set_allocated:pxd.proto.Candy.Name)
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)

}  // namespace proto
}  // namespace pxd

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_proto_2fcandy_2eproto

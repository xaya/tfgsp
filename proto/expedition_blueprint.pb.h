// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: proto/expedition_blueprint.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_proto_2fexpedition_5fblueprint_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_proto_2fexpedition_5fblueprint_2eproto

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
#define PROTOBUF_INTERNAL_EXPORT_proto_2fexpedition_5fblueprint_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_proto_2fexpedition_5fblueprint_2eproto {
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
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_proto_2fexpedition_5fblueprint_2eproto;
namespace pxd {
namespace proto {
class ExpeditionBlueprint;
struct ExpeditionBlueprintDefaultTypeInternal;
extern ExpeditionBlueprintDefaultTypeInternal _ExpeditionBlueprint_default_instance_;
}  // namespace proto
}  // namespace pxd
PROTOBUF_NAMESPACE_OPEN
template<> ::pxd::proto::ExpeditionBlueprint* Arena::CreateMaybeMessage<::pxd::proto::ExpeditionBlueprint>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace pxd {
namespace proto {

// ===================================================================

class ExpeditionBlueprint PROTOBUF_FINAL :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:pxd.proto.ExpeditionBlueprint) */ {
 public:
  inline ExpeditionBlueprint() : ExpeditionBlueprint(nullptr) {}
  ~ExpeditionBlueprint() override;
  explicit constexpr ExpeditionBlueprint(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  ExpeditionBlueprint(const ExpeditionBlueprint& from);
  ExpeditionBlueprint(ExpeditionBlueprint&& from) noexcept
    : ExpeditionBlueprint() {
    *this = ::std::move(from);
  }

  inline ExpeditionBlueprint& operator=(const ExpeditionBlueprint& from) {
    CopyFrom(from);
    return *this;
  }
  inline ExpeditionBlueprint& operator=(ExpeditionBlueprint&& from) noexcept {
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
  static const ExpeditionBlueprint& default_instance() {
    return *internal_default_instance();
  }
  static inline const ExpeditionBlueprint* internal_default_instance() {
    return reinterpret_cast<const ExpeditionBlueprint*>(
               &_ExpeditionBlueprint_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(ExpeditionBlueprint& a, ExpeditionBlueprint& b) {
    a.Swap(&b);
  }
  inline void Swap(ExpeditionBlueprint* other) {
    if (other == this) return;
    if (GetArena() == other->GetArena()) {
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(ExpeditionBlueprint* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetArena() == other->GetArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  inline ExpeditionBlueprint* New() const final {
    return CreateMaybeMessage<ExpeditionBlueprint>(nullptr);
  }

  ExpeditionBlueprint* New(::PROTOBUF_NAMESPACE_ID::Arena* arena) const final {
    return CreateMaybeMessage<ExpeditionBlueprint>(arena);
  }
  void CopyFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void MergeFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) final;
  void CopyFrom(const ExpeditionBlueprint& from);
  void MergeFrom(const ExpeditionBlueprint& from);
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
  void InternalSwap(ExpeditionBlueprint* other);
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "pxd.proto.ExpeditionBlueprint";
  }
  protected:
  explicit ExpeditionBlueprint(::PROTOBUF_NAMESPACE_ID::Arena* arena);
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
    kBaseRewardsTableIdFieldNumber = 7,
    kDurationFieldNumber = 3,
    kMinSweetnessFieldNumber = 4,
    kMaxSweetnessFieldNumber = 5,
    kMaxRewardQualityFieldNumber = 6,
    kBaseRollCountFieldNumber = 8,
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

  // optional string BaseRewardsTableId = 7;
  bool has_baserewardstableid() const;
  private:
  bool _internal_has_baserewardstableid() const;
  public:
  void clear_baserewardstableid();
  const std::string& baserewardstableid() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_baserewardstableid(ArgT0&& arg0, ArgT... args);
  std::string* mutable_baserewardstableid();
  std::string* release_baserewardstableid();
  void set_allocated_baserewardstableid(std::string* baserewardstableid);
  private:
  const std::string& _internal_baserewardstableid() const;
  void _internal_set_baserewardstableid(const std::string& value);
  std::string* _internal_mutable_baserewardstableid();
  public:

  // optional int32 Duration = 3;
  bool has_duration() const;
  private:
  bool _internal_has_duration() const;
  public:
  void clear_duration();
  ::PROTOBUF_NAMESPACE_ID::int32 duration() const;
  void set_duration(::PROTOBUF_NAMESPACE_ID::int32 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::int32 _internal_duration() const;
  void _internal_set_duration(::PROTOBUF_NAMESPACE_ID::int32 value);
  public:

  // optional uint32 MinSweetness = 4;
  bool has_minsweetness() const;
  private:
  bool _internal_has_minsweetness() const;
  public:
  void clear_minsweetness();
  ::PROTOBUF_NAMESPACE_ID::uint32 minsweetness() const;
  void set_minsweetness(::PROTOBUF_NAMESPACE_ID::uint32 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint32 _internal_minsweetness() const;
  void _internal_set_minsweetness(::PROTOBUF_NAMESPACE_ID::uint32 value);
  public:

  // optional uint32 MaxSweetness = 5;
  bool has_maxsweetness() const;
  private:
  bool _internal_has_maxsweetness() const;
  public:
  void clear_maxsweetness();
  ::PROTOBUF_NAMESPACE_ID::uint32 maxsweetness() const;
  void set_maxsweetness(::PROTOBUF_NAMESPACE_ID::uint32 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint32 _internal_maxsweetness() const;
  void _internal_set_maxsweetness(::PROTOBUF_NAMESPACE_ID::uint32 value);
  public:

  // optional uint32 MaxRewardQuality = 6;
  bool has_maxrewardquality() const;
  private:
  bool _internal_has_maxrewardquality() const;
  public:
  void clear_maxrewardquality();
  ::PROTOBUF_NAMESPACE_ID::uint32 maxrewardquality() const;
  void set_maxrewardquality(::PROTOBUF_NAMESPACE_ID::uint32 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::uint32 _internal_maxrewardquality() const;
  void _internal_set_maxrewardquality(::PROTOBUF_NAMESPACE_ID::uint32 value);
  public:

  // optional int32 BaseRollCount = 8;
  bool has_baserollcount() const;
  private:
  bool _internal_has_baserollcount() const;
  public:
  void clear_baserollcount();
  ::PROTOBUF_NAMESPACE_ID::int32 baserollcount() const;
  void set_baserollcount(::PROTOBUF_NAMESPACE_ID::int32 value);
  private:
  ::PROTOBUF_NAMESPACE_ID::int32 _internal_baserollcount() const;
  void _internal_set_baserollcount(::PROTOBUF_NAMESPACE_ID::int32 value);
  public:

  // @@protoc_insertion_point(class_scope:pxd.proto.ExpeditionBlueprint)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  ::PROTOBUF_NAMESPACE_ID::internal::HasBits<1> _has_bits_;
  mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr authoredid_;
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr name_;
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr baserewardstableid_;
  ::PROTOBUF_NAMESPACE_ID::int32 duration_;
  ::PROTOBUF_NAMESPACE_ID::uint32 minsweetness_;
  ::PROTOBUF_NAMESPACE_ID::uint32 maxsweetness_;
  ::PROTOBUF_NAMESPACE_ID::uint32 maxrewardquality_;
  ::PROTOBUF_NAMESPACE_ID::int32 baserollcount_;
  friend struct ::TableStruct_proto_2fexpedition_5fblueprint_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// ExpeditionBlueprint

// optional string AuthoredId = 1;
inline bool ExpeditionBlueprint::_internal_has_authoredid() const {
  bool value = (_has_bits_[0] & 0x00000001u) != 0;
  return value;
}
inline bool ExpeditionBlueprint::has_authoredid() const {
  return _internal_has_authoredid();
}
inline void ExpeditionBlueprint::clear_authoredid() {
  authoredid_.ClearToEmpty();
  _has_bits_[0] &= ~0x00000001u;
}
inline const std::string& ExpeditionBlueprint::authoredid() const {
  // @@protoc_insertion_point(field_get:pxd.proto.ExpeditionBlueprint.AuthoredId)
  return _internal_authoredid();
}
template <typename ArgT0, typename... ArgT>
PROTOBUF_ALWAYS_INLINE
inline void ExpeditionBlueprint::set_authoredid(ArgT0&& arg0, ArgT... args) {
 _has_bits_[0] |= 0x00000001u;
 authoredid_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, static_cast<ArgT0 &&>(arg0), args..., GetArena());
  // @@protoc_insertion_point(field_set:pxd.proto.ExpeditionBlueprint.AuthoredId)
}
inline std::string* ExpeditionBlueprint::mutable_authoredid() {
  // @@protoc_insertion_point(field_mutable:pxd.proto.ExpeditionBlueprint.AuthoredId)
  return _internal_mutable_authoredid();
}
inline const std::string& ExpeditionBlueprint::_internal_authoredid() const {
  return authoredid_.Get();
}
inline void ExpeditionBlueprint::_internal_set_authoredid(const std::string& value) {
  _has_bits_[0] |= 0x00000001u;
  authoredid_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, value, GetArena());
}
inline std::string* ExpeditionBlueprint::_internal_mutable_authoredid() {
  _has_bits_[0] |= 0x00000001u;
  return authoredid_.Mutable(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, GetArena());
}
inline std::string* ExpeditionBlueprint::release_authoredid() {
  // @@protoc_insertion_point(field_release:pxd.proto.ExpeditionBlueprint.AuthoredId)
  if (!_internal_has_authoredid()) {
    return nullptr;
  }
  _has_bits_[0] &= ~0x00000001u;
  return authoredid_.ReleaseNonDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
}
inline void ExpeditionBlueprint::set_allocated_authoredid(std::string* authoredid) {
  if (authoredid != nullptr) {
    _has_bits_[0] |= 0x00000001u;
  } else {
    _has_bits_[0] &= ~0x00000001u;
  }
  authoredid_.SetAllocated(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), authoredid,
      GetArena());
  // @@protoc_insertion_point(field_set_allocated:pxd.proto.ExpeditionBlueprint.AuthoredId)
}

// optional string Name = 2;
inline bool ExpeditionBlueprint::_internal_has_name() const {
  bool value = (_has_bits_[0] & 0x00000002u) != 0;
  return value;
}
inline bool ExpeditionBlueprint::has_name() const {
  return _internal_has_name();
}
inline void ExpeditionBlueprint::clear_name() {
  name_.ClearToEmpty();
  _has_bits_[0] &= ~0x00000002u;
}
inline const std::string& ExpeditionBlueprint::name() const {
  // @@protoc_insertion_point(field_get:pxd.proto.ExpeditionBlueprint.Name)
  return _internal_name();
}
template <typename ArgT0, typename... ArgT>
PROTOBUF_ALWAYS_INLINE
inline void ExpeditionBlueprint::set_name(ArgT0&& arg0, ArgT... args) {
 _has_bits_[0] |= 0x00000002u;
 name_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, static_cast<ArgT0 &&>(arg0), args..., GetArena());
  // @@protoc_insertion_point(field_set:pxd.proto.ExpeditionBlueprint.Name)
}
inline std::string* ExpeditionBlueprint::mutable_name() {
  // @@protoc_insertion_point(field_mutable:pxd.proto.ExpeditionBlueprint.Name)
  return _internal_mutable_name();
}
inline const std::string& ExpeditionBlueprint::_internal_name() const {
  return name_.Get();
}
inline void ExpeditionBlueprint::_internal_set_name(const std::string& value) {
  _has_bits_[0] |= 0x00000002u;
  name_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, value, GetArena());
}
inline std::string* ExpeditionBlueprint::_internal_mutable_name() {
  _has_bits_[0] |= 0x00000002u;
  return name_.Mutable(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, GetArena());
}
inline std::string* ExpeditionBlueprint::release_name() {
  // @@protoc_insertion_point(field_release:pxd.proto.ExpeditionBlueprint.Name)
  if (!_internal_has_name()) {
    return nullptr;
  }
  _has_bits_[0] &= ~0x00000002u;
  return name_.ReleaseNonDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
}
inline void ExpeditionBlueprint::set_allocated_name(std::string* name) {
  if (name != nullptr) {
    _has_bits_[0] |= 0x00000002u;
  } else {
    _has_bits_[0] &= ~0x00000002u;
  }
  name_.SetAllocated(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), name,
      GetArena());
  // @@protoc_insertion_point(field_set_allocated:pxd.proto.ExpeditionBlueprint.Name)
}

// optional int32 Duration = 3;
inline bool ExpeditionBlueprint::_internal_has_duration() const {
  bool value = (_has_bits_[0] & 0x00000008u) != 0;
  return value;
}
inline bool ExpeditionBlueprint::has_duration() const {
  return _internal_has_duration();
}
inline void ExpeditionBlueprint::clear_duration() {
  duration_ = 0;
  _has_bits_[0] &= ~0x00000008u;
}
inline ::PROTOBUF_NAMESPACE_ID::int32 ExpeditionBlueprint::_internal_duration() const {
  return duration_;
}
inline ::PROTOBUF_NAMESPACE_ID::int32 ExpeditionBlueprint::duration() const {
  // @@protoc_insertion_point(field_get:pxd.proto.ExpeditionBlueprint.Duration)
  return _internal_duration();
}
inline void ExpeditionBlueprint::_internal_set_duration(::PROTOBUF_NAMESPACE_ID::int32 value) {
  _has_bits_[0] |= 0x00000008u;
  duration_ = value;
}
inline void ExpeditionBlueprint::set_duration(::PROTOBUF_NAMESPACE_ID::int32 value) {
  _internal_set_duration(value);
  // @@protoc_insertion_point(field_set:pxd.proto.ExpeditionBlueprint.Duration)
}

// optional uint32 MinSweetness = 4;
inline bool ExpeditionBlueprint::_internal_has_minsweetness() const {
  bool value = (_has_bits_[0] & 0x00000010u) != 0;
  return value;
}
inline bool ExpeditionBlueprint::has_minsweetness() const {
  return _internal_has_minsweetness();
}
inline void ExpeditionBlueprint::clear_minsweetness() {
  minsweetness_ = 0u;
  _has_bits_[0] &= ~0x00000010u;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 ExpeditionBlueprint::_internal_minsweetness() const {
  return minsweetness_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 ExpeditionBlueprint::minsweetness() const {
  // @@protoc_insertion_point(field_get:pxd.proto.ExpeditionBlueprint.MinSweetness)
  return _internal_minsweetness();
}
inline void ExpeditionBlueprint::_internal_set_minsweetness(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _has_bits_[0] |= 0x00000010u;
  minsweetness_ = value;
}
inline void ExpeditionBlueprint::set_minsweetness(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _internal_set_minsweetness(value);
  // @@protoc_insertion_point(field_set:pxd.proto.ExpeditionBlueprint.MinSweetness)
}

// optional uint32 MaxSweetness = 5;
inline bool ExpeditionBlueprint::_internal_has_maxsweetness() const {
  bool value = (_has_bits_[0] & 0x00000020u) != 0;
  return value;
}
inline bool ExpeditionBlueprint::has_maxsweetness() const {
  return _internal_has_maxsweetness();
}
inline void ExpeditionBlueprint::clear_maxsweetness() {
  maxsweetness_ = 0u;
  _has_bits_[0] &= ~0x00000020u;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 ExpeditionBlueprint::_internal_maxsweetness() const {
  return maxsweetness_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 ExpeditionBlueprint::maxsweetness() const {
  // @@protoc_insertion_point(field_get:pxd.proto.ExpeditionBlueprint.MaxSweetness)
  return _internal_maxsweetness();
}
inline void ExpeditionBlueprint::_internal_set_maxsweetness(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _has_bits_[0] |= 0x00000020u;
  maxsweetness_ = value;
}
inline void ExpeditionBlueprint::set_maxsweetness(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _internal_set_maxsweetness(value);
  // @@protoc_insertion_point(field_set:pxd.proto.ExpeditionBlueprint.MaxSweetness)
}

// optional uint32 MaxRewardQuality = 6;
inline bool ExpeditionBlueprint::_internal_has_maxrewardquality() const {
  bool value = (_has_bits_[0] & 0x00000040u) != 0;
  return value;
}
inline bool ExpeditionBlueprint::has_maxrewardquality() const {
  return _internal_has_maxrewardquality();
}
inline void ExpeditionBlueprint::clear_maxrewardquality() {
  maxrewardquality_ = 0u;
  _has_bits_[0] &= ~0x00000040u;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 ExpeditionBlueprint::_internal_maxrewardquality() const {
  return maxrewardquality_;
}
inline ::PROTOBUF_NAMESPACE_ID::uint32 ExpeditionBlueprint::maxrewardquality() const {
  // @@protoc_insertion_point(field_get:pxd.proto.ExpeditionBlueprint.MaxRewardQuality)
  return _internal_maxrewardquality();
}
inline void ExpeditionBlueprint::_internal_set_maxrewardquality(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _has_bits_[0] |= 0x00000040u;
  maxrewardquality_ = value;
}
inline void ExpeditionBlueprint::set_maxrewardquality(::PROTOBUF_NAMESPACE_ID::uint32 value) {
  _internal_set_maxrewardquality(value);
  // @@protoc_insertion_point(field_set:pxd.proto.ExpeditionBlueprint.MaxRewardQuality)
}

// optional string BaseRewardsTableId = 7;
inline bool ExpeditionBlueprint::_internal_has_baserewardstableid() const {
  bool value = (_has_bits_[0] & 0x00000004u) != 0;
  return value;
}
inline bool ExpeditionBlueprint::has_baserewardstableid() const {
  return _internal_has_baserewardstableid();
}
inline void ExpeditionBlueprint::clear_baserewardstableid() {
  baserewardstableid_.ClearToEmpty();
  _has_bits_[0] &= ~0x00000004u;
}
inline const std::string& ExpeditionBlueprint::baserewardstableid() const {
  // @@protoc_insertion_point(field_get:pxd.proto.ExpeditionBlueprint.BaseRewardsTableId)
  return _internal_baserewardstableid();
}
template <typename ArgT0, typename... ArgT>
PROTOBUF_ALWAYS_INLINE
inline void ExpeditionBlueprint::set_baserewardstableid(ArgT0&& arg0, ArgT... args) {
 _has_bits_[0] |= 0x00000004u;
 baserewardstableid_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, static_cast<ArgT0 &&>(arg0), args..., GetArena());
  // @@protoc_insertion_point(field_set:pxd.proto.ExpeditionBlueprint.BaseRewardsTableId)
}
inline std::string* ExpeditionBlueprint::mutable_baserewardstableid() {
  // @@protoc_insertion_point(field_mutable:pxd.proto.ExpeditionBlueprint.BaseRewardsTableId)
  return _internal_mutable_baserewardstableid();
}
inline const std::string& ExpeditionBlueprint::_internal_baserewardstableid() const {
  return baserewardstableid_.Get();
}
inline void ExpeditionBlueprint::_internal_set_baserewardstableid(const std::string& value) {
  _has_bits_[0] |= 0x00000004u;
  baserewardstableid_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, value, GetArena());
}
inline std::string* ExpeditionBlueprint::_internal_mutable_baserewardstableid() {
  _has_bits_[0] |= 0x00000004u;
  return baserewardstableid_.Mutable(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, GetArena());
}
inline std::string* ExpeditionBlueprint::release_baserewardstableid() {
  // @@protoc_insertion_point(field_release:pxd.proto.ExpeditionBlueprint.BaseRewardsTableId)
  if (!_internal_has_baserewardstableid()) {
    return nullptr;
  }
  _has_bits_[0] &= ~0x00000004u;
  return baserewardstableid_.ReleaseNonDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
}
inline void ExpeditionBlueprint::set_allocated_baserewardstableid(std::string* baserewardstableid) {
  if (baserewardstableid != nullptr) {
    _has_bits_[0] |= 0x00000004u;
  } else {
    _has_bits_[0] &= ~0x00000004u;
  }
  baserewardstableid_.SetAllocated(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), baserewardstableid,
      GetArena());
  // @@protoc_insertion_point(field_set_allocated:pxd.proto.ExpeditionBlueprint.BaseRewardsTableId)
}

// optional int32 BaseRollCount = 8;
inline bool ExpeditionBlueprint::_internal_has_baserollcount() const {
  bool value = (_has_bits_[0] & 0x00000080u) != 0;
  return value;
}
inline bool ExpeditionBlueprint::has_baserollcount() const {
  return _internal_has_baserollcount();
}
inline void ExpeditionBlueprint::clear_baserollcount() {
  baserollcount_ = 0;
  _has_bits_[0] &= ~0x00000080u;
}
inline ::PROTOBUF_NAMESPACE_ID::int32 ExpeditionBlueprint::_internal_baserollcount() const {
  return baserollcount_;
}
inline ::PROTOBUF_NAMESPACE_ID::int32 ExpeditionBlueprint::baserollcount() const {
  // @@protoc_insertion_point(field_get:pxd.proto.ExpeditionBlueprint.BaseRollCount)
  return _internal_baserollcount();
}
inline void ExpeditionBlueprint::_internal_set_baserollcount(::PROTOBUF_NAMESPACE_ID::int32 value) {
  _has_bits_[0] |= 0x00000080u;
  baserollcount_ = value;
}
inline void ExpeditionBlueprint::set_baserollcount(::PROTOBUF_NAMESPACE_ID::int32 value) {
  _internal_set_baserollcount(value);
  // @@protoc_insertion_point(field_set:pxd.proto.ExpeditionBlueprint.BaseRollCount)
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)

}  // namespace proto
}  // namespace pxd

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_proto_2fexpedition_5fblueprint_2eproto

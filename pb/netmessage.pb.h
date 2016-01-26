// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: netmessage.proto

#ifndef PROTOBUF_netmessage_2eproto__INCLUDED
#define PROTOBUF_netmessage_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 3000000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 3000000 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)

// Internal implementation detail -- do not call these.
void protobuf_AddDesc_netmessage_2eproto();
void protobuf_AssignDesc_netmessage_2eproto();
void protobuf_ShutdownFile_netmessage_2eproto();

class CP1;
class CP2;
class CProto;
class EchoProto;

// ===================================================================

class CProto : public ::google::protobuf::Message {
 public:
  CProto();
  virtual ~CProto();

  CProto(const CProto& from);

  inline CProto& operator=(const CProto& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _internal_metadata_.unknown_fields();
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return _internal_metadata_.mutable_unknown_fields();
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const CProto& default_instance();

  void Swap(CProto* other);

  // implements Message ----------------------------------------------

  inline CProto* New() const { return New(NULL); }

  CProto* New(::google::protobuf::Arena* arena) const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const CProto& from);
  void MergeFrom(const CProto& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  void InternalSwap(CProto* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return _internal_metadata_.arena();
  }
  inline void* MaybeArenaPtr() const {
    return _internal_metadata_.raw_arena_ptr();
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // required int32 id = 1;
  bool has_id() const;
  void clear_id();
  static const int kIdFieldNumber = 1;
  ::google::protobuf::int32 id() const;
  void set_id(::google::protobuf::int32 value);

  // required string body = 2;
  bool has_body() const;
  void clear_body();
  static const int kBodyFieldNumber = 2;
  const ::std::string& body() const;
  void set_body(const ::std::string& value);
  void set_body(const char* value);
  void set_body(const char* value, size_t size);
  ::std::string* mutable_body();
  ::std::string* release_body();
  void set_allocated_body(::std::string* body);

  // @@protoc_insertion_point(class_scope:CProto)
 private:
  inline void set_has_id();
  inline void clear_has_id();
  inline void set_has_body();
  inline void clear_has_body();

  // helper for ByteSize()
  int RequiredFieldsByteSizeFallback() const;

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  ::google::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::google::protobuf::internal::ArenaStringPtr body_;
  ::google::protobuf::int32 id_;
  friend void  protobuf_AddDesc_netmessage_2eproto();
  friend void protobuf_AssignDesc_netmessage_2eproto();
  friend void protobuf_ShutdownFile_netmessage_2eproto();

  void InitAsDefaultInstance();
  static CProto* default_instance_;
};
// -------------------------------------------------------------------

class CP1 : public ::google::protobuf::Message {
 public:
  CP1();
  virtual ~CP1();

  CP1(const CP1& from);

  inline CP1& operator=(const CP1& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _internal_metadata_.unknown_fields();
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return _internal_metadata_.mutable_unknown_fields();
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const CP1& default_instance();

  void Swap(CP1* other);

  // implements Message ----------------------------------------------

  inline CP1* New() const { return New(NULL); }

  CP1* New(::google::protobuf::Arena* arena) const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const CP1& from);
  void MergeFrom(const CP1& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  void InternalSwap(CP1* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return _internal_metadata_.arena();
  }
  inline void* MaybeArenaPtr() const {
    return _internal_metadata_.raw_arena_ptr();
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // required int32 a = 1;
  bool has_a() const;
  void clear_a();
  static const int kAFieldNumber = 1;
  ::google::protobuf::int32 a() const;
  void set_a(::google::protobuf::int32 value);

  // required int64 b = 2;
  bool has_b() const;
  void clear_b();
  static const int kBFieldNumber = 2;
  ::google::protobuf::int64 b() const;
  void set_b(::google::protobuf::int64 value);

  // required string c = 3;
  bool has_c() const;
  void clear_c();
  static const int kCFieldNumber = 3;
  const ::std::string& c() const;
  void set_c(const ::std::string& value);
  void set_c(const char* value);
  void set_c(const char* value, size_t size);
  ::std::string* mutable_c();
  ::std::string* release_c();
  void set_allocated_c(::std::string* c);

  // @@protoc_insertion_point(class_scope:CP1)
 private:
  inline void set_has_a();
  inline void clear_has_a();
  inline void set_has_b();
  inline void clear_has_b();
  inline void set_has_c();
  inline void clear_has_c();

  // helper for ByteSize()
  int RequiredFieldsByteSizeFallback() const;

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  ::google::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::google::protobuf::int64 b_;
  ::google::protobuf::internal::ArenaStringPtr c_;
  ::google::protobuf::int32 a_;
  friend void  protobuf_AddDesc_netmessage_2eproto();
  friend void protobuf_AssignDesc_netmessage_2eproto();
  friend void protobuf_ShutdownFile_netmessage_2eproto();

  void InitAsDefaultInstance();
  static CP1* default_instance_;
};
// -------------------------------------------------------------------

class CP2 : public ::google::protobuf::Message {
 public:
  CP2();
  virtual ~CP2();

  CP2(const CP2& from);

  inline CP2& operator=(const CP2& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _internal_metadata_.unknown_fields();
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return _internal_metadata_.mutable_unknown_fields();
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const CP2& default_instance();

  void Swap(CP2* other);

  // implements Message ----------------------------------------------

  inline CP2* New() const { return New(NULL); }

  CP2* New(::google::protobuf::Arena* arena) const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const CP2& from);
  void MergeFrom(const CP2& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  void InternalSwap(CP2* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return _internal_metadata_.arena();
  }
  inline void* MaybeArenaPtr() const {
    return _internal_metadata_.raw_arena_ptr();
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // required string a = 1;
  bool has_a() const;
  void clear_a();
  static const int kAFieldNumber = 1;
  const ::std::string& a() const;
  void set_a(const ::std::string& value);
  void set_a(const char* value);
  void set_a(const char* value, size_t size);
  ::std::string* mutable_a();
  ::std::string* release_a();
  void set_allocated_a(::std::string* a);

  // required string b = 2;
  bool has_b() const;
  void clear_b();
  static const int kBFieldNumber = 2;
  const ::std::string& b() const;
  void set_b(const ::std::string& value);
  void set_b(const char* value);
  void set_b(const char* value, size_t size);
  ::std::string* mutable_b();
  ::std::string* release_b();
  void set_allocated_b(::std::string* b);

  // required int64 c = 3;
  bool has_c() const;
  void clear_c();
  static const int kCFieldNumber = 3;
  ::google::protobuf::int64 c() const;
  void set_c(::google::protobuf::int64 value);

  // @@protoc_insertion_point(class_scope:CP2)
 private:
  inline void set_has_a();
  inline void clear_has_a();
  inline void set_has_b();
  inline void clear_has_b();
  inline void set_has_c();
  inline void clear_has_c();

  // helper for ByteSize()
  int RequiredFieldsByteSizeFallback() const;

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  ::google::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::google::protobuf::internal::ArenaStringPtr a_;
  ::google::protobuf::internal::ArenaStringPtr b_;
  ::google::protobuf::int64 c_;
  friend void  protobuf_AddDesc_netmessage_2eproto();
  friend void protobuf_AssignDesc_netmessage_2eproto();
  friend void protobuf_ShutdownFile_netmessage_2eproto();

  void InitAsDefaultInstance();
  static CP2* default_instance_;
};
// -------------------------------------------------------------------

class EchoProto : public ::google::protobuf::Message {
 public:
  EchoProto();
  virtual ~EchoProto();

  EchoProto(const EchoProto& from);

  inline EchoProto& operator=(const EchoProto& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _internal_metadata_.unknown_fields();
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return _internal_metadata_.mutable_unknown_fields();
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const EchoProto& default_instance();

  void Swap(EchoProto* other);

  // implements Message ----------------------------------------------

  inline EchoProto* New() const { return New(NULL); }

  EchoProto* New(::google::protobuf::Arena* arena) const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const EchoProto& from);
  void MergeFrom(const EchoProto& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  void InternalSwap(EchoProto* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return _internal_metadata_.arena();
  }
  inline void* MaybeArenaPtr() const {
    return _internal_metadata_.raw_arena_ptr();
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // required string data = 1;
  bool has_data() const;
  void clear_data();
  static const int kDataFieldNumber = 1;
  const ::std::string& data() const;
  void set_data(const ::std::string& value);
  void set_data(const char* value);
  void set_data(const char* value, size_t size);
  ::std::string* mutable_data();
  ::std::string* release_data();
  void set_allocated_data(::std::string* data);

  // @@protoc_insertion_point(class_scope:EchoProto)
 private:
  inline void set_has_data();
  inline void clear_has_data();

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  ::google::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::google::protobuf::internal::ArenaStringPtr data_;
  friend void  protobuf_AddDesc_netmessage_2eproto();
  friend void protobuf_AssignDesc_netmessage_2eproto();
  friend void protobuf_ShutdownFile_netmessage_2eproto();

  void InitAsDefaultInstance();
  static EchoProto* default_instance_;
};
// ===================================================================


// ===================================================================

#if !PROTOBUF_INLINE_NOT_IN_HEADERS
// CProto

// required int32 id = 1;
inline bool CProto::has_id() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void CProto::set_has_id() {
  _has_bits_[0] |= 0x00000001u;
}
inline void CProto::clear_has_id() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void CProto::clear_id() {
  id_ = 0;
  clear_has_id();
}
inline ::google::protobuf::int32 CProto::id() const {
  // @@protoc_insertion_point(field_get:CProto.id)
  return id_;
}
inline void CProto::set_id(::google::protobuf::int32 value) {
  set_has_id();
  id_ = value;
  // @@protoc_insertion_point(field_set:CProto.id)
}

// required string body = 2;
inline bool CProto::has_body() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void CProto::set_has_body() {
  _has_bits_[0] |= 0x00000002u;
}
inline void CProto::clear_has_body() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void CProto::clear_body() {
  body_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  clear_has_body();
}
inline const ::std::string& CProto::body() const {
  // @@protoc_insertion_point(field_get:CProto.body)
  return body_.GetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void CProto::set_body(const ::std::string& value) {
  set_has_body();
  body_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:CProto.body)
}
inline void CProto::set_body(const char* value) {
  set_has_body();
  body_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:CProto.body)
}
inline void CProto::set_body(const char* value, size_t size) {
  set_has_body();
  body_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:CProto.body)
}
inline ::std::string* CProto::mutable_body() {
  set_has_body();
  // @@protoc_insertion_point(field_mutable:CProto.body)
  return body_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* CProto::release_body() {
  clear_has_body();
  return body_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void CProto::set_allocated_body(::std::string* body) {
  if (body != NULL) {
    set_has_body();
  } else {
    clear_has_body();
  }
  body_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), body);
  // @@protoc_insertion_point(field_set_allocated:CProto.body)
}

// -------------------------------------------------------------------

// CP1

// required int32 a = 1;
inline bool CP1::has_a() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void CP1::set_has_a() {
  _has_bits_[0] |= 0x00000001u;
}
inline void CP1::clear_has_a() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void CP1::clear_a() {
  a_ = 0;
  clear_has_a();
}
inline ::google::protobuf::int32 CP1::a() const {
  // @@protoc_insertion_point(field_get:CP1.a)
  return a_;
}
inline void CP1::set_a(::google::protobuf::int32 value) {
  set_has_a();
  a_ = value;
  // @@protoc_insertion_point(field_set:CP1.a)
}

// required int64 b = 2;
inline bool CP1::has_b() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void CP1::set_has_b() {
  _has_bits_[0] |= 0x00000002u;
}
inline void CP1::clear_has_b() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void CP1::clear_b() {
  b_ = GOOGLE_LONGLONG(0);
  clear_has_b();
}
inline ::google::protobuf::int64 CP1::b() const {
  // @@protoc_insertion_point(field_get:CP1.b)
  return b_;
}
inline void CP1::set_b(::google::protobuf::int64 value) {
  set_has_b();
  b_ = value;
  // @@protoc_insertion_point(field_set:CP1.b)
}

// required string c = 3;
inline bool CP1::has_c() const {
  return (_has_bits_[0] & 0x00000004u) != 0;
}
inline void CP1::set_has_c() {
  _has_bits_[0] |= 0x00000004u;
}
inline void CP1::clear_has_c() {
  _has_bits_[0] &= ~0x00000004u;
}
inline void CP1::clear_c() {
  c_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  clear_has_c();
}
inline const ::std::string& CP1::c() const {
  // @@protoc_insertion_point(field_get:CP1.c)
  return c_.GetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void CP1::set_c(const ::std::string& value) {
  set_has_c();
  c_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:CP1.c)
}
inline void CP1::set_c(const char* value) {
  set_has_c();
  c_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:CP1.c)
}
inline void CP1::set_c(const char* value, size_t size) {
  set_has_c();
  c_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:CP1.c)
}
inline ::std::string* CP1::mutable_c() {
  set_has_c();
  // @@protoc_insertion_point(field_mutable:CP1.c)
  return c_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* CP1::release_c() {
  clear_has_c();
  return c_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void CP1::set_allocated_c(::std::string* c) {
  if (c != NULL) {
    set_has_c();
  } else {
    clear_has_c();
  }
  c_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), c);
  // @@protoc_insertion_point(field_set_allocated:CP1.c)
}

// -------------------------------------------------------------------

// CP2

// required string a = 1;
inline bool CP2::has_a() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void CP2::set_has_a() {
  _has_bits_[0] |= 0x00000001u;
}
inline void CP2::clear_has_a() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void CP2::clear_a() {
  a_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  clear_has_a();
}
inline const ::std::string& CP2::a() const {
  // @@protoc_insertion_point(field_get:CP2.a)
  return a_.GetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void CP2::set_a(const ::std::string& value) {
  set_has_a();
  a_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:CP2.a)
}
inline void CP2::set_a(const char* value) {
  set_has_a();
  a_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:CP2.a)
}
inline void CP2::set_a(const char* value, size_t size) {
  set_has_a();
  a_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:CP2.a)
}
inline ::std::string* CP2::mutable_a() {
  set_has_a();
  // @@protoc_insertion_point(field_mutable:CP2.a)
  return a_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* CP2::release_a() {
  clear_has_a();
  return a_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void CP2::set_allocated_a(::std::string* a) {
  if (a != NULL) {
    set_has_a();
  } else {
    clear_has_a();
  }
  a_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), a);
  // @@protoc_insertion_point(field_set_allocated:CP2.a)
}

// required string b = 2;
inline bool CP2::has_b() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void CP2::set_has_b() {
  _has_bits_[0] |= 0x00000002u;
}
inline void CP2::clear_has_b() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void CP2::clear_b() {
  b_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  clear_has_b();
}
inline const ::std::string& CP2::b() const {
  // @@protoc_insertion_point(field_get:CP2.b)
  return b_.GetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void CP2::set_b(const ::std::string& value) {
  set_has_b();
  b_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:CP2.b)
}
inline void CP2::set_b(const char* value) {
  set_has_b();
  b_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:CP2.b)
}
inline void CP2::set_b(const char* value, size_t size) {
  set_has_b();
  b_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:CP2.b)
}
inline ::std::string* CP2::mutable_b() {
  set_has_b();
  // @@protoc_insertion_point(field_mutable:CP2.b)
  return b_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* CP2::release_b() {
  clear_has_b();
  return b_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void CP2::set_allocated_b(::std::string* b) {
  if (b != NULL) {
    set_has_b();
  } else {
    clear_has_b();
  }
  b_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), b);
  // @@protoc_insertion_point(field_set_allocated:CP2.b)
}

// required int64 c = 3;
inline bool CP2::has_c() const {
  return (_has_bits_[0] & 0x00000004u) != 0;
}
inline void CP2::set_has_c() {
  _has_bits_[0] |= 0x00000004u;
}
inline void CP2::clear_has_c() {
  _has_bits_[0] &= ~0x00000004u;
}
inline void CP2::clear_c() {
  c_ = GOOGLE_LONGLONG(0);
  clear_has_c();
}
inline ::google::protobuf::int64 CP2::c() const {
  // @@protoc_insertion_point(field_get:CP2.c)
  return c_;
}
inline void CP2::set_c(::google::protobuf::int64 value) {
  set_has_c();
  c_ = value;
  // @@protoc_insertion_point(field_set:CP2.c)
}

// -------------------------------------------------------------------

// EchoProto

// required string data = 1;
inline bool EchoProto::has_data() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void EchoProto::set_has_data() {
  _has_bits_[0] |= 0x00000001u;
}
inline void EchoProto::clear_has_data() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void EchoProto::clear_data() {
  data_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  clear_has_data();
}
inline const ::std::string& EchoProto::data() const {
  // @@protoc_insertion_point(field_get:EchoProto.data)
  return data_.GetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void EchoProto::set_data(const ::std::string& value) {
  set_has_data();
  data_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:EchoProto.data)
}
inline void EchoProto::set_data(const char* value) {
  set_has_data();
  data_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:EchoProto.data)
}
inline void EchoProto::set_data(const char* value, size_t size) {
  set_has_data();
  data_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:EchoProto.data)
}
inline ::std::string* EchoProto::mutable_data() {
  set_has_data();
  // @@protoc_insertion_point(field_mutable:EchoProto.data)
  return data_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* EchoProto::release_data() {
  clear_has_data();
  return data_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void EchoProto::set_allocated_data(::std::string* data) {
  if (data != NULL) {
    set_has_data();
  } else {
    clear_has_data();
  }
  data_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), data);
  // @@protoc_insertion_point(field_set_allocated:EchoProto.data)
}

#endif  // !PROTOBUF_INLINE_NOT_IN_HEADERS
// -------------------------------------------------------------------

// -------------------------------------------------------------------

// -------------------------------------------------------------------


// @@protoc_insertion_point(namespace_scope)

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_netmessage_2eproto__INCLUDED
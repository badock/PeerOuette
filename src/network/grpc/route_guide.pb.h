// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: route_guide.proto

#ifndef PROTOBUF_INCLUDED_route_5fguide_2eproto
#define PROTOBUF_INCLUDED_route_5fguide_2eproto

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 3006001
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 3006001 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_table_driven.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/inlined_string_field.h>
#include <google/protobuf/metadata.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#define PROTOBUF_INTERNAL_EXPORT_protobuf_route_5fguide_2eproto 

namespace protobuf_route_5fguide_2eproto {
// Internal implementation detail -- do not use these members.
struct TableStruct {
  static const ::google::protobuf::internal::ParseTableField entries[];
  static const ::google::protobuf::internal::AuxillaryParseTableField aux[];
  static const ::google::protobuf::internal::ParseTable schema[2];
  static const ::google::protobuf::internal::FieldMetadata field_metadata[];
  static const ::google::protobuf::internal::SerializationTable serialization_table[];
  static const ::google::protobuf::uint32 offsets[];
};
void AddDescriptors();
}  // namespace protobuf_route_5fguide_2eproto
namespace gamingstreaming {
class FrameSubPacket;
class FrameSubPacketDefaultTypeInternal;
extern FrameSubPacketDefaultTypeInternal _FrameSubPacket_default_instance_;
class InputCommand;
class InputCommandDefaultTypeInternal;
extern InputCommandDefaultTypeInternal _InputCommand_default_instance_;
}  // namespace gamingstreaming
namespace google {
namespace protobuf {
template<> ::gamingstreaming::FrameSubPacket* Arena::CreateMaybeMessage<::gamingstreaming::FrameSubPacket>(Arena*);
template<> ::gamingstreaming::InputCommand* Arena::CreateMaybeMessage<::gamingstreaming::InputCommand>(Arena*);
}  // namespace protobuf
}  // namespace google
namespace gamingstreaming {

// ===================================================================

class FrameSubPacket : public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:gamingstreaming.FrameSubPacket) */ {
 public:
  FrameSubPacket();
  virtual ~FrameSubPacket();

  FrameSubPacket(const FrameSubPacket& from);

  inline FrameSubPacket& operator=(const FrameSubPacket& from) {
    CopyFrom(from);
    return *this;
  }
  #if LANG_CXX11
  FrameSubPacket(FrameSubPacket&& from) noexcept
    : FrameSubPacket() {
    *this = ::std::move(from);
  }

  inline FrameSubPacket& operator=(FrameSubPacket&& from) noexcept {
    if (GetArenaNoVirtual() == from.GetArenaNoVirtual()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }
  #endif
  static const ::google::protobuf::Descriptor* descriptor();
  static const FrameSubPacket& default_instance();

  static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY
  static inline const FrameSubPacket* internal_default_instance() {
    return reinterpret_cast<const FrameSubPacket*>(
               &_FrameSubPacket_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  void Swap(FrameSubPacket* other);
  friend void swap(FrameSubPacket& a, FrameSubPacket& b) {
    a.Swap(&b);
  }

  // implements Message ----------------------------------------------

  inline FrameSubPacket* New() const final {
    return CreateMaybeMessage<FrameSubPacket>(NULL);
  }

  FrameSubPacket* New(::google::protobuf::Arena* arena) const final {
    return CreateMaybeMessage<FrameSubPacket>(arena);
  }
  void CopyFrom(const ::google::protobuf::Message& from) final;
  void MergeFrom(const ::google::protobuf::Message& from) final;
  void CopyFrom(const FrameSubPacket& from);
  void MergeFrom(const FrameSubPacket& from);
  void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input) final;
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const final;
  ::google::protobuf::uint8* InternalSerializeWithCachedSizesToArray(
      bool deterministic, ::google::protobuf::uint8* target) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(FrameSubPacket* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return NULL;
  }
  inline void* MaybeArenaPtr() const {
    return NULL;
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // bytes data = 7;
  void clear_data();
  static const int kDataFieldNumber = 7;
  const ::std::string& data() const;
  void set_data(const ::std::string& value);
  #if LANG_CXX11
  void set_data(::std::string&& value);
  #endif
  void set_data(const char* value);
  void set_data(const void* value, size_t size);
  ::std::string* mutable_data();
  ::std::string* release_data();
  void set_allocated_data(::std::string* data);

  // int32 frame_number = 1;
  void clear_frame_number();
  static const int kFrameNumberFieldNumber = 1;
  ::google::protobuf::int32 frame_number() const;
  void set_frame_number(::google::protobuf::int32 value);

  // int32 packet_number = 2;
  void clear_packet_number();
  static const int kPacketNumberFieldNumber = 2;
  ::google::protobuf::int32 packet_number() const;
  void set_packet_number(::google::protobuf::int32 value);

  // int64 pts = 4;
  void clear_pts();
  static const int kPtsFieldNumber = 4;
  ::google::protobuf::int64 pts() const;
  void set_pts(::google::protobuf::int64 value);

  // int64 dts = 5;
  void clear_dts();
  static const int kDtsFieldNumber = 5;
  ::google::protobuf::int64 dts() const;
  void set_dts(::google::protobuf::int64 value);

  // int32 size = 3;
  void clear_size();
  static const int kSizeFieldNumber = 3;
  ::google::protobuf::int32 size() const;
  void set_size(::google::protobuf::int32 value);

  // int32 mouse_x = 8;
  void clear_mouse_x();
  static const int kMouseXFieldNumber = 8;
  ::google::protobuf::int32 mouse_x() const;
  void set_mouse_x(::google::protobuf::int32 value);

  // int64 flags = 6;
  void clear_flags();
  static const int kFlagsFieldNumber = 6;
  ::google::protobuf::int64 flags() const;
  void set_flags(::google::protobuf::int64 value);

  // int32 mouse_y = 9;
  void clear_mouse_y();
  static const int kMouseYFieldNumber = 9;
  ::google::protobuf::int32 mouse_y() const;
  void set_mouse_y(::google::protobuf::int32 value);

  // bool mouse_is_visible = 10;
  void clear_mouse_is_visible();
  static const int kMouseIsVisibleFieldNumber = 10;
  bool mouse_is_visible() const;
  void set_mouse_is_visible(bool value);

  // @@protoc_insertion_point(class_scope:gamingstreaming.FrameSubPacket)
 private:

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  ::google::protobuf::internal::ArenaStringPtr data_;
  ::google::protobuf::int32 frame_number_;
  ::google::protobuf::int32 packet_number_;
  ::google::protobuf::int64 pts_;
  ::google::protobuf::int64 dts_;
  ::google::protobuf::int32 size_;
  ::google::protobuf::int32 mouse_x_;
  ::google::protobuf::int64 flags_;
  ::google::protobuf::int32 mouse_y_;
  bool mouse_is_visible_;
  mutable ::google::protobuf::internal::CachedSize _cached_size_;
  friend struct ::protobuf_route_5fguide_2eproto::TableStruct;
};
// -------------------------------------------------------------------

class InputCommand : public ::google::protobuf::Message /* @@protoc_insertion_point(class_definition:gamingstreaming.InputCommand) */ {
 public:
  InputCommand();
  virtual ~InputCommand();

  InputCommand(const InputCommand& from);

  inline InputCommand& operator=(const InputCommand& from) {
    CopyFrom(from);
    return *this;
  }
  #if LANG_CXX11
  InputCommand(InputCommand&& from) noexcept
    : InputCommand() {
    *this = ::std::move(from);
  }

  inline InputCommand& operator=(InputCommand&& from) noexcept {
    if (GetArenaNoVirtual() == from.GetArenaNoVirtual()) {
      if (this != &from) InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }
  #endif
  static const ::google::protobuf::Descriptor* descriptor();
  static const InputCommand& default_instance();

  static void InitAsDefaultInstance();  // FOR INTERNAL USE ONLY
  static inline const InputCommand* internal_default_instance() {
    return reinterpret_cast<const InputCommand*>(
               &_InputCommand_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    1;

  void Swap(InputCommand* other);
  friend void swap(InputCommand& a, InputCommand& b) {
    a.Swap(&b);
  }

  // implements Message ----------------------------------------------

  inline InputCommand* New() const final {
    return CreateMaybeMessage<InputCommand>(NULL);
  }

  InputCommand* New(::google::protobuf::Arena* arena) const final {
    return CreateMaybeMessage<InputCommand>(arena);
  }
  void CopyFrom(const ::google::protobuf::Message& from) final;
  void MergeFrom(const ::google::protobuf::Message& from) final;
  void CopyFrom(const InputCommand& from);
  void MergeFrom(const InputCommand& from);
  void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input) final;
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const final;
  ::google::protobuf::uint8* InternalSerializeWithCachedSizesToArray(
      bool deterministic, ::google::protobuf::uint8* target) const final;
  int GetCachedSize() const final { return _cached_size_.Get(); }

  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(InputCommand* other);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return NULL;
  }
  inline void* MaybeArenaPtr() const {
    return NULL;
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // string command = 1;
  void clear_command();
  static const int kCommandFieldNumber = 1;
  const ::std::string& command() const;
  void set_command(const ::std::string& value);
  #if LANG_CXX11
  void set_command(::std::string&& value);
  #endif
  void set_command(const char* value);
  void set_command(const char* value, size_t size);
  ::std::string* mutable_command();
  ::std::string* release_command();
  void set_allocated_command(::std::string* command);

  // int32 event_type = 2;
  void clear_event_type();
  static const int kEventTypeFieldNumber = 2;
  ::google::protobuf::int32 event_type() const;
  void set_event_type(::google::protobuf::int32 value);

  // int32 key_code = 3;
  void clear_key_code();
  static const int kKeyCodeFieldNumber = 3;
  ::google::protobuf::int32 key_code() const;
  void set_key_code(::google::protobuf::int32 value);

  // int32 x = 4;
  void clear_x();
  static const int kXFieldNumber = 4;
  ::google::protobuf::int32 x() const;
  void set_x(::google::protobuf::int32 value);

  // int32 y = 5;
  void clear_y();
  static const int kYFieldNumber = 5;
  ::google::protobuf::int32 y() const;
  void set_y(::google::protobuf::int32 value);

  // int32 mouse_button = 6;
  void clear_mouse_button();
  static const int kMouseButtonFieldNumber = 6;
  ::google::protobuf::int32 mouse_button() const;
  void set_mouse_button(::google::protobuf::int32 value);

  // @@protoc_insertion_point(class_scope:gamingstreaming.InputCommand)
 private:

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  ::google::protobuf::internal::ArenaStringPtr command_;
  ::google::protobuf::int32 event_type_;
  ::google::protobuf::int32 key_code_;
  ::google::protobuf::int32 x_;
  ::google::protobuf::int32 y_;
  ::google::protobuf::int32 mouse_button_;
  mutable ::google::protobuf::internal::CachedSize _cached_size_;
  friend struct ::protobuf_route_5fguide_2eproto::TableStruct;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// FrameSubPacket

// int32 frame_number = 1;
inline void FrameSubPacket::clear_frame_number() {
  frame_number_ = 0;
}
inline ::google::protobuf::int32 FrameSubPacket::frame_number() const {
  // @@protoc_insertion_point(field_get:gamingstreaming.FrameSubPacket.frame_number)
  return frame_number_;
}
inline void FrameSubPacket::set_frame_number(::google::protobuf::int32 value) {
  
  frame_number_ = value;
  // @@protoc_insertion_point(field_set:gamingstreaming.FrameSubPacket.frame_number)
}

// int32 packet_number = 2;
inline void FrameSubPacket::clear_packet_number() {
  packet_number_ = 0;
}
inline ::google::protobuf::int32 FrameSubPacket::packet_number() const {
  // @@protoc_insertion_point(field_get:gamingstreaming.FrameSubPacket.packet_number)
  return packet_number_;
}
inline void FrameSubPacket::set_packet_number(::google::protobuf::int32 value) {
  
  packet_number_ = value;
  // @@protoc_insertion_point(field_set:gamingstreaming.FrameSubPacket.packet_number)
}

// int32 size = 3;
inline void FrameSubPacket::clear_size() {
  size_ = 0;
}
inline ::google::protobuf::int32 FrameSubPacket::size() const {
  // @@protoc_insertion_point(field_get:gamingstreaming.FrameSubPacket.size)
  return size_;
}
inline void FrameSubPacket::set_size(::google::protobuf::int32 value) {
  
  size_ = value;
  // @@protoc_insertion_point(field_set:gamingstreaming.FrameSubPacket.size)
}

// int64 pts = 4;
inline void FrameSubPacket::clear_pts() {
  pts_ = GOOGLE_LONGLONG(0);
}
inline ::google::protobuf::int64 FrameSubPacket::pts() const {
  // @@protoc_insertion_point(field_get:gamingstreaming.FrameSubPacket.pts)
  return pts_;
}
inline void FrameSubPacket::set_pts(::google::protobuf::int64 value) {
  
  pts_ = value;
  // @@protoc_insertion_point(field_set:gamingstreaming.FrameSubPacket.pts)
}

// int64 dts = 5;
inline void FrameSubPacket::clear_dts() {
  dts_ = GOOGLE_LONGLONG(0);
}
inline ::google::protobuf::int64 FrameSubPacket::dts() const {
  // @@protoc_insertion_point(field_get:gamingstreaming.FrameSubPacket.dts)
  return dts_;
}
inline void FrameSubPacket::set_dts(::google::protobuf::int64 value) {
  
  dts_ = value;
  // @@protoc_insertion_point(field_set:gamingstreaming.FrameSubPacket.dts)
}

// int64 flags = 6;
inline void FrameSubPacket::clear_flags() {
  flags_ = GOOGLE_LONGLONG(0);
}
inline ::google::protobuf::int64 FrameSubPacket::flags() const {
  // @@protoc_insertion_point(field_get:gamingstreaming.FrameSubPacket.flags)
  return flags_;
}
inline void FrameSubPacket::set_flags(::google::protobuf::int64 value) {
  
  flags_ = value;
  // @@protoc_insertion_point(field_set:gamingstreaming.FrameSubPacket.flags)
}

// bytes data = 7;
inline void FrameSubPacket::clear_data() {
  data_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline const ::std::string& FrameSubPacket::data() const {
  // @@protoc_insertion_point(field_get:gamingstreaming.FrameSubPacket.data)
  return data_.GetNoArena();
}
inline void FrameSubPacket::set_data(const ::std::string& value) {
  
  data_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:gamingstreaming.FrameSubPacket.data)
}
#if LANG_CXX11
inline void FrameSubPacket::set_data(::std::string&& value) {
  
  data_.SetNoArena(
    &::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::move(value));
  // @@protoc_insertion_point(field_set_rvalue:gamingstreaming.FrameSubPacket.data)
}
#endif
inline void FrameSubPacket::set_data(const char* value) {
  GOOGLE_DCHECK(value != NULL);
  
  data_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:gamingstreaming.FrameSubPacket.data)
}
inline void FrameSubPacket::set_data(const void* value, size_t size) {
  
  data_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:gamingstreaming.FrameSubPacket.data)
}
inline ::std::string* FrameSubPacket::mutable_data() {
  
  // @@protoc_insertion_point(field_mutable:gamingstreaming.FrameSubPacket.data)
  return data_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* FrameSubPacket::release_data() {
  // @@protoc_insertion_point(field_release:gamingstreaming.FrameSubPacket.data)
  
  return data_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void FrameSubPacket::set_allocated_data(::std::string* data) {
  if (data != NULL) {
    
  } else {
    
  }
  data_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), data);
  // @@protoc_insertion_point(field_set_allocated:gamingstreaming.FrameSubPacket.data)
}

// int32 mouse_x = 8;
inline void FrameSubPacket::clear_mouse_x() {
  mouse_x_ = 0;
}
inline ::google::protobuf::int32 FrameSubPacket::mouse_x() const {
  // @@protoc_insertion_point(field_get:gamingstreaming.FrameSubPacket.mouse_x)
  return mouse_x_;
}
inline void FrameSubPacket::set_mouse_x(::google::protobuf::int32 value) {
  
  mouse_x_ = value;
  // @@protoc_insertion_point(field_set:gamingstreaming.FrameSubPacket.mouse_x)
}

// int32 mouse_y = 9;
inline void FrameSubPacket::clear_mouse_y() {
  mouse_y_ = 0;
}
inline ::google::protobuf::int32 FrameSubPacket::mouse_y() const {
  // @@protoc_insertion_point(field_get:gamingstreaming.FrameSubPacket.mouse_y)
  return mouse_y_;
}
inline void FrameSubPacket::set_mouse_y(::google::protobuf::int32 value) {
  
  mouse_y_ = value;
  // @@protoc_insertion_point(field_set:gamingstreaming.FrameSubPacket.mouse_y)
}

// bool mouse_is_visible = 10;
inline void FrameSubPacket::clear_mouse_is_visible() {
  mouse_is_visible_ = false;
}
inline bool FrameSubPacket::mouse_is_visible() const {
  // @@protoc_insertion_point(field_get:gamingstreaming.FrameSubPacket.mouse_is_visible)
  return mouse_is_visible_;
}
inline void FrameSubPacket::set_mouse_is_visible(bool value) {
  
  mouse_is_visible_ = value;
  // @@protoc_insertion_point(field_set:gamingstreaming.FrameSubPacket.mouse_is_visible)
}

// -------------------------------------------------------------------

// InputCommand

// string command = 1;
inline void InputCommand::clear_command() {
  command_.ClearToEmptyNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline const ::std::string& InputCommand::command() const {
  // @@protoc_insertion_point(field_get:gamingstreaming.InputCommand.command)
  return command_.GetNoArena();
}
inline void InputCommand::set_command(const ::std::string& value) {
  
  command_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value);
  // @@protoc_insertion_point(field_set:gamingstreaming.InputCommand.command)
}
#if LANG_CXX11
inline void InputCommand::set_command(::std::string&& value) {
  
  command_.SetNoArena(
    &::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::move(value));
  // @@protoc_insertion_point(field_set_rvalue:gamingstreaming.InputCommand.command)
}
#endif
inline void InputCommand::set_command(const char* value) {
  GOOGLE_DCHECK(value != NULL);
  
  command_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value));
  // @@protoc_insertion_point(field_set_char:gamingstreaming.InputCommand.command)
}
inline void InputCommand::set_command(const char* value, size_t size) {
  
  command_.SetNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      ::std::string(reinterpret_cast<const char*>(value), size));
  // @@protoc_insertion_point(field_set_pointer:gamingstreaming.InputCommand.command)
}
inline ::std::string* InputCommand::mutable_command() {
  
  // @@protoc_insertion_point(field_mutable:gamingstreaming.InputCommand.command)
  return command_.MutableNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline ::std::string* InputCommand::release_command() {
  // @@protoc_insertion_point(field_release:gamingstreaming.InputCommand.command)
  
  return command_.ReleaseNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void InputCommand::set_allocated_command(::std::string* command) {
  if (command != NULL) {
    
  } else {
    
  }
  command_.SetAllocatedNoArena(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), command);
  // @@protoc_insertion_point(field_set_allocated:gamingstreaming.InputCommand.command)
}

// int32 event_type = 2;
inline void InputCommand::clear_event_type() {
  event_type_ = 0;
}
inline ::google::protobuf::int32 InputCommand::event_type() const {
  // @@protoc_insertion_point(field_get:gamingstreaming.InputCommand.event_type)
  return event_type_;
}
inline void InputCommand::set_event_type(::google::protobuf::int32 value) {
  
  event_type_ = value;
  // @@protoc_insertion_point(field_set:gamingstreaming.InputCommand.event_type)
}

// int32 key_code = 3;
inline void InputCommand::clear_key_code() {
  key_code_ = 0;
}
inline ::google::protobuf::int32 InputCommand::key_code() const {
  // @@protoc_insertion_point(field_get:gamingstreaming.InputCommand.key_code)
  return key_code_;
}
inline void InputCommand::set_key_code(::google::protobuf::int32 value) {
  
  key_code_ = value;
  // @@protoc_insertion_point(field_set:gamingstreaming.InputCommand.key_code)
}

// int32 x = 4;
inline void InputCommand::clear_x() {
  x_ = 0;
}
inline ::google::protobuf::int32 InputCommand::x() const {
  // @@protoc_insertion_point(field_get:gamingstreaming.InputCommand.x)
  return x_;
}
inline void InputCommand::set_x(::google::protobuf::int32 value) {
  
  x_ = value;
  // @@protoc_insertion_point(field_set:gamingstreaming.InputCommand.x)
}

// int32 y = 5;
inline void InputCommand::clear_y() {
  y_ = 0;
}
inline ::google::protobuf::int32 InputCommand::y() const {
  // @@protoc_insertion_point(field_get:gamingstreaming.InputCommand.y)
  return y_;
}
inline void InputCommand::set_y(::google::protobuf::int32 value) {
  
  y_ = value;
  // @@protoc_insertion_point(field_set:gamingstreaming.InputCommand.y)
}

// int32 mouse_button = 6;
inline void InputCommand::clear_mouse_button() {
  mouse_button_ = 0;
}
inline ::google::protobuf::int32 InputCommand::mouse_button() const {
  // @@protoc_insertion_point(field_get:gamingstreaming.InputCommand.mouse_button)
  return mouse_button_;
}
inline void InputCommand::set_mouse_button(::google::protobuf::int32 value) {
  
  mouse_button_ = value;
  // @@protoc_insertion_point(field_set:gamingstreaming.InputCommand.mouse_button)
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__
// -------------------------------------------------------------------


// @@protoc_insertion_point(namespace_scope)

}  // namespace gamingstreaming

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_INCLUDED_route_5fguide_2eproto

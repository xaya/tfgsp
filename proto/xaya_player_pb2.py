# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: proto/xaya_player.proto
"""Generated protocol buffer code."""
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()




DESCRIPTOR = _descriptor.FileDescriptor(
  name='proto/xaya_player.proto',
  package='pxd.proto',
  syntax='proto2',
  serialized_options=b'\370\001\001',
  create_key=_descriptor._internal_create_key,
  serialized_pb=b'\n\x17proto/xaya_player.proto\x12\tpxd.proto\"h\n\nXayaPlayer\x12\x0f\n\x07notused\x18\x01 \x01(\r\x12\x0c\n\x04role\x18\x02 \x01(\r\x12\x0f\n\x07\x62\x61lance\x18\x03 \x01(\x04\x12\x18\n\x10\x62urnsale_balance\x18\x04 \x01(\x04\x12\x10\n\x08xayaname\x18\x05 \x01(\tB\x03\xf8\x01\x01'
)




_XAYAPLAYER = _descriptor.Descriptor(
  name='XayaPlayer',
  full_name='pxd.proto.XayaPlayer',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='notused', full_name='pxd.proto.XayaPlayer.notused', index=0,
      number=1, type=13, cpp_type=3, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='role', full_name='pxd.proto.XayaPlayer.role', index=1,
      number=2, type=13, cpp_type=3, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='balance', full_name='pxd.proto.XayaPlayer.balance', index=2,
      number=3, type=4, cpp_type=4, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='burnsale_balance', full_name='pxd.proto.XayaPlayer.burnsale_balance', index=3,
      number=4, type=4, cpp_type=4, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='xayaname', full_name='pxd.proto.XayaPlayer.xayaname', index=4,
      number=5, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto2',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=38,
  serialized_end=142,
)

DESCRIPTOR.message_types_by_name['XayaPlayer'] = _XAYAPLAYER
_sym_db.RegisterFileDescriptor(DESCRIPTOR)

XayaPlayer = _reflection.GeneratedProtocolMessageType('XayaPlayer', (_message.Message,), {
  'DESCRIPTOR' : _XAYAPLAYER,
  '__module__' : 'proto.xaya_player_pb2'
  # @@protoc_insertion_point(class_scope:pxd.proto.XayaPlayer)
  })
_sym_db.RegisterMessage(XayaPlayer)


DESCRIPTOR._options = None
# @@protoc_insertion_point(module_scope)

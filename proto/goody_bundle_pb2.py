# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: proto/goody_bundle.proto
"""Generated protocol buffer code."""
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()




DESCRIPTOR = _descriptor.FileDescriptor(
  name='proto/goody_bundle.proto',
  package='pxd.proto',
  syntax='proto2',
  serialized_options=b'\370\001\001',
  create_key=_descriptor._internal_create_key,
  serialized_pb=b'\n\x18proto/goody_bundle.proto\x12\tpxd.proto\"9\n\x14\x41uthoredBundledGoody\x12\x0f\n\x07GoodyId\x18\x01 \x01(\t\x12\x10\n\x08Quantity\x18\x02 \x01(\r\"w\n\x0bGoodyBundle\x12\x12\n\nAuthoredId\x18\x01 \x01(\t\x12\x0c\n\x04Name\x18\x02 \x01(\t\x12\r\n\x05Price\x18\x03 \x01(\r\x12\x37\n\x0e\x42undledGoodies\x18\x04 \x03(\x0b\x32\x1f.pxd.proto.AuthoredBundledGoodyB\x03\xf8\x01\x01'
)




_AUTHOREDBUNDLEDGOODY = _descriptor.Descriptor(
  name='AuthoredBundledGoody',
  full_name='pxd.proto.AuthoredBundledGoody',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='GoodyId', full_name='pxd.proto.AuthoredBundledGoody.GoodyId', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='Quantity', full_name='pxd.proto.AuthoredBundledGoody.Quantity', index=1,
      number=2, type=13, cpp_type=3, label=1,
      has_default_value=False, default_value=0,
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
  serialized_start=39,
  serialized_end=96,
)


_GOODYBUNDLE = _descriptor.Descriptor(
  name='GoodyBundle',
  full_name='pxd.proto.GoodyBundle',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='AuthoredId', full_name='pxd.proto.GoodyBundle.AuthoredId', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='Name', full_name='pxd.proto.GoodyBundle.Name', index=1,
      number=2, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='Price', full_name='pxd.proto.GoodyBundle.Price', index=2,
      number=3, type=13, cpp_type=3, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='BundledGoodies', full_name='pxd.proto.GoodyBundle.BundledGoodies', index=3,
      number=4, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
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
  serialized_start=98,
  serialized_end=217,
)

_GOODYBUNDLE.fields_by_name['BundledGoodies'].message_type = _AUTHOREDBUNDLEDGOODY
DESCRIPTOR.message_types_by_name['AuthoredBundledGoody'] = _AUTHOREDBUNDLEDGOODY
DESCRIPTOR.message_types_by_name['GoodyBundle'] = _GOODYBUNDLE
_sym_db.RegisterFileDescriptor(DESCRIPTOR)

AuthoredBundledGoody = _reflection.GeneratedProtocolMessageType('AuthoredBundledGoody', (_message.Message,), {
  'DESCRIPTOR' : _AUTHOREDBUNDLEDGOODY,
  '__module__' : 'proto.goody_bundle_pb2'
  # @@protoc_insertion_point(class_scope:pxd.proto.AuthoredBundledGoody)
  })
_sym_db.RegisterMessage(AuthoredBundledGoody)

GoodyBundle = _reflection.GeneratedProtocolMessageType('GoodyBundle', (_message.Message,), {
  'DESCRIPTOR' : _GOODYBUNDLE,
  '__module__' : 'proto.goody_bundle_pb2'
  # @@protoc_insertion_point(class_scope:pxd.proto.GoodyBundle)
  })
_sym_db.RegisterMessage(GoodyBundle)


DESCRIPTOR._options = None
# @@protoc_insertion_point(module_scope)

noinst_LTLIBRARIES = libpxproto.la

CLEANFILES = \
  roconfig.pb.text roconfig.pb

BUILT_SOURCES = \
  roconfig.pb roconfig.pb.text

PROTOS = \
  $(builddir)/../proto/config.proto \
  $(builddir)/../proto/xaya_player.proto

libpxproto_la_CXXFLAGS = \
  -I$(top_srcdir) \
  $(XAYAGAME_CFLAGS) $(PROTOBUF_CFLAGS)
libpxproto_la_LIBADD = $(XAYAGAME_LIBS) $(PROTOBUF_LIBS)
libpxproto_la_SOURCES = \
  $(builddir)/../proto/roconfig.cpp \
  \
  roconfig_blob.s \
  $(PROTOS:.proto=.pb.cc)
noinst_HEADERS = \
  $(builddir)/../proto/roconfig.hpp \
  \
  $(PROTOS:.proto=.pb.h)

check_PROGRAMS = tests
TESTS = tests

tests_CXXFLAGS = \
  -I$(top_srcdir) \
  $(GTEST_MAIN_CFLAGS) \
  $(GTEST_CFLAGS) $(GLOG_CFLAGS) $(PROTOBUF_CFLAGS)
tests_LDADD = \
  $(builddir)/libpxproto.la \
  $(GTEST_MAIN_LIBS) \
  $(GTEST_LIBS) $(GLOG_LIBS) $(PROTOBUF_LIBS)
tests_SOURCES = \
  roconfig_tests.cpp

roconfig.pb roconfig.pb.text:
	$(builddir)/../proto/roconfig_gen \
          --out_binary="$(builddir)/roconfig.pb" \
          --out_text="$(builddir)/roconfig.pb.text"
	touch $(srcdir)/roconfig_blob.s
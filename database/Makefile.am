noinst_LTLIBRARIES = libdatabase.la

EXTRA_DIST = schema.sql schema_head.cpp schema_tail.cpp
CLEANFILES = schema.cpp

libdatabase_la_CXXFLAGS = \
  -I$(top_srcdir) \
  $(XAYAGAME_CFLAGS) \
  $(GLOG_CFLAGS) $(SQLITE3_CFLAGS) $(GMP_CFLAGS) $(PROTOBUF_CFLAGS)
libdatabase_la_LIBADD = \
  $(top_builddir)/proto2/libpxproto.la \
  $(XAYAGAME_LIBS) \
  $(GLOG_LIBS) $(SQLITE3_LIBS) $(GMP_LIBS) $(PROTOBUF_LIBS)
libdatabase_la_SOURCES = \
  database.cpp \
  schema.cpp \
  moneysupply.cpp \
  xayaplayer.cpp \
  uniquehandles.cpp
noinst_HEADERS = \
  database.hpp database.tpp \
  schema.hpp \
  amount.hpp \
  xayaplayer.hpp xayaplayer.tpp \
  moneysupply.hpp \
  uniquehandles.hpp uniquehandles.tpp

check_LTLIBRARIES = libdbtest.la
check_PROGRAMS = tests
TESTS = tests

libdbtest_la_CXXFLAGS = \
  $(XAYAGAME_CFLAGS) \
  $(GTEST_CFLAGS) $(GLOG_CFLAGS) $(SQLITE3_CFLAGS) $(BENCHMARK_CFLAGS)
libdbtest_la_LIBADD = \
  $(builddir)/libdatabase.la \
  $(XAYAGAME_LIBS) \
  $(GTEST_LIBS) $(GLOG_LIBS) $(SQLITE3_LIBS) $(BENCHMARK_LIBS)
libdbtest_la_SOURCES = \
  dbtest.cpp
check_HEADERS = \
  dbtest.hpp

tests_CXXFLAGS = \
  -I$(top_srcdir) \
  $(XAYAGAME_CFLAGS) \
  $(GTEST_MAIN_CFLAGS) \
  $(GTEST_CFLAGS) $(GLOG_CFLAGS)
tests_LDADD = \
  $(builddir)/libdbtest.la \
  $(builddir)/libdatabase.la \
  $(top_builddir)/proto2/libpxproto.la \
  $(XAYAGAME_LIBS) \
  $(GTEST_MAIN_LIBS) \
  $(GTEST_LIBS) $(GLOG_LIBS)
tests_SOURCES = \
  database_tests.cpp \
  schema_tests.cpp \
  moneysupply_tests.cpp \
  xayaplayer_tests.cpp \
  uniquehandles_tests.cpp

schema.cpp: schema_head.cpp schema.sql schema_tail.cpp
	cat $^ >$@

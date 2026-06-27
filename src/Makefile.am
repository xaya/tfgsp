noinst_LTLIBRARIES = libtf.la
bin_PROGRAMS = tfd
dist_data_DATA = rpc-stubs/tf-rpc.json

RPC_STUBS = \
  rpc-stubs/pxrpcserverstub.h
BUILT_SOURCES = $(RPC_STUBS)
CLEANFILES = $(RPC_STUBS)


libtf_la_CXXFLAGS = \
  -I$(top_srcdir) \
  $(XAYAGAME_CFLAGS) $(JSON_CFLAGS) \
  $(GLOG_CFLAGS) $(GFLAGS_CFLAGS) $(PROTOBUF_CFLAGS)
libtf_la_LIBADD = \
  $(top_builddir)/database/libdatabase.la \
  $(top_builddir)/proto2/libpxproto.la \
  $(XAYAGAME_LIBS) $(JSON_LIBS) \
  $(GLOG_LIBS) $(GFLAGS_LIBS) $(PROTOBUF_LIBS)
libtf_la_SOURCES = \
  context.cpp \
  forks.cpp \
  gamestatejson.cpp \
  jsonutils.cpp \
  logic.cpp \
  moveprocessor.cpp \
  burnsale.cpp \
  params.cpp \
  pending.cpp
libtfheaders = \
  fpm/fixed.hpp \
  fpm/ios.hpp \
  fpm/math.hpp \
  context.hpp \
  forks.hpp \
  gamestatejson.hpp \
  jsonutils.hpp \
  logic.hpp \
  moveprocessor.hpp \
  burnsale.hpp \
  params.hpp \
  pending.hpp

tfd_CXXFLAGS = \
  -I$(top_srcdir) \
  $(XAYAGAME_CFLAGS) \
  $(JSON_CFLAGS) $(MHD_CLFGAS) \
  $(GLOG_CFLAGS) $(GFLAGS_CFLAGS) $(PROTOBUF_CFLAGS)
  
 ifeq ($(OS),Windows_NT)
tfd_LDADD = \
  $(top_builddir)/libwinsane/libwinsane.la \
  $(builddir)/libtf.la \
  $(top_builddir)/database/libdatabase.la \
  $(XAYAGAME_LIBS) \
  $(JSON_LIBS) $(MHD_LIBS) \
  $(GLOG_LIBS) $(GFLAGS_LIBS) $(PROTOBUF_LIBS)
 else
tfd_LDADD = \
  $(builddir)/libtf.la \
  $(top_builddir)/database/libdatabase.la \
  $(XAYAGAME_LIBS) \
  $(JSON_LIBS) $(MHD_LIBS) \
  $(GLOG_LIBS) $(GFLAGS_LIBS) $(PROTOBUF_LIBS)
 endif

tfd_SOURCES = main.cpp \
  pxrpcserver.cpp \
  rest.cpp
tfdheaders = \
  pxrpcserver.hpp \
  rest.hpp \
  \
  rpc-stubs/pxrpcserverstub.h

noinst_HEADERS = $(libtfheaders) $(tfdheaders)

check_LTLIBRARIES = libtestutils.la
# goldenreplay_tests is a SEPARATE binary on purpose: it must run in a pristine
# process.  Several tests in `tests` mutate the process-global RoConfig
# singleton via const_cast (e.g. logic_tests.cpp set_max_fighter_inventory_amount),
# and that contamination persists across tests in the same binary -- which would
# make the deterministic golden snapshot order-dependent.  Isolation keeps it
# reproducible and independently runnable as the Phase 2 regression net.
check_PROGRAMS = tests goldenreplay_tests
TESTS = tests goldenreplay_tests

libtestutils_la_CXXFLAGS = \
  -I$(top_srcdir) \
  $(XAYAGAME_CFLAGS) $(JSON_CFLAGS)
libtestutils_la_LIBADD = \
  $(builddir)/libtf.la \
  $(XAYAGAME_LIBS) $(JSON_LIBS)
libtestutils_la_SOURCES = \
  testutils.cpp

tests_CXXFLAGS = \
  -I$(top_srcdir) \
  $(GTEST_MAIN_CFLAGS) \
  $(JSON_CFLAGS) $(GTEST_CFLAGS) \
  $(GLOG_CFLAGS) $(GFLAGS_CFLAGS) $(PROTOBUF_CFLAGS)
tests_LDADD = \
  $(builddir)/libtf.la \
  $(builddir)/libtestutils.la \
  $(top_builddir)/database/libdbtest.la \
  $(top_builddir)/database/libdatabase.la \
  $(top_builddir)/proto2/libpxproto.la \
  $(GTEST_MAIN_LIBS) \
  $(JSON_LIBS) $(GTEST_LIBS) \
  $(GLOG_LIBS) $(GFLAGS_LIBS) $(PROTOBUF_LIBS)
tests_SOURCES = \
  forks_tests.cpp \
  gamestatejson_tests.cpp \
  jsonutils_tests.cpp \
  logic_tests.cpp \
  pending_tests.cpp \
  burnsale_tests.cpp \
  moveprocessor_tests.cpp \
  testutils_tests.cpp

goldenreplay_tests_CXXFLAGS = \
  -I$(top_srcdir) \
  $(GTEST_MAIN_CFLAGS) \
  $(JSON_CFLAGS) $(GTEST_CFLAGS) \
  $(GLOG_CFLAGS) $(GFLAGS_CFLAGS) $(PROTOBUF_CFLAGS)
goldenreplay_tests_LDADD = \
  $(builddir)/libtf.la \
  $(builddir)/libtestutils.la \
  $(top_builddir)/database/libdbtest.la \
  $(top_builddir)/database/libdatabase.la \
  $(top_builddir)/proto2/libpxproto.la \
  $(GTEST_MAIN_LIBS) \
  $(JSON_LIBS) $(GTEST_LIBS) \
  $(GLOG_LIBS) $(GFLAGS_LIBS) $(PROTOBUF_LIBS)
goldenreplay_tests_SOURCES = \
  goldenreplay_tests.cpp

check_HEADERS = \
  testutils.hpp
EXTRA_DIST = \
  goldenreplay.golden.json

rpc-stubs/pxrpcserverstub.h: $(srcdir)/rpc-stubs/tf-rpc.json
	jsonrpcstub "$<" --cpp-server=PXRpcServerStub --cpp-server-file="$@"

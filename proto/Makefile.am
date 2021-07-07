noinst_PROGRAMS = roconfig_gen

ROCONFIG_TEXT_PROTOS = \
  roconfig/params.pb.text \
  roconfig/testnet.pb.text

BUILT_SOURCES = \
  $(PROTOS:.proto=.pb.h) $(PROTOS:.proto=_pb2.py) \
  roconfig_gen.cpp

ROCONFIG_TEXT_PROTOS_REGTEST = \
  roconfig/test_params.pb.text

PROTOS = \
  config.proto \
  xaya_player.proto
  
EXTRA_DIST = \
  $(PROTOS) $(ROCONFIG_TEXT_PROTOS) $(ROCONFIG_TEXT_PROTOS_REGTEST) \
  roconfig_gen_head.cpp roconfig_gen_mid.cpp roconfig_gen_tail.cpp  

CLEANFILES = \
  $(PROTOS:.proto=.pb.h) $(PROTOS:.proto=.pb.cc) $(PROTOS:.proto=_pb2.py)\
  roconfig_gen.cpp

noinst_HEADERS = \
  roconfig.hpp \
  \
  $(PROTOS:.proto=.pb.h)
  
roconfig_gen_CXXFLAGS = \
  -I$(top_srcdir) \
  $(GLOG_CFLAGS) $(GFLAGS_CFLAGS) $(PROTOBUF_CFLAGS)
roconfig_gen_LDADD = $(GLOG_LIBS) $(GFLAGS_LIBS) $(PROTOBUF_LIBS)
roconfig_gen_SOURCES = \
  roconfig_gen.cpp \
  \
  $(PROTOS:.proto=.pb.cc)  

noinst_PYTHON = \
  __init__.py \
  $(PROTOS:.proto=_pb2.py)
    
%.pb.h %.pb.cc: $(top_srcdir)/proto/%.proto
	protoc -I$(top_srcdir) --cpp_out=$(top_builddir) "$<"
%_pb2.py: $(top_srcdir)/proto/%.proto
	protoc -I$(top_srcdir) --python_out=$(top_builddir) "$<"

roconfig_gen.cpp: roconfig_gen_head.cpp $(ROCONFIG_TEXT_PROTOS) roconfig_gen_mid.cpp $(ROCONFIG_TEXT_PROTOS_REGTEST) roconfig_gen_tail.cpp
	cat $^ >$@
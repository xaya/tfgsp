noinst_LTLIBRARIES = libpxproto.la

CLEANFILES = \
  roconfig.pb.text roconfig.pb

BUILT_SOURCES = \
  roconfig.pb roconfig.pb.text

PROTOS = \
  $(builddir)/../proto/config.proto \
  $(builddir)/../proto/armor_piece.proto \
  $(builddir)/../proto/inventory.proto \
  $(builddir)/../proto/activity_rewards.proto \
  $(builddir)/../proto/tournament_result.proto \  
  $(builddir)/../proto/tournament_instance.proto \
  $(builddir)/../proto/activity_reward_instance.proto \
  $(builddir)/../proto/animation.proto \
  $(builddir)/../proto/candy.proto \
  $(builddir)/../proto/crystal_bundle.proto \
  $(builddir)/../proto/expedition_blueprint.proto \
  $(builddir)/../proto/fighter_move_blueprint.proto \
  $(builddir)/../proto/fighter_name.proto \
  $(builddir)/../proto/fighter_type.proto \
  $(builddir)/../proto/goody.proto \
  $(builddir)/../proto/goody_bundle.proto \
  $(builddir)/../proto/sweetener_blueprint.proto \
  $(builddir)/../proto/tournament_blueprint.proto \
  $(builddir)/../proto/xaya_player.proto \
  $(builddir)/../proto/crafted_recipe.proto \
  $(builddir)/../proto/ongoing.proto \
  $(builddir)/../proto/fighter.proto

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
  $(XAYAGAME_CFLAGS) $(JSON_CFLAGS) \
  $(GTEST_CFLAGS) $(GLOG_CFLAGS) $(PROTOBUF_CFLAGS)
tests_LDADD = \
  $(builddir)/libpxproto.la \
  $(GTEST_MAIN_LIBS) \
  $(GTEST_LIBS) $(GLOG_LIBS) $(PROTOBUF_LIBS)
tests_SOURCES = \
  roconfig_tests.cpp

# roconfig_gen writes BOTH roconfig.pb and its text form in a single run.
# Declare only roconfig.pb as the real target, with roconfig.pb.text as a
# dependent alias: a multi-target rule ("roconfig.pb roconfig.pb.text:") makes
# GNU make -j run the recipe once per requested target -- i.e. two roconfig_gen
# processes writing roconfig.pb concurrently -> a truncated/corrupt blob that
# fails to parse at runtime.  This is consensus-critical: a corrupt config blob
# aborts the daemon on startup or, worse, silently diverges it.
roconfig.pb: $(builddir)/../proto/roconfig_gen
	$(builddir)/../proto/roconfig_gen \
          --out_binary="$(builddir)/roconfig.pb" \
          --out_text="$(builddir)/roconfig.pb.text"
	touch $(srcdir)/roconfig_blob.s
roconfig.pb.text: roconfig.pb

# roconfig_blob.s embeds roconfig.pb via `.incbin`, a dependency make cannot see.
# Pin the ordering so the blob object is never assembled before roconfig.pb is
# complete (belt-and-suspenders alongside the single-output rule above).
roconfig_blob.lo: roconfig.pb
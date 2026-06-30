AC_INIT([tfd], [0.4])
AM_INIT_AUTOMAKE

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AM_SILENT_RULES([yes])

AC_PROG_CXX
AM_PROG_AS
AM_PROG_AR
AM_PATH_PYTHON([3.7])

LT_INIT

AC_LANG([C++])
AX_CXX_COMPILE_STDCXX([17], [noext])
AX_CHECK_COMPILE_FLAG([-Wall], [CXXFLAGS="${CXXFLAGS} -Wall"])

# Determinism guard for the consensus state machine: forbid the compiler from
# contracting floating-point expressions (e.g. fusing a*b+c into an FMA with a
# single rounding step).  Contraction is permitted to vary by target / -O level,
# so two honest node operators could otherwise derive a 1-ULP-different value
# from identical source and fork the chain.  The consensus code is integer /
# fixed-point today (enforced by contrib/check-consensus-determinism.py), so
# this only insures against a future stray float; no effect on current codegen.
AX_CHECK_COMPILE_FLAG([-ffp-contract=off],
                      [CXXFLAGS="${CXXFLAGS} -ffp-contract=off"])

# Windows defines ERROR, which requires us to tell glog to not define
# it as abbreviated log severity (LOG(ERROR) still works, though, and
# that is all that we actually use in the code).
# See https://hpc.nih.gov/development/glog.html.
CXXFLAGS="${CXXFLAGS} -DGLOG_NO_ABBREVIATED_SEVERITIES"

# The code includes some sanity checks / assertions that noticably slow down
# general block-processing performance.  Thus enabling them is optional,
# and should be used only for testing purposes.
AC_ARG_ENABLE([slow-asserts],
  AS_HELP_STRING([--enable-slow-asserts],
                 [Enable slow consistency checks for testing]))
AS_IF([test "x$enable_slow_asserts" = "xyes"], [
  CXXFLAGS="${CXXFLAGS} -DENABLE_SLOW_ASSERTS"
], [
  enable_slow_asserts="no"
])

PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES([XAYAGAME], [libxayautil libxayagame])
PKG_CHECK_MODULES([SQLITE3], [sqlite3])
PKG_CHECK_MODULES([JSON], [jsoncpp])
PKG_CHECK_MODULES([MHD], [libmicrohttpd])
PKG_CHECK_MODULES([GLOG], [libglog])
PKG_CHECK_MODULES([GFLAGS], [gflags])
PKG_CHECK_MODULES([GTEST], [gmock gtest])
PKG_CHECK_MODULES([GTEST_MAIN], [gtest_main])
PKG_CHECK_MODULES([PROTOBUF], [protobuf])

# Make sure that pkg-config provides the right libraries for some dependencies
# that may only be available statically.  For context, see:
# https://bugs.freedesktop.org/show_bug.cgi?id=19541.
PKG_CONFIG="${PKG_CONFIG} --static"
PKG_CHECK_MODULES([BENCHMARK], [benchmark])

AC_CONFIG_FILES([
  Makefile \
  libwinsane/Makefile \
  database/Makefile \
  proto/Makefile \
  proto2/Makefile \
  src/Makefile
])
AC_OUTPUT

echo
echo "Slow assertions: ${enable_slow_asserts}"
echo "CXXFLAGS: ${CXXFLAGS}"

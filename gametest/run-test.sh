#!/bin/sh
# Wrapper for the Python integration tests.
#
# These tests drive a full Xaya regtest environment: they need the
# `xayagametest` Python package and a Xaya Core daemon (`xayad`).  That stack
# is independent of building the GSP itself, so when it is absent (e.g. CI or a
# Docker build that only compiles tfd and runs the C++ unit tests), skip the
# Python tests cleanly rather than failing the whole `make check`.  Automake
# treats exit code 77 as SKIP.
#
# Set XAYAD=/path/to/xayad (and install xayagametest) to actually run them.

if ! python3 -c 'import xayagametest' >/dev/null 2>&1; then
  echo "SKIP: xayagametest Python package not available"
  exit 77
fi

if [ -z "$XAYAD" ] && ! command -v xayad >/dev/null 2>&1; then
  echo "SKIP: Xaya Core daemon (xayad) not available"
  exit 77
fi

exec "$@"

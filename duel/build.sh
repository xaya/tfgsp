#!/usr/bin/env bash
# Dual build: native test binary (emsdk clang) + zero-import wasm reactor (wasi-sdk 24,
# the proven bomberman blob recipe — no -Wl flags: undefined symbol = link error = zero-import).
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
NAT="${NATIVE_CXX:-$HOME/emsdk/upstream/bin/clang++}"
WASI="${WASI_SDK:-$HOME/arcade-spike/toolchains/wasi-sdk-24.0-x86_64-linux}"
SRC="$here/src/engine.cpp"
mode="${1:-all}"
if [[ "$mode" == native || "$mode" == all || "$mode" == test ]]; then
  "$NAT" -O2 -std=c++17 -fno-exceptions -fno-rtti -Wall -Wextra -Werror \
    -I"$here/src" "$SRC" "$here/test/test_main.cpp" -o "$here/test/duel_tests"
fi
if [[ "$mode" == wasm || "$mode" == all ]]; then
  "$WASI/bin/clang++" -O2 -std=c++17 -fno-exceptions -fno-rtti -mexec-model=reactor \
    -I"$here/src" "$SRC" -o "$here/engine.wasm"
  sha256sum "$here/engine.wasm" | awk '{print $1}' > "$here/engine.wasm.sha256"
fi
if [[ "$mode" == test || "$mode" == all ]]; then "$here/test/duel_tests"; fi

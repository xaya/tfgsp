#!/usr/bin/env bash
# Dual build: native test binary (emsdk clang) + zero-import wasm reactor (wasi-sdk 24,
# the proven bomberman blob recipe — no -Wl flags: undefined symbol = link error = zero-import).
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
NAT="${NATIVE_CXX:-$HOME/emsdk/upstream/bin/clang++}"
WASI="${WASI_SDK:-$HOME/arcade-spike/toolchains/wasi-sdk-24.0-x86_64-linux}"
SRC="$here/src/engine.cpp"
mode="${1:-all}"
if [[ "$mode" == native || "$mode" == all || "$mode" == test || "$mode" == golden ]]; then
  "$NAT" -O2 -std=c++17 -fno-exceptions -fno-rtti -Wall -Wextra -Werror \
    -I"$here/src" "$SRC" "$here/test/test_main.cpp" -o "$here/test/duel_tests"
fi
# sanitize: same native test binary under ASan+UBSan (abort on any UB), the
# one-shot gate for Task 4's self-play soak + byte-flip/truncation fuzz. Uses a
# host compiler that ships the sanitizer runtimes (emsdk clang doesn't), so
# SAN_CXX defaults to g++ rather than NATIVE_CXX.
SAN="${SAN_CXX:-g++}"
if [[ "$mode" == sanitize ]]; then
  "$SAN" -O1 -g -std=c++17 -fno-exceptions -fno-rtti \
    -fsanitize=address,undefined -fno-sanitize-recover=all \
    -Wall -Wextra -Werror \
    -I"$here/src" "$SRC" "$here/test/test_main.cpp" -o "$here/test/duel_tests_san"
  "$here/test/duel_tests_san"
fi
if [[ "$mode" == wasm || "$mode" == all || "$mode" == parity ]]; then
  "$WASI/bin/clang++" -O2 -std=c++17 -fno-exceptions -fno-rtti -mexec-model=reactor \
    -I"$here/src" "$SRC" -o "$here/engine.wasm"
  sha256sum "$here/engine.wasm" | awk '{print $1}' > "$here/engine.wasm.sha256"
fi
if [[ "$mode" == test || "$mode" == all ]]; then "$here/test/duel_tests"; fi
# golden: dump every behavioral vector from the just-built NATIVE binary as
# golden JSON lines (test/golden.json is a build artifact -- see .gitignore
# -- regenerate it any time engine.cpp/test_main.cpp change).
if [[ "$mode" == golden ]]; then
  "$here/test/duel_tests" --dump-golden > "$here/test/golden.json"
fi
# parity: replay test/golden.json (regenerate it first via `build.sh golden`
# if stale/missing) through the just-built WASM module (Node) and
# byte-compare every vector against the native output captured there.
if [[ "$mode" == parity ]]; then
  node "$here/test/parity.mjs"
fi

#!/usr/bin/env bash
# set-dev-address.sh — swap the production dev_address in BOTH repos in lockstep.
#
#   contrib/set-dev-address.sh 0xNEWADDRESS [--fe PATH] [--no-commit]
#
# Every WCHI payment in the game (crystal bundles, name rerolls) routes to this
# address, so it must match between the GSP roconfig and the web frontend.
# This script:
#   1. checksum-validates the address (EIP-55, via the frontend's viem),
#   2. updates proto/roconfig/params.pb.text (GSP) + src/config.ts (frontend),
#   3. regenerates the roconfig blob and RE-PINS the LaunchConfigIsPinned hash
#      (proto2/roconfig_tests.cpp) to the new deterministic blob hash,
#   4. runs the FULL GSP make check in Docker (golden must stay byte-identical:
#      dev_address does not reach replayed state) + the frontend lockstep test,
#   5. commits both repos (unless --no-commit).
set -euo pipefail

REPO="$(cd "$(dirname "$0")/.." && pwd)"
FE="$(cd "$REPO/../tf-frontend" 2>/dev/null && pwd || true)"
BUILD=/tmp/tf-build
IMAGE=tf-builder:local
COMMIT=1
ADDR=""

while [ $# -gt 0 ]; do
  case "$1" in
    --fe) FE="$(cd "$2" && pwd)"; shift 2 ;;
    --no-commit) COMMIT=0; shift ;;
    0x*) ADDR="$1"; shift ;;
    *) echo "unknown arg: $1" >&2; exit 2 ;;
  esac
done
[ -n "$ADDR" ] || { echo "usage: $0 0xNEWADDRESS [--fe PATH] [--no-commit]" >&2; exit 2; }
[ -d "$FE" ] || { echo "FATAL: frontend repo not found (use --fe PATH)" >&2; exit 1; }

GSP_FILES=(proto/roconfig/params.pb.text proto2/roconfig_tests.cpp)
FE_FILES=(src/config.ts)

# The files this script edits+commits must start clean (the pathspec commits below
# already exclude any other pre-staged files).
{ git -C "$REPO" diff --quiet -- "${GSP_FILES[@]}" && git -C "$REPO" diff --cached --quiet -- "${GSP_FILES[@]}"; } \
  || { echo "FATAL: GSP target files (${GSP_FILES[*]}) have uncommitted changes." >&2; exit 1; }
{ git -C "$FE" diff --quiet -- "${FE_FILES[@]}" && git -C "$FE" diff --cached --quiet -- "${FE_FILES[@]}"; } \
  || { echo "FATAL: frontend target files (${FE_FILES[*]}) have uncommitted changes." >&2; exit 1; }

# On any failure past this point, tell the operator exactly how to get back to
# a known state — a half-applied dev_address swap must never linger silently.
recovery () {
  echo "" >&2
  echo "[devaddr] FAILED — the swap may be half-applied. Recover with:" >&2
  echo "  git -C $REPO checkout -- ${GSP_FILES[*]}" >&2
  echo "  git -C $FE checkout -- ${FE_FILES[*]}" >&2
}
trap 'recovery' ERR

# ---- 1. EIP-55 checksum validation (via the FE's viem) ---------------------
CHECKED=$(cd "$FE" && node -e "
  const { getAddress } = require('viem');
  try { const a = getAddress(process.argv[1]); console.log(a); }
  catch (e) { console.error('FATAL: invalid/bad-checksum address: ' + e.shortMessage); process.exit(1); }
" "$ADDR")
[ "$CHECKED" = "$ADDR" ] || { echo "FATAL: address checksum mismatch — the canonical EIP-55 form is $CHECKED (you passed $ADDR). Pass it exactly to prove it wasn't typo'd." >&2; exit 1; }
echo "[devaddr] checksum OK: $ADDR"

# Each repo's OLD address is read from THAT repo's own file (a pre-existing
# divergence must surface here, not as a confusing half-applied sed later).
GSP_OLD=$(grep -oE 'dev_address: "0x[0-9a-fA-F]{40}"' "$REPO/proto/roconfig/params.pb.text" | grep -oE '0x[0-9a-fA-F]{40}')
FE_OLD=$(grep -oE "DEV_ADDRESS = '0x[0-9a-fA-F]{40}'" "$FE/src/config.ts" | grep -oE '0x[0-9a-fA-F]{40}')
if [ "$GSP_OLD" != "$FE_OLD" ]; then
  echo "[devaddr] WARNING: repos currently DIVERGE (GSP=$GSP_OLD, FE=$FE_OLD) — fixing both to $ADDR."
fi
if [ "$GSP_OLD" = "$ADDR" ] && [ "$FE_OLD" = "$ADDR" ]; then
  echo "[devaddr] no-op: both repos already use $ADDR. Nothing to do."
  trap - ERR
  exit 0
fi
echo "[devaddr] GSP: $GSP_OLD -> $ADDR   FE: $FE_OLD -> $ADDR"

# ---- 2. Edit both repos ------------------------------------------------------
sed -i "s/dev_address: \"$GSP_OLD\"/dev_address: \"$ADDR\"/" "$REPO/proto/roconfig/params.pb.text"
grep -q "dev_address: \"$ADDR\"" "$REPO/proto/roconfig/params.pb.text" || { echo "FATAL: GSP edit failed" >&2; exit 1; }
sed -i "s/export const DEV_ADDRESS = '$FE_OLD'/export const DEV_ADDRESS = '$ADDR'/" "$FE/src/config.ts"
grep -q "DEV_ADDRESS = '$ADDR'" "$FE/src/config.ts" || { echo "FATAL: frontend edit failed" >&2; exit 1; }

# ---- 3. Regenerate blob, learn the new pin hash, re-pin -----------------------
mkdir -p "$BUILD"
echo "[devaddr] docker pass 1: regenerate blob + read the new pinned hash ..."
NEWHASH=$(docker run --rm --network none \
  -v "$REPO":/src-ro:ro -v "$BUILD":/build \
  --entrypoint bash "$IMAGE" -c '
    set -e
    cp -au /src-ro/. /build/
    cmp -s /src-ro/proto/roconfig/params.pb.text /build/proto/roconfig/params.pb.text || { echo SYNC_FAIL >&2; exit 1; }
    cd /build
    if [ ! -f config.status ]; then ./autogen.sh >/dev/null 2>&1 && ./configure >/dev/null 2>&1; else ./config.status >/dev/null 2>&1 || true; fi
    # TWO make targets needed: the directory default regenerates the roconfig BLOB from the
    # edited params.pb.text; `tests` builds the check binary (check_PROGRAMS are on-demand).
    # `make tests` ALONE links against a stale blob — measured, it silently passes the pin.
    make -j"$(nproc)" -C proto2 >/tmp/p2.log 2>&1 \
      && make -j"$(nproc)" -C proto2 tests >>/tmp/p2.log 2>&1 \
      || { echo BUILD_FAIL >&2; tail -8 /tmp/p2.log >&2; exit 1; }
    cd proto2 && ./tests --gtest_filter=RoConfigTests.LaunchConfigIsPinned >/tmp/pin.log 2>&1 || true
    hash=$(grep -A1 "Which is:" /tmp/pin.log | grep -oE "[0-9a-f]{64}" | head -1)
    if [ -n "$hash" ]; then echo "$hash"
    elif grep -q "\[  PASSED  \]" /tmp/pin.log; then echo PIN_ALREADY_GREEN
    else echo EXTRACT_FAIL >&2; tail -10 /tmp/pin.log >&2; exit 1; fi
  ')
OLDHASH=$(grep -oE '"[0-9a-f]{64}"' "$REPO/proto2/roconfig_tests.cpp" | tr -d '"' | head -1)
if [ "$NEWHASH" = "PIN_ALREADY_GREEN" ]; then
  # Impossible for a real address change (dev_address is IN the blob): a green pin here
  # means the blob did not regenerate — a stale build must never masquerade as success.
  echo "FATAL: pin test passed although the address changed — stale blob build. Investigate proto2 deps." >&2
  exit 1
fi
[ ${#NEWHASH} -eq 64 ] || { echo "FATAL: could not extract new blob hash (got: $NEWHASH)" >&2; exit 1; }
if [ "$NEWHASH" != "$OLDHASH" ]; then
  echo "[devaddr] re-pinning blob hash $OLDHASH -> $NEWHASH"
  sed -i "s/$OLDHASH/$NEWHASH/" "$REPO/proto2/roconfig_tests.cpp"
fi

# ---- 4. Full GSP make check + FE lockstep test --------------------------------
echo "[devaddr] docker pass 2: FULL make check ..."
docker run --rm --network none \
  -v "$REPO":/src-ro:ro -v "$BUILD":/build \
  --entrypoint bash "$IMAGE" -c '
    set -e
    cp -au /src-ro/. /build/
    cmp -s /src-ro/proto2/roconfig_tests.cpp /build/proto2/roconfig_tests.cpp || { echo SYNC_FAIL >&2; exit 1; }
    cd /build && make -j"$(nproc)" >/tmp/make.log 2>&1 || { echo MAKE_FAIL; grep -E "error:" /tmp/make.log | head; exit 1; }
    make check >/tmp/check.log 2>&1 && echo CHECK_ALL_PASS || {
      echo CHECK_HAD_FAILURES
      grep -rhE "\[  FAILED  \]" src/test-suite.log proto2/test-suite.log database/test-suite.log 2>/dev/null | sort -u | head -20
      exit 1
    }
  '
if ! git -C "$REPO" diff --quiet -- src/goldenreplay.golden.json; then
  echo "FATAL: golden changed — dev_address must NOT reach replayed state. Investigate." >&2; exit 1
fi
echo "[devaddr] frontend: lockstep test (REQUIRED mode) + typecheck ..."
(cd "$FE" && TFGSP_PATH="$REPO" LOCKSTEP_REQUIRED=1 npx vitest run tests/config/dev-address-lockstep.test.ts \
   && npx tsc --noEmit) >/tmp/devaddr-fe.log 2>&1 \
  && echo "[devaddr] frontend checks green" \
  || { echo "FATAL: frontend checks failed:" >&2; tail -15 /tmp/devaddr-fe.log >&2; exit 1; }

# ---- 5. Commit both repos (explicit pathspec: nothing pre-staged can ride in) --
trap - ERR
if [ "$COMMIT" -eq 1 ]; then
  git -C "$REPO" -c user.name=snailbrainx -c user.email=17693211+snailbrainx@users.noreply.github.com \
    commit -m "config: set production dev_address ($ADDR) + re-pin roconfig blob hash" -- "${GSP_FILES[@]}"
  git -C "$FE" -c user.name=snailbrainx -c user.email=17693211+snailbrainx@users.noreply.github.com \
    commit -m "config: set production dev_address ($ADDR) — lockstep with GSP roconfig" -- "${FE_FILES[@]}"
  echo "[devaddr] committed BOTH repos. Push each."
else
  echo "[devaddr] --no-commit: changes left in both working trees."
fi
echo "[devaddr] DONE — dev_address=$ADDR in GSP + frontend, blob re-pinned, all checks green."

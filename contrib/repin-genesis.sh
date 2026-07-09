#!/usr/bin/env bash
# repin-genesis.sh — one-command POLYGON genesis re-pin for launch.
#
#   contrib/repin-genesis.sh [--rpc URL] [--depth N] [--height H --hash 0x...]
#                            [--no-commit] [--dry-run] [--force] [--allow-single-rpc]
#
# Fetches the current Polygon tip, cross-checks the pinned block's hash across
# MULTIPLE RPCs (a single lying endpoint must not choose the genesis), rewrites
# the POLYGON genesis pin in src/logic.cpp, regenerates the golden replay and
# runs the FULL make check inside the tf-builder:local Docker image (offline),
# and commits the result.
#
# CONSENSUS-CRITICAL: run this as the LAST code change before the live deploy
# and never change the pin after launch.  --depth (default 256 blocks, ~9 min)
# keeps the pinned block safely behind any realistic Polygon reorg.  Once a
# non-empty hash is pinned the script refuses to run again without --force.
set -euo pipefail

REPO="$(cd "$(dirname "$0")/.." && pwd)"
BUILD=/tmp/tf-build
IMAGE=tf-builder:local
RPCS=("https://1rpc.io/matic" "https://polygon-rpc.com" "https://polygon.drpc.org" \
      "https://polygon.meowrpc.com" "https://polygon-bor-rpc.publicnode.com")
DEPTH=256
HEIGHT="" HASH="" COMMIT=1 DRY=0 FORCE=0 SINGLE_OK=0

while [ $# -gt 0 ]; do
  case "$1" in
    --rpc) RPCS=("$2"); SINGLE_OK=1; shift 2 ;;
    --depth) DEPTH="$2"; shift 2 ;;
    --height) HEIGHT="$2"; shift 2 ;;
    --hash) HASH="$2"; shift 2 ;;
    --no-commit) COMMIT=0; shift ;;
    --dry-run) DRY=1; shift ;;
    --force) FORCE=1; shift ;;
    --allow-single-rpc) SINGLE_OK=1; shift ;;
    *) echo "unknown arg: $1" >&2; exit 2 ;;
  esac
done

# Explicit pin: require BOTH parts — a lone --height/--hash silently falling
# back to the RPC path was a footgun.
if { [ -n "$HEIGHT" ] && [ -z "$HASH" ]; } || { [ -z "$HEIGHT" ] && [ -n "$HASH" ]; }; then
  echo "FATAL: --height and --hash must be given together" >&2; exit 2
fi

# Refuse to re-pin over an already-pinned (non-empty-hash) genesis.
if [ "$FORCE" -eq 0 ] && ! grep -A10 'Fresh Treatfighter relaunch genesis' "$REPO/src/logic.cpp" | grep -q 'hashHex = "";'; then
  echo "FATAL: the POLYGON genesis already has a non-empty pinned hash. If you really" >&2
  echo "       mean to re-pin (pre-launch only!), rerun with --force." >&2
  exit 1
fi

# The files this script edits+commits must start clean (what gets checked = what
# ships; the pathspec commit below already excludes any other pre-staged files).
TARGETS=(src/logic.cpp src/goldenreplay.golden.json)
if [ "$DRY" -eq 0 ] && { ! git -C "$REPO" diff --quiet -- "${TARGETS[@]}" \
                         || ! git -C "$REPO" diff --cached --quiet -- "${TARGETS[@]}"; }; then
  echo "FATAL: ${TARGETS[*]} have uncommitted changes — commit or stash first." >&2; exit 1
fi

rpc_call () { # rpc_call URL METHOD PARAMS_JSON -> .result (raw JSON); fails on error/null
  curl -s --max-time 20 -X POST "$1" -H 'content-type: application/json' \
    -d "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"$2\",\"params\":$3}" \
    | python3 -c 'import json,sys
d=json.load(sys.stdin)
r=d.get("result")
sys.exit(1) if ("error" in d or r is None) else print(json.dumps(r))'
}

# ---- 1. Resolve target height + cross-checked hash -------------------------
if [ -z "$HEIGHT" ]; then
  for url in "${RPCS[@]}"; do
    tip_hex=$(rpc_call "$url" eth_blockNumber '[]' 2>/dev/null | tr -d '"') || continue
    HEIGHT=$(( $((tip_hex)) - DEPTH ))
    echo "[repin] tip $((tip_hex)) from $url -> target height $HEIGHT (tip-$DEPTH)"
    break
  done
  [ -n "$HEIGHT" ] || { echo "[repin] FATAL: no RPC reachable for the tip" >&2; exit 1; }

  # Fetch the target block's hash from EVERY reachable RPC and demand agreement.
  target_hex=$(printf '0x%x' "$HEIGHT")
  hashes=() sources=()
  for url in "${RPCS[@]}"; do
    blk=$(rpc_call "$url" eth_getBlockByNumber "[\"$target_hex\",false]" 2>/dev/null) || continue
    h=$(echo "$blk" | python3 -c 'import json,sys; print(json.load(sys.stdin)["hash"])') || continue
    hashes+=("$h"); sources+=("$url")
    echo "[repin]   $url -> $h"
  done
  [ ${#hashes[@]} -ge 1 ] || { echo "[repin] FATAL: no RPC returned block $HEIGHT" >&2; exit 1; }
  uniq_count=$(printf '%s\n' "${hashes[@]}" | sort -u | wc -l)
  [ "$uniq_count" -eq 1 ] || { echo "[repin] FATAL: RPCs DISAGREE on the hash of block $HEIGHT — do not pin. Wait and retry." >&2; exit 1; }
  if [ ${#hashes[@]} -lt 2 ] && [ "$SINGLE_OK" -eq 0 ]; then
    echo "[repin] FATAL: only ONE RPC reachable — a single endpoint must not choose the genesis." >&2
    echo "        Rerun with --allow-single-rpc to accept, or fix connectivity." >&2
    exit 1
  fi
  HASH="${hashes[0]}"
  echo "[repin] hash agreed by ${#hashes[@]} RPC(s): $HASH"
else
  echo "[repin] explicit pin: height=$HEIGHT hash=$HASH"
fi

HASH_NOPFX=${HASH#0x}
echo "$HASH_NOPFX" | grep -qE '^[0-9a-fA-F]{64}$' \
  || { echo "[repin] FATAL: hash must be 32 bytes of hex (got: $HASH)" >&2; exit 1; }
echo "$HEIGHT" | grep -qE '^[0-9]+$' \
  || { echo "[repin] FATAL: height must be a decimal integer (got: $HEIGHT)" >&2; exit 1; }

if [ "$DRY" -eq 1 ]; then
  echo "[repin] DRY RUN — would pin height=$HEIGHT hash=0x$HASH_NOPFX. No changes made."
  exit 0
fi

# ---- 2. Rewrite the POLYGON pin in src/logic.cpp ---------------------------
python3 - "$REPO/src/logic.cpp" "$HEIGHT" "$HASH_NOPFX" <<'PYEOF'
import re, sys
path, height, hashhex = sys.argv[1], sys.argv[2], sys.argv[3]
src = open(path).read()

m = re.search(r"(Fresh Treatfighter relaunch genesis on Polygon.*?)(height\s*=\s*[0-9']+;)(.*?)(hashHex\s*=\s*\"[0-9a-fA-F]*\";)", src, re.S)
if not m:
    sys.exit("FATAL: POLYGON genesis pin block not found in logic.cpp")
# Snapshot every OTHER chain's pin (i.e. 64-hex literals OUTSIDE the matched
# POLYGON hashHex) so we can prove the edit touched none of them.  The matched
# block's own hash may legitimately change on a --force re-pin.
outside = src[:m.start(4)] + src[m.end(4):]
other_pins = re.findall(r'"([0-9a-fA-F]{64})"', outside)
# The gap between height and hashHex must be pure whitespace INSIDE the POLYGON
# case — if the lazy match walked across a `break;`/`case`, we'd be about to
# clobber a DIFFERENT chain's pin (e.g. MAIN). Refuse.
if re.search(r"break|case|:", m.group(3)):
    sys.exit("FATAL: pin block regex crossed a case boundary — logic.cpp layout "
             "changed or the current hashHex is malformed. Fix manually.")
new = src[:m.start(2)] + f"height = {height};" + m.group(3) + f'hashHex\n          = "{hashhex}";' + src[m.end(4):]
new = new.replace(
    "TODO(launch): re-set to the Polygon tip at actual launch time to\n         minimise backfill.  Consensus-critical: never change after launch.",
    "PINNED at launch by contrib/repin-genesis.sh.\n         Consensus-critical: never change after launch.")

# Post-conditions: the new pin appears exactly once; every pre-existing 64-hex
# literal (MAIN/TEST pins) is byte-unchanged.
if new.count(f"height = {height};") != 1 or new.count(f'"{hashhex}"') != 1:
    sys.exit("FATAL: post-check failed — new pin not present exactly once")
for p in other_pins:
    if p.lower() != hashhex.lower() and f'"{p}"' not in new:
        sys.exit(f"FATAL: post-check failed — pre-existing pin {p[:12]}… was altered")
open(path, "w").write(new)
print(f"[repin] logic.cpp updated: height={height} hash={hashhex} (other chain pins verified unchanged)")
PYEOF

# ---- 3. Golden regen + FULL make check in Docker (offline) -----------------
mkdir -p "$BUILD"
echo "[repin] docker: build + golden regen + make check (this takes ~10-20 min) ..."
docker run --rm --network none \
  -v "$REPO":/src-ro:ro -v "$BUILD":/build \
  --entrypoint bash "$IMAGE" -c '
    set -e
    cp -au /src-ro/. /build/
    cmp -s /src-ro/src/logic.cpp /build/src/logic.cpp || { echo SYNC_FAIL: logic.cpp did not copy; exit 1; }
    cd /build
    if [ ! -f config.status ]; then ./autogen.sh >/dev/null 2>&1 && ./configure >/dev/null 2>&1; else ./config.status >/dev/null 2>&1 || true; fi
    make -j"$(nproc)" >/tmp/make.log 2>&1 || { echo MAKE_FAIL; grep -E "error:" /tmp/make.log | head -20; exit 1; }
    cd src && GOLDEN_REGEN=1 ./goldenreplay_tests >/tmp/regen.log 2>&1 || { echo REGEN_FAIL; tail -20 /tmp/regen.log; exit 1; }
    cd /build && make check >/tmp/check.log 2>&1 && echo CHECK_ALL_PASS || {
      echo CHECK_HAD_FAILURES
      grep -rhE "\[  FAILED  \]" src/test-suite.log proto2/test-suite.log database/test-suite.log 2>/dev/null | sort -u | head -20
      exit 1
    }
  '

# ---- 4. Copy the regenerated golden back + commit ---------------------------
cp "$BUILD/src/goldenreplay.golden.json" "$REPO/src/goldenreplay.golden.json"
cd "$REPO"
git --no-pager diff --stat src/logic.cpp src/goldenreplay.golden.json
if [ "$COMMIT" -eq 1 ]; then
  git -c user.name=snailbrainx -c user.email=17693211+snailbrainx@users.noreply.github.com \
    commit -m "consensus: pin POLYGON genesis at height $HEIGHT (launch)" \
    -- src/logic.cpp src/goldenreplay.golden.json
  echo "[repin] committed. Push with: git push origin polygon-rewrite && git push origin polygon-rewrite:main"
else
  echo "[repin] --no-commit: changes left in the working tree."
fi
echo "[repin] DONE — genesis pinned at height=$HEIGHT hash=0x$HASH_NOPFX, full make check green."

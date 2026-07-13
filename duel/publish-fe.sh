#!/usr/bin/env bash
# duel/publish-fe.sh — publish the built WASM artifact + its sha256 to
# tf-frontend's public/ dir (a static asset fetched by the browser at
# runtime; see the FE bridge in Task 6, `tf-frontend/src/duel/engine.ts`).
#
# Repo layout is siblings under treatfighter/: tfgsp-polygon/duel/ and
# tf-frontend/ — so relative to this script's dir ($here = .../duel), the FE
# repo root is $here/../../tf-frontend. Overridable via FE_DIR for anyone
# with a different checkout layout.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
FE_DIR="${FE_DIR:-$here/../../tf-frontend}"

if [[ ! -f "$here/engine.wasm" || ! -f "$here/engine.wasm.sha256" ]]; then
  echo "publish-fe.sh: missing engine.wasm/engine.wasm.sha256 — run duel/build.sh wasm (or all) first" >&2
  exit 1
fi
if [[ ! -d "$FE_DIR" ]]; then
  echo "publish-fe.sh: FE_DIR '$FE_DIR' does not exist" >&2
  exit 1
fi

dest="$FE_DIR/public/duel"
mkdir -p "$dest"
cp "$here/engine.wasm" "$dest/engine.wasm"
cp "$here/engine.wasm.sha256" "$dest/engine.wasm.sha256"

echo "published to $dest:"
sha256sum "$dest/engine.wasm"
cat "$dest/engine.wasm.sha256"

#!/bin/sh
# gsp-progress-watchdog.sh — host-side cron watchdog for the production GSP.
#
# Restarts the GSP container when it stops making PROGRESS, not merely when
# its RPC dies.  Both failure modes have been observed:
#   * 2026-07-09 (fork): libxayagame aborts on a benign SQLite snapshot race
#     and the process wedges half-dead — container Up, RPC gone.  No docker
#     restart policy fires for that; only an external RPC probe catches it.
#   * 2026-07-10 (live Polygon, audit P0-02): the GSP was RESPONSIVE but
#     stalled — state "catching-up" frozen ~716k blocks behind a healthy
#     XayaX for ~12 days after missed ZMQ notifications.  A liveness-only
#     probe (grep for '"state"') reports success forever in that condition;
#     this watchdog exists because that one was blind to it.
#
# Checks, in order (a failing check => docker restart $CONTAINER + loud log):
#   a) getnullstate answers within $CURL_TIMEOUT seconds;
#   b) state == "up-to-date", OR the reported height has changed since the
#      last recorded progress ($STATE_FILE).  A stall is only declared after
#      $STALL_GRACE seconds without progress, so tight cron cadences never
#      false-positive.
#   c) the GSP height is within $MAX_LAG blocks of XayaX's getblockchaininfo
#      height.  XayaX unreachable => warn and skip (restarting the GSP cannot
#      fix XayaX, and there is no lag figure to act on).
#
# Restart-safety: the GSP resumes cleanly from committed state, so a spurious
# restart costs seconds — the checks err toward restarting.
#
# CAVEAT (deliberate deep resync): while a node intentionally backfills far
# more than $MAX_LAG blocks (e.g. a fresh volume long after launch), check (c)
# would restart it every run even though it is advancing.  For that one
# operation run with MAX_LAG=0 (disables the lag check; the progress check (b)
# still guards against a wedge) and restore the bound afterwards.
#
# Configuration — env vars, overridable by flags:
#   GSP_URL      (default http://127.0.0.1:8610)  GSP JSON-RPC endpoint
#   XAYAX_URL    (default http://127.0.0.1:8700)  XayaX JSON-RPC endpoint
#   CONTAINER    (default treatfighter-tfd-1)     container to docker-restart
#   MAX_LAG      (default 5000; 0 = disabled)     max blocks behind XayaX
#   STATE_FILE   (default /var/tmp/gsp-progress-watchdog.state)
#   STALL_GRACE  (default 240)                    seconds without progress
#   CURL_TIMEOUT (default 8)                      per-RPC timeout, seconds
# Flags: --gsp-url= --xayax-url= --container= --max-lag= --state-file=
#        --stall-grace= --curl-timeout=
#
# Install (cron, every 2 minutes):
#   */2 * * * * /path/to/contrib/gsp-progress-watchdog.sh >> /var/log/gsp-watchdog.log 2>&1
#
# Docker NEVER restarts an unhealthy container on its own (restart policies
# act on process exit, not on healthcheck state).  The compose healthcheck in
# docker/docker-compose.yml only surfaces status in `docker ps`; THIS script
# does the restarting.

set -u

GSP_URL="${GSP_URL:-http://127.0.0.1:8610}"
XAYAX_URL="${XAYAX_URL:-http://127.0.0.1:8700}"
CONTAINER="${CONTAINER:-treatfighter-tfd-1}"
MAX_LAG="${MAX_LAG:-5000}"
STATE_FILE="${STATE_FILE:-/var/tmp/gsp-progress-watchdog.state}"
STALL_GRACE="${STALL_GRACE:-240}"
CURL_TIMEOUT="${CURL_TIMEOUT:-8}"

for arg in "$@"; do
  case "$arg" in
    --gsp-url=*)      GSP_URL="${arg#*=}" ;;
    --xayax-url=*)    XAYAX_URL="${arg#*=}" ;;
    --container=*)    CONTAINER="${arg#*=}" ;;
    --max-lag=*)      MAX_LAG="${arg#*=}" ;;
    --state-file=*)   STATE_FILE="${arg#*=}" ;;
    --stall-grace=*)  STALL_GRACE="${arg#*=}" ;;
    --curl-timeout=*) CURL_TIMEOUT="${arg#*=}" ;;
    *) echo "gsp-progress-watchdog: unknown flag: $arg" >&2; exit 2 ;;
  esac
done

log ()
{
  echo "$(date -u '+%Y-%m-%dT%H:%M:%SZ') [gsp-watchdog] $*"
}

# First integer value of "<field>" in the JSON in $1 (or empty).
json_int ()
{
  printf '%s' "$1" | sed -n "s/.*\"$2\"[^0-9]*\([0-9][0-9]*\).*/\1/p" | head -n 1
}

# First string value of "<field>" in the JSON in $1 (or empty).
json_str ()
{
  printf '%s' "$1" \
    | sed -n "s/.*\"$2\"[[:space:]]*:[[:space:]]*\"\([^\"]*\)\".*/\1/p" \
    | head -n 1
}

rpc ()
{
  curl -s --max-time "$CURL_TIMEOUT" -X POST "$1" \
    -H 'content-type: application/json' \
    -d "{\"jsonrpc\":\"2.0\",\"method\":\"$2\",\"params\":[],\"id\":1}"
}

restart_gsp ()
{
  log "FAILURE: $1 — restarting container $CONTAINER"
  logger -t gsp-watchdog "FAILURE: $1 — restarting $CONTAINER" 2>/dev/null || true
  rm -f "$STATE_FILE"
  docker restart "$CONTAINER" || log "ERROR: docker restart $CONTAINER failed"
  exit 1
}

# --- (a) RPC liveness ------------------------------------------------------
resp="$(rpc "$GSP_URL" getnullstate)" || resp=""
state="$(json_str "$resp" state)"
[ -n "$state" ] \
  || restart_gsp "getnullstate gave no usable answer at $GSP_URL (response: ${resp:-<none>})"

height="$(json_int "$resp" height)"
now="$(date +%s)"

# --- (b) progress ----------------------------------------------------------
if [ "$state" = "up-to-date" ]; then
  [ -n "$height" ] && echo "$height $now" > "$STATE_FILE"
else
  [ -n "$height" ] || restart_gsp "state=$state with no height in getnullstate"

  last_height=""
  last_change=""
  [ -f "$STATE_FILE" ] && read -r last_height last_change < "$STATE_FILE"
  # Discard a corrupt/legacy state file.
  case "$last_height" in '' | *[!0-9]*) last_height="" ;; esac
  case "$last_change" in '' | *[!0-9]*) last_change="$now" ;; esac

  if [ -z "$last_height" ] || [ "$height" -ne "$last_height" ]; then
    # Any height CHANGE counts as progress (a reorg or post-restart replay may
    # briefly lower it).
    echo "$height $now" > "$STATE_FILE"
  elif [ $((now - last_change)) -ge "$STALL_GRACE" ]; then
    restart_gsp "state=$state, height stuck at $height for $((now - last_change))s (grace ${STALL_GRACE}s) — the P0-02 responsive-but-stalled wedge"
  else
    log "WARNING: state=$state, height $height unchanged for $((now - last_change))s (grace ${STALL_GRACE}s)"
  fi
fi

# --- (c) bounded lag vs XayaX ----------------------------------------------
if [ "$MAX_LAG" -eq 0 ]; then
  log "OK: state=$state height=${height:-?} (lag check disabled, MAX_LAG=0)"
  exit 0
fi

xresp="$(rpc "$XAYAX_URL" getblockchaininfo)" || xresp=""
xheight="$(json_int "$xresp" blocks)"
if [ -z "$xheight" ]; then
  log "WARNING: XayaX getblockchaininfo unreachable at $XAYAX_URL — skipping the lag check (restarting the GSP would not fix XayaX)"
elif [ -n "$height" ]; then
  lag=$((xheight - height))
  [ "$lag" -le "$MAX_LAG" ] \
    || restart_gsp "GSP height $height lags XayaX $xheight by $lag blocks (MAX_LAG=$MAX_LAG)"
  log "OK: state=$state height=$height xayax=$xheight lag=$lag"
else
  log "OK: state=$state (no height reported yet) xayax=$xheight"
fi

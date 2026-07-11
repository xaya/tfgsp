#!/bin/sh -e

# Entry point for the Treatfighter GSP on the XayaX / Polygon stack.
# The XayaX RPC URL and ZMQ address are passed as extra args ("$@"),
# typically from docker-compose, e.g.:
#   --xaya_rpc_url=http://xayax:8000 --xaya_zmq_address=tcp://xayax:28555

case $1 in
  tfd)
    shift
    # Preflight (audit P2-23 follow-up): the image runs unprivileged (uid 10001
    # 'tfd').  A named/anon volume created by an OLDER root-running image is
    # owned by root, and Docker does NOT re-chown an existing volume on upgrade,
    # so tfd could not write its datadir/log and libxayagame would crash-loop
    # opaquely.  Fail loud with the fix instead of looping.  (A fresh deploy is
    # unaffected: Docker seeds a new volume with the image dir's tfd ownership.)
    for _d in "${XAYAGAME_DIR}" "${GLOG_log_dir:-/log}"; do
      if [ ! -w "${_d}" ]; then
        echo "FATAL: ${_d} is not writable by uid $(id -u) (tfd)." >&2
        echo "       A volume from an older root-owned image is not re-chowned on upgrade." >&2
        echo "       Fix on the host (one-off): " >&2
        echo "         docker run --rm -v <volume-name>:/v busybox chown -R 10001:10001 /v" >&2
        echo "       or recreate the volume for a fresh deploy.  See docs/launch-runbook.md." >&2
        exit 1
      fi
    done
    exec /usr/local/bin/tfd \
      --datadir="${XAYAGAME_DIR}" \
      --xaya_rpc_protocol=2 \
      --enable_pruning=1000 \
      --pending_moves=false \
      --game_rpc_port=8600 \
      --game_rpc_listen_locally=false \
      "$@"
    ;;

  *)
    echo "Provide \"tfd\" as command"
    exit 1
    ;;
esac

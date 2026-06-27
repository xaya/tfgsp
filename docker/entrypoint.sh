#!/bin/sh -e

# Entry point for the Treatfighter GSP on the XayaX / Polygon stack.
# The XayaX RPC URL and ZMQ address are passed as extra args ("$@"),
# typically from docker-compose, e.g.:
#   --xaya_rpc_url=http://xayax:8000 --xaya_zmq_address=tcp://xayax:28555

case $1 in
  tfd)
    shift
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

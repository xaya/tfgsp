#!/bin/sh -e

version="0.1"
exclude="waitforchange,waitforpendingchange"

case $1 in
  tfd)
    shift
    exec /usr/local/bin/tfd \
      --datadir="${XAYAGAME_DIR}" \
      --enable_pruning=1000 \
      --game_rpc_port=8600 \
      "$@"
    ;;

  charon-client)
    shift
    exec /usr/local/bin/charon-client \
      --waitforchange --waitforpendingchange \
      --port=8600 \
      --backend_version="${version}" \
      --methods_json_spec="/usr/local/share/tf-rpc.json" \
      --methods_exclude="${exclude}" \
      --cafile="/usr/local/share/letsencrypt.pem" \
      "$@"
    ;;

  charon-server)
    shift
    exec /usr/local/bin/charon-server \
      --waitforchange --waitforpendingchange \
      --backend_version="${version}" \
      --methods_json_spec="/usr/local/share/tf-rpc.json" \
      --methods_exclude="${exclude}" \
      --cafile="/usr/local/share/letsencrypt.pem" \
      "$@"
    ;;

  *)
    echo "Provide \"tfd\", \"charon-client\" or \"charon-server\" as command"
    exit 1
esac

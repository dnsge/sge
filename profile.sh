#!/usr/bin/env bash

set -e

sudo -v

PROJ_DIR="$(dirname "$0")"

"$PROJ_DIR/build/bin/sge" &
PID=$!

sleep 0.5
flamegraph --root --deterministic -p "$PID" &

if [[ "$#" -eq 0 ]]; then
    echo "Waiting for process $PID to finish"
    wait "$PID"
else
    echo "Waiting $1 seconds"
    sleep $1
    kill -9 "$PID"
fi

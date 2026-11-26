#!/bin/sh
set -eu

export PATH="$SNAP/usr/bin:$SNAP/usr/sbin:$PATH"

export LD_LIBRARY_PATH="$SNAP/usr/lib/x86_64-linux-gnu:$SNAP/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

mkdir -p "$SNAP_COMMON/bluealsa"

exec "$SNAP/usr/bin/bluealsad" \
    --profile=a2dp-source \
    --profile=hfp-ag \
    --device=hci0 \
    --loglevel=info
#!/bin/bash

set -euo pipefail

engine="$(modelctl status --format=json | jq -r .engine)"
exec modelctl run -- "$SNAP/engines/$engine/whisper-live-server.sh" "$@"

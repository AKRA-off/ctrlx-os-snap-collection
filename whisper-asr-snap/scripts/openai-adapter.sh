#!/bin/bash

set -euo pipefail

engine="$(modelctl status --format=json | jq -r .engine)"
exec modelctl run --share-provider -- "$SNAP/engines/$engine/openai-adapter.sh" "$@"

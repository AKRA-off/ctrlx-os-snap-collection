#!/bin/sh
set -e

# Preserve the original default port.
if [ -z "${PORT:-}" ]; then
    PORT=1891
fi
export PORT

# Wait until the actual solutions storage is connected.
while ! snapctl is-connected active-solution; do
    echo "Waiting for active-solution interface..."
    sleep 5
done

ACTIVE_CONFIGURATION="$SNAP_COMMON/solutions/activeConfiguration"

while [ ! -d "$ACTIVE_CONFIGURATION" ]; do
    echo "Waiting for active configuration..."
    sleep 5
done

# ALSA / BlueALSA configuration.
#
# Recreate the ALSA configuration on every Node-RED start.
# This guarantees that the included BlueALSA configuration points
# to the currently installed snap revision.
ALSA_CONFIG="$SNAP_DATA/alsa-bluealsa.conf"
ALSA_BLUEALSA_CONF="$SNAP/usr/share/alsa/alsa.conf.d/20-bluealsa.conf"

# Verify that both required configuration files exist.
if [ ! -f "$SNAP/usr/share/alsa/alsa.conf" ]; then
    echo "ERROR: Cannot find $SNAP/usr/share/alsa/alsa.conf" >&2
    exit 1
fi

if [ ! -f "$ALSA_BLUEALSA_CONF" ]; then
    echo "ERROR: Cannot find $ALSA_BLUEALSA_CONF" >&2
    exit 1
fi

# Start with the standard ALSA configuration.
cp "$SNAP/usr/share/alsa/alsa.conf" "$ALSA_CONFIG"

# Include the BlueALSA PCM configuration.
printf '\n<%s>\n' "$ALSA_BLUEALSA_CONF" >> "$ALSA_CONFIG"

# Make ALSA and BlueALSA available to Node-RED and all processes
# launched by Node-RED, including Exec nodes.
export ALSA_CONFIG_PATH="$ALSA_CONFIG"
export ALSA_PLUGIN_DIR="$SNAP/usr/lib/x86_64-linux-gnu/alsa-lib"

# Store this app's data inside the active configuration.
USER_DIR="$ACTIVE_CONFIGURATION/node-red-custom"
mkdir -p "$USER_DIR"

# Initialize settings only when this configuration has no settings.js.
# Preserve the original template location, with a packaged fallback.
if [ ! -f "$USER_DIR/settings.js" ]; then
    if [ -f "$SNAP_DATA/Flask/settings.js" ]; then
        cp "$SNAP_DATA/Flask/settings.js" "$USER_DIR/settings.js"
    elif [ -f "$SNAP/bin/Flask/settings.js" ]; then
        cp "$SNAP/bin/Flask/settings.js" "$USER_DIR/settings.js"
    else
        echo "ERROR: Cannot find the initial settings.js template." >&2
        exit 1
    fi
fi

cd "$USER_DIR"

echo "NOW STARTING NODE-RED"
echo "User directory: $USER_DIR"
echo "Flows file: $USER_DIR/flows.json"
echo "Port: $PORT"
echo "ALSA config: $ALSA_CONFIG_PATH"
echo "ALSA plugin directory: $ALSA_PLUGIN_DIR"

exec "$SNAP/bin/node" \
    "$SNAP/lib/node_modules/.bin/node-red" \
    --userDir "$USER_DIR" \
    --settings "$USER_DIR/settings.js" \
    --port "$PORT" \
    "$USER_DIR/flows.json"
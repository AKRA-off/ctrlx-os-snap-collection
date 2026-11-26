#!/bin/bash

set -e

BACKEND_HOST=$(modelctl get whisper-live.ws.host)
BACKEND_PORT=$(modelctl get whisper-live.ws.port)
ADAPTER_HOST=$(modelctl get http.host)
ADAPTER_PORT=$(modelctl get http.port)

language=$(modelctl get transcription-language)

echo "Launching adapter..."

# Get the alias of the active model
active_model_alias=$(modelctl show-model --format=json | jq -r .alias)

# If language is set to "system", try to detect the system language and use it as default.
# If the detected language is not supported by the model, fall back to the first supported language.
if [ "$language" == "system" ]; then
    # extract language code from locale, e.g. "en_US.UTF-8" -> "en"; "en.UTF-8" -> "en"; "C.UTF-8" -> "C"
    language="${LANG%%[_.]*}"
    echo "Detected system language: $language"

    # validate language code against the list of supported languages
    supported_langs="${MODEL_SUPPORTED_LANGUAGES//\"/}"
    if ! echo "$supported_langs" | tr ',' '\n' | grep -qxF "$language"; then
        echo "System language '$language' is not supported by the model. Falling back to the first supported language."
        language=$(echo "$supported_langs" | cut -d, -f1 | tr -d ' ')
    fi
fi

status_json=$(modelctl status --format=json)
unix_socket_path=$(echo "$status_json" | jq -r '.entrypoints.myna."unix-socket"')
mkdir -p "$(dirname "$unix_socket_path")"

set -x
$SNAP/bin/whisperlive-adapter serve \
    --backend-host "$BACKEND_HOST" \
    --backend-port "$BACKEND_PORT" \
    --host "$ADAPTER_HOST" \
    --port "$ADAPTER_PORT" \
    --unix-socket "$unix_socket_path" \
    --model "$active_model_alias" \
    --language "$language" \
    --allowed-models "$active_model_alias" \
    --allowed-languages "${MODEL_SUPPORTED_LANGUAGES//\"/}"

set +x
echo "Adapter terminated."

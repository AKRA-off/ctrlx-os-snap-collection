#!/bin/bash

set -e

HOST=$(modelctl get whisper-live.ws.host)
PORT=$(modelctl get whisper-live.ws.port)
BACKEND="openvino" # options: "tensorrt", "faster_whisper", "openvino"

NPROC=$(nproc)
CLIENT_MAX_CONNECTION_TIME=2147483647 #int32 max, about 64 years

echo "Activating python venv..."
source activate 

echo "Launching engine..."


# The server implementation does not allow loading a OpenVINO model from disk. 
# Instead, it accepts a huggingface model identifier and downloads the model automatically.
# But first it will check if the model is already available in a hardcoded cache directory in $HOME.
# This workaround ensures that the model is found and no download takes place.
# See: https://github.com/collabora/WhisperLive/blob/99cbc1c33b35c372b4790975f819dfde62f3e74a/whisper_live/transcriber/transcriber_openvino.py#L11


mock_home="/tmp/mock_home"
models_dir="$mock_home"/.cache/openvino_whisper_models
rm -rf "$models_dir"
mkdir -p "$models_dir"

active_model_alias=$(modelctl show-model --format=json | jq -r .alias)
ln -s "$MODEL_DIR" "$models_dir/$active_model_alias"

# Batch inference is used to force single model mode

set -x
HOME="$mock_home" python3 "$SERVER_RUN_SCRIPT" \
    --batch_inference \
    --cache_path "$MODEL_DIR" \
    --host "$HOST" \
    --port "$PORT" \
    --backend "$BACKEND" \
    --max_connection_time "$CLIENT_MAX_CONNECTION_TIME" \
    --omp_num_threads "$NPROC"

set +x
echo "Engine terminated."

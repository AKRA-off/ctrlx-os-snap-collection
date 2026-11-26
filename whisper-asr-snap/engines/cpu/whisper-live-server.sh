#!/bin/bash

set -e

HOST=$(modelctl get whisper-live.ws.host)
PORT=$(modelctl get whisper-live.ws.port)
BACKEND="faster_whisper" # options: "tensorrt", "faster_whisper", "openvino"

NPROC=$(nproc)
CLIENT_MAX_CONNECTION_TIME=2147483647 #int32 max, about 64 years

echo "Activating python venv..."
source activate 

echo "Launching engine..."

export CUDA_VISIBLE_DEVICES="" # Prevent automatic GPU detection

set -x
python3 "$SERVER_RUN_SCRIPT" \
    --cache_path "$MODEL_DIR" \
    --faster_whisper_custom_model_path "$MODEL_DIR" \
    --host "$HOST" \
    --port "$PORT" \
    --backend "$BACKEND" \
    --max_connection_time "$CLIENT_MAX_CONNECTION_TIME" \
    --omp_num_threads "$NPROC"

set +x
echo "Engine terminated."

#!/bin/sh
set -eu

ACTIVE_CONFIG="$SNAP_COMMON/solutions/activeConfiguration/comfyui"

export HOME="$ACTIVE_CONFIG"
export XDG_CACHE_HOME="$ACTIVE_CONFIG/cache"
export HF_HOME="$ACTIVE_CONFIG/cache/huggingface"
export TORCH_HOME="$ACTIVE_CONFIG/cache/torch"
export TMPDIR="$ACTIVE_CONFIG/temp"

export OMP_NUM_THREADS=2
export MKL_NUM_THREADS=2

for dir in \
  input \
  output \
  user \
  temp \
  cache \
  workflows \
  custom_nodes \
  models/checkpoints \
  models/diffusion_models \
  models/unet \
  models/text_encoders \
  models/clip \
  models/vae \
  models/loras \
  models/embeddings \
  models/controlnet \
  models/upscale_models
do
  mkdir -p "$ACTIVE_CONFIG/$dir"
done

cd "$ACTIVE_CONFIG"

exec "$SNAP/venv/bin/python3" "$SNAP/opt/comfyui/main.py" \
  --cpu \
  --listen 0.0.0.0 \
  --port 8188 \
  --disable-auto-launch \
  --base-directory "$ACTIVE_CONFIG" \
  --input-directory "$ACTIVE_CONFIG/input" \
  --output-directory "$ACTIVE_CONFIG/output" \
  --temp-directory "$ACTIVE_CONFIG/temp" \
  --user-directory "$ACTIVE_CONFIG/user" \
  "$@"
# ComfyUI CPU snap: experimental ctrlX OS test

Not built or run on ctrlX OS by the author. No GPU, models, GGUF node,
ComfyUI Manager or FFmpeg is bundled. This intentionally uses an older pinned
ComfyUI v0.3.26 baseline for the first packaging test, not a production release.
Python dependencies are partially constrained; build-packages.txt records the
resolved set. Full reproducibility requires a complete lockfile after validation.

## Build on Ubuntu 24.04 amd64

sudo snapcraft pack --destructive-mode

Destructive mode installs build packages into this build machine. Use a dedicated
build VM. Internet is required during the build. No CUDA/ROCm wheel index is used.

## Install on ctrlX OS

sudo snap install --dangerous ./comfyui_0.1-cpu_amd64.snap
snap services comfyui
sudo snap logs comfyui -n=100

The test service binds all interfaces on TCP 8188, without authentication or TLS.
Use an isolated trusted test network only. For local-only access change the
launcher to --listen 127.0.0.1. Device firewall rules may also require configuration.

## Test on the device (ordinary SSH shell, not a snap shell)

python3 tests/smoke-test.py

Or run the script from another computer with the device base URL as its argument.
The smoke test creates a 64x64 solid image using built-in nodes, not a diffusion
model. It validates HTTP, queue execution, CPU tensors and output storage only.
A successful test does not validate FLUX, LoRA, GGUF, CUDA or Open WebUI generation.

## Persistent data

/var/snap/comfyui/common/models
/var/snap/comfyui/common/input
/var/snap/comfyui/common/output
/var/snap/comfyui/common/user
/var/snap/comfyui/common/workflows

No automatic workflow loader is implemented. Submit API-format workflow JSON
through /prompt. No models are downloaded by this project.

## Diagnostics

snap run --shell comfyui
"$SNAP/usr/bin/python3.12" -c 'import torch; print(torch.__version__); print(torch.version.cuda); print(torch.ones(2) + 1)'
exit

If QEMU fails with a CPU instruction/baseline error, retain the traceback.
CPU-only PyTorch does not guarantee compatibility with every emulated CPU.

## Rebuild after editing the launcher

sudo snapcraft clean launcher
sudo snapcraft pack --destructive-mode

## Known scope limitations

The writable base directory makes custom_nodes writable but this build installs
no custom nodes. GPU and GGUF support will be separate validated changes.
ComfyUI source is GPL-3.0; bundled dependencies retain their own licences.

## Origin and license

Packages [Comfy-Org/ComfyUI](https://github.com/Comfy-Org/ComfyUI) (fetched from upstream at build time) and includes [1038lab/ComfyUI-Pollinations](https://github.com/1038lab/ComfyUI-Pollinations). Not affiliated with or endorsed by the ComfyUI authors.

This folder: GPL-3.0 (see `LICENSE` and `NOTICE`). Model weights are not bundled and carry their own licenses.

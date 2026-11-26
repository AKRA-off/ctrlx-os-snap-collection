# Third-party notices and per-folder licenses

The root `LICENSE.txt` (MIT) applies unless a folder contains its own license.

| Folder | License of the folder | Upstream / components |
|---|---|---|
| `ctrlX-contour-cnc-path`, `Modbus_RTU_snap`, `unity3d-snap` | MIT (root) | Bosch Rexroth Data Layer binaries keep their own terms (see root `LICENSE.txt`) |
| `ctrlX-yolo-Instance_segmentation_ONNX_HEF` | AGPL-3.0 (code is MIT, bundled YOLOv8 models are AGPL-3.0) | Ultralytics YOLOv8 weights |
| `moveit-mtc_kassow_onrobot` | MIT (root); `src/*/LICENSE.txt` for upstream code | MoveIt Task Constructor |
| `qwen3-snap` | GPL-3.0 (packaging, fork of [canonical/qwen3-snap](https://github.com/canonical/qwen3-snap)) | Qwen 3 model: Apache-2.0 (`LICENSE-qwen3`) |
| `whisper-asr-snap` | GPL-3.0 (packaging, fork of [canonical/whisper-snap](https://github.com/canonical/whisper-snap)) | WhisperLive: MIT; Whisper models: Apache-2.0 (`LICENSE-whisper`) |
| `comfyui-snap` | GPL-3.0 | ComfyUI: GPL-3.0 (fetched at build time); ComfyUI-Pollinations: GPL-3.0 |
| `ctrlX-vision-obb` | AGPL-3.0 | Weights created with Ultralytics YOLO (AGPL-3.0); `roLabelImg/` is MIT ([cgvict/roLabelImg](https://github.com/cgvict/roLabelImg), from [tzutalin/labelImg](https://github.com/tzutalin/labelImg)) |
| `node-red-snap` | Riboni non-commercial license (`LICENSE`), **not MIT** | [mauringo/node-red-industrial-snap](https://github.com/mauringo/node-red-industrial-snap), fork of [dceejay/nodered.snap](https://github.com/dceejay/nodered.snap) (Apache-2.0); Node-RED: Apache-2.0 |
| `mysql-snap` | Apache-2.0 (packaging, fork of [canonical/mysql-snap](https://github.com/canonical/mysql-snap)) | MySQL Server 8.0: GPL-2.0 (Ubuntu package, not stored here) |

Notes:
- Model weights and downloaded models carry their own licenses; check them before commercial use.
- `ctrlX-vision-obb` is AGPL-3.0: network users must be offered the source.
- `node-red-snap` does not permit commercial use.

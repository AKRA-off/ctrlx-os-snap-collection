#!/usr/bin/env python3
"""Submit a model-free EmptyImage -> SaveImage workflow."""
import json
import sys
import time
import urllib.error
import urllib.parse
import urllib.request

base = (sys.argv[1] if len(sys.argv) > 1 else "http://127.0.0.1:8188").rstrip("/")
opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))

def request(path, data=None):
    body = None if data is None else json.dumps(data).encode()
    req = urllib.request.Request(base + path, data=body,
                                 headers={"Content-Type": "application/json"})
    try:
        with opener.open(req, timeout=15) as response:
            return json.load(response)
    except urllib.error.HTTPError as error:
        raise RuntimeError(f"HTTP {error.code}: {error.read().decode(errors='replace')}") from error

print("System:", json.dumps(request("/system_stats"), indent=2))
nodes = request("/object_info")
for name in ("EmptyImage", "SaveImage"):
    if name not in nodes:
        raise SystemExit(f"Required built-in node not loaded: {name}")
workflow = {
    "1": {"class_type": "EmptyImage", "inputs": {
        "width": 64, "height": 64, "batch_size": 1, "color": 16711680}},
    "2": {"class_type": "SaveImage", "inputs": {
        "images": ["1", 0], "filename_prefix": "smoke-test"}}
}
result = request("/prompt", {"prompt": workflow})
if result.get("node_errors"):
    raise SystemExit(json.dumps(result, indent=2))
prompt_id = result["prompt_id"]
print("Queued:", prompt_id)
for _ in range(60):
    entry = request("/history/" + urllib.parse.quote(prompt_id)).get(prompt_id)
    if entry:
        if entry.get("status", {}).get("status_str") == "error":
            raise SystemExit(json.dumps(entry, indent=2))
        images = entry.get("outputs", {}).get("2", {}).get("images", [])
        if images:
            print("PASS: model-free workflow saved an image:")
            print(json.dumps(images, indent=2))
            break
    time.sleep(1)
else:
    raise SystemExit("No saved image reported. Inspect snap logs comfyui -n=100.")

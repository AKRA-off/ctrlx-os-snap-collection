# Myna Adapter

Rely on existing transcription projects while exposing an OpenAI-compliant transcription API.

## Run the Server

The server needs a running backend, and can bind to a TCP socket, a Unix domain socket, or both at the same time. At least one of `--port` or `--unix-socket` must be specified.

### TCP mode

Run with custom host and port (TCP is only enabled when `--port` is set):

```bash
go run ./cmd/whisperlive-adapter serve --host 0.0.0.0 --port 8080
```

Quick liveness check:

```bash
curl http://127.0.0.1:8080/
```

### Unix socket mode

Bind to a Unix domain socket path:

```bash
go run ./cmd/whisperlive-adapter serve --unix-socket /tmp/myna-adapter.sock
```

Quick liveness check over Unix socket:

```bash
curl --unix-socket /tmp/myna-adapter.sock http://localhost/
```

To bind both a TCP socket and a Unix domain socket at the same time, set both `--port` and `--unix-socket`.

### Backend configuration

The server will reach a running backend at the default address of `127.0.0.1:9090`. 
This value can be changed by setting `--backend-host` and `--backend-port`.

You can quickly launch a [WhisperLive](https://github.com/collabora/WhisperLive) backend via docker:

```bash
sudo docker run -e OMP_NUM_THREADS=$(nproc) -it -p 9090:9090 ghcr.io/collabora/whisperlive-cpu:latest 
```

For more information about launching a WhisperLive docker image on dedicated hardware, see the dedicated [documentation page](https://github.com/collabora/WhisperLive#whisper-live-server-in-docker).

The default model and language can be changed by setting `--model` and `--language`.
To restrict which models and languages are allowed, use `--allowed-models` and `--allowed-languages`.

```bash
go run ./cmd/whisperlive-adapter serve \
	--unix-socket /tmp/myna-adapter.sock \
	--backend-host 127.0.0.1 \
	--backend-port 9090 \
	--model small \
	--language en \
	--allowed-models "small,tiny" \
	--allowed-languages "auto,en,fr"
```

The models listed in `--allowed-models` are exposed via an OpenAI-compatible `/v1/models` endpoint:

```json
{
  "object": "list",
  "data": [
    { "id": "small", "object": "model", "created": 1717000000, "owned_by": "myna-adapter" },
    { "id": "tiny", "object": "model", "created": 1717000000, "owned_by": "myna-adapter" }
  ]
}
```

## Debugging

This project includes a debug entry point to run inference directly against the backend or through a running instance of the Myna Adapter.

### Prompting the backend

The `use-backend` command connects directly to a backend and prints the final transcript for a local audio file. This procedure requires `ffmpeg`.

Basic usage:

```bash
go run ./cmd/debug use-backend --audio test/samples/jfk.flac
```

Example with custom model, language, and timeout:

```bash
go run ./cmd/debug use-backend \
	--host 127.0.0.1 \
	--port 9090 \
	--audio test/samples/jfk.flac \
	--model small \
	--lang en \
	--timeout-sec 180
```

Expected output shape:

```text
connecting to whisper live at 127.0.0.1:9090
streaming audio file: test/samples/jfk.flac
final transcript:
<transcribed text>
```

### Prompting the Myna Adapter

The `use-adapter` command streams audio to a running Myna Adapter server and prints the transcription events as they arrive. The procedure requires `ffmpeg`.

**Run over TCP**:

```bash
go run ./cmd/debug use-adapter --url ws://127.0.0.1:8080/v1/realtime
```

**Run over a Unix domain socket**:

```bash
go run ./cmd/debug use-adapter --unix-socket /tmp/myna-adapter.sock
```

#### Choosing the recording device

The application will open the default streaming device, but you can optionally override that by specifiyng the `--audio-device` argument:

```bash
go run ./cmd/debug use-adapter --audio-device alsa_input.pci-0000_c4_00.6.Hi
```

The available devices can be listed by running `pactl list sources`.

> **NOTE**: the `--audio-device` and `--audio-file` arguments are mutually exclusive and cannot be set at the same time.

#### Audio file streaming

To enable more repducible tests you can also stream audio from a local file, just set the `--audio-file` argument to the file path:

```bash
go run ./cmd/debug use-adapter --unix-socket /tmp/myna-adapter.sock --audio-file test/samples/jfk.flac --realtime-factor 3.0
```

The `--realtime-factor` realtime argument is an optional parameter used to speed up transcription at a rate higher than realtime.

> **NOTE**: the `--audio-device` and `--audio-file` arguments are mutually exclusive and cannot be set at the same time.

## Origin and license

Fork of [canonical/whisper-snap](https://github.com/canonical/whisper-snap) at commit `f567bc3` (2026-10-06), modified for ctrlX OS. Not affiliated with or endorsed by Canonical or OpenAI.

Snap packaging: GPL-3.0 (see `LICENSE`). WhisperLive: MIT. Whisper models: Apache-2.0 (see `LICENSE-whisper` and `NOTICE`).

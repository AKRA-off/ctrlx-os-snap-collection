# OpenAI WebSocket API Spec Notes

This directory contains AI-generated documentation for the Myna subset of OpenAI specification.

## What Myna Implements

Myna is a **subset-compatible** interface of the OpenAI Realtime transcription protocol with local-first behavior.

Key points:

- Transport is WebSocket over TCP or Unix Domain Socket.
- Audio input is PCM16 mono.
- Audio chunks are sent as base64 inside JSON events.
- Session configuration is done through `session.update` patch/merge semantics.
- Transcription is streamed through delta events and finalized with a completed event.

## Compatibility and Deviations from OpenAI Realtime

According to the spec, notable deviations are:

- No required Authorization header.
- No required session model query parameter in URL.
- Turn detection defaults differ: OpenAI defaults to `server_vad` behavior for conversational mode, Myna has turn detection disabled by default.

The PDF also contains an "Example using OpenAI Realtime API" with many additional events. Those examples are informative, but not all events are in scope for Myna.

## Supported Event Types (Authoritative)

The JSON Schema `oneOf` list in the spec defines the supported payload families.

### Session lifecycle/configuration

- `session.created` (server -> client)
- `session.update` (client -> server)
- `session.updated` (server -> client)

### Audio input control

- `input_audio_buffer.append` (client -> server)
- `input_audio_buffer.commit` (client -> server)

### Model lifecycle

- `model.loaded` (server -> client)
- `model.unloaded` (server -> client)

### Transcription output

- `conversation.item.input_audio_transcription.delta` (server -> client)
- `conversation.item.input_audio_transcription.completed` (server -> client)

### Error

- `error` (both directions according to spec narrative)

## Command References

Detailed references are in `docs/spec/commands/`:

- `session.created.md`
- `session.update.md`
- `session.updated.md`
- `input_audio_buffer.append.md`
- `input_audio_buffer.commit.md`
- `model.loaded.md`
- `model.unloaded.md`
- `conversation.item.input_audio_transcription.delta.md`
- `conversation.item.input_audio_transcription.completed.md`
- `error.md`

## End-to-End Flow (Spec Sequence)

1. Client opens WebSocket.
2. Server sends `session.created` with defaults.
3. Client sends `session.update` with desired overrides.
4. Server replies with `session.updated` (or `error` if rejected).
5. During streaming:
   - Client repeatedly sends `input_audio_buffer.append`.
   - Server streams transcription `delta` events.
   - Client signals speech/end chunk with `input_audio_buffer.commit`.
   - Server emits one `completed` event for the chunk.
6. Client closes connection.

## Data Model Constraints Worth Enforcing

From the schema text in the PDF:

- `session.type` is constant `realtime`.
- `audio.input.format.rate` is integer, minimum 1.
- `audio.input.transcription.language` must match `^[a-z]{2}$` (ISO-639-1 lowercase 2-letter code).
- `input_audio_buffer.append.audio` is base64-encoded PCM16 bytes.
- `include` currently supports `item.input_audio_transcription.logprobs`.
- Transcription delta/completed require `item_id` and `content_index`.
- Error model supports:
  - `invalid_request_error` with codes `unknown_parameter`, `invalid_parameter`
  - `server_error` with codes `server_error`, `no_model_error`

## Important Scope Note for Implementers

The OpenAI example section in the PDF includes extra events NOT in scope for Myna, such as:

- `input_audio_buffer.speech_started`
- `input_audio_buffer.speech_stopped`
- `input_audio_buffer.committed`
- `conversation.item.added`
- `conversation.item.done`

These are useful for future roadmap thinking, but they are **not included** in the current authoritative schema `oneOf` set and should be treated as out-of-scope unless the spec is expanded.

## Future Implementation Checklist (Non-Code)

When implementation starts, validate against this checklist:

- Define strict inbound/outbound event parsing by `type` discriminator.
- Apply session updates as patch/merge over server defaults.
- Preserve event ordering per connection where required by flow semantics.
- Handle streaming partial transcript accumulation and finalized transcript dispatch.
- Gate optional `logprobs` population on `session.include`.
- Enforce structured error responses with typed code/message.
- Keep OpenAI-only example events behind explicit feature flags or out-of-scope handling.

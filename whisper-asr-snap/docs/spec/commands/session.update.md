# session.update

## Purpose

Client-to-server event used to override server defaults after `session.created`.

The spec states updates are applied as a **patch/merge** over the existing configuration, not a full replacement.

## Direction

- Client -> Server

## Type Discriminator

```json
{
  "type": "session.update"
}
```

## Payload Shape

```json
{
  "type": "session.update",
  "session": {
    "type": "realtime",
    "instructions": "optional string",
    "prompt": "optional string",
    "audio": {
      "input": {
        "format": {
          "rate": 16000
        },
        "transcription": {
          "model": "whisper-large",
          "language": "en"
        }
      }
    },
    "include": [
      "item.input_audio_transcription.logprobs"
    ]
  }
}
```

## Field Constraints

From schema:

- `session.type` must be `realtime`.
- `audio.input.format.rate` is integer with minimum `1`.
- `audio.input.transcription.language` must match `^[a-z]{2}$`.
- `include` currently supports only `item.input_audio_transcription.logprobs`.

## Semantics

- Partial objects should be merged into current server session state.
- Unknown keys should produce `error` with `invalid_request_error/unknown_parameter`.
- Invalid values should produce `error` with `invalid_request_error/invalid_parameter`.

## Example Intent Patterns

- Lower input rate from 24kHz to 16kHz.
- Change transcription model.
- Pin language to improve recognition stability.
- Request token-level logprobs.

## Implementation Notes for Future Work

- Define deterministic deep-merge behavior for nested config blocks.
- Document whether `null` clears existing fields or is treated as invalid.
- Preserve and echo effective config through `session.updated` after merge.

## Related Events

- Sent after `session.created`.
- Successful application yields `session.updated`.
- Failed application yields `error`.

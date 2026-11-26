# session.created

## Purpose

Server-to-client event sent immediately after connection is established. It communicates the server default session configuration.

## Direction

- Server -> Client

## Type Discriminator

```json
{
  "type": "session.created"
}
```

## Payload Shape

```json
{
  "type": "session.created",
  "session": {
    "type": "realtime",
    "instructions": "string (optional in schema but present in practice)",
    "prompt": "string|null",
    "audio": {
      "input": {
        "format": {
          "rate": 24000
        },
        "transcription": {
          "model": "whisper-small",
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

## Field Notes

- `session.type` must be `realtime`.
- `session.prompt` may be `null` in examples, even though schema text shows `string`.
- `session.audio.input.format.rate` is the negotiated/active sample rate.
- `session.audio.input.transcription.model` indicates active transcription model.
- `session.include` controls optional output enrichments, such as logprobs.

## Client Behavior Expectations

- Treat this event as the source of truth for default config.
- Build a local session state object from this payload.
- Send `session.update` only for values that must change.

## Validation Rules

- Reject if `type` is not exactly `session.created`.
- Reject if `session` is missing.
- Validate nested `session` using the same config rules as `session.update` and `session.updated`.

## Relationship to Other Events

- First event in normal handshake.
- Followed by optional client `session.update`.
- If update accepted, expect `session.updated`; otherwise expect `error`.

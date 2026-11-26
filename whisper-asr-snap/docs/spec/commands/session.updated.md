# session.updated

## Purpose

Server-to-client acknowledgment event emitted when a `session.update` is accepted and applied.

## Direction

- Server -> Client

## Type Discriminator

```json
{
  "type": "session.updated"
}
```

## Payload Shape

```json
{
  "type": "session.updated",
  "session": {
    "type": "realtime",
    "instructions": "string",
    "prompt": "string|null",
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

## Semantics

- Represents the effective session config after merge.
- Should be interpreted by client as authoritative current state.
- May include inherited defaults not present in client patch payload.

## Client Handling

- Replace local session cache with `session` from this event.
- Reconfigure client-side assumptions (sample rate, language metadata, optional logprob handling) to match server state.

## Validation Rules

- Must include `type` and `session`.
- `session` should satisfy same constraints as in `session.created`/`session.update`.

## Failure Path

If update is rejected, this event is not sent; server should emit `error` instead.

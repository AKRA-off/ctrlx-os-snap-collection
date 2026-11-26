# input_audio_buffer.commit

## Purpose

Client-to-server control event indicating the client has finished sending chunks for the current buffered audio segment.

## Direction

- Client -> Server

## Type Discriminator

```json
{
  "type": "input_audio_buffer.commit"
}
```

## Payload Shape

```json
{
  "type": "input_audio_buffer.commit"
}
```

## Semantics

- Triggers server-side finalization of the accumulated input buffer.
- Marks the boundary of an utterance/chunk for transcription completion.
- After commit, server should emit final `conversation.item.input_audio_transcription.completed` for that chunk.

## Validation Rules

- No additional fields required by schema.
- Unknown extra fields should be rejected or ignored consistently (recommended: reject with `unknown_parameter` for strictness).

## Ordering Expectations

- Must follow at least one `input_audio_buffer.append` in standard flow.
- Multiple commits with no new audio should have deterministic handling (either explicit no-op or explicit error).

## Related Events

- Preceded by one or more `input_audio_buffer.append`.
- Followed by server transcription completion event.

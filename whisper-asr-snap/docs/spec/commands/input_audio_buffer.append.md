# input_audio_buffer.append

## Purpose

Client-to-server streaming event carrying a chunk of input audio.

This event is sent repeatedly while audio is being captured.

## Direction

- Client -> Server

## Type Discriminator

```json
{
  "type": "input_audio_buffer.append"
}
```

## Payload Shape

```json
{
  "type": "input_audio_buffer.append",
  "audio": "<base64_pcm16_bytes>"
}
```

## Field Constraints

- `audio` is required.
- `audio` is a base64 string representing PCM16 raw bytes.

## Audio Expectations from Spec

- Encoding: PCM16
- Channels: mono
- Sample rate: negotiable via session config
- Transport encoding: base64 in JSON payload

## Stream Behavior

- Multiple append events form a logical buffered utterance.
- Buffer is finalized by `input_audio_buffer.commit`.

## Validation/Robustness Considerations

- Validate base64 decode success before buffering.
- Define max chunk size policy to avoid memory abuse.
- Ensure sample format consistency with current session config.
- Decide policy for empty chunks (ignore or error).

## Related Events

- Usually follows `session.created`/`session.updated` setup.
- Repeated until client emits `input_audio_buffer.commit`.
- Leads to server transcription delta/completed events.

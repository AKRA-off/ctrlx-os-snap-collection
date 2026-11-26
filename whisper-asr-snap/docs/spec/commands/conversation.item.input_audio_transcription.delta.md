# conversation.item.input_audio_transcription.delta

## Purpose

Server-to-client streaming event carrying incremental transcription text for in-progress recognition.

## Direction

- Server -> Client

## Type Discriminator

```json
{
  "type": "conversation.item.input_audio_transcription.delta"
}
```

## Payload Shape

```json
{
  "type": "conversation.item.input_audio_transcription.delta",
  "delta": "partial transcript text",
  "logprobs": [
    {
      "token": "The",
      "logprob": -0.0007,
      "bytes": [84, 104, 101]
    }
  ]
}
```

## Required Fields

- `type`
- `delta`

## Optional Fields

- `logprobs` (when requested through `session.include`)

## Semantics

- `delta` is an incremental text fragment, not necessarily token-aligned.
- Multiple delta events are concatenated in arrival order to form a candidate transcript.

## Logprobs Structure

Each logprob entry includes:

- `token`: token text
- `logprob`: numeric log probability
- `bytes`: token byte sequence as integers

`TranscriptionLogprob` schema requires `token` and `logprob`; `bytes` is present in examples.

## Client Handling Strategy

- Append each `delta` as received.
- Optionally render live transcript updates in UI/consumer pipeline.
- Do not treat deltas as final until completed event arrives.

## Related Events

- Emitted after audio append streaming begins.
- Zero or more delta events are followed by one completed event for the same item/content.

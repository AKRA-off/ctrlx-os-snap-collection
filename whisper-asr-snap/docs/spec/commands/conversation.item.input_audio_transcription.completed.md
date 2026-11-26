# conversation.item.input_audio_transcription.completed

## Purpose

Server-to-client terminal event for a committed audio chunk. Contains the finalized transcript for the correlated item/content.

## Direction

- Server -> Client

## Type Discriminator

```json
{
  "type": "conversation.item.input_audio_transcription.completed"
}
```

## Payload Shape

```json
{
  "type": "conversation.item.input_audio_transcription.completed",
  "transcript": "full final text",
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
- `transcript`

## Optional Fields

- `logprobs` (if requested in session include)

## Semantics

- Represents the final consolidated transcript for the committed chunk.
- Supersedes any intermediate aggregation built from delta events.

## Client Handling Strategy

- Finalize transcript state for the correlated item/content.
- Stop appending further deltas for that completed unit (unless protocol later allows revisions).
- Persist finalized text and optional confidence metadata.

## Notes from PDF Example Section

The long OpenAI example includes extra fields such as `usage`, `event_id`, and `obfuscation` on related events. Those fields are not in the authoritative schema subset for Myna and should be treated as optional/unknown unless Myna explicitly adopts them.

## Related Events

- Follows zero or more `conversation.item.input_audio_transcription.delta` events.
- Typically emitted after client sends `input_audio_buffer.commit`.

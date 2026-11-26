# model.loaded

## Purpose

Server-to-client event emitted once the transcription model is loaded and ready for serving client requests.

## Direction

- Server -> Client

## Type Discriminator

```json
{
  "type": "model.loaded"
}
```

## Payload Shape

```json
{
  "type": "model.loaded"
}
```

## Semantics

- Indicates the server is ready to begin processing transcription requests.
- May be emitted after `session.created` or `session.updated` if model initialization was needed.
- Clients can use this to know when to start sending audio.

## Required Fields

- `type` only

## Related Events

- Typically followed by transcription events like `conversation.item.input_audio_transcription.delta`.
- May precede `model.unloaded` if the model is later evicted from memory.

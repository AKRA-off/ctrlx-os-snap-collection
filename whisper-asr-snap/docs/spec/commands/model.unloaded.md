# model.unloaded

## Purpose

Server-to-client event emitted when the transcription model is unloaded from memory. This can occur when the server automatically unloads the model to save resources after a predefined idle period.

## Direction

- Server -> Client

## Type Discriminator

```json
{
  "type": "model.unloaded"
}
```

## Payload Shape

```json
{
  "type": "model.unloaded"
}
```

## Semantics

- Indicates the model has been removed from server memory for resource management.
- The model may need to be re-loaded before subsequent transcription requests.
- Clients should be prepared to handle this event and potentially retry requests.

## Required Fields

- `type` only

## Handling Guidance

- Treat as a resource cleanup event, not necessarily an error.
- If a transcription request is made after this event, expect `error` with `server_error/no_model_error`.
- Consider implementing automatic model reload if needed by the application.

## Related Events

- Typically preceded by a period of inactivity.
- Followed by `model.loaded` when the model is re-initialized.

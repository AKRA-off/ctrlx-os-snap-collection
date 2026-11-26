# error

## Purpose

Error event for request validation failures and server-side failures.

The spec text says errors cover both client and server-side errors.

## Direction

- Server -> Client (common)
- Client -> Server (allowed by narrative wording, but uncommon in standard WS APIs)

## Type Discriminator

```json
{
  "type": "error"
}
```

## Payload Shape

```json
{
  "type": "error",
  "error": {
    "type": "invalid_request_error",
    "code": "unknown_parameter",
    "message": "Unknown \"encoding\" field for session.audio.input"
  }
}
```

## Error Variants

### invalid_request_error

- `unknown_parameter`: unsupported field was provided.
- `invalid_parameter`: supported field has invalid value.

### server_error

- `server_error`: generic internal failure.
- `no_model_error`: Server unable to transcribe because there is no model loaded.

## Required Fields

At top level:

- `type`
- `error`

In `error` object:

- `type`
- `code`
- `message`

## Validation Logic

- `error.type` controls valid `code` values.
- Reject mismatched type/code pairings.

## Handling Guidance

- Log structured fields (`type`, `code`, `message`) for diagnostics.
- Distinguish retryable from non-retryable cases:
  - Potentially retryable: `server_error/no_model_error`
  - Usually non-retryable until request is changed: `invalid_request_error/*`
- Surface actionable message upstream for client/operator fixes.

## Related Events

- Commonly emitted in place of expected `session.updated` when `session.update` fails.
- May appear during streaming if request/state becomes invalid.

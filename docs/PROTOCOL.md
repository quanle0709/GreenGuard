# Local protocol v2

The browser and NodeMCU communicate over same-origin HTTP. All JSON responses use UTF-8 and `Cache-Control: no-store` for API routes.

## Status

`GET /api/status` returns controller identity, dry-run/auth flags, rain, mode, shield, motion, estimated position/confidence, fault, Wi-Fi, and request lifecycle. Required correlation fields are:

```json
{
  "protocolVersion": 2,
  "platformioBoard": "nodemcuv2",
  "lastRequestId": 3819201,
  "lastCommand": "DEPLOY",
  "lastCommandResult": "ACCEPTED",
  "commandPhase": "STARTED"
}
```

The UI rejects a status object that is not protocol 2 for `nodemcuv2`.

## Commands

`POST /api/command`:

```http
Content-Type: application/json
X-GreenGuard-Token: <CONTROL_TOKEN when configured>
```

```json
{
  "protocolVersion": 2,
  "requestId": 3819201,
  "command": "DEPLOY"
}
```

`requestId` is a nonzero uint32. The browser generates it once and reuses it only for a transport retry. The controller remembers the most recently processed ID; receiving it again returns `DUPLICATE` without executing another action.

Commands:

| Command | Exact meaning |
| --- | --- |
| `SET_AUTO` | Select automatic rain policy and clear STOP latch |
| `SET_MANUAL` | Stop current motion and select explicit manual control |
| `DEPLOY` | In MANUAL, move the shield over the plant |
| `RETRACT` | In MANUAL, move the shield away; requires confirmed dry |
| `STOP` | Immediately stop, select MANUAL, and latch STOP |
| `RESET_FAULT` | Clear an inspected recoverable fault, remain stopped/MANUAL |
| `CALIBRATE_DEPLOYED` | While stopped/MANUAL, record a visually confirmed deployed endpoint |
| `CALIBRATE_RETRACTED` | While stopped/MANUAL, record a visually confirmed retracted endpoint |

The HTTP response distinguishes receipt and acceptance:

```json
{
  "received": true,
  "accepted": true,
  "duplicate": false,
  "requestId": 3819201,
  "command": "DEPLOY",
  "result": "ACCEPTED",
  "phase": "ACCEPTED"
}
```

## Lifecycle

| Phase | Meaning | Terminal? |
| --- | --- | --- |
| `ACCEPTED` | Valid request accepted; physical completion has not happened | No |
| `STARTED` | Controller has left dead-time and requested one motor direction | No |
| `COMPLETED` | Requested mode/calibration or inferred/limited movement completed | Yes |
| `STOPPED` | STOP or rain safety ended the requested motion | Yes |
| `REJECTED` | Request was parsed but violated mode/rain/fault/state rules | Yes |
| `FAULT` | Motion ended in a controller fault | Yes |

Queue/HTTP acceptance is never shown as physical success. The browser keeps a pending action until a fresh status contains the same request ID and a terminal phase. Delayed or mismatched acknowledgements cannot finish another action. After 90 seconds without a matching terminal status, the UI reports uncertainty rather than success.

Command results are `ACCEPTED`, `REJECTED_MODE`, `REJECTED_RAIN`, `REJECTED_FAULT`, `REJECTED_STATE`, `DUPLICATE`, or `INVALID`.

## Configuration

`GET /api/config` reads safe runtime values. `POST /api/config` requires the token and is rejected while moving. It supports wet/dry confirmation, dead-time, inferred travel, maximum runtime, rain polarity, deploy PWM side, and PWM duty. Dry-run, GPIO pins, enable control, and limit-switch enable are compile-time hardware safety choices and cannot be changed through the web UI.

## Errors

- `400`: malformed/oversized JSON, zero ID, or invalid command.
- `401`: configured control token missing or wrong.
- `409`: understood request rejected by current mode, rain, fault, or motion state.
- `500`: persistence/config write failure; controller transitions to `STORAGE_ERROR`.

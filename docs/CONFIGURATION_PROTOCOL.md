# Red Monkey MPG configuration protocol v1

The receiver setup transport is USB CDC-ACM on the same physical connector as
the CNC controller keyboard interface. Each message is one UTF-8 JSON object followed by
`\n`. The browser sends `protocol: 1` in every request. Unknown fields are
ignored and unknown commands are rejected. Protocol v1 does not implement
request IDs; clients match the small set of commands to their typed responses
and consume pairing events independently.

Opening a configuration session is fail-closed: the receiver sends an all-keys
release, suppresses every motion command, and requires neutral re-arm after the
session closes or USB is reset/suspended.

## Requests

```json
{"protocol":1,"command":"GET_INFO"}
{"protocol":1,"command":"GET_CONFIG"}
{"protocol":1,"command":"GET_CNC_PROFILES"}
{"protocol":1,"command":"START_SCAN"}
{"protocol":1,"command":"PAIR_CANDIDATE","address":"00:00:5E:00:53:01"}
{"protocol":1,"command":"CONFIRM_PAIRING"}
{"protocol":1,"command":"CANCEL_PAIRING"}
{"protocol":1,"command":"SET_CONFIG","schema":1,"mapping":{"cncProfile":"masso-g3-touch-5.13"}}
```

## Responses and events

```json
{"protocol":1,"ok":true,"result":{}}
{"protocol":1,"ok":false,"error":"INVALID_MAPPING"}
{"protocol":1,"event":"SCAN_RESULT","candidate":{}}
{"protocol":1,"event":"PAIRING_STATE","state":"awaiting_confirmation"}
```

Maximum input frame length is 1024 bytes. ASCII control bytes other than CR/LF
invalidate a frame, overlong frames receive `FRAME_TOO_LARGE`, critical scalar
fields must occur exactly once, and parsing is capped at 256 input bytes per
main-loop pass. Unsupported protocol/schema values and malformed symbolic
fields are rejected without changing persistent state. The current embedded
parser accepts a deliberately narrow JSON shape and is not a general-purpose
JSON implementation. Responses are queued at complete-line boundaries and
retained across partial TinyUSB writes; new commands are backpressured until
the prior response drains.

## Approved mappings

The app sends symbolic actions and a compiled CNC profile ID, never raw USB
usages. Firmware accepts only controls, actions, and output profiles compiled
into the receiver. Version 1 supports:

- CNC profile: a value returned by `GET_CNC_PROFILES`
- configurable motion controls: `L1`, `L2`, `R2` (`R1` is fixed precision)
- legacy resolution field: `UNASSIGNED` (Plus/Minus are reserved for
  profile-defined override adjustment when supported)
- face actions: `UNASSIGNED`, `CANCEL`, `CYCLE_RESOLUTION`, `STEP_0_01`,
  `STEP_0_10`, `STEP_0_50`, and `STEP_1_00`

The dead-man, continuous modifier, and fixed R1 precision control must be
pairwise distinct. Joystick axis assignment, one-axis locking,
center-before-switch, stale timeout, disconnect release, and neutral re-arm are
firmware safety invariants and are not configurable.

`GET_CNC_PROFILES` returns stable IDs, names, descriptions, and capability
flags. An unsupported or corrupted stored profile fails closed. Protocol v1
does not accept user-authored keycodes or key chords.

## Pairing transaction

The active bond remains valid while scanning. A candidate becomes active only
after address selection, expected HID descriptor length/report shape, a stored
Bluetooth link key, and the three-second L2 + R2 + Plus physical confirmation
chord pass. An uncommitted candidate is never a motion source. Closing setup
cancels an in-progress pairing transaction and restores the last committed
controller. Configuration/address records use alternating TLV tags with
monotonically increasing sequence numbers, schema validation, CRC32, and
post-write readback. BTstack link keys are stored separately.

Commercial builds require an approved exact HID descriptor SHA-256 in addition
to length checking. Capturing, validating, and approving that fingerprint for
each supported controller revision remains a release gate. Protocol v1
intentionally does not expose raw USB usages, arbitrary macros, firmware
update, reboot, or factory-reset commands.

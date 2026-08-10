# 8BitDo Lite 2 — D-input commissioning profile

This profile records observations from the specific controller used during
Red Monkey MPG commissioning. Firmware revision and manufacturing variation must be
considered before treating it as universal.

## Identity

- Advertised name: `8BitDo Lite 2`
- Commissioning address: unit-specific. Documentation and tests use the
  reserved example address `00:00:5E:00:53:01`; substitute your own.
- Mode switch: `D`
- Transport: Bluetooth Classic HID
- Input descriptor length: 83 bytes
- Approved descriptor SHA-256: **pending production capture and qualification**
- Interrupt report: 11 bytes including `A1` transaction header
- Report ID: `01`

## Confirmed controls

| Physical control | HID usage | Minimum | Center | Maximum | Direction |
|---|---:|---:|---:|---:|---|
| Left stick X | Generic Desktop `0x30` | 0–2 | 127 | 255 | Left → right |
| Left stick Y | Generic Desktop `0x31` | 0–34 | 127 | 255 | Up → down |
| Right stick X | Generic Desktop `0x32` (Z) | 0 | 127 | 255 | Left → right |
| Right stick Y | Generic Desktop `0x35` (Rz) | 0 | 128 | 255 | Up → down |

Cross-axis values can drift under full deflection. Red Monkey MPG applies a deadzone,
selects the dominant axis, and locks that axis until all sticks are centered.

## Reconnection

BTstack stores the Classic link key in the Pico's flash-backed TLV database.
After initial pairing, pressing Home in D mode should let the controller
reconnect without pressing Pair. The host accepts incoming connections only
from the commissioned Bluetooth address and also retries an outgoing connection
while the controller is awake.

Red Monkey MPG uses right-stick Y (`Rz`) for machine Z jogging. Right-stick X remains
unassigned so a diagonal right-stick movement cannot select another machine
axis.

## D-pad / hat

| Direction | Hat value |
|---|---:|
| Up | 0 |
| Right | 2 |
| Down | 4 |
| Left | 6 |
| Neutral | 8 |

The descriptor supports diagonal hat values as well, but Red Monkey MPG will accept
only the four cardinal values above. Diagonal values will produce no action.

## Production descriptor capture

Flash the current `openmpg_hid_dump.uf2`, reconnect the Lite 2 in D mode, and
record the complete `DESCRIPTOR_SHA256` and `DESCRIPTOR_BYTES` lines. Verify
the SHA-256 independently from the captured bytes, then repeat on controllers
from each hardware/firmware revision intended for sale. Do not approve a hash
based only on the 83-byte length.

After the report semantics and safety regression pass, supply the approved
hash as `OPENMPG_LITE2_DESCRIPTOR_SHA256`. A commercial build refuses to
configure without it, and the receiver rejects a nonmatching controller before
accepting reports.

## Buttons

The following mapping assumes the commissioning capture was performed in the
requested A, B, X, Y, L, R, L2, R2, left-click, right-click, Minus, Plus, Home
order.

| Physical control | Button mask | Additional value |
|---|---:|---|
| A | `0x00001` | — |
| B | `0x00002` | — |
| X | `0x00008` | — |
| Y | `0x00010` | — |
| L | `0x00040` | — |
| R | `0x00080` | — |
| L2 | `0x00100` | Consumer `C5`: 0–255 |
| R2 | `0x00200` | Consumer `C4`: 0–255 |
| Left-stick click | `0x02000` | — |
| Right-stick click | `0x04000` | — |
| Minus | `0x00400` | — |
| Plus | `0x00800` | — |
| Home | `0x00004` | — |
| Star | Not reported alone | Local Turbo function in D mode |

Clicking a stick can displace its analog axes. Button actions must use the
digital mask and must not treat the accompanying axis noise as deliberate
motion. L2's digital mask is the intended dead-man input; releasing it must
immediately release every machine-jog key.

Pressing and releasing Star alone produced no HID input report during
commissioning. It is therefore unavailable as an Red Monkey MPG command in D mode.

# Live Bluetooth-to-USB bridge bench test

`red_monkey_mpg_live_bridge_bench.uf2` connects the commissioned 8BitDo Lite 2 to a
normal computer and emits the active compiled CNC profile's keyboard jog keys.

**Do not connect this bench image to any machine controller.** Validate
every press, release, reconnect, and fault case on the Mac first.

## Status LED

| LED pattern | State |
|---|---|
| Brief flash once per second | Waiting for the controller or HID descriptor |
| Fast blink | Connected but disarmed; release L2 and center both sticks |
| Solid | Connected, centered once, and ready for deliberate input |

## Mac procedure

1. Disconnect the Pico from all machine controllers.
2. Open macOS Keyboard Viewer.
3. Flash `red_monkey_mpg_live_bridge_bench.uf2` using BOOTSEL.
4. If Keyboard Setup Assistant appears, close it.
5. Keep the Lite 2 in D mode and press Home once to reconnect.
6. With L2 released, center both sticks until the Pico LED becomes solid.
7. Perform the checks below while watching Keyboard Viewer.

## Required checks

| Test | Expected keyboard result |
|---|---|
| Move either stick without L2 | No key |
| Press A once with L2 released and sticks centered | `4`, then release |
| Release and press A three more times | `3`, `2`, `1`, one key per press |
| Hold A | No repeated keys |
| Press A while L2 is held or a stick is displaced | No resolution key |
| L2 + left stick left/right | Left/Right Arrow |
| L2 + left stick up/down | Up/Down Arrow |
| L2 + right stick up/down | U/D |
| L2 + right stick left/right | No key |
| Release L2 while a stick remains displaced | Immediate all-keys release |
| L2 + R2 + a direction | Shift + direction |
| Diagonal left-stick motion | Exactly one dominant axis |
| Cross from one axis toward another without centering | Release; no new axis |
| Center, then choose a new axis | New single direction key |
| Turn the controller off while jogging | Immediate release; fast/wait LED state |
| Reconnect while L2 or a stick is held | No key until L2 is released and sticks center |

While a jog key is active, the bridge releases all USB keys if valid controller
reports stop for more than 150 ms. Idle controller silence is safe and does not
disarm the bridge. After an active stale-input, Bluetooth, or USB fault, it
requires a neutral controller state before it can re-arm.

Precision behavior is defined by the selected CNC profile. The first compiled
profile emits an ordinary unmodified step-jog key for precision.

## Exit

Unplug the Pico. To restore the serial diagnostic, flash
`outputs/red_monkey_mpg_hid_dump.uf2` again using BOOTSEL.

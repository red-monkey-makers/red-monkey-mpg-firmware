# iPhone receiver: MASSO commissioning test

This procedure is the hardware-in-the-loop qualification gate for
`red_monkey_mpg_mobile_receiver.uf2`. It bridges the Red Monkey MPG iOS app to
MASSO through the Pico 2 W USB keyboard interface.

This is release-candidate testing, not proof of commercial product safety. The
receiver and phone are not safety-rated controls and never replace the physical
E-stop, machine limits, guarding, or trained operator supervision.

The mobile image is an alternative to the gamepad receiver image. The current
Bluetooth stack supports one active client; it does not accept an iPhone and a
gamepad simultaneously.

## Required setup

- Raspberry Pi Pico 2 W
- Red Monkey MPG iOS app on a physical iPhone
- MASSO G3 or G3 Touch on the F3 Jog screen
- A known-good gamepad production UF2 available for rollback
- The physical E-stop verified and within immediate reach

Record the UF2 SHA-256 before flashing. Never use a file whose checksum or
source is uncertain.

## Stage 0 — Computer keyboard test

Complete this stage before connecting the receiver to MASSO.

1. Flash `red_monkey_mpg_mobile_receiver.uf2` with the Pico disconnected from
   the machine.
2. Connect the Pico to a Mac or Windows computer using the intended USB cable.
3. Open Keyboard Viewer (macOS) or an equivalent keyboard-event tester.
4. Open Red Monkey MPG on the iPhone, find **Red Monkey MPG**, connect, and
   accept the Bluetooth pairing prompt.
5. Wait for the app to report that the receiver is ready. The app automatically
   sends a neutral frame to satisfy the receiver's re-arm interlock.
6. In **Single Step**, tap X−, X+, Y−, Y+, Z−, and Z+. Confirm one bounded key
   press and a release for each tap.
7. In **Continuous**, hold and release each direction. Confirm Shift plus only
   one direction key while held and an immediate all-keys-up state on release.
8. Press two directions, slide between directions, and use multi-touch. Confirm
   that a second axis never becomes active until all direction controls have
   returned to neutral.
9. Select all four resolutions and confirm MASSO-profile keys 4, 3, 2, and 1
   respectively.
10. While a continuous direction is held, perform each fault separately:
    background the app, lock the phone, disable Bluetooth, force-quit the app,
    and move the phone out of range. Every fault must release all keys. Reconnect
    and confirm motion remains locked until the app sends neutral.
11. Unplug and reconnect USB. Confirm no key appears during enumeration and a
    fresh neutral state is required before another direction is accepted.

Stop if any key remains held, two direction keys appear together, a direction
is reversed, or motion resumes without a fresh neutral state. Reflash the
known-good gamepad image and open an issue with the recorded firmware hash.

## Stage 1 — MASSO with motion physically disabled

1. Stop every program, turn off the spindle, remove the tool when practical,
   and clear the work envelope.
2. Use the machine manufacturer's normal procedure to prevent drive motion.
   Do not bypass or jumper a safety circuit.
3. Independently verify the physical E-stop.
4. Connect the Pico to MASSO and open F3 Jog.
5. Select the smallest step size and lowest practical Jog Feed Rate on MASSO.
6. Connect the iPhone and choose the MASSO profile.
7. Test all six directions in **Single Step** and verify the indicated axis and
   sign on MASSO.
8. Test every resolution. Confirm the green F3 indicator before issuing a jog.
9. Briefly test **Continuous** for each axis. Release the screen control and
   confirm MASSO stops immediately.
10. Repeat the background, Bluetooth-off, force-quit, and receiver-power-loss
    tests while watching MASSO. Each must stop the command and require neutral
    after reconnect.

## Stage 2 — Minimum-speed powered validation

Proceed only after every Stage 0 and Stage 1 check passes.

1. Position the machine well away from limits, stock, fixtures, and people.
2. Set MASSO's F3 Jog Feed Rate to the minimum practical value.
3. Restore drive power normally and keep one hand ready at the physical E-stop.
4. Verify X−, X+, Y−, Y+, Z−, and Z+ individually in Single Step.
5. Verify each displayed resolution with one clear-axis step.
6. Test Continuous last, using very short holds and ample stopping distance.
7. Repeat one app-background release and one Bluetooth-loss release at the
   minimum feed rate.

## Commissioning record

| Field | Value |
|---|---|
| MASSO model and software | |
| iPhone model and iOS | |
| App version/build | |
| Firmware version | `0.4.0-rc.1` |
| UF2 SHA-256 | |
| Machine | |
| Operator and date | |
| Stage 0 result | |
| Stage 1 result | |
| Stage 2 result | |
| Notes | |

Do not publish or promote the mobile image as qualified until the completed
record is reviewed and every result is a pass.

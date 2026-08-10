# MASSO G3 Touch 5.13 commissioning

This procedure qualifies the
[`masso-g3-touch-5.13`](cnc-profiles/MASSO_G3_TOUCH_5_13.md) CNC controller
profile. It is profile-specific and does not define the project-wide output
mapping.

This procedure applies only to:

- MASSO G3 Touch
- MASSO software version 5.13, released 1 November 2025
- The commissioned 8BitDo Lite 2 in D mode
- Controller address `E4:17:D8:32:66:02`
- `openmpg_masso_g3_513_commissioning.uf2`

The firmware can issue real machine-motion commands. It is not a substitute for
a hard-wired E-stop, machine guarding, limits, or normal operator training.

Official references:

- https://docs.masso.com.au/getting-started-guides/Software/masso-g3/release_notes_5.13
- https://docs.masso.com.au/getting-started-guides/machining-with-masso/keyboard-and-key-shortcuts/rapid-jog

## Stage 1 — Motion physically disabled

1. Stop all programs and turn the spindle off.
2. Remove the cutting tool when practical and clear the work envelope.
3. Use the machine manufacturer's normal method to ensure the drives cannot
   energize. Do not defeat, jumper, or bypass any safety circuit.
4. Verify the physical E-stop independently before connecting Red Monkey MPG.
5. Connect the Pico to a MASSO USB port.
6. Put the Lite 2 in D mode and press Home once.
7. Keep L2 released and center both sticks. A solid Pico LED means ready.
8. Open MASSO's F3 Jog / Rapid screen.
9. Select the smallest step size and lowest practical Jog Feed Rate. On F3,
   verify that controller Plus/Minus increase and decrease the displayed
   override before using continuous jog.
10. Confirm stick movement without L2 causes no MASSO jog indication or
    commanded-position change.
11. Test one direction at a time with brief L2 presses:

| Controller input | Expected MASSO command |
|---|---|
| L2 + left stick left | X- |
| L2 + left stick right | X+ |
| L2 + left stick up | Y+ |
| L2 + left stick down | Y- |
| L2 + right stick up | Z+ |
| L2 + right stick down | Z- |

12. Confirm right-stick horizontal motion produces nothing.
13. Hold a direction, then release L2 while the stick remains displaced. The
    command must stop immediately.
14. Begin a diagonal left-stick input. Confirm only one axis is commanded.
15. Without centering, cross toward the other axis. Confirm no new axis starts.
16. While commanding a direction, turn the controller off. The command must
    release, and reconnecting must require neutral controls before re-arming.
17. Test L2 + R2 + direction and confirm MASSO recognizes Shift-modified
    rapid/continuous jogging. Keep motion physically disabled for this test.
18. Release L2, center both sticks, and press A four separate times. Confirm
    the F3 green resolution indicator selects 0.01, 0.10, 0.50, then 1.00 mm.
    Holding A must not repeat. A must do nothing with L2 held or a stick
    displaced.

Stop commissioning if any expected direction, release, or re-arm behavior is
wrong. Disconnect the Pico and return to the Mac live-bridge diagnostic.

## Stage 2 — Minimum-speed powered validation

Proceed only after every Stage 1 check passes.

1. Place the machine in a clear, known-safe position, well away from all limits,
   fixtures, stock, and personnel.
2. Set the F3 Jog Feed Rate to the minimum practical value.
3. Restore drive power using the machine's normal procedure.
4. Keep one hand ready at the physical E-stop.
5. Verify X-, X+, Y-, Y+, Z-, and Z+ individually using short step jogs.
6. Reconfirm that releasing L2 stops output before testing a longer hold.
7. Test rapid/continuous jogging last, at a deliberately low F3 feed rate and
   with ample stopping distance.
8. Verify each A-selected step distance with one clear-axis step jog.
9. Record the result, MASSO version, firmware SHA-256, machine identity, date,
   and operator in the commissioning log below.

## Commissioning record

| Field | Value |
|---|---|
| MASSO model | G3 Touch |
| MASSO software | 5.13 |
| Firmware SHA-256 | Record from release handoff |
| Machine | |
| Date | |
| Operator | |
| Stage 1 result | |
| Stage 2 result | |
| Notes | |

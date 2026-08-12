# Red Monkey MPG iPhone user guide — MASSO G3 Touch

This guide is for a Pico 2 W running
`red-monkey-mpg-mobile-receiver-<version>.uf2` and connected to a MASSO G3/G3
Touch. It does not apply to the separately flashed Bluetooth gamepad receiver.

> **Important:** Red Monkey MPG is a convenience control, not a safety device.
> Keep the physical E-stop accessible and retain every machine guard,
> interlock, limit, and normal operating procedure.

## Before every use

1. Stop any running program and disable cutting energy.
2. Clear the machine travel area and verify the physical E-stop.
3. Open MASSO's **F3 Jog/Rapid** screen.
4. Set a low jog feed and confirm the displayed step resolution.
5. Plug the receiver into MASSO and open Red Monkey MPG on the iPhone.

## Connect

1. Tap the antenna button in the app toolbar.
2. Find and select **Red Monkey MPG**.
3. Accept the iOS Bluetooth pairing request if shown.
4. Select the **MASSO** CNC controller profile.
5. Wait until the app reports the receiver connected and shows the motion
   controls.

The app hides jogging controls until a receiver is connected. Reconnection
after a normal power cycle may be automatic; otherwise open the connection
menu and select the receiver again.

## Motion modes

### Single Step

Select **Single Step**, choose the desired resolution, confirm the same value
on MASSO, and tap X−, X+, Y−, Y+, Z−, or Z+. Each recognized touch sends one
bounded step. You may tap repeatedly, but watch the machine and MASSO display
between movements.

### Continuous

Select **Continuous**, then press and hold one direction button. Movement is
requested only while that button remains held. Lift your finger to release the
command. Step-resolution controls are hidden because they do not apply to
continuous motion.

## One-axis behavior

Only one direction can be active at a time. Release all direction controls
before choosing another axis or reversing direction. Multi-touch or sliding
between controls must not command two axes or bypass the neutral requirement.

## Normal stop and faults

Lift your finger from the active direction button for a normal stop. The
receiver also releases USB keyboard output on stale input, app backgrounding,
phone lock, Bluetooth loss, USB interruption, or receiver reset. After a fault
or reconnect, all controls must be neutral before another movement is accepted.

For dangerous or unexpected movement, use the machine's physical E-stop—not
the phone, Bluetooth, app, or Pico.

## Required first-use check

Complete this with cutting energy disabled and motion prevented according to
the machine manufacturer's procedure:

- [ ] The app connects only to the intended **Red Monkey MPG** receiver.
- [ ] X−, X+, Y−, Y+, Z−, and Z+ match MASSO's display and machine directions.
- [ ] Single Step sends one bounded step per recognized touch.
- [ ] Each displayed resolution matches the selection shown by MASSO.
- [ ] Continuous motion stops immediately when the direction is released.
- [ ] A second direction cannot activate until all controls are neutral.
- [ ] Backgrounding or locking the app, disabling Bluetooth, and unplugging the
      receiver each release the current command.
- [ ] Motion never resumes automatically after reconnecting.

Then follow the minimum-speed powered stage in
[the iPhone receiver MASSO test](MOBILE_RECEIVER_MASSO_TEST.md).

## Troubleshooting

### The receiver does not appear

- Confirm the Pico has the iPhone receiver UF2, not the gamepad UF2.
- Power-cycle the receiver and toggle Bluetooth on the iPhone.
- Remove an obsolete **Red Monkey MPG** bond from iOS Bluetooth settings, then
  connect again from the app.
- Keep the phone close to the receiver during pairing.

### Connected, but the controls are hidden

Open the connection menu and confirm the app reports the receiver ready. If it
is still connecting, disconnect, power-cycle the Pico, and reconnect.

### MASSO does not move

Confirm MASSO is on F3, the intended profile is selected, the jog feed is not
zero, and the app is not in Demo mode. Demo mode deliberately sends no
Bluetooth or CNC commands.

### Movement is unexpected or does not stop

Use the physical E-stop, disconnect the receiver, and remove it from service
until the firmware image, app version, CNC profile, and staged safety tests
have been verified.

# Red Monkey MPG iOS: motion-safe BLE bench test

The `red_monkey_mpg_mobile_ble_bench` target lets you test the iOS client and Pico BLE
protocol without producing USB keyboard reports.

> **Do not connect this diagnostic firmware to a CNC controller or machine.**
> It is a communications test, not production motion-control firmware. Keep the
> machine powered down and physically disconnected throughout this procedure.

## What the diagnostic firmware does

- Advertises the BLE name `Red Monkey MPG`.
- Requires encrypted Bluetooth Low Energy pairing with bonding.
- Validates packet version, length, controller profile, ranges, sequence, and CRC.
- Accepts only a supported CNC controller profile and reports the active profile
  back to the app. The initial supported profile is MASSO G3 / G3 Touch.
- Enforces dead-man, one-axis, neutral-before-switch, timeout, and disconnect
  rules while reporting the interpreted state over USB serial.
- Sets the protocol's diagnostic-mode status bit.
- Does **not** initialize TinyUSB keyboard output or emit CNC keystrokes.

The current Pico Bluetooth stack supports one active connection, so this bench
target treats the iOS client as an alternative input device rather than running
it alongside the Bluetooth gamepad.

## Build and flash

From the firmware repository root:

```sh
cmake -S . -B build-mobile-pico \
  -DRED_MONKEY_MPG_BUILD_PICO=ON \
  -DRED_MONKEY_MPG_BUILD_HOST_TESTS=OFF
cmake --build build-mobile-pico \
  --target red_monkey_mpg_mobile_ble_bench --parallel
```

The UF2 is `build-mobile-pico/red_monkey_mpg_mobile_ble_bench.uf2`.

1. Disconnect the Pico from every CNC controller.
2. Hold **BOOTSEL** while plugging the Pico into the computer.
3. Release BOOTSEL after the `RP2350` volume appears.
4. Copy `red_monkey_mpg_mobile_ble_bench.uf2` to that volume.
5. Wait for the Pico to reboot.

## Observe the receiver

On macOS, locate and open the USB serial port:

```sh
ls /dev/cu.usbmodem*
screen /dev/cu.usbmodemXXXX 115200
```

Replace `XXXX` with the displayed port suffix. Press `Control-A`, then `K`,
then `Y` to exit `screen`.

## Run the iPhone app

1. Open `RedMonkeyCNCJogger.xcodeproj` in Xcode.
2. Select a physical iPhone and your Apple development team.
3. Build and run the `RedMonkeyCNCJogger` scheme. The installed app is named
   **Red Monkey MPG**.
4. Tap **Find receiver**, select **Red Monkey MPG**, and approve the iOS
   pairing prompt.
5. Confirm the app displays `Bench mode — keyboard output is disabled.`

Exercise each control and compare the app with the USB serial log:

- In **Single Step** mode, tap X−, X+, Y−, Y+, Z−, and Z+ separately. Verify
  that every tap produces one bounded direction pulse followed by neutral.
- In **Continuous** mode, press and hold each direction separately. Verify that
  releasing the direction immediately returns to neutral.
- While holding one direction, verify that a second direction cannot activate.
- Select every resolution while neutral and confirm the reported value.
- Background the app and turn off Bluetooth; both must fail closed.
- Leave a motion control held and stop the app; the receiver must time out and
  return to neutral within 150 ms of its last valid control frame.

## Pass criteria

The test passes only if:

- every displayed action matches the intended control;
- malformed, replayed, stale, or ambiguous frames are rejected;
- every release, disconnect, and timeout produces neutral state; and
- macOS Keyboard Viewer shows no keys from the diagnostic receiver.

After testing, either restore the gamepad production UF2 or build the separate
`red_monkey_mpg_mobile_receiver` candidate. Do not connect the mobile candidate
to a machine until following every stage in
[`MOBILE_RECEIVER_MASSO_TEST.md`](MOBILE_RECEIVER_MASSO_TEST.md).

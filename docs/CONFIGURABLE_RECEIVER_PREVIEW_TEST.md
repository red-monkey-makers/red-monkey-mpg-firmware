# Configurable receiver preview — Mac bench test

This test uses `red_monkey_mpg_configurable_receiver_preview.uf2` and the local
`configurator/` web app. It validates composite USB, configuration persistence,
and pairing on a Mac before any production firmware is considered.

**Never connect this preview image to any machine controller.** It
contains a USB keyboard interface. Keep the Pico connected only to the Mac and
use Keyboard Viewer while testing.

## 1. Flash and enumerate

1. Disconnect the Pico from every CNC controller.
2. Flash `outputs/red_monkey_mpg_configurable_receiver_preview.uf2` using BOOTSEL.
3. Open the local configurator in desktop Chrome or Edge.
4. Click **Connect receiver**.
5. Select the serial device associated with `Red Monkey MPG Configurable Receiver
   Preview`. Do not select macOS Bluetooth or unrelated debug ports.
6. Confirm the app displays a 16-character receiver serial and firmware
   `0.3.2-config-preview`.

Opening the serial port must release every keyboard key and suppress all jog
output. Leave macOS Keyboard Viewer open and verify moving the controller while
the app is connected produces no keys.

## 2. Mapping persistence

1. Change the dead-man to L1 and continuous modifier to L2. R1 is fixed as the
   precision control and must never appear as a configurable motion choice.
2. Save. The app must report success.
3. Disconnect/unplug the Pico, reconnect it, and reconnect the app.
4. Confirm L1/L2 reload from the receiver.
5. Restore dead-man L2 and continuous R2, then save again.
6. Attempt to assign the same control to both fields. The app and receiver must
   reject the invalid mapping.

## 3. Pairing replacement

1. Click **Replace controller**.
2. Put exactly one Lite 2 in D-mode pairing near the receiver.
3. Select the discovered controller in the app.
4. When prompted, hold **L2 + R2 + Plus** on that controller for three seconds.
5. The app must report that the bond was saved.
6. Unplug and reconnect the Pico. Wake the controller with Home.
7. Reconnect the app and confirm the saved controller address is reported.

Pairing must time out or cancel without replacing the previous stored address.
An unsupported HID descriptor must be rejected.

## 4. Fail-closed checks

- Close the app while a controller input is held: no stuck key is permitted.
- Unplug USB while a controller input is held: reconnect requires neutral.
- Turn off the controller during a jog-key test: release within the existing
  150 ms active-report timeout.
- Reconnect with L2 or a stick held: no output until L2 is released and all
  assigned sticks are centered.

After the configuration checks pass, repeat the existing Mac live keyboard
check using the restored default mapping. Do not proceed to a machine; this
preview requires a separate production hardening and profile-specific
commissioning gate.

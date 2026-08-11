# Unified receiver release-candidate test

Use `outputs/openmpg_production_receiver.uf2`. This single image replaces the
separate setup-preview and commissioning images for controlled testing after
the checks below pass. It is not yet an approved commercial release.

## Configure on Mac or Windows

1. Flash the UF2 with BOOTSEL and reconnect the Pico using a USB data cable.
2. Open the local configurator in desktop Chrome or Edge and connect the
   filtered Red Monkey MPG receiver.
3. Confirm the displayed firmware is `0.4.0-rc.1` and the receiver
   serial is unique.
4. If no controller is shown, pair the Lite 2 in D mode. Select the discovered
   controller and hold L2 + R2 + Plus for three seconds when prompted.
5. Select the intended CNC controller profile and save the mapping: L2
   dead-man, R2 continuous, A resolution cycle, and B cancel/release.
6. Use **Finish & disconnect**. Motion output remains disabled until the
   controller reports released buttons and centered sticks.

## Setup fault checks

1. Start pairing, select a candidate, then close the configurator before the
   confirmation chord. Reopen it and verify the previous controller remains
   selected and the candidate cannot produce keys.
2. Select a Bluetooth HID device with an unsupported descriptor. Verify it is
   rejected, its uncommitted bond is discarded, and the previous controller
   reconnects after setup closes.
3. Save a mapping, immediately power-cycle, and verify either the complete old
   record or complete new record loads—never a partial mapping.
4. Suspend and resume the USB host with a jog control held. Verify an immediate
   release and neutral re-arm before any new key.
5. Feed malformed and over-1024-byte setup frames. Verify they are rejected and
   the receiver remains responsive without keyboard output.

## Persistence check

1. Unplug and reconnect the receiver without holding BOOTSEL.
2. Turn on the paired controller normally; do not put it in pairing mode.
3. Confirm it reconnects automatically.
4. Reopen the configurator and verify the same controller address and mapping
   are reported, then explicitly disconnect setup again.

## CNC controller check

1. Keep machine motion disabled and follow the selected profile's commissioning
   procedure. The first supported profile uses `MASSO_G3_513_COMMISSIONING.md`.
2. Connect this same receiver image to the CNC controller; do not reflash it.
3. Confirm no movement without L2, one axis at a time, immediate release on L2
   release, USB suspend, or signal loss, and neutral re-arm after reconnect.
4. Confirm every capability advertised by the selected profile, including step
   resolution when supported.

Opening the setup serial port immediately releases every HID key and keeps
motion locked until the setup session closes and neutral controls are observed.
Repeat the complete test on every supported CNC controller model, software
version, and profile revision before adding it to the compatibility statement.

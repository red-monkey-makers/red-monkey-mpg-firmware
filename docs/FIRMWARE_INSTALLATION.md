# Install or update Red Monkey MPG firmware

<p align="center">
  <img src="assets/red-monkey-mpg-banner.webp" alt="Red Monkey MPG — Pair once. Jog with confidence." width="100%">
</p>

This guide is for an end user installing an official Red Monkey MPG firmware
release on a Raspberry Pi Pico 2 W receiver. Building source code, installing a
Pico SDK, and opening a serial terminal are not required.

> **Important:** Disconnect the receiver from the CNC controller before
> updating it. Perform the entire installation with the receiver connected
> only to a Mac or Windows computer. Red Monkey MPG is not safety-rated and
> never replaces a hard-wired E-stop or normal machine safeguards.

## What you need

- The Red Monkey MPG receiver
- A Mac or Windows computer
- A data-capable Micro-USB cable
- The official firmware file ending in `.uf2`

Download firmware from the project's
[GitHub Releases page](https://github.com/red-monkey-makers/red-monkey-mpg-firmware/releases).
Under **Assets**, download the receiver variant you intend to use:

```text
red-monkey-mpg-firmware-0.4.0-rc.2.uf2
red-monkey-mpg-iphone-receiver-0.4.0-rc.2.uf2
```

The first image accepts the supported Bluetooth gamepad; the second accepts the
Red Monkey MPG iPhone app. Install exactly one variant. The version will change
in later releases. Do not copy GitHub's automatically
generated source-code ZIP or TAR files to the receiver. Do not install files
named `keyboard_bench`, `hid_dump`, `bt_scan`, `live_bridge_bench`,
`commissioning`, or `preview`; those are development and diagnostic images.

## Optional: verify the download

Each release lists the firmware SHA-256 in its release notes. Verifying it
confirms that the downloaded file matches the published asset.

On macOS, open Terminal and run:

```sh
shasum -a 256 ~/Downloads/red-monkey-mpg-firmware-0.4.0-rc.2.uf2
```

On Windows, open PowerShell and run:

```powershell
Get-FileHash "$HOME\Downloads\red-monkey-mpg-firmware-0.4.0-rc.2.uf2" -Algorithm SHA256
```

Use the actual downloaded filename. The resulting value must exactly match the
SHA-256 shown in the GitHub release notes. Stop if it does not match.

## Install on macOS or Windows

1. Stop the CNC machine safely and unplug the receiver from the CNC
   controller.
2. Disconnect the receiver's USB cable.
3. Press and hold the receiver's **BOOTSEL** button.
4. While continuing to hold BOOTSEL, connect the receiver to the computer with
   a data-capable Micro-USB cable.
5. Release BOOTSEL when a removable drive named **RPI-RP2** appears:
   - On macOS, find it in Finder under **Locations**.
   - On Windows, find it in File Explorer under **This PC**.
6. Copy the downloaded `.uf2` file onto the **RPI-RP2** drive.
7. Wait. The drive should disappear automatically after the copy finishes.
   This is normal and means the receiver has rebooted into the new firmware.
8. Allow approximately 10 seconds for startup, then unplug the receiver from
   the computer.

Do not press BOOTSEL during an ordinary startup. BOOTSEL is needed only when
installing or recovering firmware.

If the receiver enclosure does not provide safe BOOTSEL access, follow the
supplier's instructions. Do not open or modify a supplied enclosure unless the
supplier specifically instructs you to do so.

## Pairing and mapping after an update

A normal UF2 installation is designed to preserve the paired controller and
saved mapping. They are stored in reserved flash sectors outside the firmware
application image.

Do **not** use a full-chip erase utility or a `flash_nuke.uf2` file. Those can
erase the pairing keys and saved mapping. Even after a normal update, always
verify reconnection and configuration before returning the receiver to machine
service. If either is missing, reconnect the receiver to a computer and use
the Red Monkey MPG Configurator to pair and save the intended profile again.

## Reconnect after installation

1. Confirm the controller is in its commissioned mode. The supplied 8BitDo
   Lite 2 configuration uses **D mode**.
2. Release L2, R2, and every other button, and center both sticks.
3. Connect the receiver to a computer first if you want to verify its displayed
   firmware version in the configurator.
4. If you open the configurator, confirm the expected firmware version,
   controller, mapping, and CNC profile. Select **Finish & disconnect**, then
   close the setup session before unplugging the receiver. Motion output is
   intentionally locked while setup is connected.
5. Unplug the receiver from the computer and connect it to the CNC controller's
   USB keyboard port.
6. Press the gamepad's **Home** button once. Do not put an already paired
   controller into pairing mode.
7. Keep every control neutral until the receiver LED becomes solid.

## Required post-update safety check

Perform this check with cutting energy disabled, the work area clear, the
physical E-stop accessible, the smallest step size selected, and a very low jog
feed:

- [ ] The expected firmware version, controller, mapping, and CNC profile are
      shown by the configurator.
- [ ] The controller reconnects normally without entering pairing mode.
- [ ] The receiver becomes ready only after all buttons are released and both
      sticks are centered.
- [ ] Moving either stick without holding L2 causes no machine movement.
- [ ] Each commanded axis and direction matches the machine display and
      physical movement.
- [ ] A diagonal input never moves more than one axis.
- [ ] Releasing L2 immediately stops pendant-commanded movement.
- [ ] Turning off the controller during a low-risk test releases movement.
- [ ] Step-size controls change only the intended displayed CNC setting.

Stop and remove the receiver from service if any check fails. Use the physical
E-stop for dangerous or unexpected movement.

## Troubleshooting

### The RPI-RP2 drive does not appear

- Disconnect the cable, hold BOOTSEL first, and reconnect while continuing to
  hold the button.
- Confirm the cable supports data; some Micro-USB cables provide power only.
- Try another USB port directly on the computer instead of a hub or dock.
- Try another known-good data cable.
- Check Finder **Locations** on macOS or File Explorer **This PC** on Windows.

### The RPI-RP2 drive disappears after copying

That is expected. The receiver automatically disconnects the boot drive and
starts the installed firmware.

### The RPI-RP2 drive appears at every startup

Make sure BOOTSEL is not stuck or being held. Reinstall the official UF2. If
the problem continues, disconnect the receiver and contact the supplier.

### The controller no longer reconnects

Confirm the controller mode, charge it, center both sticks, and press Home
once. Do not press Pair during a normal reconnect. If it still does not
connect, use the configurator to verify or repeat pairing while the receiver is
connected only to the computer.

### The receiver flashes rapidly after the update

Release L2 and every other button, then center both sticks. Rapid flashing
means the controller is connected but the receiver is waiting for a neutral
state before it can arm.

### The update was interrupted

Disconnect the receiver and repeat the BOOTSEL installation from the
beginning. If **RPI-RP2** still appears, copy the same verified official UF2
again. Do not connect the receiver to a CNC controller until the complete
post-update check passes.

## Downgrading

Do not install an older firmware version unless its release notes explicitly
support downgrading from the installed version. Configuration formats and CNC
or controller profiles can change. A downgrade may require pairing and mapping
the receiver again.

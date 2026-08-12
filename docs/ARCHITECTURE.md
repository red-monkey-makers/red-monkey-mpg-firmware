# Architecture

Red Monkey MPG builds two separately flashed receiver applications around one
portable safety and CNC-output core:

- `red_monkey_mpg_production_receiver` is a Bluetooth Classic HID host for a
  supported gamepad and includes the USB browser-configuration channel.
- `red_monkey_mpg_mobile_receiver` is a Bluetooth Low Energy peripheral for
  the Red Monkey MPG iPhone app and does not include gamepad pairing or the
  browser configurator.

The Pico runs exactly one application at a time. Both separate safety logic
from I/O:

1. The selected transport validates bounded wireless input: the gamepad image
   matches and parses a HID profile; the iPhone image validates the versioned
   mobile command frame and CRC.
2. The transport normalizes valid input into the shared control state.
3. `ControlMapper` rejects disconnected, stale, unarmed, and invalid input,
   selects at most one axis, and produces a semantic `OutputFrame`.
4. The selected `CncControllerProfile` converts that frame into a bounded,
   reviewed USB keyboard report for the target CNC controller.
5. A TinyUSB HID-device adapter sends the selected profile's report.

On the gamepad image, unsupported, invalid, or equal-strength ambiguous profile
matches never reach the mapper as connected input. Registration order is not a
compatibility rule. See [Controller profiles](CONTROLLER_PROFILES.md) for the
extension contract.

Red Monkey MPG never emits simultaneous axis commands. The mapper locks to one
dominant axis and requires all joystick axes to
return to center before switching. This prevents diagonal stick noise from
alternating commands. Platform adapters send an all-keys-released report on
disconnect, timeout, USB reset/suspend, pairing-mode entry, malformed HID
report, or internal error. Every such transition requires neutral controls
before output can re-arm.

The gamepad receiver's USB device is a composite HID keyboard plus CDC setup
port. Opening CDC locks motion output, releases every HID key, and bounds setup
parsing work per main-loop pass so a noisy host cannot starve stale-input
handling. A bounded transmit queue retains partial USB writes and pauses new
command frames until the prior response drains. Closing CDC does not
immediately enable output; neutral re-arm is mandatory.

The iPhone receiver exposes only the USB HID keyboard interface. Its BLE GATT
service accepts fixed-length, versioned frames with a CRC, sequence tracking,
and bounded enum/range validation. A direction touch supplies the held motion
intent. Disconnect, stale input, app backgrounding, malformed frames, or USB
loss releases output and requires a fresh neutral frame before re-arm.

Configurable builds use the Pico SDK's polled CYW43 architecture. Bluetooth
callbacks, TinyUSB work, configuration parsing, flash transactions, and mapper
updates therefore run serially on the main loop. A configurable target will not
compile without this mode; background IRQ callbacks must never touch TinyUSB or
mutate the active mapping concurrently.

In the gamepad receiver, configuration, the selected CNC output profile, and
the selected gamepad address are stored in alternating,
versioned, CRC-checked flash records with post-write readback. BTstack stores
Classic link keys separately. A newly discovered controller cannot become a
motion source until the physical L2 + R2 + Plus confirmation chord succeeds,
its link key exists, and the new configuration commits. Closing the app during
pairing discards the candidate and restores the last committed controller.

The controller profile requires the commissioned Lite 2 D-mode report shape
and 83-byte HID descriptor length. Commercial configuration additionally
requires an approved descriptor SHA-256; the receiver compares it before
accepting reports. Production qualification must still capture and approve the
hash and test every supported controller revision.

In the iPhone receiver, the app supplies the supported CNC profile choice as
part of the controlled session. The current firmware accepts only the compiled
MASSO profile and does not reuse or mutate the gamepad configuration record.

For the gamepad receiver, input-controller profiles and CNC-controller profiles
are separate registries.
Input profiles normalize HID reports; CNC profiles contain the verified
keyboard translation and supported capabilities for a machine controller.
Neither registry can alter dead-man, one-axis, neutral-rearm, timeout, or
disconnect-release safety policy. Unsupported IDs fail closed.

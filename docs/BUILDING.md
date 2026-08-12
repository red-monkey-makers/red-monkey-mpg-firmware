# Build and setup

## Host-side safety tests

Requirements: CMake 3.20+ and a C++17 compiler.

```sh
cmake -S . -B build/host
cmake --build build/host
ctest --test-dir build/host --output-on-failure
```

These tests exercise the mapper, protocol parser, flash record recovery, and
deterministic safety properties. They require no Pico SDK. Run the same suite
with AddressSanitizer and UndefinedBehaviorSanitizer before a release:

```sh
cmake -S . -B build/host-sanitize -DRED_MONKEY_MPG_ENABLE_HOST_SANITIZERS=ON
cmake --build build/host-sanitize
ctest --test-dir build/host-sanitize --output-on-failure
```

## Pico receiver firmware

1. Install a current Raspberry Pi Pico SDK with RP2350/Pico 2 W support and its
   toolchain. Initialize the SDK submodules.
2. Set `PICO_SDK_PATH` to that SDK directory.
3. Configure for the Pico 2 W:

```sh
cmake -S . -B build/pico \
  -DRED_MONKEY_MPG_BUILD_HOST_TESTS=OFF \
  -DRED_MONKEY_MPG_BUILD_PICO=ON \
  -DPICO_BOARD=pico2_w
cmake --build build/pico --target \
  red_monkey_mpg_production_receiver red_monkey_mpg_mobile_receiver
```

The build produces two alternative images:

| Build output | Release name | Wireless input |
|---|---|---|
| `build/pico/red-monkey-mpg-gamepad-receiver.uf2` | `red-monkey-mpg-gamepad-receiver-<version>.uf2` | Supported gamepad |
| `build/pico/red-monkey-mpg-mobile-receiver.uf2` | `red-monkey-mpg-mobile-receiver-<version>.uf2` | Red Monkey MPG iPhone app |

4. Hold BOOTSEL while connecting the Pico, then copy exactly one UF2 to the
   RPI-RP2 drive. Read [firmware variants](FIRMWARE_VARIANTS.md) before choosing.

All diagnostic, stub, preview, and commissioning targets are excluded from the
default build. Build one explicitly only when following its matching test
document. Never distribute those images as machine firmware.

## iPhone receiver

Build the separate iPhone BLE-to-USB keyboard image with:

```sh
cmake --build build/pico --target red_monkey_mpg_mobile_receiver
```

The output is `build/pico/red-monkey-mpg-mobile-receiver.uf2`. It uses the
MASSO CNC profile and is intentionally separate from the gamepad receiver so a
known-good image remains available for rollback. Before connecting it to a
machine, complete every stage in
[`MOBILE_RECEIVER_MASSO_TEST.md`](MOBILE_RECEIVER_MASSO_TEST.md).

## Commercial build guards

The default `0xCAFE` USB vendor ID is for development only. A commercial build
also requires a separately reviewed RP2350 signed-boot provisioning process:

```sh
cmake -S . -B build/commercial \
  -DRED_MONKEY_MPG_BUILD_HOST_TESTS=OFF \
  -DRED_MONKEY_MPG_BUILD_PICO=ON \
  -DRED_MONKEY_MPG_COMMERCIAL_RELEASE=ON \
  -DRED_MONKEY_MPG_USB_VID=0x1234 \
  -DRED_MONKEY_MPG_LITE2_DESCRIPTOR_SHA256=<64-hex-qualified-hash> \
  -DRED_MONKEY_MPG_SECURE_BOOT_PROVISIONED=ON
```

Replace `0x1234` with a VID/PID combination you are authorized to use. The
`RED_MONKEY_MPG_SECURE_BOOT_PROVISIONED` flag is an assertion, not provisioning code;
it must never be enabled merely to bypass the guard. Verify the final artifact
and device OTP policy using the approved manufacturing procedure.

For the full reproducible build, test, SBOM, checksum, and optional manifest-
signing workflow, follow [the controlled release process](RELEASE_PROCESS.md).

## Bluetooth discovery diagnostic

The Pico build also creates `red_monkey_mpg_bt_scan.uf2`. This safe bench image scans
Bluetooth Classic devices and prints discovery results over USB serial. It has
no USB keyboard output and cannot command a CNC controller. Flash it while connected only
to a development computer, open the Pico's USB serial port, and put the Lite 2
into pairing mode. A matching discovery line provides the controller name and
Bluetooth address needed for the allow-listing step.

After recording the controller address, configure `RED_MONKEY_MPG_CONTROLLER_ADDRESS`
and build `red_monkey_mpg_hid_dump`. This second safe diagnostic connects to that one
controller and prints raw HID reports over USB serial. Capture the centered
report, then move or press exactly one control at a time so each byte or bit can
be mapped unambiguously. It also prints the exact descriptor bytes and SHA-256
needed by the commercial allowlist, and contains no USB keyboard output.

After commissioning, the same diagnostic also passes verified Lite 2 reports
through `Lite2ReportParser` and `ControlMapper`. Lines beginning with `MAPPED`
show normalized axes, dead-man/rate state, and the semantic result (`RELEASE`
or one `JOG` axis). This exercises the production input path without including
or emitting a USB keyboard report.

## Pairing plan

The Lite 2 supports multiple controller modes. The implemented adapter must
document and require one tested mode because button/axis reports can differ by
mode and firmware revision. Pair first on the bench, log the HID descriptor and
raw reports, then create fixtures for every control before enabling USB output.

## Commissioning checklist

1. Confirm the cable is data-capable and the Pico enumerates independently.
2. Verify all gamepad inputs in a diagnostic-only firmware build.
3. Verify emitted keys against a normal computer keyboard viewer.
4. Select a CNC controller profile and confirm its shortcuts with motion power disabled.

## USB keyboard bench image

Build the bounded Mac-only keyboard diagnostic with:

```sh
cmake --build build --target red_monkey_mpg_keyboard_bench
```

Flash `build/red_monkey_mpg_keyboard_bench.uf2` only after reading
[`USB_KEYBOARD_BENCH_TEST.md`](USB_KEYBOARD_BENCH_TEST.md). Never connect that
diagnostic image to a machine because its fixed sequence intentionally
exercises all documented jog direction keys for the compiled test profile.

## Live bridge bench image

After the bounded keyboard test passes, build the controller-driven Mac test:

```sh
cmake --build build --target red_monkey_mpg_live_bridge_bench
```

Follow [`LIVE_BRIDGE_BENCH_TEST.md`](LIVE_BRIDGE_BENCH_TEST.md). This target
combines Bluetooth input with USB keyboard output, but remains explicitly
restricted to computer-side commissioning.
5. Test every release/fault path from `SAFETY.md`.
6. Enable one axis at minimum rate and validate direction.
7. Repeat for remaining axes; only then consider higher rates.

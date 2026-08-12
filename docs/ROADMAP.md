# Roadmap

## 0.1 — Bench input

- [x] Integrate Pico SDK BTstack HID host over CYW43
- [x] Pair/reconnect one allow-listed Lite 2
- [x] Capture and fixture HID reports for the selected controller mode

## 0.2 — Bench USB output

- [x] Translate a verified CNC-controller shortcut set in portable code
- [x] Add TinyUSB keyboard descriptor and explicit press/release state machine
- [x] Test output with a keyboard event viewer and automated fixtures
- [x] Validate the live Bluetooth-to-USB path and fail-closed transitions

## 0.3 — Safe machine commissioning

- [x] Implement arm state, held dead-man, watchdog, and disconnect release
- [x] Pin and label a profile-specific commissioning image
- [x] Validate directions and rates one axis at a time on one prototype
- [x] Add host fault, persistence, parser, and property tests
- [ ] Repeat hardware fault injection across a production-representative lot

## 0.4 — Configurable receiver

- [x] Edge-triggered step resolution and one-axis selection
- [x] Constrained user mappings with versioned, redundant storage
- [x] Production-safe pairing transaction and browser setup workflow

## 0.5 — CNC controller profiles

- [x] Add a fail-closed output-profile registry
- [x] Persist profile selection without changing the existing flash record size
- [x] Expose compiled profiles and capabilities to the setup app
- [x] Preserve the verified MASSO G3 Touch 5.13 translator as the first profile
- [ ] Qualify and add a second CNC controller profile
- [ ] Add profile-specific automated keyboard fixtures and commissioning records

## 0.6 — Input controller profiles

- [x] Add a fail-closed profile registry and normalized parser contract
- [x] Migrate the Lite 2 D-input parser into the first built-in profile
- [x] Reject unsupported, incomplete, and ambiguous profiles in host tests
- [ ] Persist the selected profile ID with each controller bond
- [ ] Expose detected profile, maturity, and compatibility status to the setup app
- [ ] Add a diagnostic-only generic HID inspector for profile contributors
- [ ] Qualify a second controller through the complete contribution workflow

## 0.7 — iPhone receiver

- [x] Add a bounded BLE command protocol and shared safety mapper
- [x] Add a separately flashed iPhone-to-USB-keyboard receiver
- [x] Add mobile protocol/bridge tests and staged MASSO qualification
- [x] Package gamepad and iPhone UF2 images in the same firmware release
- [ ] Qualify the iPhone receiver across a production-representative hardware lot

## 1.0 — Commercial release gates

- Obtain an authorized USB VID/PID and update firmware/app filters
- Provision and independently verify RP2350 signed boot and update recovery
- Capture qualified descriptor hashes and freeze an approved Lite 2
  hardware/firmware compatibility matrix
- Complete multi-unit power-loss, RF-loss, suspend/resume, brownout, and soak tests
- Complete hazard analysis, regulatory assessment, external security review, and
  production fixture/traceability procedures
- [x] Add reproducible UF2 build comparison, SPDX inventories, and checksum
  staging
- Establish protected release signing, support policy, and private
  vulnerability-reporting channel

## Later

- Optional wired gamepad and physical MPG front ends
- Enclosure, mounting, and EMC guidance

# Production and commercial readiness review

Review date: 2026-08-10  
Reviewed candidate: `0.3.2-production-rc1`

## Verdict

The codebase is substantially hardened for continued prototype and controlled
release-candidate testing, but it is **not ready for commercial sale**. Passing
software tests does not certify a wireless machine-motion accessory.

## Controls implemented

- Held dead-man, one dominant axis, center-before-axis-change, stale-report
  timeout, neutral re-arm, and immediate all-keys release on relevant faults
- Release on Bluetooth loss, malformed controller report, USB unmount/suspend,
  HID send failure, setup entry, and watchdog reset
- Setup work bounded per loop; overlong/control-byte frames rejected; exact
  protocol/schema and symbolic allowlists enforced
- CDC responses are retained across partial TinyUSB writes; command parsing is
  backpressured at frame boundaries so responses remain ordered and complete
- Pairing restricted to an app-selected candidate plus physical controller
  chord; uncommitted candidates never control motion and are discarded if the
  setup session closes
- Redundant, versioned, CRC-checked persistent records with post-write readback
- Browser serial filters, explicit disconnect, receiver-confirmed saves,
  bounded/untrusted device messages, and restrictive response headers
- Bluetooth callbacks, USB setup traffic, mapping changes, and persistence are
  serialized in one polled main-loop execution context
- Diagnostic and commissioning targets excluded from default builds
- Commercial CMake guard rejects the development VID and requires an explicit
  signed-boot manufacturing-process assertion plus a qualified controller HID
  descriptor SHA-256
- Controlled release tooling performs two clean firmware builds, byte compares
  them, runs sanitizer/configurator checks, and generates checksums and SPDX
  inventories without generating or storing signing keys

## Verification completed

- Five host test executables pass normally and under ASan/UBSan, including
  unsupported/incomplete/ambiguous controller-profile rejection
- Deterministic property tests exercise 20,000 state sequences plus full analog
  byte ranges, wraparound timing, invalid mappings, and keyboard report bounds
- Parser tests cover protocol/schema confusion, duplicate critical fields,
  control bytes, oversize input, and bounded service work
- Persistence tests cover dual-record selection, torn/corrupt records, failed
  writes/readback, invalid addresses, and stale-address clearing
- The independently versioned configurator's lint, production build, rendered
  security tests, and production-only dependency audit pass
- Pico 2 W ARM release firmware builds with strict warnings on owned sources
- Firmware version/release date are pinned in binary metadata and independent
  clean UF2 builds are compared before handoff

## Blocking requirements before sale

1. **USB identity:** obtain an authorized VID/PID, update firmware and app
   filters, and complete applicable USB compliance/licensing work.
2. **Boot and updates:** design, provision, and independently verify RP2350
   signed boot, key custody/rotation, anti-rollback policy, recovery, and secure
   release signing. The current RC reports extra boot security as disabled.
3. **Controller identity qualification:** capture the diagnostic
   `DESCRIPTOR_SHA256` from each controller/firmware revision intended for sale,
   validate the matching report semantics, and approve the exact hash. The
   enforcement mechanism is implemented; no production hash is approved yet.
4. **Hardware qualification:** test a production-representative lot for power
   interruption at every flash phase, brownout, USB suspend/reset, controller
   replacement, RF loss/interference, watchdog recovery, long-duration soak,
   and repeated flash endurance.
5. **Machine compatibility:** repeat direction, step, override, stuck-key, and
   loss-of-link tests on every advertised CNC controller model/software/profile
   combination. Requalify the polled Bluetooth build and every profile-specific
   key usage before publishing compatibility.
6. **Product safety:** complete a formal hazard/risk analysis with qualified
   review, warnings/instructions, misuse analysis, support limits, and product
   liability/regulatory advice. Never market Red Monkey MPG as an E-stop.
7. **Regulatory/manufacturing:** assess end-product radio/EMC/electrical,
   enclosure, antenna, labeling, regional, and host-integration obligations;
   establish fixtures, per-unit test records, serial traceability, calibration
   policy, and controlled component substitutions.
8. **Release operations:** the reproducible build, checksum, and SPDX tooling is
   implemented. Still establish protected signing/key custody, immutable source
   and toolchain archives, independent signature verification, approval records,
   rollback/field-update procedure, vulnerability intake, and incident response.
9. **Independent review:** commission external firmware/web security review and
   machine-safety/fault-injection validation.
10. **Configurator compatibility:** pin the independently released configurator
    version tested with each firmware release and retain its CI, SBOM, and
    dependency-review evidence in the approval record.

## Known residual design limits

- Bluetooth Classic Just Works does not provide authenticated MITM protection;
  the physical chord is an application-level provisioning control.
- The setup channel has no account login. Browser permission and physical USB
  possession are its intended authorization boundary.
- The embedded setup parser is intentionally narrow, not a general JSON parser.
- A browser CSP currently permits inline framework code required by vinext.

Treat every change to mapping logic, USB descriptors, Bluetooth security,
persistence, configuration parsing, watchdog behavior, or toolchain versions as
a safety-relevant change requiring regression and hardware requalification.

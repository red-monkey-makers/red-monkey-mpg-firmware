# Controller profiles

Controller profiles translate one known HID report format into Red Monkey MPG's
normalized `GamepadState`. They are deliberately upstream of `ControlMapper`:
a profile can describe hardware, but it cannot weaken motion-safety policy or
emit USB keyboard commands. These are **input-controller profiles** and are
separate from [CNC controller profiles](CNC_CONTROLLER_PROFILES.md).

## Selection model

`ControllerProfileRegistry` evaluates the Bluetooth transport, advertised
identity when available, device class, and HID descriptor characteristics.
Matches have three strengths: unsupported, compatible, and exact. The highest
single match is selected. Equal-strength matches are ambiguous and motion
remains disabled.

Bonded Bluetooth reconnects may not repeat the advertised name or device class.
A compatible descriptor match is therefore supported. As more profiles are
added, the selected profile ID should be persisted with the bond so reconnects
do not depend on descriptor-only inference.

## Required profile contract

Every motion-capable profile provides:

- a stable, namespaced profile ID and user-facing name;
- transport and maturity metadata;
- a bounded identity/descriptor matcher;
- a parser that either replaces a complete `GamepadState` or returns false;
- X, Y, and Z control capability; and
- at least one held dead-man candidate.

The registry rejects incomplete profiles. Parser failure, unsupported identity,
and ambiguous selection all stay outside the safety engine and force release.

## Adding a controller

1. Capture its advertised identity, transport, complete HID descriptor, and
   centered report.
2. Capture every axis and button separately, including minimum, center,
   maximum, cross-axis drift, and malformed/short reports.
3. Implement a profile-specific matcher and parser. Do not edit
   `ControlMapper` for controller quirks.
4. Add raw report fixtures and exhaustive normalization/bounds tests.
5. Add the profile to `builtin_controller_profiles()`.
6. Test an ambiguous synthetic match and prove it remains fail-closed.
7. Run host tests, sanitizers, computer-side keyboard testing, and the complete
   hardware fault checklist before changing the profile maturity.

## Fixed safety boundary

Profiles cannot configure or bypass the held dead-man requirement, one-axis
selection, center-before-axis-switch, stale timeout, neutral re-arm, disconnect
release, malformed-report release, USB failure handling, or the selected CNC
profile's compiled keyboard-command allowlist. Those rules remain owned by the
shared mapper and output layers.

The first built-in profile is `8bitdo.lite2.dinput`, currently marked
`community_tested`. Generic HID inspection may be added later, but unknown
controllers must remain diagnostic-only until a complete compatible profile is
selected.

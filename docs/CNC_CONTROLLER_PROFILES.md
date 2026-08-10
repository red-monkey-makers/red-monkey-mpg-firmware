# CNC controller profiles

CNC controller profiles are the output side of Red Monkey MPG. They translate
an already safety-filtered semantic `OutputFrame` into the exact USB keyboard
report expected by one CNC controller model and software family.

They are distinct from [input-controller profiles](CONTROLLER_PROFILES.md),
which parse Bluetooth gamepad reports. An input profile answers “what did the
operator do?” A CNC profile answers “which approved keyboard report represents
that safe action on this machine controller?”

## Profile contract

Every compiled CNC profile contains:

- a stable machine-readable ID and user-facing name;
- a bounded `OutputFrame` to `KeyboardReport` translator;
- declared capabilities such as step-resolution selection and override
  adjustment;
- an explicit keyboard-command allowlist;
- source/version notes for every shortcut;
- host-side keyboard fixtures; and
- a motion-disabled commissioning procedure and compatibility record.

Profiles cannot emit more than one axis command at once and cannot alter the
dead-man, center-before-switch, stale timeout, neutral-rearm, disconnect
release, malformed-input, or setup-session lockout rules. Unknown profile IDs
produce an all-keys-released report.

## Selection and persistence

The configurator asks the receiver for `GET_CNC_PROFILES` and displays only
profiles compiled into that firmware. `SET_CONFIG` stores the selected
`cncProfile` with the constrained gamepad mapping. Selection is persisted in a
previously reserved byte of the redundant schema-1 flash record, so existing
records remain compatible: value zero selects the original MASSO profile.

Raw USB usages, arbitrary macros, and user-supplied key chords are never
accepted over the setup protocol. Adding a new profile requires firmware code,
tests, review, and machine qualification.

## Built-in profiles

| Profile ID | Controller | Status |
|---|---|---|
| `masso-g3-touch-5.13` | MASSO G3 Touch 5.13 | Prototype commissioned; commercial qualification pending |

See [MASSO G3 Touch 5.13](cnc-profiles/MASSO_G3_TOUCH_5_13.md) for its keyboard
mapping. Supporting multiple CNC controllers means adding more reviewed rows
and translators—not making raw keyboard output configurable.

## Adding a CNC controller

1. Record the exact controller model and software version.
2. Obtain its authoritative external-keyboard shortcut documentation.
3. Verify every shortcut on a non-moving bench setup.
4. Implement a pure translator and declare its capabilities.
5. Add positive, release, unsupported-ID, and one-axis host fixtures.
6. Add the profile to the compiled registry and bounded protocol response.
7. Complete motion-disabled direction, rate, override, disconnect, and fault
   commissioning on every advertised controller/software combination.
8. Publish the compatibility evidence before calling the profile supported.

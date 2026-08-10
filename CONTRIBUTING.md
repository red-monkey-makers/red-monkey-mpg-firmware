# Contributing

Keep transport code separate from `ControlMapper`, add tests for every mapping
or safety change, and default new features to disabled. Pull requests affecting
motion must describe release, disconnect, stale-input, and reboot behavior.
Run the host test suite before submitting changes.

Run both normal and ASan/UBSan suites and the Pico cross-build before requesting
release review. Configurator changes belong in the separate Red Monkey MPG
Configurator repository and must pass that project's CI. Changes to
Bluetooth pairing/security, USB descriptors, persistence, setup parsing,
watchdog behavior, or build tooling are safety-relevant even when the mapper is
unchanged. Never weaken a fail-closed invariant to make a new mapping possible.

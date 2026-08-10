## Summary

Describe the change and the hardware, firmware, and controller profiles affected.

## Safety impact

- [ ] Dead-man behavior is unchanged or explicitly tested.
- [ ] One-axis and neutral-before-switch behavior remains enforced.
- [ ] Disconnect, malformed input, timeout, and USB failure release all keys.
- [ ] New controller input cannot bypass the shared safety engine.

## Verification

- [ ] Normal host tests
- [ ] ASan/UBSan host tests
- [ ] Pico 2 W cross-build
- [ ] Hardware testing described when required

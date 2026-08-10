# CNC profile: MASSO G3 Touch 5.13

- Profile ID: `masso-g3-touch-5.13`
- Controller: MASSO G3 Touch
- Verified software family: 5.13
- Status: prototype commissioned; commercial qualification pending

This is the first built-in CNC controller profile. Its presence documents one
supported output translator; it does not make Red Monkey MPG a MASSO-specific
project.

## Keyboard mapping

| Semantic action | USB keyboard shortcut |
|---|---|
| X− / X+ | Left Arrow / Right Arrow |
| Y− / Y+ | Down Arrow / Up Arrow |
| Z− / Z+ | D / U |
| Continuous jog | Shift + direction |
| Step 1.00 / 0.50 / 0.10 / 0.01 mm | 1 / 2 / 3 / 4 |
| Override increase / decrease | Numeric keypad + / - |

The direction keys without Shift perform step jogging. Shift plus a direction
performs continuous jogging at the controller's configured jog feed rate. The
profile emits keypad Plus/Minus only; it does not select an override context.

Shortcut source and commissioning details remain in
[MASSO G3 Touch 5.13 commissioning](../MASSO_G3_513_COMMISSIONING.md). Physical
direction, active screen, displayed resolution, and override behavior must be
recorded for each supported software release.

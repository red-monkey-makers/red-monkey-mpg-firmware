# Control mapping

This document describes the semantic control layout. The selected
[CNC controller profile](CNC_CONTROLLER_PROFILES.md) determines which verified
USB keyboard reports implement these actions on a particular machine.

| Control | Semantic action | Safety behavior |
|---|---|---|
| Left stick X/Y | Jog X/Y | Dominant axis locks until all sticks are centered |
| Right stick Y | Jog Z | Z takes priority; deadzone applied |
| L2 (hold) | Motion enable / dead-man | Releasing it releases all USB keys |
| R2 (hold) | Continuous jog rate | Ignored unless the dead-man is held |
| L1 | Configurable motion modifier | Cannot alias dead-man, continuous, or fixed precision |
| R1 (hold) | Precision rate | Fixed in mapping schema 1 |
| A | Cycle/select step resolution | One event per new press; only while neutral and disarmed |
| Plus | Increase override when supported by the CNC profile | One event per new press |
| Minus | Decrease override when supported by the CNC profile | One event per new press |
| D-pad | Unassigned | Reserved for future constrained actions |
| B | Cancel / release all | Immediately releases output and requires neutral re-arm |
| X / Y | Configurable approved non-motion actions | Never accepts arbitrary USB keys |
| Start / Select / Star | Unassigned | Potentially destructive actions remain unavailable |

The configurator sends symbolic choices only. Firmware validates them against
the fixed safety policy and the compiled CNC profile registry before writing a
redundant configuration record.

## Step-resolution behavior

The semantic resolution values are 0.01, 0.10, 0.50, and 1.00 mm. A profile
may expose all, some, or none of them. A cycle action advances only when the
dead-man is released and every assigned stick axis is centered. Holding a face
button never repeats the selection.

The operator must confirm the visible resolution on the CNC controller. Red
Monkey MPG cannot read the machine's current UI state and does not assume the
display is synchronized after startup.

## One-axis policy

Red Monkey MPG emits exactly one axis command at a time, including on CNC
controllers that may accept multiple simultaneous keyboard directions.

When motion begins, the left-stick axis with the larger absolute deflection is
selected. That axis remains locked until **all joystick axes return to the
deadzone**. Crossing a diagonal cannot switch directly from X to Y. Right-stick
Z follows the same center-to-switch rule.

This policy belongs to the shared safety mapper, not to a machine-specific
profile, so adding a CNC controller cannot enable diagonal or multi-axis jog.

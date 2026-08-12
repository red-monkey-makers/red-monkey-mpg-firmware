# Safety

Red Monkey MPG is not a safety-rated control and must never be the only means of
stopping a machine.

- Keep the machine's physical, hard-wired E-stop accessible and tested.
- Commission with spindle/laser/plasma output disabled and axes clear.
- Begin with reduced jog velocity and acceleration.
- Confirm axis direction from a safe position. The gamepad image inverts the
  Y stick by convention; the iPhone image uses explicitly labeled buttons.
- Require held motion intent for every command: gamepad dead-man plus
  direction, or an actively held iPhone direction control.
- Treat Bluetooth disconnect, stale reports, reboot, USB reset, and exceptions
  as an immediate all-keys-release condition.
- Never bind cycle start, spindle start, tool change, or program execution to a
  single gamepad press or phone tap.
- Verify the selected CNC profile's exact shortcut set and behavior on a
  non-moving test setup.
- Do not rely on the pendant near radio interference or when loss of control
  could create a hazard.

Before each use, test E-stop, normal direction/dead-man release, wireless-input
power-off or app backgrounding, Bluetooth loss, and out-of-range behavior. If
any test leaves motion commanded, disconnect Red Monkey MPG and correct the
fault before use.

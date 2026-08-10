# Safety

Red Monkey MPG is not a safety-rated control and must never be the only means of
stopping a machine.

- Keep the machine's physical, hard-wired E-stop accessible and tested.
- Commission with spindle/laser/plasma output disabled and axes clear.
- Begin with reduced jog velocity and acceleration.
- Confirm axis direction from a safe position; gamepad Y axes are inverted in
  software by convention.
- Require the held dead-man control for every motion command.
- Treat Bluetooth disconnect, stale reports, reboot, USB reset, and exceptions
  as an immediate all-keys-release condition.
- Never bind cycle start, spindle start, tool change, or program execution to a
  single gamepad press.
- Verify the selected CNC profile's exact shortcut set and behavior on a
  non-moving test setup.
- Do not rely on the pendant near radio interference or when loss of control
  could create a hazard.

Before each use, test E-stop, dead-man release, controller power-off, Bluetooth
loss, and out-of-range behavior. If any test leaves motion commanded, disconnect
Red Monkey MPG and correct the fault before use.

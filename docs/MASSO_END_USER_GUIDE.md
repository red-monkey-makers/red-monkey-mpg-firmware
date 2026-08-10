# Red Monkey MPG user guide — MASSO G3 Touch

<p align="center">
  <img src="assets/red-monkey-mpg-banner.webp" alt="Red Monkey MPG — Pair once. Jog with confidence." width="100%">
</p>

This guide is for an end user who received a Red Monkey MPG receiver already
programmed, configured, and paired for a MASSO G3 Touch. No firmware tools or
setup application are required for normal operation.

For firmware installation or updates, follow the separate
[Firmware installation guide](FIRMWARE_INSTALLATION.md) before returning the
receiver to machine service.

This guide applies to the supplied configuration:

- MASSO G3 Touch software profile 5.13
- 8BitDo Lite 2 controller in **D mode**
- A receiver paired to that controller by the supplier

> **Important:** Red Monkey MPG is a convenience control, not a safety device.
> It does not replace a hard-wired emergency stop, machine guarding, limit
> switches, or normal safe operating procedures. Keep the physical E-stop
> accessible whenever the pendant is in use.

## Quick control reference

| Control | Function |
|---|---|
| L2, held | Dead-man / motion enable |
| Left stick left / right | X− / X+ |
| Left stick up / down | Y+ / Y− |
| Right stick up / down | Z+ / Z− |
| R2, held with L2 | Continuous jog |
| A, pressed once | Cycle step size: 0.01, 0.10, 0.50, 1.00 mm |
| Plus, pressed once | Increase the active MASSO override or jog feed |
| Minus, pressed once | Decrease the active MASSO override or jog feed |
| B | Cancel and release all pendant output |

The D-pad, X, Y, L1, R1, Start, Select, and Star controls have no machine
function in the supplied MASSO configuration. The Home button is used only to
turn on and reconnect the controller.

## Before every use

1. Stop any running program and make sure the spindle, laser, plasma source,
   or other cutting equipment is off.
2. Clear people, tools, clamps, and loose material from the machine's possible
   travel path.
3. Confirm that the hard-wired E-stop is accessible and working.
4. Release L2, R2, and every other button. Center both sticks.
5. Begin with the lowest practical MASSO jog feed rate and the smallest step
   size.

For first use, after service, or after a MASSO software change, disable cutting
energy and perform the validation checklist near the end of this guide before
normal operation.

## Connect and get ready

1. Plug the Red Monkey MPG receiver into the MASSO USB keyboard port.
2. Put the 8BitDo Lite 2 mode switch in **D**.
3. Press the controller's **Home** button once to turn it on.
4. Do **not** press the small Pair button during normal startup. Pairing was
   completed by the supplier.
5. Release L2 and all other buttons, and center both sticks.
6. Wait for the receiver LED to become solid.
7. Open the MASSO **F3 Jog/Rapid** screen.
8. With L2 released and both sticks centered, press **A** once. Confirm that
   MASSO visibly selects the 0.01 mm step size before jogging.

Always trust the value shown on the MASSO screen. The receiver sends keyboard
commands but cannot read MASSO's current screen, step size, or feed setting.

## Receiver LED

| LED pattern | Meaning | What to do |
|---|---|---|
| Brief flash about once per second | Waiting for the controller | Confirm D mode, charge the controller, and press Home once |
| Fast flashing | Connected but not armed | Release L2 and every button; center both sticks |
| Solid | Connected, centered, and ready | The pendant can accept a deliberate dead-man and direction input |

A solid LED does not mean the machine is safe to move. It only reports the
receiver's connection and neutral state.

## Step jog

Step jog moves one axis by the step size displayed on MASSO.

1. Confirm the desired step size on the F3 screen.
2. Hold **L2**.
3. Briefly move one stick in the desired direction.
4. Return the stick to center.
5. Release L2 when finished.

Moving a stick without L2 held must not move the machine.

## Continuous jog

Continuous jog moves while the controls are held, at the jog feed configured
on MASSO.

1. Confirm a safe jog feed on the F3 screen.
2. Hold **L2** and **R2**.
3. Move one stick in the desired direction.
4. To stop, release **L2 first**, then center the stick and release R2.

Do not release R2 while continuing to hold a deflected stick. Doing so changes
the request from continuous jog to step jog. Releasing L2 first immediately
releases all keyboard output from the pendant.

## Axis directions and single-axis behavior

| Stick direction | MASSO movement |
|---|---|
| Left stick left | X− |
| Left stick right | X+ |
| Left stick up | Y+ |
| Left stick down | Y− |
| Right stick up | Z+ |
| Right stick down | Z− |
| Right stick left / right | No movement |

Red Monkey MPG intentionally permits only one axis at a time. On a diagonal
left-stick movement, the more strongly deflected X or Y axis is selected. Z
takes priority when the right stick requests Z movement.

Once an axis is selected, center **both sticks** before selecting another axis.
This prevents diagonal movement and accidental axis changes. If the pendant
seems unwilling to change axes, release L2 and center both sticks completely.

## Change the step size

With L2 released and both sticks centered, press **A once** for each change:

```text
0.01 mm → 0.10 mm → 0.50 mm → 1.00 mm → 0.01 mm
```

Confirm the highlighted value on the MASSO F3 screen after every change.
Holding A does not repeatedly cycle the selection. After the receiver restarts,
one press of A selects 0.01 mm in the supplied profile, but the MASSO display
is always the final authority.

## Change jog feed or override

On the MASSO F3 screen, with L2 released and both sticks centered:

- Press **Plus** once to increase the active jog feed or override.
- Press **Minus** once to decrease it.

Watch the displayed MASSO value and use individual presses. The receiver sends
numeric-keypad Plus or Minus; MASSO decides what those keys control on the
current screen. Avoid pressing them on other screens or when the displayed
effect is unclear.

## Stop movement

For a normal stop, release **L2**. The receiver immediately releases all USB
keyboard keys. Then center both sticks before jogging again.

Pressing **B** also cancels all pendant output and requires the sticks and
buttons to return to neutral before motion can resume. B is useful for clearing
the pendant's current request, but it is **not an emergency stop**.

For any dangerous or unexpected machine movement, use the machine's physical
E-stop. Do not depend on Bluetooth, USB, software, or the B button for an
emergency stop.

## Reconnect after power-off or signal loss

The supplied controller and receiver remember each other.

1. Keep the controller in D mode.
2. Release every button and center both sticks.
3. Press Home once to turn on the controller.
4. Wait for the receiver LED to become solid.
5. Confirm the MASSO screen, step size, and jog feed before moving.

Do not put the controller in pairing mode. If the Bluetooth connection drops
during use, the receiver releases its keyboard output and requires a centered,
neutral controller after reconnection.

## Troubleshooting

### The receiver keeps flashing slowly

- Confirm that the controller switch is in D mode.
- Charge the controller and press Home once.
- Move the controller closer to the receiver.
- Do not press the Pair button.

### The receiver flashes quickly and never becomes solid

- Release L2, R2, and every other button.
- Center both sticks and make sure neither is being touched.
- Turn the controller off and on again while leaving all controls neutral.

### The LED is solid, but the machine does not move

- Confirm MASSO is on the F3 Jog/Rapid screen.
- Hold L2 before moving a stick.
- Confirm that a setup/configurator application or serial terminal is not open;
  setup mode intentionally locks motion output.
- With the machine in a safe state, unplug and reconnect the receiver, reconnect
  the controller normally, and wait for a solid LED.

### The selected axis will not change

Release L2 and center both sticks completely. A new axis cannot be selected
until every stick axis has returned to neutral.

### A, Plus, or Minus has no visible effect

Release L2, center both sticks, and press the button once. Confirm that MASSO is
on the appropriate F3 screen and watch its displayed value.

### Movement is unexpected or does not stop

Press the physical E-stop, power down the machine according to its operating
procedure, and unplug the receiver. Do not use the pendant again until the
receiver, controller, configuration, and machine behavior have been checked.

## First-use and periodic validation

Perform this check with cutting energy disabled, the work area clear, and a
very low jog feed:

- [ ] The physical E-stop stops the machine independently of the pendant.
- [ ] The receiver becomes solid only after all controls are neutral.
- [ ] Moving either stick without L2 held causes no movement.
- [ ] Releasing L2 stops continuous motion immediately.
- [ ] Turning off the controller during a low-risk jog releases motion.
- [ ] X−, X+, Y−, Y+, Z−, and Z+ match the machine display and physical axes.
- [ ] A cycles the four displayed MASSO step sizes in the documented order.
- [ ] Plus and Minus change the intended displayed MASSO value.
- [ ] A diagonal stick input never moves two axes at once.
- [ ] Changing axes requires both sticks to return to center.

Repeat the relevant checks after firmware changes, MASSO updates, controller
replacement, machine service, or any unexplained behavior.

## Care and support

- Keep the receiver and controller dry and away from conductive debris.
- Inspect the USB cable, receiver enclosure, and controller before use.
- Do not modify the configuration or pair a replacement controller unless
  following instructions from the supplier or project documentation.
- When requesting support, provide the receiver label/serial number, MASSO
  software version, controller model, LED pattern, and a description of what is
  shown on the MASSO screen.


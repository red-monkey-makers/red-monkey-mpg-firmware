# Choose a receiver firmware image

Every GitHub firmware release contains two installable UF2 images for the
Raspberry Pi Pico 2 W. They are alternative receiver personalities. Install
one—not both—according to the wireless control you intend to use.

| Release asset | Wireless input | Setup | Current CNC profile |
|---|---|---|---|
| `red-monkey-mpg-firmware-<version>.uf2` | Supported Bluetooth gamepad; currently 8BitDo Lite 2 in D mode | Red Monkey MPG browser configurator | Selectable; currently MASSO G3/G3 Touch 5.13 |
| `red-monkey-mpg-iphone-receiver-<version>.uf2` | Red Monkey MPG iOS app | Profile selected in the iPhone app; no browser configurator | MASSO G3/G3 Touch 5.13 |

## Gamepad receiver

Choose the gamepad image when the operator will use the commissioned 8BitDo
Lite 2. The browser configurator pairs the controller and saves the constrained
mapping and CNC profile. Normal motion requires L2 plus a stick direction; R2
selects continuous jog and A cycles the supported MASSO step sizes.

Read the [gamepad + MASSO user guide](MASSO_END_USER_GUIDE.md) and complete the
[gamepad receiver test](PRODUCTION_RECEIVER_TEST.md).

## iPhone receiver

Choose the iPhone image when the operator will use the Red Monkey MPG app. The
Pico advertises as **Red Monkey MPG** over Bluetooth Low Energy. The app chooses
Single Step or Continuous mode and the direction button itself supplies the
held motion intent; there is no separate on-screen dead-man button.

The iPhone image does not accept a gamepad and does not expose the browser
configurator. Read the [iPhone + MASSO user guide](IPHONE_END_USER_GUIDE.md)
and complete the staged [iPhone receiver test](MOBILE_RECEIVER_MASSO_TEST.md).

## Switching variants

Switching is a normal BOOTSEL firmware installation. It replaces the running
application; it does not combine the two wireless transports. Always:

1. Disconnect the receiver from the CNC controller.
2. Install the selected, checksum-verified UF2 using the
   [firmware installation guide](FIRMWARE_INSTALLATION.md).
3. Reconnect or pair the intended wireless input.
4. Repeat that variant's complete computer-side and motion-disabled checks
   before returning it to machine service.

Keep the previous known-good UF2 available for rollback. Never assume a saved
gamepad mapping applies to the iPhone receiver or that an iPhone connection
configures the gamepad receiver.

## Safety behavior shared by both images

Both receiver variants enforce one axis at a time, neutral before changing
axes, bounded input parsing, stale-input timeout, release on wireless or USB
loss, and neutral re-arm after a fault. Neither variant is a safety-rated
control or a substitute for the machine's hard-wired E-stop.

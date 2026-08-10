# USB keyboard bench test

`openmpg_keyboard_bench.uf2` is a bounded commissioning image for a normal
computer. **Do not connect this image to any machine controller.** It
deliberately exercises every jog direction in the compiled bench profile.

The device identifies itself as `Red Monkey MPG Keyboard Bench Test`. Eight seconds
after USB enumeration, it emits this sequence exactly once:

1. Left Arrow (X-)
2. Right Arrow (X+)
3. Down Arrow (Y-)
4. Up Arrow (Y+)
5. D (Z-)
6. U (Z+)
7. Shift + Right Arrow (rapid X+)

Each key is held for 600 ms, followed by an explicit all-keys-released report
for 600 ms. It cannot repeat without a full Pico power cycle and reflash/reboot.

## Mac test procedure

1. Disconnect the Pico from every machine controller.
2. Open a keyboard event viewer on the Mac.
3. Flash `openmpg_keyboard_bench.uf2` using BOOTSEL.
4. If macOS opens Keyboard Setup Assistant, close it; no layout setup is needed.
5. Select the event viewer within eight seconds and observe the sequence above.
6. Confirm that every press is followed by a release and no key remains held.
7. Unplug the Pico after the one-time sequence.

Avoid testing in a terminal, editor containing important work, or any program
where these keystrokes could trigger an action. This image contains no
Bluetooth code and is not the Red Monkey MPG bridge firmware.

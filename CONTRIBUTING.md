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

## Contribution licensing

This project is **not** open source. It is released under the
[PolyForm Shield License 1.0.0](LICENSE), which permits use and modification
but prohibits competing with the licensor, and Red Monkey Makers distributes
commercial builds of this software.

By submitting a pull request, patch, or other contribution, you:

1. Certify it under the
   [Developer Certificate of Origin 1.1](https://developercertificate.org/) —
   that you wrote it or otherwise have the right to submit it; and
2. Grant Red Monkey Makers a perpetual, worldwide, irrevocable, royalty-free,
   non-exclusive license, with the right to sublicense, to use, reproduce,
   modify, and distribute your contribution as part of this software,
   including in commercial and closed-source releases, under the PolyForm
   Shield License or other terms the licensor selects.

Sign off every commit to certify point 1:

```sh
git commit -s -m "Your change"
```

Pull requests whose commits lack a `Signed-off-by:` trailer cannot be merged.

> This is an inbound-contribution policy, not legal advice. Have counsel review
> a formal CLA before accepting substantial external contributions.

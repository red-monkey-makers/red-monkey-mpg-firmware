# Controlled commercial release process

The release script builds the receiver twice in clean directories, compares
the UF2 files byte-for-byte, runs sanitizer-backed host tests, creates a
firmware SPDX SBOM, and writes a checksum manifest. It refuses to overwrite an
existing release directory.

The browser configurator has an independent repository, CI pipeline, version,
dependency inventory, and release process. Record the tested configurator
version in the firmware approval record; do not build one repository from an
uncontrolled sibling checkout.

It intentionally does **not** provision RP2350 OTP, generate signing keys, or
claim that signed boot is enabled. Those manufacturing controls need a separate
reviewed process with offline key custody, recovery, rotation, and anti-rollback
decisions.

## Prerequisites

- A reviewed Pico SDK/toolchain tree
- An authorized USB VID
- A qualified 8BitDo Lite 2 D-mode descriptor SHA-256
- A reviewed signed-boot manufacturing process already in effect

## Run

```bash
export PICO_SDK_PATH=/reviewed/pico-sdk
export OPENMPG_USB_VID=0x1234
export OPENMPG_USB_BCD_DEVICE=0x0100
export OPENMPG_LITE2_DESCRIPTOR_SHA256=<64-hex-qualified-hash>
export OPENMPG_SECURE_BOOT_PROVISIONED=ON
export OPENMPG_RELEASE_DATE=2026-08-10
scripts/build-commercial-release.sh 1.0.0
```

Optionally set `OPENMPG_RELEASE_SIGNING_KEY` to an existing protected private
key. The script will sign `SHA256SUMS`; it will never create or store a key.
Verify the signature independently before publication.

Archive the immutable source revision, SDK and toolchain installers, release
directory, signing verification record, hardware qualification record, and
approval record together. A generated SBOM is an inventory aid, not a license
or vulnerability clearance.

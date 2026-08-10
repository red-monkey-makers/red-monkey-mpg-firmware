# Firmware release process

## Preview releases

Preview releases are tagged evaluation builds that retain the development USB
identity and are never represented as approved commercial firmware. The
[firmware prerelease workflow](../.github/workflows/firmware-prerelease.yml)
runs only for tags beginning with `v` and requires a SemVer prerelease suffix.

Before tagging, update the `OPENMPG_RELEASE_VERSION`,
`OPENMPG_RELEASE_DATE`, and `OPENMPG_USB_BCD_DEVICE` defaults in
`CMakeLists.txt` through a reviewed pull request. The tag without its leading
`v` must exactly match the embedded version. For example:

```bash
git checkout main
git pull --ff-only
git tag -a v0.4.0-rc.1 -m "Red Monkey MPG firmware 0.4.0-rc.1"
git push origin v0.4.0-rc.1
```

The workflow checks out that exact tag, fetches the pinned Pico SDK, performs
two clean firmware builds, compares the UF2 files byte-for-byte, runs the
sanitizer-backed host tests, and generates:

- the versioned UF2 firmware image;
- `SHA256SUMS`;
- an SPDX firmware SBOM;
- tagged-source checksums; and
- release metadata containing the source and Pico SDK revisions.

It uploads the complete verification package as a short-lived workflow
artifact. The **draft GitHub prerelease** attaches only the UF2 that users copy
to the device. The workflow never publishes the release. Before manually
publishing the draft, review the generated changes and add the tested configurator version,
controller/CNC-profile compatibility, commissioning results, known limits,
and any safety-relevant changes to the release notes. Do not distribute a
workflow artifact or a draft that has not completed that review.

To stage the same preview package locally with the pinned SDK already present:

```bash
export PICO_SDK_PATH=/path/to/pinned/pico-sdk
scripts/stage-preview-release.sh 0.4.0-rc.1
```

The script refuses to overwrite an existing version directory under
`outputs/releases/`. Delete nothing merely to reuse a version; increment the
prerelease number instead. Published release tags and assets should be treated
as immutable.

## Controlled commercial releases

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

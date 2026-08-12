#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <release-version>" >&2
  exit 2
fi

release_version=$1
: "${PICO_SDK_PATH:?Set PICO_SDK_PATH to the reviewed Pico SDK tree}"
: "${RED_MONKEY_MPG_USB_VID:?Set RED_MONKEY_MPG_USB_VID to the authorized production VID}"
: "${RED_MONKEY_MPG_USB_BCD_DEVICE:?Set RED_MONKEY_MPG_USB_BCD_DEVICE to the reviewed USB revision}"
: "${RED_MONKEY_MPG_LITE2_DESCRIPTOR_SHA256:?Set the approved 64-hex descriptor SHA-256}"
: "${RED_MONKEY_MPG_SECURE_BOOT_PROVISIONED:?Set only after the reviewed signed-boot process is active}"
: "${RED_MONKEY_MPG_RELEASE_DATE:?Set RED_MONKEY_MPG_RELEASE_DATE to YYYY-MM-DD}"

if [[ ! $release_version =~ ^[0-9]+\.[0-9]+\.[0-9]+([-+][0-9A-Za-z.-]+)?$ ]]; then
  echo "release version must be SemVer-like" >&2
  exit 2
fi
if [[ ! $RED_MONKEY_MPG_LITE2_DESCRIPTOR_SHA256 =~ ^[0-9A-Fa-f]{64}$ ]]; then
  echo "RED_MONKEY_MPG_LITE2_DESCRIPTOR_SHA256 must contain exactly 64 hex characters" >&2
  exit 2
fi
if [[ $RED_MONKEY_MPG_SECURE_BOOT_PROVISIONED != ON ]]; then
  echo "RED_MONKEY_MPG_SECURE_BOOT_PROVISIONED must be ON" >&2
  exit 2
fi
if [[ ! $RED_MONKEY_MPG_RELEASE_DATE =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}$ ]]; then
  echo "RED_MONKEY_MPG_RELEASE_DATE must use YYYY-MM-DD" >&2
  exit 2
fi
if [[ ! $RED_MONKEY_MPG_USB_BCD_DEVICE =~ ^0[xX][0-9A-Fa-f]{4}$ ]]; then
  echo "RED_MONKEY_MPG_USB_BCD_DEVICE must be a four-digit hex value" >&2
  exit 2
fi

repo_root=$(cd "$(dirname "$0")/.." && pwd)
ninja_bin=${RED_MONKEY_MPG_NINJA:-$(command -v ninja || true)}
if [[ -z $ninja_bin || ! -x $ninja_bin ]]; then
  echo "ninja not found; install it or set RED_MONKEY_MPG_NINJA to its path" >&2
  exit 2
fi
release_root="$repo_root/outputs/releases/$release_version"
if [[ -e $release_root ]]; then
  echo "refusing to overwrite existing release: $release_root" >&2
  exit 3
fi
mkdir -p "$release_root"

configure_pico() {
  local build_dir=$1
  cmake -S "$repo_root" -B "$build_dir" -G Ninja \
    -DCMAKE_MAKE_PROGRAM="$ninja_bin" \
    -DPICO_SDK_PATH="$PICO_SDK_PATH" \
    -DPICO_BOARD=pico2_w \
    -DCMAKE_BUILD_TYPE=Release \
    -DRED_MONKEY_MPG_BUILD_HOST_TESTS=OFF \
    -DRED_MONKEY_MPG_BUILD_PICO=ON \
    -DRED_MONKEY_MPG_COMMERCIAL_RELEASE=ON \
    -DRED_MONKEY_MPG_SECURE_BOOT_PROVISIONED=ON \
    -DRED_MONKEY_MPG_USB_VID="$RED_MONKEY_MPG_USB_VID" \
    -DRED_MONKEY_MPG_USB_BCD_DEVICE="$RED_MONKEY_MPG_USB_BCD_DEVICE" \
    -DRED_MONKEY_MPG_RELEASE_VERSION="$release_version" \
    -DRED_MONKEY_MPG_RELEASE_DATE="$RED_MONKEY_MPG_RELEASE_DATE" \
    -DRED_MONKEY_MPG_LITE2_DESCRIPTOR_SHA256="$RED_MONKEY_MPG_LITE2_DESCRIPTOR_SHA256"
}

for pass in a b; do
  build_dir="$release_root/build-$pass"
  configure_pico "$build_dir"
  "$ninja_bin" -C "$build_dir" red_monkey_mpg_production_receiver
done
cmp "$release_root/build-a/red-monkey-mpg-gamepad-receiver.uf2" \
    "$release_root/build-b/red-monkey-mpg-gamepad-receiver.uf2"
cp "$release_root/build-a/red-monkey-mpg-gamepad-receiver.uf2" \
   "$release_root/red-monkey-mpg-gamepad-receiver-$release_version.uf2"

host_dir="$release_root/host-tests"
cmake -S "$repo_root" -B "$host_dir" -G Ninja \
  -DCMAKE_MAKE_PROGRAM="$ninja_bin" \
  -DRED_MONKEY_MPG_BUILD_PICO=OFF -DRED_MONKEY_MPG_BUILD_HOST_TESTS=ON \
  -DRED_MONKEY_MPG_ENABLE_HOST_SANITIZERS=ON -DCMAKE_BUILD_TYPE=Debug
"$ninja_bin" -C "$host_dir"
ctest --test-dir "$host_dir" --output-on-failure

node "$repo_root/scripts/generate-firmware-sbom.mjs" \
  "$PICO_SDK_PATH" "$release_root/firmware.spdx.json" "$release_version" \
  "$RED_MONKEY_MPG_RELEASE_DATE"

(
  cd "$repo_root"
  find CMakeLists.txt firmware docs scripts -type f -print0 \
    | LC_ALL=C sort -z | xargs -0 shasum -a 256
) > "$release_root/source-files.sha256"
(cd "$release_root" && shasum -a 256 \
  "red-monkey-mpg-gamepad-receiver-$release_version.uf2" \
  firmware.spdx.json source-files.sha256 > SHA256SUMS)

if [[ -n ${RED_MONKEY_MPG_RELEASE_SIGNING_KEY:-} ]]; then
  openssl dgst -sha256 -sign "$RED_MONKEY_MPG_RELEASE_SIGNING_KEY" \
    -out "$release_root/SHA256SUMS.sig" "$release_root/SHA256SUMS"
fi

echo "Release staged at $release_root"
echo "Signing is present only if RED_MONKEY_MPG_RELEASE_SIGNING_KEY was explicitly supplied."

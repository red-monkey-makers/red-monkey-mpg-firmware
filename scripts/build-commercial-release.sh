#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <release-version>" >&2
  exit 2
fi

release_version=$1
: "${PICO_SDK_PATH:?Set PICO_SDK_PATH to the reviewed Pico SDK tree}"
: "${OPENMPG_USB_VID:?Set OPENMPG_USB_VID to the authorized production VID}"
: "${OPENMPG_USB_BCD_DEVICE:?Set OPENMPG_USB_BCD_DEVICE to the reviewed USB revision}"
: "${OPENMPG_LITE2_DESCRIPTOR_SHA256:?Set the approved 64-hex descriptor SHA-256}"
: "${OPENMPG_SECURE_BOOT_PROVISIONED:?Set only after the reviewed signed-boot process is active}"
: "${OPENMPG_RELEASE_DATE:?Set OPENMPG_RELEASE_DATE to YYYY-MM-DD}"

if [[ ! $release_version =~ ^[0-9]+\.[0-9]+\.[0-9]+([-+][0-9A-Za-z.-]+)?$ ]]; then
  echo "release version must be SemVer-like" >&2
  exit 2
fi
if [[ ! $OPENMPG_LITE2_DESCRIPTOR_SHA256 =~ ^[0-9A-Fa-f]{64}$ ]]; then
  echo "OPENMPG_LITE2_DESCRIPTOR_SHA256 must contain exactly 64 hex characters" >&2
  exit 2
fi
if [[ $OPENMPG_SECURE_BOOT_PROVISIONED != ON ]]; then
  echo "OPENMPG_SECURE_BOOT_PROVISIONED must be ON" >&2
  exit 2
fi
if [[ ! $OPENMPG_RELEASE_DATE =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}$ ]]; then
  echo "OPENMPG_RELEASE_DATE must use YYYY-MM-DD" >&2
  exit 2
fi
if [[ ! $OPENMPG_USB_BCD_DEVICE =~ ^0[xX][0-9A-Fa-f]{4}$ ]]; then
  echo "OPENMPG_USB_BCD_DEVICE must be a four-digit hex value" >&2
  exit 2
fi

repo_root=$(cd "$(dirname "$0")/.." && pwd)
ninja_bin=${OPENMPG_NINJA:-/Users/kevin/.pico-sdk/ninja/v1.13.2/ninja}
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
    -DOPENMPG_BUILD_HOST_TESTS=OFF \
    -DOPENMPG_BUILD_PICO=ON \
    -DOPENMPG_COMMERCIAL_RELEASE=ON \
    -DOPENMPG_SECURE_BOOT_PROVISIONED=ON \
    -DOPENMPG_USB_VID="$OPENMPG_USB_VID" \
    -DOPENMPG_USB_BCD_DEVICE="$OPENMPG_USB_BCD_DEVICE" \
    -DOPENMPG_RELEASE_VERSION="$release_version" \
    -DOPENMPG_RELEASE_DATE="$OPENMPG_RELEASE_DATE" \
    -DOPENMPG_LITE2_DESCRIPTOR_SHA256="$OPENMPG_LITE2_DESCRIPTOR_SHA256"
}

for pass in a b; do
  build_dir="$release_root/build-$pass"
  configure_pico "$build_dir"
  "$ninja_bin" -C "$build_dir" openmpg_production_receiver
done
cmp "$release_root/build-a/openmpg_production_receiver.uf2" \
    "$release_root/build-b/openmpg_production_receiver.uf2"
cp "$release_root/build-a/openmpg_production_receiver.uf2" \
   "$release_root/red-monkey-mpg-$release_version.uf2"

host_dir="$release_root/host-tests"
cmake -S "$repo_root" -B "$host_dir" -G Ninja \
  -DCMAKE_MAKE_PROGRAM="$ninja_bin" \
  -DOPENMPG_BUILD_PICO=OFF -DOPENMPG_BUILD_HOST_TESTS=ON \
  -DOPENMPG_ENABLE_HOST_SANITIZERS=ON -DCMAKE_BUILD_TYPE=Debug
"$ninja_bin" -C "$host_dir"
ctest --test-dir "$host_dir" --output-on-failure

node "$repo_root/scripts/generate-firmware-sbom.mjs" \
  "$PICO_SDK_PATH" "$release_root/firmware.spdx.json" "$release_version" \
  "$OPENMPG_RELEASE_DATE"

(
  cd "$repo_root"
  find CMakeLists.txt firmware docs scripts -type f -print0 \
    | LC_ALL=C sort -z | xargs -0 shasum -a 256
) > "$release_root/source-files.sha256"
(cd "$release_root" && shasum -a 256 "red-monkey-mpg-$release_version.uf2" \
  firmware.spdx.json source-files.sha256 > SHA256SUMS)

if [[ -n ${OPENMPG_RELEASE_SIGNING_KEY:-} ]]; then
  openssl dgst -sha256 -sign "$OPENMPG_RELEASE_SIGNING_KEY" \
    -out "$release_root/SHA256SUMS.sig" "$release_root/SHA256SUMS"
fi

echo "Release staged at $release_root"
echo "Signing is present only if OPENMPG_RELEASE_SIGNING_KEY was explicitly supplied."

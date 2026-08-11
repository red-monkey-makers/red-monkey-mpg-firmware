#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <release-version>" >&2
  exit 2
fi

release_version=$1
: "${PICO_SDK_PATH:?Set PICO_SDK_PATH to the pinned Pico SDK tree}"

if [[ ! $release_version =~ ^[0-9]+\.[0-9]+\.[0-9]+-[0-9A-Za-z.-]+$ ]]; then
  echo "preview release version must include a SemVer prerelease suffix" >&2
  exit 2
fi

repo_root=$(cd "$(dirname "$0")/.." && pwd)
if [[ -n $(git -C "$repo_root" status --porcelain) ]]; then
  echo "refusing to stage a release from a dirty worktree" >&2
  exit 3
fi

source_revision=$(git -C "$repo_root" rev-parse HEAD)
sdk_revision=$(git -C "$PICO_SDK_PATH" rev-parse HEAD)
expected_sdk_revision=98a542c1a62fb549ffb5d66a3e5892b06276b670
if [[ $sdk_revision != "$expected_sdk_revision" ]]; then
  echo "Pico SDK revision $sdk_revision does not match pinned revision $expected_sdk_revision" >&2
  exit 3
fi

cmake_file="$repo_root/CMakeLists.txt"
configured_version=$(sed -nE \
  's/^set\(RED_MONKEY_MPG_RELEASE_VERSION "([^"]+)".*$/\1/p' "$cmake_file")
release_date=$(sed -nE \
  's/^set\(RED_MONKEY_MPG_RELEASE_DATE "([^"]+)".*$/\1/p' "$cmake_file")
usb_bcd_device=$(sed -nE \
  's/^set\(RED_MONKEY_MPG_USB_BCD_DEVICE "([^"]+)".*$/\1/p' "$cmake_file")

if [[ $configured_version != "$release_version" ]]; then
  echo "tag version $release_version does not match CMake version $configured_version" >&2
  exit 2
fi
if [[ ! $release_date =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}$ ]]; then
  echo "CMake release date must use YYYY-MM-DD" >&2
  exit 2
fi
if [[ ! $usb_bcd_device =~ ^0[xX][0-9A-Fa-f]{4}$ ]]; then
  echo "CMake USB bcdDevice must be a four-digit hex value" >&2
  exit 2
fi

ninja_bin=${RED_MONKEY_MPG_NINJA:-$(command -v ninja)}
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
    -DRED_MONKEY_MPG_COMMERCIAL_RELEASE=OFF \
    -DRED_MONKEY_MPG_RELEASE_VERSION="$release_version" \
    -DRED_MONKEY_MPG_RELEASE_DATE="$release_date" \
    -DRED_MONKEY_MPG_USB_BCD_DEVICE="$usb_bcd_device"
}

for pass in a b; do
  build_dir="$release_root/build-$pass"
  configure_pico "$build_dir"
  cmake --build "$build_dir" --target red_monkey_mpg_production_receiver --parallel
done

cmp "$release_root/build-a/red_monkey_mpg_production_receiver.uf2" \
    "$release_root/build-b/red_monkey_mpg_production_receiver.uf2"
cp "$release_root/build-a/red_monkey_mpg_production_receiver.uf2" \
   "$release_root/red-monkey-mpg-firmware-$release_version.uf2"

host_dir="$release_root/host-tests"
cmake -S "$repo_root" -B "$host_dir" -G Ninja \
  -DCMAKE_MAKE_PROGRAM="$ninja_bin" \
  -DRED_MONKEY_MPG_BUILD_PICO=OFF \
  -DRED_MONKEY_MPG_BUILD_HOST_TESTS=ON \
  -DRED_MONKEY_MPG_ENABLE_HOST_SANITIZERS=ON \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build "$host_dir" --parallel
ctest --test-dir "$host_dir" --output-on-failure

node "$repo_root/scripts/generate-firmware-sbom.mjs" \
  "$PICO_SDK_PATH" "$release_root/firmware.spdx.json" "$release_version" \
  "$release_date"

cat > "$release_root/RELEASE_METADATA.txt" <<EOF
product=Red Monkey MPG firmware
version=$release_version
release_date=$release_date
source_revision=$source_revision
pico_sdk_revision=$sdk_revision
usb_bcd_device=$usb_bcd_device
release_class=preview
commercial_release=false
EOF

(
  cd "$repo_root"
  find README.md LICENSE CMakeLists.txt cmake firmware docs scripts .github \
    -type f -print0 | LC_ALL=C sort -z | xargs -0 shasum -a 256
) > "$release_root/source-files.sha256"

(
  cd "$release_root"
  shasum -a 256 \
    "red-monkey-mpg-firmware-$release_version.uf2" \
    firmware.spdx.json RELEASE_METADATA.txt source-files.sha256 > SHA256SUMS
)

echo "Preview release staged at $release_root"
echo "This artifact uses the development USB identity and is not a commercial image."


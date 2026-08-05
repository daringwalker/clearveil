#!/usr/bin/env bash

set -euo pipefail

artifact_dir=${1:-release-artifacts}
version=${2:-$(<VERSION)}

require_one() {
  local description=$1
  shift
  local matches=()
  shopt -s nullglob
  matches=("${artifact_dir}"/$@)
  shopt -u nullglob
  if [[ ${#matches[@]} -ne 1 || ! -s "${matches[0]}" ]]; then
    echo "Expected exactly one non-empty ${description} in ${artifact_dir}." >&2
    return 1
  fi
}

require_one "source archive" "clearveil-${version}-Source.tar.gz"
require_one "Arch Linux package" "clearveil-*.pkg.tar.zst"
require_one "Debian package" "clearveil_*.deb"
require_one "RPM package" "clearveil-*.rpm"
require_one "AppImage" "Clearveil-${version}-x86_64.AppImage"
require_one "release PKGBUILD" "PKGBUILD"
require_one "checksum manifest" "SHA256SUMS"

(
  cd "${artifact_dir}"
  sha256sum --check SHA256SUMS
)

echo "Release artifacts are complete and verified: ${version}"

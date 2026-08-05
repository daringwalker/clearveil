#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 5 ]]; then
  echo "Usage: $0 SOURCE_DIR BUILD_DIR APPDIR OUTPUT_DIR VERSION" >&2
  exit 2
fi

source_dir=$1
build_dir=$2
appdir=$3
output_dir=$4
version=$5

linuxdeploy=${LINUXDEPLOY:-linuxdeploy-x86_64.AppImage}
qmake=${QMAKE:-}

if [[ -z "${qmake}" || ! -x "${qmake}" ]]; then
  echo "QMAKE must point to the Qt qmake executable." >&2
  exit 2
fi
if ! command -v "${linuxdeploy}" >/dev/null 2>&1 \
    && [[ ! -x "${linuxdeploy}" ]]; then
  echo "linuxdeploy is not available: ${linuxdeploy}" >&2
  exit 2
fi

rm -rf "${build_dir}" "${appdir}"
mkdir -p "${output_dir}"

cmake -S "${source_dir}" -B "${build_dir}" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr \
  -DBUILD_TESTING=OFF \
  -DCLEARVEIL_BUILD_BENCHMARKS=OFF
cmake --build "${build_dir}"
DESTDIR="${appdir}" cmake --install "${build_dir}"

export QMAKE="${qmake}"
export EXTRA_QT_MODULES="svg"
export EXTRA_PLATFORM_PLUGINS="libqwayland-egl.so;libqwayland-generic.so"
export APPIMAGE_EXTRACT_AND_RUN=1
export OUTPUT="${output_dir}/Clearveil-${version}-x86_64.AppImage"

"${linuxdeploy}" \
  --appdir "${appdir}" \
  --desktop-file \
    "${appdir}/usr/share/applications/io.github.daringwalker.clearveil.desktop" \
  --icon-file \
    "${appdir}/usr/share/icons/hicolor/scalable/apps/io.github.daringwalker.clearveil.svg" \
  --plugin qt \
  --output appimage

test -s "${OUTPUT}"
chmod 755 "${OUTPUT}"
QT_QPA_PLATFORM=offscreen "${OUTPUT}" \
  --appimage-extract-and-run --version

#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 SOURCE_ARCHIVE.tar.gz" >&2
    exit 2
fi

readonly archive=$1
if [[ ! -f "${archive}" ]]; then
    echo "Source archive not found: ${archive}" >&2
    exit 2
fi

readonly generated_pattern='/(build|build-[^/]+|cmake-build-[^/]+|dist|CMakeFiles|_CPack_Packages|\.flatpak-builder|\.agents|\.codex)(/|$)|/(CMakeCache\.txt|cmake_install\.cmake|compile_commands\.json)$'
readonly archive_listing=$(tar -tzf "${archive}")

if grep -E "${generated_pattern}" <<<"${archive_listing}"; then
    echo "Source archive contains generated or private local files." >&2
    exit 1
fi

for required in VERSION CMakeLists.txt LICENSE README.md src/app/main.cpp; do
    if ! grep -Eq "/${required}$" <<<"${archive_listing}"; then
        echo "Source archive is missing ${required}." >&2
        exit 1
    fi
done

echo "Source archive hygiene check passed: ${archive}"

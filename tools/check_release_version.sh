#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 vVERSION[-PRERELEASE]" >&2
    exit 2
fi

readonly tag=$1
readonly tag_version=${tag#v}
readonly project_version=$(tr -d '[:space:]' < VERSION)

if [[ -z "${project_version}" ]]; then
    echo "Could not read the CMake project version." >&2
    exit 1
fi

if [[ "${tag_version}" != "${project_version}" ]]; then
    echo "Tag ${tag} does not match CMake project version ${project_version}." >&2
    exit 1
fi

echo "Release tag ${tag} matches project version ${project_version}."

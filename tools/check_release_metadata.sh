#!/usr/bin/env bash
set -euo pipefail

readonly version=$(tr -d '[:space:]' < VERSION)
readonly tag="v${version}"
readonly pkgver=${version//-/.}

check_contains() {
    local file=$1
    local expected=$2
    if ! grep -Fq -- "${expected}" "${file}"; then
        echo "${file} is not synchronized with VERSION (${version}): ${expected}" >&2
        exit 1
    fi
}

check_contains CHANGELOG.md "## [${version}]"
check_contains packaging/clearveil.1 "Clearveil ${version}"
check_contains packaging/io.github.daringwalker.clearveil.metainfo.xml \
    "<release version=\"${version}\""
check_contains packaging/io.github.daringwalker.clearveil.yml "tag: ${tag}"
check_contains packaging/PKGBUILD "pkgver=${pkgver}"
check_contains packaging/PKGBUILD "_release_version=${version}"

echo "Release metadata matches VERSION: ${version}"

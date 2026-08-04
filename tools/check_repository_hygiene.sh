#!/usr/bin/env bash
set -euo pipefail

if [[ "$(git rev-parse --is-inside-work-tree 2>/dev/null)" != "true" ]]; then
    echo "Repository hygiene check must run inside a Git worktree." >&2
    exit 2
fi

readonly private_pattern='t530\.eki\.cc|git@[^:]+:|v[0-9]+\.[0-9]+\.[0-9]+-cs[0-9]+'

if git grep -nE "${private_pattern}" -- \
    ':!tools/check_repository_hygiene.sh'; then
    echo "Private repository references remain in tracked files." >&2
    exit 1
fi

readonly generated_pattern='(^|/)(build|build-[^/]+|cmake-build-[^/]+|dist|CMakeFiles|_CPack_Packages|\.flatpak-builder)(/|$)'

if git ls-files | grep -E "${generated_pattern}"; then
    echo "Generated build or packaging files are tracked." >&2
    exit 1
fi

echo "Repository hygiene check passed."

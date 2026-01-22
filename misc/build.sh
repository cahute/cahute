#!/bin/bash
# Build the project out of environment variables.
# This is mostly useful for the CI/CD jobs.

set -euo pipefail

CONFIGURE_OPTS=()

while IFS='\n' read -r LINE; do
    IFS='=' read -r NAME VALUE <<< "$LINE"

    if [[ "$NAME" =~ ^(CAHUTE|CMAKE|CPACK)_ ]]; then
        CONFIGURE_OPTS+=("-D$NAME=$VALUE")
    fi
done < <(env -0 | sort)

if [[ "${#CONFIGURE_OPTS[@]}" -eq 0 ]]; then
    echo "No configure options passed." >&2
else
    echo "Configure options:" >&2
    for OPT in "${CONFIGURE_OPTS[@]}"; do
        echo "  $OPT"
    done
fi

cmake -S . -B "build" "${CONFIGURE_OPTS[@]}"
cmake --build "build"

#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "$0")"
if [[ ! -x build/triewise ]]; then bash build.sh; fi
exec ./build/triewise "$@"

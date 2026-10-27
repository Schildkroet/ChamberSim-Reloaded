#!/usr/bin/env bash
# Configure, build and test with all cores.   usage: tools/build.sh [debug|release] [extra cmake --build args]
set -euo pipefail
cd "$(dirname "$0")/.."
jobs="$(nproc 2>/dev/null || sysctl -n hw.ncpu)"   # nproc is missing on macOS
preset="${1:-debug}"; [ $# -gt 0 ] && shift
cmake --preset "$preset"
cmake --build --preset "$preset" --parallel "$jobs" "$@"
ctest --preset "$preset" -j "$jobs"

#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Configure only if build system isn't generated yet
if [[ ! -f "$ROOT/build/build.ninja" && ! -f "$ROOT/build/Makefile" ]]; then
  cmake -S "$ROOT/src" -B "$ROOT/build" -G Ninja
fi

cmake --build "$ROOT/build"


#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

cmake -S "$ROOT/src" -B "$ROOT/build" -G Ninja
cmake --build "$ROOT/build"


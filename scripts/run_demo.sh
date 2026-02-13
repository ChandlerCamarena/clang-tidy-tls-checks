#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

clang-tidy -p "$ROOT/demo" \
  -load "$ROOT/build/TlsTidyModule.so" \
  -checks='-*,tls-*' \
  "$ROOT/demo/bad/disable_verify.c" \
  "$ROOT/demo/good/verify_peer.c"


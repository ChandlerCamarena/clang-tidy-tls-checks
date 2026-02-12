#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

clang-tidy -p "$ROOT/demo" \
  "$ROOT/demo/bad/disable_verify.c" \
  "$ROOT/demo/bad/insecure_method.c" \
  "$ROOT/demo/good/verify_peer.c" \
  "$ROOT/demo/good/tls_method.c"


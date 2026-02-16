#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Ensure plugin exists
"$ROOT/scripts/build.sh"

# Run ONLY our checks, quietly (no extra tidy noise)
clang-tidy -quiet -p "$ROOT/demo" \
  -load "$ROOT/build/TlsTidyModule.so" \
  -checks='-*,tls-*' \
  "$ROOT/demo/bad/disable_verify.c" \
  "$ROOT/demo/good/verify_peer.c" \
  "$ROOT/demo/bad/insecure_method.c" \
  "$ROOT/demo/good/tls_method.c" \
  "$ROOT/demo/bad/disable_verify_ssl.c" \
  "$ROOT/demo/good/verify_peer_ssl.c" \
  "$ROOT/demo/bad/insecure_client_method.c" \
  "$ROOT/demo/good/tls_client_method.c"



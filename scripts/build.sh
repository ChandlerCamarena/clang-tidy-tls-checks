#!/usr/bin/env bash
# build.sh — builds TlsTidyModule.so
# Works on Arch Linux (LLVM 21 via pacman) and Ubuntu (LLVM 18 via apt).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "=== Building thesis checker plugin ==="
echo "    LLVM:  $(llvm-config --version)"
echo "    Clang: $(clang --version | head -1)"

if [[ ! -f "$ROOT/build/build.ninja" && ! -f "$ROOT/build/Makefile" ]]; then
  echo "--- Configuring with CMake ---"
  cmake -S "$ROOT/src" -B "$ROOT/build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
fi

echo "--- Building ---"
cmake --build "$ROOT/build"

echo ""
echo "=== Build complete ==="
echo "    Plugin: $ROOT/build/TlsTidyModule.so"
echo ""
echo "To verify the plugin loads:"
echo "    clang-tidy -load $ROOT/build/TlsTidyModule.so -list-checks 2>/dev/null | grep -E 'tls-|security-misc'"

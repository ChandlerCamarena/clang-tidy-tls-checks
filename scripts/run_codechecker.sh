#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# run_codechecker.sh
#
# Runs the thesis padding-leak checker over a project via CodeChecker and
# collects the raw event log for Table 8.1 / 8.2 metric collection.
#
# Usage:
#   ./scripts/run_codechecker.sh [PROJECT_DIR] [COMPILE_COMMANDS]
#
# Examples:
#   ./scripts/run_codechecker.sh /path/to/zlib /path/to/zlib/build/compile_commands.json
#   ./scripts/run_codechecker.sh  # uses demo/ as a quick smoke test
#
# Environment variables:
#   PLUGIN_PATH  — override path to TlsTidyModule.so (default: build/TlsTidyModule.so)
#   CC_RESULTS   — output directory for CodeChecker results (default: ./cc-results)
#   EVENT_LOG    — path for the CSV event log (default: ./events.csv)
# ---------------------------------------------------------------------------
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# ---------------------------------------------------------------------------
# Config
# ---------------------------------------------------------------------------
PLUGIN_PATH="${PLUGIN_PATH:-$ROOT/build/TlsTidyModule.so}"
CC_RESULTS="${CC_RESULTS:-$ROOT/cc-results}"
EVENT_LOG="${EVENT_LOG:-$ROOT/events.csv}"
PROJECT_DIR="${1:-$ROOT/demo}"
COMPILE_COMMANDS="${2:-$ROOT/demo/compile_commands.json}"

# ---------------------------------------------------------------------------
# Validate
# ---------------------------------------------------------------------------
if [[ ! -f "$PLUGIN_PATH" ]]; then
  echo "[ERROR] Plugin not found at: $PLUGIN_PATH"
  echo "        Run:  cmake -S src -B build -G Ninja && ninja -C build"
  exit 1
fi

if [[ ! -f "$COMPILE_COMMANDS" ]]; then
  echo "[ERROR] compile_commands.json not found at: $COMPILE_COMMANDS"
  echo "        Generate with:  cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..."
  exit 1
fi

command -v CodeChecker >/dev/null 2>&1 || {
  echo "[ERROR] CodeChecker not found in PATH."
  echo "        Install: pip install codechecker"
  echo "        Or activate your venv: source ~/codechecker-env/bin/activate"
  exit 1
}

# ---------------------------------------------------------------------------
# Initialise event log with header (append mode for multi-project runs)
# ---------------------------------------------------------------------------
if [[ ! -f "$EVENT_LOG" ]]; then
  echo "boundary_fn,event_kind,type_name,has_padding,pad_bytes,init_class,evidence_level,diag_emitted" \
    > "$EVENT_LOG"
fi
export PADDING_LEAK_LOG="$EVENT_LOG"

# ---------------------------------------------------------------------------
# Baseline timing
# ---------------------------------------------------------------------------
echo "=== [1/3] Baseline analysis (no custom checker) ==="
T0=$(date +%s%N)
CodeChecker analyze "$COMPILE_COMMANDS" \
  --analyzers clang-tidy \
  --tidyargs "-checks=-*,clang-diagnostic-*" \
  --output "$CC_RESULTS/baseline" \
  --jobs 4 \
  --quiet 2>/dev/null || true
T1=$(date +%s%N)
BASELINE_MS=$(( (T1 - T0) / 1000000 ))
echo "    Baseline: ${BASELINE_MS} ms"

# ---------------------------------------------------------------------------
# Checker analysis
# ---------------------------------------------------------------------------
echo "=== [2/3] Analysis with padding-leak checker ==="
T0=$(date +%s%N)
CodeChecker analyze "$COMPILE_COMMANDS" \
  --analyzers clang-tidy \
  --tidyargs "-load $PLUGIN_PATH -checks=-*,security-misc-padding-boundary-leak,tls-cert-verify-disabled,tls-insecure-proto-method" \
  --output "$CC_RESULTS/with_checker" \
  --jobs 4
T1=$(date +%s%N)
CHECKER_MS=$(( (T1 - T0) / 1000000 ))
echo "    With checker: ${CHECKER_MS} ms"

OVERHEAD_PCT="N/A"
if [[ $BASELINE_MS -gt 0 ]]; then
  OVERHEAD_PCT=$(python3 -c "print(f'{100*($CHECKER_MS-$BASELINE_MS)/$BASELINE_MS:.1f}%')")
fi
echo "    Overhead: ${OVERHEAD_PCT}"

# ---------------------------------------------------------------------------
# Parse results
# ---------------------------------------------------------------------------
echo "=== [3/3] Parsing results ==="
mkdir -p "$CC_RESULTS/json"
CodeChecker parse "$CC_RESULTS/with_checker" \
  --export json \
  --output "$CC_RESULTS/json/results.json" || true

CodeChecker parse "$CC_RESULTS/with_checker" || true

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
echo ""
echo "=== Run complete ==="
echo "  Baseline:     ${BASELINE_MS} ms"
echo "  With checker: ${CHECKER_MS} ms"
echo "  Overhead:     ${OVERHEAD_PCT}"
echo "  Event log:    $EVENT_LOG"
echo "  CC results:   $CC_RESULTS/with_checker"
echo "  JSON export:  $CC_RESULTS/json/results.json"
echo ""
echo "  Next: python3 scripts/collect_metrics.py --log $EVENT_LOG --cc $CC_RESULTS/json/results.json"

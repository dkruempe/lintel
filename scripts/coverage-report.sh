#!/usr/bin/env bash
#
# coverage-report.sh -- turn the gcov data of a coverage build into an lcov
# tracefile and an html report.
#
# Prerequisite: configure and build with -DENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
# and run the tests, so the .gcda files exist next to the object files in the
# build directory. The CI job `coverage` does exactly that; locally:
#
#   cmake -S . -B build/build/Debug ... -DENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
#   cmake --build build/build/Debug --parallel "$(nproc)"
#   (cd build/build/Debug && ctest --output-on-failure --label-regex unit)
#   ./scripts/coverage-report.sh
#
# Usage: ./scripts/coverage-report.sh [build-dir] [output-dir]
#   build-dir   default: <repo>/build/build/Debug
#   output-dir  default: <repo>/coverage

set -euo pipefail

PROJ_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${1:-$PROJ_ROOT/build/build/Debug}"
OUTPUT_DIR="${2:-$PROJ_ROOT/coverage}"

if ! command -v lcov >/dev/null 2>&1; then
  echo "lcov not found - install it first (e.g. ./scripts/install-deps.sh lcov)" >&2
  exit 1
fi
if [[ ! -d "$BUILD_DIR" ]]; then
  echo "build directory not found: $BUILD_DIR" >&2
  exit 1
fi

GCOV_TOOL="${GCOV:-gcov}"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "$WORK_DIR"' EXIT

# --capture walks the build tree for .gcno/.gcda and calls gcov. --ignore-errors
# keeps lcov going on the "no data"/"mismatch" errors that a partial build (some
# translation units not compiled or not run) produces.
lcov --capture \
  --directory "$BUILD_DIR" \
  --gcov-tool "$GCOV_TOOL" \
  --ignore-errors mismatch,empty,gcov,source \
  --output-file "$WORK_DIR/coverage.raw.info"

# Only the library sources are interesting: system headers, the Conan cache,
# the tests, examples and benchmarks would drown the report in noise.
lcov --remove "$WORK_DIR/coverage.raw.info" \
  '/usr/*' \
  '*/conan2/*' \
  '*/_deps/*' \
  "$PROJ_ROOT/tests/*" \
  "$PROJ_ROOT/examples/*" \
  "$PROJ_ROOT/benchmarks/*" \
  --ignore-errors unused,empty \
  --output-file "$WORK_DIR/coverage.info"

rm -rf "$OUTPUT_DIR"
genhtml "$WORK_DIR/coverage.info" \
  --output-directory "$OUTPUT_DIR" \
  --ignore-errors source \
  --title "lintel coverage" >/dev/null

summary="$(lcov --summary "$WORK_DIR/coverage.info" 2>&1 || true)"
echo "$summary"
echo "html report: $OUTPUT_DIR/index.html"

# Make the numbers visible in the GitHub Actions run summary when available.
if [[ -n "${GITHUB_STEP_SUMMARY:-}" ]]; then
  {
    echo '## coverage'
    echo '```'
    echo "$summary"
    echo '```'
  } >> "$GITHUB_STEP_SUMMARY"
fi

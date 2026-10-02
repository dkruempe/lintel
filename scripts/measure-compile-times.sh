#!/usr/bin/env bash
# Reproducible compile-time measurement for cpp-base-library.
# See ROADMAP.md section 4 (P2: "Mess-Skripte versionieren") and
# docs/compile-time-measurements.md for the reference protocol.
#
# Usage:
#   scripts/measure-compile-times.sh [--build-dir build/build/Release] [--jobs 1] [--nice] [--cold] [--incremental]
#
# Reads the same layout as CI/AGENTS.md. Does NOT touch the source tree.
# Output: wall-clock time of the configure+build phase plus ccache statistics,
# so results are reproducible and can be diffed across commits.
#
# Defaults to --jobs 1 (single-core measurements, preferred on machines that
# also run unrelated services); add --nice to lower the build priority further.
# Load averages are recorded around the build for context. For CI-numbers use
# --jobs <cpu count> without --nice (see docs/compile-time-measurements.md).
# --cold and --incremental are combinable: one run yields both reference values.
set -euo pipefail

BUILD_DIR="build/build/Release"
JOBS="${JOBS:-1}"
NICE=""
MODE="cold"   # cold | warm (cold wipes ccache first)
INCREMENTAL=0 # additionally touch one header and measure the rebuild delta
CONFIG_DIR="${BUILD_DIR%/Release}/Release"
CONAN_PRESENT=0
command -v conan >/dev/null 2>&1 && CONAN_PRESENT=1

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir) BUILD_DIR="$2"; shift 2 ;;
        --jobs) JOBS="$2"; shift 2 ;;
        --nice) NICE="nice -n 10"; shift ;;
        --cold) MODE="cold"; shift ;;
        --incremental) INCREMENTAL=1; shift ;;
        *) echo "unknown option: $1" >&2; exit 2 ;;
    esac
done

# 1. Fresh configure so the build starts from a known state
rm -rf "$BUILD_DIR" "$CONFIG_DIR/generators"

# 2. Restore/refetch Conan deps (fast if cached) into the fresh layout
if [[ "$CONAN_PRESENT" == "1" ]]; then
    conan install . --output-folder=build --build=missing -s build_type=Release \
        -c tools.cmake.cmaketoolchain:generator=Ninja
fi

cmake -S . -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$CONFIG_DIR/generators/conan_toolchain.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -G Ninja

# 3. Cold: wipe ccache so the measurement reflects a from-scratch compile
if [[ "$MODE" == "cold" ]]; then
    ccache -C >/dev/null 2>&1 || true
fi
ccache -z >/dev/null 2>&1 || true

# 4. Time the build (single-core preferred; track host load for context)
load() { awk '{print $1" "$2" "$3}' /proc/loadavg; }
echo "== build: mode=$MODE jobs=$JOBS build_dir=$BUILD_DIR =="
echo "== host load before: $(load) (5min 10min 15min) =="
START=$(date +%s.%N)
$NICE cmake --build "$BUILD_DIR" --parallel "$JOBS"
END=$(date +%s.%N)
echo "== host load after:  $(load) (5min 10min 15min) =="
echo "== BUILD WALL-CLOCK TIME: $(echo "$END $START" | awk '{printf "%.2f", $1 - $2}') s =="

# 5. ccache statistics (cacheability is a core compile-time lever)
ccache -s

# 6. Optional incremental step: touch one header, rebuild, and report the delta
if [[ "$INCREMENTAL" == "1" ]]; then
    touch src/include/base_library/core/utils/StringUtils.h
    echo "== incremental rebuild after touching StringUtils.h =="
    echo "== host load before: $(load) (5min 10min 15min) =="
    START=$(date +%s.%N)
    $NICE cmake --build "$BUILD_DIR" --parallel "$JOBS"
    END=$(date +%s.%N)
    echo "== host load after:  $(load) (5min 10min 15min) =="
    echo "== INCREMENTAL WALL-CLOCK TIME: $(echo "$END $START" | awk '{printf "%.2f", $1 - $2}') s =="
fi

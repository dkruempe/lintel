#!/usr/bin/env bash
#
# clang-tidy for CI and local use: analyses only the files changed by the current
# branch instead of the whole project (a full run is hours of work and the
# existing `.clang-tidy` config with `Checks: "*"` is not actionable as a gate).
#
# Scope: changed C/C++ files, production code only (`^tests/` and `^external/` are
# skipped by default — unchecked optionals and mock expectations are idiomatic
# there, and `external/` is vendored).
#
# Requirements: a CMake configure that produced a *non-unity*, *PCH-free*
# compile_commands.json, because clang-tidy cannot map a file to its command line
# inside a unity TU, and a GCC-built PCH cannot be read by Clang:
#
#   conan install . --output-folder=build --build=missing -s build_type=Release \
#     -c tools.cmake.cmaketoolchain:generator=Ninja
#   cmake -S . -B build/tidy -G Ninja \
#     -DCMAKE_TOOLCHAIN_FILE=build/build/Release/generators/conan_toolchain.cmake \
#     -DCMAKE_BUILD_TYPE=Release \
#     -DENABLE_UNITY_BUILD=OFF -DENABLE_PCH=OFF
#
# Usage:
#   scripts/check-tidy.sh [BASE_REF] [BUILD_DIR]
#
# Environment:
#   CLANG_TIDY         clang-tidy binary to use (auto-detected otherwise)
#   TIDY_CHECKS        checks to enable, default correctness/bug-prone families
#   TIDY_MAX_FILES     upper bound of files per run, default 20 (each TU costs 15-30 s)
#   TIDY_EXCLUDE       regex of paths to skip, default '^external/|^tests/|^benchmarks/'
#   SUMMARY_FILE       if set, the report is appended to that file (GitHub step summary)
set -uo pipefail

cd "$(git rev-parse --show-toplevel)" || exit 1

BASE="${1:-}"
BUILD_DIR="${2:-build/tidy}"
COMPILE_COMMANDS="$BUILD_DIR/compile_commands.json"
EXCLUDE_RE="${TIDY_EXCLUDE:-^external/|^tests/|^benchmarks/}"
MAX_FILES="${TIDY_MAX_FILES:-20}"
CHECKS="${TIDY_CHECKS:--*,clang-analyzer-*,bugprone-*,cert-*,performance-*,portability-*}"

find_clang_tidy() {
    if [[ -n "${CLANG_TIDY:-}" ]]; then
        printf '%s' "$CLANG_TIDY"
        return
    fi
    local candidate
    for candidate in clang-tidy-19 clang-tidy-18 clang-tidy-17 clang-tidy; do
        if command -v "$candidate" >/dev/null 2>&1; then
            printf '%s' "$candidate"
            return
        fi
    done
    return 1
}

TIDY_BIN="$(find_clang_tidy)" || {
    echo "check-tidy: no clang-tidy binary found" >&2
    exit 2
}

report() {
    printf '%s\n' "$*"
    if [[ -n "${SUMMARY_FILE:-}" ]]; then
        printf '%s\n' "$*" >>"$SUMMARY_FILE"
    fi
}

if [[ ! -f "$COMPILE_COMMANDS" ]]; then
    report "**error:** $COMPILE_COMMANDS not found — see the header of this script for the required CMake configure."
    exit 2
fi

if [[ -z "$BASE" ]]; then
    BASE="$(git rev-parse --verify --quiet HEAD~1 || true)"
fi
if [[ -z "$BASE" || "$BASE" =~ ^0+$ ]] || ! git rev-parse --verify --quiet "$BASE^{commit}" >/dev/null; then
    report "**skip:** no usable BASE_REF"
    exit 0
fi

mapfile -t candidates < <(
    {
        git diff --name-only --diff-filter=ACMR "$BASE"
        git ls-files --others --exclude-standard
    } | sort -u
)

selected=()
skipped_excluded=0
for file in "${candidates[@]}"; do
    case "$file" in
        *.cpp | *.cc | *.cxx) ;;
        *.h | *.hpp | *.hxx) selected+=("$file"); continue ;;
        *) continue ;;
    esac
    if [[ "$file" =~ $EXCLUDE_RE ]]; then
        skipped_excluded=$((skipped_excluded + 1))
        continue
    fi
    selected+=("$file")
done

report "### clang-tidy ($("$TIDY_BIN" --version | head -1))"
report ""
report "base: \`$BASE\` · compile_commands: \`$COMPILE_COMMANDS\` · checks: \`$CHECKS\`"

if [[ "${#selected[@]}" -eq 0 ]]; then
    report ""
    report "**pass:** no analysable C++ files changed (excluded: $skipped_excluded)"
    exit 0
fi

truncated=0
if [[ "${#selected[@]}" -gt "$MAX_FILES" ]]; then
    truncated=$((${#selected[@]} - MAX_FILES))
    selected=("${selected[@]:0:MAX_FILES}")
fi

report "changed files: ${#selected[@]} analysed (excluded: $skipped_excluded, not analysed: $truncated)"
report ""

findings=0
for file in "${selected[@]}"; do
    output="$("$TIDY_BIN" -p "$BUILD_DIR" --quiet \
        --extra-arg=-Wno-unknown-warning-option \
        --checks="$CHECKS" "$file" 2>&1 |
        grep -E 'warning:|error:' |
        grep -v 'clang-diagnostic-' || true)"
    if [[ -n "$output" ]]; then
        findings=$((findings + $(grep -c 'warning:\|error:' <<<"$output")))
        report "<details><summary>$file</summary>"
        report ""
        report '```'
        report "$output"
        report '```'
        report ""
        report "</details>"
        report ""
    fi
done

report "**$findings finding(s)** in ${#selected[@]} analysed file(s)"
[[ "$truncated" -gt 0 ]] && report "" && report "_$truncated further changed file(s) not analysed (TIDY_MAX_FILES)_"

[[ "$findings" -gt 0 ]] && exit 1
exit 0
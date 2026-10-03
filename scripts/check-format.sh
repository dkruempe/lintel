#!/usr/bin/env bash
#
# clang-format check for CI and local use.
#
# The repository is not formatted end-to-end yet, so a repo-wide
# `clang-format --dry-run --Werror` would fail on ~90% of all files. This script
# therefore enforces a *regression* rule instead:
#
#   For every C/C++ file touched by the current branch, compare the number of
#   clang-format violations against the same file at BASE_REF. More violations
#   than before == the change made the formatting worse == failure.
#
# That gives new/changed code real teeth without forcing a 500-file reformat
# into an unrelated change.
#
# Usage:
#   scripts/check-format.sh [BASE_REF]        # regression check on changed files (CI default)
#   scripts/check-format.sh --all             # repo-wide violation report (advisory, exit 0)
#   scripts/check-format.sh --all --strict    # repo-wide check, exit 1 on any violation
#
# Environment:
#   CLANG_FORMAT       clang-format binary to use (auto-detected otherwise)
#   FORMAT_EXCLUDE     regex of paths to skip, default '^external/' (vendored code)
#   SUMMARY_FILE       if set, the report is appended to that file (GitHub step summary)
set -uo pipefail

cd "$(git rev-parse --show-toplevel)" || exit 1

EXCLUDE_RE="${FORMAT_EXCLUDE:-^external/}"
STRICT=0
BASE=""
for arg in "$@"; do
    case "$arg" in
        --all) BASE="__ALL__" ;;
        --strict) STRICT=1 ;;
        -h | --help)
            sed -n '2,25p' "$0"
            exit 0
            ;;
        *) BASE="$arg" ;;
    esac
done

find_clang_format() {
    if [[ -n "${CLANG_FORMAT:-}" ]]; then
        printf '%s' "$CLANG_FORMAT"
        return
    fi
    local candidate
    for candidate in clang-format-19 clang-format-18 clang-format-17 clang-format; do
        if command -v "$candidate" >/dev/null 2>&1; then
            printf '%s' "$candidate"
            return
        fi
    done
    return 1
}

FORMAT_BIN="$(find_clang_format)" || {
    echo "check-format: no clang-format binary found" >&2
    exit 2
}

report() {
    printf '%s\n' "$*"
    if [[ -n "${SUMMARY_FILE:-}" ]]; then
        printf '%s\n' "$*" >>"$SUMMARY_FILE"
    fi
}

violation_count_stdin() {
    # Reads a C++ file on stdin, prints the number of clang-format violations.
    "$FORMAT_BIN" --dry-run --assume-filename="$1" - 2>&1 >/dev/null |
        grep -c 'code should be clang-formatted'
}

violation_count_file() {
    # Prints the number of violations of a file in the working tree.
    "$FORMAT_BIN" --dry-run "$1" 2>&1 >/dev/null |
        grep -c 'code should be clang-formatted'
}

violation_count_for_ref() {
    # Prints the number of violations the given file has in BASE_REF (0 if absent).
    if git cat-file -e "$BASE:$1" 2>/dev/null; then
        git show "$BASE:$1" | violation_count_stdin "$1"
    else
        printf '0'
    fi
}

violation_report() {
    # Prints the line numbers of the violations in the working-tree file.
    "$FORMAT_BIN" --dry-run "$1" 2>&1 >/dev/null |
        grep 'code should be clang-formatted' |
        sed -E 's/^[^:]+:([0-9]+):[0-9]+:.*/\1/' |
        head -20 |
        paste -sd, -
}

tracked_cpp_files() {
    git ls-files -z '*.cpp' '*.cc' '*.cxx' '*.h' '*.hpp' '*.hxx' | tr '\0' '\n'
}

report "### clang-format ($("$FORMAT_BIN" --version))"

if [[ "$BASE" == "__ALL__" ]]; then
    total_files=0
    dirty_files=0
    total_hunks=0
    top=""
    while IFS= read -r file; do
        [[ -n "$file" ]] || continue
        if [[ "$file" =~ $EXCLUDE_RE ]]; then
            continue
        fi
        total_files=$((total_files + 1))
        hunks=$(violation_count_file "$file")
        if [[ "$hunks" -gt 0 ]]; then
            dirty_files=$((dirty_files + 1))
            total_hunks=$((total_hunks + hunks))
            top+="$hunks $file"$'\n'
        fi
    done < <(tracked_cpp_files)

    report ""
    report "| metric | value |"
    report "|---|---|"
    report "| checked files | $total_files |"
    report "| files with violations | $dirty_files |"
    report "| total violations | $total_hunks |"
    report ""
    report "<details><summary>10 files with the most violations</summary>"
    report ""
    report '```'
    printf '%s' "$top" | sort -rn | head -10 | sed 's/^/  /'
    report '```'
    report ""
    report "</details>"

    if [[ "$STRICT" -eq 1 && "$total_hunks" -gt 0 ]]; then
        report "**fail:** $total_hunks violations in $dirty_files of $total_files files"
        exit 1
    fi
    report "advisory only (use --strict to enforce): $total_hunks violations remain"
    exit 0
fi

if [[ -z "$BASE" ]]; then
    BASE="$(git rev-parse --verify --quiet HEAD~1 || true)"
fi
if [[ -z "$BASE" || "$BASE" =~ ^0+$ ]] || ! git rev-parse --verify --quiet "$BASE^{commit}" >/dev/null; then
    report "**skip:** no usable BASE_REF, cannot compare formatting (pass a commit or branch)"
    exit 0
fi

# BASE..working tree (plus untracked files), so the same command also works
# locally with uncommitted changes
mapfile -t changed < <(
    {
        git diff --name-only --diff-filter=ACMR "$BASE"
        git ls-files --others --exclude-standard
    } | sort -u
)
report ""
report "base: \`$BASE\` · changed C/C++ files: ${#changed[@]}"

checked=0
regressions=0
for file in "${changed[@]}"; do
    case "$file" in
        *.cpp | *.cc | *.cxx | *.h | *.hpp | *.hxx) ;;
        *) continue ;;
    esac
    if [[ "$file" =~ $EXCLUDE_RE ]]; then
        report "- skipped (vendored/excluded): \`$file\`"
        continue
    fi
    [[ -f "$file" ]] || continue

    checked=$((checked + 1))
    before=$(violation_count_for_ref "$file")
    after=$(violation_count_file "$file")
    if [[ "$after" -gt "$before" ]]; then
        regressions=$((regressions + 1))
        report "- **$file**: $before → $after violations (lines $(violation_report "$file"))"
    else
        report "- $file: $before → $after violations"
    fi
done

report ""
if [[ "$regressions" -gt 0 ]]; then
    report "**fail:** formatting got worse in $regressions of $checked checked files"
    report ""
    report 'Fix with:'
    report '```'
    report 'clang-format -i <file>'
    report '```'
    exit 1
fi

report "**pass:** no formatting regression in $checked changed C/C++ files"
exit 0
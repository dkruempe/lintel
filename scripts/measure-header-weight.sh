#!/usr/bin/env bash
# Single-core weight probe for public headers of cpp-base-library.
# Mirrors how a LOG_*-consumer TU is compiled and reports the preprocessed
# size and a -fsyntax-only compile time of a probe TU. Single-threaded by
# design so numbers are reproducible and the machine stays responsive.
#
# Usage:
#   scripts/measure-header-weight.sh [--build-dir build/build/Release] [--header LoggerService.h]
#
# Flags (-I/-isystem/-D/-std) are extracted from the given build dir's
# compile_commands.json. Unity builds have no per-TU LoggerService.cpp entry,
# so the library TU (base_library.dir) with the widest include set is used as
# template. Nothing in the source tree is modified.
set -euo pipefail

BUILD_DIR="build/build/Release"
TARGET="LoggerService.h"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir) BUILD_DIR="$2"; shift 2 ;;
        --header) TARGET="$2"; shift 2 ;;
        *) echo "unknown option: $1" >&2; exit 2 ;;
    esac
done

COMPILE_DB="$BUILD_DIR/compile_commands.json"
if [[ ! -f "$COMPILE_DB" ]]; then
    echo "no compile_commands.json at $COMPILE_DB (run cmake with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON)" >&2
    exit 1
fi

PROJ_ROOT="$(git rev-parse --show-toplevel)"
PROBE="$(mktemp /tmp/opencode/header_weight_probe.XXXXXX.cc)"
PREDIR="$(dirname "$PROBE")"
cat > "$PROBE" <<'EOF'
#include "base_library/core/services/LoggerService.h"
void log_probe()
{
  int id = 1;
  LOG_INFO("probe {}/{}", id, 2);
  LOG_ERROR("probe error: {}", 3);
}
EOF

# Extract compiler flags (drop -o/-c/ccache/-include PCH from the template cmd).
FLAGS=$(
  python3 - "$COMPILE_DB" "$PREDIR" "$PROJ_ROOT" <<'PY'
import json, sys
db, predir, proj_root = json.load(open(sys.argv[1])), sys.argv[2], sys.argv[3]
cmd = ""
for e in db:
    if e["file"].endswith("LoggerService.cpp"):
        cmd = e["command"]
        break
if not cmd:
    best, best_n = "", -1
    for e in db:
        n = e["command"].count("-isystem")
        if "base_library.dir" in e["file"] and n > best_n:
            best, best_n = e["file"], n
    if best:
        cmd = next(e["command"] for e in db if e["file"] == best)
if not cmd and db:
    cmd = db[0]["command"]
parts = cmd.split()
out = []
skip = False
i = 0
while i < len(parts):
    p = parts[i]
    if skip:
        skip = False
        i += 1
        continue
    if p == "ccache":
        i += 1
        continue
    if p == "-include":
        skip = True
        i += 1
        continue
    if p.startswith("-o"):
        i += 1
        continue
    if p == "-c":
        i += 1
        continue
    if p.endswith((".cpp", ".cxx", ".c", ".cc")):
        i += 1
        continue
    if p == "-isystem":
        out.append(p)
        out.append(parts[i + 1])
        i += 2
        continue
    if p.startswith(("-I", "-D", "-std", "-pthread")):
        out.append(p)
    i += 1
# always add the source include root on top of the library include set
out.append("-I" + predir)
out.append("-isystem")
out.append(proj_root + "/src/include")
print(" ".join(out))
PY
)

echo "== header-weight: $TARGET (single core, -fsyntax-only) =="
echo "-- preprocessed size --"
g++ $FLAGS -E "$PROBE" > "${PROBE%.cc}.i"
wc -l -c "${PROBE%.cc}.i"
echo "-- -fsyntax-only compile time --"
/usr/bin/time -f "%e s wall" g++ $FLAGS -fsyntax-only -ftime-report "$PROBE" 2>&1 | grep -E "TOTAL" || true
rm -f "$PROBE" "${PROBE%.cc}.i"

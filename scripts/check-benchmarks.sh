#!/usr/bin/env bash
#
# check-benchmarks.sh -- compare a benchmark run against the recorded baseline
# and report the entries whose latency grew beyond a threshold.
#
# `ROADMAP.md` P2 asks the advisory `benchmarks` CI job to compare against a
# fixed baseline instead of only uploading numbers. The absolute numbers are
# dominated by the machine, so the comparison uses a *relative fingerprint*:
# every entry is divided by the geometric mean of the run, so a machine-wide
# speed factor (a slower CPU, a busier runner) cancels out and only benchmarks
# that moved relative to the rest of the run are reported.
#
#   ./scripts/check-benchmarks.sh [options] <results.json>
#
# Options:
#   --baseline <file>   baseline to compare with
#                       (default: benchmarks/baseline.json)
#   --threshold <pct>   report an entry as a regression when its normalized
#                       latency grew by more than this many percent (default: 10)
#   --update-baseline   write <results.json> to the baseline and exit
#   --quiet             only report problems
#
# The primary metric is real time, not CPU time: the TLS delayed-ack stall that
# `ROADMAP.md` P6 fixed cost 40 ms of real time per request while the CPU time
# stayed flat, so a CPU-time comparison would have missed it.
#
# Exit code: 0 when nothing crossed the threshold, 1 otherwise (the caller
# decides whether that blocks; the CI job runs it advisory).

set -euo pipefail

PROJ_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BASELINE="$PROJ_ROOT/benchmarks/baseline.json"
THRESHOLD=10
UPDATE=0
QUIET=0
RESULTS=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --baseline)
            BASELINE="$2"
            shift 2
            ;;
        --threshold)
            THRESHOLD="$2"
            shift 2
            ;;
        --update-baseline)
            UPDATE=1
            shift
            ;;
        --quiet)
            QUIET=1
            shift
            ;;
        -*)
            echo "unknown option: $1" >&2
            exit 2
            ;;
        *)
            RESULTS="$1"
            shift
            ;;
    esac
done

if [[ -z "$RESULTS" ]]; then
    echo "usage: $0 [--baseline <file>] [--threshold <pct>] [--update-baseline] <results.json>" >&2
    exit 2
fi

python3 - "$RESULTS" "$BASELINE" "$THRESHOLD" "$UPDATE" "$QUIET" <<'PY'
import json
import math
import sys
from datetime import datetime, timezone

results_path, baseline_path, threshold_text, update_text, quiet_text = sys.argv[1:6]
threshold = float(threshold_text)
update = update_text == "1"
quiet = quiet_text == "1"


def fail(message):
    sys.exit(message)


def read_json(path, required):
    try:
        with open(path, encoding="utf-8") as handle:
            return json.load(handle)
    except FileNotFoundError:
        if required:
            fail(f"{path} not found")
        return None
    except json.JSONDecodeError as error:
        fail(f"{path} is not valid JSON: {error}")


# google/benchmark reports each entry with unit that makes the number readable
# ("ns", "us", "ms", "s"); convert everything to nanoseconds so a run that
# formats a benchmark differently cannot look like a 1000x regression.
UNIT_TO_NS = {"ns": 1.0, "us": 1e3, "ms": 1e6, "s": 1e9}


def collect(path):
    """@return {name: (real_time_ns, cpu_time_ns)} for one benchmark run."""
    document = read_json(path, required=True)
    benchmarks = document.get("benchmarks")
    if not isinstance(benchmarks, list):
        fail(f"{path} has no 'benchmarks' list (run with --benchmark_out=...)")

    # With --benchmark_repetitions the file holds one entry per repetition plus
    # an aggregate; the median aggregate is the stable number to compare. An
    # aggregate is named "<benchmark>_median", so key on `run_name`.
    aggregates = {}
    iterations = {}
    for entry in benchmarks:
        if not isinstance(entry, dict) or "name" not in entry:
            continue
        if entry.get("skipped"):
            continue
        factor = UNIT_TO_NS.get(entry.get("time_unit", "ns"))
        if factor is None:
            continue
        real = entry.get("real_time")
        cpu = entry.get("cpu_time")
        if real is None:
            continue
        name = entry.get("run_name") or entry["name"]
        value = (real * factor, (cpu * factor) if cpu is not None else None)
        if entry.get("run_type") == "aggregate":
            if entry.get("aggregate_name") == "median":
                aggregates[name] = value
        elif entry.get("run_type") in (None, "iteration"):
            iterations[name] = value
    # an aggregate wins; a benchmark that ran without repetitions falls back
    merged = dict(iterations)
    merged.update(aggregates)
    return merged


current = collect(results_path)
if not current:
    fail(f"{results_path} contains no benchmark results")

if update:
    if not current:
        fail("refusing to write an empty baseline")
    baseline = {
        "format": "lintel-benchmark-baseline/1",
        "recorded_on": datetime.now(timezone.utc).strftime("%Y-%m-%d"),
        "note": (
            "Recorded reference for scripts/check-benchmarks.sh. The comparison uses the "
            "geometric-mean-normalized index, so the machine-wide speed factor cancels out; "
            "refresh with './scripts/check-benchmarks.sh --update-baseline <results.json>'."
        ),
        "unit": "ns",
        "real_time": {name: value[0] for name, value in sorted(current.items())},
        "cpu_time": {name: value[1] for name, value in sorted(current.items()) if value[1] is not None},
    }
    with open(baseline_path, "w", encoding="utf-8") as handle:
        json.dump(baseline, handle, indent=2, sort_keys=False)
        handle.write("\n")
    print(f"baseline written: {baseline_path} ({len(baseline['real_time'])} benchmarks)")
    sys.exit(0)

baseline_document = read_json(baseline_path, required=True)
baseline = baseline_document.get("real_time")
if not isinstance(baseline, dict):
    fail(f"{baseline_path} has no 'real_time' mapping")

shared = sorted(set(current) & set(baseline))
if not shared:
    fail("no benchmark name is present in both the run and the baseline")

# The fingerprint: normalize each median by the geometric mean of the run, so
# the comparison is "how did this entry move relative to the whole suite".
def index(values):
    mean = math.exp(sum(math.log(value) for value in values) / len(values))
    return mean


current_index = index([current[name][0] for name in shared])
baseline_index = index([baseline[name] for name in shared])

regressions = []
improvements = []
rows = []
for name in shared:
    baseline_time = float(baseline[name])
    current_time = current[name][0]
    raw_ratio = current_time / baseline_time
    normalized = raw_ratio * (baseline_index / current_index)
    change = (normalized - 1.0) * 100.0
    rows.append((change, name, baseline_time, current_time, raw_ratio, normalized))
    if change > threshold:
        regressions.append((change, name))
    elif change < -threshold:
        improvements.append((change, name))

rows.sort(reverse=True)

if not quiet:
    print(f"\nbenchmark baseline: {baseline_path}")
    print(f"normalized index: baseline {baseline_index:.0f} ns, run {current_index:.0f} ns")
    print(f"threshold: {threshold:.0f}% (real time, geometric-mean normalized)\n")
    print(f"{'benchmark':<48} {'baseline':>12} {'run':>12} {'raw':>8} {'normalized':>11}")
    print("-" * 96)
    for change, name, baseline_time, current_time, raw_ratio, normalized in rows:
        flag = ""
        if change > threshold:
            flag = "  REGRESSION"
        elif change < -threshold:
            flag = "  improved"
        print(f"{name:<48} {baseline_time:>10.0f}ns {current_time:>10.0f}ns "
              f"{raw_ratio:>7.2f}x {normalized:>10.2f}x{flag}")
    print("")

if regressions:
    print(f"**warning:** {len(regressions)} benchmark(s) slower than the baseline by more than "
          f"{threshold:.0f}% (normalized):")
    for change, name in sorted(regressions, reverse=True):
        print(f"  - {name}: {change:+.1f}%")
    print("\nA real regression, or a busy runner - re-run and refresh the baseline with "
          "--update-baseline if the change is intended.")
    sys.exit(1)

print(f"**pass:** no benchmark exceeds the {threshold:.0f}% threshold"
      f" ({len(improvements)} improved, {len(shared)} compared)")
PY

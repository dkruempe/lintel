#!/usr/bin/env bash
#
# Installs the system packages the build needs, robustly.
#
# Every workflow needs a package set, and every workflow used to run a bare
# `sudo apt-get update && sudo apt-get install -y ...`. That form hangs: on the
# hosted runners apt regularly stalls on the archive mirror (azure.archive
# .ubuntu.com / packages.microsoft.com) or on the dpkg lock left behind by the
# image's unattended-upgrades, and a plain hang eats the whole job timeout.
#
# Observed on 2026-10-07: one runner got azure.archive.ubuntu.com at ~12 kB/s and
# spent 16m45s on 14.3 MB, while the jobs next to it fetched the same bytes in
# 1 s. A job timeout cannot fix that - only tolerating the slow transfer (or
# skipping it) can.
#
# This script therefore:
#   - skips packages that are already installed (a no-op apt call costs a
#     mirror round-trip and can hang even when it has nothing to do),
#   - retries apt, which resumes from /var/cache/apt/archives/partial instead of
#     re-downloading from zero,
#   - lets apt detect a stalled mirror itself (Acquire::*::Timeout) so the
#     per-call cap only ever fires on a slow-but-alive transfer,
#   - waits for / kills stale dpkg and unattended-upgrades locks,
#   - runs fully non-interactive (--force-confdef/--force-confold), so a
#     maintainer prompt cannot stall the step,
#   - verifies afterwards that every requested package is really installed, so
#     a silently skipped package fails here instead of 20 minutes later inside
#     the Conan build.
#
# Usage:
#   scripts/install-deps.sh pkg1 pkg2 ...          # install the given packages
#   APT_UPDATE=0 scripts/install-deps.sh pkg ...   # skip `apt-get update`
#
# Environment:
#   APT_UPDATE      1 (default) run `apt-get update` first, 0 to skip
#   APT_RETRIES     attempts per apt call, default 3
#   APT_TIMEOUT     seconds per apt call, default 1200. Generous on purpose:
#                   runners repeatedly get a throttled mirror node where
#                   azure.archive.ubuntu.com delivers ~12 kB/s (measured: 11.2 MB
#                   of cmake in 15m30s, while other jobs got the same bytes in
#                   1 s). Killing such a transfer and restarting it is strictly
#                   worse than waiting - genuinely stalled connections are
#                   caught far earlier by Acquire::*::Timeout below.
#   APT_LOCK_TIMEOUT seconds to wait for the dpkg lock, default 60
#   APT_BUDGET      seconds for the whole script, default 1800. Retries stop when
#                   it is exceeded, so a hopeless mirror fails instead of
#                   consuming the job timeout.
#   APT_NO_SUDO      set to 1 to run apt without sudo (already root)
set -uo pipefail

APT_UPDATE="${APT_UPDATE:-1}"
APT_RETRIES="${APT_RETRIES:-3}"
APT_TIMEOUT="${APT_TIMEOUT:-1200}"
APT_LOCK_TIMEOUT="${APT_LOCK_TIMEOUT:-60}"
APT_BUDGET="${APT_BUDGET:-1800}"
SCRIPT_START=$SECONDS

# Array, not a string: an empty "sudo" string would still occupy a word.
SUDO=()
if [ "${APT_NO_SUDO:-0}" != "1" ] && [ "$(id -u)" -ne 0 ]; then
    SUDO=(sudo)
fi

if [ "$#" -eq 0 ]; then
    echo "install-deps: no packages given" >&2
    exit 2
fi

# dpkg prompts are the classic way an apt step dies silently on a runner: there
# is no tty, so the question is either auto-answered wrongly or blocks.
export DEBIAN_FRONTEND=noninteractive

APT_OPTS=(
    -o Dpkg::Options::=--force-confdef
    -o Dpkg::Options::=--force-confold
    -o Dpkg::Use-Pty=0
    -o Acquire::Retries=3
    -o Acquire::http::Timeout=30
    -o Acquire::https::Timeout=30
)

log() { printf 'install-deps: %s\n' "$*"; }

# Runs a command under `timeout`, as the last line of defence against something
# that never returns (a dpkg prompt, an unreachable mirror). The value is set
# high enough not to interrupt a merely slow transfer.
run_bounded() {
    local seconds="$1"
    shift
    timeout --signal=INT --kill-after=15s "$seconds" "$@"
}

# Waits until no process holds the dpkg/apt locks. unattended-upgrades runs on
# the hosted images and can hold the frontend lock for a while; if it is stuck
# it is killed, because its work is irrelevant for a CI runner.
wait_for_locks() {
    local deadline=$((SECONDS + APT_LOCK_TIMEOUT))
    while [ "$SECONDS" -lt "$deadline" ]; do
        if ! "${SUDO[@]}" fuser /var/lib/dpkg/lock-frontend \
            /var/lib/dpkg/lock /var/lib/apt/lists/lock \
            /var/cache/apt/archives/lock >/dev/null 2>&1; then
            return 0
        fi
        log "waiting for the dpkg/apt lock (${APT_LOCK_TIMEOUT}s budget)"
        sleep 5
    done

    log "dpkg lock still held after ${APT_LOCK_TIMEOUT}s, stopping the holder"
    "${SUDO[@]}" pkill -f 'unattended-upgrade' >/dev/null 2>&1
    "${SUDO[@]}" pkill -x apt-get >/dev/null 2>&1
    "${SUDO[@]}" pkill -x dpkg >/dev/null 2>&1
    sleep 3

    if "${SUDO[@]}" fuser /var/lib/dpkg/lock-frontend >/dev/null 2>&1; then
        log "could not free the dpkg lock"
        return 1
    fi
    return 0
}

# Runs one apt invocation with retries. $1 is a label, the rest the apt command.
apt_retry() {
    local label="$1"
    shift
    local attempt=1

    while [ "$attempt" -le "$APT_RETRIES" ]; do
        if [ $((SECONDS - SCRIPT_START)) -ge "$APT_BUDGET" ]; then
            echo "install-deps: budget of ${APT_BUDGET}s exhausted, giving up on $label" >&2
            return 1
        fi

        if wait_for_locks; then
            log "$label (attempt $attempt/$APT_RETRIES)"
            if run_bounded "$APT_TIMEOUT" "${SUDO[@]}" apt-get "${APT_OPTS[@]}" "$@"; then
                return 0
            fi
            log "$label failed"
        else
            log "$label: dpkg lock unavailable, retrying"
        fi

        # A half-finished install leaves dpkg in a state where the next call
        # fails for an unrelated-looking reason. Repair before retrying.
        run_bounded 120 "${SUDO[@]}" dpkg --configure -a >/dev/null 2>&1
        attempt=$((attempt + 1))
        if [ "$attempt" -le "$APT_RETRIES" ]; then
            sleep $((attempt * 5))
        fi
    done

    echo "install-deps: $label failed after $APT_RETRIES attempts" >&2
    return 1
}

# Prints the subset of "$@" that is not installed yet. Skipping these is what
# keeps the mirror out of the loop on the (common) case where the runner image
# already ships the toolchain.
collect_missing() {
    local -n _target="$1"
    shift
    _target=()
    local pkg
    for pkg in "$@"; do
        if ! dpkg-query -W -f='${db:Status-Status}' "$pkg" 2>/dev/null | grep -qx 'installed'; then
            _target+=("$pkg")
        fi
    done
}

collect_missing MISSING "$@"

# Before, not after `apt-get update`: the runner images already ship most of the
# toolchain, and an update that has nothing to install is a pure mirror round
# trip that can hang for no benefit.
if [ "${#MISSING[@]}" -eq 0 ]; then
    log "all requested packages already installed"
    exit 0
fi

if [ "$APT_UPDATE" = "1" ]; then
    apt_retry "apt-get update" update || exit 1

    # Re-check: a package could have been installed while we were waiting.
    collect_missing MISSING "${MISSING[@]}"
    if [ "${#MISSING[@]}" -eq 0 ]; then
        log "all requested packages already installed"
        exit 0
    fi
fi

log "installing: ${MISSING[*]}"
# No --no-install-recommends: the working runs installed recommends (cmake pulls
# gcc/make, python3-pip pulls python3-dev), and silently dropping them is a
# behaviour change that only shows up as a missing header much later.
apt_retry "apt-get install" install -y "${MISSING[@]}" || exit 1

# Verification: apt can exit 0 with a package left unconfigured, and the next
# step (Conan/CMake) would only fail with a confusing "library not found".
STILL_MISSING=()
collect_missing STILL_MISSING "${MISSING[@]}"

if [ "${#STILL_MISSING[@]}" -ne 0 ]; then
    echo "install-deps: still not installed after apt succeeded: ${STILL_MISSING[*]}" >&2
    exit 1
fi

log "done"
exit 0

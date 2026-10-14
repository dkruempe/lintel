#!/usr/bin/env bash
#
# Run a lintel example locally with the configuration from the repository's
# .env file (git-ignored). Every assignment in .env is exported before the
# example binary is started.
#
# Usage:
#   scripts/run-example.sh [example] [args...]
#
#   scripts/run-example.sh                 # run bin/main (default)
#   scripts/run-example.sh worker          # run bin/worker with the worker config
#   scripts/run-example.sh main --help     # pass arguments through to the example
#
# Environment:
#   ENV_FILE   path to the env file (default: <repo>/.env, may be absent)
#
# The full-stack examples (main, worker) use HttpFeature and need the
# self-signed development certificates; they are generated automatically on
# first use. Note that main starts the interactive CLI, so run it from a
# terminal (stdin must stay open).
set -euo pipefail

REPO_ROOT="$(git -C "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)" rev-parse --show-toplevel)"
ENV_FILE="${ENV_FILE:-$REPO_ROOT/.env}"

if [[ -f "$ENV_FILE" ]]; then
  set -a
  # shellcheck disable=SC1090
  source "$ENV_FILE"
  set +a
else
  echo "note: $ENV_FILE not found - using built-in defaults" >&2
fi

EXAMPLE="${1:-main}"
[[ $# -gt 0 ]] && shift

# A manually started worker must not spawn further workers, so it defaults to
# the DB-free worker config unless .env overrides it.
if [[ "$EXAMPLE" == "worker" ]]; then
  : "${BOOTSTRAP_CONFIG_NAME:=bootstrap_worker}"
else
  : "${BOOTSTRAP_CONFIG_NAME:=bootstrap}"
fi
: "${CONFIG_DIRECTORY:=$REPO_ROOT/cfg}"
export CONFIG_DIRECTORY BOOTSTRAP_CONFIG_NAME

BIN="$REPO_ROOT/bin/$EXAMPLE"
if [[ ! -x "$BIN" ]]; then
  echo "error: $BIN not found or not executable - build first (see AGENTS.md)" >&2
  exit 1
fi

case "$EXAMPLE" in
  main | worker)
    if [[ ! -f "$CONFIG_DIRECTORY/certs/server.crt" || ! -f "$CONFIG_DIRECTORY/certs/server.key" ]]; then
      echo "generating self-signed development certificates in $CONFIG_DIRECTORY/certs" >&2
      "$REPO_ROOT/cfg/certs/generate_certs.sh"
    fi
    ;;
esac

echo "starting $BIN (CONFIG_DIRECTORY=$CONFIG_DIRECTORY, BOOTSTRAP_CONFIG_NAME=$BOOTSTRAP_CONFIG_NAME)" >&2
exec "$BIN" "$@"

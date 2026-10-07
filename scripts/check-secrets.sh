#!/usr/bin/env bash
#
# Secret guard: blocks plaintext credentials in the working tree.
#
# Why this exists: `secret_scanning_push_protection` cannot be enabled for this
# repo via API (all fields in `security_and_analysis` are rejected with 422,
# even with admin rights - verified 06.10.2026). Until this has been added via
# the UI, this script takes over the check.
#
# IMPORTANT - demarcation. A secret detector that only looks at the name
# produces useless noise: in this repo there are dozens of places like
# `const char *const PASSWORD = "password";` - those are JSON key names, not
# credentials. A first draft flagged exactly those and was useless.
#
# Therefore two stages, both of which must be satisfied:
#
#   1. Context: only config formats (.xml, .yml, .yaml, .env, .properties,
#      .ini, .conf) and PEM key blocks. C++ files are checked for key
#      material, not for attribute values.
#   2. Entropy: the value must look like a credential - at least
#      ZEICHEN_MIN characters, at least one digit, and at least one additional
#      character class (mixed upper/lower case OR special characters). This
#      reliably filters out key names ("password", "old_password"), german
#      words and empty values.
#
# Accepted placeholders are: ${...}, REDACTED/PLACEHOLDER/CHANGEME/YOUR_,
# values in angle brackets, REPLACE_WITH_*, the seed placeholder and empty values.
#
# Usage:
#   scripts/check-secrets.sh              # working tree (versioned + untracked)
#   scripts/check-secrets.sh --staged     # only the ones staged for the commit
#   scripts/check-secrets.sh --all        # only versioned files
#
# Return value: 0 = clean, 1 = finding, 2 = invocation error
set -uo pipefail

cd "$(git rev-parse --show-toplevel)" || exit 2

MODE="${1:-}"
case "$MODE" in
  --staged) mapfile -t FILES < <(git diff --cached --name-only --diff-filter=ACM) ;;
  --all)    mapfile -t FILES < <(git ls-files) ;;
  "")       mapfile -t FILES < <(git ls-files --cached --others --exclude-standard) ;;
  *)        echo "Usage: $0 [--staged|--all]" >&2; exit 2 ;;
esac

[ "${#FILES[@]}" -gt 0 ] || { echo "no files to check"; exit 0; }

ZEICHEN_MIN=8

# Excluded from the check. Every exclusion is commented - the
# SKIP_DIRS_pruefen does not cover that, it is a deliberate exception.
skip_path() {
  case "$1" in
    .git/*|build/*|bin/*)        return 0 ;;  # build artifacts
    external/*)                   return 0 ;;  # vendored third-party code, no credential
    cfg/certs/*)                  return 0 ;;  # dev certificates, gitignored
    cfg/database/*)               return 0 ;;  # seeds: the password is a documented
                                             # placeholder REPLACE_WITH_..., see the
                                             # comment in the SQL. The corresponding
                                             # seed history is a separate item.
    *)                            return 1 ;;
  esac
}

is_placeholder() {
  case "$1" in
    '${'*|*'{'*|*'$('*)            return 0 ;;
    'REDACTED'*|'redacted'*)      return 0 ;;
    *PLACEHOLDER*|*placeholder*)  return 0 ;;
    CHANGEME*|'YOUR_'*|*YOUR_*)    return 0 ;;
    '<'*'>'*)                     return 0 ;;
    REPLACE_WITH_*)               return 0 ;;
    ''|'""'|"''")                 return 0 ;;
  esac
  return 1
}

# Entropy criterion: length, digit, and additional character class.
looks_like_credential() {
  local v="$1"
  [ "${#v}" -ge "$ZEICHEN_MIN" ] || return 1
  [[ "$v" =~ [0-9] ]]               || return 1
  [[ "$v" =~ [A-Z] && "$v" =~ [a-z] ]] && return 0
  [[ "$v" =~ [^A-Za-z0-9] ]]        && return 0
  return 1
}

is_config_file() {
  case "$1" in
    *.xml|*.yml|*.yaml|*.env|*.properties|*.ini|*.conf|*.toml) return 0 ;;
  esac
  return 1
}

found=0

echo "### Secret guard (mode: ${MODE:-working tree})"
echo "Files: ${#FILES[@]}"

for f in "${FILES[@]}"; do
  skip_path "$f" && continue
  [ -f "$f" ] || continue

  # 1) Key material: applies to all file types
  if grep -qE "^-----BEGIN [A-Z ]*PRIVATE KEY-----" "$f" 2>/dev/null; then
    found=1
    echo "  FINDING  $f: PEM private key in the working tree"
  fi

  # 2) Attribute values: only in config formats
  is_config_file "$f" || continue
  while IFS= read -r line; do
    value="${line#*=}"
    value="${value%\"}"; value="${value#\"}"
    value="${value%\'}"; value="${value#\'}"
    is_placeholder "$value" && continue
    looks_like_credential "$value" || continue
    found=1
    echo "  FINDING  $f: possible plaintext credential"
    printf '        %s\n' "${line:0:110}"
  done < <(grep -nEi "(password|passwd|secret|api[_-]?key)[[:space:]]*[:=][[:space:]]*['\"]?[A-Za-z0-9!@#$%^&*_+.-]{8,}" "$f" 2>/dev/null)
done

echo
if [ "$found" -ne 0 ]; then
  cat <<'MSG'
**fail:** possible plaintext credential found.

What to do:
  1. Remove the value and replace it with a placeholder (${...}, REDACTED, <...>).
  2. If the value is real: rotate it immediately - from now on it counts as compromised.
  3. Remove it from the git history (git filter-repo). Rotation is the more
     important step here: a rewrite only deletes the trace, not the access.
  4. Exceptions belong in skip_path() commented with a justification, not silently.
MSG
  exit 1
fi

echo "**pass:** no plaintext credentials found"

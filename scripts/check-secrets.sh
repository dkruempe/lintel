#!/usr/bin/env bash
#
# Secret-Guard: blockiert Klartext-Credentials im Arbeitsbaum.
#
# Warum es das gibt: `secret_scanning_push_protection` lässt sich für dieses Repo
# per API nicht aktivieren (alle Felder in `security_and_analysis` werden mit
# 422 abgelehnt, auch bei Admin-Rechten - verifiziert 06.10.2026). Bis das über
# die UI nachgezogen ist, übernimmt dieses Skript die Kontrolle.
#
# WICHTIG - Abgrenzung. Ein Secret-Detektor, der nur auf den Namen schaut,
# produziert unbrauchbaren Lärm: In diesem Repo gibt es Dutzende Stellen wie
# `const char *const PASSWORD = "password";` - das sind JSON-Schlüsselnamen, keine
# Zugangsdaten. Ein erster Entwurf flaggte genau die und war unbrauchbar.
#
# Deshalb zwei Stufen, die beide erfüllt sein müssen:
#
#   1. Kontext: nur Konfigurationsformate (.xml, .yml, .yaml, .env, .properties,
#      .ini, .conf) und PEM-Schlüsselblöcke. C++-Dateien werden auf Schlüssel-
#      material geprüft, nicht auf Attributwerte.
#   2. Entropie: der Wert muss wie ein Credential aussehen - mindestens
#      ZEICHEN_MIN Zeichen, mindestens eine Ziffer, und mindestens eine zusätzliche
#      Zeichenklasse (Groß-/Kleinbuchstaben gemischt ODER Sonderzeichen). Das
#      filtert Schlüsselnamen ("password", "old_password"), deutsche Wörter und
#      leere Werte zuverlässig weg.
#
# Als Platzhalter gelten: ${...}, REDACTED/PLACEHOLDER/CHANGEME/YOUR_,
# Werte in spitzen Klammern, REPLACE_WITH_*, der Seed-Platzhalter und leere Werte.
#
# Aufruf:
#   scripts/check-secrets.sh              # Arbeitsbaum (versioniert + untracked)
#   scripts/check-secrets.sh --staged     # nur die für den Commit vorgemerkten
#   scripts/check-secrets.sh --all        # nur versionierte Dateien
#
# Rückgabewert: 0 = sauber, 1 = Fund, 2 = Aufruffehler
set -uo pipefail

cd "$(git rev-parse --show-toplevel)" || exit 2

MODE="${1:-}"
case "$MODE" in
  --staged) mapfile -t FILES < <(git diff --cached --name-only --diff-filter=ACM) ;;
  --all)    mapfile -t FILES < <(git ls-files) ;;
  "")       mapfile -t FILES < <(git ls-files --cached --others --exclude-standard) ;;
  *)        echo "Aufruf: $0 [--staged|--all]" >&2; exit 2 ;;
esac

[ "${#FILES[@]}" -gt 0 ] || { echo "keine Dateien zu prüfen"; exit 0; }

ZEICHEN_MIN=8

# Von der Prüfung ausgenommen. Jeder Ausschluss ist kommentiert - die
# SKIP_DIRS_pruefen sichert das nicht ab, das ist eine bewusste Ausnahme.
skip_path() {
  case "$1" in
    .git/*|build/*|bin/*)        return 0 ;;  # Build-Artefakte
    external/*)                   return 0 ;;  # vendorter Fremdcode, kein Zugangsdatum
    cfg/certs/*)                  return 0 ;;  # dev-Zertifikate, .gitignoriert
    cfg/database/*)               return 0 ;;  # Seeds: Passwort ist dokumentierter
                                             # Platzhalter REPLACE_WITH_..., siehe
                                             # Kommentar im SQL. Die zugehoerige
                                             # Seed-Historie ist ein eigener Punkt.
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

# Entropie-Kriterium: laenge, Ziffer, und zusaetzliche Zeichenklasse.
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

echo "### Secret-Guard (Modus: ${MODE:-Arbeitsbaum})"
echo "Dateien: ${#FILES[@]}"

for f in "${FILES[@]}"; do
  skip_path "$f" && continue
  [ -f "$f" ] || continue

  # 1) Schlüsselmaterial: gilt für alle Dateitypen
  if grep -qE "^-----BEGIN [A-Z ]*PRIVATE KEY-----" "$f" 2>/dev/null; then
    found=1
    echo "  FUND  $f: PEM-Private-Key im Arbeitsbaum"
  fi

  # 2) Attributwerte: nur in Konfigurationsformaten
  is_config_file "$f" || continue
  while IFS= read -r line; do
    value="${line#*=}"
    value="${value%\"}"; value="${value#\"}"
    value="${value%\'}"; value="${value#\'}"
    is_placeholder "$value" && continue
    looks_like_credential "$value" || continue
    found=1
    echo "  FUND  $f: möglicher Klartext-Credential"
    printf '        %s\n' "${line:0:110}"
  done < <(grep -nEi "(password|passwd|secret|api[_-]?key)[[:space:]]*[:=][[:space:]]*['\"]?[A-Za-z0-9!@#$%^&*_+.-]{8,}" "$f" 2>/dev/null)
done

echo
if [ "$found" -ne 0 ]; then
  cat <<'MSG'
**fail:** möglicher Klartext-Credential-Fund.

Was zu tun ist:
  1. Wert entfernen und durch einen Platzhalter ersetzen (${...}, REDACTED, <...>).
  2. Falls der Wert echt ist: sofort rotieren - er gilt ab jetzt als kompromittiert.
  3. Aus der Git-Historie entfernen (git filter-repo). Die Rotation ist dabei der
     wichtigere Schritt: ein Rewrite löscht nur die Spur, nicht das Zugangsdienst.
  4. Ausnahmen gehören in skip_path() mit Begründung kommentiert, nicht stillschweigend.
MSG
  exit 1
fi

echo "**pass:** keine Klartext-Credentials gefunden"

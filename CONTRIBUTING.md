# CONTRIBUTING.md

## Einleitung

Vielen Dank für dein Interesse an `cpp-base-library`! Wir akzeptieren Beiträge in Form von Bugreports, Pull Requests, Dokumentationsverbesserungen oder Vorschlägen.

## Voraussetzungen

- C++17-kompatibler Compiler (CI testet GCC 13 und Clang 18)
- CMake 3.16+
- Conan 2
- Ninja
- Optional: ccache, Docker (für PostgreSQL-Tests), PostgreSQL (lokal)

## Entwicklungsworkflow

1. Fork oder Clone des Repos (`https://github.com/dkruempe/cpp-base-library`)
2. Lokales Build- und Testsetup gemäß Build-Kommandos
3. Änderungen in einem Feature-Branch vornehmen
4. Format und Lint lokal prüfen
5. Tests ausführen
6. Pull Request gegen `master` stellen

## Build- und Test-Kommandos

### Build (Release, wie CI)

```bash
# 1. Conan-Abhängigkeiten
conan install . --output-folder=build --build=missing -s build_type=Release -c tools.cmake.cmaketoolchain:generator=Ninja

# 2. CMake konfigurieren
cmake -S . -B build/build/Release \
  -DCMAKE_TOOLCHAIN_FILE=build/build/Release/generators/conan_toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -G Ninja

# 3. Build
cmake --build build/build/Release --parallel $(nproc)
```

Binärdateien landen in `bin/`.

### Tests

```bash
cd build/build/Release && ctest --output-on-failure --label-regex unit
```

Der einzelne Test-Binary ist `base_tests` (Catch2 + trompeloeil). PostgreSQL-abhängige Tests benötigen eine laufende Postgres-Instanz, z. B.:

```bash
docker compose up postgres
```

(Datenbank/Benutzer/Passwort: `test`/`test`/`test`, Port `5432`)

## Format und Lint

- Formatprüfung (blockierend in CI): `./scripts/check-format.sh master` (prüft nur geänderte Dateien gegen den Base-Commit). Repo-weiter Drift: `./scripts/check-format.sh --all` (informativ, mit `--strict` blockierend).
- clang-tidy (derzeit advisory in CI): `./scripts/check-tidy.sh master build/tidy` (benötigt ein Non-Unity-/PCH-freies `compile_commands.json`).

Formatierung mit `clang-format -i` auf geänderten Dateien durchführen. CI erwartet keine neuen `clang-format`-Verstöße in geänderten Dateien.

## Code-Ergänzungen

- **Neue Testdateien:** Müssen explizit in `tests/CMakeLists.txt` zur Liste des `ADD_EXECUTABLE(base_tests ...)` hinzugefügt werden (keine automatische Erkennung).
- **Neue Quelldateien:** Müssen explizit in `src/CMakeLists.txt` zu `headers` bzw. `sources` hinzugefügt werden (keine Glob-Muster).
- **Voraussetzungen beim Bearbeiten:** `config.h` wird von CMake aus `config.h.in` generiert; `-Werror` ist standardmäßig aktiv (`myproject_WARNINGS_AS_ERRORS`). Alle Warnungen müssen behoben werden.
- **Unity-Builds:** Standardmäßig aktiviert – kann Include-Reihenfolge maskieren; bei Verdacht auf unvollständige Includes lohnt ein Build mit deaktiviertem Unity-Build.
- **TLS-Zertifikate:** Niemals private Schlüssel committen. Dev-Zertifikate liegen nicht im Repo; lokal mit `./cfg/certs/generate_certs.sh` erzeugen (siehe `cfg/certs/README.md`).

## Commit-Konvention

Der reale Commit-Stil im Repo ist deutsch, kurz und imperativ (z. B. `docs: README an CI angleichen, Badges und Zertifikats-Hinweise`, `Build-Fixes fuer GCC 13: ...`). Verwende eine ähnliche Form: Präfix sinnvoll wählen (`docs:`, `ci:`, `fix:`, `feat:`, `refactor:`, ...), nur eine logische Änderung pro Commit, keine Secrets committen.

## Weitere Hinweise

- Arbeitsstand und offene Punkte werden nicht im Repository geführt; `TODO.md`, `ANALYSIS.md` und `ROADMAP.md` wurden entfernt (Begründung siehe `CHANGELOG.md`, Abschnitt „Entfernt").
- Build-Layout folgt CI (`build/build/Release`), README weist auf diesen Pfad hin.
- Vor dem Erstellen eines Pull Requests: Status prüfen, Format-Gate lokal bestehen, relevante Unit-Tests ausführen.

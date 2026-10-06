# AGENTS.md

## Build & Test Commands

CI is the source of truth (`.github/workflows/ci.yml`). C++17, CMake 3.16+, Conan 2, Ninja.

```bash
# 1. Conan dependencies
conan install . --output-folder=build --build=missing -s build_type=Release -c tools.cmake.cmaketoolchain:generator=Ninja

# 2. CMake configure
cmake -S . -B build/build/Release \
  -DCMAKE_TOOLCHAIN_FILE=build/build/Release/generators/conan_toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -G Ninja

# 3. Build
cmake --build build/build/Release --parallel $(nproc)

# 4. Run tests (single binary, Catch2 + trompeloeil)
cd build/build/Release && ctest --output-on-failure --label-regex unit
```

Binaries output to `bin/`.

## CI (`.github/workflows/`)

| Workflow | Trigger | Inhalt |
|----------|---------|--------|
| `ci.yml` → `build` | push/PR auf `master`, `workflow_dispatch` | Matrix `gcc-release` (Gate), `clang-release`, `gcc-debug-nonunity` (Debug + Non-Unity in einem Lauf) |
| `ci.yml` → `format` | dito | **blockierend**: geänderte Dateien dürfen keine neuen `clang-format`-Verstöße enthalten; zusätzlich repo-weiter Drift-Report (nur informativ) |
| `ci.yml` → `lint` | dito | `clang-tidy` auf geänderten Dateien (`src/`, ohne `tests/`/`external/`), aktuell `continue-on-error` |
| `release.yml` | Tag `v*`, `workflow_dispatch` (dry-run) | Version prüfen → Release-Build → `ctest` → Tarball mit `bin/`, `cfg/` (ohne `certs/`), `swagger.yaml` → `gh release create` |

Lokal dasselbe prüfen:

```bash
./scripts/check-format.sh master   # Format-Regression auf geänderten Dateien
./scripts/check-format.sh --all    # repo-weiter Drift-Report (Blockierend: --strict)
./scripts/check-tidy.sh master build/tidy   # braucht Non-Unity-/PCH-freies compile_commands.json
./scripts/check-secrets.sh         # Klartext-Credentials (--staged / --all)
```

## Secrets

`secret_scanning_push_protection` lässt sich für dieses Repo **nicht per API aktivieren** –
alle Felder in `security_and_analysis` werden mit `422` ablehnt, auch mit Admin-Rechten
(verifiziert 06.10.2026). Solange das nicht über *Settings → Code security* nachgezogen ist,
übernimmt `scripts/check-secrets.sh` (blockierender CI-Job `secrets`).

Das Skript prüft **nicht** gegen eine Liste bekannter Geheimnisse, sondern in zwei Stufen:
Konfigurationsformate (`.xml`/`.yml`/`.env`/…) bzw. PEM-Schlüsselblöcke, **und** einen
Entropie-Test auf den Wert. reine Namenssuche ist unbrauchbar – das Repo hat Dutzende
`const char *const PASSWORD = "password";`, das sind JSON-Schlüsselnamen.

Keine Zugangsdaten committen. Passwörter in Config und Beispielen als `${VAR}` oder
`<PLACEHOLDER>` schreiben. `cfg/database/*` und `cfg/certs/*` sind bewusst ausgenommen
(dokumentierter Seed-Platzhalter bzw. gitignorierte Dev-Zertifikate).

## Code Style

- **Formatting**: `clang-format` — 120-col limit, 2-space indent, custom brace wrapping. Run `clang-format -i` on changed files. CI (`format`-Job) vergleicht Verstöße gegen den Base-Commit: geänderte Dateien dürfen **keine neuen** Verstöße enthalten (Legacy-Verstöße sind toleriert).
- **Warnings**: `-Werror` is ON by default (`myproject_WARNINGS_AS_ERRORS`). Fix all warnings before committing.
- **clang-tidy**: Off by default. Enable with `-DMYPROJECT_ENABLE_CLANG_TIDY=ON`. Header filter: `.*base_library/.*`.
- **Unity builds**: ON by default (`ENABLE_UNITY_BUILD`). Can mask include-order bugs — disable to test `#include` completeness.

## Architecture

`base_library` is a CMake library (`kruempelmann::base_library`).

- **`src/core/`** — StartupBuilder, DI wiring, persistence abstraction (PostgreSQL + SQLite), logging, services, plugins.
- **`src/features/`** — Optional modules: `http/` (cpp-httplib server/client), `cli/`, `property/` (XML/file/DB/SHM config), `base/` (auth, processes, message queues, event bus, shared memory).
- **`src/include/base_library/`** — Public headers. `config.h` is generated from `config.h.in` by CMake.
- **`external/Hypodermic/`** — Vendored DI framework.
- **`cfg/`** — Runtime XML config (`bootstrap.xml`, `bootstrap_worker.xml`). `CONFIG_DIRECTORY` env var overrides path.
- **`examples/`** — Reference apps showing how to wire StartupBuilder.
- **`tests/`** — Single binary (`base_tests`), all test files listed explicitly in `tests/CMakeLists.txt`.

## Gotchas

- **New test files**: Must be added to the `ADD_EXECUTABLE(base_tests ...)` list in `tests/CMakeLists.txt`. They won't be discovered automatically.
- **New source files**: Must be added to the `headers`/`sources` lists in `src/CMakeLists.txt`. No globbing.
- **Build dir mismatch**: CI uses `build/build/Release`, README mentions `cmake-build-debug`. Follow CI layout for consistency.
- **PostgreSQL tests**: Require a running Postgres. Use `docker compose up postgres` (user/pass/db: test/test/test on port 5432).
- **TLS certs**: Self-signed dev certs in `cfg/certs/`. Regenerate with `./cfg/certs/generate_certs.sh`. Referenced by `cfg/bootstrap.xml`.
- **PCH**: Precompiled headers are ON by default (`ENABLE_PCH`). New stdlib headers needed by source files should be added to the PCH list in `src/CMakeLists.txt`.
- **`config.h.in`**: CMake substitutes `PROJECT_PATH`, `CONFIG_DIRECTORY`, `BOOTSTRAP_CONFIG_NAME`, `MSG_QUEUE_NAME_SIZE`, `MSG_QUUEUE_CONTENT_SIZE` (note typo in original). Build touches `src/include/base_library/config.h`.
- **Docker dev**: `docker compose --profile dev run dev` mounts source + builds inside container.
- **Debug + GCC**: Boost.UUIDs SSE2-Pfad nutzt `_mm_slli_si128`; bei `-O0` inlined GCC 13 das Intrinsic nicht mehr und bricht mit `the last argument must be an 8-bit immediate` ab. `src/CMakeLists.txt` definiert für `CONFIG:Debug` deshalb `BOOST_UUID_NO_SIMD`.
- **`-Wnull-dereference`**: nur für Clang aktiv (siehe `cmake/CompilerWarnings.cmake`). GCC 13 meldet in `std::function::_M_empty()` einen Fehlalarm, der mit `-Werror` den Release-Build stoppt.

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

| Workflow | Trigger | Content |
|----------|---------|--------|
| `ci.yml` → `build` | push/PR to `master`, `workflow_dispatch` | matrix `gcc-release` (gate), `clang-release`, `gcc-debug-nonunity` (Debug + Non-Unity in one run) |
| `ci.yml` → `format` | same | **blocking**: changed files must not contain new `clang-format` violations; additionally repo-wide drift report (informational only) |
| `ci.yml` → `lint` | same | `clang-tidy` on changed files (`src/`, without `tests/`/`external/`), currently `continue-on-error` |
| `release.yml` | tag `v*`, `workflow_dispatch` (dry-run) | check version → release build → `ctest` → tarball with `bin/`, `cfg/` (without `certs/`), `swagger.yaml` → `gh release create` |

Check the same locally:

```bash
./scripts/check-format.sh master   # format regression on changed files
./scripts/check-format.sh --all    # repo-wide drift report (blocking: --strict)
./scripts/check-tidy.sh master build/tidy   # needs a compile_commands.json without Non-Unity and without PCH
./scripts/check-secrets.sh         # plaintext credentials (--staged / --all)
./scripts/install-deps.sh build-essential cmake   # system packages, retrying (see below)
```

## System packages in CI

All workflows install packages via `scripts/install-deps.sh`, **never** with a bare
`sudo apt-get update && sudo apt-get install -y …`. That form hangs: apt stalls on
the hosted-runner mirrors (`azure.archive.ubuntu.com`, `packages.microsoft.com`) or on
the dpkg lock of the image's `unattended-upgrades`, and the hang burns the whole job
timeout (90 min) instead of failing.

Measured on 2026-10-07: one runner got `azure.archive.ubuntu.com` at **~12 kB/s** and
spent **16 min 45 s** on 14.3 MB (11.2 MB of it `cmake`), while the jobs next to it
fetched the same bytes in 1 s. So the per-call cap is deliberately generous
(`APT_TIMEOUT` 1200 s) — killing a slow-but-alive transfer and restarting it is
strictly worse than waiting. Genuinely dead connections are caught much earlier by
apt itself (`Acquire::*::Timeout`, `Acquire::Retries`).

The script also checks `dpkg-query` first and skips `apt-get update` entirely when the
image already ships everything, retries (`APT_RETRIES`), waits out / kills stale dpkg
locks, runs non-interactively (`--force-confdef`/`--force-confold`), and verifies the
result. `APT_BUDGET` (default 1800 s) caps the total; the workflow step adds
`timeout-minutes: 40` as a hard backstop (12 min for the small format/doxygen steps).

## Secrets

`secret_scanning_push_protection` **cannot be enabled via API** for this repo –
all fields in `security_and_analysis` are rejected with `422`, even with admin rights
(verified 06.10.2026). As long as this has not been added via *Settings → Code security*,
`scripts/check-secrets.sh` takes over (blocking CI job `secrets`).

The script does **not** check against a list of known secrets, but in two stages:
configuration formats (`.xml`/`.yml`/`.env`/…) resp. PEM key blocks, **and** an
entropy test on the value. Pure name search is useless – the repo has dozens of
`const char *const PASSWORD = "password";`, those are JSON key names.

Do not commit credentials. Write passwords in config and examples as `${VAR}` or
`<PLACEHOLDER>`. `cfg/database/*` and `cfg/certs/*` are deliberately excluded
(documented seed placeholder resp. git-ignored dev certificates).

## Code Style

- **Formatting**: `clang-format` — 120-col limit, 2-space indent, custom brace wrapping. Run `clang-format -i` on changed files. CI (`format` job) compares violations against the base commit: changed files must **not** contain **new** violations (legacy violations are tolerated).
- **Warnings**: `-Werror` is ON by default (`lintel_WARNINGS_AS_ERRORS`). Fix all warnings before committing.
- **clang-tidy**: Off by default. Enable with `-DLINTEL_ENABLE_CLANG_TIDY=ON`. Header filter: `.*lintel/.*`.
- **Unity builds**: ON by default (`ENABLE_UNITY_BUILD`). Can mask include-order bugs — disable to test `#include` completeness.

## Architecture

`lintel` is a CMake library (`lintel::lintel`).

- **`src/core/`** — StartupBuilder, DI wiring, persistence abstraction (PostgreSQL + SQLite), logging, services, plugins.
- **`src/features/`** — Optional modules: `http/` (cpp-httplib server/client), `cli/`, `property/` (XML/file/DB/SHM config), `base/` (auth, processes, message queues, event bus, shared memory).
- **`src/include/lintel/`** — Public headers. `config.h` is generated from `config.h.in` by CMake.
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
- **`config.h.in`**: CMake substitutes `PROJECT_PATH`, `CONFIG_DIRECTORY`, `BOOTSTRAP_CONFIG_NAME`, `MSG_QUEUE_NAME_SIZE`, `MSG_QUUEUE_CONTENT_SIZE` (note typo in original). Build touches `src/include/lintel/config.h`.
- **Docker dev**: `docker compose --profile dev run dev` mounts source + builds inside container.
- **Debug + GCC**: the Boost.UUIDs SSE2 path uses `_mm_slli_si128`; at `-O0` GCC 13 no longer inlines the intrinsic and fails with `the last argument must be an 8-bit immediate`. `src/CMakeLists.txt` therefore defines `BOOST_UUID_NO_SIMD` for `CONFIG:Debug`.
- **`-Wnull-dereference`**: active for Clang only (see `cmake/CompilerWarnings.cmake`). GCC 13 reports a false positive in `std::function::_M_empty()` that stops the Release build via `-Werror`.

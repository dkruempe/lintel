# lintel

[![build](https://github.com/dkruempe/lintel/actions/workflows/ci.yml/badge.svg)](https://github.com/dkruempe/lintel/actions/workflows/ci.yml)
[![docs](https://github.com/dkruempe/lintel/actions/workflows/doxygen.yml/badge.svg)](https://dkruempe.github.io/lintel/)
[![coverage](https://codecov.io/gh/dkruempe/lintel/branch/master/graph/badge.svg)](https://codecov.io/gh/dkruempe/lintel)
[![license](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

A modular and feature-rich C++ library designed to accelerate the development of modern, high-performance applications.

> **Status:** pre-1.0 API. Nothing here is covered by semantic versioning yet.

## Overview

`lintel` provides a solid foundation for C++ projects by offering a collection of robust, reusable, and loosely-coupled components. The architecture is split into a powerful `core` engine and a set of optional `features`, allowing you to include only what you need.

The library is designed with extensibility in mind, featuring a plugin system and service-oriented architecture that makes it easy to add custom functionality.

**Start with the [architecture overview](https://dkruempe.github.io/lintel/architecture.html)** for the layer split, the ten-phase bootstrap sequence, dependency injection, the property and persistence abstractions, and the extension points. It is the fastest way to understand how the pieces fit together before reading the class reference.

## Core Features

The `core` of the library provides the fundamental building blocks for any application. These components are designed to be lightweight, efficient, and highly extensible.

- **Service-Oriented Architecture**: A powerful architecture with a `StartupBuilder` for dependency injection and lifecycle management.
- **Property System**: A central and flexible system for managing application configuration. It serves as the primary interface for services to access their settings. Properties can be loaded from various sources, including files (XML, JSON), databases, or shared memory.
- **Bootstrap Plugins**: A specialized plugin system focused on the application's startup process. It allows for modular and extensible initialization routines.
- **Feature Management**: A robust system for managing and enabling optional features within the library.
- **Logging Framework**: A flexible, high-performance logging abstraction with `spdlog` as the default backend.
- **Task Scheduling**: Includes a `SchedulerService` and `ExecutorService` for managing asynchronous operations and scheduled tasks.
- **Abstract Persistence Layer**: Defines a common interface for persistence, allowing for the easy integration of different database backends.
- **Filesystem & Utilities**: Provides a set of convenient wrappers for filesystem operations and a collection of general-purpose utilities like `StringifyService` and `TypeName`.

## Additional Features

These optional modules build upon the core and can be enabled as needed to provide advanced, out-of-the-box functionality.

- **Remote Management via HTTP API**: Provides a comprehensive HTTP-based API (via `cpp-httplib`) for remotely managing and interacting with core library components. This API is the foundation for any frontend.
- **Interactive CLI**: A feature-rich, interactive command-line interface that serves as a reference client for the HTTP API. It is extensible and supports user management.
- **Shared Memory (IPC)**: A service for inter-process communication using `boost::interprocess`.
- **Persistence Backends**:
  - **PostgreSQL**: A concrete implementation of the persistence layer for PostgreSQL databases.
  - **SQLite**: A concrete implementation of the persistence layer for SQLite databases.

## HTTP API Reference

A detailed description of all available HTTP API endpoints is available in the [swagger.md](swagger.md) file. The formal specification can be found in [swagger.yaml](swagger.yaml).

### HTTP Server Configuration

The HTTP server is configured via the `<HttpHost>` section in the bootstrap XML (see `cfg/bootstrap.xml`):

```xml
<HttpHost>
    <Server host="0.0.0.0" port="8080"
            cert_path="certs/server.crt" key_path="certs/server.key"
            require_tls="true"
            trusted_proxies="10.0.0.0/8, 192.168.1.5"/>
    <Client host="localhost" port="8080" cert_path="certs/server.crt"
            key_path="certs/server.key" ca_cert_path="certs/server.crt"/>
</HttpHost>
```

- `cert_path` / `key_path` – PEM certificate/key for HTTPS. When set, the server uses TLS. Relative paths are resolved against the configuration directory (e.g. `cfg/`); the same attributes on `<Client>` enable HTTPS on the client. Self-signed test certificates are generated locally under `cfg/certs/` (see below).
- `ca_cert_path` (client) – PEM CA certificate used to verify the server certificate (e.g. the self-signed `certs/server.crt`). Without it, the HTTPS client rejects self-signed certificates.

To (re)generate self-signed test certificates during initial setup, run:

```bash
./cfg/certs/generate_certs.sh
```

This writes `cfg/certs/server.crt` and `cfg/certs/server.key` (SAN: `localhost`, `127.0.0.1`), which are referenced by the default `cfg/bootstrap.xml`. Both files are git-ignored and are **not** part of the repository, so run this script once before the first build. The CI workflows generate their own throwaway pair, because the TLS integration test requires both files. See [cfg/certs/README.md](cfg/certs/README.md) for details. Replace them with real certificates for production.

- `require_tls` – refuse to start the server without a configured TLS certificate/key (`true`/`false`). Defaults to `false`; use it in production to avoid serving credentials in clear text.
- `trusted_proxies` – comma-separated list of IPs or CIDR ranges (e.g. `10.0.0.0/8`) whose `X-Forwarded-For` header is trusted for client-IP detection. By default the header is ignored, so clients behind a proxy must be listed here for correct IP binding, rate limiting and lockout.

### Health and readiness probes

Two unauthenticated endpoints are always registered:

| Endpoint | Meaning | Response |
|---|---|---|
| `GET /health` | liveness – is the process alive? | always `200 {"status":"ok"}` |
| `GET /ready` | readiness – can the process serve? | `200` when ready, `503` when a dependency is down |

`/health` never touches a dependency, so a broken database does not make a
supervisor restart an otherwise healthy process. `/ready` runs the dependency
checks registered at startup and reports them in the body:

```json
{"status":"ready","checks":{"database:DEFAULT_PSQL":"ok","shared_memory":"ok"}}
{"status":"not_ready","checks":{"database:DEFAULT_PSQL":"failed: connection refused","shared_memory":"ok"}}
```

The checks are contributed by the components that own the dependency:
`DatabaseBootstrapPlugin` registers one `database:<connection name>` check per
configured connection (a `SELECT 1` ping) and `SharedMemoryService` a
`shared_memory` check that verifies its segments are mapped. With no check
registered yet the endpoint answers the legacy `200 {"status":"ready"}`.

Kubernetes example:

```yaml
livenessProbe:  { httpGet: { path: /health, port: 8080 }, initialDelaySeconds: 5 }
readinessProbe: { httpGet: { path: /ready,  port: 8080 }, periodSeconds: 5, failureThreshold: 3 }
```

### Graceful shutdown

On `SIGINT`/`SIGTERM` (Ctrl+C, `docker stop`, Kubernetes pod termination) the
process stops in a defined order: services are shut down by ascending
`shutdownPriorityOf()`, so ordinary (`DEFAULT`) services wind down first and the
`ProcessService` that manages child processes last, after which `start()`
returns and the process exits. A request that is already being served is
finished before the server socket is closed. Give the container a termination
grace period (`docker stop -t`, `terminationGracePeriodSeconds`) that covers the
slowest `onShutdown()`.

### Logging

The `<Loggers>` section of the bootstrap XML decides what a process writes:

```xml
<Path path="${LOG_DIR}/lintel" create_sub_dirs="true"/>
<Logger process_name=".*" level="info" json="false">
    <LoggerSink type="RotatingFileSink" size="5MB" max_files="10"/>
</Logger>
```

- `level` is the effective floor (`critical|warn|err|error|info|debug|trace|off`);
  `err` and `error` name the same level, and a sink may lower it further with its
  own `level` attribute. An unknown name is reported by `/admin/log-level` as
  `400`, while the XML silently falls back to `info`. The pattern lives either
  on `<Logger>` or on the sink and follows spdlog's `%` flags.
- `json="true"` formats every message as one JSON object per line
  (`{"time":...,"level":...,"msg":...}`) for a log shipper. It wins over a
  `pattern` on a sink as well, because the format belongs to the logger, not to
  the destination. The message text itself is not escaped, so keep structured
  data out of quotes there: a `"` or `\` in the message ends or breaks the JSON
  line, control characters do the same. The request id is the only component
  that is filtered before it is written (see the request correlation bullet).
- **Runtime level**: `POST /admin/log-level` with `{"level":"debug"}` changes
  the level of a running process immediately (`401` outside the `Admin-Log`
  group, `400` for an unknown name). The XML is not rewritten, so a restart
  goes back to the deployed level.
- **Request correlation**: every response a controller handler produces carries
  an `X-Request-Id` header - echoed when the client supplied one, generated
  otherwise (a request to an unrouted path is answered by the server's default
  `404`, which carries no header) - and the handling
  thread writes the same id in front of the log lines it produces
  (`[<id>] message`), so the log of one HTTP request can be filtered by a single
  search. A supplied id is only echoed when it matches `[A-Za-z0-9._-]{1,64}`,
  because it is written to the log unescaped; anything else is replaced by a
  generated id.

## Getting Started

### Prerequisites

- C++20 (or newer) compatible compiler (CI uses GCC 14 and Clang 18; GCC 14 is required for `std::chrono::parse`, i.e. libstdc++ 14)
- [CMake](https://cmake.org/) (version 3.16 or newer)
- [Conan](https://conan.io/) version 2 (C/C++ Package Manager)
- [Ninja](https://ninja-build.org/) (the generator used by CI)
- Optionally [ccache](https://ccache.dev/) to speed up repeated builds
- A running PostgreSQL if you want the database-backed tests to do more than skip

### Building the Project

The commands below are identical to the ones the CI pipeline runs (`Release`, GCC 13, Ninja).

1.  **Clone the repository:**
    ```bash
    git clone <repository-url>
    cd lintel
    ```

2.  **Install the dependencies with Conan:**
    ```bash
    conan install . --output-folder=build --build=missing \
        -s build_type=Release \
        -c tools.cmake.cmaketoolchain:generator=Ninja
    ```

3.  **Generate the local TLS test certificates:**
    ```bash
    ./cfg/certs/generate_certs.sh
    ```

4.  **Configure the project with CMake:**
    ```bash
    cmake -S . -B build/build/Release \
        -DCMAKE_TOOLCHAIN_FILE=build/build/Release/generators/conan_toolchain.cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
        -G Ninja
    ```

    Drop `-DCMAKE_CXX_COMPILER_LAUNCHER=ccache` if you do not have `ccache` installed.

5.  **Build the library, examples, benchmarks, and tests:**
    ```bash
    cmake --build build/build/Release --parallel $(nproc)
    ```
    The compiled binaries are written to the `bin/` directory.

A Debug build works the same way with `-s build_type=Debug`, `-DCMAKE_BUILD_TYPE=Debug`
and `-B build/build/Debug`.

## Quickstart (5 Minutes)

This minimal example shows how to bootstrap an application using `StartupBuilder`. The snippet below is fully compilable as shown and uses the real library API.

### Prerequisites for the quickstart

1. Build the project as described above (binaries land in `bin/`).
2. Generate local TLS test certificates once: `./cfg/certs/generate_certs.sh` (self-signed dev certs under `cfg/certs/`). The server requires a certificate/key when `require_tls="true"` in `cfg/bootstrap.xml`.
3. By default, runtime configuration is loaded from `cfg/` (the repository's `cfg/bootstrap.xml`). The `CONFIG_DIRECTORY` environment variable can override this path if needed (see `src/include/lintel/config.h.in`).

```cpp
#include <lintel/app/StartupBuilder.h>
#include <lintel/app/BaseFeature.h>
#include <lintel/app/PropertyFeature.h>

int main(int argc, char* argv[]) {
  auto builder = lintel::StartupBuilder::with(argc, argv);
  builder->addFeature<lintel::BaseFeature>();
  builder->addFeature<lintel::PropertyFeature>();
  builder->start();
  return 0;
}
```

What this does: `lintel::StartupBuilder::with(argc, argv)` initializes the core, `addFeature<lintel::BaseFeature>()` adds base services, `addFeature<lintel::PropertyFeature>()` enables configuration/property support on top of them, and `start()` runs the application using the bootstrap configuration from `cfg/bootstrap.xml`. The order matters: a feature may only be added after the features it requires (`Base` → `Property` → `Http` → `CommandLine`), and `start()` rejects a feature that is registered before its dependency. If TLS is enabled in that configuration, valid certificate/key files must exist under `cfg/certs/` (or the overridden `CONFIG_DIRECTORY`).

## Usage

The primary entry point for using the library is the `StartupBuilder`. This class allows you to fluently configure, initialize, and launch your application.

You can use it to:
- Register custom services and plugins.
- Enable or disable features (like the CLI or HTTP server).
- Configure persistence and other core services.

For concrete implementation details, please refer to the `examples` directory, which contains several samples demonstrating how to use the various components of the library.

## Examples

Every example is built into `bin/` (`CMAKE_RUNTIME_OUTPUT_DIRECTORY` in the top-level `CMakeLists.txt`); `examples/CMakeLists.txt` holds one `add_executable` entry per example.

| Example | What it shows | How to run |
|---|---|---|
| `main.cpp` | The full-stack reference application: `StartupBuilder` with `PropertyFeature`, `BaseFeature`, `CommandLineFeature` and `HttpFeature`, plus three custom shared-memory repositories (object, array, vector) that the HTTP and CLI shared-memory endpoints export. Also starts the `worker` processes declared in the bootstrap configuration. | `./bin/main` |
| `worker.cpp` | The worker process that `main` starts through `ProcessService`: reads properties from the shared-memory mirror, receives change notifications over the event bus and produces history entries, without ever touching a database. | `BOOTSTRAP_CONFIG_NAME=bootstrap_worker ./bin/worker` (see below) |
| `property_change.cpp` | Property values and change notifications between two processes, over a shared-memory property repository and the event bus — including a value that does not fit into the event payload and travels through the mirror instead. Self-contained: it creates its own shared-memory files in the temp directory and needs neither `cfg/` nor a database. | `./bin/property_change` |
| `event_bus.cpp` | `EventBus` on top of a `boost::interprocess` managed shared-memory file: broadcasting to subscribers plus point-to-point events between a forked worker and the main process. Self-contained, like `property_change`. | `./bin/event_bus` |
| `message_queue.cpp` | A thin wrapper around `boost::interprocess::message_queue`: blocking and non-blocking send/receive of a POD payload. | `./bin/message_queue` |
| `db.cpp` | The persistence layer on its own — `db::Connection`, `PreparedStatement`, `ParameterBuilder`, `Result` — querying the `history` table. It connects to **PostgreSQL** with a connection entry built in code instead of reading the bootstrap configuration. | `./bin/db` |
| `named_pipe_server.cpp` | A POSIX FIFO (`mkfifo`) service: creating the pipe, writing, blocking and non-blocking reads, callback-based reading and a small throughput loop. Does not use the library itself. | `./bin/named_pipe_server` |
| `cleanup.cpp` | Removes leftover `boost::interprocess` named mutexes, e.g. after a crash. An argument is also treated as a shared-memory segment name, whose remap semaphore and lock-owner sidecar are removed as well — the manual path when the automatic lock recovery cannot decide. | `./bin/cleanup <mutex-or-segment-name> [...]` |

### Running the examples locally

The full-stack examples are configured through a single, git-ignored `.env` file in the repository root. `scripts/run-example.sh` loads it (every assignment is exported), generates the self-signed development certificates on first use and then starts the selected example:

```bash
cp .env.example .env    # first time only; adjust the values
scripts/run-example.sh              # runs bin/main (HTTP + CLI + workers)
scripts/run-example.sh worker       # runs bin/worker with the worker config
scripts/run-example.sh main --help  # arguments are passed through
```

`.env` sets `CONFIG_DIRECTORY`, `BOOTSTRAP_CONFIG_NAME`, `ADMIN_USERNAME` and `ADMIN_PASSWORD` (see the file for the full list). It is excluded via `.gitignore`, so local credentials and machine-specific paths never end up in a commit. `main` starts the interactive CLI, so run it from a terminal; the HTTP API is available at `https://localhost:8080` (unauthenticated liveness: `curl -fsSk https://localhost:8080/health`).

### Configuration directory

The `StartupBuilder`-based examples (`main`, `worker`) read their bootstrap XML from the directory named by the `CONFIG_DIRECTORY` environment variable. If that variable is not set, the path compiled into the binary is used: `CONFIG_DIRECTORY` is a macro generated into `lintel/config.h` from `src/include/lintel/config.h.in`, which CMake fills with `${PROJECT_SOURCE_DIR}/../cfg` — i.e. the `cfg/` directory of the checkout the binary was built in. The file inside that directory is `BOOTSTRAP_CONFIG_NAME` + `.xml`, where `BOOTSTRAP_CONFIG_NAME` defaults to `bootstrap`. The defaults therefore resolve to `<repo>/cfg/bootstrap.xml` (used by `main`) and `<repo>/cfg/bootstrap_worker.xml` (used by `worker`).

`ProcessService` starts every `<Process>` entry it finds in the loaded configuration. `cfg/bootstrap_worker.xml` has no `<Processes>` section, while `cfg/bootstrap.xml` declares two `../bin/worker` entries, so a manually started worker needs `BOOTSTRAP_CONFIG_NAME=bootstrap_worker` to avoid spawning workers itself. `main` sets that variable automatically for the processes it spawns.

Note that the database schema and seed files are an exception to the `CONFIG_DIRECTORY` override: they are always read from the compiled-in path (`<compiled CONFIG_DIRECTORY>/database/`).

### TLS

`cfg/bootstrap.xml` configures the HTTP server with `cert_path="certs/server.crt"`, `key_path="certs/server.key"` and `require_tls="true"`, all resolved relative to the configuration directory. `./bin/main` therefore refuses to start until the self-signed development certificates have been generated once:

```bash
./cfg/certs/generate_certs.sh
```

The certificates are deliberately not part of the repository (`.gitignore` ignores `*.key`, `*.crt`, `*.csr`, `*.srl`) and are not part of the release tarball either: it ships `bin/` and `cfg/`, minus `cfg/certs`. Examples that do not enable `HttpFeature` need no certificates. See [HTTP Server Configuration](#http-server-configuration) and [cfg/certs/README.md](cfg/certs/README.md).

#### Renewing a certificate without a restart

The pair is loaded into the server's OpenSSL context once at startup, so replacing the files on disk does nothing for a running process. `POST /admin/reload-tls` re-reads them and swaps the pair in place:

```bash
# log in once and keep the bearer token
TOKEN=$(curl -sk -u "$ADMIN_USER:$ADMIN_PASSWORD" https://localhost:8080/user/login | jq -r .id)

# after certbot (or any other renewal) wrote the new pair
curl -sk -X POST https://localhost:8080/admin/reload-tls \
     -H "Authorization: Bearer $TOKEN" -H "Content-Type: application/json"
```

- `200 {"status":"reloaded","results":{"tls":"ok"}}` — the new pair is in use.
- `500` with `"failed: <reason>"` — a file could not be read or OpenSSL rejected the pair. The server **keeps serving the previous certificate**, so a truncated renewal never takes the endpoint down; fix the files and call the endpoint again.
- `409 {"status":"nothing_to_reload"}` — this process serves clear text, so there is no pair to reload.

The endpoint is restricted to the `Admin` group and the reload is safe under load: httplib swaps the context under the mutex the handshake path also takes, so an in-flight handshake sees either the old or the new certificate, never a half-applied pair. The startup path is equally strict — a certificate OpenSSL cannot load stops the server with `cannot load the TLS certificate ...` rather than a misleading bind error.

### SQLite vs. PostgreSQL

`cfg/bootstrap.xml` ships with SQLite as the default connection (`DEFAULT_SQLITE`, `~/temp.db`, `default="true"`). A leading `~` is expanded to the user's home directory, so the database file is created there and its schema and seed data are applied from `cfg/database/DEFAULT_SQLITE/`. No database server is required for `./bin/main`.

PostgreSQL is the alternative: declare a `<DatabaseConnection>` with `type="PostgreSQL"` and mark it as the default; schema and seed data for it live in `cfg/database/DEFAULT_PSQL/`. `./bin/db` is the only example that talks to PostgreSQL directly, because it does not use the bootstrap configuration — it hard-codes host `127.0.0.1`, database `temp` and user `example_user` (the seed user from `cfg/database/*/data_schema_default_version_1.sql`) and expects you to replace the password placeholder in the source.

## Documentation

The full API reference is generated from the public headers with Doxygen and published on every push to `master`:

**<https://dkruempe.github.io/lintel/>**

| Page | What it covers |
|---|---|
| [Architecture](https://dkruempe.github.io/lintel/architecture.html) | The `core`/`features` split, the bootstrap sequence and its ordering contract, dependency injection, properties, persistence, shared memory, transports, extension points, integration with `find_package` |
| [Conventions](https://dkruempe.github.io/lintel/architecture_conventions.html) | Header hygiene, namespaces, error handling, test constraints |
| [Class reference](https://dkruempe.github.io/lintel/annotated.html) | Generated from the public headers |
| [Tutorial: the HTTP API in 15 minutes](docs/tutorial.md) | Start the reference app, talk to the API over TLS, authenticate, renew the certificate, deploy |

The `Doxygen` workflow runs with `WARN_AS_ERROR`, so undocumented or malformed comments fail the build rather than accumulating. To generate the documentation locally:

```bash
doxygen Doxyfile   # output goes to build/doxygen/html
```

`Doxyfile` takes `docs/architecture.dox` (the two hand-written pages above) plus `src/include/lintel` — the public headers only. Implementation files, tests and examples are intentionally excluded.

## Dependencies

This project uses [Conan](https://conan.io/) to manage the following external libraries:

- [Boost](https://www.boost.org/)
- [Catch2](https://github.com/catchorg/Catch2)
- [cpp-httplib](https://github.com/yhirose/cpp-httplib)
- [date](https://github.com/HowardHinnant/date)
- [fmt](https://fmt.dev/)
- [libpq](https://www.postgresql.org/docs/current/libpq.html)
- [magic_enum](https://github.com/Neargye/magic_enum)
- [OpenSSL](https://www.openssl.org/)
- [RapidJSON](https://rapidjson.org/)
- [spdlog](https://github.com/gabime/spdlog)
- [SQLite3](https://www.sqlite.org/index.html)
- [tinyxml2](https://github.com/leethomason/tinyxml2)
- [zlib](https://zlib.net/)

## Docker

Docker workflows are defined in [`docker-compose.yml`](docker-compose.yml). The following profiles and commands match the actual configuration:

- **`dev` profile** (`service: dev`, image `lintel:dev`, target `base`, working dir `/workspace`): mounts the source (`.:/workspace`), caches Conan (`/root/.conan2`), build artifacts (`/workspace/build`), and ccache (`/root/.ccache`). On startup it runs `conan profile detect --force`, installs dependencies with Conan (Release), configures CMake using the generated toolchain, and builds the target in parallel. Note that this profile does not pass `-G Ninja`, so the container builds with CMake's default generator (Makefiles) even though `ninja-build` is installed; pass `-G Ninja` in `docker-compose.yml` if you want the CI generator.  
  ```bash
  docker compose --profile dev up --build
  ```
- **`test` profile** (`service: test`, target `test`, depends on `postgres` with `service_healthy`): runs the tests via the image's `CMD ["ctest", "--output-on-failure"]` from `/workspace/build/build/Release`, with `CONFIG_DIRECTORY=/workspace/cfg`. Postgres (image `postgres:17`, user/password/db `test`/`test`/`test`, port `5432:5432`) is started as a dependency.  
  ```bash
  docker compose --profile test up --build --abort-on-container-exit
  ```
- **`app` profile** (`service: app`, target `runtime`, working dir `/workspace`, ports `8080:8080`, `CONFIG_DIRECTORY=/workspace/cfg`, depends on `postgres` healthy): runs `./bin/main`. `cfg/bootstrap.xml` sets `require_tls="true"`, so the API is served over **HTTPS** on port `8080` using the self-signed development certificates in `cfg/certs/` — run `./cfg/certs/generate_certs.sh` before building the image. The service declares a healthcheck against the unauthenticated `GET /health` endpoint (`curl -fsSk https://localhost:8080/health`; `-k` skips verification of the self-signed dev certificate). The runtime image contains the required libraries (`libpq5`, `libsqlite3-0`, `libssl3t64`, `ca-certificates`), `curl` for the healthcheck, and copies `bin/` and `cfg/` from the builder stage.  
  ```bash
  docker compose --profile app up --build
  ```

Notes: the dev container exposes no published ports by default; `postgres` is published on `5432` (as defined). These commands were verified against `docker compose config`.

### Development defaults are not production-safe

The compose profiles are development defaults, not a production deployment:

- **Database credentials**: `postgres` runs with user/password/db `test`/`test`/`test` and publishes port `5432` to the host, so it is reachable with guessable credentials. The app's default connection is SQLite (see [SQLite vs. PostgreSQL](#sqlite-vs-postgresql)), so this Postgres is only needed by the DB-backed tests.
- **TLS**: the app terminates HTTPS, but with the **self-signed development certificate** in `cfg/certs/`, which is git-ignored and not shipped in the release tarball.
- **For production**: do not publish `5432`, replace the credentials with real secrets, keep `require_tls="true"` in `cfg/bootstrap.xml` and point `cert_path`/`key_path` at real certificates. See [HTTP Server Configuration](#http-server-configuration) and [TLS](#tls).

### Production profile

`cfg/bootstrap_production.xml` is a ready-to-adapt starting point that does not
carry the development conveniences. Select it without touching the default file:

```bash
export BOOTSTRAP_CONFIG_NAME=bootstrap_production
export LOG_DIR=/var/log/lintel SHM_DIR=/dev/shm/lintel
export DB_HOST=db.internal DB_PORT=5432 DB_USER=lintel DB_PASSWORD='<secret>' DB_NAME=lintel
export TLS_CERT_FILE=/etc/lintel/tls/server.crt TLS_KEY_FILE=/etc/lintel/tls/server.key
./bin/main
```

It differs from `cfg/bootstrap.xml` in the ways that matter for an operation:

- TLS is mandatory (`require_tls="true"`) and reads the certificate/key from
  `${TLS_CERT_FILE}` / `${TLS_KEY_FILE}` instead of the self-signed files under
  `cfg/certs/`.
- The default connection is **PostgreSQL** with credentials from the environment
  (no SQLite fallback, no committed password).
- The log directory and the shared-memory directory come from `${LOG_DIR}` and
  `${SHM_DIR}` (put the latter on a tmpfs), not from the developer's `$HOME`.
- The logger rotates at `info`.

Every `${NAME}` is expanded from the environment; a variable that is not set
stays literal, so a missing `${DB_*}`/`${TLS_*}` makes startup fail loudly
instead of silently connecting somewhere unexpected. Adjust the file for your
deployment (segment sizes, `trusted_proxies`, child `<Process>` entries) as
needed.

## Testing

The unit tests use [Catch2](https://github.com/catchorg/Catch2) and are built as a single `base_tests` binary. Build the project and run the unit tests from the build directory:

```bash
cd build/build/Release
ctest --output-on-failure --label-regex unit
```

All test files are listed explicitly in `tests/CMakeLists.txt`; new test files are not discovered automatically.

The PostgreSQL-backed tests need a running server (`docker compose up postgres`, database/user/password `test`/`test`/`test` on port `5432`). Without one they are reported as skipped rather than failed.

### Coverage

Line and function coverage are measured with gcov/lcov by the advisory CI job `coverage` and published to [Codecov](https://codecov.io/gh/dkruempe/lintel) (see the badge above). To reproduce it locally, configure a Debug build with `-DENABLE_COVERAGE=ON`, run the unit tests, and generate the report:

```bash
cmake -S . -B build/build/Debug ... -DENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build/build/Debug --parallel "$(nproc)"
(cd build/build/Debug && ctest --output-on-failure --label-regex unit)
./scripts/coverage-report.sh          # writes coverage/coverage.info and coverage/index.html
```

### Benchmarks

`BUILD_BENCHMARKS=ON` builds `bin/base_benchmarks` (google/benchmark). It covers the property hotpath, DTO (de)serialization, the event bus, message queues, SQLite persistence and HTTP transport throughput (plain and TLS). The TLS cases read `LINTEL_BENCH_TLS_CERT`/`LINTEL_BENCH_TLS_KEY` and fall back to `cfg/certs/`; they report themselves as skipped when there is no pair, so generate it once with `./cfg/certs/generate_certs.sh`.

```bash
./bin/base_benchmarks --benchmark_filter='BM_Http' --benchmark_min_time=0.2s
```

The advisory CI job `benchmarks` compares a run against the recorded
[`benchmarks/baseline.json`](benchmarks/baseline.json) and reports every benchmark whose
latency grew by more than 10%:

```bash
./bin/base_benchmarks --benchmark_min_time=0.2s --benchmark_repetitions=3 \
  --benchmark_report_aggregates_only=true --benchmark_out=benchmark_results.json --benchmark_out_format=json
./scripts/check-benchmarks.sh --threshold 10 benchmark_results.json
./scripts/check-benchmarks.sh --update-baseline benchmark_results.json   # refresh the baseline
```

Absolute numbers depend on the machine, so the comparison normalizes each benchmark against the geometric mean of the whole run: a globally slower host cancels out and only benchmarks that moved relative to the rest are reported. Real time is the compared metric, not CPU time — the delayed-acknowledgement stall that `ROADMAP.md` P6 fixed cost 40 ms of real time per TLS request while the CPU time stayed flat.

## Contributing

The checks a change has to pass are documented in [AGENTS.md](AGENTS.md): build, `ctest --output-on-failure --label-regex unit`, `./scripts/check-format.sh <base-branch>` (blocking format gate), `./scripts/check-secrets.sh` (blocking plaintext-credential guard) and `./scripts/check-tidy.sh <base-branch> build/tidy` (advisory).

See [CONTRIBUTING.md](CONTRIBUTING.md) for the full contribution flow and [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md). Security issues are handled separately — see [SECURITY.md](SECURITY.md).

[`docs/code-scanning.md`](docs/code-scanning.md) describes how the CodeQL setup works and how to triage an alert. The short version: check an alert's coordinates against the actual code before changing anything — with `build-mode: none` CodeQL has no compilation database and its line numbers are frequently fiction.

## License

This project is licensed under the [MIT License](LICENSE). Third-party dependencies and their licenses are listed in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

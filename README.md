# C++ Base Library

[![build](https://github.com/dkruempe/cpp-base-library/actions/workflows/ci.yml/badge.svg)](https://github.com/dkruempe/cpp-base-library/actions/workflows/ci.yml)
[![license](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

A modular and feature-rich C++ library designed to accelerate the development of modern, high-performance applications.

> **Status:** private repository, pre-1.0 API. Nothing here is covered by semantic versioning yet.

## Overview

`cpp-base-library` provides a solid foundation for C++ projects by offering a collection of robust, reusable, and loosely-coupled components. The architecture is split into a powerful `core` engine and a set of optional `features`, allowing you to include only what you need.

The library is designed with extensibility in mind, featuring a plugin system and service-oriented architecture that makes it easy to add custom functionality.

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

## Getting Started

### Prerequisites

- C++17 (or newer) compatible compiler (CI uses GCC 13 and Clang 18)
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
    cd cpp-base-library
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
3. By default, runtime configuration is loaded from `cfg/` (the repository's `cfg/bootstrap.xml`). The `CONFIG_DIRECTORY` environment variable can override this path if needed (see `src/include/base_library/config.h.in`).

```cpp
#include <base_library/core/StartupBuilder.h>
#include <base_library/features/base/BaseFeature.h>
#include <base_library/features/property/PropertyFeature.h>

int main(int argc, char* argv[]) {
  auto builder = StartupBuilder::with(argc, argv);
  builder->addFeature<PropertyFeature>();
  builder->addFeature<BaseFeature>();
  builder->start();
  return 0;
}
```

What this does: `StartupBuilder::with(argc, argv)` initializes the core, `addFeature<PropertyFeature>()` enables configuration/property support, `addFeature<BaseFeature>()` adds base services, and `start()` runs the application using the bootstrap configuration from `cfg/bootstrap.xml`. If TLS is enabled in that configuration, valid certificate/key files must exist under `cfg/certs/` (or the overridden `CONFIG_DIRECTORY`).

## Usage

The primary entry point for using the library is the `StartupBuilder`. This class allows you to fluently configure, initialize, and launch your application.

You can use it to:
- Register custom services and plugins.
- Enable or disable features (like the CLI or HTTP server).
- Configure persistence and other core services.

For concrete implementation details, please refer to the `examples` directory, which contains several samples demonstrating how to use the various components of the library.

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

- **`dev` profile** (`service: dev`, image `cpp-base-library:dev`, target `base`, working dir `/workspace`): mounts the source (`.:/workspace`), caches Conan (`/root/.conan2`), build artifacts (`/workspace/build`), and ccache (`/root/.ccache`). On startup it runs `conan profile detect --force`, installs dependencies with Conan (Release), configures CMake using the generated toolchain, and builds with Ninja in parallel.  
  ```bash
  docker compose --profile dev up --build
  ```
- **`test` profile** (`service: test`, target `test`, depends on `postgres` with `service_healthy`): runs the tests via the image's `CMD ["ctest", "--output-on-failure"]` from `/workspace/build/build/Release`, with `CONFIG_DIRECTORY=/workspace/cfg`. Postgres (image `postgres:17`, user/password/db `test`/`test`/`test`, port `5432:5432`) is started as a dependency.  
  ```bash
  docker compose --profile test up --build --abort-on-container-exit
  ```
- **`app` profile** (`service: app`, target `runtime`, working dir `/workspace`, ports `8080:8080`, `CONFIG_DIRECTORY=/workspace/cfg`, depends on `postgres` healthy): runs `./bin/main`. The runtime image contains only the required libraries (`libpq5`, `libsqlite3-0`, `libssl3t64`, `ca-certificates`) and copies `bin/` and `cfg/` from the builder stage.  
  ```bash
  docker compose --profile app up --build
  ```

Notes: the dev container exposes no published ports by default; `postgres` is published on `5432` (as defined). These commands were verified against `docker compose config`.

## Testing

The unit tests use [Catch2](https://github.com/catchorg/Catch2) and are built as a single `base_tests` binary. Build the project and run the unit tests from the build directory:

```bash
cd build/build/Release
ctest --output-on-failure --label-regex unit
```

All test files are listed explicitly in `tests/CMakeLists.txt`; new test files are not discovered automatically.

The PostgreSQL-backed tests need a running server (`docker compose up postgres`, database/user/password `test`/`test`/`test` on port `5432`). Without one they are reported as skipped rather than failed.

## Contributing

The checks a change has to pass are documented in [AGENTS.md](AGENTS.md): build, `ctest --output-on-failure --label-regex unit`, `./scripts/check-format.sh <base-branch>` (blocking format gate) and `./scripts/check-tidy.sh <base-branch> build/tidy` (advisory). `CONTRIBUTING.md` and `SECURITY.md` are still on the roadmap.

## License

This project is licensed under the [MIT License](LICENSE). Third-party dependencies and their licenses are listed in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

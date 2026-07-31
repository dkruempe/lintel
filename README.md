# C++ Base Library

A modular and feature-rich C++ library designed to accelerate the development of modern, high-performance applications.

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
            key_path="certs/server.key"/>
</HttpHost>
```

- `cert_path` / `key_path` – PEM certificate/key for HTTPS. When set, the server uses TLS. Relative paths are resolved against the configuration directory (e.g. `cfg/`); the same attributes on `<Client>` enable HTTPS on the client. Self-signed test certificates are committed under `cfg/certs/` for local development.
- `require_tls` – refuse to start the server without a configured TLS certificate/key (`true`/`false`). Defaults to `false`; use it in production to avoid serving credentials in clear text.
- `trusted_proxies` – comma-separated list of IPs or CIDR ranges (e.g. `10.0.0.0/8`) whose `X-Forwarded-For` header is trusted for client-IP detection. By default the header is ignored, so clients behind a proxy must be listed here for correct IP binding, rate limiting and lockout.

## Getting Started

### Prerequisites

- C++17 (or newer) compatible compiler
- [CMake](https://cmake.org/) (version 3.16 or newer)
- [Conan](https://conan.io/) (C/C++ Package Manager)

### Building the Project

1.  **Clone the repository:**
    ```bash
    git clone <repository-url>
    cd cpp-base-library
    ```

2.  **Install dependencies using Conan:**
    This command will download and set up the required libraries.
    ```bash
    conan install . --output-folder=cmake-build-debug --build=missing
    ```

3.  **Configure the project with CMake:**
    This command generates the build files, linking the Conan dependencies.
    ```bash
    cmake -S . -B cmake-build-debug -DCMAKE_TOOLCHAIN_FILE=cmake-build-debug/conan_toolchain.cmake
    ```

4.  **Build the library, examples, and tests:**
    ```bash
    cmake --build cmake-build-debug --parallel
    ```
    The compiled binaries will be located in the `bin` directory.

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

## Testing

The library is tested using the [Catch2](https://github.com/catchorg/Catch2) framework. To run the tests, build the project and then execute `ctest` from the build directory:

```bash
cd cmake-build-debug
ctest
```

## License

This project is licensed under the [MIT License](LICENSE).

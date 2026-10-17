# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- **`${ENV_VAR}` expansion for the bootstrap configuration.** `EnvironmentConfiguration::expand()`
  replaces every `${NAME}` (`NAME` = `[A-Za-z_][A-Za-z0-9_]*`) with the environment variable of
  that name; an unset variable stays literal and `$${NAME}` escapes to `${NAME}`.
  `Configuration::loadConfiguration()` expands the raw XML before parsing, and a malformed
  document now aborts startup with a `ConfigurationException` carrying the tinyxml2 error and
  line number. Documented in `docs/architecture.dox` and covered by integration tests.
- **Runtime benchmark suite.** New `PropertyService`, `EventBus` and `MessageQueue` benchmarks
  (alongside the existing DTO/Regex/SQLite ones) and an advisory CI job `benchmarks` that builds
  `base_benchmarks`, runs it and uploads a JSON result artifact.
- **ThreadSanitizer CI job** `tsan` (advisory, GCC 14), modelled on the existing ASan+UBSan job.
- **Code-coverage measurement.** New CMake option `ENABLE_COVERAGE` (gcov instrumentation) and
  `scripts/coverage-report.sh`, which turns the gcov data into an lcov tracefile and an html
  report. The advisory CI job `coverage` builds a Debug binary with coverage, runs the unit
  tests, uploads the report as an artifact and publishes the tracefile to **Codecov**
  (`codecov/codecov-action@v7`, authenticated with the `CODECOV_TOKEN` secret). The README
  carries a coverage badge and `codecov.yml` configures the status checks (`ROADMAP.md` §4).
  The lcov filter also now excludes Conan 2's `~/.conan2` cache — the previous `*/conan2/*`
  pattern missed the leading dot and left third-party Boost headers in the report.
- **Fuzzing harnesses for the parsers.** New `fuzz/` directory with `fuzz_config_env`
  (`EnvironmentConfiguration::expand`), `fuzz_json_dto` (every JSON DTO `deserialize`) and
  `fuzz_property_xml` (`XMLConfigSerializationStrategy::deserialize`), built with
  `-DBUILD_FUZZERS=ON`. Clang builds real libFuzzer targets; other compilers build the same
  harness behind a standalone driver. The advisory CI job `fuzz` runs each against a seed
  corpus with ASan.
- **API-specification lint and a written versioning policy (`ROADMAP.md` A7).** New
  `scripts/check-swagger.sh` (blocking CI job `swagger`) validates `swagger.yaml` (Swagger 2.0):
  version against `LINTEL_VERSION`, dangling `$ref`s, operations without `responses`,
  undeclared security schemes, and that the `swagger.md` mirror's `## Version:` matches.
  `docs/versioning.md` documents the SemVer contract, the public-API surface and the
  deprecation/release process.
- **Docker `app` healthcheck** via the unauthenticated `GET /health` endpoint (`curl` added to
  the runtime image), plus explicit development-safety warnings in `docker-compose.yml` and the
  README about published `test/test` PostgreSQL credentials and non-production TLS.
- **Shared-memory grow/shrink semantics are now tested and documented (`ROADMAP.md` A6).**
  `tests/features/SharedMemoryServiceTest.cpp` exercises `SharedMemoryService::growOf()` /
  `shrinkOf()` against real segments: `grow=0` is a no-op, a request beyond the remaining
  headroom is capped at `maxSize`, growing an already-maximal segment throws, an unknown segment
  raises `ShmSegmentNotFound`, and the generation counter advances. `docs/architecture.dox`
  documents the sizing semantics.
- **LTO Release artifacts measured (`ROADMAP.md` §4).** `docs/compile-time-measurements.md`
  (Record 4) compares a clean `-DENABLE_LTO=ON` build against the default: **−6…−19 %** binary
  size at essentially unchanged build time, but workload-dependent throughput (a regex-heavy
  microbenchmark regresses ~15 %, the library's DTO paths are unchanged). `ENABLE_LTO` stays
  opt-in.
- **Dependency-checked readiness probe and a production profile (`ROADMAP.md` item 18 / U4).**
  `GET /ready` no longer answers unconditionally: it runs the checks of the new process-wide
  `lintel::ReadinessRegistry` and answers `503` with the reasons while a dependency is down
  (`DatabaseBootstrapPlugin` registers a `database:<name>` `SELECT 1` check per connection,
  `SharedMemoryService` a `shared_memory` check; a throwing check counts as failed and the body
  is serialized with rapidjson so an exception message cannot break it). With no check
  registered the historical `200 {"status":"ready"}` is preserved; `/health` stays a
  dependency-free liveness probe. `cfg/bootstrap_production.xml` is a new starting point that
  forces TLS, reads PostgreSQL credentials and the log/shared-memory directories from the
  environment, and ships no fallback credentials. The graceful-shutdown order (services are shut
  down by ascending `shutdownPriorityOf()`, the process manager last) is now pinned by a test in
  `tests/core/StartupBuilderTest.cpp` and documented in `docs/architecture.dox` and the README.
- **TLS certificate renewal without a restart, plus a session-handling audit
  (`ROADMAP.md` item 17 / U3).** New `POST /admin/reload-tls` (admin group) re-reads the
  configured certificate/key pair and swaps it into the running OpenSSL context: `200`
  `{"status":"reloaded","results":{"tls":"ok"}}`, `500` with the reason when a file cannot be
  read or OpenSSL rejects the pair, `409` when the process serves clear text. The new
  `lintel::TlsReloadRegistry` holds the reload handlers; `lintel::Server` registers the TLS pair
  on startup and unregisters it on shutdown, so the registry never keeps a handler pointing at a
  destroyed server. A rejected renewal keeps the pair that was already being served — the loader
  installs the certificate before it validates the key, so a mismatched pair would otherwise
  leave the running server unable to complete a handshake. A certificate OpenSSL cannot load now
  stops startup with `cannot load the TLS certificate ...` instead of a misleading
  `failed to bind`. Wire contract: `swagger.yaml` / `swagger.md`.
- **One canonical client address for token binding, lockout and rate limiting.**
  `AuthService::onLogoutOf()` compared the raw incoming address with the stored, canonical one,
  so a client whose address arrived in another spelling (`2001:DB8::1` vs. `2001:db8::1`, or an
  IPv4 client reported as `::ffff:192.0.2.1`) kept its session alive after a logout that the API
  reported as successful. Login, access and logout now share one `normalizedIpOf()`, which also
  makes the sliding-window rate limit a single budget per real client. The brute-force
  protections (per-address `m_maxLoginAttempts`/`m_rateLimitWindow` and
  `m_maxLoginFailures`/`m_loginLockout`), their NAT and `trusted_proxies` consequences and the
  limits of SQLite-side secret scrubbing are documented in `docs/architecture.dox`.

### Changed
- **The public API moved into `namespace lintel` (breaking).** Every public class, free
  function, enum and macro expansion that used to live in the global C++ namespace is now
  qualified with `lintel::`; the persistence layer sits one level deeper in `lintel::db` and the
  shared-memory string helpers in `lintel::shm` (with `lintel::postgresql` / `lintel::sqlite` for
  the backend details). Include paths and include guards are unchanged (`#include <lintel/...>`,
  `LINTEL_*`). The generated `config.h` (version macros and the message-queue constants) stays at
  global scope. Consumers must qualify the names they use:

  | Before (global) | After |
  |---|---|
  | `StartupBuilder` | `lintel::StartupBuilder` |
  | `Features`, `Feature<T>` | `lintel::Features`, `lintel::Feature<T>` |
  | `Property`, `PropertyService`, `PropertyFeature` | `lintel::Property`, `lintel::PropertyService`, `lintel::PropertyFeature` |
  | `EventBus`, `EventBusService`, `Event<T>` | `lintel::EventBus`, `lintel::EventBusService`, `lintel::Event<T>` |
  | `type_name<T>()` | `lintel::type_name<T>()` |
  | `db::Connection`, `db::Result`, `db::SQLException` | `lintel::db::Connection`, `lintel::db::Result`, `lintel::db::SQLException` |
  | `shm::String`, `shm::constructString` | `lintel::shm::String`, `lintel::shm::constructString` |

  This was a one-time hard cut **without** a `using`-directive transition window (see `ROADMAP.md` A1). A new blocking CI job `namespace` (`scripts/check-namespace.sh`) fails if a public header declares its own names at global scope again.
- **`core` and every feature are now separately linkable CMake targets (breaking, `ROADMAP.md` A3).**
  What used to be one monolithic `lintel::lintel` is now an acyclic stack of exported
  targets - `lintel::core` &larr; `lintel::base` &larr; `lintel::property` &larr; `lintel::http`
  &larr; `lintel::cli` &larr; `lintel::app` - so an application only links the layers it
  needs. The composition classes (`StartupBuilder`, `ConfigurationComponentBuilder`,
  every `*Feature`) were moved into the new top layer `lintel::app`. `lintel::lintel`
  remains as an `INTERFACE` aggregate that links everything. Several public headers
  moved to their correct layer:

  | Before | After |
  |---|---|
  | `lintel/core/StartupBuilder.h`, `lintel/core/plugins/*Feature` | `lintel/app/StartupBuilder.h`, `lintel/app/{Base,Property,Http,CommandLine}Feature.h` |
  | `lintel/core/plugins/*` (concrete bootstrap plugins) | `lintel/app/plugins/` |
  | `lintel/features/base/configuration/{Component,Configuration,Entry,ConfigurationException,EnvironmentConfiguration,Logger*,DatabaseConnectionEntry}.h` | `lintel/core/configuration/` |
  | `lintel/features/base/models/{ProcessName,SharedMemorySegmentInfo}.h`, `lintel/features/base/events/{ShmSegmentAccessor,BoostSegmentAllocator}.h` | `lintel/core/models/`, `lintel/core/shm/` |
  | property models (`Property.h`, `PropertyBase.h`, `DataStorage.h`, …) | `lintel/core/models/` |
  | `lintel/features/http/service/HttpBadRequestException.h` | `lintel/core/exceptions/HttpBadRequestException.h` |
  | `lintel/core/services/ProcessService.h`, `SharedMemoryService.h` | `lintel/features/base/services/…` |
  | `lintel/features/property/controller/{PropertyApi,PropertyController}.h` | `lintel/features/http/controllers/…` |
  | `lintel/features/property/command_line/PropertyCliComponent.h` | `lintel/features/cli/components/…` |

  The stale `base_library` references in `cmake/lintelConfig.cmake.in` (wrong targets file
  and `check_required_components`) were fixed as well.
- **The error-handling model is now documented per layer (`ROADMAP.md` A4).** The library stays
  exception-based (no `Result`/`std::expected`): `core` persistence/config throw
  (`db::SQLException` / `ConfigurationException`), feature repositories degrade deliberately
  (empty result = "keep the configured default") and log, services propagate domain errors but
  degrade once a change is already effective in memory, and the HTTP layer maps to status codes.
  See `docs/architecture.dox` (*Errors*); `DatabasePropertyRepository` is the reference case.
- **The DI container is now `lintel::di` (breaking, `ROADMAP.md` A2).** The vendored, unmaintained
  Hypodermic (2017) was replaced by a header-only `lintel::di::Container` / `ContainerBuilder`
  (`lintel/core/di/Container.h`) with the same registration API the code already used
  (`registerInstance`, `registerType<T>().as<I>().asSelf().singleInstance()`, `resolve<T>()`,
  `resolveAll<T>()`). Constructor parameters of type `std::shared_ptr<...>` and
  `std::vector<std::shared_ptr<...>>` are injected; unregistered concrete types are auto-registered
  transiently; registrations resolve to the last registration resp. all in registration order.
  Unlike before, a dependency cycle is reported as `lintel::di::ResolutionException` instead of
  recursing without bound. Public headers now forward-declare `lintel::di::Container` /
  `ContainerBuilder` instead of `Hypodermic::Container` / `ContainerBuilder`.

### Removed
- **The vendored `external/Hypodermic/` tree** (and its `external/CMakeLists.txt`, package-config
  glue in `cmake/lintelConfig.cmake.in`, and `tests/external/HypodermicSignalTest.cpp`) is gone.
  Hypodermic was an unmaintained dependency; `lintel::di` replaces it.

### Fixed
- **Lost wakeup when stopping a worker thread.** `ExecutorService`'s destructor set its `m_exit`
  flag and called `notify_all()` without holding the mutex that the worker's `wait(lock, pred)`
  reads the flag under. When the notification landed in the window between the worker's predicate
  check and its block, the worker slept forever and the destructor's `join()` hung — the
  intermittent `ExecutorService: execute runs a task and resolves its future` hang under
  ThreadSanitizer. The flag is now flipped under the mutex; the same anti-pattern in
  `SchedulerService::onShutdown()`/destructor and `StartupBuilder::onShutdown()` is fixed as well.
  Discovered tests now carry a 120 s CTest `TIMEOUT` so a future hang fails fast instead of
  blocking the sanitizer job.
- **A crashed shared-memory holder no longer deadlocks the segment (`ROADMAP.md` A6).**
  `shm::SegmentSemaphoreGuard` acquired the segment's named remap semaphore with an unbounded
  `wait()`; a process that died while holding it left the count at zero, so every other process
  blocked forever. Acquisition is now bounded (default 30 s) and throws a descriptive
  `std::runtime_error` on timeout, leaving the orphaned handle for out-of-band recovery.
- **`swagger.md` advertised the wrong version.** The mirror's heading still said `Version:
  0.1.0` while `swagger.yaml` had advanced to `0.2.0`; the CMake check only covers
  `swagger.yaml`. Corrected, and `scripts/check-swagger.sh` now keeps both in sync (`ROADMAP.md`
  A7).
- **Schema migrations of an existing database were silently dropped (`ROADMAP.md` A5).**
  `DatabaseBootstrapPlugin::initDatabase()` started its version counter at `0` instead of the
  version recorded in `schema_version`, so the first `init_*_version_N.sql` of an upgrade
  (`N > 1`) tripped the "version jump" guard and the migration never ran. The counter now
  starts at the recorded version, so higher-versioned init/data file pairs form a working
  migration path. The file ordering comparator was also not a strict weak ordering (undefined
  behaviour for `std::sort`) and is fixed. Tests cover a single upgrade and a multi-step chain.
- **`DatabasePropertyRepository::allOf()`** default `instanceName` was `".*0"` instead of `".*"`,
  so every call omitting the instance filter returned nothing (`LIKE '%0'`). Fixed the default;
  regression test added.
- **`DELETE /user/delete` with two or more names answered 500.** `UserRepository::deleteOf(vector)`
  committed the transaction inside the loop; it now commits once after the loop. Regression tests
  in `UserGroupRepositoryTest.cpp` and `UserControllerTest.cpp`.
- **`POST /process/start` returned an empty body.** It now returns `{"id":"<uuid>"}` of the
  started process, documented in `swagger.yaml`/`swagger.md`.
- **`ADD_HANDLER_METHOD` defaulted `Content-Type: application/json` only for GET**, so a
  PUT/POST/DELETE with a JSON body but no explicit header was answered 403. The default now
  applies to all verbs; swagger documents the behaviour and tests pin it.
- **Code scanning `cpp/cleartext-storage-database` false positive.** The repeated alert was
  removed at its source: the `AdminUserBootstrapPluginTest` fixture now inserts through
  `db::ParameterBuilder` with `?` placeholders instead of splicing a `password` variable into the
  SQL text, so the rule no longer follows a test value into `sqlite3_exec`.

## [0.2.0] - 2026-10-11

### Added
- **Secret guard** (`scripts/check-secrets.sh`, blocking CI job `secrets`): blocks plaintext credentials in the working tree. Replacement for `secret_scanning_push_protection`, which cannot be enabled for this repo via API (all fields in `security_and_analysis` are rejected with 422, even with admin rights). Checks in two stages – config formats resp. PEM key blocks, **and** an entropy test on the value; a pure name search is useless, because the repo has dozens of `const char *const PASSWORD = "password";` as JSON key names.
- README: Doxygen badge and a "Documentation" section with the published API reference URL and the local generation command.
- Official OSS documents: `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `SECURITY.md` as well as `CHANGELOG.md` (this document).
- README: CI status badge and MIT license badge, a status note about pre-1.0/API stability as well as notes on tests, the PostgreSQL dependency and certificates (correctly aligned with CI).
- **Working CMake package:** `find_package(base_library CONFIG)` finds the installed package, `base_libraryConfig.cmake` (new) with a `find_dependency` chain and relocatability as well as `base_libraryConfigVersion.cmake` in canonical naming. `LICENSE`, `README.md` and `swagger.yaml` are installed along with it. The installed package works from an arbitrary prefix – verified with an external counter-sample project.
- README: "Examples" section with a table of all examples (what it shows, how to start it), an explanation of `CONFIG_DIRECTORY` and `BOOTSTRAP_CONFIG_NAME`, TLS prerequisites and SQLite vs. PostgreSQL.

### Changed
- **The project now requires C++20 (breaking for consumers still on C++17).** The library, tests, benchmarks and examples set `target_compile_features(... cxx_std_20)` and `.clang-format` uses `Standard: c++20`. GCC 14 / libstdc++ 14 is the new minimum, because `std::chrono::parse` (used for timestamp deserialization) does not exist in libstdc++ 13; CI builds with `gcc-14`/`g++-14` (the Clang jobs rely on libstdc++ 14 as well).
- **The `date` library (Howard Hinnant) was removed.** Timestamps now use `std::chrono::parse` and `std::format`; the dependency is gone from `conanfile.txt`, the CMake config and the build files.
- **`fmt` was replaced by `std::format`.** Every `fmt::` call site uses `std::format`, and `LoggerService` erases its format arguments with `std::make_format_args`/`std::vformat`, exposing `std::format_args`/`std::string_view` in its public header. `find_package(fmt)` and the `<fmt/base.h>` PCH entry were dropped; `fmt` now reaches the build only transitively through spdlog.
- Build instructions in the README consistently aligned with CI (`.github/workflows/ci.yml`) and `AGENTS.md` (Conan 2, toolchain path under `build/build/Release/generators/conan_toolchain.cmake`, release build with `ccache`, Ninja, output to `bin/`).
- TLS certificates: private key (`server.key`) removed from the repository, generated certificates excluded via `.gitignore`; CI generates certificates itself. Doc (`cfg/certs/README.md`) adjusted accordingly.
- Configuration and examples: path parametrization (e.g. named pipes, configuration), removal of plaintext passwords/credentials and unnecessary personal data from examples/configuration.
- CI/build: adjustments for GCC 13 (Debug/unity), `-Wnull-dereference` only for Clang, `BOOST_UUID_NO_SIMD` in Debug (GCC 13), `CCACHE_DIR` set explicitly; format and lint gates (advisory for lint) documented.
- **`ProcessController`: an unknown process ID now answers 404 instead of 501.** 501 ("Not Implemented") was semantically wrong – the resource does not exist rather than the functionality being missing. Affects `stopProcessDelete`, `terminateProcessDelete`, `restartProcessPut`, `resetProcessPost` and `healthProcessGet`; `swagger.yaml` and `swagger.md` were adjusted in sync. **Breaking change** for clients that checked for 501.
- **Repository moved** to `https://github.com/dkruempe/lintel`. The reason is not the beauty of the name but a security problem: in the old repo the four `refs/pull/{1,3,4,5,6}` of the merged pull requests delivered a plaintext DB password, because GitHub binds these refs to the repository. A newly created repository has no pull requests and therefore no such refs. The rewritten history was adopted **completely** (no discarding of the commits) — after the `filter-repo` pass it no longer contained the password, verified via an object scan of all 4759 blobs with a positive control. The old documentation URL is accordingly now `https://dkruempe.github.io/lintel/`.
- **Internal rename to `lintel`.** The public include directory moved from `src/include/base_library/` to `src/include/lintel/`, so includes read `#include <lintel/core/StartupBuilder.h>`. Include guards renamed from `CPP_BASE_LIBRARY_*` / `CPP_SYSTEM_LIBRARY_*` / `CPP_BASIC_LIBRARIES_*` to `LINTEL_*`. The CMake export namespace `kruempelmann::` became `lintel::`, the CMake project `cpp-base-library` and the placeholder API `myproject_*` / `MYPROJECT_*` became `lintel_*` / `LINTEL_*`, and the package is now `find_package(lintel)` with `lintelConfig.cmake` / `lintelTargets.cmake`. **No breaking change for the CMake target name of the library itself**, which was already `base_library` and remains the source-level target; the visible change is the export namespace and the include path.
- **The repository is now entirely in English.** All German comments, Doxygen blocks, documentation, CMake comments and script messages were translated. Verified by a repository-wide grep for German function words across every tracked file, which returns nothing.


### Fixed
- **`CommandLineService::onPrompt()` called an unqualified `format(...)`.** It only compiled because `using namespace date;` leaked a `format` name into scope; removing the `date` library surfaced the bug. It now calls `std::format("{:%FT%TZ}", std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()))`.
- **Public header `ProcessService.h` only compiled in one include order.** It forward-declared `boost::process::v1` with a hardcoded `inline namespace`, but boost makes `v1` an inline namespace only for `BOOST_PROCESS_VERSION == 1` and a plain nested namespace otherwise. Every consumer that includes a boost/process header before `ProcessService.h` — i.e. any code that starts a process itself — failed with *"inline namespace must be specified at initial definition"*. The declaration now goes through `BOOST_PROCESS_V1_INLINE` from `boost/process/v1/detail/config.hpp`, so both include orders compile. `child` stays an incomplete type, so no boost/process header is pulled into every TU.
- **Every CLI component answered an unknown prompt line with silence.** `CommandLineComponent::onMenu()` returns "handled"; `CommandLineService` prints *"Invalid command"* for `false`. All five components returned `true` unconditionally, so a typo at the prompt produced no output and no error at all. They now return `false`, and the contract is documented at the interface.
- `UserManagementCliComponent::onShowMenu()` printed nothing, while all four peers print a "no submenu available" line. It now prints the same line.
- `scripts/check-format.sh` understands renamed files. A mass rename (such as moving the public include directory) previously made every moved file look brand new, so its legacy violations were counted as a regression and the gate failed. The gate now maps a renamed path to its old path and compares against that. Without this, 204 unchanged files were reported as regressions.
- Include order in `EventBusComponent.cpp`: `<cstring>` sorted incorrectly after the include-path rename.
- **Regression test corrected:** the test for `db::Argument::getValue<TYPE>()` itself created undefined behavior by constructing an enumerator outside the valid range via `memcpy`. Loading an invalid enum value is UB in C++, and the new UBSan job reported exactly that. The test now checks the three valid connection types; the `default:` branch in `getValue<TYPE>()` remains as hardening, but is deliberately untested and justified in the test comment.
- `db::Argument::getValue<TYPE>()`: with an unknown `ConnectionType` the function left a non-`void` function body (undefined behavior). It now throws `db::SQLException`. The `UNDEFINED`, SQLite and PostgreSQL paths are unchanged.
- `Process::inActiveWindow()` and the `operator<<` in `examples/message_queue.cpp` no longer use the thread-unsafe `std::localtime`/`std::ctime`, but `localtime_r` (no shared static buffer anymore). The time format in the example is now deterministic (`%Y-%m-%d %H:%M:%S`) and without an appended newline.
- Examples and public headers cleaned up: no C casts over `void*` anymore in the message queue example, missing receive check secured there, `getValue<TYPE>()` with an explicit `default` branch and the two `operator++(int)` iterators with `const` return type (`CERT DCL21-CPP`). The files touched by the CodeQL fix are free of clang-tidy findings.
- The library's `INSTALL_INTERFACE` no longer contains build-machine absolute paths of the Conan cache paths, includes of `magic_enum`/`rapidjson` come via targets instead of baked paths, and the private target `project_warnings` no longer ends up in the public export.
- `BaseFeature::registerTypes` and `UserManagementCliComponent::onCommand` are now documented at the definition (flow, edge cases, `@param`), not only at the header – the Doxygen workflow checks this as a gate, and CodeQL's `cpp/poorly-documented-function` counts at the definition.
- Four CodeQL alarms dismissed as false positives: three `cpp/ambiguously-signed-bit-field` (the rule reads the scope operator `::` from `boost::interprocess::named_semaphore` as a bitfield declaration – there is not a single bitfield anywhere in the entire `include` tree) and `cpp/cleartext-storage-database` (the hashed, not the plaintext-stored password value is written). CodeQL therefore reports **zero** warnings; only style notes on `note` level remain.
- Duplicate `install(FILES)` entry in the vendored Hypodermic cleaned up (same file listed twice in one line).


- Property repositories: errors that were previously only logged as `LOG_ERROR("{}", exception.what())` now name the concrete operation, the process ID and the number of properties. The deliberate "best effort" behavior is justified in the file header.
- Empty branches in `SharedMemoryPropertyRepository.h` (`cpp/empty-block`) switched to their positive condition; `FilePropertyRepository::save()` and `SharedMemoryPropertyRepository::onMigrate()` are documented as deliberate no-ops.

### Known gaps
- `DatabasePropertyRepository::allOf()` has `instanceName = ".*0"` as default parameter value (`DatabasePropertyRepository.h`) instead of `".*"`. The default therefore becomes `LIKE '%0'`, and every call that omits the instance filter returns nothing. Not fixed here because changing a public default parameter is an API change; noted in `tests/property/DatabasePropertyRepositoryTest.cpp`, where every test passes all four filters explicitly.
- `DELETE /user/delete` with two or more names answers 500: `UserRepository::deleteOf()` calls `transaction.commit()` inside its loop, so the second iteration throws *"SQLite Transaction was finished before with commit or rollback"*. `swagger.yaml` documents 200. The test suite pins the working single-name case and reports the defect in a comment instead of encoding the wrong behaviour.
- `POST /process/start` returns an empty body and the new process ID is only discoverable via `GET /process/processes/{regex}` — the DTO's `id` is ignored and a fresh UUID is generated. `swagger.yaml` documents neither.
- `ADD_HANDLER_METHOD` defaults `Content-Type: application/json` only for GET, so a PUT/POST/DELETE without an explicit `Content-Type` header is answered with 403. `swagger.yaml` does not mention the requirement for bodyless verbs.

### Security
- Sensitive data removed from the repo (TLS private key, plaintext credentials). Reporting path for security vulnerabilities defined in `SECURITY.md` (email instead of a public issue).
- CodeQL scan for C++ enabled (`.github/workflows/codeql.yml`). The three findings with defect character (missing `return`, two `localtime`/`ctime` calls) are fixed; one finding (`cpp/cleartext-storage-database`) was triaged as a false positive – exclusively the hashed password value is stored. The triage table with findings, fixes and counter-checks as well as the breakdown of the remaining style notes is not kept in the repo.

### Removed
- `zlib` dropped from the direct `[requires]` of `conanfile.txt`; it is still pulled in transitively (libpq, OpenSSL, cpp-httplib, Boost).
- `TODO.md`, `ANALYSIS.md` and `ROADMAP.md` removed from the repository. They contained operator-internal work states (among other things open security actions and escalation paths) that should not be part of a public repo. The content remains fully retrievable via the git history (`git show <previous-commit>:ROADMAP.md`).

### Developer experience
- **The (advisory) clang-tidy job is green.** All findings were resolved instead of suppressed: the `Kind` and `LogLevel` enums use `std::uint8_t`, the empty catch blocks in `ProcessService` acknowledge their exception via `(void)ex`, unchecked optionals go through local references, `sscanf` in `Cryption` became `std::from_chars`, and the swappable-parameter warning in `MemorySize` is justified with a `NOLINT`. `Cryption.cpp` was reformatted to the project style (whitespace only).
- **Test coverage of the previously untested modules (364 → 652 test cases, 2643 assertions).** New tests for the four untested bootstrap plugins (`AdminUser`, `Database`, `MessageQueue`, `VirtualGroup`), `LoggerService`, `StartupBuilder`, the four untested CLI components, the four HTTP controllers, `SchedulerService`, `ExecutorService` and the `Database`/`File` property repositories. The `ProcessController` 404 gap listed above is closed without an API change: `ProcessService` turns out to be constructible from four plain values, so all six 404 branches are covered.
- **Missing regression tests for the documented past bugs.** Added: `MessageQueueRepository::allOf()` regex-to-column binding (the sibling of the already-covered `pageOf()`), the empty-path guards of `HttpComponent`, `LoggerComponent` and `SharedMemorySegmentComponent` plus `EnvironmentConfiguration`'s empty override, `PropertyDataDto::getSize()`, and `JsonSerializable::deserialize` with a non-object/non-array body — `HttpBadRequestException` previously appeared nowhere in the suite.
- **The last places where a test could pass while the code was broken are gone.** A `REQUIRE` nested in a conditional in `CommandLineHistoryServiceTest.cpp` (which asserted nothing if the accessor always returned `nullopt`) and one in `ProcessServiceTest.cpp` are now unconditional; two test cases without any assertion got one; the PostgreSQL suite no longer skips silently in CI.
- **`[pg]` tests fail instead of skipping in CI.** `requirePg()` turned an unreachable PostgreSQL server into a Catch2 `SKIP`, which `catch_discover_tests` maps to ctest exit code 4 — a broken `postgres` service would have turned all ten tests into "Skipped" and left the job green, silently returning the PostgreSQL backend to untested. `LINTEL_REQUIRE_PG=1` (set by the build, sanitizer and release jobs) turns that into a failure; a local run without a server keeps skipping.
- **The suite is safe under `ctest -jN`.** `catch_discover_tests` runs one process per test case, so helpers that built temp-file and shared-memory names from a `static` counter without the PID collided as soon as the tests ran in parallel. Six such helpers now include `getpid()`, and the shm objects of `MessageQueueOverflowTest.cpp` are cleaned up by an RAII guard even when a `REQUIRE` aborts the test.
- Test section switched to `ctest --output-on-failure --label-regex unit`; note about the explicit test list in `tests/CMakeLists.txt` added.
- Format check script (`scripts/check-format.sh`) and clang-tidy script (`scripts/check-tidy.sh`) are referenced in the documentation.
- Regression test for the `ConnectionType` error path in `db::Argument::getValue<TYPE>()` added.
- The files touched by the CodeQL fixes were fully `clang-format`ted so that the format regression gate stays green.
- `PropertyRepository.h` and `CommandLineComponent.h` fully re-formatted to the project style (whitespace only, no token changed), which retires their legacy violations instead of adding to them.

## [0.1.0] - 2026-10-03

First release tag `v0.1.0` (commit `86b72f4`). Main topics (from the commit history):
- CI end-to-end: trigger on `master`, GCC/Clang matrix, format gate, release workflow.
- Build system: CMake 3.16+, Conan 2, Ninja, unity builds, PCH, compiler warnings with `-Werror`.
- Core library (`kruempelmann::base_library`), features (HTTP via cpp-httplib, CLI, property, persistence PostgreSQL/SQLite), vendored DI (Hypodermic).
- Tests (Catch2 + trompeloeil), examples, configuration (`cfg/`), Docker profiles.

The commits following this tag (among other things `LICENSE`, `THIRD_PARTY_NOTICES.md`, CI fixes, security and doc audits) are recorded above under `[Unreleased]`.

[Unreleased]: https://github.com/dkruempe/lintel/compare/v0.2.0...HEAD
[0.2.0]: https://github.com/dkruempe/lintel/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/dkruempe/lintel/releases/tag/v0.1.0

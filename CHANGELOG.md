# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- **Secret guard** (`scripts/check-secrets.sh`, blocking CI job `secrets`): blocks plaintext credentials in the working tree. Replacement for `secret_scanning_push_protection`, which cannot be enabled for this repo via API (all fields in `security_and_analysis` are rejected with 422, even with admin rights). Checks in two stages – config formats resp. PEM key blocks, **and** an entropy test on the value; a pure name search is useless, because the repo has dozens of `const char *const PASSWORD = "password";` as JSON key names.
- README: Doxygen badge and a "Documentation" section with the published API reference URL and the local generation command.
- Official OSS documents: `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `SECURITY.md` as well as `CHANGELOG.md` (this document).
- README: CI status badge and MIT license badge, a status note about pre-1.0/API stability as well as notes on tests, the PostgreSQL dependency and certificates (correctly aligned with CI).
- **Working CMake package:** `find_package(base_library CONFIG)` finds the installed package, `base_libraryConfig.cmake` (new) with a `find_dependency` chain and relocatability as well as `base_libraryConfigVersion.cmake` in canonical naming. `LICENSE`, `README.md` and `swagger.yaml` are installed along with it. The installed package works from an arbitrary prefix – verified with an external counter-sample project.
- README: "Examples" section with a table of all examples (what it shows, how to start it), an explanation of `CONFIG_DIRECTORY` and `BOOTSTRAP_CONFIG_NAME`, TLS prerequisites and SQLite vs. PostgreSQL.

### Changed
- Build instructions in the README consistently aligned with CI (`.github/workflows/ci.yml`) and `AGENTS.md` (Conan 2, toolchain path under `build/build/Release/generators/conan_toolchain.cmake`, release build with `ccache`, Ninja, output to `bin/`).
- TLS certificates: private key (`server.key`) removed from the repository, generated certificates excluded via `.gitignore`; CI generates certificates itself. Doc (`cfg/certs/README.md`) adjusted accordingly.
- Configuration and examples: path parametrization (e.g. named pipes, configuration), removal of plaintext passwords/credentials and unnecessary personal data from examples/configuration.
- CI/build: adjustments for GCC 13 (Debug/unity), `-Wnull-dereference` only for Clang, `BOOST_UUID_NO_SIMD` in Debug (GCC 13), `CCACHE_DIR` set explicitly; format and lint gates (advisory for lint) documented.
- **`ProcessController`: an unknown process ID now answers 404 instead of 501.** 501 ("Not Implemented") was semantically wrong – the resource does not exist rather than the functionality being missing. Affects `stopProcessDelete`, `terminateProcessDelete`, `restartProcessPut`, `resetProcessPost` and `healthProcessGet`; `swagger.yaml` and `swagger.md` were adjusted in sync. **Breaking change** for clients that checked for 501.
- **Repository moved** to `https://github.com/dkruempe/lintel`. The reason is not the beauty of the name but a security problem: in the old repo the four `refs/pull/{1,3,4,5,6}` of the merged pull requests delivered a plaintext DB password, because GitHub binds these refs to the repository. A newly created repository has no pull requests and therefore no such refs. The rewritten history was adopted **completely** (no discarding of the commits) — after the `filter-repo` pass it no longer contained the password, verified via an object scan of all 4759 blobs with a positive control. The old documentation URL is accordingly now `https://dkruempe.github.io/lintel/`.
- **Internal rename to `lintel`.** The public include directory moved from `src/include/base_library/` to `src/include/lintel/`, so includes read `#include <lintel/core/StartupBuilder.h>`. Include guards renamed from `CPP_BASE_LIBRARY_*` / `CPP_SYSTEM_LIBRARY_*` / `CPP_BASIC_LIBRARIES_*` to `LINTEL_*`. The CMake export namespace `kruempelmann::` became `lintel::`, the CMake project `cpp-base-library` and the placeholder API `myproject_*` / `MYPROJECT_*` became `lintel_*` / `LINTEL_*`, and the package is now `find_package(lintel)` with `lintelConfig.cmake` / `lintelTargets.cmake`. **No breaking change for the CMake target name of the library itself**, which was already `base_library` and remains the source-level target; the visible change is the export namespace and the include path.
- **The repository is now entirely in English.** All German comments, Doxygen blocks, documentation, CMake comments and script messages were translated. Verified by a repository-wide grep for German function words across every tracked file, which returns nothing.


### Fixed
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
- `TODO.md`, `ANALYSIS.md` and `ROADMAP.md` removed from the repository. They contained operator-internal work states (among other things open security actions and escalation paths) that should not be part of a public repo. The content remains fully retrievable via the git history (`git show <previous-commit>:ROADMAP.md`).

### Developer experience
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

[Unreleased]: https://github.com/dkruempe/lintel/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/dkruempe/lintel/releases/tag/v0.1.0

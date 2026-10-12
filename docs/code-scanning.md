# Code scanning (CodeQL)

How the CodeQL setup in `.github/workflows/codeql.yml` works, and what to do
about an alert. Originally written 07.10.2026 while working through the first
`build-mode: manual` run; updated to the single-mode setup.

## One build mode: `manual`

The workflow runs a single analysis, `build-mode: manual`, and it is blocking.
`manual` is not the action's default name; `full` is rejected with
`Supported build modes are: none, autobuild, manual`.

There used to be a second, fast `build-mode: none` job. It was removed because
without a compilation database CodeQL guesses which declaration a name belongs
to, and for C++ it guesses wrong often enough that the reported coordinates are
fiction. Examples from that job:

| Alert | Reported | Actual content of that line |
|---|---|---|
| `cpp/ambiguously-signed-bit-field`, `ShmConstructors.h:69` | "Bit field `named_semaphore` of type int" | `explicit SegmentSemaphoreGuard(named_semaphore&)` |
| `cpp/missing-return`, `DatabaseBootstrapPluginTest.cpp:42` | function does not return a value | the function returns `std::filesystem::path{...}` |
| `cpp/loop-variable-changed`, `RegexUtils.cpp:73` | loop variable changed | a `for` body |

A real analysis resolves macros and types, so it is the only signal worth
gating on. Without a `none` job there is nothing to compare against, so
`continue-on-error` and the two-mode matrix are gone as well.

The job must build with **GCC 14**: this project's floor is GCC 14 /
libstdc++ 14 because `std::chrono::parse` (used for timestamp deserialization)
does not exist in libstdc++ 13. The ubuntu-24.04 default `g++` is 13, so an
unqualified `CXX: g++` made the build fail at `StringifyService.h:53`; because
the job was `continue-on-error`, the analysis uploaded zero results and every
previous `manual` alert stayed open at its original commit. The workflow now
installs `g++-14` and sets `CC`/`CXX` to it.

## Excluding generated and vendored code

`paths` and `paths-ignore` in the CodeQL configuration file are honoured **only
for analyses that do not build the code** (`build-mode: none`). GitHub's
documentation is explicit that for a built analysis you must exclude
directories in the build itself, not in the configuration.

This was the root cause of the 243 `cpp/include-non-header` alerts per run.
CMake's unity build generates `build/**/Unity/unity_*_cxx.cxx` amalgamations
that `#include` `.cpp` files, and the instrumented build fed those into the
compilation database. `paths-ignore: [build]` did nothing, because the analysis
built the code. The fix is in the configure step:

```bash
cmake ... -DENABLE_UNITY_BUILD=OFF ...
```

No amalgamations, nothing to include, no alerts. `external/` (vendored
Hypodermic) and `benchmarks/` (off by default, `BUILD_BENCHMARKS=OFF`) do not
produce findings under the built analysis either.

## Excluding queries repo-wide

Three quality queries are excluded in `codeql.yml`:

```yaml
query-filters:
  - exclude:
      id: cpp/unused-static-function
  - exclude:
      id: cpp/unused-local-variable
  - exclude:
      id: cpp/unused-static-variable
```

`query-filters` is the category fix for the two mechanisms that produce every
one of these alerts, neither of which is a defect:

- Catch2's `TEST_CASE` and Google Benchmark's `BENCHMARK()` emit a static
  function *plus* a static registrar that references it. CodeQL does not score
  that reference as a use, so every test case - and every helper that only a
  test case calls - is reported as an unused static function. `paths-ignore` on
  `tests/` would stop the whole directory from being scanned; excluding the
  query keeps every other query running there.
- A parameter pack forwarded straight into a variadic helper (`std::forward`,
  `std::make_format_args`) is not seen as a read, so `args` in
  `SchedulerService`, `ExecutorService` and `LoggerService` is reported as an
  unused local/static variable even though dropping it would not compile.

These exclusions are safe because the compiler already covers every real case:
`-Wall -Wunused` with `-Werror` (`cmake/CompilerWarnings.cmake`) turns a
genuinely unused static function or variable into a build failure. A real defect
therefore cannot reach a CodeQL alert, only the macro/template false positives
described above.

## Working with an alert

1. **Check the coordinates against the code first.** If the line does not
   contain what the alert names, it is a heuristic (or stale) result; do not
   change code to satisfy it.
2. **For a real finding, fix the code.** Add a regression test if the class of
   bug is testable.
3. **For a false positive, dismiss it with a reason and a comment.**
   ```bash
   gh api -X PATCH repos/{owner}/{repo}/code-scanning/alerts/<n> \
     -f state=dismissed -f dismissed_reason="false positive" \
     -f dismissed_comment="Why it cannot be a finding."
   ```
   Valid reasons: `false positive`, `won't fix`, `used in tests`, `mitigated`.
   The comment is limited to 280 characters.

### Dismissal vs. suppression comment

A `// codeql[<rule>]` comment in the source and a dismissed alert are different
tools:

- A **suppression comment** belongs next to a sink that must stay, when the
  code around it is correct. `examples/named_pipe_server.cpp` uses one for
  `cpp/path-injection`.
- A **dismissal** belongs to a finding in code you do not want to annotate -
  generated files, vendored code, library macros.

Do not suppress at a *generic* sink. A `// codeql[cpp/cleartext-storage-database]`
at the `sqlite3_bind_text` in `Connection::executeParameters` would silence the
whole rule class at that point, including real leaks of tokens and session
secrets that travel through the same bind.

### Open alerts are a lagging indicator

An alert dismissed today is re-created by the next run, because dismissal is
per-commit and is not carried over automatically. An alert that keeps
reappearing is a signal to fix the *category* - a `query-filters` exclusion, a
build exclusion, or a source-level `// codeql[...]` suppression - not to
dismiss it again.

For the same reason, changing the configuration does not clean up *existing*
alerts: they keep their `open` state until a run completes against the new
config. Alerts from a category that no longer exists (for example the removed
`build-mode: none` job) are never re-triaged and must be dismissed once.

## The cleartext-storage alert

`cpp/cleartext-storage-database` at
`src/core/persistence/sqlite3/Connection.cpp:239` is correct about the sink and
wrong about the value.

`sqlite3_bind_text` binds `param` from `UserRepository::changePasswordOf`. Both
call paths hash before the value gets here - `Cryption::hashOf` in
`AuthService.cpp:94` (hash upgrade at login) and in `UserController.cpp:458`
(password change) - and `Cryption::verifyOf` compares via the same hash. The
rule does not model the hashing step, so it cannot see it in the data-flow
graph.

Whether the *stored* form should be encrypted anyway is a deployment decision
(SQLCipher, volume encryption), not something a library bind site can decide, so
it is not a code fix here. Dismissed rather than suppressed, for the reason in
the section above.

## Alert counts

| Point in time | Open | Composition |
|---|---|---|
| `build-mode: none` only | 33 | 1 high, 32 note - mostly phantom coordinates |
| First `manual` run (broken build, 0 results) | 300 | stale: 285 `unused-static-function` (Catch2), 90 `include-non-header` (unity amalgamations) |
| After query-filters + unity off | ~10 real findings | 4 `large-parameter`, 3 `loop-variable-changed`, 1 `long-switch`, 1 `array-in-interface`, 1 `cleartext-storage-database` (FP) |

The ~10 remaining notes are real and were fixed in code rather than dismissed:
the `large-parameter` call sites pass by `const &`, the `loop-variable-changed`
loops in `RegexUtils.cpp`/`Command.cpp` use `while` with an explicit advance,
the long `case 27` of `InputService::onRead` moved into
`InputService::readEscapeSequence`, and `StartupBuilder::with` declares
`char **argv` instead of `char *argv[]`.

## Reproducing the analysis locally

There is no local CodeQL setup. To reproduce roughly what the action builds:

```bash
cmake -S . -B build/build/Release \
  -DCMAKE_TOOLCHAIN_FILE=build/build/Release/generators/conan_toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -DENABLE_UNITY_BUILD=OFF -G Ninja
cmake --build build/build/Release --parallel "$(nproc)"
```

The compile database that makes the difference is `compile_commands.json`.
The `-DENABLE_UNITY_BUILD=OFF` is what keeps the generated amalgamation files
out of it; without it, `cpp/include-non-header` reports every `.cpp` include in
the amalgamation.

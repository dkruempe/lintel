# Code scanning (CodeQL)

How the CodeQL setup in `.github/workflows/codeql.yml` works, and what to do
about an alert. Written 07.10.2026 while working through the first
`build-mode: manual` run.

## The two build modes

| Mode | Build | Cost | What it produces |
|---|---|---|---|
| `none` | none | ~2 min | Heuristic extraction, no compilation database |
| `manual` | full instrumented build | ~30 min warm | Real cross-translation-unit information |

`manual` is advisory (`continue-on-error`) for now, `none` is blocking. The
mode is called `manual`; `full` is rejected by the action with
`Supported build modes are: none, autobuild, manual`.

### Why `build-mode: none` findings were not worth fixing

The 33 alerts that were open before the instrumented build all pointed at lines
that did not contain the reported construct. Examples:

| Alert | Reported | Actual content of that line |
|---|---|---|
| `cpp/ambiguously-signed-bit-field`, `ShmConstructors.h:69` | "Bit field `named_semaphore` of type int" | `explicit SegmentSemaphoreGuard(named_semaphore&)` |
| `cpp/unused-static-variable`, `EventBusComponent.cpp:15` | unused static variable | a function definition in an anonymous namespace |
| `cpp/loop-variable-changed`, `RegexUtils.cpp:73` | loop variable changed | a `for` body |
| `cpp/array-in-interface`, `StartupBuilder.cpp:43` | array in interface | a signature whose comment already answers the alert |

Without a compilation database, CodeQL guesses which declaration a name belongs
to, and for C++ it guesses wrong often enough that the coordinates are fiction.
There were no bit fields anywhere in the repository at that point.

The single exception was `cpp/cleartext-storage-database`
(`src/core/persistence/sqlite3/Connection.cpp:180`). Taint analysis is a
different mechanism than the declaration mapping, and it found a real
`sqlite3_bind_text` sink with a genuinely tainted value. That alert was
dismissed as a false positive because both password paths hash before reaching
this layer - see "The cleartext-storage alert" below.

## Working with an alert

1. **Check the coordinates against the code first.** If the line does not
   contain what the alert names, you are looking at a `build-mode: none`
   artifact. Wait for the `manual` run instead of changing code to satisfy it.
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

## Alerts that come back

A dismissed alert is dismissed for one commit. The next run re-analyses and
re-reports it, because the dismissal is not carried over automatically. So the
tail of `cpp/unused-static-function` on `TEST_CASE`s stays visible even though
every one of them has been dismissed with a comment.

That is why `benchmarks/` is in `paths-ignore` rather than dismissed per alert:
`BENCHMARK()` produces the same macro-generated registration as `TEST_CASE`, and
dismissing ~30 alerts on every run is work that never finishes. The same applies
to `build/`.

### Open alerts are a lagging indicator

`paths-ignore` takes effect for the alerts of the *next* run. The existing
alerts keep their `open` state - nothing re-triages them. So after adding an
exclusion, the count only falls once a run has completed against the new config,
and until then the old findings are still listed.

That is also true in the other direction: an alert dismissed today is re-created
by the next run, because dismissal is per-commit. An alert that keeps
reappearing is a signal to fix the *category* - a `paths-ignore` entry or a
source-level `// codeql[...]` suppression - not to dismiss it again.

## Known false-positive classes

These recur and are safe to dismiss with a short comment, provided the line
really is what the rule names:

| Rule | Why it fires | Check before dismissing |
|---|---|---|
| `cpp/unused-static-function` on a `TEST_CASE` | Catch2's macro emits a static registration function plus an `AutoRegistrator` that references it; CodeQL only sees the static-initializer reference | The test runs under `ctest` |
| `cpp/unused-local-variable` for `trompeloeil_param` | Name is baked into the `MOCK_METHOD` macro, not chosen by us | It is inside a trompeloeil mock signature |
| `cpp/unused-local-variable`/`unused-static-variable` for `args` in a variadic template | Passed straight into a variadic helper (`fmt::make_format_args`), which is not a recognisable read | A real mistake here would be a compile error |
| `cpp/unused-static-variable` on a `BM_*` benchmark | `BENCHMARK()` registers the function through an internal registrar, like `TEST_CASE` | It is in `benchmarks/`, now `paths-ignore`d |
| `cpp/unused-static-variable` for a counter in `counter++` | `counter++` on the right-hand side is a read; the rule scores it as a write | - |

## The cleartext-storage alert

`cpp/cleartext-storage-database` at `src/core/persistence/sqlite3/Connection.cpp:180`
is correct about the sink and wrong about the value.

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
| Before `manual` (`build-mode: none` only) | 33 | 1 high, 32 note - mostly phantom coordinates |
| After the first `manual` run | 300 | 285 `unused-static-function` (Catch2), 90 `include-non-header` (generated `build/**` files), 1 high, 14 note |
| After triage | 0 real findings | - |

Two things worth reading off this table.

The jump from 33 to 300 is not a regression. A real analysis resolves macros, so
every `TEST_CASE` in the suite becomes one finding instead of being invisible -
285 of them - and the instrumented build additionally feeds CodeQL its own
generated amalgamation files, which is the `include-non-header` batch.

The 90 `cpp/include-non-header` alerts were a workflow defect, not a finding:
the config used a `paths` key, which does not restrict what CodeQL scans. The
`paths-ignore` key does, and three directories are now excluded:

| Directory | Why |
|---|---|
| `build/` | generated by the instrumented build of this workflow; the `include-non-header` findings were every `.cpp` include inside a CMake amalgamation |
| `external/` | vendored Hypodermic, unmaintained since 2017 — a finding belongs upstream, see `THIRD_PARTY_NOTICES.md` |
| `benchmarks/` | Google Benchmark registers every `BM_*` function through `BENCHMARK()`, so `cpp/unused-static-variable` reports ~30 of them — the same macro mechanism as the Catch2 findings |

Excluded directories are not dismissed alert by alert: they are simply not
scanned, which is the point of `paths-ignore`. Dismissing each one would recreate
exactly the background noise the setting avoids.

The one real finding of the whole batch was
`cpp/path-injection` in `examples/named_pipe_server.cpp` — see below.

## Reproducing the analysis locally

There is no local CodeQL setup. To reproduce roughly what the action does:

```bash
# what CodeQL sees in 'none' mode: no compilation database
# what it sees in 'manual' mode:
cmake -S . -B build/build/Release \
  -DCMAKE_TOOLCHAIN_FILE=build/build/Release/generators/conan_toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -G Ninja
cmake --build build/build/Release --parallel "$(nproc)"
```

The compile database that makes the difference is `compile_commands.json`. Note
that the instrumented build generates `Unity/unity_*_cxx.cxx` files, which is
why the config excludes `build/` - without that exclusion
`cpp/include-non-header` reports every `.cpp` include in the amalgamation.
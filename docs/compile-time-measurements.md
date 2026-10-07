# Compile-time measurement log (reference)

> Reference log for compile-time optimization (P2: "version the measurement scripts").
> Goal: **reproducible** numbers for the compile time of `lintel`,
> comparable across commits.

## Environment & ground rules

This machine is a **production machine with foreign processes running on it**.
Therefore the following applies to **all** measurements in this document:

- **Single-core** (`--jobs 1`), plus `--nice` (priority `nice -n 10`).
  Deliberately never more than **one** compiler process is started, so that the
  services running in parallel are not slowed down.
- Load average (1/5/15 min) is recorded before/after every run, so that
  outliers caused by external load are recognizable.
- The source tree is **never** modified; only `build/` is created resp. emptied.
- Scripts: `scripts/measure-compile-times.sh`, `scripts/measure-header-weight.sh`
  (both are versioned – `.gitignore` allows `scripts/*.sh`).

Comparison states are set up via a worktree of the previous commit:

```bash
git worktree add --detach /tmp/opencode/baseline <commit>
```

## Measurement tools

### 1. Full build (configure + build + ccache statistics)

```bash
# Full reference run: empty ccache + incremental delta in one run
scripts/measure-compile-times.sh --jobs 1 --nice --cold --incremental
```

Output: wall-clock time of the build, host load before/after, `ccache -s`, then
`touch src/include/lintel/core/utils/StringUtils.h` + rebuild time.
`--cold` and `--incremental` are combinable (previously `--incremental` was
an exclusive mode and swallowed `--cold`).

### 2. Header weight (single-TU probe)

Preprocessed size and `-fsyntax-only` time of a typical `LOG_*` consumer TU:

```bash
scripts/measure-header-weight.sh --build-dir build/nonunity --header LoggerService.h
```

Flags (`-I`/`-isystem`/`-D`/`-std`) are extracted from `compile_commands.json`
(TU `LoggerService.cpp`, Non-Unity build without PCH) and are therefore identical to the
real library environment. The value `TOTAL` from `-ftime-report` is **CPU time**
(user + sys), not wall-clock – so it is also comparable under external load;
`max resident set size` is the fourth column.

---

## Record 1: `LoggerService.h` refactor (fmt + forward declaration)

Before: `ee55c8d` · After: `fcebfa7` + `<fmt/base.h>` in the PCH
Goal: convert `LoggerService.h` to `<fmt/base.h>` + `namespace spdlog { class logger; }`
and eliminate the `log()` template following the fmt pattern
(`fmt::format` → `fmt::vformat`: the template now only boxes with
`fmt::make_format_args`, everything else happens in the non-template `logImpl()`).
spdlog now only appears in `LoggerService.cpp`.

### Header weight (probe TU with `LOG_INFO`/`LOG_ERROR`, GCC `-fsyntax-only`)

| Metric                         | before (`ee55c8d`) | after            | Δ           |
|--------------------------------|--------------------|------------------|-------------|
| Preprocessed lines             | 104.378 | 84.450 | **−19,1 %** |
| Preprocessed bytes             | 2.784.245 | 2.262.935 | −18,7 % |
| CPU time `-fsyntax-only` (TOTAL) | 4,42–4,43 s | 2,02 s | **−54,4 %** |
| max. resident set size        | 319 MB | 122 MB | **−61,8 %** |

Reference: **`<spdlog/logger.h>` alone** = 97.735 lines / 2.606.128 bytes /
4,42 s / 312 MB. The old public header was therefore almost entirely spdlog
(104.378 lines in total); after the refactoring the format-argument weight
(`<fmt/base.h>`, 2,6 k lines) is the remainder.

### Full build (`measure-compile-times.sh --jobs 1 --nice --cold --incremental`)

| State                 | Mode                     | Wall-Clock | Host load 1/5/15 before → after | ccache (cacheable) |
|-----------------------|--------------------------|-----------:|-----------------------------|--------------------|
| before (`ee55c8d`)    | cold (ccache cleared)    | **511 s** | 2,75 3,05 3,14 → 3,18 3,13 3,16 | 0/14 hits |
| after                 | cold (ccache cleared)    | **416 s** | 3,05 3,09 3,11 → 3,39 3,23 3,16 | 0/14 hits |
| after                 | warm (ccache populated)  | 243 s | 2,94 2,87 3,10 → 3,45 3,12 3,14 | 14/14 hits (100 %) |
| before (`ee55c8d`)    | incremental (`StringUtils.h`) | **278 s** | 3,18 3,13 3,16 → … | – |
| after                 | incremental (`StringUtils.h`) | **245 s** | 3,39 3,23 3,16 → 3,80 3,48 3,28 | – |

Result: full build **−95 s (−18,6 %)**, incremental rebuild **−33 s (−11,9 %)**.
(Historical comparison value from 02.09.2026 with full parallelism:
`tests/unity_0` with 470 k lines / 71,8 s, overall approx. 3,86 Mio. lines – not
directly comparable, because the number of jobs differs.)

### Full build with parallelism (4 jobs, without `nice`) – CI-like

| State               | Mode                         | Wall-Clock | Host load before → after   | ccache |
|---------------------|-----------------------------|-----------:|-----------------------|--------|
| before (`ee55c8d`) | cold (ccache cleared)     | 164 s | 3,26 2,49 2,24 → 4,99 3,90 2,84 | 0/14 |
| after            | cold (ccache cleared)     | **134 s** | 2,97 3,81 2,97 → 4,69 4,31 3,27 | 0/14 |
| after            | incremental (`StringUtils.h`) | **82 s** | 4,69 4,31 3,27 → 4,99 4,55 3,45 | – |
| before (`ee55c8d`) | incremental (`StringUtils.h`) | 92 s | – | – |

Result: **−30 s (−18,3 %)** full build, **−10 s (−10,9 %)** incremental – the same
relative improvement as in the single-core run, so not a measurement artifact of the
number of jobs. Host load rises accordingly to ~5 (4 compilers on 4 CPUs);
for runs next to active services use the single-core numbers above.

### Reproduction

```bash
git worktree add --detach /tmp/opencode/baseline ee55c8d
cp scripts/measure-compile-times.sh scripts/measure-header-weight.sh /tmp/opencode/baseline/scripts/
# in the worktree:
nice -n 10 scripts/measure-compile-times.sh --jobs 1 --nice --cold --incremental
nice -n 10 scripts/measure-header-weight.sh --build-dir build/build/Release --header LoggerService.h
# identical in the main tree, so both sides have the same setup
# (CI-like variant: --jobs 4 without --nice)
```

---

## Record 2: Boost decoupling (headers without Boost)

Before: `250ff06` · After: working state (Boost suffix refactor)
Goal: remove heavy Boost headers from public headers. Affected were
`boost/process`, `boost/interprocess` (message queue, event bus, file lock) and
the shared memory constructors. Ground rule: **Boost types now appear only in
implementation files or in deliberately Boost-bound headers**, never in a
header that feature consumers include.

### What was changed

| Area | Solution | API break |
|---------|--------|-----------|
| `ProcessResourceReader`, `ProcessInfo`, `ProcessInfoDto`, `ProcessService` | `boost::process::v1::pid_t` → `pid_t` (`<unistd.h>`), `child` only forward-declared | `getProcessId()` now returns `pid_t` by value |
| `MessageQueue<T>` | non-template core `MessageQueueCore` (Pimpl), Boost in `MessageQueueCore.cpp` | no (template API unchanged) |
| `EventBus` | `ShmSegmentAccessor` (Boost-free, 1 pointer) + `ShmSegmentAccessor::Allocator` interface; Boost-bound in `BoostSegmentAllocator.h` | constructor takes `ShmSegmentAccessor` instead of `segment_manager*` |
| `SingleInstanceBootstrapPlugin` | `Lock` class (Boost `file_lock`) as private Pimpl in the `.cpp` | no |
| `SharedMemoryService` | Boost-bound free functions in `shm::` (`ShmConstructors.h`), segment state in `SharedMemorySegmentHandle` | **yes**: `constructX()` → `shm::constructX(service, …)`, `ShmString` → `shm::String`, `getSegmentManager()` → `shm::segmentManagerOf()` |

### Header weight (GCC `-fsyntax-only`, without PCH, `build/nonunity`)

| Header | before lines / CPU | after lines / CPU | Δ lines | Δ CPU |
|--------|--------------------|----------------------|---------|-------|
| `ProcessInfo.h` | 144.195 / 3,28 s | 71.136 / 1,44 s | −50,7 % | −56,1 % |
| `ProcessResourceReader.h` | 139.902 / 3,08 s | 12.995 / 0,23 s | −90,7 % | −92,5 % |
| `ProcessService.h` | 187.623 / 5,28 s | 124.055 / 3,43 s | −33,9 % | −35,0 % |
| `ProcessInfoDto.h` | 152.060 / 3,56 s | 79.810 / 1,66 s | −47,5 % | −53,4 % |
| `MessageQueue.h` | 143.244 / 3,23 s | 62.930 / 1,27 s | −56,1 % | −60,7 % |
| `EventBus.h` | 89.532 / 1,68 s | 35.125 / 0,66 s | −60,8 % | −60,7 % |
| `SingleInstanceBootstrapPlugin.h` | 83.934 / 2,08 s | 76.335 / 1,96 s | −9,0 % | −5,8 % |
| `SharedMemoryService.h` | 168.813 / 4,30 s | 114.504 / 3,08 s | −32,2 % | −28,4 % |

`SharedMemoryService.h` is **Boost-free** (no `boost/interprocess` occurs in the
preprocessor output anymore); the remaining 114 k lines come from
spdlog/fmt, Hypodermic and magic_enum – a topic of its own.

`SingleInstanceBootstrapPlugin.h` and `EventBus` consumers remain
partially heavy because they still include `SharedMemoryRepository.h`, which
mentions `shm::String`/`shm::Map`/`shm::Vector` in its
template API as a generic abstraction.

### Boost headers per translation unit (`ninja -t deps`, Non-Unity)

| Boost header | before | after | remaining users |
|--------------|--------|---------|---------------------|
| `boost/process/v1/child.hpp` | 14 | 2 | `ProcessService.cpp`, `ProcessServiceTest.cpp` |
| `boost/interprocess/ipc/message_queue.hpp` | 9 | 2 | `MessageQueueCore.cpp`, example `message_queue.cpp` |
| `boost/date_time/posix_time/posix_time.hpp` | 8 | 1 | `MessageQueueCore.cpp` |
| `boost/interprocess/sync/file_lock.hpp` | 3 | 1 | `SingleInstanceBootstrapPlugin.cpp` |
| `boost/interprocess/managed_mapped_file.hpp` | 28 | 22 | all via `ShmConstructors.h` resp. `SharedMemoryRepository.h` |

### Deliberately not implemented

- **Conan `without_options`** (`without_process`, `without_filesystem`, …): the
  headers are installed anyway, the effect would be install time/size, not
  compile time. `BOOST_PROCESS_USE_STD_FS` (Boost.Process v1) was checked and
  brings no measurable gain (`child.hpp` 138.918 → 138.914 lines).

### Reproduction

```bash
cmake -S . -B build/nonunity -DENABLE_PCH=OFF -DENABLE_UNITY_BUILD=OFF
cmake --build build/nonunity --parallel $(nproc)
nice -n 10 scripts/measure-header-weight.sh --build-dir build/nonunity --header EventBus.h
```

---

## Record 3: Hypodermic (vendored DI container)

Before: `250ff06` · After: working state (Hypodermic patch)
Trigger: waiting time in CLion on changes under `src/features/`.
`Hypodermic/ContainerBuilder.h` was the heaviest single header block of the
project: **151.116 preprocessed lines and 2.983 Boost files** alone,
although the container only has 95 small headers. For comparison `httplib.h` = 20,6 k
lines. This was amplified by the fact that `Feature.h`/`Features.h` pull the container in
via `#include` into every TU that touches a feature type.

### What was changed

| File (vendored) | before | after | Rationale |
|-----------------|--------|---------|------------|
| `TypeInfo.h` | `<boost/algorithm/string.hpp>`, `<regex>` | `<string>` + inline loop | `<regex>` is only used in the `#else` branch (non-GNU) → dead include; `algorithm/string.hpp` (94,8 k lines) alone for `replace_all_copy(name, "::", ".")` at line 30 |
| `ComponentContext.h` | `<boost/range/adaptor/reversed.hpp>` | `rbegin()`/`rend()` | 62,2 k lines for a reverse iteration |
| `ResolutionContainer.h` | `<boost/range/sub_range.hpp>` | deleted | 62,4 k lines, **never used** anywhere in the tree |
| `RegistrationActivator.h`, `IRegistrationDescriptor.h` | `<boost/signals2.hpp>` | `Hypodermic/Signal.h` | 109,0 k lines, the most expensive single include of the project |
| `CMakeLists.txt` | `target_include_directories(... INTERFACE ...)` | `... SYSTEM INTERFACE ...` | third-party code deserves to be treated as a system header |
| `LogLevel.h` | `(int) logLevel` | `static_cast<int>(logLevel)` | otherwise a `-Werror` blind spot |

In addition, on the project side: `Feature.h`/`Features.h` forward-declare
`Hypodermic::Container`/`ContainerBuilder` (both are only used as `&` resp. in
`std::shared_ptr`). IWYU consequence: 7 implementation files now include the
Hypodermic headers themselves (`StartupBuilder.cpp`, `BaseFeature.cpp`,
`CommandLineFeature.cpp`, `HttpFeature.cpp`, `PropertyFeature.cpp`,
`examples/main.cpp`, `examples/worker.cpp`) – four of them were masked in the Unity build
by neighbouring TUs.

### `Hypodermic/ContainerBuilder.h` in isolation

| | before | after | Δ |
|---|--------|---------|---|
| preprocessed lines | 151.116 | 74.438 | **−50,7 %** |
| Bytes | 4.166.824 | 1.952.702 | **−53,1 %** |
| Boost files in the graph | 2.983 | 0 | −100 % |

### `-fsyntax-only` per affected TU (Non-Unity flags of the TU, best-of-2)

| TU | before | after | Δ |
|----|--------|---------|---|
| `StartupBuilder.cpp` | 3,77 s | 2,29 s | −39,3 % |
| `examples/worker.cpp` | 5,48 s | 3,63 s | −33,8 % |
| `VirtualGroupBootstrapPlugin.cpp` | 2,88 s | 2,08 s | −27,8 % |
| `PropertyFeature.cpp` | 10,09 s | 8,19 s | −18,8 % |
| `CommandLineFeature.cpp` | 10,56 s | 8,59 s | −18,7 % |
| `HttpFeature.cpp` | 10,06 s | 8,18 s | −18,7 % |
| `BaseFeature.cpp` | 8,42 s | 7,10 s | −15,7 % |
| `examples/main.cpp` | ~6,06 s | 4,88 s | −19,4 % (≈) |
| **Total** | **57,3 s** | **44,9 s** | **−21,6 %** |

`examples/main.cpp` is only approximately reliable: the `git show HEAD:` before-state
does not build against the WIP headers of the feature work running in parallel, only
the headers changed by this record were substituted there.

### The IDE case: a TU that only includes `Feature.h`

The actually interesting case for CLion – e.g. a TU that only
calls `builder.addFeature<HttpFeature>()` and never touches the container:

| | before | after | Δ |
|---|--------|---------|---|
| preprocessed lines | 153.570 | 55.711 | **−63,7 %** |
| `-fsyntax-only` | 2,84 s | 0,56 s | **−80,3 %** |

### Across all TUs

218 Non-Unity TUs, sum of the preprocessed lines: 24.150.450 → 23.658.075
(**−2,0 %**). The full build gains little, because the 8 affected TUs only make up
a small part of the volume; the relevant goals are (a) the
indexing time of the 8 TUs (−21,6 %) and (b) the fact that a change to
`Feature.h`/`Features.h` in CLion no longer drills into any TU that does not use the
DI container at all.

### Side finding: `-isystem` shielded from `-Werror`

While forward-declaring, `LogLevel.h:31` failed as `-Werror=old-style-cast`, although
the cast has existed all along. Cause: `src/include` is marked as `-isystem`,
Hypodermic's own include dir is not. So anyone reaching `LogLevel.h` via `Feature.h`
had all warnings in it suppressed; anyone including it directly did not.
The forward declarations made exactly this latent landmine visible. As a
fix, `SYSTEM` on the Hypodermic target (correct handling of third-party code) **plus**
`static_cast`, so the header stays warning-free even as an `-I` header.

### Tests

`tests/external/HypodermicSignalTest.cpp` (12 cases) pins down the semantics of the
signal replacement that the container needs: call order, empty slots,
`disconnect_all_slots`, `Connection::disconnect` (incl. idempotency),
multiple arguments, as well as both reentrancy cases – a slot disconnects during the
emit itself (the pattern from `ContainerBuilder`) and a slot connects
during the emit. Without the reentrancy case a naive implementation
would have slipped through: the first version iterated a copy of the slot list and still
called already-disconnected slots.

Result: **353/353 green in the Unity and in the Non-Unity build.**

### Reproduction

```bash
cmake -S . -B build/nonunity -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build/nonunity --parallel $(nproc)

# weight of the container in isolation
echo '#include "Hypodermic/ContainerBuilder.h"' | c++ -x c++ -E - \
  $(python3 -c "
import json;d=json.load(open('build/nonunity/compile_commands.json'))
print(next(e['command'] for e in d if 'StartupBuilder' in e['file']))") | wc -l
```

---

## ccache notes

- The local cache is small (cache size 0,06–0,45 % of 5 GB): the Unity TUs
  are so large that a single cache entry barely fills the cache – the hit rate is
  locally **100 %** as soon as only the same TUs are rebuilt.
- For the first CI run of a commit the rate is naturally low
  (0 hits). A meaningful value (> 70 %) requires at least two
  consecutive runs with different but similarly large
  changes – the cache key has already been switched to `hashFiles(...)` (content) instead of
  `github.sha`, so that second runs hit the same key.
- `CCACHE_BASEDIR` points at `${{ github.workspace }}` (GH runner) resp.
  `/workspace` (Docker), `CCACHE_COMPRESS=1`, `CCACHE_MAXSIZE=500M`.
- **`CCACHE_DIR` has been set explicitly since 03.10.2026** (GH runner:
  `${{ github.workspace }}/.ccache`, Docker: `/root/.ccache`): ccache 4.x and later
  otherwise uses `~/.cache/ccache`, while `actions/cache` wanted to save the path
  expected in `~/.ccache`. The result was a no-op cache –
  the job logs reported `Path Validation Error: Path(s) specified in the action
  for caching do(es) not exist, hence no cache is being saved`, and all runs
  up to and including release `v0.1.0` were objectively cold (0 hits).
  **Hit rate measurements of the CI are only meaningful from the first run after this
  fix.**
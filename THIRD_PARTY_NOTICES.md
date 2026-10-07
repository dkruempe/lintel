# Third-Party Notices

`lintel` itself is licensed under the MIT License; see [`LICENSE`](LICENSE).
This document lists the third-party components that are redistributed as part of a build or
release of this repository, together with their licenses.

## How this list was produced

- Versions are the ones pinned in [`conanfile.txt`](conanfile.txt) (Conan 2, CMake layout).
- The vendored component comes from [`external/Hypodermic/CMakeLists.txt`](external/Hypodermic/CMakeLists.txt).
- Every license was **read from the license files inside the Conan packages** in the local cache
  (`~/.conan2/p/<pkg>/licenses/`), not from memory or from secondary sources. Project URLs were
  taken from files shipped in the same packages (headers, CMake files, license texts).
- Entries marked **to be reviewed** were ambiguous or incomplete in the package itself; the
  evidence is named so the question can be closed quickly.

All licenses listed below are permissive (MIT, BSD, Boost, zlib, Apache-2.0) or a public-domain
dedication, i.e. compatible with distributing this project under MIT. Two licenses carry
redistribution duties beyond "keep the copyright notice": Apache-2.0 (`openssl`, `benchmark`) and
the JSON License part of `rapidjson`. See "Obligations" below.

## Runtime and build dependencies

| Component | Version | License | Project URL | Role |
| --- | --- | --- | --- | --- |
| Hypodermic | vendored (upstream 2017) | MIT | https://github.com/TheranBryant/Hypodermic | DI container, vendored in `external/Hypodermic` |
| fmt | 12.1.0 | MIT | https://github.com/fmtlib/fmt | header-only formatting |
| spdlog | 1.17.0 | MIT | https://github.com/gabime/spdlog | logging backend (header-only) |
| tinyxml2 | 11.0.0 | zlib | https://github.com/leethomason/tinyxml2 | XML property files |
| magic_enum | 0.9.7 | MIT | https://github.com/Neargye/magic_enum | enum reflection |
| cpp-httplib | 0.47.0 | MIT | https://github.com/yhirose/cpp-httplib | HTTP server/client (`with_openssl=True`) |
| rapidjson | cci.20250205 | MIT + BSD-3-Clause (msinttypes) + JSON License (`bin/jsonchecker` only) | https://github.com/Tencent/rapidjson | JSON parsing |
| date | 3.0.4 | MIT (per file) | https://github.com/HowardHinnant/date | date/time types (`std::chrono` tz) |
| boost | 1.91.0 | BSL-1.0 | https://www.boost.org | `boost::interprocess`, `boost::filesystem`, UUIDs |
| libpq | 17.7 | PostgreSQL License (BSD-3-Clause text) | https://www.postgresql.org | PostgreSQL client library |
| sqlite3 | 3.53.3 | Public domain (dedication) | https://www.sqlite.org | embedded database |
| openssl | 3.6.3 | Apache-2.0 | https://www.openssl.org | TLS for cpp-httplib |
| zlib | 1.3.2 | zlib | https://www.zlib.net | transitive: required by `openssl` and `libpq` |
| lz4 | 1.9.4 | BSD-2-Clause (LZ4) | https://github.com/lz4/lz4 | transitive: runtime dependency of `libpq`, not pinned in `conanfile.txt` |
| Catch2 | 3.15.2 | BSL-1.0 | https://github.com/catchorg/Catch2 | tests only (`tests/`) |
| trompeloeil | 49 | BSL-1.0 | https://github.com/rollbear/trompeloeil | test mocks, tests only |
| benchmark | 1.9.5 | Apache-2.0 | https://github.com/google/benchmark | optional benchmark suite (`BUILD_BENCHMARKS`, default `OFF`) |

Not redistributed: the build-time tool packages that Conan uses to compile `libpq` from source
(`meson`, `ninja`, `pkgconf`, `flex`, `bison`, `m4`, `gnu-config`). They never end up in a binary
or source release of this project.

## Vendored source: Hypodermic

`external/Hypodermic` contains a vendored copy of Hypodermic (last synced from upstream in 2017,
unmaintained since). It carries its own license and attribution files, which take precedence over
this table:

- `external/Hypodermic/LICENSE` — MIT, Copyright (c) 2016 Theran Bryant
- `external/Hypodermic/NOTICE` — upstream provenance and the list of local modifications

## Copyright holders

Taken verbatim from the license files in the packages:

| Component | Copyright |
| --- | --- |
| Hypodermic | (c) 2016 Theran Bryant |
| fmt | (c) 2012 - present, Victor Zverovich and {fmt} contributors |
| spdlog | (c) 2016 - present, Gabi Melman and spdlog contributors |
| magic_enum | (c) 2019 - 2024 Daniil Goncharov |
| cpp-httplib | (c) 2017 yhirose |
| tinyxml2 | not stated in the license text; author "Lee Thomason" per `include/tinyxml2.h` |
| date | (c) 2015, 2016, 2017 Howard Hinnant (per file, the project licenses each file individually) |
| libpq | portions (c) 1996-2025 PostgreSQL Global Development Group; portions (c) 1994 The Regents of the University of California |
| sqlite3 | none — the author disclaims copyright to the source code (public-domain dedication) |
| zlib | (C) 1995-2026 Jean-loup Gailly and Mark Adler |
| lz4 | (c) 2011-2020, Yann Collet |
| rapidjson | (C) 2015 THL A29 Limited (a Tencent company) and Milo Yip; bundled msinttypes (c) 2006-2013 Alexander Chemeris; `bin/jsonchecker` (c) 2002 JSON.org |
| Catch2, trompeloeil, boost, openssl, benchmark | not stated as a single holder in the packaged license text (BSL-1.0 / Apache-2.0 refer to "the copyright notices in the Software") |

## Obligations when redistributing binaries or sources

- **Every entry**: reproduce the upstream copyright notice and license text. All Conan packages
  keep them in `licenses/`, and Conan installs them next to the binaries; `THIRD_PARTY_NOTICES.md`
  and `LICENSE` must be shipped alongside.
- **Apache-2.0 (`openssl`, `benchmark`)**: in addition, mark modified files as changed and carry
  the Apache-2.0 `NOTICE` content if the upstream project provides one.
- **rapidjson**: the MIT part applies to the library. `bin/jsonchecker/` is under the JSON License;
  the Conan package ships only the headers, so it is not affected. `msinttypes/` is BSD-3-Clause.
- **date**: license is per file, so any vendored copy must keep the per-file headers intact.

## To be reviewed

- **`openssl/3.6.3` package ships Perl sources under a copyleft license.** The Conan package
  contains `licenses/external/perl/Text-Template-1.56/LICENSE`: "copyright (c) 2013 by Mark Jason
  Dominus … under the same terms as the Perl 5 programming language system itself" (GPL-1+ or
  Artistic-1.0). These are OpenSSL build/test helpers. Whether they are part of an actual
  redistribution has to be decided per artifact — check what the release tarball actually
  contains.
- **`benchmark` Apache-2.0 copyright holder is unset.** The packaged `LICENSE` still has the
  unfilled `[yyyy] [name of copyright owner]` placeholder in its appendix, so no single holder can
  be named here.
- **Copyright holders for the BSL-1.0 packages (Catch2, trompeloeil, boost)** are not derivable
  from the packaged license text; they live in the upstream repositories' `AUTHORS`/`COPYING`
  files, which Conan does not package. Nothing here is legally relevant for redistribution (BSL-1.0
  only requires keeping the notices), but a stricter attribution audit would read them upstream.
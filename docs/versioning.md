# Versioning and API stability

`lintel` follows [Semantic Versioning](https://semver.org/) once it reaches `1.0.0`.
Before `1.0.0` there is **no** compatibility guarantee: a minor release may rename or
remove public API, and that is called out as a breaking change in the changelog.

## The single version source

`LINTEL_VERSION` in the top-level `CMakeLists.txt` is the only place the version is
defined. Everything else is derived from it or verified against it:

- `VERSION_MAJOR` / `VERSION_MINOR` / `VERSION_PATCH` in the generated
  `lintel/config.h`.
- `info.version` in `swagger.yaml`, checked at configure time; `swagger.md` is checked
  by `scripts/check-swagger.sh`.
- The git release tag `vX.Y.Z`, checked against `LINTEL_VERSION` by the `release`
  workflow before anything is published.

There is no separate "API version" that could drift from the library version.

## What counts as the public API

The following are covered by the compatibility promise:

- Every header under `src/include/lintel/` (the generated `config.h` is exempt from
  the namespace rule but still public).
- The exported CMake targets and their namespace: `lintel::core`, `lintel::base`,
  `lintel::property`, `lintel::http`, `lintel::cli`, `lintel::app` and the
  `lintel::lintel` aggregate. `find_package(lintel)` and the target names are part of
  the contract.
- The REST API described by `swagger.yaml` (the wire contract; `swagger.md` mirrors
  it).
- The bootstrap XML element names that `Configuration` / the components parse.

Everything in `namespace lintel` is public by default - there is no "internal"
namespace that is allowed to break silently. Types are only non-public if they are
not reachable from a public header.

## What a version bump means (from `1.0.0` on)

| Change | Bump |
|--------|------|
| Rename or remove a public symbol, change a signature, change a CMake target name, remove or rename a bootstrap-XML element, incompatible REST change | **MAJOR** |
| Add a public symbol, target, endpoint or optional XML attribute; add an overload | **MINOR** |
| Bug fix with no API change, performance, documentation | **PATCH** |

Behavioural changes that do not change the signature (for example the migration fix in
`DatabaseBootstrapPlugin`, which now actually applies higher-versioned schema files) are
**MINOR** changes and are described in the changelog.

## Deprecation

Once `1.0.0` is out, a public symbol is marked with `[[deprecated("...")]]` and kept
working for at least one **MINOR** release before a **MAJOR** release removes it. The
changelog entry names the replacement.

## Changelog and release process

- Every user-visible change gets an entry in `CHANGELOG.md` under `[Unreleased]`,
  following [Keep a Changelog](https://keepachangelog.com/); breaking changes include a
  migration note.
- A release moves `[Unreleased]` to a dated version heading, sets `LINTEL_VERSION`
  accordingly (and `swagger.yaml`/`swagger.md`), and tags `vX.Y.Z`. The `release`
  workflow refuses a tag that does not match `LINTEL_VERSION`.

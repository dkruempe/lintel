# Changelog

Alle nennenswerten Änderungen an diesem Projekt werden in dieser Datei dokumentiert.

Das Format basiert auf [Keep a Changelog](https://keepachangelog.com/de/1.0.0/),
und dieses Projekt orientiert sich an [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Hinzugefügt
- Offizielle OSS-Dokumente: `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `SECURITY.md` sowie `CHANGELOG.md` (dieses Dokument).
- README: CI-Status-Badge und MIT-Lizenz-Badge, Statushinweis für Pre-1.0/API-Stabilität sowie Hinweise zu Tests, PostgreSQL-Abhängigkeit und Zertifikaten (korrekt an CI angepasst).

### Geändert
- Build-Anleitung im README konsequent an CI (`.github/workflows/ci.yml`) und `AGENTS.md` ausgerichtet (Conan 2, Toolchain-Pfad unter `build/build/Release/generators/conan_toolchain.cmake`, Release-Build mit `ccache`, Ninja, Ausgabe nach `bin/`).
- TLS-Zertifikate: Private Schlüssel (`server.key`) aus dem Repository entfernt, generierte Zertifikate über `.gitignore` ausgeschlossen; CI erzeugt Zertifikate selbst. Doku (`cfg/certs/README.md`) entsprechend angepasst.
- Konfiguration und Beispiele: Parametrisierung von Pfaden (z. B. Named-Pipes, Konfiguration), Entfernung von Klartext-Passwörtern/Credentials und unnötigen Personendaten aus Beispielen/Konfiguration.
- CI/Build: Anpassungen für GCC 13 (Debug/Unity), `-Wnull-dereference` nur für Clang, `BOOST_UUID_NO_SIMD` in Debug (GCC 13), `CCACHE_DIR` explizit gesetzt; Format- und Lint-Gates (advisory für Lint) dokumentiert.

### Sicherheitsrelevant
- Sensitive Daten aus Repo entfernt (TLS-Private-Key, Klartext-Credentials). Meldeweg für Sicherheitslücken in `SECURITY.md` festgelegt (E-Mail statt öffentlichem Issue).

### Entwicklererfahrung
- Testabschnitt auf `ctest --output-on-failure --label-regex unit` umgestellt; Hinweis auf explizite Testliste in `tests/CMakeLists.txt` ergänzt.
- Formatprüf-Skript (`scripts/check-format.sh`) und clang-tidy-Skript (`scripts/check-tidy.sh`) werden in Doku referenziert.

## [0.1.0] - 2026-10-03

Erster Release-Tag `v0.1.0` (Commit `86b72f4`). Wesentliche Themen (aus Commit-Historie):
- CI-End-to-End: Trigger auf `master`, Matrix GCC/Clang, Format-Gate, Release-Workflow.
- Build-System: CMake 3.16+, Conan 2, Ninja, Unity-Builds, PCH, Compiler-Warnings mit `-Werror`.
- Core-Bibliothek (`kruempelmann::base_library`), Features (HTTP via cpp-httplib, CLI, Property, Persistence PostgreSQL/SQLite), vendored DI (Hypodermic).
- Tests (Catch2 + trompeloeil), Beispiele, Konfiguration (`cfg/`), Docker-Profile.

Die folgenden Commits nach diesem Tag (u. a. `LICENSE`, `THIRD_PARTY_NOTICES.md`, CI-Korrekturen, Sicherheits- und Doku-Audits) sind oben unter `[Unreleased]` erfasst.

[Unreleased]: https://github.com/dkruempe/cpp-base-library/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/dkruempe/cpp-base-library/releases/tag/v0.1.0

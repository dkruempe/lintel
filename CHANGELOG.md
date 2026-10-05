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

### Behoben
- `db::Argument::getValue<TYPE>()`: bei einem unbekannten `ConnectionType` Verlassen die Funktion aus einem Non-`void`-Funktionsrumpf heraus (Undefined Behaviour). Sie wirft jetzt `db::SQLException`. Der `UNDEFINED`-, SQLite- und PostgreSQL-Pfad sind unverändert.
- `Process::inActiveWindow()` und der `operator<<` in `examples/message_queue.cpp` nutzen nicht mehr die thread-unsicheren `std::localtime`/`std::ctime`, sondern `localtime_r` (kein geteilter statischer Puffer mehr). Zeitformat im Beispiel ist jetzt deterministisch (`%Y-%m-%d %H:%M:%S`) und ohne angehängten Newline.
- Beispiele und öffentliche Header bereinigt: keine C-Casts über `void*` mehr im Message-Queue-Beispiel, fehlende Empfangsprüfung dort abgesichert, `getValue<TYPE>()` mit explizitem `default`-Zweig und die beiden `operator++(int)`-Iteratoren mit `const`-Rückgabetyp (`CERT DCL21-CPP`). Die vom CodeQL-Fix berührten Dateien sind frei von clang-tidy-Funden.

### Sicherheitsrelevant
- Sensitive Daten aus Repo entfernt (TLS-Private-Key, Klartext-Credentials). Meldeweg für Sicherheitslücken in `SECURITY.md` festgelegt (E-Mail statt öffentlichem Issue).
- CodeQL-Scan für C++ aktiviert (`.github/workflows/codeql.yml`). Die drei Befunde mit Defekt-Charakter (fehlendes `return`, zwei `localtime`/`ctime`-Aufrufe) sind behoben; ein Befund (`cpp/cleartext-storage-database`) wurde als Fehlalarm triagiert – gespeichert wird ausschließlich der gehashte Passwortwert. Die Triage-Tabelle mit Fundstellen, Fixes und Gegenproben sowie die Aufschlüsselung der verbleibenden Stil-Hinweise wird nicht im Repo geführt.

### Entfernt
- `TODO.md`, `ANALYSIS.md` und `ROADMAP.md` aus dem Repository entfernt. Sie enthielten betreiberinterne Arbeitsstände (u. a. offene Sicherheitsaktionen und Eskalationswege), die nicht Teil eines öffentlichen Repos sein sollten. Der Inhalt bleibt über die Git-Historie vollständig abrufbar (`git show <vorheriger-Commit>:ROADMAP.md`).

### Entwicklererfahrung
- Testabschnitt auf `ctest --output-on-failure --label-regex unit` umgestellt; Hinweis auf explizite Testliste in `tests/CMakeLists.txt` ergänzt.
- Formatprüf-Skript (`scripts/check-format.sh`) und clang-tidy-Skript (`scripts/check-tidy.sh`) werden in Doku referenziert.
- Regressionstest für den `ConnectionType`-Fehlerpfad in `db::Argument::getValue<TYPE>()` ergänzt.
- Die von den CodeQL-Fixes berührten Dateien wurden vollständig `clang-format`iert, damit das Format-Regressions-Gate grün bleibt.

## [0.1.0] - 2026-10-03

Erster Release-Tag `v0.1.0` (Commit `86b72f4`). Wesentliche Themen (aus Commit-Historie):
- CI-End-to-End: Trigger auf `master`, Matrix GCC/Clang, Format-Gate, Release-Workflow.
- Build-System: CMake 3.16+, Conan 2, Ninja, Unity-Builds, PCH, Compiler-Warnings mit `-Werror`.
- Core-Bibliothek (`kruempelmann::base_library`), Features (HTTP via cpp-httplib, CLI, Property, Persistence PostgreSQL/SQLite), vendored DI (Hypodermic).
- Tests (Catch2 + trompeloeil), Beispiele, Konfiguration (`cfg/`), Docker-Profile.

Die folgenden Commits nach diesem Tag (u. a. `LICENSE`, `THIRD_PARTY_NOTICES.md`, CI-Korrekturen, Sicherheits- und Doku-Audits) sind oben unter `[Unreleased]` erfasst.

[Unreleased]: https://github.com/dkruempe/cpp-base-library/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/dkruempe/cpp-base-library/releases/tag/v0.1.0

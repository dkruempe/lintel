# Changelog

Alle nennenswerten Änderungen an diesem Projekt werden in dieser Datei dokumentiert.

Das Format basiert auf [Keep a Changelog](https://keepachangelog.com/de/1.0.0/),
und dieses Projekt orientiert sich an [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Hinzugefügt
- **Secret-Guard** (`scripts/check-secrets.sh`, blockierender CI-Job `secrets`): blockiert Klartext-Credentials im Arbeitsbaum. Ersatz für `secret_scanning_push_protection`, das sich für dieses Repo nicht per API aktivieren lässt (alle Felder in `security_and_analysis` werden mit 422 abgelehnt, auch mit Admin-Rechten). Prüft in zwei Stufen – Konfigurationsformate bzw. PEM-Schlüsselblöcke, **und** ein Entropie-Test auf den Wert; eine reine Namenssuche ist unbrauchbar, weil das Repo Dutzende `const char *const PASSWORD = "password";` als JSON-Schlüsselnamen hat.
- README: Doxygen-Badge und Abschnitt „Documentation" mit der URL der veröffentlichten API-Referenz und dem lokalen Generierungskommando.
- Offizielle OSS-Dokumente: `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `SECURITY.md` sowie `CHANGELOG.md` (dieses Dokument).
- README: CI-Status-Badge und MIT-Lizenz-Badge, Statushinweis für Pre-1.0/API-Stabilität sowie Hinweise zu Tests, PostgreSQL-Abhängigkeit und Zertifikaten (korrekt an CI angepasst).
- **Funktionsfähiges CMake-Paket:** `find_package(base_library CONFIG)` findet das installierte Paket, `base_libraryConfig.cmake` (neu) mit `find_dependency`-Kette und Relokationsfähigkeit sowie `base_libraryConfigVersion.cmake` in kanonischem Namen. `LICENSE`, `README.md` und `swagger.yaml` werden mitinstalliert. Das installierte Paket funktioniert aus einem beliebigen Präfix – mit einem externen Gegenproben-Projekt geprüft.
- README: Abschnitt „Examples" mit Tabelle aller Beispiele (was es zeigt, wie es startet), Erläuterung von `CONFIG_DIRECTORY` und `BOOTSTRAP_CONFIG_NAME`, TLS-Voraussetzungen und SQLite vs. PostgreSQL.

### Geändert
- Build-Anleitung im README konsequent an CI (`.github/workflows/ci.yml`) und `AGENTS.md` ausgerichtet (Conan 2, Toolchain-Pfad unter `build/build/Release/generators/conan_toolchain.cmake`, Release-Build mit `ccache`, Ninja, Ausgabe nach `bin/`).
- TLS-Zertifikate: Private Schlüssel (`server.key`) aus dem Repository entfernt, generierte Zertifikate über `.gitignore` ausgeschlossen; CI erzeugt Zertifikate selbst. Doku (`cfg/certs/README.md`) entsprechend angepasst.
- Konfiguration und Beispiele: Parametrisierung von Pfaden (z. B. Named-Pipes, Konfiguration), Entfernung von Klartext-Passwörtern/Credentials und unnötigen Personendaten aus Beispielen/Konfiguration.
- CI/Build: Anpassungen für GCC 13 (Debug/Unity), `-Wnull-dereference` nur für Clang, `BOOST_UUID_NO_SIMD` in Debug (GCC 13), `CCACHE_DIR` explizit gesetzt; Format- und Lint-Gates (advisory für Lint) dokumentiert.
- **`ProcessController`: unbekannte Prozess-ID antwortet jetzt 404 statt 501.** 501 („Not Implemented") war semantisch falsch – die Ressource existiert nicht, statt dass die Funktionalität fehlt. Betrifft `stopProcessDelete`, `terminateProcessDelete`, `restartProcessPut`, `resetProcessPost` und `healthProcessGet`; `swagger.yaml` und `swagger.md` sind synchron angepasst. **Breaking Change** für Clients, die auf 501 geprüft haben.

### Behoben
- **Regressionstest korrigiert:** Der Test für `db::Argument::getValue<TYPE>()` erzeugte selbst undefiniertes Verhalten, indem er per `memcpy` einen Enumerator außerhalb der gültigen Range konstruierte. Das Laden eines ungültigen Enum-Werts ist in C++ UB, und der neue UBSan-Job hat genau das gemeldet. Der Test prüft jetzt die drei gültigen Connection-Types; der `default:`-Zweig in `getValue<TYPE>()` bleibt als Härtung, ist aber bewusst ungetestet und im Testkommentar begründet.
- `db::Argument::getValue<TYPE>()`: bei einem unbekannten `ConnectionType` Verlassen die Funktion aus einem Non-`void`-Funktionsrumpf heraus (Undefined Behaviour). Sie wirft jetzt `db::SQLException`. Der `UNDEFINED`-, SQLite- und PostgreSQL-Pfad sind unverändert.
- `Process::inActiveWindow()` und der `operator<<` in `examples/message_queue.cpp` nutzen nicht mehr die thread-unsicheren `std::localtime`/`std::ctime`, sondern `localtime_r` (kein geteilter statischer Puffer mehr). Zeitformat im Beispiel ist jetzt deterministisch (`%Y-%m-%d %H:%M:%S`) und ohne angehängten Newline.
- Beispiele und öffentliche Header bereinigt: keine C-Casts über `void*` mehr im Message-Queue-Beispiel, fehlende Empfangsprüfung dort abgesichert, `getValue<TYPE>()` mit explizitem `default`-Zweig und die beiden `operator++(int)`-Iteratoren mit `const`-Rückgabetyp (`CERT DCL21-CPP`). Die vom CodeQL-Fix berührten Dateien sind frei von clang-tidy-Funden.
- `INSTALL_INTERFACE` der Bibliothek enthält keine Build-Maschinen-Absolutpfade der Conan-Cache-Pfade mehr, Includes von `magic_enum`/`rapidjson` kommen über Targets statt über gebackene Pfade, und das private Target `project_warnings` landet nicht mehr im öffentlichen Export.
- `BaseFeature::registerTypes` und `UserManagementCliComponent::onCommand` sind jetzt an der Definition dokumentiert (Ablauf, Randbedingungen, `@param`), nicht nur am Header – der Doxygen-Workflow prüft das als Gate, und CodeQLs `cpp/poorly-documented-function` zählt an der Definition.
- Vier CodeQL-Alarme als Fehlalarm dismissed: drei `cpp/ambiguously-signed-bit-field` (die Regel liest den Scope-Operator `::` aus `boost::interprocess::named_semaphore` als Bitfeld-Deklaration – im gesamten `include`-Baum existiert kein einziges Bitfeld) und `cpp/cleartext-storage-database` (es wird der gehashte, nicht der im Klartext gespeicherte Passwortwert geschrieben). Damit meldet CodeQL **null** Warnungen; übrig sind nur Stil-Hinweise auf `note`-Ebene.
- Doppelter `install(FILES)`-Eintrag im vendorten Hypodermic bereinigt (dieselbe Datei zweimal in einer Zeile gelistet).


- Property-Repositories: die bislang nur als`LOG_ERROR("{}", exception.what())` protokollierten Fehler nennen jetzt die konkrete Operation, die Prozess-ID und die Anzahl der Properties. Das bewusste „Best Effort"-Verhalten ist im Dateikopf begründet.
- Leere Zweige in `SharedMemoryPropertyRepository.h` (`cpp/empty-block`) auf ihre positive Bedingung umgestellt; `FilePropertyRepository::save()` und `SharedMemoryPropertyRepository::onMigrate()` sind als bewusste No-Op dokumentiert.

### Bekannte Lücken
- Die 404-Umstellung in `ProcessController` ist nicht durch Tests abgedeckt. Grund: `ProcessController` nimmt den konkreten `ProcessService` statt eines Interfaces, dessen Konstruktor über `PropertyRegistration` eine vollwertige Bootstrap-Umgebung braucht. Ein `IProcessService`-Interface wäre ein API-Bruch und ist für die Namespace-/API-Phase vorgemerkt.

### Sicherheitsrelevant
- Sensitive Daten aus Repo entfernt (TLS-Private-Key, Klartext-Credentials). Meldeweg für Sicherheitslücken in `SECURITY.md` festgelegt (E-Mail statt öffentlichem Issue).
- CodeQL-Scan für C++ aktiviert (`.github/workflows/codeql.yml`). Die drei Befunde mit Defekt-Charakter (fehlendes `return`, zwei `localtime`/`ctime`-Aufrufe) sind behoben; ein Befund (`cpp/cleartext-storage-database`) wurde als Fehlalarm triagiert – gespeichert wird ausschließlich der gehashte Passwortwert. Die Triage-Tabelle mit Fundstellen, Fixes und Gegenproben sowie die Aufschlüsselung der verbleibenden Stil-Hinweise wird nicht im Repo geführt.

### Geändert
- **Repository umgezogen** nach `https://github.com/dkruempe/lintel`. Grund ist nicht die Schönheit des Namens, sondern ein Sicherheitsproblem: Im alten Repo lieferten die vier `refs/pull/{1,3,4,5,6}` der gemergten Pull Requests ein Klartext-DB-Passwort aus, weil GitHub diese Refs an das Repository bindet. Ein neu angelegtes Repository hat keine Pull Requests und damit auch keine solchen Refs. Die umgeschriebene Historie wurde **vollständig** übernommen (kein Verwerfen der Commits) — nach dem `filter-repo`-Durchgang enthielt sie das Passwort nicht mehr, verifiziert über einen Objekt-Scan aller 4759 Blobs mit Positivkontrolle. Die alte Doku-Adresse lautet entsprechend jetzt `https://dkruempe.github.io/lintel/`.

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

[Unreleased]: https://github.com/dkruempe/lintel/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/dkruempe/lintel/releases/tag/v0.1.0

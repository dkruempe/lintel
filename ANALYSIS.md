# Projektanalyse: cpp-base-library

## Stärken

1. **Saubere, modulare Architektur** – Feature-basiertes Plugin-System mit Serviceorientierung und Dependency Injection (Hypodermic). Jedes Feature (Base, CLI, HTTP, Property) ist in sich geschlossen mit eigenen Models, Services, Controllern, Repositories und Configuration-Komponenten.

2. **Hervorragende Build- und Toolchain-Qualität** – Warnings-as-Errors (MSVC/Clang/GCC), clang-tidy, cppcheck (exhaustive), include-what-you-use, clang-format – alles konfiguriert und eingebunden. Das ist vorbildlich für ein C++ Projekt.

3. **Durchdachte Abstraktionsschichten** – Die Persistenzschicht (`core/persistence/`) definiert saubere Interfaces (`Connection`, `Statement`, `PreparedStatement`, `Result`, `Transaction`) mit zwei konkreten Backends (PostgreSQL, SQLite). Der `db::Connection` nutzt ein Variant-ähnliches Pattern.

4. **Umfangreicher Feature-Umfang** – HTTP-API (via cpp-httplib), interaktive CLI, Property-System, Shared Memory (Boost.Interprocess), Message Queues, Task Scheduling.

5. **Gutes Dependency Management** – Alle externen Abhängigkeiten über Conan mit spezifischen, getesteten Versionen.

6. **Konsistenter Code-Stil** – `.clang-format` und `.clang-tidy` stellen einheitliche Formatierung sicher. Naming Conventions (PascalCase Klassen, `m_` Member, camelCase Methoden) werden durchgehend eingehalten.

7. **Dokumentation** – README mit Architekturüberblick, Build-Anleitung, Abhängigkeiten. Swagger-Dokumentation für die HTTP API.

---

## Schwächen

1. **Vendored DI Container** – Hypodermic liegt als Copy in `external/` und ist seit 2017 nicht mehr aktiv maintained (letzter Commit). Moderne Alternativen wie Boost.DI wären wartbarer.

---

## Sicherheitsanalyse (Stand: 19.08.2026)

### Hoch

1. **Hardcodierte Default-Credentials im Seed-Script** – `cfg/database/DEFAULT_SQLITE/data_schema_default_version_1.sql` legt den Admin-User `dominik` mit festem SHA-512-Hash an (vermutlich bekanntes Passwort). Auslieferung mit Default-Credentials ist ein Sicherheitsrisiko.
   - **Status: BEWUSST BEIBEHALTEN (nur für lokale Tests)** – Vom Betreiber als pragmatischer Weg zum Testen bestätigt; Hash wird bei erstem Login automatisch auf salted PBKDF2 migriert. Für Produktion Default-Passwort ändern bzw. Seed-Script nicht ausliefern.

### Zu beachten

2. **`/hello`-Endpoint (ExampleController)** und Swagger-Datei liegen ungeschützt im Repo; die Swagger-Spezifikation sollte an den tatsächlichen Auth-/Status-Codes-Stand angepasst werden (dokumentiert derzeit teilweise 200 ohne Auth-Anforderung).
3. **Docker-Compose** exponiert PostgreSQL (`test/test`) auf `5432` und die App auf `8080` ohne TLS – für Produktiv-Setups nicht geeignet.

---

## Offene Bugs / bekannte Fehler

> Stand: 19.08.2026 – Alle nachfolgend gelisteten Punkte wurden behoben und über einen vollständigen Build (`-Werror`) sowie `ctest` (311/311 grün) verifiziert.

Alle identifizierten Bugs wurden behoben. Die vollständige Liste der Fixes findet sich in `TODO.md` unter „Kritische Bugs". Im Folgenden eine Zusammenfassung der wichtigsten Kategorien:

- **Process-Service** (5 Fixes): disableAutoStart-Copy-Paste-Fehler, Monitor-Thread-Exception-Behandlung, doppeltes Promise-Setzen in detachOf, stopOf-Auto-Restart-Race, exit_code für laufende Kinder.
- **Shared Memory** (4 Fixes): growOf-Remapping, maxSize-Erzwingung, Thread-Safe-Check, PropertyDataDto-Größenberechnung.
- **Persistenz** (12 Fixes): NULL-Handling, PreparedStatement-Validierung, LISTEN/NOTIFY-Thread-Safety, SQLite-Update-Hook, Transaction-Destruktor, Iterator-Postfix-Increment, Connection-Leaks, Close/Finalize-Reihenfolge, Fehlercode-Prüfung, SQL-Injection.
- **Repositories** (4 Fixes): Regex-Filter-Vertauschung, allMessageQueueNameOf-Fehler, Geister-User bei leerem Ergebnis, GroupRepository-Cache-Synchronisation.
- **HTTP/API** (11 Fixes): Non-Object-JSON-Crash, SSL-Client-Plaintext-Fallback, ContentType-EXAKT-Vergleich, dereferencing fehlgeschlagener Ergebnisse, fehlende Route-Weiterleitung, falsche Hardcodierung, API-Ergebnis-Verwerfung, Server-Listen-Fehlschlag.
- **Services/Threading** (4 Fixes): StartupBuilder-Data-Race, Scheduler/Executor-Exceptions, AuthService-Sliding-Window, FileService-Sparse-File.
- **DTOs/Utils/CLI** (6 Fixes): GroupDto-Serialize, UserNameDto-UB, MemorySize-deserialize, XMLConfig-deserialize, OOB-Read bei leerem Pfad, ProcessCliComponent-Flag.

---

## Optimierungsmöglichkeiten (priorisiert)

| Priorität | Maßnahme | Status | Begründung |
|-----------|----------|--------|------------|
| **Hoch** | **Fehlende Features abschließen (TODO.md)** | Offen | MySQL Support, Cursor, Paging |
| **Mittel** | **Benchmark-Suite aufsetzen** | Offen | Performance-Messungen für Property-System, Persistenz, Serialisierung |
| **Mittel** | **Swagger-Dokumentation an tatsächliche Auth-/Status-Codes angleichen** | Offen | Doku spiegelt Auth-Modell nicht korrekt wider |
| **Niedrig** | **CMake modernisieren** (`include_directories` → `target_include_directories`) | Teilweise erledigt | Library-Target (`src/CMakeLists.txt:495`) nutzt bereits `target_include_directories`. Noch alt: `tests/CMakeLists.txt:10`, `examples/CMakeLists.txt:9`, `src/CMakeLists.txt:33-35` (Hypodermic). |
| **Niedrig** | **Hypodermic durch Boost.DI ersetzen** | Offen | Aktiver maintained, standardkonformer |

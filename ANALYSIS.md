# Projektanalyse: cpp-base-library

## Stärken

1. **Saubere, modulare Architektur** – Feature-basiertes Plugin-System mit Serviceorientierung und Dependency Injection (Hypodermic). Jedes Feature (Base, CLI, HTTP, Property) ist in sich geschlossen mit eigenen Models, Services, Controllern, Repositories und Configuration-Komponenten.

2. **Hervorragende Build- und Toolchain-Qualität** – Warnings-as-Errors (MSVC/Clang/GCC), clang-tidy, cppcheck (exhaustive), include-what-you-use, clang-format – alles konfiguriert und eingebunden.

3. **Durchdachte Abstraktionsschichten** – Die Persistenzschicht (`core/persistence/`) definiert saubere Interfaces (`Connection`, `Statement`, `PreparedStatement`, `Result`, `Transaction`) mit zwei konkreten Backends (PostgreSQL, SQLite). Der `db::Connection` nutzt ein Variant-ähnliches Pattern.

4. **Umfangreicher Feature-Umfang** – HTTP-API (via cpp-httplib), interaktive CLI, Property-System, Shared Memory (Boost.Interprocess), Message Queues, Task Scheduling.

5. **Gutes Dependency Management** – Alle externen Abhängigkeiten über Conan mit spezifischen, getesteten Versionen.

6. **Konsistenter Code-Stil** – `.clang-format` und `.clang-tidy` stellen einheitliche Formatierung sicher. Naming Conventions (PascalCase Klassen, `m_` Member, camelCase Methoden) werden durchgehend eingehalten.

7. **Dokumentation** – README mit Architekturüberblick, Build-Anleitung, Abhängigkeiten. Swagger-Dokumentation für die HTTP API.

---

## Schwächen

1. **Vendored DI Container** – Hypodermic liegt als Copy in `external/` und ist seit 2017 nicht mehr aktiv maintained (letzter Commit). Moderne Alternativen wie Boost.DI wären wartbarer.

---

## Architektur-Details (Stand: 29.08.2026)

### Cursor-Architektur

`db::Cursor` ist eine einheitliche Fassade über zwei Backend-Implementierungen:

- **PostgreSQL** (`postgresql::Cursor`): Zwei Modi:
  - *Server-Side-Cursor* (`fetchSize > 0`): `DECLARE CURSOR` → `FETCH <N>` → `CLOSE` mit konfigurierbarer Batch-Größe (Default 100). Optimiert für große Ergebnismengen, vermeidet volles Laden in den Client-Speicher.
  - *Client-Side-Streaming* (`fetchSize == 0`): `PQsetSingleRowMode` (Legacy-Modus).
  - `FOR UPDATE` wird manuell in der Query-SQL angegeben (nicht über API-Flag).
- **SQLite** (`sqlite::Cursor`): `sqlite3_step` mit konfigurierbarem Client-Puffer (Default 64). Kein Server-Side-Cursor möglich.
- **Lazy-Backend-Init**: `db::Cursor` speichert Query/Params/Connection-Shared-Ptrs und erstellt den Backend-Cursor erst beim ersten `fetchNext()`/`begin()` via `initBackendCursor()`. Konfigurationsmethoden (`setFetchSize`, `setCursorName`, `setNoScroll`) können daher nach `executeCursor()` aufgerufen werden.
- **Friend-Pattern**: `db::Cursor` ist Friend von `db::Connection` für Zugriff auf private `m_conn`/`m_connSQLite` Shared-Ptrs.

### Keyset-Pagination

- `Page<T>`-Modell mit Response-Envelope `{items, has_more, next_after}`.
- Zweiphasige Abfrage für Multi-Group-User: Erste Phase `LIMIT n+1`, zweite Phase `OFFSET` für korrekte Seitengrenze.
- `PagingParams`/`pagingParamsOf` in Controller-Basisklasse.
- `?after=<sortKey>&limit=<n>` (Default 100, Max 1000, invalid limit → 400) für `/user/users`, `/user/users/{userName}` und `/messageQueue/...`.

### Persistenzschicht

`core/persistence/` definiert eine einheitliche Abstraktion über PostgreSQL und SQLite:

- `db::Connection` nutzt ein Union-ähnliches Pattern mit `ConnectionType`-Enum.
- `db::Cursor` (Iterator-API: `begin()`/`end()`, Input-Iterator) über `Statement::executeCursor`.
- `db::Transaction` (RAII): COMMIT im Destruktor bei Erfolg, ROLLBACK bei Exception (Exception-sicher).
- `db::PreparedStatement` mit typsicherer Parameter-Bindung über `ParameterBuilder`.

---

## Sicherheitsanalyse (Stand: 29.08.2026)

### Hoch

1. **Hardcodierte Default-Credentials im Seed-Script** – `cfg/database/DEFAULT_SQLITE/data_schema_default_version_1.sql` legt den Admin-User `dominik` mit festem SHA-512-Hash an.
   - **Status: BEWUSST BEIBEHALTEN (nur für lokale Tests)** – Hash wird bei erstem Login automatisch auf salted PBKDF2 migriert.

### Health/Readiness

- **`GET /health`** (Liveness) und **`GET /ready`** (Readiness) antworten unauthentifiziert mit `200` und einem JSON-Body (`{"status":"ok"}` / `{"status":"ready"}`). Bewusst ohne Bearer-Token-Pflicht, damit Orchestratoren (K8s-Probes, Docker healthcheck) sie als Liveness/Readiness-Hooks verwenden können.
- Registriert direkt in `Server::registerHealthEndpoints` (Plain- und SSL-Pfad); kein echter Dependency-/DB-Check (lightweight, nur "Prozess läuft").

### Zu beachten

2. **Swagger-Datei** liegt ungeschützt im Repo; die Spezifikation sollte an den tatsächlichen Auth-/Status-Codes-Stand angepasst werden.
3. **Docker-Compose** exponiert PostgreSQL (`test/test`) auf `5432` und die App auf `8080` ohne TLS – für Produktiv-Setups nicht geeignet.

---

## Offene Bugs / bekannte Fehler

> Stand: 26.08.2026 – Alle identifizierten Bugs wurden behoben und über einen vollständigen Build (`-Werror`) sowie `ctest` (331/331 grün) verifiziert.

Die vollständige Liste der Fixes wurde in der vorherigen Version dieses Dokuments dokumentiert. Zusammenfassung der wichtigsten Kategorien:

- **Process-Service** (5 Fixes): disableAutoStart-Copy-Paste-Fehler, Monitor-Thread-Exception-Behandlung, doppeltes Promise-Setzen in detachOf, stopOf-Auto-Restart-Race, exit_code für laufende Kinder.
- **Shared Memory** (4 Fixes): growOf-Remapping, maxSize-Erzwingung, Thread-Safe-Check, PropertyDataDto-Größenberechnung.
- **Persistenz** (12 Fixes): NULL-Handling, PreparedStatement-Validierung, LISTEN/NOTIFY-Thread-Safety, SQLite-Update-Hook, Transaction-Destruktor, Iterator-Postfix-Increment, Connection-Leaks, Close/Finalize-Reihenfolge, Fehlercode-Prüfung, SQL-Injection.
- **Repositories** (4 Fixes): Regex-Filter-Vertauschung, allMessageQueueNameOf-Fehler, Geister-User bei leerem Ergebnis, GroupRepository-Cache-Synchronisation.
- **HTTP/API** (11 Fixes): Non-Object-JSON-Crash, SSL-Client-Plaintext-Fallback, ContentType-EXAKT-Vergleich, dereferencing fehlgeschlagener Ergebnisse, fehlende Route-Weiterleitung, falsche Hardcodierung, API-Ergebnis-Verwerfung, Server-Listen-Fehlschlag.
- **Services/Threading** (4 Fixes): StartupBuilder-Data-Race, Scheduler/Executor-Exceptions, AuthService-Sliding-Window, FileService-Sparse-File.
- **DTOs/Utils/CLI** (6 Fixes): GroupDto-Serialize, UserNameDto-UB, MemorySize-deserialize, XMLConfig-deserialize, OOB-Read bei leerem Pfad, ProcessCliComponent-Flag.

---

## Offene Maßnahmen (priorisiert)

| Priorität | Maßnahme | Status | Begründung |
|-----------|----------|--------|------------|
| **Hoch** | **`MemorySize`-Größen beim Grow begrenzen** | In Arbeit (2026-08-29) | Resource Exhaustion über Shm-API, Overflow-Check in `SharedMemoryService::onCheck` |
| **Hoch** | **Health/Readiness endpoint** | Erledigt (2026-08-29) | `GET /health` (liveness), `GET /ready` (readiness), unauthentifiziert, 200 JSON |
| **Hoch** | **Backup/Archive-Strategie für Shared Memory** | Offen | Kein Dump/Restore-Mechanismus vorhanden |
| **Mittel** | **MySQL Support** | Zurückgestellt | Bewusst nach hinten verschoben (User-Anforderung) |
| **Niedrig** | **Hypodermic durch Boost.DI ersetzen** | Offen | Aktiver maintained, standardkonformer |

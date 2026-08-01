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

## Sicherheitsanalyse (Stand: Juli 2026)

> Stand: Juli 2026 – Alle ursprünglich als `FIXED` markierten Punkte wurden am 31.07.2026 behoben und über `ctest` verifiziert. Nachfolgend sind nur noch offene bzw. teilweise behobene Punkte gelistet.

### Hoch

1. **Hardcodierte Default-Credentials im Seed-Script** – `cfg/database/DEFAULT_SQLITE/data_schema_default_version_1.sql` legt den Admin-User `dominik` mit festem SHA-512-Hash an (vermutlich bekanntes Passwort). Auslieferung mit Default-Credentials ist ein Sicherheitsrisiko.
   - **Status: BEWUSST BEIBEHALTEN (nur für lokale Tests)** – Vom Betreiber als pragmatischer Weg zum Testen bestätigt; Hash wird bei erstem Login automatisch auf salted PBKDF2 migriert. Für Produktion Default-Passwort ändern bzw. Seed-Script nicht ausliefern.

### Zu beachten

2. **`/hello`-Endpoint (ExampleController)** und Swagger-Datei liegen ungeschützt im Repo; die Swagger-Spezifikation sollte an den tatsächlichen Auth-/Status-Codes-Stand angepasst werden (dokumentiert derzeit teilweise 200 ohne Auth-Anforderung).
3. **Docker-Compose** exponiert PostgreSQL (`test/test`) auf `5432` und die App auf `8080` ohne TLS – für Produktiv-Setups nicht geeignet.

---

## Offene Bugs / bekannte Fehler

> Stand: 01.08.2026 – Alle nachfolgend gelisteten Punkte wurden behoben und über einen vollständigen Build (`-Werror`) sowie `ctest` (21/21 grün) verifiziert.

1. **`Client::post` (SSL-Pfad) übergibt falsches Argument** – **FIXED (01.08.2026)**: `m_sslClient->Post(pathStr, contentType, contentType)` wurde auf `m_sslClient->Post(pathStr, std::move(contentProvider), contentType)` korrigiert (`Client.cpp:227`).

2. **Data Race auf `m_processes` in `ProcessService`** – **FIXED (01.08.2026)**: `terminateOf` und `detachOf` führen Promise-Set und `erase` jetzt unter `m_processesMutex` aus; `monitorProcess` kann die Map nicht mehr nebenläufig modifizieren. Zusätzlich wird geprüft, ob der Eintrag nach dem Warten noch existiert (doppeltes `set_value` vermieden).

3. **`ProcessService::stopOf` – Condition-Variable ohne Notifier** – **FIXED (01.08.2026)**: Die nie notifizierte lokale Condition-Variable wurde durch ein Polling auf `child::running()` bis `m_processStopWaitTime` ersetzt (`ProcessService.cpp:253-258`). (Das deprecated/unzuverlässige `child::wait_for` wurde dabei bewusst nicht verwendet.)

4. **`ProcessService::terminateOf` – `exit_code()` direkt nach `terminate()`** – **FIXED (01.08.2026)**: Nach `terminate()` wird mit `child::wait()` auf das tatsächliche Prozessende gewartet, bevor `exit_code()` gelesen wird (`ProcessService.cpp:314-315`).

5. **`MessageQueueService::numberMessagesOf` – falsches Konstruktorargument** – **FIXED (01.08.2026)**: Als Queue-Name wird nun `entry.get_message_queue_name()` statt `get_process_name()` übergeben (`MessageQueueService.cpp:54`).

6. **`DatabaseConnectionComponent` – OOB bei leerem SQLite-Pfad** – **FIXED (01.08.2026)**: Zugriff auf `tmpPath[0]` ist durch `!tmpPath.empty() &&` abgesichert (`DatabaseConnectionComponent.cpp:72`).

7. **Scheduler-Lambdas mit `[&]`-Capture (`this`)** – **Bereits FIXED (31.07.2026)**: `AuthService` und `SharedMemoryService` nutzen bereits das `shared_from_this()`/`weak_ptr`-Muster; kein `[&]`-Capture auf `this` mehr.

8. **`startProcessPost` deserialisiert Body vor Content-Type-Prüfung** – **FIXED (01.08.2026)**: `deserialize(request.body)` wurde in den `ApplicationJson`-Zweig verschoben (`ProcessController.cpp:95-96`); bei ungültigem Content-Type wird nicht mehr geparst.

9. **`AuthService::onLoginOf` – IP-Normalisierung** – **FIXED (01.08.2026)**: Validierung erfolgt über `error_code` statt String-Vergleich; die normalisierte Darstellung (`ip.to_string()`) wird einheitlich für Lockout, Rate-Limit und Token-Speicherung verwendet. `onAccessOf` normalisiert den eingehenden IP-Vergleich entsprechend.

10. **`Group::operator<<` inkonsistent** – **FIXED (01.08.2026)**: Der Komma-Check nutzt die äußere Liste (`group.m_groups.size() - 1`) statt `memberGroup.m_groups.size() - 1`; Elemente werden per `const Group &` ausgegeben (`Group.cpp:22-28`).

---

## Optimierungsmöglichkeiten (priorisiert)

| Priorität | Maßnahme | Begründung |
|-----------|----------|------------|
| **Hoch** | **Fehlende Features abschließen (TODO.md)** | MySQL Support, Cursor, Paging |
| **Mittel** | **Unit-Tests für ProcessService, UserRepository/GroupRepository ergänzen** | Process-Management und User-/Gruppen-Persistenz sind ungetestet |
| **Mittel** | **Benchmark-Suite aufsetzen** | Performance-Messungen für Property-System, Persistenz, Serialisierung |
| **Mittel** | **Swagger-Dokumentation an tatsächliche Auth-/Status-Codes angleichen** | Doku spiegelt Auth-Modell nicht korrekt wider |
| **Niedrig** | **CMake modernisieren** (`include_directories` → `target_include_directories`) | Saubereres Target-Modell |
| **Niedrig** | **Hypodermic durch Boost.DI ersetzen** | Aktiver maintained, standardkonformer |

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

1. **`Client::post` (SSL-Pfad) übergibt falsches Argument** – `m_sslClient->Post(pathStr, contentType, contentType)` (`src/features/http/service/Client.cpp:208`) sollte `std::move(contentProvider)` als Provider übergeben (Copy-Paste-Fehler). Kompiliert nur, weil der OpenSSL-Pfad aktuell nicht gebaut wird; bricht den SSL-Build.

2. **Data Race auf `m_processes` in `ProcessService`** – `terminateOf` (`ProcessService.cpp:312`) und `detachOf` (`ProcessService.cpp:340`) rufen `m_processes.erase(...)` **ohne** `m_processesMutex` auf, während der Monitor-Thread die Map durchläuft.

3. **`ProcessService::stopOf` – Condition-Variable ohne Notifier** – Eine lokale `std::condition_variable cond` wird mit `m_conditionMutex` kombiniert, niemand notifiziert sie; `wait_for` läuft damit immer bis zum Timeout (`ProcessService.cpp:250-253`). Funktional träge, aber ineffektiv.

4. **`ProcessService::terminateOf` – `exit_code()` direkt nach `terminate()`** – Der Exit-Code wird unmittelbar nach dem Absetzen von SIGKILL gelesen; das Kind kann noch nicht beendet sein → undefinierter Wert.

5. **`MessageQueueService::numberMessagesOf` – falsches Konstruktorargument** – Wird die Queue in `m_messageQueues` nicht gefunden, wird `MessageQueue<Message>` mit `entry.get_process_name()` als **Queue-Name** erzeugt statt `get_message_queue_name()` (`src/features/base/services/MessageQueueService.cpp:53-54`).

6. **`DatabaseConnectionComponent` – OOB bei leerem SQLite-Pfad** – `std::string tmpPath = connection; if (tmpPath[0] == '~')` (`DatabaseConnectionComponent.cpp:72`): Bei leerem `connection`-Attribut ist `tmpPath[0]` ein OOB-Zugriff (UB).

7. **Scheduler-Lambdas mit `[&]`-Capture (`this`)** – `AuthService::onCheck`/`SharedMemoryService::onCheck` registrieren `[&]() { onCheck(); }` (`AuthService.cpp:112,117`, `SharedMemoryService.cpp:93-95`). Läuft ein getakteter Task nach Zerstörung des Service (Shutdown-Reihenfolge), droht Use-after-Free.

8. **`startProcessPost` deserialisiert Body vor Content-Type-Prüfung** – `ProcessController::startProcessPost` ruft `deserialize(request.body)` vor dem `switch (contentType)` auf (`ProcessController.cpp:96`); bei ungültigem Content-Type wird trotzdem geparst.

9. **`AuthService::onLoginOf` – IP-Normalisierung** – Der Check `ip.to_string() != userLogin.m_ipAddress` lehnt legitime Darstellungen (z. B. IPv4-mapped IPv6) ab bzw. ist von `remote_addr`-Format abhängig; bei IPv6-Zonen/Portformaten fragil.

10. **`Group::operator<<` inkonsistent** – In der Schleife wird `i != memberGroup.m_groups.size() - 1` geprüft (Member statt äußerer Liste) (`Group.cpp:25`); die Komma-Separation ist bei verschachtelten Gruppen falsch.

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

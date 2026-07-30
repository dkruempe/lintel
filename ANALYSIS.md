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

1. **Shared Memory Tests fehlen** – SharedMemoryRepository und SharedMemorySegmentManager sind aufgrund tiefer Boost.Interprocess-Kopplung in `SharedMemorySegment`/`SharedMemorySegmentInfo` (shm-Konstruktor, managed_shared_memory) nicht isoliert testbar. **Status: weiterhin offen.**

3. **Vendored DI Container** – Hypodermic liegt als Copy in `external/` und ist seit 2017 nicht mehr aktiv maintained (letzter Commit). Moderne Alternativen wie Boost.DI wären wartbarer.

4. **Kein Binary-Separation** – Core und Features werden in eine einzige Shared Library (`base_library`) kompiliert. Optionalität der Features ist nur zur Compile-Zeit über das Registrieren im StartupBuilder gegeben – nicht auf Binärebene.

---

## Kürzlich behobene Bugs

1. **SQLite REGEXP fehlt** – Conans SQLite-Bibliothek hat `REGEXP` nicht aktiviert. Runtime-Register via `sqlite3_create_function` im `sqlite::Connection`-Konstruktor gelöst (`src/core/persistence/sqlite3/Connection.cpp:46`).

2. **DatabaseBootstrapPlugin bricht Loop ab** – Bei `db::SQLException` (z. B. PostgreSQL nicht erreichbar) wurde der Fehler weitergereicht, sodass nachfolgende Verbindungen (z. B. SQLite) nie bootstrapt wurden. Fix: `throw` im `catch (db::SQLException)` entfernt, nur noch loggen (`src/core/plugins/DatabaseBootstrapPlugin.cpp:16`).

3. **Bootstrap-Reihenfolge kaputt** – `VirtualGroupBootstrapPlugin` resolved `std::vector<std::shared_ptr<GroupProvider>>` bereits im Konstruktor. Hypodermic konstruiert alle BootstrapPlugins beim `resolve<BootstrapService>()` – also *vor* dem DB-Bootstrap. GroupProvider umfassen u. a. `MessageQueueController`, das `MessageQueueService` injiziert bekommt, das im Konstruktor die (noch nicht bootstrapte) Datenbank abfragt. Fix: `GroupProvider`-Resolution in `onStart()` verschoben, Container wird per Hypodermic auto-injiziert (`src/core/plugins/VirtualGroupBootstrapPlugin.cpp:26`).

4. **`std::size_t` Serialisierung platform-spezifisch** – `#ifdef __APPLE__` in `postgresql/Serialization.h` und `sqlite3/Serialization.h`. Fix: Durch generische SFINAE-Partialspezialisierung ersetzt – erkennt automatisch, ob `std::size_t` bereits durch `uint64_t` abgedeckt ist. (`src/include/base_library/core/persistence/postgresql/Serialization.h:22`, `src/include/base_library/core/persistence/sqlite3/Serialization.h:25`).

5. **Mock-Framework implementiert** – Fehlende Isolierung in Unit-Tests durch Einführung von trompeloeil + Catch2 als Mocking-Framework. Dafür wurden Interfaces extrahiert (`IAuthService`, `IMessageQueueRepository`) und 6 Mock-Klassen erstellt. 4 neue Unit-Tests decken BootstrapService, CLI-CommandLineComponent, HTTP-Controller und MessageQueueService isoliert ab. (`tests/mocks/`, `tests/cli/`, `tests/http/`, `tests/services/`, `tests/CMakeLists.txt`).

6. **Signal-Handling modernisiert** – `std::signal()` + static raw pointer durch dedizierten Signal-Thread mit `sigwait()` ersetzt. Signale werden für alle Threads geblockt (`pthread_sigmask`); nur der Signal-Thread verarbeitet SIGINT/SIGTERM/SIGCHLD via `sigwait`. `SignalService` aufgeräumt (Dead Code entfernt). (`src/core/StartupBuilder.cpp:74-81,115-139`, `src/include/base_library/core/services/SignalService.h`).

7. **MessageQueueService DB-Zugriff aus Konstruktor entfernt** – `m_messageQueueRepository->allOf(".*", ".*")` wurde im Konstruktor aufgerufen (vor DB-Bootstrap). Jetzt in `onInitialize()` – die `InitializeService`-Pipeline ruft dies nach dem Bootstrap auf. (`src/features/base/services/MessageQueueService.cpp:39-47`, `src/include/base_library/features/base/services/MessageQueueService.h:46`).

8. **CLI InputService: std::getchar → ::read bricht Prompt-Flush** – Der Wechsel von `std::getchar()` zu `::read(STDIN_FILENO, ...)` in `InputService::onRead()` unterbrach die implizite Flush-Kette: `std::getchar()` triggert über den C-Standard (C11 §7.21.3/7) einen Flush aller line-buffered Output-Streams; `::read()` als reiner Syscall tut dies nicht. Dadurch blieben Login-Prompts im `std::cout`-Puffer unsichtbar, der Benutzer drückte Enter, der leere Username führte zum sofortigen Exit. Fix: `std::cout.flush()` vor `::read()` in `onRead()`. (`src/features/cli/services/InputService.cpp:29`).

9. **CLI-Services nicht mockbar (fehlende Interfaces)** – `InputService`, `TerminalService` und `UserApi` hatten keine virtuellen Methoden/Interfaces, was isolierte Unit-Tests für CLI-Komponenten unmöglich machte. Fix: Interfaces `IInputService`, `ITerminalService` extrahiert (`src/include/base_library/features/cli/services/IInputService.h`, `src/include/base_library/features/cli/services/ITerminalService.h`), gemeinsame Typen (`KeyType`, `Symbol`) in `CliTypes.h` ausgelagert (`src/include/base_library/features/cli/CliTypes.h`). `UserApi`-Methoden (`loginOf`, `logoutOf`, `isLoggedIn`) auf virtual geändert, protected Default-Konstruktor für Mocking ergänzt (`src/include/base_library/features/http/controllers/UserApi.h`). Dependency Injection in `CommandLineFeature.cpp` registriert die Typen nun auch als Interfaces (`as<IInputService>`, `as<ITerminalService>`).

10. **Fehlende Unit-Tests für AuthCliService** – Der Login-Flow (`readUserName`, `readPassword`, `onLogin`) war ungetestet. Fix: 5 neue Unit-Tests in `tests/cli/AuthCliServiceMockTest.cpp` mit `StubInputService`/`StubTerminalService` (queue-basierte Test-Doubles) und `MockUserApi`. Getestet: CtrlC → nullopt, Eof → nullopt, erfolgreicher Login, leeres Passwort → Wiederholung, fehlgeschlagener Login → Wiederholung. (`tests/cli/AuthCliServiceMockTest.cpp`, `tests/mocks/MockUserApi.h`, `tests/CMakeLists.txt`).

11. **UserApi nicht vollständig mockbar** – Nur 3 von 11 Methoden waren virtual (`loginOf`, `logoutOf`, `isLoggedIn`); die von `UserManagementCliComponent` genutzten Management-Methoden (`allOf` in 4 Varianten, `allUsersOf` in 2 Varianten, `createOf`, `updateOf`, `deleteOf`) waren nicht überschreibbar. Fix: Alle 11 Methoden in `UserApi` auf `virtual` gesetzt, `MockUserApi` um alle fehlenden Methoden erweitert. (`src/include/base_library/features/http/controllers/UserApi.h`, `tests/mocks/MockUserApi.h`).

12. **Fehlende Unit-Tests für UserManagementCliComponent** – Die 5 CLI-Commands (`show_groups`, `show_users`, `user_add`, `user_update`, `user_delete`) waren ungetestet. Fix: 10 Unit-Tests mit vollständigem `MockUserApi`. Getestet: alle 4 `allOf`-Varianten, `allUsersOf` (mit/ohne Name), `deleteOf` (mit/ohne Flag), Fehlerbehandlung bei fehlenden Pflichtflags (`user_add`, `user_update`, `user_delete`). (`tests/cli/UserManagementCliComponentMockTest.cpp`, `tests/mocks/MockUserApi.h`, `tests/CMakeLists.txt`).

---

## Optimierungsmöglichkeiten (priorisiert)

| Priorität | Maßnahme | Begründung |
|-----------|----------|------------|
| **Hoch** | **Fehlende Features abschließen (TODO.md)** | MySQL Support, Cursor, HTTP Exception Handling, Paging |
| **Mittel** | **Benchmark-Suite aufsetzen** | Performance-Messungen für Property-System, Persistenz, Serialisierung |
| **Niedrig** | **CMake modernisieren** (`include_directories` → `target_include_directories`) | Saubereres Target-Modell |
| **Niedrig** | **Hypodermic durch Boost.DI ersetzen** | Aktiver maintained, standardkonformer |
| **Niedrig** | **Makro-basiertes Property-System überarbeiten** | `IMPLEMENT_PROPERTY` könnte durch C++17 `if constexpr` oder C++20 Concepts abgelöst werden |

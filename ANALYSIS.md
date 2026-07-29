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

1. **Fehlendes Mocking-Framework** – Tests nutzen reale Datenbankinstanzen statt Mocks. Dadurch kann die Business-Logik von HTTP-Controllern, CLI-Kommandos, Shared Memory und Message Queues nicht isoliert getestet werden. Nur 10 Testdateien, überwiegend SQLite-Integrationstests.

2. **Services greifen im Konstruktor auf die Datenbank zu** – `MessageQueueService` führt DB-Query im Konstruktor aus (`m_messageQueueRepository->allOf(".*", ".*")`). Setzt voraus, dass DB bereits bootstrapt ist – implizite Abhängigkeit, nicht durch Typsystem abgesichert. DB-Zugriffe sollten in einer `initialize()`-Methode nach dem Bootstrap erfolgen, nicht im Konstruktor.

3. **Signal-Handling mit rohem static Pointer** (`StartupBuilder.cpp`) – `std::signal` + static raw pointer statt modernem `sigaction` + `sigwait` in dediziertem Thread. Nicht thread-safety und potentiell problematisch bei Mehrfachinstanzen.

4. **Minimale Code-Dokumentation** – Kaum Doxygen-Kommentare. Öffentliche API ohne Header-Dokumentation.

5. **Vendored DI Container** – Hypodermic liegt als Copy in `external/` und ist seit 2017 nicht mehr aktiv maintained (letzter Commit). Moderne Alternativen wie Boost.DI wären wartbarer.

6. **Kein Binary-Separation** – Core und Features werden in eine einzige Shared Library (`base_library`) kompiliert. Optionalität der Features ist nur zur Compile-Zeit über das Registrieren im StartupBuilder gegeben – nicht auf Binärebene.

---

## Kürzlich behobene Bugs

1. **SQLite REGEXP fehlt** – Conans SQLite-Bibliothek hat `REGEXP` nicht aktiviert. Runtime-Register via `sqlite3_create_function` im `sqlite::Connection`-Konstruktor gelöst (`src/core/persistence/sqlite3/Connection.cpp:46`).

2. **DatabaseBootstrapPlugin bricht Loop ab** – Bei `db::SQLException` (z. B. PostgreSQL nicht erreichbar) wurde der Fehler weitergereicht, sodass nachfolgende Verbindungen (z. B. SQLite) nie bootstrapt wurden. Fix: `throw` im `catch (db::SQLException)` entfernt, nur noch loggen (`src/core/plugins/DatabaseBootstrapPlugin.cpp:16`).

3. **Bootstrap-Reihenfolge kaputt** – `VirtualGroupBootstrapPlugin` resolved `std::vector<std::shared_ptr<GroupProvider>>` bereits im Konstruktor. Hypodermic konstruiert alle BootstrapPlugins beim `resolve<BootstrapService>()` – also *vor* dem DB-Bootstrap. GroupProvider umfassen u. a. `MessageQueueController`, das `MessageQueueService` injiziert bekommt, das im Konstruktor die (noch nicht bootstrapte) Datenbank abfragt. Fix: `GroupProvider`-Resolution in `onStart()` verschoben, Container wird per Hypodermic auto-injiziert (`src/core/plugins/VirtualGroupBootstrapPlugin.cpp:26`).

4. **`std::size_t` Serialisierung platform-spezifisch** – `#ifdef __APPLE__` in `postgresql/Serialization.h` und `sqlite3/Serialization.h`. Fix: Durch generische SFINAE-Partialspezialisierung ersetzt – erkennt automatisch, ob `std::size_t` bereits durch `uint64_t` abgedeckt ist. (`src/include/base_library/core/persistence/postgresql/Serialization.h:22`, `src/include/base_library/core/persistence/sqlite3/Serialization.h:25`).

---

## Optimierungsmöglichkeiten (priorisiert)

| Priorität | Maßnahme | Begründung |
|-----------|----------|------------|
| **Hoch** | **Mocking-Framework einführen** | Ermöglicht isolierte Unit-Tests für HTTP-Controller, CLI, Shared Memory, Message Queues ohne reale Infrastruktur |
| **Hoch** | **Fehlende Features abschließen (TODO.md)** | MySQL Support, Cursor, HTTP Exception Handling, Paging |
| **Mittel** | **Signal-Handling modernisieren** | `sigaction` + worker thread statt `std::signal` + static pointer |

| **Mittel** | **Benchmark-Suite aufsetzen** | Performance-Messungen für Property-System, Persistenz, Serialisierung |
| **Niedrig** | **Doxygen-Dokumentation für public API ergänzen** | Erleichtert Nutzung der Bibliothek durch Dritte |
| **Niedrig** | **CMake modernisieren** (`include_directories` → `target_include_directories`) | Saubereres Target-Modell |
| **Niedrig** | **Hypodermic durch Boost.DI ersetzen** | Aktiver maintained, standardkonformer |
| **Niedrig** | **Makro-basiertes Property-System überarbeiten** | `IMPLEMENT_PROPERTY` könnte durch C++17 `if constexpr` oder C++20 Concepts abgelöst werden |

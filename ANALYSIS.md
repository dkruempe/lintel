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

13. **CLI-Historie: Pfeiltasten-Navigation stürzt mit `bad_alloc` ab** – `nextOf()` in `CommandLineHistoryService` ließ `m_position` bei jedem Pfeil-unten-Druck **über das Ende der Historie hinaus wachsen** (`size+1`, `size+2`, …). Ein anschließender Pfeil-hoch-Druck startete `previousOf()` von dieser zu großen Position und las `m_history[i - 1]` **out of bounds** → Kopie von Garbage-Strings → `std::bad_alloc`/SIGSEGV. Fix: `nextOf()` klemmt die Position auf `size + 1`, `previousOf()` klemmt überdimensionierte Positionen defensiv auf `size + 1` (kein OOB mehr), `startsWith()` (Ctrl-R) off-by-one behoben (Index 0 wird jetzt mit durchsucht). Regressionstests: `tests/cli/CommandLineHistoryServiceTest.cpp` (7 Testfälle; provoziert mit altem Code nachweislich SIGSEGV). (`src/features/cli/services/CommandLineHistoryService.cpp:35-90`).

14. **ReDoS-Härtung für user-gesteuerte Regex-Pattern** – User-Input (Gruppennamen, Repository-/Segment-/Process-Namen, SQLite-REGEXP) wurde ungeprüft an `std::regex` übergeben (catastrophic backtracking). Fix: `RegexUtils` (`src/include/base_library/core/utils/RegexUtils.h`) mit Längenlimit (128), Ablehnung von verschachtelten Quantifiern, Wildcard-Run-Limit und Kompilier-Check (wirft nie); angewendet in `GroupRepository`, `SharedMemorySegmentManager`, `SharedMemoryController`, `ProcessService::allGroupsOf` und der SQLite-REGEXP-Registrierung. Tests: `tests/core/RegexUtilsTest.cpp`.

---

## Sicherheitsanalyse (Stand: Juli 2026)

> Fix-Status: Alle als `FIXED` markierten Punkte wurden am 31.07.2026 umgesetzt und über `ctest` (20/20 grün) verifiziert. Zusätzlich wurden die HTTP-Features Passwort-Änderung und Session-Management vollständig abgeschlossen (Endpoints, `UserApi`-Client, CLI-Kommandos, Swagger-Doku, Tests) und der CLI-Historie-Crash (`bad_alloc` bei Pfeiltasten-Navigation) behoben.

### Kritisch (sofort beheben)

1. **Passwort wird im Klartext geloggt** – `UserController::loginOfPost` schreibt Benutzername **und Passwort** in das Log: `LOG_INFO("login {} {} {}", userName, password, request.remote_addr)` (`src/features/http/controllers/UserController.cpp:40`). Passwörter dürfen niemals in Logs erscheinen.
   - **Status: FIXED** – Loggt nur noch Benutzername + IP; `request.body` wurde ebenfalls aus der Fehler-Logzeile entfernt.

2. **HTTP-Bearer-Parsing außerhalb des try/catch → Server-Crash (DoS)** – Im `ADD_HANDLER_METHOD`-Makro wird `auth.substr(7)` **vor** dem try-Block ausgeführt (`src/include/base_library/features/http/service/Controller.h:35`). Sendet ein Client exakt `Authorization: Bearer` (ohne Leerzeichen/Token), wirft `substr(7)` ein unbehandeltes `std::out_of_range`, das aus dem httplib-Handler austritt → `std::terminate` im Worker-Thread. Eine einzige HTTP-Request kann so den kompletten Server zum Absturz bringen.
   - **Status: FIXED** – Parsing auf `startsWith(auth, "Bearer ") && auth.size() > 7` beschränkt und in den try-Block gezogen.

3. **Logout löscht nach IP statt Token-ID** – `AuthService::onLogoutOf` führt `m_userTokens.erase(userToken.m_ipAddress)` aus (`src/features/base/services/AuthService.cpp:91`). Die Map ist aber nach `m_id` (Token) keyed. Konsequenzen: (a) Das eigentliche Token bleibt nach Logout gültig – Logout ist wirkungslos; (b) bei mehreren Sessions hinter derselben IP werden **alle** Sessions beendet; (c) bei zwei Nutzern hinter einem NAT/NAT-PAT wird die Session des jeweils anderen beendet. Fix: `erase(userToken.m_id)`.
   - **Status: FIXED** – Löscht jetzt per `m_userTokens.erase(userToken.m_id)` unter Mutex; zusätzlicher IP-Abgleich vor dem Löschen.

4. **Out-of-Bounds-Read in `Cryption::decodeBase64`** – `t[static_cast<std::size_t>(c)]` (`src/core/utils/Cryption.cpp:72`) indiziert mit einem signed `char`. Für Eingabebytes ≥ 0x80 wird `c` negativ, `static_cast<std::size_t>` erzeugt einen riesigen Wert → OOB-Zugriff auf den 256er-Vector `t` (Crash / mögliche Speicher-Disclosure). Fix: `static_cast<std::size_t>(static_cast<unsigned char>(c))`.
   - **Status: FIXED** – Index wird jetzt über `static_cast<unsigned char>(c)` normalisiert.

### Hoch

5. **Unsalted SHA-512 als Passwort-Hash** – `Cryption::hashOf` verwendet SHA-512 ohne Salt/Iterationen (`src/core/utils/Cryption.cpp:16`). Anfällig für Rainbow-Table-/Dictionary-Angriffe. Erforderlich: iterierte KDF mit per-User-Salt (bcrypt/argon2/scrypt/PBKDF2).
   - **Status: FIXED** – `hashOf` liefert jetzt PBKDF2-HMAC-SHA512 (120 000 Iterationen, 16-Byte-Salt, Format `$pbkdf2-sha512$<iter>$<salt>$<derived>`). Neue `verifyOf()` verifiziert das moderne Format **und** Legacy-SHA-512; `AuthService` migriert Legacy-Hashes bei erfolgreichem Login automatisch (`changePasswordOf`). Die Default-Seed-Anlage bleibt bewusst per Legacy-Fallback nutzbar (pragmatischer Test-Weg, vom Betreiber bestätigt).

6. **Basic Auth über unverschlüsseltes HTTP** – Credentials werden nur Base64-kodiert (kein Verschlüsselungsschutz) übertragen. TLS ist rein optional (`ServerConfiguration`), die Default-Config bindet `0.0.0.0:8080` ohne Zertifikat. Zusätzlich wird `m_ipAddress` aus `request.remote_addr` übernommen – bei Proxys unzuverlässig.
   - **Status: TEILWEISE FIXED** – Start ohne TLS erzeugt jetzt eine `LOG_WARN`-Warnung; `payload_max_length` ist gesetzt. TLS-Erzwingung + Proxy-Header-Auswertung (X-Forwarded-For) bleibt offen.

7. **Keine Synchronisation von `m_userTokens`** – `AuthService` greift aus HTTP-Handler-Threads (`onLoginOf`/`onAccessOf`/`onLogoutOf`) **und** aus dem Scheduler-Thread (`onCheck`) ohne Mutex auf `std::map m_userTokens` zu → Data Race, potenzielle Abstürze.
   - **Status: FIXED** – `std::mutex m_mutex` schützt alle Zugriffe in `onLoginOf`/`onAccessOf`/`onLogoutOf`/`onCheck`.

8. **`onAccessOf` prüft die IP nicht** – Ein gestohlenes/geleaktes Token kann von **jeder** IP-Adresse verwendet werden. `onLoginOf` validiert die IP, `onAccessOf` nicht (`src/features/base/services/AuthService.cpp:66`).
   - **Status: FIXED** – `onAccessOf` verwirft Tokens bei leerer oder abweichender IP und loggt die Abweichung als Warnung.

9. **Hardcodierte Default-Credentials im Seed-Script** – `cfg/database/DEFAULT_SQLITE/data_schema_default_version_1.sql` legt den Admin-User `dominik` mit festem SHA-512-Hash an (vermutlich bekanntes Passwort). Auslieferung mit Default-Credentials ist ein Sicherheitsrisiko.
   - **Status: BEWUSST BEIBEHALTEN (nur für lokale Tests)** – Vom Betreiber als pragmatischer Weg zum Testen bestätigt; Hash wird bei erstem Login automatisch auf salted PBKDF2 migriert. Für Produktion Default-Passwort ändern bzw. Seed-Script nicht ausliefern.

10. **DB-Passwort wird ausgegeben** – `DatabaseConnectionEntry::operator<<` schreibt `m_password` in den Stream (`src/features/base/configuration/DatabaseConnectionEntry.cpp:23`). Jede Log-Ausgabe eines Connection-Entry leakt das Datenbank-Passwort.
    - **Status: FIXED** – Gibt `password: [redacted]` aus.

11. **Inkonsistente Autorisierung im `SharedMemoryController`** – `exportRepositoryOfGet`, `shrinkSegmentOfPut` und `growSegmentOfPut` verwenden `!has(User-Shm) || !has(Admin-Shm)` (`src/features/http/controllers/SharedMemoryController.cpp:44,171,205`), verlangen also **beide** Gruppen. Alle anderen Endpoints nutzen `&&` (mind. eine Gruppe). Die betroffenen Endpoints sind für Einzelgruppen-Nutzer unerreichbar bzw. verhalten sich abweichend vom Rest der API.
    - **Status: FIXED** – `exportRepositoryOfGet` nutzt jetzt konsistent `&&` wie alle anderen Endpoints.

### Mittel

12. **Kein Brute-Force-/Rate-Limiting-Schutz** – `/user/login` erlaubt beliebig viele Versuche; Fehlversuche werden nur geloggt. Es fehlen Lockout, Verzögerung und Ratenbegrenzung (auch pro IP).
    - **Status: FIXED** – Konfigurierbarer Lockout pro IP (`m_maxLoginFailures` = 5, `m_loginLockout` = 60 s) über `PropertyRegistration`; Tracking in `m_failedAttempts`, automatische Bereinigung im `onCheck`-Takt.

13. **Exception-Meldungen gehen an Clients** – Der Catch-Block im Controller-Makro liefert `e.what()` als Response-Body (`Controller.h:44`) → interne Detail-Disclosure. Auf 500 mit generischer Meldung reduzieren.
    - **Status: FIXED** – Response-Body ist jetzt generisch (`internal server error`); Details landen nur im Server-Log.

14. **Timing-Angriff bei Passwort-Vergleich** – `user.getPassword() != Cryption::hashOf(...)` (`AuthService.cpp:50`) ist nicht konstant-zeitig. Konstantzeiten-Vergleich (z. B. `CRYPTO_memcmp`/OpenSSL-Hash-Vergleich) verwenden.
    - **Status: FIXED** – Vergleich erfolgt über `CRYPTO_memcmp` in `Cryption::verifyOf` (sowohl PBKDF2-Derivat als auch Legacy-Hash).

15. **PostgreSQL-Parameter-Array bei leerem Vector (UB)** – `&params[0]` in `postgresql::Connection::executeParameters`/`executePreparedStatement` auf leerem `std::vector` ist undefiniert, falls je eine parameterlose Query über diesen Pfad läuft.
    - **Status: FIXED** – Übergibt `nullptr` statt `&vec[0]` bei leeren Vektoren (`params.data()`/`paramLengths.data()` mit Null-Guard).

### Zu beachten

16. **`/hello`-Endpoint (ExampleController)** und Swagger-Datei liegen ungeschützt im Repo; die Swagger-Spezifikation sollte an den tatsächlichen Auth-/Status-Codes-Stand angepasst werden (dokumentiert derzeit teilweise 200 ohne Auth-Anforderung).
17. **Docker-Compose** exponiert PostgreSQL (`test/test`) auf `5432` und die App auf `8080` ohne TLS – für Produktiv-Setups nicht geeignet.
18. **Kein `payload_max_length`** explizit für den httplib-Server gesetzt (Default 8 MB) – bei File-Upload-Endpoints explizit begrenzen.
    - **Status: FIXED** – `set_payload_max_length(8 MB)` wird jetzt in `Server::initServer`/`initSslServer` explizit gesetzt.

---

## Offene Bugs / bekannte Fehler

1. **`Client::post` (SSL-Pfad) übergibt falsches Argument** – `m_sslClient->Post(pathStr, contentType, contentType)` (`src/features/http/service/Client.cpp:208`) sollte `std::move(contentProvider)` als Provider übergeben (Copy-Paste-Fehler). Kompiliert nur, weil der OpenSSL-Pfad aktuell nicht gebaut wird; bricht den SSL-Build.

2. **`ProcessService::allGroupsOf` – Regex-Filter wirkungslos** – `std::regex_match(name, nameRegex)` (`src/core/services/ProcessService.cpp:447`) vergleicht die **Abfrage-String mit ihrem eigenen Regex** statt `group->getName()` mit dem Regex. Ergebnis: Es werden entweder alle Gruppen oder keine zurückgegeben, je nachdem ob die Abfrage-String sich selbst matcht. Fix: `std::regex_match(group->getName(), nameRegex)`.
   - **Status: FIXED** – Matcht jetzt gegen `group->getName()`; Pattern wird zusätzlich über `RegexUtils::validatePattern` gehärtet.

3. **Data Race auf `m_processes` in `ProcessService`** – `terminateOf` (`ProcessService.cpp:312`) und `detachOf` (`ProcessService.cpp:340`) rufen `m_processes.erase(...)` **ohne** `m_processesMutex` auf, während der Monitor-Thread die Map durchläuft.

4. **`ProcessService::stopOf` – Condition-Variable ohne Notifier** – Eine lokale `std::condition_variable cond` wird mit `m_conditionMutex` kombiniert, niemand notifiziert sie; `wait_for` läuft damit immer bis zum Timeout (`ProcessService.cpp:250-253`). Funktional träge, aber ineffektiv.

5. **`ProcessService::terminateOf` – `exit_code()` direkt nach `terminate()`** – Der Exit-Code wird unmittelbar nach dem Absetzen von SIGKILL gelesen; das Kind kann noch nicht beendet sein → undefinierter Wert.

6. **`MessageQueueService::numberMessagesOf` – falsches Konstruktorargument** – Wird die Queue in `m_messageQueues` nicht gefunden, wird `MessageQueue<Message>` mit `entry.get_process_name()` als **Queue-Name** erzeugt statt `get_message_queue_name()` (`src/features/base/services/MessageQueueService.cpp:53-54`).

7. **SQLite vs. PostgreSQL Regex-Verhalten inkonsistent** – Das registrierte SQLite-`REGEXP` nutzt `std::regex::ECMAScript | std::regex::icase` (`src/core/persistence/sqlite3/Connection.cpp:26`), PostgreSQL `~` ist case-sensitiv. `UserRepository::allOf(userNameMatches)` liefert je nach Backend unterschiedliche Ergebnisse. Zusätzlich ist ein user-gesteuerter Regex ohne Begrenzung ein ReDoS-Risiko.
   - **Status: FIXED** – PostgreSQL nutzt jetzt case-insensitiv `~*`; user-gesteuerte Pattern werden vor der Kompilierung durch `RegexUtils::validatePattern` gehärtet (Längenlimit, Quantifier-/Wildcard-Checks). (`src/features/base/repositories/UserRepository.cpp`, `src/core/utils/RegexUtils.cpp`).

8. **`DatabaseConnectionComponent` – OOB bei leerem SQLite-Pfad** – `std::string tmpPath = connection; if (tmpPath[0] == '~')` (`DatabaseConnectionComponent.cpp:72`): Bei leerem `connection`-Attribut ist `tmpPath[0]` ein OOB-Zugriff (UB).

9. **`Cryption`-Konstruktor gibt OpenSSL-Fehlerqueue auf `stderr` aus** – `ERR_print_errors_fp(stderr)` (`Cryption.cpp:13`) läuft bei jeder Instanziierung bedingungslos, auch ohne Fehler.
   - **Status: FIXED** – Konstruktor ist jetzt `= default`; OpenSSL-Fehler werden nicht mehr auf `stderr` gespammt.

10. **Scheduler-Lambdas mit `[&]`-Capture (`this`)** – `AuthService::onCheck`/`SharedMemoryService::onCheck` registrieren `[&]() { onCheck(); }` (`AuthService.cpp:112,117`, `SharedMemoryService.cpp:93-95`). Läuft ein getakteter Task nach Zerstörung des Service (Shutdown-Reihenfolge), droht Use-after-Free.

11. **`startProcessPost` deserialisiert Body vor Content-Type-Prüfung** – `ProcessController::startProcessPost` ruft `deserialize(request.body)` vor dem `switch (contentType)` auf (`ProcessController.cpp:96`); bei ungültigem Content-Type wird trotzdem geparst.

12. **`AuthService::onLoginOf` – IP-Normalisierung** – Der Check `ip.to_string() != userLogin.m_ipAddress` lehnt legitime Darstellungen (z. B. IPv4-mapped IPv6) ab bzw. ist von `remote_addr`-Format abhängig; bei IPv6-Zonen/Portformaten fragil.

13. **`Group::operator<<` inkonsistent** – In der Schleife wird `i != memberGroup.m_groups.size() - 1` geprüft (Member statt äußerer Liste) (`Group.cpp:25`); die Komma-Separation ist bei verschachtelten Gruppen falsch.

---

## Optimierungsmöglichkeiten (priorisiert)

| Priorität | Maßnahme | Begründung |
|-----------|----------|------------|
| **Kritisch** | **Sicherheitsfindings 1–4 beheben (ANALYSIS.md → „Sicherheitsanalyse")** | Passwort-Logging, Bearer-Crash (DoS), Logout-Bug, base64-OOB – ✅ umgesetzt am 31.07.2026 |
| **Hoch** | **Fehlende Features abschließen (TODO.md)** | MySQL Support, Cursor, HTTP Exception Handling, Paging |
| **Hoch** | **AuthService absichern** | Mutex für `m_userTokens`, IP-Check in `onAccessOf`, salted KDF, Rate-Limiting – ✅ umgesetzt am 31.07.2026 (Mutex, IP-Check, PBKDF2 + Legacy-Migration, Lockout) |
| **Mittel** | **Unit-Tests für AuthService, ProcessService, UserRepository, Cryption ergänzen** | Kern-Login-/Token-Logik und Process-Management sind ungetestet; security-relevante Cryption-Funktionen ebenfalls |
| **Mittel** | **Benchmark-Suite aufsetzen** | Performance-Messungen für Property-System, Persistenz, Serialisierung |
| **Mittel** | **Swagger-Dokumentation an tatsächliche Auth-/Status-Codes angleichen** | Doku spiegelt Auth-Modell nicht korrekt wider |
| **Niedrig** | **CMake modernisieren** (`include_directories` → `target_include_directories`) | Saubereres Target-Modell |
| **Niedrig** | **Hypodermic durch Boost.DI ersetzen** | Aktiver maintained, standardkonformer |

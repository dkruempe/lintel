# TODOs

> Stand: 19.08.2026 – ctest 311/311 grün. Alle Punkte unten wurden am Quellcode verifiziert; Datei:Zeile bezieht sich auf den aktuellen Stand.

## Kritische Bugs (Hoch)

### Process
- [x] **`Process::disableAutoStart()` aktiviert Auto-Restart statt ihn zu deaktivieren** – `src/features/base/models/Process.cpp:71-74` setzt `m_autoRestart = true` (Copy-Paste-Fehler). Zusammen mit `m_maxAutoRestarts = -1` führt das in `monitorProcess` (`ProcessService.cpp:160-163`) zu einer **unbegrenzten Restart-Schleife**. Fix: `m_autoRestart = false`.
- [x] **Monitor-Thread beendet Prozess bei Exception** – `ProcessService::run()` (`ProcessService.cpp:142-150`) ruft `monitorProcess()` ohne try/catch auf; `monitorProcess` wirft `std::runtime_error("path invalid")` (`:169`), Filesystem- oder History-Exceptions → Exception entweicht aus dem Thread-Lambda → `std::terminate`. Fix: Loop-Body in try/catch, loggen und weiterlaufen.
- [x] **`detachOf` setzt Promise ggf. doppelt → `std::future_error`** – `ProcessService.cpp:347-355`: zwischen erster Lock-Sektion (Copy) und zweiter kann `monitorProcess` den Eintrag bereits erfüllt/gelöscht haben; `setPromiseValue` wirft dann. Fix: wie `stopOf`/`terminateOf` Map neu nachschlagen.
- [x] **`stopOf` löscht Eintrag eines gerade neu gestarteten Prozesses** – `ProcessService.cpp:266-274`: Bei Auto-Restart kann `monitorProcess` zwischen den beiden Lock-Sektionen das `child` ersetzen; `stopOf` erfüllt die Future mit altem Exit-Code und entfernt den Eintrag, der neue Prozess läuft unmonitoriert weiter. Fix: prüfen ob `getChild()` noch identisch ist bzw. Auto-Restart während `stopOf` unterbrechen.
- [x] **`exit_code()` für laufende Kinder liefert Müll** – `ProcessService.cpp:76-88, 485-489`: `child::exit_code()` ist nur nach `wait()` gültig; `ProcessInfo` für laufende Prozesse trägt den "still active"-Sentinel. Fix: Exit-Code erst nach `wait()` liefern.

### Shared Memory
- [x] **`growOf` wächst Datei, nicht das Live-Mapping → Auto-Extend funktioniert nicht** – `SharedMemoryService.cpp:27-31,103-120`: `managed_mapped_file::grow()` (statisch) wächst die Datei auf Disk, das `m_managedMappedFile` in `m_segments` behält die alte Größe → Allokationen über altem Mapping faulten; Free-Memory-Statistiken stimmen nie. Fix: Mapping neu öffnen/remappen.
- [x] **`maxSize` wird nie erzwungen** – `SharedMemoryService::onCheck` konsultiert nur `isAutoExtend()`/`m_autoExtendEpsilon`, niemals `getMaxSize()`; ein Segment wächst über sein deklariertes Maximum. Zusätzlich wird das globale Property `m_autoExtend` (`SharedMemoryService.cpp:18-20`) nie gelesen.
- [x] **`onCheck` läuft alle 2s auf Scheduler-Thread während Repositories Referenzen halten** – `SharedMemoryService.cpp:103-120`: `grow()` ist laut Boost nicht thread-safe und remappt den Speicher; Repositories (z. B. `SharedMemoryRepository.h:114/173/236/308`, Property-Feature) halten Referenzen aus dem Mapping → Use-after-free/Dangling.
- [x] **`PropertyDataDto::getSize()` zu klein** – `src/include/base_library/features/property/repositories/SharedMemoryPropertyRepository.h:19-21`: `sizeof(ShmString) * 4`, aber das Struct hat **sechs** `ShmString`-Member → Segment wird ~1/3 zu klein dimensioniert. Fix: `* 6`.

### Persistence
- [x] **NULL-Spaltenwerte → UB (Crash)** – `sqlite3/Connection.cpp:134` (`std::basic_string<unsigned char> temp = text;` ohne Null-Check) und `postgresql/Result.cpp:11,31-33` (`std::string value = PQgetvalue(...)`). Fix: Null-Check/`PQgetisnull`/`sqlite3_column_type`.
- [x] **`postgresql::PreparedStatement::execute` – Param-Count wird nicht geprüft** – `postgresql/PreparedStatement.cpp:42-56` → `Connection::executePreparedStatement` (`Connection.cpp:28-39`) übergibt `m_nParams` an `PQexecPrepared`, ohne `params.size() == m_nParams` zu prüfen → Heap-OOB-Read bei zu wenigen Parametern. Fix: Validierung + `SQLException`.
- [x] **LISTEN/NOTIFY-Thread teilt sich die libpq-`PGconn`** – `postgresql/Notify.cpp:14,58-73`: `PQexec`/`PQconsumeInput`/`PQnotifies` aus Hintergrund-Thread neben normalen Queries = Data Race (libpq nicht thread-safe). Fix: dedizierte Connection oder Mutex.
- [x] **SQLite `update_hook` hängt an raw `this` ohne Destruktor** – `sqlite3/Notify.cpp:11-13`: kein `sqlite3_update_hook(db, nullptr, nullptr)` im Destruktor → Use-after-free, wenn `Notify` vor der Connection zerstört wird. Fix: Destruktor ergänzen.
- [x] **Transaction-Destruktor COMMITet statt ROLLBACK** – `postgresql/Transaction.cpp:59-64`, `sqlite3/Transaction.cpp:36-41`: führt `END` (= COMMIT) aus; bei Exception-Unwinding wird Teil-Arbeit stillschweigend committet (Header-Kommentar verspricht Rollback). Fix: `ROLLBACK` im Destruktor.
- [x] **`db::Result::of(pos)` ohne Null-Check** – `core/persistence/Result.cpp:42-54`: bei `m_result == nullptr` wird `m_resultSQLite->of(pos)` ohne Null-Check aufgerufen → Crash bei default-konstruiertem Result. Ebenso `postgresql::Result::of` bei `m_res == nullptr` (OOM/Connection-Verlust), `PQntuples(NULL)` in `Result.cpp:6`.
- [x] **Postfix-`operator++` der Iteratoren falsch** – `core/persistence/Result.h:48-51,86-89` und `core/persistence/Arguments.h:101-104,139-142`: Postfix-Inkrement erhöht `m_pos` und gibt `*this` (schon inkrementiert) zurück statt Kopie des alten Zustands. Fix: Kopie vor Inkrement.
- [x] **Connection-Leaks bei Konstruktor-Fehler** – `postgresql/Connection.cpp:47-50,82-85` (throw ohne `PQfinish`) und `sqlite3/Connection.cpp:46-52,57-60` (throw ohne `sqlite3_close`).
- [x] **`sqlite3_close` vor `sqlite3_finalize`** – `sqlite3/Connection.cpp:66-77`: `sqlite3_close` schlägt mit `SQLITE_BUSY` fehl (ignoriert), Connection wird nie geschlossen, erst danach finalize. Fix: erst alle Statements finalisieren.
- [x] **SQLite-Fehlercodes ignoriert** – `sqlite3/Connection.cpp:117-143,156-185`: `sqlite3_step`/`sqlite3_bind_text`-Ergebnisse werden nicht geprüft (UNIQUE-Verletzung erscheint als Erfolg); `executePreparedStatement` bindet ohne `sqlite3_clear_bindings` → stale Bindings bei weniger Parametern.
- [x] **SQL-Injection über Identifier-Konkatenation** – `postgresql/Notify.cpp:60` (`"LISTEN " + m_tableName`), `Transaction.cpp:40,56` (`SAVEPOINT`/`ROLLBACK TO`), `sqlite3/Transaction.cpp:45,49`. Fix: Identifier validieren/quoten.

### Repositories
- [x] **`MessageQueueRepository::allOf` – Regex-Filter vertauscht** – `MessageQueueRepository.cpp:60-61,78-79`: SQL filtert `name REGEXP ?` mit dem *Process*-Regex und umgekehrt; `MessageQueueController.cpp:35` bekommt daher falsche/leere Ergebnisse.
- [x] **`allMessageQueueNameOf` – falsche Spalte + SQLite-inkompatibles SQL** – `MessageQueueRepository.cpp:142-144`: Queue-Name wird aus Spalte 1 (`process_name`) statt Spalte 0 gelesen; `fetch first row only` (`:124`) ist PostgreSQL-only (SQLite braucht `LIMIT 1`); kein Empty-Result-Check (`result.of(0)` wirft).
- [x] **`UserRepository::allOf` liefert Geister-User bei leerem Ergebnis** – `UserRepository.cpp:275,346`: `func()` wird unbedingt aufgerufen → bei 0 Zeilen wird ein leerer `User` eingefügt. Fix: `if (!lastUserName.empty())`.
- [x] **`GroupRepository::deleteOf` – hardcoded `public.`-Schema + Cache nicht aktualisiert** – `GroupRepository.cpp:184-193`: `public.user_groups_relation` bricht auf SQLite; `m_groupMap`/`m_groups` bleiben unverändert (gelöschte Gruppen weiter sichtbar).
- [x] **`GroupRepository::removeGroupOf` – ungültiges SQL** – `GroupRepository.cpp:233-237`: `delete from group_groups_relation(group_name, base_group_name) where ...` – Spaltenliste ist in `DELETE` ungültig → wirft immer.
- [x] **`GroupRepository::addGroupOf`/`removeGroupOf` aktualisieren den Cache nicht** – `GroupRepository.cpp:195-238`: nur DB wird geändert; `of()`/`allOf()` liefern stale Gruppenmitgliedschaft.

### HTTP / API
- [x] **`JsonSerializable::deserialize(string)` – Non-Object-Root → Server-Crash** – `JsonSerializable.h:51-55` + DTOs ohne `IsObject()`-Guard (`UserPasswordChangeDto.cpp:42` usw.): Body `123` mit `Content-Type: application/json` auf z. B. `PUT /user/password` → rapidjson-Assert/UB. Fix: Parse-Status + `IsObject()` prüfen, `400` liefern.
- [x] **`Client` fällt still auf HTTP/Plaintext zurück** – `Client.cpp:5-10`: `buildSslClient` gibt `nullptr`, sobald keyFile **oder** certFile leer ist → reine CA-TLS-Konfiguration (Server-Verifikation ohne Client-Cert) wird als Klartext-HTTP-Client verwendet; keine Warnung. Fix: SSL-Client auch ohne Client-Cert bauen, sonst warnen.
- [x] **`ContentType::build` vergleicht exakt** – `ContentType.cpp:88-97`: `application/json; charset=utf-8` oder fehlendes Content-Type auf GET → `UNDEFINED` → Controller antworten **403** statt Daten. Fix: Media-Type vor `;` parsen, fehlenden Header nicht als Permission-Fehler behandeln.
- [x] **`PropertyApi` dereferenziert fehlgeschlagene HTTP-Ergebnisse** – `PropertyApi.cpp:14-17,24-30,48-63`: `result->status`/`result->body` ohne `hasResponse`-Prüfung → UB bei Netzwerkfehler. Fix: `HttpClientHelper::hasResponse` verwenden.
- [x] **`UserApi::allOf(groupName, isVirtualGroup)` ignoriert `isVirtualGroup`** – `UserApi.cpp:215-221`: `boolStr` wird berechnet, aber nie benutzt; Route `/user/groups/{name}/{bool}` existiert (`UserController.h:29`) wird aber nie gerufen. Fix: `"/user/groups/" + groupName + "/" + boolStr`.
- [x] **`GET /process/processes/{name}` ignoriert den Namen** – `ProcessController.cpp:36`: `request.matches[1]` wird nie gelesen, es kommt immer `allActiveOf()`.
- [x] **`exportRepositoryOf` liefert 200 mit leerem Body für unbekannte UUID** – `SharedMemoryController.cpp:51-59`: kein `else`-Zweig mit `NotFound`.
- [x] **`addUserPost` hardcodiert `User::Sex::Male`** – `UserController.cpp:315-317`: übergebenes Geschlecht wird verworfen.
- [x] **`ProcessApi::startOf/stopOf/terminateOf` verwerfen das HTTP-Ergebnis** – `ProcessApi.cpp:72-89`: Fehler/401/5xx werden verschluckt → Aufrufer glauben Erfolg.
- [x] **`Server` ignoriert `listen()`-Fehlschlag** – `Server.cpp:92-100`: Bind-Fehler (Port belegt) läuft still weiter ohne HTTP-Server.
- [x] **Unauthentifizierter `/hello`-Endpoint** – `ExampleController.cpp:8-18` (registriert in `HttpFeature.cpp:22`): Beispiel-Handler in Produktion aktiv.
- [x] **`/user/state` verlangt Gruppenmitgliedschaft** – `UserController.cpp:528-532`: Nutzer ohne Default-Gruppe bekommt nach erfolgreichem Login 401 → `isLoggedIn()` liefert false.

### Services / Threading
- [x] **`StartupBuilder` – Data Race + Thread-Leak** – `StartupBuilder.cpp:84,94,144-158`: Signal-Thread startet **vor** `m_abstractServices = ...`; `onShutdown` liest den Vector unsynchronisiert. Zusätzlich bleibt `m_signalThread` bei jeder Exception zwischen `:84` und `:115` joinable → `std::terminate` im Destruktor. Fix: Mutex/Verzögerung + RAII-join.
- [x] **Scheduler/Executor-Threads sterben bei Task-Exceptions** – `SchedulerService.cpp:45`, `ExecutorService.cpp:4`: `funcTask()` ohne try/catch → `std::terminate`; `ExecutorService` liest `m_tasks.empty()` ohne Mutex (Data Race).
- [x] **`AuthService` Lockout-Fenster gleitet nicht** – `AuthService.cpp:222-244`: Fenster ist an `m_firstFailure` verankert; Angreifer kann Versuche über das Fenster verteilen und sperrt nie aus. Fix: Sliding-Window (wie `m_loginAttempts`).
- [x] **`FileService::createFile(0)` erzeugt riesige Sparse-Datei** – `FileService.cpp:89`: `sizeOfFile - 1` unterläuft bei 0 (size_t) → ~2^63-Byte-Sparse-File.
- [x] **`LoggerService::get()` nicht-atomarer Singleton-Check** – `LoggerService.cpp:225-230`: `m_instance == nullptr` unsynchronisiert; `LOG_*` vor `getOrCreate()` installiert stillschweigend den Default-"test"-Logger.

### DTOs / Utils / CLI
- [x] **`GroupDto::serialize` – GROUPS-Schlüssel nur bei leerer Liste** – `GroupDto.cpp:19`: `if (m_groups.empty())` → Untergruppen gehen verloren. Fix: `if (!m_groups.empty())`.
- [x] **`UserNameDto::init(vector<string>)` – UB bei leerem Vector** – `UserNameDto.cpp:57`: `std::transform(..., userNames.begin(), ...)` schreibt über das Ende (nur `reserve`d). Fix: `std::back_inserter`.
- [x] **`MemorySize::deserialize` – ungeprüftes `std::stoul`** – `MemorySize.cpp:14-32`, `Component::convertToBytes` (`Component.cpp:19-33`): Input wie `"kB"` → uncaught `std::invalid_argument` (Startup-Crash); Überlauf ohne Check.
- [x] **`XMLConfigSerializationStrategy::deserialize` – `std::string(nullptr)` UB** – `XMLConfigSerializationStrategy.cpp:74-87`: fehlende Attribute → `std::string(nullptr)`; `PropertyFactory::Create`-Ergebnis wird ohne Null-Check dereferenziert.
- [x] **`tmpPath[0]` bei leerem Pfad** – `SharedMemorySegmentComponent.cpp:26-27`, `LoggerComponent.cpp:114-115`: `path=""` → OOB-Read (in `DatabaseConnectionComponent` bereits gefixt, gleiche Prüfung hier nachziehen).
- [x] **`ProcessCliComponent` – Single-Dash-Long-Flag** – `ProcessCliComponent.cpp:11`: `{"-process-name", "-p"}` → dokumentiertes `--process-name` wird abgelehnt. Fix: `--process-name`.
- [x] **`MessageQueueService::numberMessagesOf` erzeugt Queue als Nebenwirkung** – `MessageQueueService.cpp:49-58`: `open_or_create` legt eine Queue an, nur um die Anzahl zu lesen; Map ist nur nach `get_message_queue_name()` keyed (Kollision zwischen Prozessen mit gleichem Namen).

## Tests / Mocks
- [x] unit tests for ProcessService (start/stop/terminate/restart, allGroupsOf, isLastProcess, of, currentOf, detachOf, group lifecycle)
- [x] unit tests for UserRepository / GroupRepository (CRUD, allOf-Regex, Gruppen-Relationen, virtual groups, addGroupOf/removeGroupOf)
- [ ] Tests für oben gelistete Bugs (Regex-Vertauschung in MessageQueueRepository, `MemorySize` mit `"kB"`, leere Pfade, `PropertyDataDto::getSize`, non-object JSON-Body)

## Security
- [ ] remove hardcoded default admin credentials from seed data (cfg/database/DEFAULT_SQLITE/data_schema_default_version_1.sql) – bewusst beibehalten für lokale Tests; Hash wird bei Login automatisch migriert
- [ ] health/readiness endpoint + Swagger-UI
- [ ] `MemorySize`-Größen beim Grow begrenzen (Resource Exhaustion über Shm-API) und Overflow-Check

## Features (nicht Bugs)
- [ ] mySql Support
- [ ] Cursor implementation
- [ ] HTTP-Paging
- [ ] adds constexpr implementation for better usage and performance
- [ ] backup or archive strategy for shm
- [ ] add better usage of semaphores
- [ ] adds better possibility to manages process automatically

## Docker Support
- [ ] working docker container for testing macOS ?
- [ ] review docker-compose defaults – Postgres test/test auf 5432 und App ohne TLS exponiert

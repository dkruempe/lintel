# TODOs

> Stand: 30.08.2026 – ctest 340/340 grün, 27 Benchmarks (Google Benchmark), `TODO.md`-Feature „Adds better possibility to manage processes automatically" umgesetzt (Backoff/Circuit-Breaker, Group-Monitoring, Ressourcen-Observability, Scheduler). Alle identifizierten Bugs wurden behoben, `constexpr`-Refactor abgeschlossen (siehe ANALYSIS.md für Zusammenfassung).

## Nächste Prioritäten (fokussiert)

1. **Security-Härtung**
   - [x] `MemorySize`-Größen beim Grow begrenzen (Resource Exhaustion über Shm-API) und Overflow-Check
   - [x] Health/Readiness endpoint (`GET /health`, `GET /ready`, unauthentifiziert, 200 JSON)
   - [x] Swagger-Spezifikation an tatsächlichen Auth-/Status-Code-Stand anpassen (Spec-only, `/hello`, `/health`, `/ready`; kein Serving/Embedding)
2. **Shared Memory Robustheit**
   - [x] Backup-Strategie geprüft → **verworfen** (Property-Persistenz läuft bereits über DB-Priorität + Shadowing; kein separater SHM-Backup-Mechanismus)
   - [x] Bessere Semaphore-Nutzung + Resource-/Overflow-Checks
       – Quer-Prozess-Semaphore pro Segment (`shm_<name>_remap_sem`) um Grow/Shrink/Remap + construct*-Pfade;
         `MessageQueue::sendOfWithTimeout` + Drop-Zähler (HistoryService nutzt `m_sendTimeout`);
         `serialize`/`getMap` unter `shm_property_mutex_upgradable` (shared ReadLock via `ReadLockGuard`);
         2 neue Unit-Tests (Queue-Overflow). Boost-Wrap separat zurückgestellt.

## Tests / Mocks

- [ ] Tests für gefundene Bugs (Regex-Vertauschung in MessageQueueRepository, `MemorySize` mit `"kB"`, leere Pfade, `PropertyDataDto::getSize`, non-object JSON-Body)
- [ ] PostgreSQL server-side Cursor Tests (`[pg]`-Tag) – erfordert laufendes Postgres (`docker compose up postgres`, user/pass/db: test/test/test auf Port 5432)

## Security

- [ ] Remove hardcoded default admin credentials from seed data (`cfg/database/DEFAULT_SQLITE/data_schema_default_version_1.sql`) – bewusst beibehalten für lokale Tests; Hash wird bei Login automatisch migriert

## Features

- [x] Backup or archive strategy for shared memory → verworfen (siehe Nächste Prioritäten); Persistenz über DB-Priorität + Shadowing
- [x] Bad usage of semaphores (`shm_<name>_remap_sem` pro Segment; MessageQueue-Timeouts; Named-Mutex um serialize) → Details unter „Nächste Prioritäten“
- [x] Adds better possibility to manage processes automatically
    - Backoff/Circuit-Breaker beim Auto-Restart (`restartDelay`/`restartDelayMax`/`maxRestartRate`/`minUptime`/`restartWindow`), kein Endlos-Restart-Loop
    - ProcessGroup-Monitoring reaktiviert (Recovery von `failed`-Prozessen), Scheduler (`restartInterval`/`activeFrom`/`activeTo`)
    - Ressourcen-Observability (uptime/CPU/Mem) via `ProcessResourceReader` (Linux/macOS), Notify-Schwellwerte (`cpuNotify`/`memNotify`)
    - HTTP: `PUT /process/restart/{id}`, `POST /process/reset/{id}`, `GET /process/health/{id}`; CLI: `restart_process`/`reset_process`/`show_process_details`
    - Bugfixes: sicheres `exit_code`/`wait`, null-Promise, spawn/wait nicht mehr unter `m_processesMutex`
- [ ] MySQL Support (zurückgestellt – aktuell nicht priorisiert)

## Docker Support

- [ ] Working docker container for testing on macOS
- [ ] Review docker-compose defaults – Postgres `test/test` auf 5432 und App ohne TLS exponiert

## Tech-Debt / Bekannte Macken

- [ ] `config.h.in` Typo: `MSG_QUUEUE_CONTENT_SIZE` (doppeltes U) – wird durch CMake substituiert, der Name ist irreführend
- [ ] Neue Test-Dateien müssen explizit in `tests/CMakeLists.txt` zur `ADD_EXECUTABLE(base_tests ...)`-Liste hinzugefügt werden (kein Auto-Discovery)
- [ ] Neue Source-Dateien müssen explizit in `src/CMakeLists.txt` zu `headers`/`sources`-Listen hinzugefügt werden (kein Globbing)

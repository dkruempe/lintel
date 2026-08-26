# TODOs

> Stand: 26.08.2026 – ctest 331/331 grün, 27 Benchmarks (Google Benchmark). Alle identifizierten Bugs wurden behoben (siehe ANALYSIS.md für Zusammenfassung).

## Tests / Mocks

- [ ] Tests für gefundene Bugs (Regex-Vertauschung in MessageQueueRepository, `MemorySize` mit `"kB"`, leere Pfade, `PropertyDataDto::getSize`, non-object JSON-Body)
- [ ] PostgreSQL server-side Cursor Tests (`[pg]`-Tag) – erfordert laufendes Postgres (`docker compose up postgres`, user/pass/db: test/test/test auf Port 5432)

## Security

- [ ] Remove hardcoded default admin credentials from seed data (`cfg/database/DEFAULT_SQLITE/data_schema_default_version_1.sql`) – bewusst beibehalten für lokale Tests; Hash wird bei Login automatisch migriert
- [ ] Health/Readiness endpoint + Swagger-UI
- [ ] `MemorySize`-Größen beim Grow begrenzen (Resource Exhaustion über Shm-API) und Overflow-Check

## Features

- [ ] MySQL Support
- [ ] Adds `constexpr` implementations for better usage and performance
- [ ] Backup or archive strategy for shared memory
- [ ] Add better usage of semaphores
- [ ] Adds better possibility to manage processes automatically

## Docker Support

- [ ] Working docker container for testing on macOS
- [ ] Review docker-compose defaults – Postgres `test/test` auf 5432 und App ohne TLS exponiert

## Tech-Debt / Bekannte Macken

- [ ] `config.h.in` Typo: `MSG_QUUEUE_CONTENT_SIZE` (doppeltes U) – wird durch CMake substituiert, der Name ist irreführend
- [ ] Neue Test-Dateien müssen explizit in `tests/CMakeLists.txt` zur `ADD_EXECUTABLE(base_tests ...)`-Liste hinzugefügt werden (kein Auto-Discovery)
- [ ] Neue Source-Dateien müssen explizit in `src/CMakeLists.txt` zu `headers`/`sources`-Listen hinzugefügt werden (kein Globbing)

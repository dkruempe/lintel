# IPC für Driver-/Unit-Framework - Umsetzungsplan

Stand: 2026-10-20  
Branch: `feat/ipc-driver-unit-framework`  
Basis: `docs/ipc-potencials.md`

Ziel: IPC (Shared Memory, Event Bus, Message Queue) produktiv für Driver-/Unit-Framework nutzbar machen - State in SHM, Changes über Event Bus, Commands über Message Queue.

## Verwendungsrichtlinien
| Kanal | Zweck | Nutzung | Nicht nutzen |
|-------|-------|---------|--------------|
| **Shared Memory** | Zustand | Register, Sensorwerte, Unit-State, große Strukturen, häufig gelesen | Kleine Events, Commands mit Ack/Timeout, Kontrollfluss |
| **Event Bus** | Notifications/Signale | Statusänderungen, Property-Changes, Heartbeats, Alarme (Broadcast/P2P) | Request/Reply mit Retries, große Payloads |
| **Message Queue** | Commands/Work | SET_CONFIG, START/STOP, CALIBRATE, RPC-ähnlich, Jobs | Sehr hoher Broadcast, reiner State-Publish |

## Phase A - Kurzfristig (hoher Hebel) [Priorität: Hoch]

| ID | Task | Status | Dateien | Tests | Akzeptanzkriterien | Aufwand |
|----|------|--------|---------|-------|---------------------|---------|
| **SHM-3** | Segment-Lifecycle-Hooks (Grow/Shrink/Remap) | [x] Erledigt | `src/include/lintel/features/base/services/ISharedMemoryService.h`, `src/include/lintel/features/base/services/SharedMemoryService.h`, `src/features/base/services/SharedMemoryService.cpp` | `tests/features/SharedMemoryServiceTest.cpp` (erweitern) | Hooks `onBeforeGrow/AfterGrow/onBeforeShrink/AfterShrink/onRemap` werden aufgerufen. Callbacks thread-safe, weak_ref oder safe unreg. Generation-Change triggert `onRemap`. | Niedrig-Mittel |
| **SHM-4** | Basis-Metriken (belegte Bytes, Grow/Shrink/Recovery-Count, Generation) | [x] Erledigt | `src/include/lintel/features/base/models/SharedMemorySegmentStats.h` (neu), `src/include/lintel/features/base/models/SharedMemorySegmentInfo.h` (erweitern), `src/include/lintel/features/base/services/SharedMemoryService.h/cpp` | `tests/features/SharedMemoryServiceTest.cpp` | Metriken pro Segment abrufbar, Atomics/thread-safe, keine Hotpath-Kosten. | Niedrig |
| **EB-1** | Typisierte/schema-basierte Events (Type-Tags + Version) | [x] Erledigt | `src/include/lintel/features/base/events/Event.h` (erweitern), `src/include/lintel/features/base/events/EventSchema.h` (neu), Beispiele aktualisieren | `tests/events/EventBusTest.cpp` | `schemaId` + `schemaVersion` (uint16) Felder vorhanden, Default-Werte rückwärtskompatibel, Roundtrip getestet. | Mittel |
| **EB-2** | Sticky / Last-Value-Events | [x] Erledigt | `src/include/lintel/features/base/events/EventBus.h/cpp`, `src/include/lintel/features/base/configuration/EventBusEntry.h/cpp` (falls nötig), `src/include/lintel/features/base/services/EventBusService.h/cpp` | `tests/events/EventBusTest.cpp`, `tests/property/PropertyChangeTest.cpp` | Pro Topic letzter Event gecacht. Neuer Subscriber erhält letzten Wert (falls vorhanden). `sticky` + `stickyTtlMs` konfigurierbar. Reset löscht Cache. TTL verfällt korrekt. | Mittel |
| **EB-4** | Backpressure & Drop-Policies (DropOldest/DropNewest/Fail/Block) | [x] Erledigt | `src/include/lintel/features/base/events/EventBus.h/cpp`, `src/include/lintel/features/base/configuration/EventBusEntry.h` | `tests/events/EventBusTest.cpp` | `DropPolicy` Enum, konfigurierbar per Bus. `publish`/`publishToSubscribers` respektieren Policy. `droppedEventsOf()` erweitert. Default = `Fail` (rückwärtskompatibel). | Mittel |
| **EB-6** | Basis-Observability EventBus (Queue-Tiefen, Drops, Counts) | [x] Erledigt | `src/include/lintel/features/base/events/EventBus.h/cpp`, `src/include/lintel/features/base/events/IEventBus.h` (falls benötigt) | `tests/events/EventBusTest.cpp` | Getter für Queue-Tiefen (bus + subscriber), Drop-Counts pro Topic optional, `sequenceOf()` bereits vorhanden. Keine Locks im Hotpath. | Niedrig-Mittel |
| **MQ-3** | Overflow-/Dead-Letter-Policy für MessageQueue | [x] Erledigt | `src/include/lintel/features/base/msg/MessageQueueCore.h`, `src/features/base/msg/MessageQueueCore.cpp`, `src/include/lintel/features/base/msg/MessageQueue.h`, `src/include/lintel/features/base/services/MessageQueueService.h/cpp` | `tests/features/MessageQueue*` (erweitern) | `OverflowPolicy { Fail, DropNewest, DropOldest, BlockWithTimeout, DeadLetter }`. DLQ `<name>.dlq` konfigurierbar. Bestehende API kompatibel (Defaults unverändert). | Mittel |

**Phase A DoD:** Driver kann State in SHM ablegen, typisierte Events mit Sticky+Backpressure publischen, Commands via MQ mit Overflow-Policy empfangen. Lifecycle-Hooks + Basis-Metriken vorhanden.

## Phase B - Mittelfristig (Robustheit + API-Level)

| ID | Task | Status | Dateien | Tests | Akzeptanzkriterien | Aufwand |
|----|------|--------|---------|-------|---------------------|---------|
| **MQ-1** | Owner-/Liveness-Tracking + Orphan-Recovery (analog SHM) | [~] In Bearbeitung | `src/include/lintel/features/base/models/MessageQueueOwner.h` (neu), `src/include/lintel/features/base/repositories/MessageQueueRepository.h/cpp`, `src/features/base/services/MessageQueueService.cpp`, `src/features/base/msg/MessageQueueCore.cpp`, `src/app/plugins/MessageQueueBootstrapPlugin.cpp`, DB-Schema (`cfg/database/*/init_schema*.sql`) | `tests/plugins/MessageQueueBootstrapPluginTest.cpp` (erweitern) | Owner (PID/Host/Process/Start/Heartbeat), Heartbeat-Timer. Stale Owner → Recovery (bereinigt Orphans). Einheitlicher Mechanismus wie SHM. DB-Felder erweitert. | Mittel-Hoch |
| **MQ-2** | Höheres API-Level (Request/Reply, Correlation-ID, Timeout, typed Payload) | [~] In Bearbeitung | `src/include/lintel/features/base/msg/CorrelationId.h` (neu), `src/include/lintel/features/base/msg/MessageQueueRequestReply.h` (neu), Beispiele | Neue Tests `tests/features/MessageQueueRpcTest.cpp` | `Request<TReq,TResp>`, Correlation-ID, ReplyTo, `call()` sync mit Timeout, Handler-Registry. Typensicher, parallele Calls. | Hoch |
| **SHM-1** | Typed SHM-Repositories (Object/Array/RingBuffer/Queue) | [~] In Bearbeitung | `src/include/lintel/core/shm/SharedMemoryRepository.h` (neu), `src/include/lintel/core/shm/containers/*.h` (neu), `src/include/lintel/features/base/services/SharedMemoryRepositoryRegistry.h` (neu) | Neue Tests `tests/features/SharedMemoryRepositoryTest.cpp` | Generisch `SharedMemoryRepository<T>`, generation-aware, nutzt SHM-3 Hooks. DB-Shadow konsistent. | Mittel |
| **EB-3** | Prioritäten / Ordering-Garantien | [~] In Bearbeitung | `src/include/lintel/features/base/events/Event.h` (Priorität), `src/include/lintel/features/base/events/EventBus.h/cpp` | `tests/events/EventBusTest.cpp` | `Priority { Critical,High,Normal,Low }`, separate Ringe oder Prioritätslisten. Höchste Prio zuerst, FIFO innerhalb Prio. Konfigurierbar. | Mittel |
| **EB-5** | Topic-Filter / Hierarchie (Wildcard) | [~] In Bearbeitung | `src/include/lintel/features/base/events/TopicMatcher.h` (neu), `src/include/lintel/features/base/events/EventBus.h/cpp` | `tests/events/EventBusTest.cpp` | Wildcards `*`, `?` oder Prefix `gpio.*`. Pattern-Subscriptions, abwärtskompatibel zu exakten Topics. | Mittel |
| **MQ-5** | Ops-Endpoints (HTTP/CLI) - Stats/Depth/Lag/Purge | [ ] Erledigt | `src/features/cli/components/MessageQueueCliComponent.cpp`, `src/include/lintel/features/http/controllers/MessageQueue*`, `src/features/http/controllers/MessageQueue*`, DTOs | `tests/cli/MessageQueueCliComponentTest.cpp`, `tests/http/MessageQueueControllerTest.cpp` | CLI erweitert (Depth, Owner/Liveness, DLQ), HTTP GET/POST Stats/Purge. Read-first, Mutationen konservativ. | Niedrig-Mittel |

## Phase C - Später / bei Bedarf

| ID | Task | Status | Dateien | Tests | Akzeptanzkriterien | Aufwand |
|----|------|--------|---------|-------|---------------------|---------|
| **SHM-2** | Zero-Copy Memory-Views / Slices (generation-aware) | [ ] Erledigt | `src/include/lintel/core/shm/ShmView.h` (neu) | Neue Tests | `ShmView<T>`/`ShmSlice<T>`, Span-ähnlich, generation-check, Invalidation via Hooks. | Hoch |
| **MQ-6** | SHM-backed MessageQueue (Zero-Copy) | [ ] Erledigt | `src/include/lintel/features/base/msg/MessageQueueShmCore.h` (neu), `src/features/base/msg/MessageQueueShmCore.cpp` | Neue Tests + Benchmarks | API-kompatibel zu `MessageQueueCore`, Feature-Flag. Einheitliches SHM-Modell. | Hoch |
| **SHM-5** | Automatisches Cleanup orphaned Objects (Service) | [ ] Erledigt | `src/include/lintel/features/base/services/SharedMemoryCleanupService.h` (neu), `src/features/base/services/SharedMemoryCleanupService.cpp` | Neue Tests | Scheduler-basiert, erkennt Orphans, Dry-Run/Best-Effort, nie aktive Owner anfassen, idempotent. | Mittel |

## Umsetzungshinweise

- **Rückwärtskompatibilität:** Defaults unverändert. Nur erweitern, keine Layout-Brüche.
- **Cross-Process:** Tests für Sticky/SHM-Repos/MQ-Recovery wo möglich.
- **Thread-Safety:** Atomics nutzen, Locks minimieren, SHM-Lock-Recovery wiederverwenden.
- **CMake:** Neue Dateien in `src/CMakeLists.txt` / `tests/CMakeLists.txt` eintragen (keine Globs).
- **Namespace/Format:** `namespace lintel` für öffentliche Symbole, `clang-format` konform.
- **CI:** `./scripts/check-format.sh master`, `./scripts/check-namespace.sh` vor Commit.

## Fortschritt

**Phase A:** 7/7 erledigt  
**Phase B:** 0/6 erledigt  
**Phase C:** 0/3 erledigt  

Letztes Update: 2026-10-20

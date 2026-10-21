# IPC-Potenziale: Shared Memory, Event Bus, Message Queue

Stand: 2026-10-19  
Kontext: lintel (pre-1.0) + geplantes Driver-/Unit-Framework

---

## 1. Shared Memory

### Ist-Zustand
- Shadow-Repositories (DB ↔ SHM)
- Grow/Shrink + Generation-Counter
- Bounded Semaphore + Orphan-Lock-Recovery (PR #2)
- Property-Change über Event Bus
- Readiness-Check (`shared_memory` in `/ready`)

### Offene Potenziale

| ID | Potenzial | Priorität | Aufwand | Status | Notizen |
|----|-----------|-----------|---------|--------|---------|
| SHM-1 | Generische / typed `SharedMemoryRepository<T>` (Object, Array, Ring-Buffer, Queue) | Hoch | Mittel | ☐ | Basis für Driver/Units |
| SHM-2 | Zero-Copy Memory-Views / Slices | Mittel | Hoch | ☐ | Für High-Frequency-Daten (GPIO, Sensoren) |
| SHM-3 | Segment-Lifecycle-Hooks (Grow/Shrink/Remap) | Hoch | Niedrig–Mittel | ☐ | Units können Pointer invalidieren |
| SHM-4 | Metriken (belegte Bytes, Grow-Count, Recovery-Count, Generation) | Mittel | Niedrig | ☐ | Observability |
| SHM-5 | Automatisches Cleanup orphaned Objects (Bootstrap oder API) | Mittel | Mittel | ☐ | `cleanup.cpp` ist manuell |

### Abhängigkeiten
- SHM-1 und SHM-3 sind besonders relevant für das Driver-/Unit-Framework

---

## 2. Event Bus

### Ist-Zustand
- Broadcast + Point-to-Point über Shared Memory
- Property-Change-Notifications
- Beispiele: `event_bus.cpp`, `property_change.cpp`

### Offene Potenziale

| ID | Potenzial | Priorität | Aufwand | Status | Notizen |
|----|-----------|-----------|---------|--------|---------|
| EB-1 | Typisierte / schema-basierte Events (Type-Tags + Payload-Layout) | Hoch | Mittel | ☐ | Robustes Driver↔Unit-Protokoll |
| EB-2 | Sticky / Last-Value-Events | Hoch | Mittel | ☐ | Neuer Subscriber bekommt letzten Status |
| EB-3 | Prioritäten / Ordering-Garantien | Mittel | Mittel | ☐ | Alarm > Status > Telemetrie |
| EB-4 | Backpressure & Drop-Policies (drop-oldest, drop-newest, block, fail) | Hoch | Mittel | ☐ | Wichtig für RT-nahe Szenarien |
| EB-5 | Topic-Filter / Hierarchie (`gpio.*`, `unit.42.status`) | Mittel | Mittel | ☐ | Lose Kopplung |
| EB-6 | Observability (Drops, Queue-Tiefen, Latenz) | Mittel | Niedrig–Mittel | ☐ | Analog Logger/HTTP |

### Abhängigkeiten
- EB-1, EB-2, EB-4 sind die wichtigsten Hebel für das Driver-/Unit-Framework

---

## 3. Message Queue

### Ist-Zustand
- Dünner Wrapper um `boost::interprocess::message_queue`
- Blocking / Non-blocking
- Bootstrap-Plugin vorhanden
- Kein Owner-Tracking → nur manuelles Cleanup

### Offene Potenziale

| ID | Potenzial | Priorität | Aufwand | Status | Notizen |
|----|-----------|-----------|---------|--------|---------|
| MQ-1 | Owner-/Liveness-Tracking + Orphan-Recovery (analog SHM) | Hoch | Mittel–Hoch | ☐ | Aktuell Schwachstelle |
| MQ-2 | Höheres API-Level (Request/Reply, Correlation-ID, Timeout, typed Payload) | Hoch | Hoch | ☐ | Komfort + Sicherheit |
| MQ-3 | Explizite Overflow-/Dead-Letter-Policy | Hoch | Mittel | ☐ | Verhalten bei voller Queue |
| MQ-4 | Integration / einheitliches Modell mit Event Bus | Mittel | Mittel | ☐ | Klare Rollen: EventBus vs. MQ |
| MQ-5 | HTTP/CLI: Queue-Tiefen, Consumer-Lag, Purge | Mittel | Niedrig–Mittel | ☐ | Ops-Sichtbarkeit |
| MQ-6 | Optional: SHM-Ring-Buffer-Queue (Zero-Copy) | Niedrig | Hoch | ☐ | Nur bei sehr hohen Raten |

### Abhängigkeiten
- MQ-1 und MQ-3 schließen wichtige Betriebs-Lücken
- MQ-2 wird relevant, sobald Units Commands austauschen

---

## 4. Empfohlene Reihenfolge (für Driver-/Unit-Framework)

### Phase A – Kurzfristig (hoher Hebel)
- [ ] **EB-1** Typisierte Events
- [ ] **EB-2** Sticky / Last-Value
- [ ] **EB-4** Backpressure & Drop-Policies
- [ ] **MQ-3** Overflow-Policy
- [ ] **SHM-3** Lifecycle-Hooks
- [ ] **EB-6 / SHM-4** Basis-Observability

### Phase B – Mittelfristig
- [ ] **MQ-1** Owner-Tracking / Orphan-Recovery
- [ ] **MQ-2** Höheres Message-Queue-API
- [ ] **SHM-1** Typed SHM-Repositories
- [ ] **EB-3** Prioritäten
- [ ] **EB-5** Topic-Filter
- [ ] **MQ-5** Ops-Endpoints

### Phase C – Später / bei Bedarf
- [ ] **SHM-2** Zero-Copy Views
- [ ] **MQ-6** SHM-backed Queue
- [ ] **SHM-5** Automatisches Cleanup

---

## 5. Rollen-Modell (Vorschlag)

| Kanal | Empfohlene Verwendung |
|-------|------------------------|
| **Shared Memory** | Zustand (Status, Konfiguration, Spiegel, große Strukturen) |
| **Event Bus** | Signale, Status-Änderungen, Notifications (klein, häufig) |
| **Message Queue** | Commands, Request/Reply, größere Payloads |

---

## 6. Offene Fragen

- [ ] Sollen Event-Typen versioniert werden (Schema-Evolution)?
- [ ] Brauchen wir für RT-Patch-Szenarien harte Latenz-Budgets / Prioritäten?
- [ ] Soll Orphan-Recovery für Message Queues denselben Mechanismus wie SHM nutzen?
- [ ] Wie stark soll die HTTP/CLI-Oberfläche in die IPC-Observability eingreifen?

---

## 7. Referenz

- Architektur: https://dkruempe.github.io/lintel/architecture.html
- Beispiele: `event_bus.cpp`, `property_change.cpp`, `message_queue.cpp`, `cleanup.cpp`
- PR #2: SHM Lock-Recovery (A6)

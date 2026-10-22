# Shared-Memory Payload-ABI — Umsetzungsplan für Stufe 2–4

Stand: 2026-10-22  
Basis: `docs/shm-migration-strategy.md` §5 (Backlog Stufe 2, 3, 4)  
Branch: aktueller Arbeitsstand

Ziel: Der Inhalt eines Segments wird nicht mehr als Ganzes verworfen, nur weil
sich die Buchhaltung der Bibliothek ändert. Jedes benannte Objekt bekommt eine
eigene, versionierte Hülle; Nutzdaten und interne Struktur (Accessor, Header,
Record) werden getrennt versioniert. Die Bibliothek kann dadurch selbst
entscheiden, welche Objekte sie verwerfen darf.

## Leitentscheidungen

1. **Ein stabiler Header pro benanntem Objekt.** `ShmObjectHeader` ist ein
   `trivially copyable` POD mit fester Größe (64 Byte) und steht am Anfang des
   benannten Objekts. Seine Felder `headerVersion` und `payloadVersion` sind
   getrennt: Änderungen an der Hülle invalidieren die Nutzdaten nicht.
2. **Nutzdaten liegen in einer separaten Allokation.** Der Header speichert
   `payloadOffset`, `payloadBytes` und `payloadAlignment` relativ zu sich
   selbst. Ein Layout-Wechsel der Bibliothek verschiebt damit nie die Nutzdaten.
3. **Persistenzklasse pro Objekt.** `Volatile`, `Derived`, `Persistent` werden
   beim Registrieren mitgegeben und im Header gespeichert. `Persistent` ist der
   sichere Default; nur `Volatile`/`Derived` dürfen ohne Migrator verworfen
   werden.
4. **Migration pro Typ und Version.** `ShmPayloadMigrations` registriert
   `v(n) → v(n+1)`-Schritte. Die Bibliothek verkettet sie beim Öffnen eines
   Objekts, bevor der erste Zugriff auf die Nutzdaten erfolgt.
5. **Segmentweite Entscheidung bleibt der letzte Fallback.** Der Migrator aus
   `SegmentLayoutPolicy` hat weiterhin Vorrang. Ohne Migrator darf ein Segment
   automatisch neu angelegt werden, wenn **alle** enthaltenen Objekte
   `Volatile` oder `Derived` sind und ihre Header lesbar sind.

## Phasen und Tasks

### Phase 1 — Hülle und Migrationskern (Stufe 2 + 4)

| ID | Task | Dateien | Tests | Akzeptanzkriterien |
|----|------|---------|-------|---------------------|
| **ABI-1** | `ShmObjectHeader` + `ShmPersistence` + `ShmObjectOptions` | `src/include/lintel/core/shm/ShmObjectHeader.h` (neu) | `tests/features/ShmObjectMigrationTest.cpp` | 64 Byte, `trivially copyable`, `headerVersion`/`payloadVersion` getrennt, Magic- und State-Prüfung. |
| **ABI-2** | Typ-ID und Migrationsregistry `v(n) → v(n+1)` | `ShmObjectHeader.h`, `src/features/base/services/ShmObject.cpp` (neu) | `ShmObjectMigrationTest.cpp` | Typ-ID stabil innerhalb eines Builds, Registrierung pro Typ/Version, Verkettung mehrerer Schritte, `reset()` für Tests. |
| **ABI-3** | `ShmObject` als typ-erased Hülle (bisher `ShmCellRecord`) | `src/include/lintel/core/shm/ShmObject.h` (neu), `src/features/base/services/ShmObject.cpp` | `ShmObjectMigrationTest.cpp`, `SharedMemoryRepositoryTest.cpp` | Header + separate Payload-Allokation, `resolve()` migriert oder legt neu an, `isStale()`/`generation()` bleiben. |
| **ABI-4** | `ShmCell`/`shm::SharedMemoryRepository<T>` auf `ShmObject` umstellen | `src/include/lintel/core/shm/SharedMemoryRepository.h` | `SharedMemoryRepositoryTest.cpp` | Konstruktor und `of()` akzeptieren `ShmObjectOptions`; `header()`, `persistence()`, `payloadVersion()` verfügbar. Bestehende Aufrufe bleiben quellkompatibel. |
| **ABI-5** | Segment-Layout auf 3 anheben | `src/features/base/services/SharedMemoryService.cpp` | `SharedMemoryServiceTest.cpp` | Marker wird auf 3 gestempelt; Tests prüfen 3 statt 2. |

**Phase-1-DoD:** Ein Objekt mit alter `payloadVersion` wird über eine
registrierte Kette migriert; ohne Migration und mit `Persistent` bricht der
Start ab. Nutzdaten liegen physisch getrennt vom Header.

### Phase 2 — Persistenzklassen und segmentweite Entscheidung (Stufe 3)

| ID | Task | Dateien | Tests | Akzeptanzkriterien |
|----|------|---------|-------|---------------------|
| **ABI-6** | Persistenzklasse in alle Registrierungspfade | `ShmObjectOptions`, `ShmConstructors.h`, `features/base/repositories/SharedMemoryRepository.h`, `SharedMemoryPropertyRepository.h` | `SharedMemoryRepositoryTest.cpp`, `SharedMemoryServiceTest.cpp` | Stack A und B können `Volatile`/`Derived`/`Persistent` setzen; Property-Spiegel = `Derived`; Default = `Persistent`. |
| **ABI-7** | Segment-Inspektion vor dem Verwerfen | `SharedMemoryService.cpp`, `ShmObjectHeader.h` | `SharedMemoryServiceTest.cpp` | Iteration über benannte Objekte, Header lesen, `allDiscardable` nur wenn alle Header gültig und nicht `Persistent`. |
| **ABI-8** | Auto-Recreate für rein flüchtige/abgeleitete Segmente | `SharedMemoryService.cpp` | `SharedMemoryServiceTest.cpp` | Alt-Segment mit nur `Volatile`/`Derived` wird ohne Env/Migrator neu angelegt; ein `Persistent`-Objekt verhindert das und liefert Pfad + Anleitung. |
| **ABI-9** | Objekt-Migration bei Versionskonflikt | `ShmObject.cpp` | `ShmObjectMigrationTest.cpp` | `Persistent` ohne Migration → `runtime_error`; `Volatile`/`Derived` ohne Migration → Objekt neu; mit Migration → Daten übernommen. |

**Phase-2-DoD:** Die Bibliothek unterscheidet selbst zwischen „darf weg" und
„muss migriert werden"; der Operator-Eingriff ist nur noch bei `Persistent`
nötig.

### Phase 3 — Bestehende Objekttypen integrieren (Stufe 2/3 flächendeckend)

| ID | Task | Dateien | Tests | Akzeptanzkriterien |
|----|------|---------|-------|---------------------|
| **ABI-10** | `ShmConstructors`-Helfer auf Header umstellen | `ShmConstructors.h` | `SharedMemoryRepositoryTest.cpp`, `ShmViewTest.cpp`, `SharedMemoryCleanupServiceTest.cpp` | `constructArray/Map/Vector/Object/ObjectWith` liefern weiter `T&`, legen aber Header + Payload an; `constructString` bleibt Wert-Allokation. |
| **ABI-11** | EventBus als `Volatile` registrieren | `EventBusService.cpp`, `SharedMemoryBootstrapPlugin.cpp` | `EventBusServiceTest.cpp`, `SharedMemoryBootstrapPluginTest.cpp` | Bus-Objekt hat Header; Bootstrap liest die Payload über den Header; alte Segmente werden nicht als gültiger Bus interpretiert. |
| **ABI-12** | MessageQueueShmCore als `Volatile` registrieren | `ShmSegmentAccessor.h`, `MessageQueueShmCore.cpp` | `MessageQueueShmCoreTest.cpp` | Queue-Header liegt in einer versionierten Hülle; ein Versionskonflikt verwirft die flüchtige Queue, ohne das Segment zu löschen. |
| **ABI-13** | `ShmView` und Cleanup header-aware machen | `ShmView.cpp`, `SharedMemoryCleanupService.cpp` | `ShmViewTest.cpp`, `SharedMemoryCleanupServiceTest.cpp` | Views zeigen auf die Payload, nicht auf den Header; Cleanup gibt Header und Payload frei, ohne fremde Objekte zu treffen. |

**Phase-3-DoD:** Jedes benannte Objekt im Segment trägt einen Header; die
bestehenden APIs bleiben kompatibel.

### Phase 4 — Dokumentation und Abnahme (Stufe 4)

| ID | Task | Dateien | Tests | Akzeptanzkriterien |
|----|------|---------|-------|---------------------|
| **ABI-14** | Migrationsstrategie fortschreiben | `docs/shm-migration-strategy.md` | — | Stufe 2–4 als umgesetzt beschrieben, neue API und Grenzen dokumentiert. |
| **ABI-15** | Changelog | `CHANGELOG.md` | — | `Added`/`Changed`-Einträge mit Verweis auf Plan und Strategie. |
| **ABI-16** | Abnahme | Build + CTest | alle SHM-Tests | Release-Build, `ctest -L unit`, Format- und Namespace-Check grün. |

## API-Skizze

```cpp
#include <lintel/core/shm/ShmObjectHeader.h>

lintel::shm::ShmObjectOptions options{
  .payloadVersion = 2,
  .persistence = lintel::shm::ShmPersistence::Persistent,
};

lintel::shm::registerPayloadMigration<MyDto>({
  .fromVersion = 1,
  .toVersion = 2,
  .apply = [](const void *oldBytes, std::size_t oldSize, void *newPayload) {
    // alten Stand lesen, neuen Stand an newPayload konstruieren
  },
});

lintel::shm::SharedMemoryRepository<MyDto> repository(
  service, segment, "my-dto", options, MyDto{});
```

## Risiken und Grenzen

- **Kein Destruktor für alte Nutzdaten.** Eine Migration liest die alten Bytes
  und konstruiert den neuen Stand; verschachtelte Allokationen des alten Typs
  werden nicht freigegeben. Das ist bei einem Layout-Wechsel unvermeidbar, weil
  der alte Typ im neuen Build nicht mehr existiert. Der Speicher wird erst beim
  nächsten Neuanlegen des Segments frei.
- **Header-Änderungen.** Der Header ist ab Version 1 eingefroren. Neue Felder
  dürfen nur in `reserved` wandern; eine echte Header-Migration ist ein eigener
  Schritt und muss `headerVersion` erhöhen.
- **Typ-ID.** Die ID wird aus dem vollqualifizierten Typnamen gehasht. Bei einem
  Compiler-/ABI-Wechsel kann sie sich ändern; dann ist eine explizite ID in
  `ShmObjectOptions` zu setzen.
- **`Derived`** bedeutet „die Anwendung kann den Inhalt rekonstruieren". Die
  Bibliothek ruft keinen Rebuild-Hook auf; das bleibt Aufgabe der besitzenden
  Service-Schicht (z. B. `PropertyService`).
- **`Persistent`** ist der Default. Bestehende Aufrufer ohne explizite Angabe
  verlieren also nie stillschweigend Daten.

## Fortschritt

| Phase | Status |
|-------|--------|
| 1 — Hülle und Migrationskern | ☑ |
| 2 — Persistenzklassen | ☑ |
| 3 — Integration bestehender Typen | ☑ |
| 4 — Doku und Abnahme | ☑ |

Letztes Update: 2026-10-22

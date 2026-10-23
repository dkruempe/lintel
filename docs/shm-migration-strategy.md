# Migrationsstrategie für Shared-Memory-Segmente

Stand: 22. Oktober 2026 · gilt ab Layout-Version 3 (`lintel_segment_layout`)

Ein Segment ist eine Datei mit einem Layout, das die Bibliothek definiert. Ändert sich
dieses Layout (in 0.2 zum Beispiel: `ShmSegmentAccessor` von einem Zeiger auf 40 Byte,
neuer Queue-Header), können die Bytes eines alten Segments nicht mehr gedeutet werden.
Dieses Dokument beschreibt, welche Daten das trifft, wer entscheidet und wie eine
Migration aussieht.

## 1. Welche Daten liegen im Segment?

| Objekt | Autoritativ? | Verlust beim Neuanlegen |
|--------|--------------|-------------------------|
| `SharedMemoryPropertyRepository` (Spiegel) | **nein** – Werte stehen in DB bzw. Bootstrap-XML, der SHM-Repository ist eine Kopie für prozessübergreifende Änderungen | wird beim Start neu aufgebaut (`PropertyService::init`) |
| `shm::SharedMemoryRepository<T>` (typisierte Repositories) | **ja** – das ist Nutzdaten-API | **Datenverlust** |
| `RingBufferShm` / `QueueShm` / `ArrayShm` als Payload | **ja**, wie oben | **Datenverlust** |
| `EventBus` (Ringe, Sticky-Werte, Subscriptions) | nein, flüchtig | verloren, per Definition |
| `MessageQueueShmCore` (Header + Ring) | Nachrichten sind flüchtig | verloren |
| Users, Groups, Properties, History | liegen **außerhalb** (DB/XML) | unberührt |

Damit ist die Sorge berechtigt: alles, was über `SharedMemoryRepository<T>` abgelegt
wird, existiert nur im Segment. Die Bibliothek darf das nicht eigenmächtig löschen.

## 2. Wer entscheidet?

`SharedMemoryService` fragt beim Öffnen den Layout-Marker ab. Bei einer Abweichung gilt
folgende Reihenfolge (`SharedMemoryService.cpp`, `create()`):

1. **Migrator registriert** (`lintel::shm::SegmentLayoutPolicy::setMigrator`) → er
   entscheidet. `true` = Inhalte dürfen weg, `false` = Start abbrechen.
2. **Alle Objekte `Volatile`/`Derived`** (und ihre Header lesbar) → das Segment wird
   ohne Operator-Eingriff neu angelegt.
3. **Policy `Recreate`** (`set(LayoutMismatchPolicy::Recreate)`) oder
   `LINTEL_SHM_RECREATE=1` → Inhalt wird verworfen.
4. Sonst **`Fail`** (Default): `std::runtime_error` mit Segmentname, Dateipfad und den
   blockierenden Objekten; die Datei bleibt unangetastet.

Der Migrator läuft, **bevor** irgendetwas gelöscht wird, und bekommt den Pfad der
aktuellen Datei sowie die gefundene und die erwartete Layout-Version.

## 3. Einen Migrator schreiben

```cpp
#include <lintel/core/shm/SegmentLayoutPolicy.h>

// einmalig beim Start, vor dem DI-Container
lintel::shm::SegmentLayoutPolicy::setMigrator(
  [](const std::filesystem::path &stale, std::uint64_t found, std::uint64_t current) {
    // 1) alten Stand lesen - die Datei ist noch da und noch nicht verändert
    boost::interprocess::managed_mapped_file old(boost::interprocess::open_only, stale.c_str());
    for (const auto &name : old.get_named_objects_handles()) { /* ... */ }
    // 2) was wichtig ist, dauerhaft ablegen (JSON, DB, eigene Datei)
    //    z. B. die Payloads der typisierten Repositories über ihre serialize()
    exportToJson(old, "shm-migration-" + stale.filename().string() + "-v" + std::to_string(found) + ".json");
    // 3) true = der Neuanfang ist erlaubt, false = Start abbrechen
    return true;
  });
```

Regeln für den Migrator:

- Er darf die Datei **lesen**, nicht schreiben; die Bibliothek hat sie zu diesem
  Zeitpunkt bereits gemappt.
- Er muss idempotent sein: schlägt der Start danach fehl, läuft er beim nächsten
  Versuch erneut.
- Er sollte protokollieren, was übernommen und was verworfen wurde (der Report gehört
  ins Log, nicht in die Ausnahme).
- Ohne Migrator und ohne `LINTEL_SHM_RECREATE` passiert **nichts** – das ist die
  sichere Voreinstellung für Produktion.

## 4. Grenze der aktuellen Umsetzung

Die segmentweite Entscheidung bleibt der letzte Fallback. Ab Layout-Version 3
trägt jedes benannte Objekt einen eigenen `ShmObjectHeader`; die Bibliothek kann
damit **pro Objekt** entscheiden statt nur pro Segment:

- `Volatile` und `Derived` dürfen ohne Migrator neu angelegt werden.
- `Persistent` verlangt eine registrierte Migration oder einen Migrator.
- Ein Objekt ohne lesbaren Header (Segment aus einem älteren Build) gilt
  weiterhin als `Persistent`, weil seine Persistenzklasse unbekannt ist.

Der Header liegt am Anfang des benannten Objekts; die Nutzdaten selbst sind eine
separate Allokation, die über `payloadOffset` adressiert wird. Änderungen an der
internen Buchhaltung (Accessor, Header, Record) verschieben die Nutzdaten daher
nicht mehr.

## 5. Nächste Stufen (Backlog)

| Stufe | Inhalt | Status |
|-------|--------|--------|
| **2** | `ShmObjectHeader { magic, headerVersion, payloadVersion, payloadBytes, … }` vor jedem benannten Objekt; pro Typ eine Migrationsfunktion `v(n) → v(n+1)` | **umgesetzt** (Layout-Version 3) |
| **3** | Persistenzklasse pro Objekt beim Registrieren: `Volatile`, `Derived`, `Persistent` | **umgesetzt** |
| **4** | stabile Payload-ABI: Nutzdaten als versionierte, separat allokierte Blobs; interne Buchhaltung (Accessor, Header) versioniert getrennt | **umgesetzt** (Header v1, Payload v1) |

Die Umsetzung ist in `docs/shm-payload-abi-plan.md` in Phasen und
Akzeptanzkriterien zerlegt.

### 5.1 Objekt registrieren

```cpp
#include <lintel/core/shm/ShmObjectHeader.h>

lintel::shm::ShmObjectOptions options{
  .payloadVersion = 2,
  .persistence = lintel::shm::ShmPersistence::Persistent,
};

lintel::shm::SharedMemoryRepository<MyDto> repository(
  service, segment, "my-dto", options, MyDto{});
```

Ohne explizite Angabe bleibt `payloadVersion = 1` und `Persistent` der Default.
Das ist die sichere Variante: ein bestehender Aufrufer verliert nie
stillschweigend Daten. Der Property-Spiegel registriert sich als `Derived`,
EventBus und MessageQueue als `Volatile`.

### 5.2 Payload migrieren

```cpp
lintel::shm::registerPayloadMigration<MyDto>({
  .fromVersion = 1,
  .toVersion = 2,
  .apply = [](const void *oldBytes, std::size_t oldSize, void *newPayload) {
    // alten Stand lesen und den neuen Typ an newPayload konstruieren
  },
});
```

Die Registry verkettet `1 → 2 → 3` automatisch. Der alte Payload bleibt
unangetastet, bis die Migration vollständig durchgelaufen ist; schlägt sie fehl,
öffnet sich das Objekt beim nächsten Versuch erneut mit dem alten Stand.

### 5.3 Segmentweite Entscheidung

`SharedMemoryService` prüft bei einem Layout-Konflikt zuerst den Migrator, dann
die Objekt-Header:

1. Migrator registriert → er entscheidet.
2. Alle Objekte sind `Volatile`/`Derived` und lesbar → Segment wird neu angelegt.
3. Policy `Recreate` bzw. `LINTEL_SHM_RECREATE=1` → Inhalt wird verworfen.
4. Sonst → `std::runtime_error` mit Segmentname, Pfad und den blockierenden
   Objekten; die Datei bleibt unangetastet.

### 5.4 Grenzen

- Eine Payload-Migration kann den Destruktor des alten Typs nicht aufrufen, weil
  dessen Layout im neuen Build nicht mehr existiert. Verschachtelte Allokationen
  des alten Stands werden erst beim nächsten Neuanlegen des Segments frei.
- Die Typ-ID ist ein FNV-1a-Hash des vollqualifizierten Typnamens. Bei einem
  Compiler-/ABI-Wechsel kann sie sich ändern; dann ist `payloadTypeId` explizit
  zu setzen.
- `Derived` heißt „die Anwendung kann den Inhalt rekonstruieren“. Die Bibliothek
  ruft keinen Rebuild-Hook auf; das bleibt Aufgabe der besitzenden
  Service-Schicht (z. B. `PropertyService`).

## 6. Betrieb

```bash
# Produktion: nichts anfassen, Start bricht mit Pfad und Anleitung ab
./bin/main

# Entwicklung: Segmente dürfen neu angelegt werden
LINTEL_SHM_RECREATE=1 ./bin/main

# Migration: Migrator im Programm registrieren (siehe oben), dann ohne Env-Variable starten
```

Die Segmentdateien liegen im konfigurierten Verzeichnis (`<SharedMemorySegments><Path>`,
Standard `~/dev/shm`). Ein `POST /shm/cleanup` entfernt verwaiste *Repositories*, keine
Segmente – Segmente werden nur über den Layout-Pfad oder manuell ersetzt.

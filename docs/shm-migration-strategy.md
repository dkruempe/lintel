# Migrationsstrategie für Shared-Memory-Segmente

Stand: 22. Oktober 2026 · gilt ab Layout-Version 2 (`lintel_segment_layout`)

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
2. **Policy `Recreate`** (`set(LayoutMismatchPolicy::Recreate)`) oder
   `LINTEL_SHM_RECREATE=1` → Inhalt wird verworfen.
3. Sonst **`Fail`** (Default): `std::runtime_error` mit Segmentname und Dateipfad, die
   Datei bleibt unangetastet.

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

Die Entscheidung gilt **pro Segment, nicht pro Objekt**: die Objekte im Segment tragen
keine eigene Version, also kann der Migrator nur „alles retten" oder „alles aufgeben".
Für die typisierten Repositories ist das ausreichend, weil ihr Payload ohnehin über
`serialize()`/`deserialize()` der DTOs geht.

## 5. Nächste Stufen (Backlog)

| Stufe | Inhalt | Nutzen |
|-------|--------|--------|
| **2** | `ShmObjectHeader { magic, version, payloadBytes }` vor jedem benannten Objekt; pro Typ eine Migrationsfunktion `v(n) → v(n+1)` | echte Teil-Migration, ohne Export/Import |
| **3** | Persistenzklasse pro Objekt beim Registrieren: `Volatile` (darf weg), `Derived` (aus DB/XML rekonstruierbar), `Persistent` (muss migriert werden, sonst Startabbruch) | die Bibliothek weiß selbst, was sie verwerfen darf |
| **4** | stabile Payload-ABI: Nutzdaten als versionierte Blobs, interne Buchhaltung (Accessor, Header) versioniert getrennt | Layout-Änderungen der Bibliothek invalidieren Nutzdaten nie wieder |

Stufe 2 ist die Voraussetzung für 3 und 4; ohne sie bleibt es beim Segment als
Ganzes.

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

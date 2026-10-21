# Plan: NOLINT-Abbau, HTTP/CLI-Integrationslücken und `std::source_location`

Stand: 21. Oktober 2026 · Basis: `master` @ `1333386` · Bibliotheksversion 0.2.0

Dieses Dokument ist ein **Arbeitsplan**, keine Beschreibung eines bereits erreichten
Zustands. Jeder Schritt nennt die betroffenen Dateien, die konkrete Änderung und den
Befehl, mit dem sich das Ergebnis prüfen lässt.

| Teil | Thema | Umfang | Ergebnis |
|------|-------|--------|----------|
| **A** | NOLINT-Marker vollständig entfernen | 13 Marker in 10 Dateien, 4 Check-Klassen | 0 Marker, keine neuen CodeQL-Alerts |
| **B** | Fehlende HTTP-/CLI-Integration | 11 Lücken + 7 Defekte | priorisierter Backlog |
| **C** | `__FILE__`/`__LINE__` → `std::source_location` | 2 Makro-Familien, ~50 Aufrufstellen | eine API-Änderung (Breaking) |

---

## 1. Warum die NOLINTs überhaupt nötig sind

### 1.1 Der `--checks`-Override ist die Ursache, nicht die Regel

`.clang-tidy` schaltet zwei der betroffenen Checks explizit ab:

```yaml
        -bugprone-easily-swappable-parameters,
        -performance-unnecessary-value-param,
```

`scripts/check-tidy.sh` überschreibt diese Liste aber auf der Kommandozeile:

```bash
CHECKS="${TIDY_CHECKS:--*,clang-analyzer-*,bugprone-*,cert-*,performance-*,portability-*}"
```

Der CLI-Wert ersetzt `Checks:` aus der Konfigurationsdatei vollständig. Damit sind
beide Checks im Gate **wieder an** – für geänderte Dateien, obwohl die Projektkonfiguration
sie als nicht zielführend eingestuft hat. Die NOLINTs sind die Folge.

Es gibt zwei grundsätzlich verschiedene Auswege, und der Plan verfolgt bewusst den
ersten:

1. **Code reparieren**, sodass der Check nichts mehr findet (dieses Dokument).
2. Den Override entschärfen, z. B. `-bugprone-easily-swappable-parameters` in
   `TIDY_CHECKS` aufnehmen. Das wäre eine Policy-Änderung am Gate und wird hier
   **nicht** empfohlen: die Prüfung hat in diesem Repo mehrfach echte Fehlerklassen
   aufgedeckt (siehe `a4c1c59`), und ein pauschales Abschalten würde sie wieder
   verlieren.

### 1.2 Drei Werkzeuge, die sich widersprechen

Bei den vier `performance-no-int-to-ptr`-Stellen kollidieren drei Prüfungen, die alle
Teil der CI sind:

| Werkzeug | Fordert | Warum |
|----------|---------|-------|
| `clang-tidy` (`performance-no-int-to-ptr`) | **kein** `IntegralToPointer`-Cast | Check matcht `castExpr(hasCastKind(CK_IntegralToPointer))` – jede Ganzzahl→Zeiger-Konvertierung außer aus einem Integer-Literal |
| `CodeQL` (`cpp/suspicious-pointer-scaling`) | **keine** Byte-Zeiger-Arithmetik auf einem `reinterpret_cast` aus einem breiteren Objektzeiger | Meldung: *"This pointer might have type ShmRing (size 160), but this pointer arithmetic is done with type byte \* (size 1)."* |
| `GCC` (`-Wstringop-overflow`, `-Werror`) | Zeiger darf **nicht** auf das umgebende Objekt zurückführbar sein | Sonst meldet der Optimierer einen Überlauf, weil der Slot-Puffer im Segment und nicht im Objekt liegt |

Deshalb ist die aktuelle Lösung „Rechnung in der Integer-Domäne, Rückführung per
`reinterpret_cast`“ entstanden: sie erfüllt CodeQL und GCC und verletzt clang-tidy.

---

## 2. Teil A — NOLINT-Marker

### A.0 Bestandsaufnahme

```bash
grep -rn "NOLINT" --include="*.cpp" --include="*.h" src/ examples/ tests/
```

| # | Datei:Zeile | Check | betroffene Funktion |
|---|-------------|-------|---------------------|
| 1 | `src/features/base/events/EventBus.cpp:156` | `performance-no-int-to-ptr` | `ShmRing::buffer()` |
| 2 | `src/features/base/events/EventBus.cpp:163` | `performance-no-int-to-ptr` | `ShmRing::buffer() const` |
| 3 | `src/features/base/services/SharedMemoryRepository.cpp:96` | `performance-no-int-to-ptr` | `ShmCellRecord::payload()` |
| 4 | `src/features/base/msg/MessageQueueShmCore.cpp:254` | `performance-no-int-to-ptr` | `ShmRing::slotAt()` |
| 5 | `src/features/property/repositories/SharedMemoryPropertyRepository.cpp:92` | `bugprone-easily-swappable-parameters` | `allOf(process, class, instance, name)` |
| 6 | `src/features/cli/services/CommandLineService.cpp:140` | `bugprone-easily-swappable-parameters` | `handleCommandInput(ev, bool&, bool&)` |
| 7 | `src/features/cli/services/CommandLineService.cpp:171` | `bugprone-easily-swappable-parameters` | `handleTabInput(ev, bool&, bool&)` |
| 8 | `src/features/cli/services/CommandLineHistoryService.cpp:67` | `bugprone-easily-swappable-parameters` | `startsWith(command, menu)` |
| 9 | `src/core/persistence/sqlite3/Connection.cpp:140` | `bugprone-easily-swappable-parameters` | `prepareStatement(queryName, query)` |
| 10 | `src/core/persistence/sqlite3/Connection.cpp:257` | `bugprone-easily-swappable-parameters` | `execute()`-Callback (`char**, char**`) |
| 11 | `src/core/utils/MemorySize.cpp:19` | `bugprone-easily-swappable-parameters` | `parseAndScale(input, prefix, multiplier)` |
| 12 | `src/features/http/controllers/HistoryController.cpp:41` | `performance-unnecessary-value-param` | unbenutzter `groupRepository`-Parameter |
| 13 | `examples/main.cpp:128` | `bugprone-exception-escape` | impliziter Move-`operator=` von `TestDataDto` |

Dazu kommt `SharedMemoryPropertyRepository.cpp:91` – kein Marker, sondern der Kommentar,
der erklärt, warum kein Zeilenumbruch möglich ist. Er entfällt mit dem Umbau in A.2.

**Reihenfolge der Arbeitspakete** (jedes ist einzeln mergebar):

| Paket | Marker | Aufwand | Risiko |
|-------|--------|---------|--------|
| A.1 | 4 × `performance-no-int-to-ptr` | klein | mittel (Werkzeugkonflikt) |
| A.3 | 1 × `performance-unnecessary-value-param` | klein | keins |
| A.4 | 1 × `bugprone-exception-escape` | klein | keins |
| A.2a | 3 × swappable (`startsWith`, `prepareStatement`, `parseAndScale`) | klein | keins (keine Aufrufstellen-Änderung) |
| A.2b | 2 × swappable (`handleCommandInput`, `handleTabInput`) | klein | keins (privat) |
| A.2c | 1 × swappable (`execute()`-Callback) | mittel | niedrig |
| A.2d | 1 × swappable (`PropertyRepository::allOf`) | groß | API-Bruch, ~27 Aufrufstellen |

---

### A.1 `performance-no-int-to-ptr` (4 Marker)

#### Befund

Alle vier Stellen adressieren einen Puffer, der **im Shared-Memory-Segment** liegt, über
einen **Byte-Offset relativ zum umgebenden Objekt**:

```cpp
// EventBus.cpp:131 – im Konstruktor
m_bufferOffset = aligned - reinterpret_cast<std::byte *>(this);
```

Das Offset-Muster ist korrekt und muss so bleiben: das Objekt liegt selbst im Segment und
wird in jedem Prozess an einer **anderen** virtuellen Adresse eingeblendet, nur die
Relative bleibt stabil. Ein absoluter Zeiger wäre prozesslokal und damit falsch (siehe
A.1.4).

#### Gemessene Varianten

Alle Varianten wurden in `src/` bzw. einer isolierten Probe gegen die echten
Projektflags (`-Werror -Wall -Wextra -Wcast-align …`) und die CI-Checkliste geprüft:

| Variante | GCC-Build | clang-tidy (CI-Checks) | CodeQL |
|----------|-----------|------------------------|--------|
| V1 Integer-Domäne + `reinterpret_cast<T *>` **(Status quo)** | ok | **Finding** | ok |
| V2 `std::byte *base = …; return reinterpret_cast<T *>(base + offset);` | **Fehler**: `writing 345 bytes into a region of size 152 [-Werror=stringop-overflow=]` (`slot.value = event` in `EventBus::publish`) | ok | **Alert** `cpp/suspicious-pointer-scaling` |
| V2b V2 + `// codeql[cpp/suspicious-pointer-scaling]` | Fehler | ok | unterdrückt |
| **V3 `std::bit_cast<T *>(address)`** | **ok** | **ok** | kein Alert zu erwarten (kein Cast-Ausdruck, keine Zeigerarithmetik) |
| V4 absoluter `std::byte *m_buffer`-Member (Muster aus `RingBufferShm`) | ok | ok | ok – **aber fachlich falsch**, siehe A.1.4 |

V2 scheitert an GCC: sobald der Zeiger über eine `std::byte *`-Basis zurückführbar ist,
sieht der Optimierer das umgebende Objekt (152 Byte) und den 345 Byte großen Schreibvorgang.
Das ist ein **False Positive** – der Puffer liegt im Segment –, aber mit `-Werror`
buildbrechend. `std::bit_cast` (V3) unterbricht die Rückführbarkeit und ist damit die
einzige Variante, die alle drei Werkzeuge gleichzeitig erfüllt.

#### Änderung (V3)

```cpp
// src/features/base/events/EventBus.cpp  (+ #include <bit>)
EventBus::ShmRing::Slot *EventBus::ShmRing::buffer()
{
  // Der Puffer liegt im Segment, nicht in diesem Objekt, und wird über den Byte-Offset
  // m_bufferOffset adressiert. Die Adresse entsteht in der Integer-Domäne; std::bit_cast
  // stellt sie wieder her: ein reinterpret_cast aus der Ganzzahl meldet clang-tidy als
  // performance-no-int-to-ptr, Byte-Zeiger-Arithmetik meldet CodeQL als
  // cpp/suspicious-pointer-scaling.
  const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(this) + static_cast<std::uintptr_t>(m_bufferOffset);
  return std::bit_cast<Slot *>(address);
}

const EventBus::ShmRing::Slot *EventBus::ShmRing::buffer() const
{
  const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(this) + static_cast<std::uintptr_t>(m_bufferOffset);
  return std::bit_cast<const Slot *>(address);
}
```

```cpp
// src/features/base/services/SharedMemoryRepository.cpp  (+ #include <bit>)
[[nodiscard]] void *payload() const
{
  const std::uintptr_t address =
    reinterpret_cast<std::uintptr_t>(this) + static_cast<std::uintptr_t>(m_payloadOffset);
  return std::bit_cast<void *>(address);
}
```

```cpp
// src/features/base/msg/MessageQueueShmCore.cpp  (+ #include <bit>)
[[nodiscard]] SlotHeader *slotAt(std::uint64_t position) const
{
  const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(this)
                                 + static_cast<std::uintptr_t>(m_bufferOffset)
                                 + (position % m_capacity) * m_stride;   // kein Cast: size_t == uintptr_t
  return std::bit_cast<SlotHeader *>(address);
}
```

**Fallstricke, die beim Nachbauen auftreten:**

- `static_cast<std::uintptr_t>((position % m_capacity) * m_stride)` ist ein
  `-Wuseless-cast`-Fehler (das Produkt ist bereits `std::size_t`, auf LP64 identisch mit
  `std::uintptr_t`). Der Cast muss weg.
- In `const`-Memberfunktionen ist `reinterpret_cast<std::byte *>(this)` unzulässig; die
  Integer-Variante umgeht das, weil eine Ganzzahl keine Constness trägt. Genau deshalb
  bleibt der Cast `reinterpret_cast<std::uintptr_t>(this)` (ohne `const_cast`).
- `-Wcast-align` schlägt bei keiner der Varianten zu (mit den Projektflags geprüft).

#### Verifikation

```bash
cmake --build build/build/Release --parallel "$(nproc)"
cd build/build/Release && ctest --output-on-failure --label-regex unit   # 1194 Tests, grün
scripts/check-tidy.sh master build/tidy                                  # 0 findings
```

Zusätzlich muss der CodeQL-Lauf im PR grün sein; lokal ist CodeQL nicht installiert
(`docs/code-scanning.md`, Abschnitt *Reproducing the analysis locally*).

#### A.1.4 Warum der „Präzedenzfall“ `RingBufferShm` nicht kopiert werden darf

`src/include/lintel/core/shm/containers/RingBufferShm.h` hat den NOLINT in `a4c1c59`
durch einen absoluten Zeiger ersetzt:

```cpp
m_buffer = aligned;                       // statt m_bufferOffset = aligned - (byte*)this
[[nodiscard]] Slot *buffer() { return reinterpret_cast<Slot *>(m_buffer); }
```

Das ist **kein gültiges Muster** für Objekte im Segment: `m_buffer` enthält die Adresse
aus dem Mapping des *erzeugenden* Prozesses. Der Klassenkommentar (Zeilen 27–32)
beschreibt weiterhin korrekt das Offset-Layout – Code und Dokumentation sind hier
auseinandergelaufen. Der Test `SharedMemoryRepositoryTest.cpp:392`
("QueueShm in shared memory is FIFO") deckt nur den Ein-Prozess-Fall ab, deshalb ist der
Fehler bisher nicht aufgefallen.

**Konsequenz für den Plan:** `RingBufferShm` wird in einem eigenen Schritt auf das
Offset-Muster (mit `std::bit_cast`) zurückgeführt, sobald A.1 gemergt ist. Bis dahin
darf es nicht als Vorbild für weitere Stellen dienen.

#### A.1.5 Strukturelle Folgelösung (optional, nach A.1)

Die saubere Langfristlösung adressiert relativ zur **Segmentbasis** statt zum Objekt:

1. `ShmSegmentAccessor::Allocator` um `virtual std::byte *baseAddress() const noexcept`
   erweitern (Default `nullptr`), `ShmSegmentAccessor::baseAddress()` durchreichen.
2. `BoostSegmentAllocator` implementiert das über die Adresse der gemappten Datei
   (`basic_managed_memory_impl::get_address()`, siehe
   `boost/interprocess/detail/managed_memory_impl.hpp:228`); der Allocator braucht dafür
   zusätzlich die Basisadresse im Konstruktor.
3. Offsets relativ zur Segmentbasis speichern und mit
   `reinterpret_cast<T *>(segmentBase + offset)` adressieren – Byte-Arithmetik auf einem
   `std::byte *`, dessen statischer Typ bereits `std::byte *` ist. Damit entfallen
   `std::bit_cast` **und** jede Unterdrückung.

**Blocker, die vorher gelöst werden müssen** (eigene Arbeitspakete, siehe B.4):

- `ShmSegmentAccessor` wird **im Segment** gespeichert und hält einen Zeiger auf den
  Allocator des *erzeugenden* Prozesses. In einem zweiten Prozess ist dieser Zeiger
  ungültig; `baseAddress()` würde die falsche Basis liefern. Der Accessor muss pro Prozess
  neu gebunden werden (z. B. über einen prozesslokalen Registry-Eintrag je Segmentname,
  analog zu `mqshm_detail::g_registry`).
- `ShmConstructors.h` (`constructObjectWith`) ruft den Konstruktor nur beim *Erzeugen*;
  ein anhängender Prozess bekommt das Objekt ohne Rebinding.

---

### A.2 `bugprone-easily-swappable-parameters` (7 Marker)

#### Regel

Der Check meldet Nachbarparameter **gleichen Typs**, deren Namen nicht durch ein
gemeinsames Präfix/Suffix „geschützt“ sind (`NamePrefixSuffixSilenceDissimilarityTreshold`,
Default 1). Empirisch gilt für dieses Repo:

- **Umbenennen allein hilft nicht** (außer bei fast identischen Namen wie `name`/`names`).
- Die Typen müssen sich **unterscheiden** – zwei verschiedene Strong Types oder z. B.
  `std::string` vs. `std::string_view`.
- Ein gemeinsames Aggregat für *beide* Parameter hilft nicht, nur ein einzelner
  Aggregat-Parameter (wie in `a4c1c59` mit `MessageQueueFilter` geschehen).

#### A.2a Drei Stellen ohne Änderung der Aufrufstellen

| Stelle | Änderung | Warum ohne Aufrufstellen-Änderung |
|--------|----------|-----------------------------------|
| `CommandLineHistoryService::startsWith(command, menu)` | `menu` → `std::string_view` | wird nur verglichen (`iter.getMenu() != menu`) |
| `Connection::prepareStatement(queryName, query)` | `queryName` → `std::string_view` | Schlüssel wird einmalig als `std::string key(queryName)` materialisiert; `std::string` und `const char *` konvertieren implizit |
| `MemorySize::parseAndScale(input, prefix, multiplier)` | beide → `std::string_view`, `std::stoul` → `std::from_chars` | anonymer Namespace, zwei Aufrufe in derselben Datei |

Für `parseAndScale` fällt dabei ein Nebengewinn an: `std::from_chars` ist `noexcept` und
allokiert nicht, die Fehlermeldungen laufen über `std::format` (im TU bereits eingebunden):

```cpp
std::size_t parseAndScale(std::string_view input, std::string_view prefix, std::size_t multiplier)
{
  unsigned long value = 0;
  const auto [end, error] = std::from_chars(prefix.data(), prefix.data() + prefix.size(), value);
  if (error != std::errc{} || end != prefix.data() + prefix.size()) {
    throw std::invalid_argument(std::format("invalid memory size: '{}'", input));
  }
  ...
}
```

#### A.2b `CommandLineService`: Out-Parameter → Rückgabewert

```cpp
// CommandLineService.h
struct InputOutcome
{
  bool m_tabPressed = false;
  bool m_printPrompt = true;
};

InputOutcome handleCommandInput(const SymbolEvent &symbolEvent);
InputOutcome handleTabInput(const SymbolEvent &symbolEvent);
```

Einziger Aufrufer ist `CommandLineService.cpp:264/267`. Damit verschwinden beide Marker
und die beiden `bool &`-Parameter, die den Prompt-Zustand durch die Funktion tragen.

#### A.2c `Connection::execute()`: Callback entfernen

Der Callback hat die von der C-API vorgegebene Signatur
`(void *, int, char **, char **)` – zwei `char **` lassen sich nicht durch Typen
unterscheiden. Deshalb nicht umbenennen, sondern **die API wechseln**: `execute()` auf
`sqlite3_prepare_v2` + `sqlite3_step` + `sqlite3_column_*` umstellen, so wie
`executeParameters()` (Zeilen ~225–251) es bereits tut.

Gewinn: `Connection::callBack` (Zeile 279), der `std::function`-Wrapper und der
`static_cast<void *>(&func)`-Trick entfallen; der Marker verschwindet ohne Ersatz.

#### A.2d `PropertyRepository::allOf` – Aggregat (Breaking Change)

```cpp
// src/include/lintel/features/property/repositories/PropertyRepository.h
struct PropertyFilter
{
  std::string m_processName{ ".*" };
  std::string m_className{ ".*" };
  std::string m_instanceName{ ".*" };
  std::string m_name{ ".*" };
};

virtual std::vector<std::shared_ptr<PropertyBase>> allOf(const PropertyFilter &filter = {}) = 0;
```

Muster: `MessageQueueFilter` / `EventBusCapacities` aus `a4c1c59`.

Betroffen:

- `PropertyRepository.h` (Interface), `SharedMemoryPropertyRepository`,
  `DatabasePropertyRepository`, Mocks in `tests/`
- ~27 Aufrufstellen, u. a. `PropertyService.cpp:190/203/220/241/379`,
  `PropertyController.cpp:43`, `PropertyCliComponent.cpp:44`,
  `PropertyChangeTest.cpp:299`, `DatabasePropertyRepositoryTest.cpp:196–202`

Das ist der einzige echte API-Bruch in Teil A und gehört in `CHANGELOG.md` unter
`[Unreleased]` (siehe `docs/versioning.md`: vor 1.0.0 ist ein Minor-Bruch erlaubt, muss
aber ausgewiesen werden).

*Alternative, falls der Umbau verschoben wird:* Interface unverändert lassen und den
Marker stehen lassen – dann bleibt A.2 offen. Nicht empfohlen, weil der Marker dann
dauerhaft als „bekannte Schuld“ im Code steht.

---

### A.3 `performance-unnecessary-value-param` (1 Marker)

`HistoryController` bekommt einen `std::shared_ptr<GroupRepository>`, den der Konstruktor
nie verwendet (die Gruppen sind fest verdrahtet: `m_adminGroup("Admin-History")`,
`m_userGroup("User-History")`). Der Kommentar begründet die Beibehaltung mit der
DI-Auflösung – das ist nicht nötig, der Container löst nach Typ auf und kommt mit zwei
Parametern aus.

```diff
-  explicit HistoryController(const std::shared_ptr<IAuthService> &authService,
-    std::shared_ptr<GroupRepository> groupRepository,
-    std::shared_ptr<HistoryRepository> historyRepository);
+  explicit HistoryController(const std::shared_ptr<IAuthService> &authService,
+    std::shared_ptr<HistoryRepository> historyRepository);
```

Mit anzupassen:

- `src/features/http/controllers/HistoryController.cpp` (Definition + irreführender Kommentar)
- `tests/http/HistoryControllerTest.cpp` (`makeController()`, unbenutztes `m_groupRepository`
  samt Include entfernen)
- `#include "…/GroupRepository.h"` in `HistoryController.h` entfällt

Die DI-Registrierung `builder.registerType<HistoryController>()…` in
`src/app/HttpFeature.cpp:34` bleibt unverändert.

---

### A.4 `bugprone-exception-escape` (1 Marker)

Betroffen ist **nicht** der Kopier-Zuweisungsoperator, sondern der **implizit erzeugte
Move-`operator=`** von `TestDataDto`: `bugprone-exception-escape` prüft Move-Konstruktoren,
Move-Zuweisungen, Destruktoren, `swap`, `main` und alles mit `noexcept` – und der
implizite Move-`operator=` der `shm::String`-Member kann `bad_alloc` werfen.

**Nicht** funktioniert (jeweils geprüft):

- `operator=(const TestDataDto &) = delete;` → `boost::container::vector::push_back`
  braucht die Zuweisung (`copy_move_algo.hpp:889`), Build bricht.
- Move-Operationen explizit `= default` setzen → `TestDataDto` ist dann kein Aggregat
  mehr, die Initialisierung mit fünf Argumenten (`examples/main.cpp`) bricht.

**Funktioniert:** den Destruktor explizit defaulten. Das unterdrückt die impliziten
Move-Operationen; Züge fallen auf die Kopieroperationen zurück (was im Beispiel ohnehin
passiert), und der Check hat kein Move mehr zu prüfen:

```cpp
struct TestDataDto
{
  // Die shm::String-Member allokieren aus dem Segment, also kann auch ein Move das
  // Werfen nicht versprechen. Der explizit gedefaultete Destruktor unterdrückt die
  // impliziten Move-Operationen; Kopien laufen über die Kopieroperationen, was jede
  // Verwendung in diesem Beispiel ohnehin tut.
  ~TestDataDto() = default;
  ...
};
```

Aggregat-Initialisierung bleibt möglich (ein Destruktor zählt nicht zu den
„user-declared constructors“).

---

### A.5 Definition of Done für Teil A

```bash
grep -rn "NOLINT" --include="*.cpp" --include="*.h" src/ examples/ tests/   # leer
cmake --build build/build/Release --parallel "$(nproc)"                     # grün
cd build/build/Release && ctest --output-on-failure --label-regex unit      # grün
scripts/check-tidy.sh master build/tidy                                     # 0 findings
```

Zusätzlich: `CHANGELOG.md` unter `[Unreleased]` um die API-Änderung
(`PropertyRepository::allOf` samt Aufrufern) ergänzen.

---

## 3. Teil B — fehlende HTTP-/CLI-Integration

### B.1 Methodik

Verglichen wurde die Menge der öffentlichen Service-Methoden (`src/include/lintel/**/services/*.h`,
`*Repository.h`, `*Api.h`) mit der Menge der HTTP-Routen (`ADD_HANDLER_METHOD` in
`controllers/`) und der CLI-Kommandos (`CommandParser::addCommand` in `components/`).

Wesentliche Strukturerkenntnis: **die CLI reitet auf HTTP**. Jede CLI-Komponente hält eine
`*Api`-Klasse, die den HTTP-Client des gleichen Endpunkts aufruft. CLI-Abdeckung ist damit
immer eine Teilmenge der HTTP-Abdeckung – eine Lücke im HTTP ist automatisch eine Lücke in
der CLI, umgekehrt nicht.

### B.2 Lücken, nach Nutzen sortiert

| # | Lücke | Ist-Zustand | Vorschlag | Nutzen |
|---|-------|-------------|-----------|--------|
| 1 | **EventBus** (publish/subscribe/stats/dropped) | **kein** Controller, **keine** Api, **kein** CLI-Kommando – `grep EventBus src/features/http src/features/cli` ist leer | `EventBusController` (`/eventBus/…`: publish, subscribe, stats, purge) + `EventBusApi` + CLI-Komponente + swagger | hoch – das Subsystem ist komplett unsichtbar |
| 2 | `SharedMemoryCleanupService` | Klasse existiert, ist aber **nicht in der DI** (`src/app/BaseFeature.cpp:117–158`) und hat keinen Endpoint | Registrierung + `/sharedMemory/cleanup/…` + CLI | hoch – sonst nur programmatisch erreichbar |
| 3 | `MessageQueueService::recoverOrphans(staleThreshold)` | nur Service (`MessageQueueService.cpp:95`) | `POST /messageQueue/recover` + CLI | hoch (Betrieb) |
| 4 | `SharedMemoryService::statsOf(segment)` | nur auf der **konkreten** Klasse, nicht auf `ISharedMemoryService` | Methode ins Interface, `/sharedMemory/stats/…` | hoch |
| 5 | `SharedMemoryService::recover(...)` | **null Aufrufer**, Doc-Kommentar beschreibt ein anderes Verhalten | Endpoint bauen **oder** entfernen (toter Code) | mittel |
| 6 | MQ-Überlauf-/Dead-Letter-Policy (`setOverflowPolicy`, `setOverflowTimeout`, `setDeadLetterQueueName`) | wird von `MessageQueue<T>` und `MessageQueueService` **nicht** durchgereicht | Service-API + `PUT /messageQueue/policy/…` | mittel |
| 7 | `MessageQueueRequestReply` | nirgends exponiert | optional API | mittel |
| 8 | `ProcessService`: die `ProcessGroup`-Überladungen (`startOf/restartOf/stopOf/terminateOf/detachOf(const ProcessGroup &)`, Zeilen 264–281) | HTTP kennt nur die `Process`-Variante (`ProcessController.h:21–31`) | Gruppen-Endpoints | mittel |
| 9 | `HistoryService`-Abfrage-Überladungen | `HistoryController` greift **direkt** auf `HistoryRepository` zu, umgeht den Service | Controller auf den Service umstellen | mittel (Architektur) |
| 10 | `SharedMemoryRepositoryRegistry` | Singleton (`instance()`), **kein einziger Aufrufer** im Repo | verdrahten oder löschen | niedrig |
| 11 | `ShmView`/`ShmSlice`, SHM-Lifecycle-Hooks, typisierte SHM-Payload-API | nicht exponiert | optional, erst nach A.1.5 | niedrig |

### B.3 Konkrete Defekte in bestehender CLI/Swagger

Alle im Code verifiziert, unabhängig von den Lücken oben:

| Datei:Zeile | Defekt |
|-------------|--------|
| `src/features/cli/components/SharedMemoryCliComponent.cpp:26` | `grow_segment` hat die Beschreibung **„Shrink segment“** |
| `src/features/cli/components/SharedMemoryCliComponent.cpp:30–33` | `export_repository` bindet `m_type` und `m_file` nicht als Argumente – die Flags fehlen, die Member sind toter Zustand |
| `src/features/cli/components/PropertyCliComponent.cpp:12/18/25` | `--process-name` und `--process_name` gemischt (gleiche Option, zwei Schreibweisen) |
| `src/features/cli/components/UserManagementCliComponent.cpp:19/84` | Enum-Slots `AddGroup`, `RemoveGroup`, `UpdateGroup` haben **weder** `addCommand` **noch** `case` |
| `src/features/cli/components/HistoryCliComponent.cpp:9` | Parametername `n_name` (Tippfehler) |
| `src/features/cli/components/HistoryCliComponent.cpp` | Hilfe-Text mit überzähliger `)` |
| `swagger.yaml` | `/messageQueue/stats/{processName}/{messageQueueName}` und `/messageQueue/purge/{processName}` sind implementiert (`MessageQueueController.h:39–40`), fehlen aber in `swagger.yaml` **und** `swagger.md` – `scripts/check-swagger.sh` prüft nur die Gegenrichtung |

### B.4 Nebenbefunde (SHM-Korrektheit, blockieren A.1.5)

1. **`MessageQueueShmCore` ist nicht prozessübergreifend.** `openOrCreateState()` legt den
   `QueueState` in einem prozesslokalen `std::map`-Registry ab
   (`MessageQueueShmCore.cpp:306–357`); jeder Prozess erzeugt damit seinen **eigenen** Ring
   im Segment. Der Test „is visible to a second core of the same segment“ prüft nur zwei
   Cores **im selben Prozess**. Der SHM-Backend liefert damit faktisch keine IPC.
2. **`ShmSegmentAccessor` im Segment zeigt auf den erzeugenden Prozess.** Der Accessor wird
   als Member im Segmentobjekt kopiert (`ShmSegmentAccessor.h:16–19`) und dereferenziert
   `m_allocator` lazy – in einem zweiten Prozess ist das ein fremder Prozesszeiger.
   `~ShmRing` ruft `m_segment.deallocate(...)`.
3. **`RingBufferShm` speichert eine absolute Adresse** (siehe A.1.4).

Diese drei Punkte gehören in ein eigenes Arbeitspaket „SHM-Prozessmodell“, bevor
B.2 #2/#4/#11 sinnvoll werden.

---

## 4. Teil C — `__FILE__`/`__LINE__` → `std::source_location`

### C.1 Bestandsaufnahme

```bash
grep -rn "__FILE__" src/ tests/ examples/ benchmarks/    # 51 Treffer
```

| Ort | Vorkommen | Ersetzbar? |
|-----|-----------|------------|
| `src/include/lintel/core/services/LoggerService.h:201–213` | 6 `LOG_*`-Makros, jedes mit `__FILE__, __LINE__, LINTEL_FUNCTION` | **ja** |
| `src/include/lintel/core/services/PropertyRegistration.h:26–31` | `registerProperty(name, defaultValue, description, runtime, fileName, position)`, `DataStorage`-Kennung `"<basename>:<line>"` | **ja** |
| `src/include/lintel/features/property/services/PropertyService.h:22–24` | `DEFINE_PROPERTY`-Makro | **ja** |
| 19 direkte Aufrufe `registerProperty<T>(…, __FILE__, __LINE__)` in `AuthService` (6), `HistoryService` (5), `ProcessService` (2), `SharedMemoryService` (3), `SchedulerService` (1), `CommandLineHistoryService` (2) | Aufrufstellen | **ja** |
| `tests/core/LoggerServiceTest.cpp` | 25 Aufrufe `service.log(__FILE__, __LINE__, "test", …)` | **ja** |
| `src/include/lintel/core/utils/TypeName.h` | `__PRETTY_FUNCTION__` zur **Compile-Zeit** | **nein** – `std::source_location` ist ein Laufzeitobjekt und kann den Typnamen nicht ersetzen. Bleibt unverändert und wird hier nur abgegrenzt. |

Die 351 `LOG_*`-Aufrufe im Repo bleiben **unverändert** – die Makros kapseln die Signatur.

Verhaltensprüfung (GCC 14, `-O0` und `-O2`, in Makros, Default-Argumenten und Lambdas):
`std::source_location::current()` liefert exakt dieselben `file_name()`/`line()`-Werte wie
`__FILE__`/`__LINE__`, `function_name()` entspricht `__PRETTY_FUNCTION__` (inkl.
Lambda-Schreibweise). Die Migration ist damit **verhaltensneutral**, auch für die
`DataStorage`-Kennung `"<basename>:<line>"`.

### C.2 Ziel-API

```cpp
// LoggerService.h – Signatur von log()/logImpl() umstellen, Makros bleiben in der Form
void log(const std::source_location &location, LogLevel lvl, std::string_view message, Args &&...args)
{
  logImpl(location, lvl, message, std::make_format_args(args...));
}

#define LOG_INFO(message, ...) \
  lintel::LoggerService::get().log(std::source_location::current(), lintel::LogLevel::info, message, ##__VA_ARGS__)
```

```cpp
// PropertyRegistration.h
template<class type>
std::shared_ptr<Property<type>> registerProperty(std::string name,
  type defaultValue,
  std::string description,
  bool runtime,
  const std::source_location &location = std::source_location::current())
{
  ...
  const std::filesystem::path path(location.file_name());
  property->setDataStorage(DataStorage(PropertyRepositoryType::DEFAULT,
    path.filename().generic_string() + ":" + std::to_string(location.line())));
  ...
}
```

```cpp
// PropertyService.h
#define DEFINE_PROPERTY(name, type, defaultValue, description, runtime) \
  std::shared_ptr<lintel::Property<type>> name =                        \
    registerProperty<type>(std::string(#name), defaultValue, description, runtime)
```

`logImpl()` in `src/core/services/LoggerService.cpp:263–287` baut heute
`spdlog::source_loc{file, line, function}`; danach:

```cpp
const spdlog::source_loc source{ location.file_name(),
  static_cast<int>(location.line()),
  location.function_name() };
```

`log()` bleibt ein Template (`std::make_format_args`), deshalb kann `logImpl()` die
`source_location` nicht selbst erzeugen – das `current()` muss im Makro stehen. Genau
deshalb ist die Umstellung verhaltensneutral: der Aufrufpunkt ist derselbe wie bei
`__FILE__`/`__LINE__`.

### C.3 Migrationsschritte

1. `LoggerService.h` (6 Makros + `log()`) und `LoggerService.cpp` (`logImpl()`, `spdlog::source_loc`)
   umstellen. Die 351 `LOG_*`-Aufrufstellen bleiben unberührt.
2. `PropertyRegistration.h` (`registerProperty`, `DataStorage`-Kennung) und
   `PropertyService.h` (`DEFINE_PROPERTY`) umstellen.
3. Die 19 direkten `registerProperty`-Aufrufe: `, __FILE__, __LINE__` **entfernen**
   (Default-Argument greift). Das ist der einzige Punkt, an dem Aufrufstellen angefasst
   werden – ein rein mechanischer, zeilenweiser Edit.
4. `tests/core/LoggerServiceTest.cpp` (25 Stellen):
   `service.log(__FILE__, __LINE__, "test", …)` →
   `service.log(std::source_location::current(), …)`; das dritte Argument (Funktionsname)
   entfällt, weil `function_name()` jetzt aus der Location kommt.
5. Verifikation: `ctest -R Logger`, `ctest -R Property`, plus Sichtprüfung einer Logzeile
   und einer persistierten Property (`DataStorage`-Kennung `"<basename>:<line>"` muss
   identisch bleiben).

### C.4 Bruchstelle

`PropertyRegistration::registerProperty` ist **öffentliche API**
(`src/include/lintel/core/services/PropertyRegistration.h`). Der Wegfall von
`fileName`/`position` ist ein Signaturbruch und muss in `CHANGELOG.md` unter
`[Unreleased]` stehen (siehe `docs/versioning.md`: vor 1.0.0 erlaubt, aber auszuweisen).

`LoggerService::log()` ist ebenfalls öffentlich, wird aber praktisch nur über die Makros
benutzt; die Umstellung ist dort unkritisch, weil die Makros die Signatur kapseln.

---

## 5. Risiken und offene Entscheidungen

| Risiko | Auswirkung | Umgang |
|--------|-----------|--------|
| `std::bit_cast` zwischen Ganzzahl und Zeiger ist formal nicht garantiert (der Standard sichert nur die Objektrepräsentation zu) | theoretisch; auf allen Zielplattformen (LP64, flache Adressräume) korrekt | in A.1.5 durch Segmentbasis-Adressierung ersetzen; bis dahin im Kommentar benennen |
| CodeQL-Verhalten von `std::bit_cast` ist lokal nicht prüfbar | ein Alert könnte im PR auftauchen | PR-Lauf abwarten; Fallback ist `// codeql[cpp/suspicious-pointer-scaling]` (dokumentierter Mechanismus, `docs/code-scanning.md`) |
| A.2d bricht die `PropertyRepository`-API | ~27 Aufrufstellen | eigener PR, `CHANGELOG.md`, ggf. `[[deprecated]]`-Überladung für eine Minor-Version |
| `RingBufferShm`-Adressfehler ist unentdeckt | Cross-Process-Korruption | eigener Fix **vor** allen Erweiterungen in B.2 #11 |
| `MessageQueueShmCore` ist nicht prozessübergreifend | SHM-Backend erfüllt MQ-6 aus `docs/ipc-driver-unit-plan.md` nicht | dortigen Status korrigieren; eigenes Arbeitspaket |

**Zu entscheiden:**

1. Wird A.2d (Aggregat) jetzt gemacht oder bleibt der Marker bis zur nächsten
   Property-Überarbeitung stehen?
2. Wird `SharedMemoryService::recover()` gebaut oder gelöscht?
3. Wird `SharedMemoryRepositoryRegistry` verdrahtet oder entfernt?
4. Reihenfolge B.4 (SHM-Prozessmodell) gegen B.2 – ohne B.4 sind #2/#4/#11 nur Fassade.

---

## 6. Anhang

### 6.1 Verifikationskommandos

```bash
# Build (CI-Layout)
cmake --build build/build/Release --parallel "$(nproc)"

# Unit-Tests (PostgreSQL-Tests werden ohne laufenden Server übersprungen)
cd build/build/Release && ctest --output-on-failure --label-regex unit

# clang-tidy-Gate (braucht build/tidy mit -DENABLE_UNITY_BUILD=OFF -DENABLE_PCH=OFF)
scripts/check-tidy.sh master build/tidy

# Format / Namespace / Secrets
scripts/check-format.sh master
scripts/check-namespace.sh
scripts/check-secrets.sh
```

### 6.2 Lokal validierte Patches

Die Änderungen aus A.1, A.3 und A.4 wurden in dieser Sitzung angewendet und geprüft:

| Prüfung | Ergebnis |
|---------|----------|
| Build `build/build/Release` (GCC 14, `-Werror`) | grün |
| `ctest --label-regex unit` | 1194/1194 grün (22 PostgreSQL-Tests übersprungen) |
| `clang-tidy` mit CI-Checkliste auf allen betroffenen Dateien | 0 Findings |

Sie sind im Arbeitsbaum **nicht** enthalten – dieses Dokument ist der Plan, nicht der
Patch. Auf Wunsch lassen sie sich als ein Commit „refactor: drop NOLINT suppressions in
logging-adjacent helpers“ anwenden.

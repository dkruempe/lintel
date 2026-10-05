# Kompilierzeit-Messprotokoll (Referenz)

> Referenzprotokoll zur Kompilierzeit-Optimierung (P2: „Mess-Skripte versionieren").
> Ziel: **reproduzierbare**, über Commits hinweg vergleichbare Zahlen für die
> Kompilierzeit von `cpp-base-library`.

## Umgebung & Grundregeln

Diese Maschine ist eine **Produktionsmaschine mit laufenden Fremdprozessen**.
Deshalb gilt für **alle** Messungen in diesem Dokument:

- **Single-Core** (`--jobs 1`), dazu `--nice` (Priorität `nice -n 10`).
  Es wird bewusst nie mehr als **ein** Compiler-Prozess gestartet, damit die
  parallel laufenden Dienste nicht ausgebremst werden.
- Lastdurchschnitt (1/5/15 min) wird vor/nach jedem Lauf protokolliert, damit
  Ausreißer durch Fremdlast erkennbar sind.
- Es wird **nie** der Quellbaum verändert; nur `build/` wird erzeugt bzw. geleert.
- Skripte: `scripts/measure-compile-times.sh`, `scripts/measure-header-weight.sh`
  (beide sind versioniert – `.gitignore` lässt `scripts/*.sh` zu).

Vergleichsstände werden über einen Worktree des Vor-Commits hergestellt:

```bash
git worktree add --detach /tmp/opencode/baseline <commit>
```

## Messwerkzeuge

### 1. Voll-Build (configure + Build + ccache-Statistik)

```bash
# Vollständiger Referenzlauf: ccache leer + Incremental-Delta in einem Lauf
scripts/measure-compile-times.sh --jobs 1 --nice --cold --incremental
```

Ausgabe: Wall-Clock des Builds, Host-Load vor/nach, `ccache -s`, danach
`touch src/include/base_library/core/utils/StringUtils.h` + Rebuild-Zeit.
`--cold` und `--incremental` sind kombinierbar (vorher war `--incremental`
ein exklusiver Modus und hat thereby `--cold` verschluckt).

### 2. Header-Gewicht (Single-TU-Probe)

Preprocessed-Größe und `-fsyntax-only`-Zeit einer typischen `LOG_*`-Consumer-TU:

```bash
scripts/measure-header-weight.sh --build-dir build/nonunity --header LoggerService.h
```

Flags (`-I`/`-isystem`/`-D`/`-std`) werden aus `compile_commands.json` extrahiert
(TU `LoggerService.cpp`, Non-Unity-Build ohne PCH) und sind damit identisch zur
realen Library-Umgebung. Der Wert `TOTAL` aus `-ftime-report` ist **CPU-Zeit**
(user + sys), nicht Wall-Clock – deshalb ist er auch unter Fremdlast vergleichbar;
`max resident set size` ist die vierte Spalte.

---

## Datensatz 1: `LoggerService.h`-Refactor (fmt + Forward-Declaration)

Vorher: `ee55c8d` · Nachher: `fcebfa7` + `<fmt/base.h>` im PCH
Ziel: `LoggerService.h` auf `<fmt/base.h>` + `namespace spdlog { class logger; }`
umstellen und das `log()`-Template nach fmt-Vorbild eliminieren
(`fmt::format` → `fmt::vformat`: das Template boxt nur noch mit
`fmt::make_format_args`, alles andere passiert im nicht-template `logImpl()`).
spdlog taucht nur noch in `LoggerService.cpp` auf.

### Header-Gewicht (Probe-TU mit `LOG_INFO`/`LOG_ERROR`, GCC `-fsyntax-only`)

| Metrik                        | vorher (`ee55c8d`) | nachher          | Δ           |
|-------------------------------|--------------------|------------------|-------------|
| Preprocessed Zeilen           | 104.378 | 84.450 | **−19,1 %** |
| Preprocessed Bytes            | 2.784.245 | 2.262.935 | −18,7 % |
| CPU-Zeit `-fsyntax-only` (TOTAL) | 4,42–4,43 s | 2,02 s | **−54,4 %** |
| max. resident set size        | 319 MB | 122 MB | **−61,8 %** |

Referenz: **`<spdlog/logger.h>` allein** = 97.735 Zeilen / 2.606.128 Bytes /
4,42 s / 312 MB. Der alte Public-Header war also fast vollständig spdlog
(104.378 Zeilen gesamt); nach dem Umbau ist das Format-Argument-Gewicht
(`<fmt/base.h>`, 2,6 k Zeilen) der Rest.

### Voll-Build (`measure-compile-times.sh --jobs 1 --nice --cold --incremental`)

| Stand                | Modus                    | Wall-Clock | Host-Load 1/5/15 vor → nach | ccache (cacheable) |
|----------------------|--------------------------|-----------:|-----------------------------|--------------------|
| vorher (`ee55c8d`)   | cold (ccache geleert)    | **511 s** | 2,75 3,05 3,14 → 3,18 3,13 3,16 | 0/14 Treffer |
| nachher              | cold (ccache geleert)    | **416 s** | 3,05 3,09 3,11 → 3,39 3,23 3,16 | 0/14 Treffer |
| nachher              | warm (ccache gefüllt)    | 243 s | 2,94 2,87 3,10 → 3,45 3,12 3,14 | 14/14 Treffer (100 %) |
| vorher (`ee55c8d`)   | inkrementell (`StringUtils.h`) | **278 s** | 3,18 3,13 3,16 → … | – |
| nachher              | inkrementell (`StringUtils.h`) | **245 s** | 3,39 3,23 3,16 → 3,80 3,48 3,28 | – |

Ergebnis: Voll-Build **−95 s (−18,6 %)**, inkrementeller Rebuild **−33 s (−11,9 %)**.
(Historischer Vergleichswert vom 02.09.2026 mit vollem Parallelismus:
`tests/unity_0` mit 470 k Zeilen / 71,8 s, Gesamt ca. 3,86 Mio. Zeilen – nicht
direkt vergleichbar, da andere Job-Anzahl.)

### Voll-Build mit Parallelismus (4 Jobs, ohne `nice`) – CI-nah

| Stand              | Modus                     | Wall-Clock | Host-Load vor → nach   | ccache |
|--------------------|---------------------------|-----------:|-----------------------|--------|
| vorher (`ee55c8d`) | cold (ccache geleert)     | 164 s | 3,26 2,49 2,24 → 4,99 3,90 2,84 | 0/14 |
| nachher            | cold (ccache geleert)     | **134 s** | 2,97 3,81 2,97 → 4,69 4,31 3,27 | 0/14 |
| nachher            | inkrementell (`StringUtils.h`) | **82 s** | 4,69 4,31 3,27 → 4,99 4,55 3,45 | – |
| vorher (`ee55c8d`) | inkrementell (`StringUtils.h`) | 92 s | – | – |

Ergebnis: **−30 s (−18,3 %)** Voll-Build, **−10 s (−10,9 %)** inkrementell – dieselbe
relative Verbesserung wie im Single-Core-Lauf, also kein Messartefakt der
Job-Anzahl. Host-Load steigt dabei erwartungsgemäß auf ~5 (4 Compiler auf 4 CPUs);
für Läufe neben laufenden Diensten die Single-Core-Zahlen oben verwenden.

### Reproduktion

```bash
git worktree add --detach /tmp/opencode/baseline ee55c8d
cp scripts/measure-compile-times.sh scripts/measure-header-weight.sh /tmp/opencode/baseline/scripts/
# im Worktree:
nice -n 10 scripts/measure-compile-times.sh --jobs 1 --nice --cold --incremental
nice -n 10 scripts/measure-header-weight.sh --build-dir build/build/Release --header LoggerService.h
# im Haupt-Tree identisch, damit beide Seiten denselben Aufbau haben
# (CI-nahe Variante: --jobs 4 ohne --nice)
```

---

## Datensatz 2: Boost-Entkopplung (Header ohne Boost)

Vorher: `250ff06` · Nachher: Arbeitsstand (Boost-Suffix-Refactor)
Ziel: schwere Boost-Header aus öffentlichen Headern entfernen. Betroffen waren
`boost/process`, `boost/interprocess` (Message-Queue, EventBus, Dateisperre) und
die Shared-Memory-Konstruktoren. Grundregel: **Boost-Typen stehen nur noch in
Implementierungsdateien oder in bewusst boost-gebundenen Headern**, nie in einem
Header, den fachliche Consumer includieren.

### Was geändert wurde

| Bereich | Lösung | API-Bruch |
|---------|--------|-----------|
| `ProcessResourceReader`, `ProcessInfo`, `ProcessInfoDto`, `ProcessService` | `boost::process::v1::pid_t` → `pid_t` (`<unistd.h>`), `child` nur vorwärtsdeklariert | `getProcessId()` liefert jetzt `pid_t` by value |
| `MessageQueue<T>` | nicht-template Kern `MessageQueueCore` (Pimpl), Boost in `MessageQueueCore.cpp` | nein (Template-API unverändert) |
| `EventBus` | `ShmSegmentAccessor` (boost-frei, 1 Zeiger) + `ShmSegmentAccessor::Allocator`-Interface; boost gebunden in `BoostSegmentAllocator.h` | Konstruktor nimmt `ShmSegmentAccessor` statt `segment_manager*` |
| `SingleInstanceBootstrapPlugin` | `Lock`-Klasse (boost `file_lock`) als private Pimpl im `.cpp` | nein |
| `SharedMemoryService` | boost-gebundene freie Funktionen in `shm::` (`ShmConstructors.h`), Segment-Zustand in `SharedMemorySegmentHandle` | **ja**: `constructX()` → `shm::constructX(service, …)`, `ShmString` → `shm::String`, `getSegmentManager()` → `shm::segmentManagerOf()` |

### Header-Gewicht (GCC `-fsyntax-only`, ohne PCH, `build/nonunity`)

| Header | vorher Zeilen / CPU | nachher Zeilen / CPU | Δ Zeilen | Δ CPU |
|--------|--------------------|----------------------|---------|-------|
| `ProcessInfo.h` | 144.195 / 3,28 s | 71.136 / 1,44 s | −50,7 % | −56,1 % |
| `ProcessResourceReader.h` | 139.902 / 3,08 s | 12.995 / 0,23 s | −90,7 % | −92,5 % |
| `ProcessService.h` | 187.623 / 5,28 s | 124.055 / 3,43 s | −33,9 % | −35,0 % |
| `ProcessInfoDto.h` | 152.060 / 3,56 s | 79.810 / 1,66 s | −47,5 % | −53,4 % |
| `MessageQueue.h` | 143.244 / 3,23 s | 62.930 / 1,27 s | −56,1 % | −60,7 % |
| `EventBus.h` | 89.532 / 1,68 s | 35.125 / 0,66 s | −60,8 % | −60,7 % |
| `SingleInstanceBootstrapPlugin.h` | 83.934 / 2,08 s | 76.335 / 1,96 s | −9,0 % | −5,8 % |
| `SharedMemoryService.h` | 168.813 / 4,30 s | 114.504 / 3,08 s | −32,2 % | −28,4 % |

`SharedMemoryService.h` ist **boost-frei** (im Präprozessor-Output kommt kein
`boost/interprocess` mehr vor); die verbleibenden 114 k Zeilen stammen aus
spdlog/fmt, Hypodermic und magic_enum – ein eigenes Thema.

`SingleInstanceBootstrapPlugin.h` und `EventBus`-Konsumenten bleiben
teilweise schwer, weil sie weiterhin `SharedMemoryRepository.h` einbinden, das
als generische Abstraktion `shm::String`/`shm::Map`/`shm::Vector` in der
Template-API nennt.

### Boost-Header je Übersetzungseinheit (`ninja -t deps`, Non-Unity)

| Boost-Header | vorher | nachher | verbleibende Nutzer |
|--------------|--------|---------|---------------------|
| `boost/process/v1/child.hpp` | 14 | 2 | `ProcessService.cpp`, `ProcessServiceTest.cpp` |
| `boost/interprocess/ipc/message_queue.hpp` | 9 | 2 | `MessageQueueCore.cpp`, Beispiel `message_queue.cpp` |
| `boost/date_time/posix_time/posix_time.hpp` | 8 | 1 | `MessageQueueCore.cpp` |
| `boost/interprocess/sync/file_lock.hpp` | 3 | 1 | `SingleInstanceBootstrapPlugin.cpp` |
| `boost/interprocess/managed_mapped_file.hpp` | 28 | 22 | alle über `ShmConstructors.h` bzw. `SharedMemoryRepository.h` |

### Bewusst nicht umgesetzt

- **Conan `without_options`** (`without_process`, `without_filesystem`, …): die
  Header werden trotzdem installiert, der Effekt wäre Installzeit/Platz, nicht
  Kompilierzeit. `BOOST_PROCESS_USE_STD_FS` (Boost.Process v1) wurde geprüft und
  bringt keinen messbaren Gewinn (`child.hpp` 138.918 → 138.914 Zeilen).

### Reproduktion

```bash
cmake -S . -B build/nonunity -DENABLE_PCH=OFF -DENABLE_UNITY_BUILD=OFF
cmake --build build/nonunity --parallel $(nproc)
nice -n 10 scripts/measure-header-weight.sh --build-dir build/nonunity --header EventBus.h
```

---

## Datensatz 3: Hypodermic (vendorter DI-Container)

Vorher: `250ff06` · Nachher: Arbeitsstand (Hypodermic-Patch)
Anlass: Wartezeit in CLion bei Änderungen unter `src/features/`.
`Hypodermic/ContainerBuilder.h` war der schwerste einzelne Header-Block des
Projekts: **151.116 präprozessierte Zeilen und 2.983 Boost-Dateien** allein,
obwohl der Container nur 95 kleine Header hat. Zum Vergleich `httplib.h` = 20,6 k
Zeilen. Verstärkt wurde das dadurch, dass `Feature.h`/`Features.h` den Container
per `#include` in jeden TU holten, der einen Feature-Typ anfasst.

### Was geändert wurde

| Datei (vendort) | vorher | nachher | Begründung |
|-----------------|--------|---------|------------|
| `TypeInfo.h` | `<boost/algorithm/string.hpp>`, `<regex>` | `<string>` + Inline-Loop | `<regex>` nur im `#else`-Zweig (Nicht-GNU) benutzt → toter Include; `algorithm/string.hpp` (94,8 k Z.) allein für `replace_all_copy(name, "::", ".")` in Z. 30 |
| `ComponentContext.h` | `<boost/range/adaptor/reversed.hpp>` | `rbegin()`/`rend()` | 62,2 k Z. für eine Rückwärts-Iteration |
| `ResolutionContainer.h` | `<boost/range/sub_range.hpp>` | gelöscht | 62,4 k Z., im ganzen Baum **nirgends benutzt** |
| `RegistrationActivator.h`, `IRegistrationDescriptor.h` | `<boost/signals2.hpp>` | `Hypodermic/Signal.h` | 109,0 k Z., der teuerste Einzel-Include des Projekts |
| `CMakeLists.txt` | `target_include_directories(... INTERFACE ...)` | `... SYSTEM INTERFACE ...` | Fremdcode gehört als System-Header behandelt |
| `LogLevel.h` | `(int) logLevel` | `static_cast<int>(logLevel)` | sonst `-Werror`-Blindgänger |

Dazu in der Projekt-Halbschale: `Feature.h`/`Features.h` forward-deklarieren
`Hypodermic::Container`/`ContainerBuilder` (beide werden nur als `&` bzw. in
`std::shared_ptr` gebraucht). IWYU-Folge: 7 Implementierungsdateien includen die
Hypodermic-Header jetzt selbst (`StartupBuilder.cpp`, `BaseFeature.cpp`,
`CommandLineFeature.cpp`, `HttpFeature.cpp`, `PropertyFeature.cpp`,
`examples/main.cpp`, `examples/worker.cpp`) – vier davon waren im Unity-Build
von Nachbar-TUs maskiert.

### `Hypodermic/ContainerBuilder.h` isoliert

| | vorher | nachher | Δ |
|---|--------|---------|---|
| präprozessierte Zeilen | 151.116 | 74.438 | **−50,7 %** |
| Bytes | 4.166.824 | 1.952.702 | **−53,1 %** |
| Boost-Dateien im Graph | 2.983 | 0 | −100 % |

### `-fsyntax-only` je betroffener TU (Non-Unity-Flags des TUs, Best-of-2)

| TU | vorher | nachher | Δ |
|----|--------|---------|---|
| `StartupBuilder.cpp` | 3,77 s | 2,29 s | −39,3 % |
| `examples/worker.cpp` | 5,48 s | 3,63 s | −33,8 % |
| `VirtualGroupBootstrapPlugin.cpp` | 2,88 s | 2,08 s | −27,8 % |
| `PropertyFeature.cpp` | 10,09 s | 8,19 s | −18,8 % |
| `CommandLineFeature.cpp` | 10,56 s | 8,59 s | −18,7 % |
| `HttpFeature.cpp` | 10,06 s | 8,18 s | −18,7 % |
| `BaseFeature.cpp` | 8,42 s | 7,10 s | −15,7 % |
| `examples/main.cpp` | ~6,06 s | 4,88 s | −19,4 % (≈) |
| **Summe** | **57,3 s** | **44,9 s** | **−21,6 %** |

`examples/main.cpp` ist nur näherungsweise belastbar: der `git show HEAD:`-Vorher-Stand
baut nicht gegen die parallel laufenden WIP-Header der Feature-Arbeit, dort
wurden nur die von diesem Datensatz geänderten Header substituiert.

### Der IDE-Fall: TU, die nur `Feature.h` includiert

Der eigentlich interessante Fall für CLion – z. B. eine TU, die nur
`builder.addFeature<HttpFeature>()` aufruft und den Container nie anfasst:

| | vorher | nachher | Δ |
|---|--------|---------|---|
| präprozessierte Zeilen | 153.570 | 55.711 | **−63,7 %** |
| `-fsyntax-only` | 2,84 s | 0,56 s | **−80,3 %** |

### Über alle TUs

218 Non-Unity-TUs, Summe der präprozessierten Zeilen: 24.150.450 → 23.658.075
(**−2,0 %**). Der Voll-Build gewinnt wenig, weil die 8 betroffenen TUs nur einen
kleinen Teil des Volumens ausmachen; die relevanten Ziele sind (a) die
Inkubationszeit der 8 TUs (−21,6 %) und (b) die Tatsache, dass eine Änderung an
`Feature.h`/`Features.h` in CLion keine TU mehr aufbohrt, die den DI-Container
selbst gar nicht benutzt.

### Nebenbefund: `-isystem` schützte vor `-Werror`

Beim Forward-Deklarieren flog `LogLevel.h:31` als `-Werror=old-style-cast` auf, obwohl
der Cast seit jeher existierte. Ursache: `src/include` ist als `-isystem` markiert,
Hypodermics eigenes Include-Dir nicht. Wer `LogLevel.h` also über `Feature.h`
erreichte, bekam alle Warnungen darin unterdrückt; wer es direkt includierte, nicht.
Die Forward-Declarations haben genau diese latente Landmine sichtbar gemacht. Als
Fix `SYSTEM` am Hypodermic-Target (korrekte Behandlung von Fremdcode) **plus**
`static_cast`, damit der Header auch als `-I`-Header warn-frei bleibt.

### Tests

`tests/external/HypodermicSignalTest.cpp` (12 Fälle) sichert die Semantik des
Signal-Ersatzes ab, die der Container benötigt: Aufrufreihenfolge, leere Slots,
`disconnect_all_slots`, `Connection::disconnect` (inkl. Idempotenz),
Mehrfachargumente, sowie die beiden Reentrancy-Fälle – Slot trennt sich während
des Emit selbst (das Muster aus `ContainerBuilder`) und Slot verbindet sich
während des Emit. Ohne den Reentrancy-Fall wäre eine naive Implementierung
durchgerutscht: die erste Fassung iterierte eine Kopie der Slot-Liste und rief
abgehängte Slots trotzdem noch auf.

Ergebnis: **353/353 grün im Unity- und im Non-Unity-Build.**

### Reproduktion

```bash
cmake -S . -B build/nonunity -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build/nonunity --parallel $(nproc)

# Gewicht des Containers isoliert
echo '#include "Hypodermic/ContainerBuilder.h"' | c++ -x c++ -E - \
  $(python3 -c "
import json;d=json.load(open('build/nonunity/compile_commands.json'))
print(next(e['command'] for e in d if 'StartupBuilder' in e['file']))") | wc -l
```

---

## ccache-Hinweise

- Der lokale Cache ist klein (Cache-Größe 0,06–0,45 % von 5 GB): die Unity-TUs
  sind so groß, dass ein Cache-Eintrag den Cache kaum füllt – die Trefferquote ist
  lokal aber **100 %**, sobald nur dieselben TUs erneut gebaut werden.
- Für den ersten CI-Lauf eines Commits ist die Quote naturgemäß niedrig
  (0 Treffer). Ein belastbarer Wert (> 70 %) braucht mindestens zwei
  aufeinanderfolgende Läufe mit unterschiedlichen, aber ähnlich großen
  Änderungen – der Cache-Key ist bereits auf `hashFiles(...)` (Inhalt) statt
  `github.sha` umgestellt, damit Zweitläufe denselben Key treffen.
- `CCACHE_BASEDIR` zeigt auf `${{ github.workspace }}` (GH-Runner) bzw.
  `/workspace` (Docker), `CCACHE_COMPRESS=1`, `CCACHE_MAXSIZE=500M`.
- **`CCACHE_DIR` ist seit 03.10.2026 explizit gesetzt** (GH-Runner:
  `${{ github.workspace }}/.ccache`, Docker: `/root/.ccache`): ccache ab 4.x
  nutzt sonst `~/.cache/ccache`, während `actions/cache` den in
  `~/.ccache` erwarteten Pfad sichern wollte. Folge war ein No-op-Cache –
  die Joblogs meldeten `Path Validation Error: Path(s) specified in the action
  for caching do(es) not exist, hence no cache is being saved`, und alle Läufe
  bis einschließlich Release `v0.1.0` waren objektseitig kalt (0 Treffer).
  **Hit-Rate-Messungen der CI sind erst ab dem ersten Lauf nach diesem Fix
  aussagekräftig.**
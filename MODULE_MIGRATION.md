# Module-Migration (C++20-Modules)

> Stand: 05.08.2026 – Umstellung der header-basierten Public-API auf C++20-Modules.
> Strategie: **Big-Bang ohne Shims**, Package-Module, bottom-up. Branch: `feature/cpp23-bump-and-modules`.
> Vorgelagert abgeschlossen: Makro-Split (`LoggerMacros.h`, `HistoryEntryMacros.h`, `PropertyMacros.h`, `StdIncludes.h`), Stage A (Ninja-Tree, `ENABLE_UNITY_BUILD=OFF`, 0 Warnings, ctest 21/21), Stage B (Feasibility-Spike: `export module` + `import`, P1689-Scan, GMF-Muster, Cross-Target-Import).

## Zielbild

- Public-API wird zu C++20-Modulen, benannt nach Packages:
  - `base_library.core.{exceptions, configuration, utils, models, property, persistence, persistence.postgresql, persistence.sqlite, services}`
  - `base_library.features.{base, http, cli, property}` (+ Untermodule)
  - `base_library.core.startup` (Composition Root)
- Makro-Header bleiben Plain-Header (Macros überleben keine Modulgrenzen) und werden per `#include` im Global-Module-Fragment bzw. von Consumer-TUs eingebunden.
- Externe Abhängigkeiten (Boost, spdlog, fmt, rapidjson, httplib, Hypodermic, magic_enum, date, …) sind nicht modularisierbar → kommen als `#include` ins Global-Module-Fragment.
- `import std;` erst, wenn Toolchains es voll unterstützen (GCC ≥ 15, libc++ voll) – verschoben.

## Technische Grundmuster (aus Stage-B-Spike verifiziert)

### Modul-Interface-Unit (`.cppm`)
```cpp
module;
#include "base_library/StdIncludes.h"        // STL-Aggregator
#include <spdlog/logger.h>                    // externe Header
#include "base_library/core/services/LoggerMacros.h"  // Makro-Header (Plain)

export module base_library.core.utils;

export class StringUtils { /* ... */ };       // exportierte Deklarationen
```

### Implementierungs-Unit
```cpp
module;
#include "base_library/StdIncludes.h"

module base_library.core.utils;               // eigenes GMF nötig – wird NICHT vom Interface geerbt

void StringUtils::trim(std::string &s) { /* ... */ }
```

### Consumer
```cpp
import base_library.core.utils;               // Module
#include "base_library/core/services/LoggerMacros.h"  // Macros via Plain-Header
```

## Abhängigkeitsanalyse (Ist-Zustand)

- Header-DAG: **keine Zyklen** (215 Header, Tarjan-SCC verifiziert).
- Paket-Ebene: **invertierte Kanten** vorhanden → Modul-Granularität nach Verzeichnissen ergäbe zyklische Imports. Auflösung in Phase 0 (Re-Layering).

| Invertierte Kante | Verursacher | Fix |
|---|---|---|
| `core/models → features/http` | `JsonSerializable.h` → `HttpBadRequestException.h` | Exception nach `core/exceptions` verschieben |
| `core/persistence → features/base` | `Connection/DatabaseConnectionConfigurations/Serialization` → `Configuration/DatabaseConnectionEntry` | Konfig-Typen nach `core/configuration` verschieben |
| `core/services → features/base` | `ProcessService`, `SharedMemoryService`, `LoggerService` → `ProcessName` | `ProcessService`/`SharedMemoryService` → `features/base/services`; `ProcessName` → `core/models` |
| `core/services → features/property` | `PropertyRegistration` → `Property<T>` | `Property`-Modelle + Exceptions nach `core/property` verschieben |
| `core/services → core/plugins → features/base` | `BootstrapService` → `BootstrapPlugin` → `features/base` | Plugins + `BootstrapService` → `features/base` |

## Phasen

### Phase 0 – Re-Layering ✅ (04.08.2026)
Datei-Verschiebungen aus der Tabelle, `#include`-Pfade anpassen, DAG verifiziert (azyklisch, 15 Pakete). Verifiziert: 0 Warnings, ctest 21/21.

Durchgeführte Moves:
- `features/http/service/HttpBadRequestException.h` → `core/exceptions/`
- **Config-Split**: 16 Header + 15 Cpps nach `core/configuration/` verschoben (Component, Configuration, ConfigurationException, DatabaseConnectionComponent, DatabaseConnectionEntry, Entry, EnvironmentConfiguration, Logger*, MessageQueue*, Process*, LoggerEnrty, LoggerSink/PathConfiguration); in `features/base/configuration/` verblieben nur `ConfigurationComponentBuilder`, `SharedMemorySegmentComponent`, `SharedMemorySegmentEntry`
- `core/persistence/ConnectionType.h` → `core/configuration/` (bricht config↔persistence-Zyklus)
- `features/base/models/{Process,ProcessGroup,ProcessName}.h` (+Cpps) → `core/models/`
- `core/plugins/*` (6 Header, 5 Cpps) → `features/base/plugins/`; `core/services/BootstrapService.{h,cpp}` → `features/base/services/`
- `core/services/{ProcessService,SharedMemoryService}.{h,cpp}` → `features/base/services/`
- `core/services/StringifyService.h` → `core/utils/` (header-only)
- `features/property/models/*` (4 Header + Property.cpp) → `core/property/`

**Resultierender azyklischer Paket-Graph** (bottom-up):
`core.exceptions`, `core.utils` → `core.models`, `core.property` → `core.configuration` → `core.services` → `core.persistence` → `features.base` → `features.http` → `features.cli`; `features.property` → {cli, http, base}. `core.StartupBuilder` = Composition Root (importiert alles).
Alle Include-Pfade via Skript angepasst (127 Dateien) + CMake-Listen aktualisiert (215 Header, 163 Cpps konsistent).

### Phase 1 – Build-Rahmen & Risiko-Spike

### Phase 1 – Build-Rahmen & Risiko-Spike
- **Spike-Erweiterung**: externer Header (z. B. Hypodermic) im GMF + exportiertes Template (`Feature<T>`, `SharedMemoryRepository`) kompilieren – kritischster Risikopunkt.
- **CMake**: `set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "2c20cdca-4eab-4ef3-9865-38eaa2046b81")`, Modul-Sourcen via `FILE_SET CXX_MODULES`, `ENABLE_UNITY_BUILD=OFF` (Pflicht), `ENABLE_PCH=OFF` (PCH × Module unter GCC riskant; Ersatz ccache), `-fmodules-ts` für alle `base_library`-TUs.
- **Makro-Finalisierung**: `ControllerMacros.h` aus `Controller.h` extrahieren; `config.h` bleibt Plain-Header (GMF bzw. Tests).
- **Modul-Inventar**: Mapping Package → Modulname → Dateien.

#### ⚠️ Spike-Befunde (GCC 14.2, verifiziert in `/tmp/opencode/module-spike` / `hypmod`)
Die Spike-Erweiterung ergab **harte Toolchain-Grenzen**, die den Plan signifikant einschränken:

| Muster | Ergebnis |
|---|---|
| GMF inkl. STL + externer Header; Consumer `import` + nutzt API (ohne Reinclude) | ✅ |
| `import` in Plain-Header | ✅ |
| Exportiertes Template, Body nutzt STL- / magic_enum-Typen; Consumer instanziiert | ✅ |
| Consumer: `#include <memory>`/`<optional>` **vor** `import` | ✅ |
| Consumer: irgendein `#include` **nach** `import` (pulls GMF'd Header transitiv) | ❌ Redefinitionen (bis 2000 Fehler) |
| Hypodermic-Typen in exportierter Signatur + Consumer nutzt sie | ❌ **ICE/Segfault** (GCC 14.2) |
| Hypodermic in Modul-Impl-Unit-GMF | ❌ **ICE** |
| Forward-Declaration eines Modul-Typs im Global Module | ❌ „redeclaring … conflicts with import“ |
| `StdIncludes.h` (STL-Aggregator) im GMF + utils-Exporte; Consumer = große TU | ❌ **ICE** `propagate_necessity` (`tree-ssa-dce.cc:1001`) – Workaround: `StdIncludes.h` **nicht** ins GMF; die Utils-Platform-Header sind selbst-enthalten |

**Konsequenzen:**
1. **Hypodermic darf keine Modulgrenze überschreiten.** Der DI-Kern (Feature.h, Features.h, Feature-Klassen, StartupBuilder, VirtualGroupBootstrapPlugin, alle `registerType`-Komponenten-Cpps) **bleibt header-basiert** und `import` die Module. Nur Hypodermic-freie Schichten werden Module.
2. **Include-Reihenfolge-Regel** (GCC-Pflicht): In jeder TU stehen ALLE `#include` vor dem ersten `import`. Headers, die Module importieren, müssen also zuletzt inkludiert werden.
3. **Keine Forward-Declarations über Modulgrenzen** – Header, die Modul-Typen referenzieren, müssen `import`.
4. Nur GCC 14.2 installiert (kein GCC 15/16 als Fix verfügbar) → Einschränkungen bindend.
5. **`StdIncludes.h` nicht ins Modul-GMF** (nur in `#include`-Konsumenten-TUs): Der aggregierte STL-Include im GMF des `base_library.core.utils`-Moduls löst bei Import in große TUs (AuthService, ProcessController, …) einen deterministischen GCC-14.2-DCE-ICE aus. Ohne den Aggregator baut das Modul (Utils-Header sind selbst-enthalten) und der gesamte Build ist grün. Das `base_library.core.exceptions`-Modul (kleiner Inhalt) verträgt `StdIncludes.h` im GMF weiterhin.

**Revidiertes Zielbild:** `base_library.core.*` (exceptions, utils, models, configuration, property, services, persistence, persistence.postgresql, persistence.sqlite) werden Module. `base_library.features.*` + DI-Hülle bleiben Header, importieren die core-Module und folgen der Include-Reihenfolge-Regel. (Volle Modularisierung inkl. features.* ist mit GCC 14.2 + Hypodermic nicht stabil möglich.)

### Phasen 2–11 – Konversion bottom-up
Je Layer: 1 Modul-Interface aus den Header-Inhalten (Export-Marker, Include-Guards raus, `friend`/Templates ins Interface), Impl-`.cpp` auf `module X;`, Layer isoliert bauen.

1. `core.exceptions` (+ `HttpBadRequestException`)
2. `core.configuration` (`Configuration`, `DatabaseConnectionEntry`)
3. `core.utils`
4. `core.models` (+ `ProcessName`) + `core.property` (`Property<T>`, `PropertyBase`, `PropertyRepositoryType`, `DataStorage`, Property-Exceptions)
5. `core.persistence` + `core.persistence.postgresql` + `core.persistence.sqlite` (`Serialization<T>`-Spezialisierungen via `IMPLEMENT_SERIALIZE` ins Interface, P2388-Risiko)
6. `core.services` (`AbstractService`, `LoggerService`, `Persistable*`, `FileService`, `DirectoryService`, `SignalService`, `StopWatchService`, `StringifyService`, `PropertyRegistration`, `ISharedMemoryService`)
7. `features.base` (größter Layer: models → configuration → msg → provider → repositories → services [inkl. verschobener `ProcessService`/`SharedMemoryService`/`BootstrapService`/Plugins] → controller/DTOs)
8. `features.http` (zuvor `ControllerMacros.h` extrahieren; controllers/service/configuration/provider)
9. `features.cli`
10. `features.property`
11. `core.startup` (`StartupBuilder` – Composition Root, importiert alles)

### Phase 12 – Flip & Consumer
- `base_library` komplett als Modul-Lib; alle 163 Impl-`.cpp` auf `module X;` + eigenes GMF.
- Tests + Examples von `#include` auf `import` umgestellt; Catch2/trompeloeil bleiben Plain-Header.
- Modul-Interfaces + Modul-Dateien installieren; Header-Installation (optional, für Nicht-Modul-Consumer) entscheiden.
- Final: Ninja-Build `-j2`, 0 Warnings, `ctest` 21/21, Examples lauffähig.
- Makefile-Tree `build/Release` ist nach der Umstellung obsolet (Makefile-Generator unterstützt keine Module) → entfernen/ignorieren; es zählt `build/Release_ninja/build/Release`.

## Risiken & offene Punkte

- **GMF-Templates**: exportiertes Template, dessen Body GMF-Typen (Hypodermic, rapidjson, boost) nutzt → im Spike verifizieren (GCC ok, Clang prüfen).
- **P2388**: externe explizite Spezialisierung von Modul-Templates → Serialization-Spezialisierungen ins Modul-Interface ziehen.
- **PCH × Module**: GCC-PCH mit `-fmodules-ts` unzuverlässig → Phase 1 `ENABLE_PCH=OFF`, später optimieren.
- **`db`-Namespace**: `friend`-Deklarationen (`Transaction`/`Statement`/`PreparedStatement`/`Notify`) bleiben innerhalb des Moduls gültig.
- **GCC 14-False-Positives** (unity-OFF, `-O3`): `-Wno-array-bounds`, `-Wno-stringop-overflow`, `-Wno-stringop-overread` bereits in `cmake/CompilerWarnings.cmake` deaktiviert.
- **Verteilung**: Modul-Consumer brauchen Toolchain-Unterstützung (GCC ≥ 11 / Clang ≥ 16 / MSVC); `import` in konsumierenden Projekten.

## Verifikationskriterien

- Nach jeder Phase: DAG-Skript azyklisch; betroffene Layer-Targets isoliert kompilierbar.
- Nach Phase 12: `ninja -C build/Release_ninja/build/Release -j2` 0 Warnings; `ctest --test-dir build/Release_ninja/build/Release` 21/21; Examples laufen.

# ROADMAP – Veröffentlichung von `cpp-base-library`

> Stand: 20.09.2026 · Ziel: öffentliches Open-Source-Release (MIT)
> Basis: aktuelle Analyse (341 Tests grün in Unity- UND Nicht-Unity-Build) + Inventur aus `TODO.md`, `ANALYSIS.md`, CI/CMake/Docker/README.

## Prioritäts-Legende

| Symbol | Bedeutung |
|--------|-----------|
| 🔴 | **Blocker** – muss vor einem öffentlichen Release geklärt sein |
| 🟠 | **Hoch** – wichtig für Release-Qualität und Vertrauen |
| 🟡 | **Mittel** – Qualität/Innenwirkung, nicht release-kritisch |
| 🟢 | **Niedrig / Nice-to-have** – nach Release |

---

## Die 5 Leitfragen in Kürze

1. **Software**: regressions Tests nachziehen, PostgreSQL-Backend testen, Seed-Credentials entfernen, Versionierung + Swagger synchronisieren.
2. **Repo-Umzug**: **Ja, vor Release durchführen** – `<kruempelmann::>`-Namespace, globaler C++-Namespace und drei Namenswelten (`cpp-base-library`/`base_library`/`kruempelmann`) sind schwerwiegende strukturelle Risiken. Jetzt ist es am günstigsten (keine Nutzer).
3. **Erreichbarkeit**: Repo öffentlich machen, LICENSE, README-Quickstart + Badges, Doxygen-Docs, CONTRIBUTING/SECURITY/CHANGELOG.
4. **Kompilierzeit**: umgesetzt (20./21.10.2026) – ccache Cross-Env, Test-Unity-Batch aufgeteilt, PCH gestrafft, HTTP-pimpl, IWYU/Non-Unity-Fixes, **`LoggerService.h` ohne spdlog** (fmt-Vorwärtsdeklaration + Template-Eliminierung). Offen: Boost-Install schrumpfen, C++20-Module neu bewerten. Messprotokoll mit Vorher/Nachher-Zahlen in `docs/compile-time-measurements.md`, Details in Abschnitt 4.
5. **Sonstiges**: CI auf `master` fixen (läuft aktuell nie), Release-Workflow, Versionierung, Secrets aus Historie entfernen.

---

## 🔴 Phase 0 – Release-Blocker (vor Go-Live)

### Secrets & persönliche Daten (kritischste Punkte)

- [ ] **Klartext-Passwort `${ADMIN_PASSWORD}` entfernen + rotieren** – liegt in `cfg/bootstrap.xml:41` (auskommentiert) und `examples/db.cpp:14`. Steckt bereits dauerhaft in der Git-Historie → auch aus allen Branches/History löschen (`git filter-repo`/`filter-branch`), Passwort auf betroffenen Systemen rotieren.
- [ ] **Commitierten TLS-Private-Key entfernen** – `cfg/certs/server.key` ist committed. Keys nie ins Repo; `generate_certs.sh` bleibt für lokale Dev-Zertifikate, generierte Dateien in `.gitignore` aufnehmen.
- [ ] **Seed-Credentials bereinigen** – `cfg/database/DEFAULT_SQLITE/data_schema_default_version_1.sql` und `DEFAULT_PSQL/...` legen Admin-User `dominik` mit festem Hash an. Ersetzen durch neutralen Beispiel-User + dokumentiertes Default-Passwort; Hash-Migration beachten.
- [ ] **Personenbezogene Dev-Daten ersetzen** – Namen/Adressen/private Texte in `examples/main.cpp`, `examples/message_queue.cpp`, `tests/services/PropertyServiceTest.cpp` durch neutrale Beispieldaten ersetzen.
- [ ] **Absolute Pfade parametrisieren** – `/home/dominik/...` (`cfg/bootstrap.xml:7,9`), `/Users/dkruempe/...` (`examples/named_pipe_server.cpp`), `~/dev/log/...` → über `CONFIG_DIRECTORY`/`PROJECT_PATH`/relative Pfade auflösen. Sonst laufen weder Beispiele noch Docker-`app` außerhalb der Dev-Maschine.

### Lizenz & Rechtliches

- [ ] **`LICENSE`-Datei anlegen** (MIT) – README verweist bereits darauf, die Datei existiert aber nicht. Ohne sie ist die GitHub-Lizenz-Erkennung und damit der rechtliche Rahmen leer.
- [ ] **Vendored Hypodermic attribuieren** – `external/Hypodermic/` (MIT, letzter Stand 2017) enthält keine Lizenzangabe. Eigene `LICENSE`/`NOTICE` im Ordner + README-Erwähnung nötig.
- [ ] **`NOTICE`/`THIRD_PARTY_NOTICES`** für alle externen Dependencies (Conan: Boost, spdlog, fmt, cpp-httplib, rapidjson, date, magic_enum, tinyxml2, libpq, sqlite, openssl, zlib, Catch2, trompeloeil, benchmark; vendored: Hypodermic).

### Repo & CI (Voraussetzung für jede Sichtbarkeit)

- [ ] **CI-Trigger auf `master` fixen** – `.github/workflows/ci.yml` triggert auf `main`, der Default-Branch heißt aber `master` → die Pipeline feuert **nie**. Entweder Branch umbenennen oder Trigger anpassen.
- [ ] **Repo öffentlich machen** – aktuell `PRIVATE`, Description leer, keine Topics, keine Homepage (`gh repo view`).

---

## 1. Software-seitig noch zu tun

### Tests nachziehen (🟠 Hoch)

- [ ] **PostgreSQL-Backend testen** – komplett ungetestet (kein einziger `[pg]`-Test): Server-Side-Cursor, Notify, PreparedStatement, Transaction. Prüfen, ob PG bereits Grundlage der CLI/HTTP-Tests ist. CI braucht dann einen Postgres-Service (`services: postgres`) oder `docker compose up postgres`.
- [ ] **Regressionstests für die 42 dokumentierten Bugfixes** – laut `TODO.md` offen: Regex-Vertauschung in `MessageQueueRepository`, `MemorySize` mit `"kB"`, leere Pfade, `PropertyDataDto::getSize`, non-object JSON-Body.
- [ ] **Weiche Tests härten** – HTTP-Tests mit `WARN`-Fallback *(bestehen auch bei Fehlschlag)* in echte `REQUIRE`s umwandeln; `SKIP`-Pfade (fehlendes `sleep`-Binary, Nicht-Linux) über Tags/Requirements statt stillem Skip.
- [ ] **Tests für ungetestete Module**: Bootstrap-Plugins (Admin/Database/MessageQueue/VirtualGroup), StartupBuilder, Scheduler/Executor, CLI-Komponenten (History/MessageQueue/Process/SharedMemory), HTTP-Controller (Process, History, User), Property-Repositories (Database/File/SharedMemory), `LoggerService`.
- [ ] **`setenv`-Nebenwirkungen isolieren** – globale `CONFIG_DIRECTORY`-envs in Tests machen die Reihenfolge fragil (z. B. `ProcessServiceTest.cpp:48`).

### Code-Reinigung (🟡 Mittel)

- [ ] **Unfertige/leere Codepfade entscheiden** – bewusst erklären oder implementieren: `FilePropertyRepository::save()` (no-op), `SharedMemoryPropertyRepository::onMigrate()`, `BaseFeature::initialize()`, `UserManagementCliComponent::onShowMenu()` u. a. Mindestens als dokumentiertes No-Op markieren.
- [ ] **`DatabasePropertyRepository` Fehlerpropagation** – schluckt `db::SQLException` (nur `LOG_ERROR`, leere Ergebnisliste). Fehler nach außen durchreichen oder explizit dokumentieren.
- [ ] **`ProcessController` 501 statt 404** bei unbekanntem Prozess – API-Semantik überdenken und mit Swagger abgleichen.
- [ ] **Tippfehler in öffentlichen Headern** – `LoggerEnrty.h`, `SharedMemoryRepsoitoriesDto.h` vor Release umbenennen (öffentliche API!), da sonst dauerhaft Breaks entstehen.
- [ ] **`VirtualGroupBootstrapPlugin` `HACK`** – Gruppen-Klassifikation über Namens-Präfix ("Admin"/"User") statt Konfigurations-Flag (`src/core/plugins/VirtualGroupBootstrapPlugin.cpp:58`).

### Offene Features (🟢 Niedrig / bewusst offen)

- [ ] MySQL-Support – bewusst zurückgestellt; im README als Roadmap-Punkt listen.
- [ ] Hypodermic → Boost.DI ersetzen (unmaintained seit 2017) – nach Release als größerer Refactor, nicht davor.
- [ ] Docker-Container für macOS-Tests – nach Release.
- [ ] `docker-compose`-Defaults reviewen (Postgres `test/test` auf 5432, App ohne TLS).

---

## 2. Struktureller Umzug auf neuen Repo-Namen – **Empfehlung: JA, vor dem Release durchführen**

### Begründung

- `kruempelmann::base_library` / `kruempelmann::Hypodermic` ⇒ personenbezogener Export-Namespace, für OSS ungeeignet.
- **Alle öffentlichen Klassen liegen im globalen C++-Namespace** (kein `base_library::`). Für eine Public-Header-Library das größte strukturelle Risiko (Symbol-Kollisionen, kein Autocomplete-Präfix).
- Drei parallele Namenswelten (`cpp-base-library` / `base_library` / `kruempelmann`) verwirren Nutzer; Guard-Altlasten `CPP_SYSTEM_LIBRARY_*`, `CPP_BASIC_LIBRARIES_*` überleben in 4 Dateien.
- Kein einziger Nutzer vorhanden → **Umbenennung ist jetzt am günstigsten** und später praktisch unmöglich (veröffentlichte Include-Pfade/Target-Namen brechen).

### Entscheidungen treffen

- [ ] Zielnamen festlegen (z. B. Library-Target/Namespace/Include-Pfad einheitlich; Beispiele: `base_library::`, anderer neutraler Name). Mindestens Version des bestehenden Install-Pfads `include/base_library/**` empfehlen.

### Mechanische Umbenennung

- [ ] `kruempelmann::` → neutraler Namespace: `src/CMakeLists.txt` (5×), `external/Hypodermic/CMakeLists.txt` (2×), `examples/CMakeLists.txt` (9×), `tests/CMakeLists.txt`, `benchmarks/CMakeLists.txt` (~19 Treffer).
- [ ] Top-Level `project(cpp-base-library)` + Docker-Image-Name (`docker-compose.yml:22`) + README/Swagger-Titel angleichen.
- [ ] **App-Name in Install Export**: `INSTALL(EXPORT ... NAMESPACE ...)`, `ALIAS` und `find_package`-Verzeichnis vereinheitlichen.
- [ ] Platzhalter-APIs neutralisieren: `myproject_*`/`MYPROJECT_*`/`project_warnings` (Top-Level-CMake, `cmake/StaticAnalyzers.cmake`).
- [ ] Include-Guard-Altlasten `CPP_SYSTEM_LIBRARY_*` / `CPP_BASIC_LIBRARIES_*` in 4 Dateien mitbereinigen.
- [ ] Git-Remote auf neutralen Zielaccount/Reponamen umstellen (`git@github.com:dkruempe/cpp-base-library.git`).

### C++-Namespace einführen (strukturelle API-Änderung)

- [ ] Öffentliche Klassen in `base_library::`-Namespace heben (bzw. Zielnamen). Betrifft alle Public-Header; größter Einzelaufwand, gehört vor die erste Veröffentlichung.
- [ ] Sub-Namespaces (`db`, `postgresql`, `sqlite`, `Hypodermic`-Forward) einbetten/kollisionsfrei benennen.
- [ ] Migrationstabelle im CHANGELOG dokumentieren (einzige Breaking-Change, die es je geben wird).

### Install/Packaging dabei gleich fixen

- [ ] **`find_package(base_library)` funktionsfähig machen** – **fehlt `base_libraryConfig.cmake`** (kein `configure_package_config_file` vorhanden; nur `base_library_targets.cmake` + Versionsdatei werden installiert). 🔴
- [ ] **Relokation** – `INSTALL_INTERFACE` enthält Build-Maschinen-Absolutpfade/Conan-Variablen (`src/CMakeLists.txt:511`) → `${CMAKE_INSTALL_PREFIX}`-relative Angaben. Installiertes Package muss auf anderen Maschinen funktionieren. 🔴
- [ ] Doppelt gelistete `install(FILES)`-Einträge bereinigen (`src/CMakeLists.txt:572`, `external/Hypodermic/CMakeLists.txt:31`).
- [ ] `LICENSE`, `README`, `swagger.yaml` mitinstallieren; `project_warnings`-Interface nicht in den öffentlichen Export aufnehmen.
- [ ] **`conanfile.py`** für eigenen Package-Export anlegen (nur `conanfile.txt` vorhanden) – als Library unter `conan-center-index` einreichen ist optional; lokal `conan export`/`conan create` für Nutzer.
- [ ] **Version zentral setzen**: Root-`project(cpp-base-library VERSION x.y.z)`, `src/`-Target-Version daraus ableiten.

---

## 3. Bessere Erreichbarkeit

### README auf Vordermann (🟠 Hoch)

- [ ] **Build-Anleitung mit CI konsistent machen** – README nutzt `cmake-build-debug`, CI/AGENTS `build/build/Release` + Ninja + `-DCMAKE_BUILD_TYPE`. Ein Layout + Flags dokumentieren.
- [ ] **Quickstart (5 Minuten)** – Minimalbeispiel mit `StartupBuilder::with(argc, argv)`, 1–2 Features und `bootstrap.xml`; plus ein echtes Code-Snippet im README (statt nur Verweis auf `examples`).
- [ ] **CI-Status-Badge + Quality-Badges** (Build/Test, Coverage optional).
- [ ] **Docker-Abschnitt** – `docker compose --profile dev|test|app`-Workflows dokumentieren (README erwähnt Docker mit keinem Wort).
- [ ] **Run-Anleitung pro Beispiel** (welches Beispiel zeigt was, `CONFIG_DIRECTORY`, TLS, SQLite vs. Postgres). `examples/playground.cpp` zu einem echten "Hello World" ausbauen oder entfernen.
- [ ] Test-Abschnitt: `--label-regex unit`, PostgreSQL-Abhängigkeit (`docker compose up postgres`) erwähnen.

### Offizielle OSS-Dokumente (🟠 Hoch)

- [ ] `LICENSE` (siehe Phase 0), `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `SECURITY.md`, `CHANGELOG.md`, `NOTICE`.
- [ ] GitHub-Metadata: Description (EN), Topics, Homepage; Repo public stellen.

### API-Dokumentation (🟡 Mittel)

- [ ] **`Doxyfile` + `docs/`** – 225/228 Public-Header haben Doxygen-Kommentare, aber es wird nie HTML generiert. Doxygen-Job in CI + GitHub Pages optional.
- [ ] **`swagger.yaml` vs. `swagger.md` synchronisieren** – `swagger.md` ist veraltet (fehlt `/health`, `/ready`), beide um neue Endpoints `PUT /process/restart/{id}`, `POST /process/reset/{id}`, `GET /process/health/{id}` ergänzen; Versionsnummern abgleichen (Swagger `1.0.0` vs. CMake `0.0.1`).

### Vorbildliche Beispiele (🟡 Mittel)

- [ ] `examples/main.cpp` auf neutrale Daten umstellen und in Teilbeispiele zerschneiden (HTTP, CLI, Property, Persistence getrennt).
- [ ] `examples/db.cpp`-Credential `${ADMIN_PASSWORD}` ersetzen; `named_pipe_server.cpp`-Pfade parametrisieren.

---

## 4. Kompilierzeit-Optimierung

Empirische Basis (gemessen 02.09.2026): Library 6 Unity-TUs (~2,05 Mio. präprozessierte Zeilen), Tests 2 TUs (die größte `tests/unity_0` mit **470 k Zeilen / 71,8 s**), Examples ~1,21 Mio. Zeilen. Gesamt ca. 3,86 Mio. Zeilen.

**Aktuelles Referenzprotokoll (21.10.2026):** `docs/compile-time-measurements.md` – alle Zahlen dort sind **Single-Core** (`--jobs 1`, `nice -n 10`), damit die parallel laufenden Dienste der Maschine nicht blockiert werden; Host-Load vor/nach jedem Lauf ist mitprotokolliert.

### P0 – schnellste Gewinne

- [x] **Test-TU aufteilen** – UMGESETZT (20.09.2026): `UNITY_BUILD_BATCH_SIZE 8` als Target-Property auf `base_tests` (statt global 32) → `tests/unity_1..unity_4`, insgesamt 5 TUs statt 1. Unity- UND Nicht-Unity-Build 341/341 grün.
- [x] **ccache cross-environment tauglich machen** – UMGESETZT (20.09.2026): `CCACHE_BASEDIR` (GH-Runner `${{ github.workspace }}`, Docker `/workspace`), `CCACHE_COMPRESS=1`, `CCACHE_MAXSIZE=500M` in `ci.yml`, `Dockerfile`-builder und `docker-compose.dev` (mit persistentem ccache-Volume); Cache-Key auf Content-Hash (`hashFiles(...)`) statt `github.sha`. **Offen:** Hit-Rate > 70 % erst nach mehreren CI-Läufen verifizierbar (lokal weiterhin ~32 % – dominiert von Unity-TUs + Erst-Compile der Non-Unity-Verifikation).
- [x] **Versions-Makros + Versionierung fixen** – UMGESETZT (20.09.2026): `@VERSION_MAJOR @` → `@VERSION_MAJOR@` in `config.h.in`; Root `project(cpp-base-library VERSION ${BASE_LIBRARY_VERSION})` (CACHE-Var `BASE_LIBRARY_VERSION=0.1.0`), `src/` leitet Target-Version und `VERSION_*` daraus ab. `config.h` generiert korrekt (`VERSION_MAJOR 0/1/0`). Offen: Swagger-Abgleich (→ §5).

### P1 – Header-Sanierung (Voraussetzung für weitere Gewinne)

- [x] **PCH straffen** – UMGESETZT (20.09.2026): die 4 Projekt-Header (`LoggerService.h`, `Configuration.h`, `StringUtils.h`, `TypeName.h`) aus dem PCH entfernt; PCH enthält nur noch stdlib + `<spdlog/common.h>` + `<spdlog/fwd.h>`. Kein Unity-Build-Effekt, dafür keine `.gch`-Invalidierung bei Projekt-Header-Änderungen. **Nachtrag 21.10.2026:** die beiden spdlog-Einträge sind durch `<fmt/base.h>` ersetzt – seit dem `LoggerService`-Umbau ist kein spdlog-Header mehr im Projekt-`#include`-Graph, `<fmt/base.h>` dagegen (fast) in jeder TU.
- [x] **`LoggerService.h` auf Forward-Declarations + fmt umstellen** – UMGESETZT + NACHGEMESSEN (21.10.2026). Der starre Weg (`spdlog/fwd.h` 1:1) war nicht möglich, weil das `log()`-Template den kompletten `spdlog::logger`-Typ brauchte; umgesetzt wurde der oben skizzierte „realistische Ansatz": Header parst nur `<fmt/base.h>` und forward-declared `namespace spdlog { class logger; }`, spdlog lebt nur noch in `LoggerService.cpp`; das `log()`-Template folgt dem fmt-Muster (`format` → `vformat`), boxiert seine Argumente mit `fmt::make_format_args` und ruft das **nicht-template `logImpl()`** (Level-Filter + `fmt::vformat` + `source_loc`) auf – Formatierung/Instantiation einmal in der `.cpp` statt an jeder Call-Site. Dazu neues `enum class LogLevel` (Werte = `spdlog::level::level_enum`), `BASE_LIBRARY_FUNCTION` statt `SPDLOG_FUNCTION`, defensives `try`/`catch` um `vformat` (kaputter Formatstring → Meldung statt Exception-Flucht), PCH auf `<fmt/base.h>` umgestellt. `LOG_*`-Makros bleiben aufrufer-seitig unverändert; direkte `LoggerService::get().log(...)`-Aufrufe mit spdlog-Typen müssten die neue Signatur nutzen (im Repo keine solchen Aufrufe). **Messwerte (Single-Core, GCC, 2 Läufe, stabil):** Header-Gewicht einer `LOG_*`-TU 104.378 → 84.450 Zeilen (−19,1 %), Bytes 2.784.245 → 2.262.935 (−18,7 %), CPU `-fsyntax-only` 4,43 s → 2,02 s (−54,4 %), RSS 319 → 122 MB (−61,8 %); Voll-Build ccache-leer 511 s → 416 s (**−18,6 %**), inkrementell nach `StringUtils.h` 278 s → 245 s (−11,9 %), warm 243 s bei 14/14 ccache-Treffern – mit 4 Jobs gegengeprüft (164 s → 134 s = −18,3 %, inkrementell 92 s → 82 s), also kein Single-Core-Messartefakt. Referenz: `<spdlog/logger.h>` allein = 97.735 Zeilen/4,42 s – der alte Header war fast nur spdlog. Pool: Unity- UND Nicht-Unity-Build (PCH aus) 341/341 grün, Log-Verhalten per Smoke-Test geprüft (Format-Specifier, Level-Filter, Exception-Fallback), `clang-format` auf dem Header clean.
- [x] **HTTP-pimpl für `Server.h`/`Client.h`/`HttpClientHelper.h` umgesetzt** – `httplib.h` (20,6 k Zeilen) ist die teuerste Einzel-Datei, zieht in alle 20 HTTP-TUs. `Server.h`/`Client.h` halten jetzt nur noch Forward-Declarations (`httplib::Result` etc.); `HttpClientHelper.h` deklariert nur noch `hasResponse` (Definition in neuem `HttpClientHelper.cpp`). Konsequenz: `httplib.h` muss in TUs, die `httplib::*`-Typen nutzen und diese Header einbinden, jetzt selbst via `#include <httplib.h>` importiert werden (IWYU; explizit ergänzt in den 6 Api-Implementierungen). Pool: 341/341 Tests grün bei Head-rebuild (inkl. Fix des via `ProcessService.cpp` aufgedeckten boost `v1::env`-Include-Fehlers), `clang-format --dry-run --Werror` sauber.
- [x] **IWYU-Einzelfunde** – UMGESETZT (20.09.2026): `boost/asio.hpp`→`boost/asio/ip/address.hpp` (AuthService.cpp); `boost/process/v1.hpp`→`v1/child.hpp`+`v1/env.hpp` (ProcessService.cpp); `Feature.h` +`<type_traits>/<memory>/<string_view>`; `date/tz.h`→`date/date.h` in 8 Dateien (kein tz-Bedarf, verifiziert). `fmt/format.h`→`fmt/core.h`: **bewusst verworfen** – fmt 12.1 `core.h` ist nur ein Deprecation-Shim auf `format.h`, kein Kompiliergewinn.
- [x] **Nicht-Unity-Build testen** – UMGESETZT (20.09.2026): `-DENABLE_UNITY_BUILD=OFF`-Release-Build grün (341/341). Deckte **8 Unity-maskierte Include-Bugs** auf, alle gefixt: `Argument.h` (`<cstdint>`), `AbstractService.h` (`<memory>`), `ConfigurationException.h` (`<string>/<cstdint>`), `CommandLineComponent.h` (`<set>/<string_view>`), `AbstractCommandLineMenu.cpp` (`<iostream>/<set>/<algorithm>/<iterator>`), `MessageQueueRepository.cpp` + `MessageQueueBootstrapPlugin.cpp` (`LoggerService.h`), `SingleInstanceBootstrapPlugin.cpp` (`SharedMemorySegmentEntry.h`).

### P2 – Metabuild & Messbarkeit

- [x] **Mess-Skripte versionieren** – ERLEDIGT (21.10.2026): `scripts/measure-compile-times.sh` (Cold/warm + inkrementelles Delta, Wall-Clock + Host-Load + `ccache -s`) und `scripts/measure-header-weight.sh` (präprozessierte Größe + `-fsyntax-only`-CPU-Zeit einer `LOG_*`-TU). **Zwei echte Lücken gefunden und behoben:** (a) `scripts/` war komplett von `.gitignore` (`*.sh`) verschluckt – die Skripte waren trotz „versioniert" **nicht** im Repo; jetzt per `!scripts/*.sh` ausgenommen und eingecheckt. (b) `--incremental` war ein exklusiver Modus und hat `--cold` verschluckt (ccache wurde nie geleert) – jetzt kombinierbar, sodass ein Lauf `scripts/measure-compile-times.sh --jobs 1 --nice --cold --incremental` beide Referenzzahlen liefert. **Referenz-Messprotokoll (vorher/nachher, Single-Core) liegt jetzt in `docs/compile-time-measurements.md`** inkl. Umgebungs-/Reproduktionsregeln und beider Datensätze (LoggerService-Refactor: 511 s → 416 s cold).
- [ ] Boost-Install schrumpfen (`boost*:without_*`, 218 MB Install) – spart Installzeit, kein Kompilier-Effekt. **VERSCHOBEN (20.09.2026, bestätigt 21.10.2026):** Options-Änderung löst einen lokalen Boost-Rebuild aus. Auf dieser Produktionsmaschine mit laufenden Diensten (evcc) wäre der Rebuild nur mit `--jobs 1` vertretbar und dauert dann entsprechend lange; sinnvolles Zeitfenster mit mehr als einer freien CPU vergeben – der Punkt ist reine Installzeit-Optimierung und blockiert nichts.
- [ ] **C++20 Module**: aktuell nicht realistisch (CMake 3.16, alle Kernsachen header-only ohne Module) – nach IWYU/Umbau erneut prüfen.

---

## 5. Sonstiges

### Release-Mechanik

- [x] **`project()`-Version im Root** setzen + `config.h.in`-Platzhalter fixen – UMGESETZT (20.09.2026, siehe §4 P0).
- [ ] **generierte `config.h`-Version mit Swagger abgleichen** (`1.0.0` vs. `0.1.0`) – offen.
- [ ] **SemVer + Git-Tags** einführen (z. B. `v0.1.0`), CHANGELOG anlegen (Keep a Changelog), Commits künftig Conventional-Commit-Stil.
- [ ] **GitHub Release-Workflow**: Trigger `on: pull_request` (PR-Test) + `on: push: tags` / `on: release`: Build → `ctest` → Artifacts → `gh release create`.
- [ ] **`base_libraryConfig.cmake`** + relozierbares Install-Package (siehe Abschnitt 2) vor erstem Tag.

### CI-Qualität & Plattformabdeckung

- [ ] **CI-Trigger auf `master`** (oder Branch umbenennen zu `main`) – aktuell tote Pipeline. 🔴
- [ ] **Plattform-Matrix ausbauen** – min. GCC+Clang auf Linux, optional macOS/Windows; Debug-Build zusätzlich.
- [ ] **`clang-format --check` + clang-tidy als CI-Job** – `-Werror` ist Build-intern aktiv, Format/Lint aber NICHT CI-erzwungen.
- [ ] **Postgres-Service in CI** für `[pg]`-Tests (`services: postgres`), sobald Tests existieren.
- [ ] **Dependabot** (Conan/conanfile.txt + GitHub-Actions), optional CodeQL-Security-Scan.
- [ ] Release-Artifakte: gebaute Binaries + `swagger.yaml` + Beispiel-Konfig an Tag hängen.

### Hygiene & Tooling

- [ ] **`.gitignore`/`git ls-files` auditieren** – Muster `*.sh`/`*.txt` global ignoriert, obwohl `examples/test.sh`, `generate_certs.sh` getrackt sind; bewusst klären (entignoren oder entfernen).
- [ ] **`bin/`-Alt-Artefakte** (27 alte Test-Binaries) löschen.
- [ ] **Commit-History-Bereinigung** des Passworts (siehe Phase 0) – ein finaler `git filter-repo`-Lauf vor dem ersten öffentlichen Push.
- [ ] Branches aufräumen (`feature/dev`, `feature/cpp23-bump-and-modules`); als Git-Tags archivieren falls relevant.

---

## Meilenstein-Plan (Vorschlag)

| Meilenstein | Inhalt | Definition of Done |
|-------------|--------|---------------------|
| **v0.1.0-Blocker** | Phase 0 vollständig: Secrets weg, LICENSE/NOTICE, CI läuft, Repo public, Versionierung steht, `config.h`-Makros funktionieren, PG-Tests minimal, Swagger synchron | CI grün auf `master`, `find_package`-Install-Paket relozierbar, Repo sichtbar |
| **v0.1.0** | Erster Tag `v0.1.0` + GitHub Release + CHANGELOG | Tag mit Assets, `conan create` funktioniert |
| **v0.2.0** | Namespace-Umzug `base_library::` + Repo-Rename abgeschlossen (Abschnitt 2), Breaking-Change dokumentiert | Alle Tests grün nach Umzug, Migrationstabelle im CHANGELOG |
| **v0.3.0** | Kompilierzeit-P0/P1 umgesetzt (20./21.10.2026, Abschnitt 4) inkl. `LoggerService.h`-Refactor; offen: Boost-Install schrumpfen, ccache-Hit-Rate über mehrere CI-Läufe messen. Regressions-Tests, CI-Lint-Jobs | Messprotokoll im Repo (✓ `docs/compile-time-measurements.md`), `tests/unity_0` aufgeteilt (✓), `LoggerService.h` ohne spdlog (✓, −18,6 % Voll-Build), ccache-Hits > 70 % |
| **v1.0.0** | API-stabil erklären (`base_library::` fix), Doxygen-docs + Pages, Plattform-Matrix grün | Semantic-Versioning ab jetzt verbindlich |

---

## Quellen

- `TODO.md` (Stand 30.08.2026), `ANALYSIS.md` (Stand 29.08.2026)
- Messungen Kompilierzeit (Unity-TU-Größen via `-E`, Kompilierzeiten aus `compile_commands.json`)
- Inventur: `src/CMakeLists.txt`, `external/Hypodermic/CMakeLists.txt`, `tests/CMakeLists.txt`, `.github/workflows/ci.yml`, `conanfile.txt`, `Dockerfile`, `docker-compose.yml`, `cfg/`, `.gitignore`, `git log`
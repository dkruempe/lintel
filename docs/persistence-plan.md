# PERSISTENZ-PLAN – `lintel`

> **Stand:** 17.10.2026 · **Scope:** alle offenen Persistenz-Themen aus `ROADMAP.md`
> (A5-Teil, P2-Teil, P5) **plus** drei im Code-Review gefundene Lücken, die dort nicht
> stehen (SQLite-PRAGMAs, Connection-pro-Aufruf, N+1).
> Verwandt: `docs/architecture.dox` (DB-Sektion), `ROADMAP.md` §1 A5, §2 P2/P5.

## 0. Ausgangslage (belegt im Code)

| Befund | Beleg |
|---|---|
| Connection pro Repository-Aufruf, kein Pool, kein Reuse | 39 Stellen `db::Connection connection(m_connectionEntry);` (alle Repositories + Bootstrap-Plugins) |
| SQLite ohne jegliche PRAGMA | `sqlite3::Connection` macht nur `sqlite3_open` + REGEXP-Registrierung (`src/core/persistence/sqlite3/Connection.cpp`) – kein `busy_timeout`, kein WAL, kein `foreign_keys` |
| N+1 real | `UserRepository::allOf()` ruft pro Gruppenname einzeln `groupRepository->of()` auf (`UserRepository.cpp:34-41`) |
| Cursor-Batch hardcoded | `kDefaultFetchSize = 100` (`Cursor.h`), pro Call setzbar, aber nicht konfigurierbar |
| Kein Reconnect/Health | `PQstatus` wird nur beim Öffnen geprüft, nicht bei Rückgabe |
| PG-Tests punktuell | 10 `[pg]`-Tests (`PostgreSqlIntegrationTest.cpp`), keine Last/Concurrency |
| Kein PG-Benchmark | `benchmarks/` ohne PostgreSQL; Cursor-Batches/Notify ungemessen (P2-Offenpost) |
| Schema-Migration ohne Realbeispiel | Mechanismus + Tests da, aber keine echte `*_version_2.sql` als Referenz |

**Grundprinzip Pool:** TCP-Verbindung (Connect + Auth + ggf. TLS-Handshake) wird pro
Prozess einmal pro Slot aufgebaut statt pro Anfrage; ein Maximum begrenzt die
Backend-Connections gegen PostgreSQL `max_connections`.

## 1. Ziele / Nicht-Ziele

**Ziele:**
1. Connections nur einmal pro Pool-Slot aufbauen, hartes Maximum konfigurierbar.
2. SQLite robust und schnell: WAL, `busy_timeout`, `foreign_keys`.
3. PostgreSQL-Abdeckung: Transaktionen unter Last, echte Schema-Migration Version 2.
4. Messbarkeit: PG-Benchmarks (Cursor-Batches, Notify, Pool-Acquire) + Baseline.
5. N+1-Queries in Repositories eliminieren.

**Nicht-Ziele (bewusst):**
- Kein Ersatz für externe Pooler (PgBouncer/pgcat) – bleibt dokumentierte Ergänzung.
- Kein Idle-Reaper, kein Async-Acquire, kein Pool-Resize – der Pool bleibt schlank.
- Kein MySQL, kein Reconnect-Automatikpfad außerhalb des Pool-Health-Checks.

## 2. Phasen

### Phase 1 – SQLite-PRAGMAs 🟡 (klein, eigenständig, sofort wirksam)

- `src/core/persistence/sqlite3/Connection.cpp`: nach `sqlite3_open` setzen
  - `PRAGMA busy_timeout` (Default 5000 ms; zweiter Writer wartet statt `SQLITE_BUSY`)
  - `PRAGMA journal_mode=WAL` (nur Datei-DB, **nicht** `:memory:` – dort beibehalten)
  - `PRAGMA foreign_keys=ON`
- Tests in `tests/core/` (SqliteConnection): WAL aktiv, busy_timeout greift (zwei
  Connections, Writer hält Sperre), FK-Constraint feuert.
- Doku-Hinweis in `architecture.dox`: WAL auf Netzwerk-FS ungeeignet.
- Eigenständiger PR, kein Abhängigkeiten auf andere Phasen.

### Phase 2 – ConnectionPool Core

- Neue Dateien: `src/include/lintel/core/persistence/ConnectionPool.h`,
  `src/core/persistence/ConnectionPool.cpp` (Einträge in `src/CMakeLists.txt`).
- API:
  - `PoolSettings { std::size_t maxConnections; std::chrono::milliseconds acquireTimeout; }`
  - `Lease acquire()` – `Lease` ist RAII, expose `db::Connection &`, gibt bei
    Destruktion an den Pool zurück (auch bei Exception – kein Leak).
  - Thread-sicher: mutex + condition_variable. Wartezeitüberschreitung →
    `db::PoolTimeoutException` (neu, Kind von `db::SQLException`).
  - **Health-Check bei Rückgabe**: `PQstatus` bzw. `sqlite3_errcode`/Fehlzustand;
    kaputte Connection wird verworfen und beim nächsten Acquire neu aufgebaut.
  - `ConnectionPoolRegistry` (Name → Pool) als DI-Instance.
- **Semantik `poolSize`** (eine Implementierung, drei Betriebsarten):

  | `poolSize` | Verhalten |
  |---|---|
  | `0` | Verhalten wie heute: eine Connection pro Aufruf |
  | `1` | Eine persistente Shared-Connection, serialisiert – **richtig für SQLite** |
  | `N>1` | Bounded Pool – **richtig für PostgreSQL** (parallele HTTP-Threads) |

- Tests `tests/core/ConnectionPoolTest.cpp` (Eintrag in `tests/CMakeLists.txt`):
  Reuse (kein zweiter Connect), Bound + Timeout, Rückgabe bei Exception,
  kaputte Connection wird verworfen, Thread-Last (läuft im TSan-Job),
  `poolSize=0` = Altverhalten.

### Phase 3 – Konfiguration

- `DatabaseConnectionEntry` (`src/include/lintel/core/configuration/`): neue Felder
  `poolSize` (Defaults: PostgreSQL 4, SQLite 1; Attribut überschreibt,
  `poolSize="0"` deaktiviert) und `poolAcquireTimeoutMs` (Default 5000).
- XML-Attribute parsen (ConfigComponents) + Tests in
  `tests/features/ConfigComponentsTest.cpp`.
- `cfg/bootstrap.xml`, `cfg/bootstrap_worker.xml`, `cfg/bootstrap_production.xml`
  ergänzen (Prod explizit gesetzt, auch über `${VAR}`).
- Warnung bei `poolSize > 1` für SQLite-Verbindungen (File-Lock-Contention).
- Neuer Abschnitt *Connection pooling* in `docs/architecture.dox`: Semantik-Tabelle,
  Session-State-Regel (keine Temp-Tables/`SET` auf gepoolten Connections – Reuse),
  SQLite-Sonderfall, PgBouncer-Hinweis als optionale Ergänzung.

### Phase 4 – DI-Wiring + Readiness

- `DatabaseBootstrapPlugin` erstellt beim Start pro Connection-Eintrag einen Pool
  und registriert ihn in `ConnectionPoolRegistry` + DI. Passt in das
  10-Phasen-Bootstrap: Pool ist vor allen Repositories available.
- Readiness-Check `database:<name>` läuft künftig über `acquire()` + `SELECT 1` und
  prüft damit die Pool-Gesundheit inklusive Neueinrichtung kaputter Connections.
- Integrations-Test: Start mit `poolSize=1`, zwei Threads lesen gleichzeitig.

### Phase 5 – Migration der Repositories (größter Diff)

- Alle 39 Stellen von `db::Connection connection(m_connectionEntry);` auf
  `auto lease = m_pool->acquire(); db::Connection &connection = lease.connection();`
- Repositories holen den Pool über `ConnectionPoolRegistry` + den bereits
  vorhandenen Entry-Namen (kein zweiter DI-Parameter pro Repo).
- Reihenfolge (jeder Schritt bau-/testgrün): MessageQueueRepository →
  HistoryRepository → GroupRepository → UserRepository →
  DatabasePropertyRepository → Bootstrap-Plugins.
- **Lease-Regeln** (in `architecture.dox` dokumentiert):
  - Transaktionen: eine Lease über den gesamten Lebenszyklus
    (acquire vor `begin`, Rückgabe nach Commit/Rollback). `UserRepository::deleteOf`
    hält eine Lease über die ganze Transaktion.
  - Cursor: `db::Cursor` referenziert `db::Connection &` – Lease muss die
    Cursor-Lebensdauer überdauern; Regressionstest dafür.
  - Schema-Migrationen: eine Lease über den gesamten `*_version_N.sql`-Lauf.
- Verifikation: Grep-Gate – kein `db::Connection connection(` mehr in
  `src/features/**/repositories/` und `src/app/plugins/`.
- **Breaking** (Repository-Konsistoren ändern sich) → CHANGELOG-Eintrag mit
  Migrationshinweis (0.x, kein SemVer-Schutz).

### Phase 6 – N+1-Queries eliminieren 🟡

- `UserRepository::allOf()`: `groupRepository->of(groupName)` pro Gruppenname
  ersetzen durch einen Join (User × UserGroup × Group) **oder**
  `WHERE name IN (...)`-Batch + Map-Lookup. Gleiche Lease, eine Transaktion.
- Pinning-Test: Query-Anzahl pro Aufruf konstant (Zähler im Test-Treiber oder
  Benchmark-Vergleich Alt/Neu).
- Sichtbarkeit: Bestands-Aufruf in `SqliteBenchmark`/`PostgresBenchmark` (Phase 8)
  mit vorher/nachher-Zahlen.
- Anschluss-Check: `pageOf`, `HistoryRepository`, `GroupRepository` auf weitere
  Schleifen-Queries prüfen; nur belegen, was gefunden wird (kein Mutmaßen).

### Phase 7 – PostgreSQL-Abdeckung (A5 schließen) 🟠

- Neue `[pg]`-Tests in `tests/integration/PostgreSqlIntegrationTest.cpp`:
  - Transaktionen unter Last: mehrere parallele Connections, Writer/Reader-Mix,
    Rollback-Race, dass kein Teil-Commit sichtbar wird.
  - Lock-Wartezeit/`statement_timeout`-Verhalten (bricht sauber ab,
    `SQLException` mit Treibertext).
  - Notify-Latenz unter Last (laufende Writes + lauschende Connection).
- Schema-Migration mit echter Version 2: `*_version_2.sql` als Referenz-Seed +
  Upgrade-Test einer Bestands-DB in `DatabaseBootstrapPluginTest` (Von→Nach,
  Ketten-Test über mehrere Versionen).
- CI: `[pg]`-Tests laufen bereits mit PG-Service (coverage-Job) – prüfen, ob der
  unit-Job sie ebenfalls abdeckt, sonst im bestehenden PG-Service-Job bündeln.
- **Akzeptanz:** ROADMAP A5 auf ✅, `ROADMAP.md` aktualisiert.

### Phase 8 – Benchmarks & Messbarkeit (P2/P5 schließen)

- CI-Benchmark-Job: PG-Service ergänzen (`docker compose`, wie coverage-Job);
  skippt sauber ohne Server (Vorbild: TLS-Fälle in `HttpBenchmark`).
- Neue `benchmarks/PostgresBenchmark.cpp`:
  - Cursor-Batch-Größen 1/10/100/1000 (Real-Time pro 1k Rows),
  - Notify-Latenz, Transaktions-Batches, prepared-statement Wiederverwendung,
  - Pool-Acquire-Last (N Threads, `poolSize` 1/4/16).
- `SqliteBenchmark` ergänzen: Connection-Open vs. Acquire (Quantifizierung des
  Pool-Gewinns), WAL-Effekt Reader/Writer, `UserRepository::allOf` vorher/nachher.
- Ergebnis entscheidet `poolSize`-Defaults und ob Cursor-Batch konfigurierbar
  wird (`DatabaseConnectionEntry`/Repo-Parameter) statt `kDefaultFetchSize=100`.
- Baseline in `benchmarks/baseline.json` pflegen (`scripts/check-benchmarks.sh`).
- **Akzeptanz:** ROADMAP P2 (PG-Offenpost) ✅, P5 ✅.

## 3. Reihenfolge & Verzahnung

```
Phase 1 → 2 → 3 → 4 → 5 → 6 → 8
                      Phase 7 ──┘ (parallel zu 5/6 möglich: nur Tests/CI, kein Code-Umbau)
```

Phase 1 ist bewusst zuerst: kleinster Aufwand, sofort wirksam, unabhängig.
Phasen 7 und 8 berühren den Pool-Code nicht und können parallel laufen.

## 4. Risiken

| Risiko | Eliminierung per Design | Rest (nichteliminierbar) |
|---|---|---|
| Session-State leakt über Connection-Reuse | Unbenannte Statements (libpq `name=""`) – kein Prepared-Stmt überlebt Release; CI-Grep-Gate wie `check-namespace.sh`: kein `SET `/`CREATE TEMP`/`ALTER SESSION` in Repositories/Plugins; Regressionstest: Lease A Temp-Tabelle → Release → Lease B prüft Abwesenheit; optional `resetOnRelease` (`DISCARD ALL`), default aus | User-Code außerhalb unserer Repositories – nur Doku + Option |
| Lease hängt, Pool erschöpft (Deadlock) | `Transaction`/`Cursor` nehmen den **Lease als Besitz**, nicht `Connection&` – kein Zugriff ohne Lease mehr kompilierbar; jeder Lease zeichnet `std::source_location` + Thread-ID + Zeitstempel auf, `PoolTimeoutException` listet alle offenen Leases; Re-Entrancy-Detektor (wartender Thread == Halter → sofort Exception, nicht erst nach Timeout); Debug-Watchdog Lease-Alter; outstanding-Count im Readiness-Check | – |
| SQLite `poolSize > 1` → WAL-Lock-Contention | Harte Startup-Validierung: `ConfigurationException` bei `poolSize>1` für SQLite-Einträge, nur mit explizitem Override-Attribut | – |
| WAL auf Netzwerk-FS | Startup-Probe `statfs` (Linux `NFS_SUPER_MAGIC`, macOS `f_fstypename`) → laute Warnung + `walMode="false"`-Hinweis; `PRAGMA quick_check` beim DB-Bootstrap (konfigurierbar, default an) fängt Korruption laut statt still | Exotische FUSE-/Netzwerk-FS ohne Detektion – quick_check als Netzfänger |
| PG-Benchmark-Job instabil (Runner-Last) | Geometrische-Mittel-Normierung + Real-Time + 3 Wiederholungen; **explizites Promotions-Kriterium**: blockierend erst nach 10 grünen advisory-Läufen; Einzelläufe im Artefakt (Median später nachrüstbar) | Runner-Geschwindigkeit selbst |
| Breaking-Überraschungen bei Repo-Konsistoren | Deprecation-Pfad statt Schnitt: Overloads `(entry, registry)` neu, Alt-Konstruktor bis 1.0 mit `[[deprecated]]` und implizitem Mini-Pool (`poolSize=1`) – Verhalten ändert sich nie still; In-Repo-Caller im selben PR; Migrations-Tabelle wie bei A1 | – |

## 5. Abnahme-Kriterien (alle Phasen zusammen)

- Gesamte Suite grün (Unity **und** Non-Unity), TSan-Job (blocking) grün – der
  Pool läuft damit automatisch im Sanitizer.
- Grep-Gate: kein `db::Connection connection(` in Repositories/Plugins mehr.
- Format-, Namespace-, Swagger-Gates grün; Doxygen ohne neue Warnings.
- Benchmarks in Baseline, PG-Artefakte im CI-Job.
- `ROADMAP.md` aktualisiert: A5 ✅ (PG-Abdeckung + Migration v2), P2 ✅
  (PG-Benchmarks), P5 ✅ (Pooling, Cursor-Batch, N+1); neue Erkenntnisse
  (SQLite-PRAGMAs) im CHANGELOG-Block `[Unreleased]`.

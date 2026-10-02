# Kompilierzeit-Messprotokoll (Referenz)

> Referenzprotokoll zu ROADMAP.md Abschnitt 4 (P2: „Mess-Skripte versionieren").
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

## ccache-Hinweise

- Der lokale Cache ist klein (Cache-Größe 0,06–0,45 % von 5 GB): die Unity-TUs
  sind so groß, dass ein Cache-Eintrag den Cache kaum füllt – die Trefferquote ist
  lokal aber **100 %**, sobald nur dieselben TUs erneut gebaut werden.
- Für den ersten CI-Lauf eines Commits ist die Quote naturgemäß niedrig
  (0 Treffer). Ein belastbarer Wert (> 70 %) braucht mindestens zwei
  aufeinanderfolgende Läuufe mit unterschiedlichen, aber ähnlich großen
  Änderungen – der Cache-Key ist bereits auf `hashFiles(...)` (Inhalt) statt
  `github.sha` umgestellt, damit Zweitläufe denselben Key treffen.
- `CCACHE_BASEDIR` zeigt auf `${{ github.workspace }}` (GH-Runner) bzw.
  `/workspace` (Docker), `CCACHE_COMPRESS=1`, `CCACHE_MAXSIZE=500M`.
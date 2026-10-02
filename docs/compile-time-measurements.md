# Kompilierzeit-Messprotokoll (Referenz)

> Referenzprotokoll zu ROADMAP.md Abschnitt 4 (P2: „Mess-Skripte versionieren").
> Ziel: **reproduzierbare**, über Commits hinweg vergleichbare Zahlen für die
> Kompilierzeit von `cpp-base-library`.

## Umgebung & Grundregeln

Diese Maschine ist eine **Produktionsmaschine mit laufenden Fremdprozessen**
(z. B. `evcc`). Deshalb gelten für alle Messungen:

- **Single-Core-Messungen bevorzugt**: `--jobs 1` (Standard), optional `--nice`.
- Keine Vollauslastung: Builds laufen nur mit 1 Core; Lastdurchschnitt vor/nach
  wird mitprotokolliert, damit Ausreißer durch Fremdlast erkennbar sind.
- Es wird **nie** der Quellbaum angefasst; nur `build/` wird erzeugt.
- Alle Skripte: `scripts/measure-compile-times.sh`, `scripts/measure-header-weight.sh`.

## Messwerkzeuge

### Voll-Build (configure + Build + ccache-Statistik)

```bash
# Cold (ccache geleert), Single-Core, niedrige Prioritaet
scripts/measure-compile-times.sh --jobs 1 --nice --cold

# Incremental (eine Header-Datei touch + Rebuild)
scripts/measure-compile-times.sh --jobs 1 --nice --incremental
```

Ausgabe: Wall-Clock des Builds, Host-Load vor/nach, `ccache -s`.

### Header-Gewicht (Single-TU-Probe)

Preprocessed-Größe und `-fsyntax-only`-Zeit einer typischen `LOG_*`-Consumer-TU:

```bash
scripts/measure-header-weight.sh --build-dir build/build/Release --header LoggerService.h
```

Flags (`-I`/`-D`/`-std`) werden aus `compile_commands.json` extrahiert (TU
`LoggerService.cpp`), dadurch identisch zur realen Library-Umgebung.

## Referenz-Messablauf (vor/nach)

1. Vorab: eincommitierter Stand messen (Worker `git worktree add /tmp/baseline HEAD`).
2. Feature umsetzen, committen.
3. Beide Stände identisch vermessen (Skripte oben, Single-Core).
4. Zahlen in die Datentabellen unten eintragen, Message im Commit verweisen lassen.

---

## Datensatz: `LoggerService.h`-Refactor (fmt/spdlog in `LoggerService.cpp`)

Ziel der Änderung: `LoggerService.h` auf Forward-Declarations + `<fmt/base.h>`
umstellen, Template-Eliminierung der `log()`-API nach fmt-Vorbild
(`format` → `vformat`-Muster). spdlog taucht nur noch in `LoggerService.cpp` auf.

### Header-Gewicht (Consumer-TU `LOG_INFO/LOG_ERROR`, Single-Core, GCC -fsyntax-only)

| Metrik                           | vorher (HEAD ee55c8d) | nachher      | Δ          |
|----------------------------------|-----------------------|--------------|------------|
| Preprocessed Zeilen              | 104.396               | 84.454       | **−19,1 %** |
| Preprocessed Bytes               | 2.788.135             | 2.262.884    | **−18,8 %** |
| `-fsyntax-only` Wall-Zeit (TOTAL)| 3,24 s                | 2,09 s       | **−35,5 %** |

Vergleichswert: nur `spdlog/logger.h` (ohne Projekt-Header) = 97.458 Zeilen /
2.602.430 Bytes — was verdeutlicht, dass der spdlog-Pfad das früher dominierende
Gewicht war.

### Voll-Build (Script `measure-compile-times.sh`, Single-Core)

| Lauf        | Modus       | Wall-Clock | Host-Load vor/nach | ccache Hits |
|-------------|-------------|-----------|--------------------|-------------|
| nachher     | cold        | _tbd_      | _tbd_ / _tbd_      | _tbd_       |
| nachher     | increment.  | _tbd_      | _tbd_ / _tbd_      | _tbd_       |

(Historischer Referenz-Vorher-Build gemessen 02.09.2026 mit vollem Parallelismus:
`tests/unity_0` 470 k Zeilen / 71,8 s; Gesamt ca. 3,86 Mio. Zeilen. Für ein echtes
Vorher/Nachher auf gleicher Basis wird empfohlen, nächste Änderungen denselben
Single-Core-Ablauf laufen zu lassen.)

## ccache-Hinweise

- Hit-Rate ist erst nach mehreren CI-Läufen aussagekräftig (warm). Lokal:
  Unity-TUs + Erste-Kompilierung der Non-Unity-Verifikation senken die Quote.
- Messung in den Protokoll-Läufen oben via `ccache -s` (Daten oben eintragen).
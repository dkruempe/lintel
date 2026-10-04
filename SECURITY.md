# Sicherheitsrichtlinie (Security Policy)

## Meldeweg

Sicherheitslücken in `cpp-base-library` sollten **nicht** über öffentliche Issues gemeldet werden.

Melde sicherheitsrelevante Probleme bitte ausschließlich per E-Mail an:
- example@example.com

Bitte gib in deiner E-Mail folgende Informationen an (so vollständig wie möglich):
- Beschreibung des Problems und potenzieller Auswirkung
- Schritte zur Reproduktion (sofern nachvollziehbar)
- Betroffene Version/Commit (z. B. Commit-Hash)
- Mögliche Abhilfen (optional)

## Reaktionsfristen

Wir bemühen uns um eine zeitnahe Prüfung gemeldeter Sicherheitsprobleme. Als realistische Richtwerte gelten:
- Erstbestätigung (Receipt): innerhalb von **7 Werktagen**
- Erste Bewertung (Impact/Validity): innerhalb von **14 Werktagen**
- Festlegung/Nachverfolgung eines Fix-Plans: nach Bewertung, zeitnah entsprechend Kritikalität

Diese Fristen sind eine realistische Zielvorgabe für dieses Projekt (pre-1.0); bei hoher Kritikalität wird versucht, schneller zu reagieren.

## Unterstützte Versionen

Derzeit wird **nur** der aktuelle Entwicklungsstand auf `master` unterstützt. Es gibt kein stabiles Release mit langfristigem Support jenseits `v0.1.0` (pre-1.0-Status). Sicherheitsfixes werden bevorzugt auf `master` bereitgestellt.

| Version | Unterstützt |
|---------|------------|
| `master` | Ja |
| `v0.1.0` | Eingeschränkt (nur schwerwiegende, nachweisbare Lücken bei begrenzter Kapazität) |
| Ältere Tags | Nein |

## Sicherheitsbestimmungen für Beiträge

- **Keine Secrets commiten:** Niemals private Schlüssel, Zertifikate mit privatem Schlüssel, Passwörter, Tokens, API-Keys oder sonstige sensible Daten ins Repository committen.
- **TLS/Keys:** Private Schlüssel (`*.key`) werden über `.gitignore` ausgeschlossen. Dev-Zertifikate werden lokal generiert (siehe `cfg/certs/README.md`). Produktiv-Keys niemals im Repo ablegen.
- **Passwörter/Klartext-Credentials:** Keine Klartext-Credentials in Code, Beispielen oder Konfiguration committen. Beispiele nutzen Platzhalter (z. B. `${ADMIN_PASSWORD}`).
- **Abhängigkeiten:** Neue Abhängigkeiten sorgfältig prüfen. CI verwendet Conan 2 und baut reproduzierbar.
- **Tests:** Sicherheitsrelevante Änderungen sollten nach Möglichkeit durch Tests abgedeckt werden, ohne dabei echte Secrets zu verwenden.
- **Veröffentlichung:** Fixes für bestätigte Lücken werden in Commits auf `master` dokumentiert (kurz, faktenbasiert). Releases erfolgen über den Release-Workflow bei Tags (`v*`).

## Hinweis

Dieses Projekt ist in der Pre-1.0-Phase. Die Sicherheitsrichtlinie kann sich mit zunehmender Reife weiterentwickeln. Für Fragen zu sicherheitsrelevanten Aspekten nutze bitte den oben genannten E-Mail-Meldeweg.

# TLS-Zertifikate

Die Zertifikate in diesem Verzeichnis sind **lokal generierte Wegwerf-Zertifikate**
und stehen absichtlich nicht im Repository – der private Schlüssel darf nicht
versioniert werden (`.gitignore`: `*.key`, `*.crt`).

## Erzeugen

```bash
./cfg/certs/generate_certs.sh
```

Erzeugt `server.crt` und `server.key` (self-signed, `CN=localhost`,
`subjectAltName=DNS:localhost,IP:127.0.0.1`, 10 Jahre). Das Skript bricht ab,
wenn die Dateien schon existieren; zum Neuerzeugen zuerst beide löschen.

## Wofür

`cfg/bootstrap.xml` und `cfg/database`/`tests` verweisen relativ auf
`certs/server.crt` und `certs/server.key`:

- `tests/integration/HttpIntegrationTest.cpp` („Client: uses SSL when TLS is
  configured") **prüft beide Dateien mit `REQUIRE`** und startet einen
  `httplib::SSLServer` damit. Ohne Zertifikate schlägt der Test fehl.
- Deshalb erzeugen die CI-Workflows (`ci.yml`, `release.yml`) ihre eigenen
  Wegwerf-Zertifikate, bevor sie bauen.

## Produktion

Für Produktion **keine** selbstsignierten Zertifikate verwenden, sondern echte
Zertifikate ausstellen lassen und `cert_path`/`key_path` in der Konfiguration
darauf zeigen lassen.
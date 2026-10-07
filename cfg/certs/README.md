# TLS certificates

The certificates in this directory are **locally generated throwaway certificates**
and are intentionally not in the repository – the private key must not be
versioned (`.gitignore`: `*.key`, `*.crt`).

## Generating

```bash
./cfg/certs/generate_certs.sh
```

Creates `server.crt` and `server.key` (self-signed, `CN=localhost`,
`subjectAltName=DNS:localhost,IP:127.0.0.1`, 10 years). The script aborts
if the files already exist; to regenerate, delete both first.

## What they are for

`cfg/bootstrap.xml` and `cfg/database`/`tests` reference
`certs/server.crt` and `certs/server.key` relatively:

- `tests/integration/HttpIntegrationTest.cpp` ("Client: uses SSL when TLS is
  configured") **checks both files with `REQUIRE`** and starts an
  `httplib::SSLServer` with them. Without certificates the test fails.
- Therefore the CI workflows (`ci.yml`, `release.yml`) generate their own
  throwaway certificates before they build.

## Production

For production do **not** use self-signed certificates, but have real
certificates issued and point `cert_path`/`key_path` in the configuration
at them.

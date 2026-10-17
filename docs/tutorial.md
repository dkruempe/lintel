# Tutorial: the HTTP API in 15 minutes

This walkthrough starts the reference application, talks to the HTTP API over TLS,
authenticates, makes an authenticated call, renews the certificate without a restart
and points out what has to change for a production deployment. It complements the
[Architecture](https://dkruempe.github.io/lintel/architecture.html) page (why the
pieces look the way they do) and `swagger.yaml` (the wire contract); this page is the
"make it work" path.

Everything below uses the repository's `bin/main`, which wires
`StartupBuilder` with `PropertyFeature`, `BaseFeature`, `HttpFeature` and
`CommandLineFeature` and reads `cfg/bootstrap.xml`.

## What you need

- A built checkout (`bin/main` present). See
  [Building the Project](../README.md#building-the-project).
- The development certificate pair, created once:

  ```bash
  ./cfg/certs/generate_certs.sh
  ```

  `cfg/bootstrap.xml` sets `require_tls="true"`, so the server refuses to start
  without the pair.
- An admin password. `main` creates the `ADMIN_USERNAME` user on first start when
  `ADMIN_PASSWORD` is set; both come from `.env` (copy `.env.example`). See
  [Running the examples locally](../README.md#running-the-examples-locally).

```bash
cp .env.example .env      # set ADMIN_PASSWORD (and CONFIG_DIRECTORY)
```

## Step 1 — start the server

```bash
scripts/run-example.sh          # loads .env, generates certs if needed, runs bin/main
```

`main` also starts the interactive CLI and the worker processes declared in
`cfg/bootstrap.xml`, so run it from a terminal. The API listens on
`https://localhost:8080`. All examples below use `curl -k`, because the development
certificate is self-signed.

## Step 2 — probes (no authentication)

`/health` is a liveness probe: it answers `200` as long as the process is alive and
never touches a dependency, so a broken database cannot make a supervisor restart a
healthy process.

```bash
curl -fsSk https://localhost:8080/health
# {"status":"ok"}
```

`/ready` is the readiness probe. It runs the checks registered at startup — a
`SELECT 1` ping per configured database connection (`database:<name>`) and
`shared_memory` for the mapped segments — and answers `503` while one of them fails:

```bash
curl -fsSk https://localhost:8080/ready
# {"status":"ready","checks":{"database:DEFAULT_SQLITE":"ok","shared_memory":"ok"}}

curl -sSk -o /dev/null -w '%{http_code}\n' https://localhost:8080/ready   # 503 while not ready
```

Use `/health` for the liveness probe and `/ready` for the readiness probe of an
orchestrator; mixing them up is the classic way to build a restart loop.

## Step 3 — log in and call an authenticated endpoint

`POST /user/login` authenticates with HTTP Basic and returns the user as a `UserDto`
whose **`id` field is the bearer token**:

```bash
TOKEN=$(curl -fsSk -X POST -u "$ADMIN_USERNAME:$ADMIN_PASSWORD" \
          https://localhost:8080/user/login | python3 -c 'import sys,json; print(json.load(sys.stdin)["id"])')

curl -fsSk -H "Authorization: Bearer $TOKEN" https://localhost:8080/user/state
# 200 with an empty body while the token is valid; 401 once it is not
```

The token is what `Authorization: Bearer <token>` expects on every protected
endpoint. A few things are worth knowing:

- Invalid credentials, a malformed Basic header, or a login while the account already
  holds a valid token all answer `403` (not `401`); see the `POST /user/login`
  responses in `swagger.yaml`.
- The token is bound to the address the client connected from. A request with the same
  token from a different address is rejected — which is why
  `trusted_proxies` in the bootstrap configuration has to name exactly the proxies in
  front of the process (otherwise every client looks like the same address, or a
  client can pick its own address through `X-Forwarded-For`).
- Failed logins are rate-limited per address and locked out after too many failures;
  the details are in the [Architecture](https://dkruempe.github.io/lintel/architecture.html)
  page (Authentication and sessions).

Log out to release the session:

```bash
curl -fsSk -X DELETE -H "Authorization: Bearer $TOKEN" https://localhost:8080/user/logout
```

## Step 4 — one business call

`GET /user/users` lists users; with `limit` (and `after` for the following pages) the
answer is a keyset-paginated envelope:

```bash
curl -fsSk -H "Authorization: Bearer $TOKEN" 'https://localhost:8080/user/users?limit=2'
# {"items":[...],"has_more":true,"next_after":"<user_name>"}
```

The same pattern applies to the other groups of endpoints (`swagger.yaml` is the
complete list): `/properties/...` reads and writes configuration values,
`/process/...` inspects and controls the managed processes, `/shm/...` reads shared
memory and `/history/...` returns the history a worker produced.

(One entry in `swagger.yaml` is not served by `bin/main`: `/hello` belongs to
`lintel::ExampleController`, which an application registers itself — it is a minimal
`Controller` example, not part of `HttpFeature`.)

## Step 5 — renew the certificate without a restart

The certificate pair is read once at startup, so replacing the files on disk does
nothing for a running process. Reload it in place:

```bash
# replace cfg/certs/server.crt and server.key first, then:
curl -fsSk -X POST -H "Authorization: Bearer $TOKEN" https://localhost:8080/admin/reload-tls
# {"status":"reloaded","results":{"tls":"ok"}}
```

`409` means the process serves clear text (nothing to reload), `500` with a reason
means the pair could not be read or OpenSSL rejected it — in that case the previous,
working pair stays in effect. The endpoint needs the admin group, like the other
`/admin/...` routes.

## Step 6 — what changes for production

- Use `cfg/bootstrap_production.xml` as the starting point instead of `cfg/bootstrap.xml`.
  It forces TLS, reads the PostgreSQL credentials and the log/shared-memory
  directories from the environment (`${VAR}` placeholders) and ships no fallback
  credentials. Never commit real credentials; `scripts/check-secrets.sh` fails CI if a
  plaintext one appears in a configuration file or a PEM block.
- Terminate TLS either in the process (as `main` does) or in front of it. If a proxy
  terminates it, keep `require_tls="true"` only for direct TLS and set
  `trusted_proxies` to the proxy addresses; a range that is too wide turns the
  per-address brute-force protection into no protection.
- Wire `/health` to the liveness probe and `/ready` to the readiness probe, and make
  sure the dependencies behind `/ready` are the ones that actually have to be up.
- Keep the request body cap in mind: the server accepts request bodies up to 8 MiB and
  rejects larger ones instead of buffering them.

## Troubleshooting

| Symptom | Cause |
|---|---|
| Startup fails with a TLS/certificate error | `cfg/certs/server.{crt,key}` missing — run `./cfg/certs/generate_certs.sh` |
| `403` from `POST /user/login` | Wrong credentials, malformed Basic header, or the account already holds a valid token (log out first) |
| `401` on a protected endpoint | Missing, malformed or expired bearer token, or the token is used from another address |
| `503` from `/ready` | A registered dependency check failed; the body names the check and the reason |
| `409` from `POST /admin/reload-tls` | The process runs without TLS; there is nothing to reload |
| `500` from `POST /admin/reload-tls` | The new pair is unreadable or OpenSSL rejected it; the previous pair stays active |

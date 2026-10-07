# Security Policy

## Reporting path

Security vulnerabilities in `lintel` should **not** be reported via public issues.

Please report security-relevant issues exclusively by email to:
- example@example.com

Please include the following information in your email (as complete as possible):
- description of the problem and potential impact
- steps to reproduce (if reproducible)
- affected version/commit (e.g. commit hash)
- possible mitigations (optional)

## Response times

We aim for a timely review of reported security issues. The following are realistic targets:
- initial acknowledgement (receipt): within **7 business days**
- initial assessment (impact/validity): within **14 business days**
- decision/follow-up on a fix plan: after the assessment, promptly according to criticality

These times are a realistic target for this project (pre-1.0); for high criticality we try to react faster.

## Supported versions

Currently **only** the current development state on `master` is supported. There is no stable release with long-term support beyond `v0.1.0` (pre-1.0 status). Security fixes are preferably provided on `master`.

| Version | Supported |
|---------|------------|
| `master` | Yes |
| `v0.1.0` | Limited (only severe, reproducible gaps, with limited capacity) |
| Older tags | No |

## Security rules for contributions

- **Do not commit secrets:** never commit private keys, certificates with a private key, passwords, tokens, API keys or other sensitive data into the repository.
- **TLS/keys:** private keys (`*.key`) are excluded via `.gitignore`. Dev certificates are generated locally (see `cfg/certs/README.md`). Never store production keys in the repo.
- **Passwords/plaintext credentials:** do not commit plaintext credentials in code, examples or configuration. Examples use placeholders (e.g. `${ADMIN_PASSWORD}`).
- **Dependencies:** review new dependencies carefully. CI uses Conan 2 and builds reproducibly.
- **Tests:** security-relevant changes should be covered by tests where possible, without using real secrets.
- **Releases:** fixes for confirmed gaps are documented in commits on `master` (short, fact-based). Releases are done via the release workflow on tags (`v*`).

## Note

This project is in its pre-1.0 phase. The security policy may evolve further as the project matures. For questions on security-relevant aspects please use the email reporting path named above.
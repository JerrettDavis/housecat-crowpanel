# Public-release security audit

## Current publishable tree

The repository-specific audit rejects high-confidence credential formats,
private keys, and an accidentally tracked local secrets file. Source packaging
excludes credentials, build products, caches, editor state, and firmware
binaries. CI runs both this policy check and Gitleaks.

Run locally:

```bash
python tools/security_audit.py
python tools/package_source.py --output ..
python tools/security_audit.py ../housecat-crowpanel-pio-$(cat VERSION)
```

## Git history

Status for this working copy: **not verified**. The supplied directory did not
contain its original `.git` metadata, so no tool can prove what earlier commits
contained. Before making an existing remote public, restore/clone the original
repository and run:

```bash
gitleaks git --redact --verbose .
```

The Security workflow fetches complete history (`fetch-depth: 0`) and repeats
that scan on every push and pull request. Any credential previously committed
must be revoked and rotated even if history is later rewritten.

## Release checklist

- Review the clean ZIP rather than distributing a development directory.
- Verify `MANIFEST.sha256` and scan the extracted ZIP.
- Confirm `include/secrets.h` and `.pio/` are absent.
- Rotate one-use enrollment keys and use least-privilege MQTT credentials.
- Enable GitHub private vulnerability reporting and branch protection.
- Require Native core, PlatformIO firmware, and Security checks before merge.

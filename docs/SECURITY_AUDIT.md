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

Status for this repository: **verified**. The supplied working directory had no
Git metadata and no matching remote repository, so the sanitized tree was
initialized as a new `main` repository with a clean root commit. Gitleaks scans
every reachable commit, Git integrity checks pass, and local secret files are
absent from the object database. Run the same audit at any time with:

```bash
gitleaks git --redact --verbose .
```

The Security workflow fetches complete history (`fetch-depth: 0`) and repeats
that scan on every push and pull request. No earlier, external history was
available or imported; do not graft an older repository onto this clean history
without separately auditing it. Any credential previously committed elsewhere
must be revoked and rotated even if that other history is later rewritten.

## Release checklist

- Review the clean ZIP rather than distributing a development directory.
- Verify `MANIFEST.sha256` and scan the extracted ZIP.
- Confirm `include/secrets.h` and `.pio/` are absent.
- Rotate one-use enrollment keys and use least-privilege MQTT credentials.
- Enable GitHub private vulnerability reporting and branch protection.
- Require Native core, PlatformIO firmware, and Security checks before merge.

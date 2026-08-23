#!/usr/bin/env python3
"""Fail on high-confidence secrets or publishability hazards in source files."""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import subprocess
import sys

SKIP_PARTS = {".git", ".pio", ".vscode", "build", "dist", "__pycache__"}
SKIP_NAMES = {"secrets.h", "secrets.local.h"}
TEXT_SUFFIXES = {
    "", ".c", ".cc", ".cpp", ".h", ".hpp", ".ini", ".json", ".md",
    ".mjs", ".ps1", ".py", ".sh", ".toml", ".txt", ".yaml", ".yml",
}
RULES = {
    "private-key": re.compile(r"-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----"),
    "tailscale-auth-key": re.compile(r"tskey-auth-[A-Za-z0-9_-]{20,}"),
    "github-token": re.compile(r"\b(?:gh[opusr]_[A-Za-z0-9_]{30,}|github_pat_[A-Za-z0-9_]{30,})\b"),
    "aws-access-key": re.compile(r"\b(?:AKIA|ASIA)[A-Z0-9]{16}\b"),
    "slack-token": re.compile(r"\bxox[baprs]-[A-Za-z0-9-]{20,}\b"),
}


def excluded(relative: Path) -> bool:
    return relative.name in SKIP_NAMES or any(
        part in SKIP_PARTS or part.startswith("build-") for part in relative.parts
    )


def scan(root: Path) -> list[str]:
    findings: list[str] = []
    for path in sorted(root.rglob("*")):
        if not path.is_file():
            continue
        relative = path.relative_to(root)
        if excluded(relative) or path.suffix.lower() not in TEXT_SUFFIXES:
            continue
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue
        for number, line in enumerate(text.splitlines(), 1):
            for name, pattern in RULES.items():
                if pattern.search(line):
                    findings.append(f"{relative.as_posix()}:{number}: {name}")
    return findings


def check_local_secret_is_ignored(root: Path) -> list[str]:
    secret = root / "include/secrets.h"
    if not secret.exists() or not (root / ".git").exists():
        return []
    result = subprocess.run(
        ["git", "check-ignore", "--quiet", "include/secrets.h"], cwd=root
    )
    return [] if result.returncode == 0 else ["include/secrets.h: local secret file is not Git-ignored"]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path", nargs="?", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    root = args.path.resolve()
    findings = scan(root) + check_local_secret_is_ignored(root)
    if findings:
        print("Security audit FAILED (values intentionally suppressed):", file=sys.stderr)
        for finding in findings:
            print(f"- {finding}", file=sys.stderr)
        return 1
    print(f"Security audit passed: {root}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""Create a clean, checksummed House Cat source release ZIP."""

from __future__ import annotations

import argparse
import fnmatch
import hashlib
from pathlib import Path
import shutil
import sys
import zipfile

EXCLUDED_DIR_NAMES = {
    ".git",
    ".pio",
    ".vscode",
    "__pycache__",
}
EXCLUDED_FILE_NAMES = {
    ".DS_Store",
    "MANIFEST.sha256",
    "secrets.h",
    "secrets.local.h",
}
EXCLUDED_DIR_PATTERNS = (
    "build",
    "build-*",
    "dist/housecat-*",
)
EXCLUDED_FILE_PATTERNS = (
    "*.pyc",
    "*.pyo",
    "*.zip",
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def should_exclude(relative: Path, *, is_dir: bool) -> bool:
    if any(part in EXCLUDED_DIR_NAMES for part in relative.parts):
        return True
    if relative.name in EXCLUDED_FILE_NAMES:
        return True
    prefixes = [Path(*relative.parts[:index]).as_posix() for index in range(1, len(relative.parts) + 1)]
    if any(fnmatch.fnmatch(prefix, pattern) for prefix in prefixes for pattern in EXCLUDED_DIR_PATTERNS):
        return True
    if not is_dir and any(fnmatch.fnmatch(relative.name, pattern) for pattern in EXCLUDED_FILE_PATTERNS):
        return True
    return False


def copy_clean(project: Path, staging: Path) -> None:
    if staging.exists():
        shutil.rmtree(staging)
    staging.mkdir(parents=True)

    for source in sorted(project.rglob("*")):
        relative = source.relative_to(project)
        if should_exclude(relative, is_dir=source.is_dir()):
            continue
        destination = staging / relative
        if source.is_dir():
            destination.mkdir(parents=True, exist_ok=True)
        elif source.is_file():
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, destination)


def write_manifest(staging: Path) -> Path:
    files = sorted(path for path in staging.rglob("*") if path.is_file())
    lines = [f"{sha256(path)}  {path.relative_to(staging).as_posix()}" for path in files]
    manifest = staging / "MANIFEST.sha256"
    manifest.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return manifest


def verify_manifest(staging: Path, manifest: Path) -> None:
    for line in manifest.read_text(encoding="utf-8").splitlines():
        expected, relative = line.split("  ", 1)
        path = staging / relative
        if not path.is_file():
            raise FileNotFoundError(f"manifest entry is missing: {relative}")
        actual = sha256(path)
        if actual != expected:
            raise ValueError(f"manifest hash mismatch: {relative}")


def write_zip(staging: Path, archive: Path) -> None:
    archive.parent.mkdir(parents=True, exist_ok=True)
    archive.unlink(missing_ok=True)
    with zipfile.ZipFile(archive, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as bundle:
        for path in sorted(item for item in staging.rglob("*") if item.is_file()):
            arcname = (Path(staging.name) / path.relative_to(staging)).as_posix()
            bundle.write(path, arcname)


def package(project: Path, output_dir: Path) -> tuple[Path, Path]:
    project = project.resolve()
    output_dir = output_dir.resolve()
    version = (project / "VERSION").read_text(encoding="utf-8").strip()
    release_name = f"housecat-crowpanel-pio-{version}"
    staging = output_dir / release_name
    archive = output_dir / f"{release_name}.zip"

    if staging == project or project in staging.parents:
        raise ValueError("output directory must be outside the project tree")

    copy_clean(project, staging)
    manifest = write_manifest(staging)
    verify_manifest(staging, manifest)
    write_zip(staging, archive)
    return staging, archive


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--output", type=Path, default=Path(__file__).resolve().parents[2])
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        staging, archive = package(args.project, args.output)
    except (OSError, ValueError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1
    file_count = sum(1 for path in staging.rglob("*") if path.is_file())
    print(f"Packaged {file_count} files: {archive}")
    print(f"SHA-256: {sha256(archive)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

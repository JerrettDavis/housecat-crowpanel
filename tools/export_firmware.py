#!/usr/bin/env python3
"""Export PlatformIO build products into a versioned, flashable bundle.

Run after a successful PlatformIO build:
    python tools/export_firmware.py --env crowpanel_idf5

The script copies the individual ESP32-S3 images, records hashes and offsets,
and tries to create a single merged image when PlatformIO's esptool package is
available. Failure to merge does not invalidate the normal PlatformIO upload
path or the individual image bundle.
"""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
from typing import Iterable, Sequence

FLASH_LAYOUT = (
    (0x0000, "bootloader.bin"),
    (0x8000, "partitions.bin"),
    (0xE000, "boot_app0.bin"),
    (0x10000, "firmware.bin"),
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def find_boot_app0(project: Path, build_dir: Path) -> Path | None:
    candidates = [
        build_dir / "boot_app0.bin",
        project / ".pio" / "packages" / "framework-arduinoespressif32" / "tools" / "partitions" / "boot_app0.bin",
        Path.home() / ".platformio" / "packages" / "framework-arduinoespressif32" / "tools" / "partitions" / "boot_app0.bin",
    ]
    for root in (project / ".pio" / "packages", Path.home() / ".platformio" / "packages"):
        if root.exists():
            candidates.extend(root.glob("framework-arduinoespressif32*/tools/partitions/boot_app0.bin"))
    return next((candidate.resolve() for candidate in candidates if candidate.is_file()), None)


def find_esptool(project: Path) -> Sequence[str] | None:
    for executable in ("esptool", "esptool.py"):
        found = shutil.which(executable)
        if found:
            return [found]

    if importlib.util.find_spec("esptool") is not None:
        return [sys.executable, "-m", "esptool"]

    roots = [project / ".pio" / "packages", Path.home() / ".platformio" / "packages"]
    for root in roots:
        if not root.exists():
            continue
        for candidate in root.glob("tool-esptoolpy*/esptool.py"):
            if candidate.is_file():
                return [sys.executable, str(candidate.resolve())]
    return None


def copy_required(source: Path, destination: Path) -> None:
    if not source.is_file():
        raise FileNotFoundError(f"required PlatformIO artifact not found: {source}")
    shutil.copy2(source, destination)


def write_flash_helpers(output: Path, merged: bool, environment: str) -> None:
    image_args = " ".join(f"0x{offset:X} {name}" for offset, name in FLASH_LAYOUT)
    merged_note = "housecat-full.bin at offset 0x0" if merged else "the four images and offsets listed below"

    (output / "FLASHING.txt").write_text(
        "House Cat CrowPanel ESP32-S3 flash bundle\n"
        "===========================================\n\n"
        "Preferred route from the repository root:\n"
        f"  python -m platformio run -e {environment} -t upload --upload-port <PORT>\n\n"
        "Raw esptool route (install esptool in the active Python environment):\n"
        f"  python -m esptool --chip esp32s3 --port <PORT> --baud 921600 "
        f"--before default_reset --after hard_reset write_flash -z "
        f"--flash_mode qio --flash_freq 80m --flash_size 8MB {image_args}\n\n"
        f"A merged image was {'created' if merged else 'not created'}; use {merged_note}.\n"
        "Open a 115200-baud serial monitor after flashing.\n",
        encoding="utf-8",
    )

    ps_images = " ".join(f"0x{offset:X} '{name}'" for offset, name in FLASH_LAYOUT)
    (output / "flash-esptool.ps1").write_text(
        "param(\n"
        "  [Parameter(Mandatory=$true)][string]$Port,\n"
        "  [int]$Baud = 921600\n"
        ")\n"
        "$ErrorActionPreference = 'Stop'\n"
        f"py -m esptool --chip esp32s3 --port $Port --baud $Baud --before default_reset --after hard_reset "
        f"write_flash -z --flash_mode qio --flash_freq 80m --flash_size 8MB {ps_images}\n"
        "if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }\n",
        encoding="utf-8",
    )

    sh_images = " ".join(f"0x{offset:X} '{name}'" for offset, name in FLASH_LAYOUT)
    (output / "flash-esptool.sh").write_text(
        "#!/usr/bin/env bash\n"
        "set -euo pipefail\n"
        "PORT=${1:?Usage: ./flash-esptool.sh /dev/ttyUSB0 [baud]}\n"
        "BAUD=${2:-921600}\n"
        f"python3 -m esptool --chip esp32s3 --port \"$PORT\" --baud \"$BAUD\" "
        f"--before default_reset --after hard_reset write_flash -z --flash_mode qio "
        f"--flash_freq 80m --flash_size 8MB {sh_images}\n",
        encoding="utf-8",
    )
    os.chmod(output / "flash-esptool.sh", 0o755)


def merge_image(project: Path, output: Path) -> tuple[bool, str | None]:
    esptool = find_esptool(project)
    if esptool is None:
        return False, "esptool was not found; individual images remain flashable"

    merged = output / "housecat-full.bin"
    command = [
        *esptool,
        "--chip",
        "esp32s3",
        "merge_bin",
        "-o",
        str(merged),
        "--flash_mode",
        "qio",
        "--flash_freq",
        "80m",
        "--flash_size",
        "8MB",
    ]
    for offset, filename in FLASH_LAYOUT:
        command.extend((hex(offset), str(output / filename)))

    completed = subprocess.run(command, cwd=output, text=True, capture_output=True, check=False)
    if completed.returncode != 0:
        merged.unlink(missing_ok=True)
        detail = (completed.stderr or completed.stdout).strip()
        return False, f"esptool merge failed: {detail}"
    return True, None


def export(project: Path, environment: str, output_root: Path | None) -> Path:
    project = project.resolve()
    version = (project / "VERSION").read_text(encoding="utf-8").strip()
    build_dir = project / ".pio" / "build" / environment
    if not build_dir.is_dir():
        raise FileNotFoundError(
            f"PlatformIO build directory not found: {build_dir}\n"
            f"Run: python -m platformio run -e {environment}"
        )

    output = (output_root or project / "dist") / f"housecat-{version}" / environment
    if output.exists():
        shutil.rmtree(output)
    output.mkdir(parents=True)

    sources = {
        "bootloader.bin": build_dir / "bootloader.bin",
        "partitions.bin": build_dir / "partitions.bin",
        "firmware.bin": build_dir / "firmware.bin",
    }
    boot_app0 = find_boot_app0(project, build_dir)
    if boot_app0 is None:
        raise FileNotFoundError("boot_app0.bin was not found in the Arduino ESP32 framework package")
    sources["boot_app0.bin"] = boot_app0

    optional = {
        "firmware.elf": build_dir / "firmware.elf",
        "firmware.map": build_dir / "firmware.map",
    }

    for destination_name, source in sources.items():
        copy_required(source, output / destination_name)
    for destination_name, source in optional.items():
        if source.is_file():
            shutil.copy2(source, output / destination_name)

    merged, merge_warning = merge_image(project, output)
    write_flash_helpers(output, merged, environment)

    files = []
    for path in sorted(item for item in output.iterdir() if item.is_file()):
        files.append({
            "name": path.name,
            "bytes": path.stat().st_size,
            "sha256": sha256(path),
        })

    manifest = {
        "project": "House Cat",
        "version": version,
        "environment": environment,
        "target": "Elecrow CrowPanel ESP32-S3 2.13-inch E-Paper N8R8",
        "chip": "esp32s3",
        "flash": {"size": "8MB", "mode": "qio", "frequency": "80m"},
        "images": [
            {"offset": f"0x{offset:X}", "file": filename}
            for offset, filename in FLASH_LAYOUT
        ],
        "merged_image": "housecat-full.bin" if merged else None,
        "merge_warning": merge_warning,
        "files": files,
    }
    manifest_path = output / "flash-manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

    # Refresh the manifest's own hash listing in a separate simple checksum file.
    checksum_lines = [f"{sha256(path)}  {path.name}" for path in sorted(output.iterdir()) if path.is_file()]
    (output / "SHA256SUMS").write_text("\n".join(checksum_lines) + "\n", encoding="utf-8")
    return output


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--env", default="crowpanel_idf5", dest="environment")
    parser.add_argument("--output", type=Path, default=None, dest="output_root")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        output = export(args.project, args.environment, args.output_root)
    except (OSError, ValueError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1
    print(f"Exported flash bundle: {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

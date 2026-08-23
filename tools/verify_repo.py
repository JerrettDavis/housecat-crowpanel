#!/usr/bin/env python3
"""Fast structural verification for the House Cat firmware repository."""

from __future__ import annotations

import configparser
import json
from pathlib import Path
import re
import sys

EXPECTED_PINS = {
    "kEpaperBusy": 9,
    "kEpaperReset": 10,
    "kEpaperMosi": 11,
    "kEpaperClock": 12,
    "kEpaperDc": 13,
    "kEpaperCs": 14,
    "kDisplayPower": 7,
    "kPowerLed": 19,
    "kMenu": 2,
    "kBack": 1,
    "kRockerUp": 6,
    "kRockerDown": 4,
    "kRockerClick": 5,
    "kExpansionA": 40,
    "kExpansionB": 41,
}


def fail(message: str) -> None:
    raise ValueError(message)


def verify_markdown_links(root: Path) -> None:
    link_pattern = re.compile(r"\]\(([^)]+)\)")
    for document in root.rglob("*.md"):
        if any(part in {".pio", "build", "dist"} or part.startswith("build-")
               for part in document.relative_to(root).parts):
            continue
        for target in link_pattern.findall(document.read_text(encoding="utf-8")):
            target = target.strip().strip("<>").split("#", 1)[0]
            if not target or "://" in target or target.startswith(("#", "mailto:")):
                continue
            if not (document.parent / target).resolve().exists():
                fail(f"broken Markdown link in {document.relative_to(root)}: {target}")


def verify(root: Path) -> None:
    required_public_files = (
        "LICENSE", "CONTRIBUTING.md", "SECURITY.md", "CODE_OF_CONDUCT.md",
        "SUPPORT.md", ".editorconfig", ".gitattributes", ".pre-commit-config.yaml",
        ".github/dependabot.yml", ".github/workflows/security.yml",
        ".github/workflows/release.yml", "docs/GETTING_STARTED.md",
        "docs/USER_GUIDE.md", "docs/TROUBLESHOOTING.md", "docs/SECURITY_AUDIT.md",
    )
    for relative in required_public_files:
        if not (root / relative).is_file():
            fail(f"missing public-release file: {relative}")
    verify_markdown_links(root)

    version = (root / "VERSION").read_text(encoding="utf-8").strip()
    config_header = (root / "include/housecat/config.h").read_text(encoding="utf-8")
    if f'kFirmwareVersion = "{version}"' not in config_header:
        fail("VERSION does not match include/housecat/config.h")

    library = json.loads((root / "lib/housecat_core/library.json").read_text(encoding="utf-8"))
    if library.get("version") != version:
        fail("VERSION does not match lib/housecat_core/library.json")

    board = json.loads((root / "boards/crowpanel_esp32s3_213.json").read_text(encoding="utf-8"))
    if board["build"].get("mcu") != "esp32s3":
        fail("custom board MCU must be esp32s3")
    if board["build"].get("psram_type") != "opi":
        fail("custom board must declare OPI PSRAM")
    if board["upload"].get("flash_size") != "8MB":
        fail("custom board must declare 8MB flash")
    flags = set(board["build"].get("extra_flags", []))
    for required in ("-DBOARD_HAS_PSRAM", "-DARDUINO_USB_MODE=0", "-DARDUINO_USB_CDC_ON_BOOT=0"):
        if required not in flags:
            fail(f"custom board missing flag {required}")

    requirements = (root / "requirements-platformio.txt").read_text(encoding="utf-8").splitlines()
    requirements = [line.strip() for line in requirements if line.strip() and not line.lstrip().startswith("#")]
    if requirements != ["platformio==6.1.19"]:
        fail("requirements-platformio.txt must pin exactly platformio==6.1.19")

    parser = configparser.ConfigParser(interpolation=None)
    parser.read(root / "platformio.ini")
    default_envs = parser["platformio"].get("default_envs", "").strip()
    if default_envs != "crowpanel_idf5":
        fail("shipping default must be crowpanel_idf5")
    if parser["housecat"].get("platform") != "espressif32@7.0.1":
        fail("PlatformIO platform is not pinned to espressif32@7.0.1")
    if parser["housecat"].get("board") != "crowpanel_esp32s3_213":
        fail("PlatformIO does not target the custom CrowPanel board")
    dependencies = {
        line.strip()
        for line in parser["housecat"].get("lib_deps", "").splitlines()
        if line.strip()
    }
    expected_dependencies = {
        "knolleary/PubSubClient@2.8",
        "bblanchon/ArduinoJson@7.4.3",
    }
    if dependencies != expected_dependencies:
        fail(f"PlatformIO external dependencies differ from the minimal pinned set: {sorted(dependencies)}")

    if "env:crowpanel" not in parser:
        fail("PlatformIO must define the crowpanel environment")
    if "env:crowpanel_idf5" not in parser:
        fail("PlatformIO must define the shipping crowpanel_idf5 environment")
    if "env:crowpanel_safe" in parser:
        fail("crowpanel_safe is obsolete because the qualified driver always uses full waveforms")

    input_sources = "\n".join(
        (root / relative).read_text(encoding="utf-8")
        for relative in ("src/main.cpp", "src/board/crowpanel_input.h", "src/board/crowpanel_input.cpp")
    )
    if "discardPending" in input_sources:
        fail("buffered controls must not discard actions captured during display refresh")

    native_workflow = (root / ".github/workflows/native.yml").read_text(encoding="utf-8")
    if "HOUSECAT_ENABLE_SANITIZERS=ON" not in native_workflow:
        fail("native CI must run with address and undefined-behavior sanitizers")
    platformio_workflow = (root / ".github/workflows/platformio.yml").read_text(encoding="utf-8")
    for environment in ("crowpanel", "crowpanel_idf5"):
        if environment not in platformio_workflow:
            fail(f"firmware CI does not build {environment}")
    security_workflow = (root / ".github/workflows/security.yml").read_text(encoding="utf-8")
    if "fetch-depth: 0" not in security_workflow or "gitleaks" not in security_workflow.lower():
        fail("security CI must scan complete Git history with Gitleaks")

    pins_text = (root / "src/board/crowpanel_pins.h").read_text(encoding="utf-8")
    for name, expected in EXPECTED_PINS.items():
        match = re.search(rf"\b{name}\s*=\s*(\d+)\s*;", pins_text)
        if match is None or int(match.group(1)) != expected:
            fail(f"pin {name} does not match expected GPIO {expected}")

    rows: list[tuple[str, int, int]] = []
    for raw in (root / "partitions.csv").read_text(encoding="utf-8").splitlines():
        raw = raw.strip()
        if not raw or raw.startswith("#"):
            continue
        parts = [part.strip() for part in raw.split(",")]
        if len(parts) < 5:
            fail(f"invalid partition row: {raw}")
        name, offset, size = parts[0], int(parts[3], 0), int(parts[4], 0)
        rows.append((name, offset, size))
    required_partitions = {"nvs", "otadata", "app0", "app1", "littlefs", "coredump"}
    found_partitions = {name for name, _, _ in rows}
    if found_partitions != required_partitions:
        fail(f"partition table entries differ from expected set: {sorted(found_partitions)}")
    app_sizes = {name: size for name, _, size in rows if name in {"app0", "app1"}}
    if app_sizes.get("app0") != app_sizes.get("app1"):
        fail("dual OTA application slots must be the same size")

    rows.sort(key=lambda row: row[1])
    for (_, offset, size), (next_name, next_offset, _) in zip(rows, rows[1:]):
        if offset + size > next_offset:
            fail(f"partition overlap before {next_name}")
    if max(offset + size for _, offset, size in rows) > 0x800000:
        fail("partition table exceeds 8MB flash")

    gitignore = (root / ".gitignore").read_text(encoding="utf-8")
    if "include/secrets.h" not in gitignore:
        fail(".gitignore does not protect include/secrets.h")

    # A configured development checkout is expected to contain secrets.h.
    # Release packaging has the separate responsibility of excluding it.
    from package_source import should_exclude
    if not should_exclude(Path("include/secrets.h"), is_dir=False):
        fail("source packaging does not exclude include/secrets.h")

    required_art = [
        "CatContent.png", "CatHappy.png", "CatAlert.png", "CatWorried.png",
        "IconHome.png", "IconMenu.png", "IconBack.png", "IconPaw.png",
    ]
    for filename in required_art:
        if not (root / "assets/source" / filename).is_file():
            fail(f"missing required art asset: {filename}")

    generated = root / "lib/housecat_core/include/housecat/generated/assets_generated.h"
    if not generated.is_file() or generated.stat().st_size < 50_000:
        fail("generated bitmap asset header is missing or unexpectedly small")


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    try:
        verify(root)
    except (OSError, ValueError, KeyError, json.JSONDecodeError) as error:
        print(f"Repository verification FAILED: {error}", file=sys.stderr)
        return 1
    print("Repository verification passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

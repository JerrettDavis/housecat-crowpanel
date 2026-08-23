#!/usr/bin/env python3
"""Convert native PGM snapshots into exact-pixel PNGs and a presentation sheet."""
from __future__ import annotations

import subprocess
import os
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
NATIVE = ROOT / "previews" / "native"
OUTPUT = ROOT / "previews"


def resolve_font() -> str:
    try:
        path = subprocess.check_output(
            ["fc-match", "-f", "%{file}", "DejaVu Sans:style=Bold"], text=True
        ).strip()
        if path:
            return path
    except Exception:
        pass
    if os.name == "nt":
        candidate = Path(os.environ.get("WINDIR", r"C:\Windows")) / "Fonts" / "consolab.ttf"
        if candidate.exists():
            return str(candidate)
    return "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"


def main() -> None:
    NATIVE.mkdir(parents=True, exist_ok=True)
    font = ImageFont.truetype(resolve_font(), 16)

    for pgm in sorted(NATIVE.glob("*.pgm")):
        Image.open(pgm).convert("1").save(pgm.with_suffix(".png"))

    cards: list[Image.Image] = []
    for png in sorted(NATIVE.glob("*.png")):
        image = Image.open(png).convert("L")
        scale = 3
        screen = image.resize((image.width * scale, image.height * scale), Image.Resampling.NEAREST)
        pad = 22
        label_height = 34
        card = Image.new("RGB", (screen.width + pad * 2, screen.height + pad * 2 + label_height), (241, 235, 219))
        draw = ImageDraw.Draw(card)
        draw.rounded_rectangle(
            (4, 4, card.width - 5, card.height - label_height - 1),
            radius=20,
            fill=(35, 38, 43),
            outline=(12, 14, 17),
            width=3,
        )
        draw.rounded_rectangle(
            (pad - 2, pad - 2, pad + screen.width + 1, pad + screen.height + 1),
            radius=4,
            fill=(252, 250, 242),
            outline=(0, 0, 0),
            width=2,
        )
        card.paste(screen.convert("RGB"), (pad, pad))
        label = png.stem.split("-", 1)[1].replace("-", " ").title()
        bounds = draw.textbbox((0, 0), label, font=font)
        draw.text(
            ((card.width - (bounds[2] - bounds[0])) // 2, card.height - label_height + 7),
            label,
            font=font,
            fill=(35, 38, 43),
        )
        cards.append(card)

    if not cards:
        raise SystemExit("No simulator PNGs found. Run housecat_simulator first.")

    columns = 4
    rows = (len(cards) + columns - 1) // columns
    cell_width = max(card.width for card in cards) + 20
    cell_height = max(card.height for card in cards) + 20
    sheet = Image.new("RGB", (cell_width * columns, cell_height * rows), (228, 217, 193))
    for index, card in enumerate(cards):
        x = (index % columns) * cell_width + (cell_width - card.width) // 2
        y = (index // columns) * cell_height + (cell_height - card.height) // 2
        sheet.paste(card, (x, y))
    sheet.save(OUTPUT / "housecat-ui-first-pass.png")


if __name__ == "__main__":
    main()

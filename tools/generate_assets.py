#!/usr/bin/env python3
"""Generate House Cat's monochrome sprites, icons, bitmap fonts, and previews.

The generated C++ header is checked in so firmware builds do not depend on Pillow
or local font files. Only rasterized glyphs are emitted; no font files are copied.
"""
from __future__ import annotations

import argparse
import math
import os
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Iterable

from PIL import Image, ImageDraw, ImageFont

SCALE = 4
INK = 0
PAPER = 255

ROOT = Path(__file__).resolve().parents[1]
GENERATED = ROOT / "lib" / "housecat_core" / "include" / "housecat" / "generated"
PREVIEWS = ROOT / "previews"
ASSET_SOURCE = ROOT / "assets" / "source"


def _font_path(pattern: str, fallback: str) -> str:
    try:
        resolved = subprocess.check_output(["fc-match", "-f", "%{file}", pattern], text=True).strip()
        if resolved and Path(resolved).exists():
            return resolved
    except Exception:
        pass
    if Path(fallback).exists():
        return fallback
    if os.name == "nt":
        windows_fonts = Path(os.environ.get("WINDIR", r"C:\Windows")) / "Fonts"
        filename = "consolab.ttf" if "Bold" in pattern else "consola.ttf"
        candidate = windows_fonts / filename
        if candidate.exists():
            return str(candidate)
    raise FileNotFoundError(f"Could not resolve font: {pattern}")


FONT_REGULAR = _font_path("DejaVu Sans Mono", "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf")
FONT_BOLD = _font_path("DejaVu Sans Mono:style=Bold", "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf")
LABEL_FONT = _font_path("DejaVu Sans:style=Bold", "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf")


def canvas(size: tuple[int, int]) -> tuple[Image.Image, ImageDraw.ImageDraw]:
    image = Image.new("L", (size[0] * SCALE, size[1] * SCALE), PAPER)
    return image, ImageDraw.Draw(image)


def p(v: float) -> int:
    return int(round(v * SCALE))


def box(values: Iterable[float]) -> tuple[int, ...]:
    return tuple(p(v) for v in values)


def line(draw: ImageDraw.ImageDraw, points: Iterable[tuple[float, float]], width: float = 2.0, fill: int = INK) -> None:
    draw.line([(p(x), p(y)) for x, y in points], fill=fill, width=max(1, p(width)), joint="curve")


def bezier_points(points: tuple[tuple[float, float], ...], steps: int = 30) -> list[tuple[float, float]]:
    if len(points) != 4:
        raise ValueError("Cubic bezier requires four control points")
    result: list[tuple[float, float]] = []
    for i in range(steps + 1):
        t = i / steps
        u = 1 - t
        x = u**3 * points[0][0] + 3*u*u*t*points[1][0] + 3*u*t*t*points[2][0] + t**3*points[3][0]
        y = u**3 * points[0][1] + 3*u*u*t*points[1][1] + 3*u*t*t*points[2][1] + t**3*points[3][1]
        result.append((x, y))
    return result


def outlined_tail(draw: ImageDraw.ImageDraw, points: tuple[tuple[float, float], ...], outer: float = 8, inner: float = 4) -> None:
    curve = bezier_points(points)
    line(draw, curve, outer, INK)
    line(draw, curve, inner, PAPER)


def ellipse(draw: ImageDraw.ImageDraw, coords: tuple[float, float, float, float], fill: int = PAPER, outline: int = INK, width: float = 2.0) -> None:
    draw.ellipse(box(coords), fill=fill, outline=outline, width=max(1, p(width)))


def polygon(draw: ImageDraw.ImageDraw, points: Iterable[tuple[float, float]], fill: int = PAPER, outline: int = INK, width: float = 2.0) -> None:
    pts = [(p(x), p(y)) for x, y in points]
    draw.polygon(pts, fill=fill)
    draw.line(pts + [pts[0]], fill=outline, width=max(1, p(width)), joint="curve")


def arc(draw: ImageDraw.ImageDraw, coords: tuple[float, float, float, float], start: float, end: float, width: float = 2.0, fill: int = INK) -> None:
    draw.arc(box(coords), start=start, end=end, fill=fill, width=max(1, p(width)))


def rounded(draw: ImageDraw.ImageDraw, coords: tuple[float, float, float, float], radius: float, fill: int = PAPER, outline: int = INK, width: float = 2.0) -> None:
    draw.rounded_rectangle(box(coords), radius=p(radius), fill=fill, outline=outline, width=max(1, p(width)))


def heart(draw: ImageDraw.ImageDraw, x: float, y: float, size: float, fill: int = INK) -> None:
    r = size * 0.27
    draw.ellipse(box((x, y, x + 2*r, y + 2*r)), fill=fill)
    draw.ellipse(box((x + 2*r, y, x + 4*r, y + 2*r)), fill=fill)
    draw.polygon([(p(x), p(y+r)), (p(x+4*r), p(y+r)), (p(x+2*r), p(y+size))], fill=fill)


def star(draw: ImageDraw.ImageDraw, cx: float, cy: float, outer: float, inner: float | None = None, fill: int = INK) -> None:
    inner = inner if inner is not None else outer * 0.45
    pts = []
    for i in range(10):
        radius = outer if i % 2 == 0 else inner
        a = -math.pi / 2 + i * math.pi / 5
        pts.append((p(cx + math.cos(a)*radius), p(cy + math.sin(a)*radius)))
    draw.polygon(pts, fill=fill)


def final_1bit(image: Image.Image, threshold: int = 174) -> Image.Image:
    image = image.resize((image.width // SCALE, image.height // SCALE), Image.Resampling.LANCZOS)
    return image.point(lambda x: PAPER if x > threshold else INK, mode="1")


def draw_face(draw: ImageDraw.ImageDraw, mood: str, ox: float = 0, oy: float = 0) -> None:
    if mood in {"happy", "pet"}:
        arc(draw, (31+ox, 31+oy, 41+ox, 42+oy), 15, 165, 2.2)
        arc(draw, (48+ox, 31+oy, 58+ox, 42+oy), 15, 165, 2.2)
        draw.polygon([(p(43+ox), p(42+oy)), (p(47+ox), p(42+oy)), (p(45+ox), p(45+oy))], fill=INK)
        arc(draw, (39+ox, 42+oy, 51+ox, 53+oy), 5, 175, 2.0)
    elif mood == "alert":
        ellipse(draw, (31+ox, 32+oy, 40+ox, 44+oy), PAPER, INK, 1.8)
        ellipse(draw, (49+ox, 32+oy, 58+ox, 44+oy), PAPER, INK, 1.8)
        draw.ellipse(box((34+ox, 36+oy, 38+ox, 42+oy)), fill=INK)
        draw.ellipse(box((52+ox, 36+oy, 56+ox, 42+oy)), fill=INK)
        draw.ellipse(box((42+ox, 45+oy, 48+ox, 51+oy)), fill=INK)
    elif mood == "sleepy":
        line(draw, [(31+ox, 39+oy), (39+ox, 39+oy)], 2.0)
        line(draw, [(50+ox, 39+oy), (58+ox, 39+oy)], 2.0)
        draw.polygon([(p(43+ox), p(43+oy)), (p(47+ox), p(43+oy)), (p(45+ox), p(46+oy))], fill=INK)
        arc(draw, (41+ox, 45+oy, 49+ox, 51+oy), 190, 350, 1.8)
    elif mood == "worried":
        line(draw, [(31+ox, 36+oy), (39+ox, 39+oy)], 2.0)
        line(draw, [(50+ox, 39+oy), (58+ox, 36+oy)], 2.0)
        draw.ellipse(box((34+ox, 39+oy, 38+ox, 43+oy)), fill=INK)
        draw.ellipse(box((52+ox, 39+oy, 56+ox, 43+oy)), fill=INK)
        draw.polygon([(p(43+ox), p(45+oy)), (p(47+ox), p(45+oy)), (p(45+ox), p(48+oy))], fill=INK)
        arc(draw, (40+ox, 48+oy, 50+ox, 56+oy), 195, 345, 1.8)
    elif mood == "curious":
        ellipse(draw, (31+ox, 34+oy, 40+ox, 43+oy), PAPER, INK, 1.7)
        ellipse(draw, (49+ox, 32+oy, 59+ox, 42+oy), PAPER, INK, 1.7)
        draw.ellipse(box((34+ox, 37+oy, 38+ox, 41+oy)), fill=INK)
        draw.ellipse(box((52+ox, 35+oy, 56+ox, 39+oy)), fill=INK)
        draw.polygon([(p(43+ox), p(44+oy)), (p(47+ox), p(44+oy)), (p(45+ox), p(47+oy))], fill=INK)
        arc(draw, (41+ox, 46+oy, 50+ox, 52+oy), 350, 175, 1.8)
    else:
        ellipse(draw, (32+ox, 34+oy, 39+ox, 42+oy), INK, INK, 1)
        ellipse(draw, (51+ox, 34+oy, 58+ox, 42+oy), INK, INK, 1)
        draw.ellipse(box((34+ox, 35+oy, 36+ox, 37+oy)), fill=PAPER)
        draw.ellipse(box((53+ox, 35+oy, 55+ox, 37+oy)), fill=PAPER)
        draw.polygon([(p(43+ox), p(43+oy)), (p(47+ox), p(43+oy)), (p(45+ox), p(46+oy))], fill=INK)
        arc(draw, (39+ox, 44+oy, 45+ox, 51+oy), 350, 90, 1.6)
        arc(draw, (45+ox, 44+oy, 51+ox, 51+oy), 90, 190, 1.6)

    # whiskers
    line(draw, [(30+ox, 46+oy), (18+ox, 44+oy)], 1.3)
    line(draw, [(30+ox, 49+oy), (17+ox, 51+oy)], 1.3)
    line(draw, [(60+ox, 46+oy), (72+ox, 44+oy)], 1.3)
    line(draw, [(60+ox, 49+oy), (73+ox, 51+oy)], 1.3)


def draw_sitting_cat(draw: ImageDraw.ImageDraw, mood: str = "content", extras: str | None = None) -> None:
    # tail behind body
    outlined_tail(draw, ((62, 70), (84, 69), (82, 42), (70, 52)), 8, 4)
    # body and haunches
    ellipse(draw, (27, 49, 65, 84), PAPER, INK, 2.5)
    ellipse(draw, (20, 69, 42, 85), PAPER, INK, 2.2)
    ellipse(draw, (51, 69, 72, 85), PAPER, INK, 2.2)
    # head + ears
    polygon(draw, [(24, 28), (29, 8), (42, 23)], PAPER, INK, 2.5)
    polygon(draw, [(49, 23), (63, 8), (68, 29)], PAPER, INK, 2.5)
    draw.polygon([(p(29), p(24)), (p(31), p(14)), (p(38), p(23))], fill=INK)
    draw.polygon([(p(53), p(23)), (p(61), p(14)), (p(63), p(25))], fill=INK)
    ellipse(draw, (21, 18, 70, 63), PAPER, INK, 2.7)
    draw_face(draw, mood)
    # chest and toes
    arc(draw, (34, 56, 56, 77), 25, 155, 1.5)
    line(draw, [(31, 78), (31, 82)], 1.3)
    line(draw, [(61, 78), (61, 82)], 1.3)
    # forehead markings
    line(draw, [(42, 21), (40, 28)], 1.4)
    line(draw, [(46, 20), (46, 28)], 1.4)
    line(draw, [(50, 21), (52, 28)], 1.4)

    if extras == "happy":
        # raised paws, stars/confetti
        ellipse(draw, (12, 49, 28, 65), PAPER, INK, 2.0)
        ellipse(draw, (64, 48, 80, 64), PAPER, INK, 2.0)
        star(draw, 13, 26, 4)
        star(draw, 77, 24, 3.5)
        line(draw, [(10, 37), (5, 34)], 1.6)
        line(draw, [(80, 36), (85, 32)], 1.6)
    elif extras == "alert":
        rounded(draw, (72, 8, 84, 36), 4, PAPER, INK, 2.0)
        line(draw, [(78, 13), (78, 25)], 2.5)
        draw.ellipse(box((76.5, 29, 79.5, 32)), fill=INK)
    elif extras == "curious":
        # question mark and lifted paw
        ellipse(draw, (60, 57, 76, 72), PAPER, INK, 2.0)
        arc(draw, (7, 10, 25, 28), 200, 80, 2.0)
        line(draw, [(17, 25), (17, 31)], 2.0)
        draw.ellipse(box((15.5, 35, 18.5, 38)), fill=INK)
    elif extras == "worried":
        # sweat drop
        draw.polygon([(p(72), p(24)), (p(77), p(34)), (p(68), p(34))], fill=INK)
        draw.ellipse(box((68, 29, 77, 38)), fill=INK)
    elif extras == "pet":
        heart(draw, 7, 14, 12)
        heart(draw, 69, 10, 9)
        ellipse(draw, (60, 55, 77, 72), PAPER, INK, 2.0)
    elif extras == "explorer":
        # scarf, backpack, little antenna/radio waves
        line(draw, [(30, 58), (61, 58)], 4.0)
        draw.polygon([(p(50), p(59)), (p(64), p(72)), (p(54), p(70))], fill=INK)
        rounded(draw, (61, 55, 77, 75), 4, PAPER, INK, 2.0)
        arc(draw, (4, 13, 26, 36), 290, 70, 1.8)
        arc(draw, (8, 17, 22, 31), 290, 70, 1.8)
        draw.ellipse(box((13, 23, 17, 27)), fill=INK)


def sprite_content() -> Image.Image:
    image, draw = canvas((88, 88))
    draw_sitting_cat(draw, "content")
    return final_1bit(image)


def sprite_happy() -> Image.Image:
    image, draw = canvas((88, 88))
    draw_sitting_cat(draw, "happy", "happy")
    return final_1bit(image)


def sprite_alert() -> Image.Image:
    image, draw = canvas((88, 88))
    draw_sitting_cat(draw, "alert", "alert")
    return final_1bit(image)


def sprite_curious() -> Image.Image:
    image, draw = canvas((88, 88))
    draw_sitting_cat(draw, "curious", "curious")
    return final_1bit(image)


def sprite_worried() -> Image.Image:
    image, draw = canvas((88, 88))
    draw_sitting_cat(draw, "worried", "worried")
    return final_1bit(image)


def sprite_pet() -> Image.Image:
    image, draw = canvas((88, 88))
    draw_sitting_cat(draw, "pet", "pet")
    return final_1bit(image)


def sprite_explorer() -> Image.Image:
    image, draw = canvas((88, 88))
    draw_sitting_cat(draw, "curious", "explorer")
    return final_1bit(image)


def sprite_sleepy() -> Image.Image:
    image, draw = canvas((88, 88))
    # curled body and tail
    ellipse(draw, (13, 43, 78, 82), PAPER, INK, 2.8)
    outlined_tail(draw, ((70, 70), (81, 57), (66, 48), (52, 65)), 8, 4)
    polygon(draw, [(22, 51), (25, 31), (38, 46)], PAPER, INK, 2.5)
    polygon(draw, [(43, 46), (57, 31), (61, 52)], PAPER, INK, 2.5)
    ellipse(draw, (17, 39, 64, 72), PAPER, INK, 2.6)
    draw_face(draw, "sleepy", -4, 12)
    # tucked paw and body markings
    arc(draw, (33, 57, 61, 79), 180, 350, 1.7)
    line(draw, [(44, 61), (52, 70)], 1.5)
    # Z marks
    line(draw, [(63, 27), (77, 27), (64, 40), (79, 40)], 2.0)
    line(draw, [(70, 15), (81, 15), (72, 24), (83, 24)], 1.8)
    return final_1bit(image)


SPRITES: dict[str, Callable[[], Image.Image]] = {
    "CatContent": sprite_content,
    "CatHappy": sprite_happy,
    "CatAlert": sprite_alert,
    "CatSleepy": sprite_sleepy,
    "CatCurious": sprite_curious,
    "CatWorried": sprite_worried,
    "CatPet": sprite_pet,
    "CatExplorer": sprite_explorer,
}


def icon_image(drawer: Callable[[ImageDraw.ImageDraw], None], size: int = 24) -> Image.Image:
    image, draw = canvas((size, size))
    drawer(draw)
    return final_1bit(image, threshold=180)


def icon_home(d: ImageDraw.ImageDraw) -> None:
    polygon(d, [(3, 11), (12, 3), (21, 11), (19, 11), (19, 21), (5, 21), (5, 11)], PAPER, INK, 2)
    rounded(d, (9, 14, 15, 21), 1, PAPER, INK, 1.6)


def icon_paw(d: ImageDraw.ImageDraw) -> None:
    ellipse(d, (7, 10, 17, 21), INK, INK, 1)
    for coords in [(3, 6, 8, 12), (8, 2, 13, 9), (14, 3, 19, 10), (18, 8, 22, 14)]:
        ellipse(d, coords, INK, INK, 1)


def icon_star(d: ImageDraw.ImageDraw) -> None:
    star(d, 12, 12, 10)


def icon_radio(d: ImageDraw.ImageDraw) -> None:
    draw = d
    draw.ellipse(box((10, 10, 14, 14)), fill=INK)
    arc(draw, (6, 6, 18, 18), 300, 60, 1.8)
    arc(draw, (2, 2, 22, 22), 300, 60, 1.8)
    line(draw, [(12, 14), (12, 22)], 2)


def icon_book(d: ImageDraw.ImageDraw) -> None:
    rounded(d, (3, 4, 11.5, 21), 1.5, PAPER, INK, 1.8)
    rounded(d, (12.5, 4, 21, 21), 1.5, PAPER, INK, 1.8)
    line(d, [(12, 5), (12, 21)], 1.5)
    line(d, [(6, 8), (9, 8)], 1.2)
    line(d, [(15, 8), (19, 8)], 1.2)


def icon_flask(d: ImageDraw.ImageDraw) -> None:
    rounded(d, (9, 2, 15, 8), 1, PAPER, INK, 1.6)
    line(d, [(10, 8), (5, 18), (6, 21), (18, 21), (19, 18), (14, 8)], 2)
    line(d, [(7, 16), (17, 16)], 1.5)
    draw = d
    draw.ellipse(box((10, 17, 13, 20)), fill=INK)
    draw.ellipse(box((14, 13, 17, 16)), fill=INK)


def icon_gear(d: ImageDraw.ImageDraw) -> None:
    star(d, 12, 12, 10, 7)
    ellipse(d, (8, 8, 16, 16), PAPER, PAPER, 1)
    ellipse(d, (9, 9, 15, 15), PAPER, INK, 1.6)


def icon_sun(d: ImageDraw.ImageDraw) -> None:
    ellipse(d, (7, 7, 17, 17), PAPER, INK, 2)
    for a in range(0, 360, 45):
        r1, r2 = 7, 11
        x1, y1 = 12 + math.cos(math.radians(a))*r1, 12 + math.sin(math.radians(a))*r1
        x2, y2 = 12 + math.cos(math.radians(a))*r2, 12 + math.sin(math.radians(a))*r2
        line(d, [(x1, y1), (x2, y2)], 1.6)


def icon_cloud(d: ImageDraw.ImageDraw) -> None:
    ellipse(d, (3, 10, 11, 19), PAPER, INK, 1.8)
    ellipse(d, (8, 6, 17, 18), PAPER, INK, 1.8)
    ellipse(d, (14, 10, 22, 19), PAPER, INK, 1.8)
    draw = d
    draw.rectangle(box((6, 13, 19, 19)), fill=PAPER)
    line(d, [(5, 19), (20, 19)], 1.8)


def icon_rain(d: ImageDraw.ImageDraw) -> None:
    icon_cloud(d)
    for x in [7, 12, 17]:
        line(d, [(x, 20), (x-1, 23)], 1.4)


def icon_thermometer(d: ImageDraw.ImageDraw) -> None:
    rounded(d, (9, 2, 15, 17), 3, PAPER, INK, 1.8)
    ellipse(d, (7, 14, 17, 23), PAPER, INK, 1.8)
    line(d, [(12, 7), (12, 18)], 2.5)
    draw = d
    draw.ellipse(box((10, 17, 14, 21)), fill=INK)


def icon_wifi(d: ImageDraw.ImageDraw) -> None:
    arc(d, (2, 3, 22, 23), 220, 320, 2)
    arc(d, (6, 7, 18, 19), 220, 320, 2)
    arc(d, (9, 10, 15, 16), 220, 320, 2)
    d.ellipse(box((10.5, 18, 13.5, 21)), fill=INK)


def icon_bell(d: ImageDraw.ImageDraw) -> None:
    arc(d, (5, 4, 19, 19), 180, 360, 2)
    line(d, [(5, 11), (4, 19), (20, 19), (19, 11)], 2)
    line(d, [(9, 22), (15, 22)], 2)
    d.ellipse(box((10, 1, 14, 5)), fill=INK)


def icon_person(d: ImageDraw.ImageDraw) -> None:
    ellipse(d, (8, 2, 16, 10), PAPER, INK, 1.8)
    rounded(d, (5, 11, 19, 22), 5, PAPER, INK, 1.8)


def icon_car_charge(d: ImageDraw.ImageDraw) -> None:
    rounded(d, (2, 9, 18, 19), 3, PAPER, INK, 1.8)
    line(d, [(5, 9), (8, 4), (15, 4), (18, 9)], 1.8)
    ellipse(d, (5, 17, 9, 22), PAPER, INK, 1.5)
    ellipse(d, (13, 17, 17, 22), PAPER, INK, 1.5)
    d.polygon([(p(20), p(4)), (p(16), p(12)), (p(20), p(12)), (p(17), p(20)), (p(23), p(10)), (p(19), p(10))], fill=INK)


def icon_heart(d: ImageDraw.ImageDraw) -> None:
    heart(d, 3, 4, 19)


def icon_check(d: ImageDraw.ImageDraw) -> None:
    ellipse(d, (2, 2, 22, 22), PAPER, INK, 1.8)
    line(d, [(6, 12), (10, 17), (19, 7)], 2.8)


def icon_menu(d: ImageDraw.ImageDraw) -> None:
    for y in (6, 12, 18):
        rounded(d, (3, y-2, 7, y+2), 1, INK, INK, 1)
        line(d, [(10, y), (22, y)], 2)


def icon_back(d: ImageDraw.ImageDraw) -> None:
    line(d, [(14, 4), (6, 12), (14, 20)], 3)
    line(d, [(7, 12), (22, 12)], 2.5)


def icon_mission(d: ImageDraw.ImageDraw) -> None:
    line(d, [(5, 2), (5, 22)], 2)
    polygon(d, [(6, 4), (20, 6), (16, 12), (6, 10)], PAPER, INK, 1.8)
    star(d, 12, 8, 3)


def icon_rotation(d: ImageDraw.ImageDraw) -> None:
    arc(d, (3, 3, 21, 21), 190, 25, 2.0)
    d.polygon([(p(20), p(4)), (p(21), p(11)), (p(15), p(8))], fill=INK)
    rounded(d, (8, 7, 16, 18), 1, PAPER, INK, 1.5)


ICONS: dict[str, Callable[[ImageDraw.ImageDraw], None]] = {
    "IconHome": icon_home,
    "IconPaw": icon_paw,
    "IconStar": icon_star,
    "IconRadio": icon_radio,
    "IconBook": icon_book,
    "IconFlask": icon_flask,
    "IconGear": icon_gear,
    "IconSun": icon_sun,
    "IconCloud": icon_cloud,
    "IconRain": icon_rain,
    "IconThermometer": icon_thermometer,
    "IconWifi": icon_wifi,
    "IconBell": icon_bell,
    "IconPerson": icon_person,
    "IconCarCharge": icon_car_charge,
    "IconHeart": icon_heart,
    "IconCheck": icon_check,
    "IconMenu": icon_menu,
    "IconBack": icon_back,
    "IconMission": icon_mission,
    "IconRotation": icon_rotation,
}


def render_font(
    path: str,
    pixel_size: int,
    cell_w: int,
    cell_h: int,
    bold_offset: int = 0,
    threshold: int = 175,
) -> list[Image.Image]:
    font = ImageFont.truetype(path, pixel_size * SCALE)
    glyphs: list[Image.Image] = []
    for code in range(32, 127):
        image = Image.new("L", (cell_w * SCALE, cell_h * SCALE), PAPER)
        draw = ImageDraw.Draw(image)
        char = chr(code)
        bbox = draw.textbbox((0, 0), char, font=font, stroke_width=0)
        tw = bbox[2] - bbox[0]
        th = bbox[3] - bbox[1]
        x = (cell_w * SCALE - tw) // 2 - bbox[0]
        y = (cell_h * SCALE - th) // 2 - bbox[1]
        draw.text((x, y), char, font=font, fill=INK)
        if bold_offset:
            draw.text((x + bold_offset*SCALE, y), char, font=font, fill=INK)
        glyphs.append(final_1bit(image, threshold=threshold))
    return glyphs


def image_to_bytes(image: Image.Image) -> list[int]:
    image = image.convert("1")
    width, height = image.size
    stride = (width + 7) // 8
    data = [0] * (stride * height)
    px = image.load()
    for y in range(height):
        for x in range(width):
            if px[x, y] == 0:
                data[y*stride + x//8] |= 0x80 >> (x % 8)
    return data


def array_text(data: list[int], per_line: int = 16) -> str:
    chunks = []
    for i in range(0, len(data), per_line):
        chunks.append("    " + ", ".join(f"0x{b:02X}" for b in data[i:i+per_line]) + ",")
    return "\n".join(chunks)


def emit_header(bitmaps: dict[str, Image.Image], fonts: dict[str, tuple[int, int, list[Image.Image]]]) -> None:
    GENERATED.mkdir(parents=True, exist_ok=True)
    out = GENERATED / "assets_generated.h"
    lines = [
        "#pragma once",
        "",
        "#include <cstdint>",
        '#include "housecat/ui/bitmap.h"',
        '#include "housecat/ui/font.h"',
        "",
        "namespace housecat::generated {",
        "",
    ]
    for name, image in bitmaps.items():
        data = image_to_bytes(image)
        lines.append(f"inline constexpr std::uint8_t k{name}Data[] = {{")
        lines.append(array_text(data))
        lines.append("};")
        lines.append(f"inline constexpr Bitmap k{name}{{{image.width}, {image.height}, k{name}Data}};")
        lines.append("")
    for name, (cell_w, cell_h, glyphs) in fonts.items():
        data: list[int] = []
        for glyph in glyphs:
            data.extend(image_to_bytes(glyph))
        lines.append(f"inline constexpr std::uint8_t k{name}Data[] = {{")
        lines.append(array_text(data))
        lines.append("};")
        lines.append(f"inline constexpr BitmapFont k{name}{{32, 126, {cell_w}, {cell_h}, k{name}Data}};")
        lines.append("")
    lines.append("}  // namespace housecat::generated")
    out.write_text("\n".join(lines) + "\n", encoding="utf-8")


def save_source_assets(bitmaps: dict[str, Image.Image]) -> None:
    ASSET_SOURCE.mkdir(parents=True, exist_ok=True)
    for name, image in bitmaps.items():
        image.convert("L").save(ASSET_SOURCE / f"{name}.png")


def sprite_sheet(sprites: dict[str, Image.Image]) -> Image.Image:
    cols = 4
    tile_w, tile_h = 132, 126
    rows = math.ceil(len(sprites) / cols)
    sheet = Image.new("L", (cols*tile_w, rows*tile_h), PAPER)
    draw = ImageDraw.Draw(sheet)
    label = ImageFont.truetype(LABEL_FONT, 14)
    for i, (name, image) in enumerate(sprites.items()):
        col, row = i % cols, i // cols
        x, y = col*tile_w, row*tile_h
        scaled = image.resize((88*1, 88*1), Image.Resampling.NEAREST).convert("L")
        sheet.paste(scaled, (x + (tile_w-88)//2, y+8))
        text = name.removeprefix("Cat")
        bbox = draw.textbbox((0,0), text, font=label)
        draw.text((x + (tile_w-(bbox[2]-bbox[0]))//2, y+101), text, font=label, fill=INK)
    return sheet


def icon_sheet(icons: dict[str, Image.Image]) -> Image.Image:
    cols = 7
    tile_w, tile_h = 78, 66
    rows = math.ceil(len(icons)/cols)
    sheet = Image.new("L", (cols*tile_w, rows*tile_h), PAPER)
    draw = ImageDraw.Draw(sheet)
    label = ImageFont.truetype(LABEL_FONT, 9)
    for i, (name, image) in enumerate(icons.items()):
        col, row = i % cols, i // cols
        x, y = col*tile_w, row*tile_h
        scaled = image.resize((36,36), Image.Resampling.NEAREST).convert("L")
        sheet.paste(scaled, (x+(tile_w-36)//2, y+4))
        text = name.removeprefix("Icon")
        bbox = draw.textbbox((0,0), text, font=label)
        draw.text((x + (tile_w-(bbox[2]-bbox[0]))//2, y+44), text, font=label, fill=INK)
    return sheet


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.parse_args()
    PREVIEWS.mkdir(parents=True, exist_ok=True)

    sprites = {name: builder() for name, builder in SPRITES.items()}
    icons = {name: icon_image(builder) for name, builder in ICONS.items()}
    bitmaps = {**sprites, **icons}
    fonts = {
        # At this size a regular antialiased face loses entire stems when
        # reduced to one bit. Use the bold face and a lighter threshold so
        # every glyph retains a connected, e-paper-friendly skeleton.
        "FontSmall": (6, 9, render_font(FONT_BOLD, 8, 6, 9, threshold=205)),
        # A higher source size and lighter threshold preserve open counters in
        # large temperature digits (notably 2, 6, 8, and 9) on the 1-bit panel.
        "FontBold": (7, 11, render_font(FONT_BOLD, 9, 7, 11, threshold=200)),
        # Extra-large, sturdy reading type sized specifically for the e-paper
        # reader. The logical pages are kept short enough for both orientations.
        "FontReader": (11, 18, render_font(FONT_BOLD, 16, 11, 18, threshold=200)),
    }

    emit_header(bitmaps, fonts)
    save_source_assets(bitmaps)
    sprite_sheet(sprites).save(PREVIEWS / "housecat-spritesheet.png")
    icon_sheet(icons).save(PREVIEWS / "housecat-icons.png")
    print(f"Generated {len(sprites)} sprites, {len(icons)} icons, and {len(fonts)} fonts")


if __name__ == "__main__":
    main()

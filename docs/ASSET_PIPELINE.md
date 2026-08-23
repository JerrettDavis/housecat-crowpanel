# House Cat Art and Asset Pipeline

## Visual target

House Cat uses authored monochrome illustration, not emoji, ASCII art, or terminal block approximations. Sprites are drawn at high resolution, antialiased during construction, reduced to exact one-bit pixels, and packed into firmware.

## Current library

### Cat poses

- Content
- Happy
- Alert
- Sleepy
- Curious
- Worried
- Pet
- Explorer

Each pose is 88×88 pixels and shares a recognizable silhouette, face construction, forehead marks, paws, and tail language.

### Icons

The initial 24×24 family covers Home, Paw, Star, Radio, Book, Lab, Settings, weather, temperature, Wi-Fi, notification, person, charging vehicle, heart, check, mission, rotation, and control symbols.

## Regeneration

```bash
python3 tools/generate_assets.py
```

The script writes:

- source PNGs under `assets/source/`
- packed C++ data in `lib/housecat_core/include/housecat/generated/assets_generated.h`
- sprite and icon preview sheets under `previews/`

The generated header is committed. Pillow and the local font installation are development dependencies only.

## Embedded format

- one bit per pixel
- MSB first in each byte
- row stride rounded to whole bytes
- `1` means ink/black
- sprites may be scaled with nearest-neighbor sampling by `MonoCanvas`
- transparent bitmap pixels leave the existing framebuffer untouched

An 88×88 sprite occupies only 968 bytes before compiler/linker deduplication and compression opportunities. The first set therefore costs very little relative to the target flash.

## Art rules for additions

- preserve the same head, ears, eye spacing, paws, and tail silhouette
- make the emotional read obvious at the final physical size
- use props sparingly and keep them outside the face
- avoid fragile one-pixel details that reproduce poorly on e-paper
- test against both portrait and landscape layouts
- preview at exact pixels, not only enlarged smoothing
- include a domain icon whenever the event cannot be understood from the cat pose alone

## Fonts

The checked-in header contains rasterized glyphs generated from DejaVu Sans Mono. No font files are distributed with the project. See `THIRD_PARTY_NOTICES.md` for attribution.

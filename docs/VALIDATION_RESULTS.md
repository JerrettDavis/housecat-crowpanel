# House Cat v0.1.0-alpha.1 validation results

Validation date: 2026-08-22

## Automated evidence

- Strict GCC 14.2 native build passes with `-Wall -Wextra -Wpedantic
  -Wconversion -Wshadow`.
- The expanded native behavior suite passes.
- Every embedded translation unit passes the host API-shape compile.
- The real ESP32-S3 `crowpanel` cross-build passes at 34.1% flash and 17.2%
  static RAM.
- Repository structure, pinned minimal dependencies, partition boundaries,
  secret exclusion, and generated-asset contracts pass `tools/verify_repo.py`.
- Python tools compile; Home Assistant YAML and all SVG files parse.
- Asset generation is deterministic: regenerating three packed bitmap fonts,
  eight cat sprites, and twenty-one icons produces the same header hash.
- Fifteen exact-pixel simulator snapshots render successfully, including the
  Pal card and Library reader in both layout families.
- Source packaging generated and verified a 197-file release with a fresh
  SHA-256 manifest and no `include/secrets.h`.
- Linux GCC and Clang CI are configured to add AddressSanitizer and
  UndefinedBehaviorSanitizer.

## Behavior and boundary coverage

- all four coordinate transforms and exact JD79661 row/polarity encoding
- hidden six-column panel padding remains white
- four-card Home navigation and Pal-card live updates
- menu, Back, global Home, and contextual action-label semantics
- bounded and normalized Home, room, mission, notification, and persisted data
- saturating XP math and monotonic-clock cooldown handling
- notification priority, capacity, preemption, deduplication, expiry, and acknowledgement
- Library page bounds, automatic bookmarks, cached resume, and transactional cache replacement
- stale weather transitions and off-screen repaint suppression
- visible cat and substantial rendering across every screen and orientation
- long unbroken words wrap without drawing outside their component

## Physical evidence

- Firmware uploads reliably over its detected USB serial port.
- The direct JD79661 waveform renders correct polarity, row order, orientation,
  and left-to-right text in roughly 720 ms.
- The panel reconnects to its configured IoT network and MQTT broker after boot.
- Retained Home Assistant weather and Hallway readings populate Home.
- LittleFS mounts, the existing 4,063-page Alice index is reusable, and cached
  Library resume survives firmware flashing and reboot.
- Runtime status after the hardened build reported about 273 KB free heap and
  8.37 MB free PSRAM.

The driver intentionally uses one qualified full-refresh environment. Logical
coalescible/immediate requests remain application scheduling concepts; they are
both promoted to the safe full waveform by the panel adapter.

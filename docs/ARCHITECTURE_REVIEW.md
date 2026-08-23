# Architecture review

Review date: 2026-08-22

## Result

The repository keeps a portable C++17 domain/application/rendering core and a
thin ESP32 adapter layer. Hardware, MQTT, Home Assistant, NVS, HTTP, and
LittleFS do not make layout decisions. The application emits semantic events
and render intent; the renderer owns pixels; the JD79661 adapter owns physical
row order, polarity, padding, and waveform presentation.

## Improvements made

- Removed stale GxEPD2/Adafruit dependencies and the byte-identical
  `crowpanel_safe` configuration.
- Centralized semantic enum names and protocol parsers in `app/semantics.h`.
- Centralized Home card count/roles and refresh-strength composition.
- Preserved controls queued during the blocking e-paper refresh instead of
  discarding them.
- Made Pal-card snapshot updates repaint when visible.
- Bounded and normalized external/persisted strings, numbers, room counts,
  humidity, missions, and reader indices at application boundaries.
- Made progression arithmetic saturating and safe across clock discontinuity.
- Made Library cache replacement transactional and validated the candidate's
  page index before replacing the offline-readable copy.
- Made wrapped text honor both horizontal and vertical component bounds,
  including long unbroken words.
- Replaced per-pixel panel conversion with a pure tested byte transform and
  moved its 4 KB output frame off the loop-task stack.
- Made control hints describe their real action (`PET`, `SCAN`, `READ`, `LIST`,
  `PROBE`, `SAVE`).
- Fixed the repository verifier's local-secret contract and strengthened it to
  enforce minimal dependencies, buffered controls, and sanitizer CI.
- Removed the stale root release manifest; releases generate and verify their
  own manifest after excluding local secrets and build output.

## Quality gates

See [VALIDATION_RESULTS.md](VALIDATION_RESULTS.md) for the executed native,
cross-compile, repository, artifact, packaging, rendering, and hardware checks.
Linux GCC and Clang CI additionally run ASan and UBSan.

## Intentional constraints

- The qualified panel sequence is a full refresh (~720 ms). Application render
  intent still controls coalescing/immediacy, while the input task remains live.
- Gutenberg transport is encrypted but currently uses `setInsecure()` because
  this Arduino build has no maintained CA bundle. The curated transfer carries
  no credentials or executable data, is capped at 1.8 MB, is ASCII-filtered,
  and is validated/indexed before transactional cache replacement.
- The public three-title catalog is deliberately fixed; arbitrary remote URLs
  never enter the firmware.

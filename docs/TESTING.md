# House Cat Testing Strategy

House Cat uses four complementary validation layers. No one layer is treated as proof of physical hardware behavior.

## 1. Repository verification

```bash
python3 tools/verify_repo.py
```

The verifier checks:

- firmware version consistency
- pinned PlatformIO platform and custom board name
- ESP32-S3, 8 MB flash, OPI PSRAM, and USB-mode flags
- exact CrowPanel pin assignments
- partition ordering, overlap, and 8 MB boundary
- Git and release-package protection of local `include/secrets.h`
- presence of key original art assets
- presence and expected scale of the packed generated asset header

## 2. Native core and embedded API-shape build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

The CMake build produces:

- `housecat_core`: the portable C++17 domain, state machine, renderer, and assets
- `housecat_tests`: deterministic behavioral and rendering tests
- `housecat_simulator`: exact-pixel screen snapshot generator
- `housecat_embedded_compilecheck`: every embedded translation unit compiled against small API-shaped Arduino, ESP32, FreeRTOS, Preferences, Wi-Fi, MQTT, filesystem, HTTP, and ArduinoJson stubs

The embedded compile check is not a replacement for PlatformIO. It catches ordinary C++ errors, missing declarations, signature mismatches, and warning regressions without downloading a cross-toolchain.

Recommended local matrix:

```bash
CC=gcc CXX=g++ cmake -S . -B build-gcc -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DHOUSECAT_ENABLE_SANITIZERS=ON
cmake --build build-gcc
ctest --test-dir build-gcc --output-on-failure

CC=clang CXX=clang++ cmake -S . -B build-clang -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DHOUSECAT_ENABLE_SANITIZERS=ON
cmake --build build-clang
ctest --test-dir build-clang --output-on-failure
```

All native targets enable `-Wall -Wextra -Wpedantic -Wconversion -Wshadow` where supported.
The recommended GCC/Clang matrix also enables AddressSanitizer and
UndefinedBehaviorSanitizer.

## 3. PlatformIO cross-build

```bash
python3 -m platformio run -e crowpanel_idf5
python3 -m platformio run -e crowpanel
```

A successful target build proves that the source resolves against the pinned ESP32 Arduino framework and actual third-party libraries. It also reports application and RAM usage against the selected partition and board profile.

Export the build products with:

```bash
python3 tools/export_firmware.py --env crowpanel_idf5
```

The GitHub workflow at `.github/workflows/platformio.yml` builds and exports both firmware environments.

## 4. Physical qualification

Native and cross builds cannot prove:

- exact panel/controller match
- display power or BUSY electrical behavior
- qualified full-refresh waveform quality
- ghosting tolerance
- physical control wiring and ergonomics
- upload reliability through a particular cable or CH340 driver
- Wi-Fi and broker behavior in the target environment
- Home Assistant processing of discovery and automations

Follow [`FIRST_FLASH.md`](FIRST_FLASH.md) and complete [`HARDWARE_ACCEPTANCE.md`](HARDWARE_ACCEPTANCE.md).

## Automated behavior coverage

Current tests cover:

- 0°, 90°, 180°, and 270° coordinate transforms
- orientation-dependent logical dimensions
- progression thresholds and bond changes
- interaction reward cooldown
- coalesced versus immediate render scheduling and refresh budgeting
- global Home/Menu and Back semantics
- rocker menu selection
- mission update and completion events, including off-screen persistence signals
- notification priority preemption
- required-alert acknowledgement behavior
- queued notification reveal
- duplicate active-notification update without queue duplication
- expiration of non-required notifications
- alert-safe remote pet behavior
- non-empty portrait and landscape frames
- a visible cat region on every screen in every orientation

## Simulator and snapshots

```bash
./build/housecat_simulator previews/native
python3 tools/build_previews.py
```

The simulator writes exact logical 122×250 or 250×122 monochrome snapshots from the shared C++ renderer. The preview tool enlarges them with nearest-neighbor scaling and assembles the presentation montage without smoothing the one-bit art.

Snapshot review should verify:

- no clipped text or control hints
- icon and label pairing
- the cat remains dominant and recognizable
- orientation causes reflow, not simple bitmap rotation
- urgent alerts remain visually distinct
- black density is suitable for e-paper

## Configuration and protocol validation

Before packaging:

- parse all YAML files
- parse all SVG files as XML
- compile every Python tool
- regenerate assets and compare the packed header
- regenerate native snapshots
- run `tools/verify_repo.py`
- ensure no local secret, `.pio`, native build directory, or Python cache is included
- generate and verify `MANIFEST.sha256`

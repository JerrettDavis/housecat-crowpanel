# CrowPanel ESP32 2.13-inch Hardware Profile

## Qualified target definition

The PlatformIO project targets an Elecrow CrowPanel ESP32-S3 2.13-inch black/white e-paper board with:

- ESP32-S3 at 240 MHz
- 8 MB QIO flash at 80 MHz
- 8 MB OPI PSRAM
- 122×250 visible e-paper area
- CH340/CH34x USB-to-UART upload path
- five active-low physical controls
- UART and two exposed expansion GPIO pins

The custom PlatformIO manifest is [`boards/crowpanel_esp32s3_213.json`](../boards/crowpanel_esp32s3_213.json). It explicitly enables OPI PSRAM and disables native USB CDC because uploads and monitoring use the board's USB-to-UART bridge.

## Pin map used by the firmware

| Function | GPIO |
|---|---:|
| E-paper BUSY | 9 |
| E-paper RESET | 10 |
| E-paper MOSI | 11 |
| E-paper SCK | 12 |
| E-paper DC | 13 |
| E-paper CS | 14 |
| Display power | 7 |
| Power LED | 19 |
| Home/Menu | 2 |
| Back/Exit | 1 |
| Rocker Up | 6 |
| Rocker Down | 4 |
| Rocker Click | 5 |
| Expansion A | 40 |
| Expansion B | 41 |

Inputs use `INPUT_PULLUP` and are treated as active-low. The mapping follows the Elecrow factory source rather than a generic ESP32-S3 development-board assumption.

## Display profile

The target uses the JD79661 initialization, active-low BUSY handling, update command, and full-refresh waveforms from Elecrow's factory source. The controller RAM is 128 pixels wide while the visible surface is 122×250. House Cat intentionally models only that visible surface.

The framebuffer uses a 16-byte row stride:

```text
ceil(122 / 8) × 250 = 16 × 250 = 4,000 bytes
```

The CrowPanel adapter:

1. enables the panel power GPIO
2. drives the controller's SPI-compatible bus on the qualified CrowPanel pins
3. keeps physical panel addressing fixed
4. applies House Cat's logical orientation transform in the framebuffer
5. draws the packed 122×250 image
6. presents the frame with the qualified factory full-refresh waveform

Physical qualification confirmed the panel offset, inversion, waveform, and active-low BUSY polarity for the installed board.

## Refresh profile

Both firmware environments log measured duration and BUSY state. The shipping
`crowpanel_idf5` environment additionally enables the VPN client. The
application still distinguishes coalescible and immediate render requests for
responsive scheduling, but the adapter safely promotes both to the qualified
full JD79661 waveform.

## Input architecture

An e-paper refresh can occupy the Arduino loop long enough to miss quick presses. `CrowPanelInput` therefore runs a FreeRTOS task on core 0:

- samples every 10 ms
- debounces for 45 ms
- enqueues pressed-edge actions
- does not auto-repeat, preventing held controls from advancing behind a slow refresh
- buffers 12 actions
- logs queue or task allocation failure at boot

The main loop drains a bounded number of queued actions and coalesces their display requests. The input task continues sampling while the e-paper driver is busy.

## Flash partition plan

| Partition | Offset | Size | Purpose |
|---|---:|---:|---|
| NVS | `0x9000` | 20 KiB | framework data and House Cat state |
| OTA metadata | `0xE000` | 8 KiB | active application selection |
| App A | `0x10000` | 3,008 KiB | current or next firmware |
| App B | `0x300000` | 3,008 KiB | rollback firmware |
| LittleFS-compatible data | `0x5F0000` | 2 MiB | future content, memory, and asset packs |
| Core dump | `0x7F0000` | 64 KiB | crash diagnostics |

The table ends exactly at the 8 MB flash boundary. First-pass art remains compiled into firmware. The filesystem partition is reserved for later local content.

## Persistence

House Cat stores a versioned JSON snapshot in the ESP32 Preferences/NVS namespace `housecat`. The snapshot currently includes:

- cat name, level, XP, bond, and interaction count
- orientation and child-oriented display settings
- current mission and progress

Writes are delayed after state changes to coalesce repeated input and reduce unnecessary flash wear. `factory-reset` clears only the House Cat namespace.

## First hardware smoke-test order

1. Build and flash with no `include/secrets.h`.
2. Confirm ESP32-S3, 8 MB flash, and 8 MB PSRAM in serial diagnostics.
3. Confirm display power and one complete full frame.
4. Render the native Home screen on hardware.
5. Verify all five controls with the `buttons` command.
6. Press controls during refresh and confirm queued movement.
7. Compare normal and safe refresh profiles.
8. Test all four physical orientations.
9. Reboot after progression, mission, and orientation changes to confirm restore.
10. Enable Wi-Fi and MQTT only after local hardware behavior is stable.

Use [`FIRST_FLASH.md`](FIRST_FLASH.md) for commands and [`HARDWARE_ACCEPTANCE.md`](HARDWARE_ACCEPTANCE.md) to capture evidence.

## Upstream basis

- Elecrow product documentation and hardware repository
- Elecrow factory source for e-paper power, SPI, and button definitions
- PlatformIO Espressif32 board conventions for ESP32-S3 N8R8 targets
- Elecrow JD79661 factory initialization and waveform sequence

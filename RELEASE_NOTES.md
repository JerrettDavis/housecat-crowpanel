# House Cat `0.1.0-alpha.1`

This is the first flash-and-test PlatformIO release for the Elecrow CrowPanel ESP32-S3 2.13-inch 122×250 monochrome e-paper HMI.

## Included

- Custom N8R8 PlatformIO board definition: 8 MB QIO flash and 8 MB OPI PSRAM
- Normal and full-refresh-safe firmware environments
- Original one-bit cat sprites and UI iconography
- Four responsive orientations
- Five-button UX for Menu, Back, rocker Up, rocker Down, and rocker Click
- Persistent buddy, settings, progression, and mission state
- Offline serial hardware-test console
- Wi-Fi, MQTT, Home Assistant discovery, semantic notifications, weather, and missions
- Dual-OTA partition layout and LittleFS allocation
- Native simulator, behavioral tests, embedded compile-check stubs, CI, and flash-bundle exporter

## First physical qualification

Begin with `crowpanel_safe`, keep networking disabled, open the 115200-baud monitor, and follow `docs/FIRST_FLASH.md` plus `docs/HARDWARE_ACCEPTANCE.md`. The safe profile uses full e-paper refreshes so partial refresh is not a variable during initial bring-up.

## Validation boundary

The portable core, simulator, tests, repository structure, generated assets, YAML, diagrams, and every embedded translation unit have been validated in the packaging environment. A target ESP32-S3 binary is intentionally not included unless it is produced by PlatformIO from this exact repository. Physical display timing, panel polarity, button behavior, upload/reset behavior, Wi-Fi, MQTT, and power characteristics remain hardware acceptance gates.

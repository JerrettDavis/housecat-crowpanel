# House Cat Roadmap from First Pass to v1

## 0.1 — Physical proof

**Current repository.**

- flash and verify the selected e-paper profile
- tune partial/full refresh policy on the actual panel
- validate all five controls and buffered input
- verify NVS persistence and MQTT reconnect behavior
- photograph all four orientations in the intended enclosure

Exit: the shipped prototype feels deliberate, readable, and reliable on the desk.

## 0.2 — Provisioned connected cat

- captive portal or BLE provisioning
- editable cat/device name
- broker TLS option
- firmware update path with rollback
- battery and stale-data indicators backed by real measurements
- Home Assistant blueprint or custom integration setup wizard
- notification history and clear-condition messages

Exit: a non-developer can put it on Wi-Fi and connect it to Home Assistant.

## 0.3 — Living cat

- personality seed and hidden dimensions
- discovered traits
- memories and milestones
- time-of-day pose selection
- multiple dialogue variants by tone and personality
- progression providers with anti-farming rules
- accessories and visual unlocks assembled from layers

Exit: two cats with similar inputs feel meaningfully different.

## 0.4 — Whiskers

- [x] passive asynchronous Wi-Fi scan with SSID, RSSI, channel, and security views
- cooperative scan activity with connectivity warning
- BLE advertisements and signal-strength list
- known/unknown device memory
- channel and proximity views
- scan missions and Radio Cat trait
- strict passive tooling boundary; no disruption or exploitation features

Exit: Whiskers is useful, playful, and safe without pretending to be a Flipper clone.

## 0.5 — House Cat Extension Protocol

- CBOR message envelope
- framing, version negotiation, and capability advertisement
- UART transport first
- commands, queries, events, streams, status, and errors
- reference simulator and conformance tests
- RP2040 reference accessory

Exit: the firmware can discover a capability it was designed to understand without knowing the accessory model.

## 0.6 — Lab collar

- [x] safe read-only GPIO 40/41 inspector on the built-in expansion header
- GPIO, ADC, PWM, I²C, SPI, and UART capabilities through a secondary MCU
- external storage capability
- GPS/environmental sensor examples
- capability permissions and user-visible connection status
- mission providers backed by hardware observations

Exit: House Cat becomes the friendly UI and orchestration brain for external tools.

## 0.7 — Library

- [x] public-domain plain-text books downloaded from a Project Gutenberg mirror
- [x] local LittleFS storage, text pagination, catalog and offline reading
- text-card and article format
- font-size choice and page navigation
- bookmarks and reading missions
- Home Assistant/HTTP content handoff
- microSD provider through the extension protocol
- optional EPUB preprocessing off-device rather than full parsing on the ESP

Exit: Library is pleasant for short-form reading and reference, not a cramped Kindle imitation.

## 1.0 — Stable platform

- versioned persisted-state migrations
- versioned MQTT and extension contracts
- OTA signing and recovery path
- accessibility and localization pass
- enclosure and accessory electrical standard
- reproducible release builds
- physical endurance testing
- complete Home Assistant integration

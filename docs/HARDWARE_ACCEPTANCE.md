# House Cat Hardware Acceptance Checklist

Use one copy of this checklist per physical CrowPanel. Record measured values rather than marking a section complete from memory.

## Test identity

- [ ] Date:
- [ ] Tester:
- [ ] Repository version: `0.1.0-alpha.1`
- [ ] Commit or ZIP SHA-256:
- [ ] CrowPanel PCB revision:
- [ ] E-paper panel/controller marking:
- [ ] Host operating system:
- [ ] USB cable/adapter:
- [ ] Serial port:
- [ ] PlatformIO Core version:
- [ ] Environment: `crowpanel_idf5`

## Build and upload

- [ ] `python tools/verify_repo.py` passes.
- [ ] PlatformIO resolves the custom `crowpanel_esp32s3_213` board.
- [ ] Firmware compiles without errors.
- [ ] Firmware size fits the 0x2F0000 application partition.
- [ ] Upload completes at 921600 baud, or the working fallback baud is recorded.
- [ ] Board resets automatically after upload.

Notes:

```text

```

## Boot diagnostics

- [ ] Firmware reports `0.1.0-alpha.1`.
- [ ] Reset reason is sensible.
- [ ] Chip reports ESP32-S3.
- [ ] Flash reports approximately 8 MB.
- [ ] PSRAM reports available and approximately 8 MB total.
- [ ] No brownout, watchdog, panic, or boot loop occurs.
- [ ] All five buttons report released when untouched.

Recorded values:

```text
Flash bytes:
PSRAM bytes:
Free heap:
Free PSRAM:
Reset reason:
```

## E-paper display

- [ ] Display power enables.
- [ ] First full frame completes.
- [ ] Image is upright at orientation 0.
- [ ] Cat art is crisp and recognizable.
- [ ] Icons are crisp and paired with readable labels.
- [ ] No repeated columns, clipped rows, or horizontal offset is visible.
- [ ] White background is clean after a full refresh.
- [ ] Black areas are dense and stable.
- [ ] BUSY returns to the idle state after each update.
- [ ] Ten partial updates complete without corruption.
- [ ] Coalesced and immediate render scheduling both present correctly.

Measured refreshes:

```text
Full refresh 1:
Full refresh 2:
Partial refresh 1:
Partial refresh 2:
BUSY idle level:
```

Ghosting assessment:

```text
After 1 partial update:
After 5 partial updates:
After 10 partial updates:
After automatic full refresh:
```

## Orientation and layout

- [ ] 0 degrees reflows correctly.
- [ ] 90 degrees reflows correctly.
- [ ] 180 degrees reflows correctly.
- [ ] 270 degrees reflows correctly.
- [ ] Text remains readable in every orientation.
- [ ] The cat remains visible on every tested screen.
- [ ] Controls remain logically Up/Down, independent of physical orientation.

Notes:

```text

```

## Physical controls

- [ ] Home/Menu reports only GPIO2 pressed.
- [ ] Back/Exit reports only GPIO1 pressed.
- [ ] Rocker Up reports only GPIO6 pressed.
- [ ] Rocker Down reports only GPIO4 pressed.
- [ ] Rocker Click reports only GPIO5 pressed.
- [ ] Each single press produces one action.
- [ ] Mechanical bounce does not produce duplicate actions.
- [ ] Holding Up repeats after the initial delay.
- [ ] Holding Down repeats after the initial delay.
- [ ] Click, Home, and Back do not repeat while held.
- [ ] Presses made during an e-paper refresh are queued and applied.
- [ ] Input queue overflow is not observed during realistic use.

Notes:

```text

```

## Core UX

- [ ] Home starts with a prominent cat and one clear card.
- [ ] Up/Down rotate through weather, rooms, and mission cards.
- [ ] Rocker Click pets the cat on Home.
- [ ] Home/Menu opens the large carousel menu.
- [ ] Back retreats exactly one level.
- [ ] Every main menu choice has a large icon, title, and short subtitle.
- [ ] No screen becomes trapped without Home or Back recovery.
- [ ] A toddler can reach Home and pet the cat without reading instructions.

## Notifications

- [ ] `person` displays the person-arrival visual treatment.
- [ ] `car` displays the vehicle-complete visual treatment.
- [ ] Higher priority notification preempts a lower priority one.
- [ ] Lower priority notification remains queued.
- [ ] A required `alert` cannot be dismissed with Back.
- [ ] Rocker Click acknowledges a required alert.
- [ ] A queued notification appears after acknowledgement.
- [ ] A short-TTL notification expires and clears.
- [ ] Re-sending the same non-empty ID updates rather than duplicates it.
- [ ] The cat remains visible on notification screens.

## Persistence

- [ ] Pet reward survives reboot after the settle delay.
- [ ] Level, XP, bond, and interaction count survive reboot.
- [ ] Orientation survives reboot.
- [ ] Current mission survives reboot.
- [ ] Interrupted power during normal idle does not corrupt NVS state.
- [ ] `factory-reset` clears House Cat state and restores defaults.

## Wi-Fi and MQTT

- [ ] Offline build remains fully usable without credentials.
- [ ] Configured build associates with the expected 2.4 GHz network.
- [ ] Serial log reports SSID, IP, and RSSI without exposing the password.
- [ ] MQTT authenticates successfully.
- [ ] Availability publishes `online`.
- [ ] Broker disconnect causes LWT `offline`.
- [ ] Device reconnects after broker restart.
- [ ] Device reconnects after access point restart.
- [ ] Home snapshot updates the display.
- [ ] Mission message updates and persists.
- [ ] Transient notification is not retained.
- [ ] Invalid JSON is rejected without reboot or state corruption.

Recorded reconnect times:

```text
Wi-Fi reconnect:
MQTT reconnect:
Home Assistant restart rediscovery:
```

## Home Assistant

- [ ] One House Cat device is discovered.
- [ ] Level entity updates.
- [ ] Bond entity updates.
- [ ] Mood entity updates.
- [ ] Screen entity updates.
- [ ] Wi-Fi signal entity updates.
- [ ] Pet button performs the action safely.
- [ ] Discovery and state return after Home Assistant restart.
- [ ] Example package sends a home snapshot.
- [ ] Example arrival automation sends a notification.
- [ ] Example vehicle completion automation sends a notification.
- [ ] Example critical alert requires acknowledgement.

## Result

- [ ] PASS - normal profile is ready for continued feature work.
- [ ] CONDITIONAL PASS - safe full-refresh profile required.
- [ ] FAIL - hardware profile or firmware must change before feature work.

Blocking findings:

```text

```

Recommended changes:

```text

```

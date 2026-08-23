# House Cat First Flash Guide

This guide brings up the CrowPanel in the safest order: offline firmware first, physical display and controls second, then Wi-Fi, MQTT, and Home Assistant.

## 1. Prepare the workstation

Requirements:

- Windows 10/11, macOS, or Linux
- Python 3.10 or newer
- a USB data cable
- the CrowPanel connected directly to the workstation for the first flash
- internet access for the first PlatformIO dependency download

The board uses a USB-to-serial bridge. On Windows, install the WCH CH340/CH34x driver if the board does not appear as a COM port.

### Windows PowerShell

From the repository root:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\scripts\bootstrap.ps1
py tools\verify_repo.py
.\scripts\build.ps1
py -m platformio device list
```

### macOS or Linux

```bash
./scripts/bootstrap.sh
python3 tools/verify_repo.py
./scripts/build.sh
python3 -m platformio device list
```

The first build downloads the pinned ESP32 platform, Arduino framework, compiler, uploader, and libraries. Later builds reuse the PlatformIO cache.

## 2. Keep the first flash offline

Do not create `include/secrets.h` yet. Empty checked-in defaults leave the
complete local buddy and serial test console available. The captive portal can
provision Wi-Fi without recompiling.

This separates display or input problems from network problems.

## 3. Identify the serial port

Connect the CrowPanel and run:

```powershell
py -m platformio device list
```

Typical ports are:

- Windows: `COM5`
- Linux: `/dev/ttyUSB0`
- macOS: `/dev/cu.usbserial-*`

If no port appears:

1. Confirm the cable supports data, not only charging.
2. Try a direct USB port instead of a hub.
3. Install or reinstall the WCH CH340/CH34x driver.
4. Disconnect and reconnect the panel.
5. Check Device Manager or the operating system USB log.

## 4. Flash the normal firmware

### Windows

```powershell
.\scripts\flash.ps1 -Port COM5
.\scripts\monitor.ps1 -Port COM5
```

### macOS or Linux

```bash
./scripts/flash.sh /dev/ttyUSB0
./scripts/monitor.sh /dev/ttyUSB0
```

Replace the example port with the one reported by PlatformIO.

The default upload speed is 921600 baud. If an unstable cable or USB bridge causes packet errors, temporarily change `upload_speed` in `platformio.ini` to `460800` or `115200` and retry.

## 5. Confirm the first boot

At 115200 baud, the serial monitor should report:

- firmware version `0.1.0-alpha.1`
- ESP32-S3 target and reset reason
- 8 MB flash
- PSRAM detected and its available size
- the exact display and control pin map
- all five controls initially reported as released
- display initialization
- a measured full-refresh duration
- `House Cat offline test console` readiness

The display should finish on the House Cat Home screen with a clearly rendered cat, weather card, text, icons, and control hints. A full e-paper refresh takes several seconds and may visibly flash.

Run this in the serial monitor:

```text
status
buttons
```

`status` should show the active screen, orientation, buddy progression, mission, free heap, and PSRAM. `buttons` should show every released control as `released`.

## 6. Test the physical controls

Use the printed labels on the CrowPanel:

| Control | Expected behavior |
|---|---|
| Rocker Up | Previous Home card or previous menu item |
| Rocker Down | Next Home card or next menu item |
| Rocker Click | Pet on Home, or activate the visible choice |
| Home/Menu | Open the global menu; press again from another screen to return Home |
| Back/Exit | Return one level; do not dismiss required alerts |

While holding Rocker Up or Down, the selection should begin repeating after a short pause. Press several times during a display refresh. The input task should queue the actions and apply them after the panel becomes available.

Run `buttons` while holding each control. Only the held control should report `PRESSED`.

## 7. Exercise every offline scenario

Enter these commands one at a time in the serial monitor:

```text
help
status
person
car
alert
select
weather 93 72 sunny
mission 2 5 Find devices|Use Whiskers to discover nearby radios.
rotate 90
rotate 180
rotate 270
rotate 0
partial
full
```

Expected results:

- `person` shows a friendly arrival card with the cat still visible.
- `car` shows a higher-priority charging completion card.
- `alert` shows an acknowledgement-required warning. Back must not clear it; Rocker Click or `select` must acknowledge it.
- `weather` updates the Home weather values.
- `mission` updates mission text and progress.
- each `rotate` command produces a reflowed layout rather than a sideways portrait bitmap.
- `partial` and `full` print measured refresh durations.

## 8. Verify persistence

1. Run `pet` once.
2. Run `rotate 90`.
3. Run `mission 3 7 Test persistence|This should survive a reboot.`
4. Wait at least three seconds for the delayed NVS write.
5. Run `reboot`.
6. Run `status` after startup.

The orientation, mission, cat XP, bond, level, and interaction count should be restored.

To clear only House Cat's saved state:

```text
factory-reset
```

## 9. Confirm the qualified display profile

The `crowpanel_idf5` environment uses the full JD79661 waveform qualified on the
installed hardware. Confirm that repeated navigation remains stable, correctly
oriented, and left-to-right. If another panel batch behaves differently, record
its markings and BUSY timing before changing the driver constants.

## 10. Enable Wi-Fi and MQTT

After the offline acceptance checks pass, create the ignored local configuration file.

### Windows

```powershell
Copy-Item include\secrets.example.h include\secrets.h
notepad include\secrets.h
```

### macOS or Linux

```bash
cp include/secrets.example.h include/secrets.h
${EDITOR:-nano} include/secrets.h
```

Fill in:

```cpp
#define HOUSECAT_WIFI_SSID "YourWifi"
#define HOUSECAT_WIFI_PASSWORD "YourPassword"
#define HOUSECAT_TAILSCALE_EXIT_NODE_IP ""
#define HOUSECAT_MQTT_HOST "10.0.0.10"
#define HOUSECAT_MQTT_PORT 1883
#define HOUSECAT_MQTT_REMOTE_HOST ""
#define HOUSECAT_HOME_NETWORK_PREFIX ""
#define HOUSECAT_MQTT_REMOTE_IPV4 0UL
#define HOUSECAT_MQTT_USERNAME "housecat"
#define HOUSECAT_MQTT_PASSWORD "YourMqttPassword"
#define HOUSECAT_DEVICE_ID "housecat-desk-01"
#define HOUSECAT_DEVICE_NAME "House Cat"
#define HOUSECAT_CAT_NAME "Kitty"
```

Rebuild and flash. The serial monitor should show Wi-Fi association, IP address, RSSI, MQTT connection, and the base topic.

Real credentials are ignored by Git through `include/secrets.h`. Keep that file out of issue reports, ZIP files, and screenshots.

## 11. Test MQTT and Home Assistant

Use Home Assistant's MQTT integration publish tool or a broker client.

Notification topic:

```text
housecat/housecat-desk-01/command/notification
```

Payload:

```json
{
  "id": "sam-home-test",
  "title": "Sam is home!",
  "body": "Hoo-ray!",
  "kind": "person",
  "priority": "notice",
  "ttl_s": 180,
  "ack": false
}
```

Home snapshot topic:

```text
housecat/housecat-desk-01/command/home
```

Payload:

```json
{
  "outside_f": 93,
  "inside_f": 72,
  "condition": "sunny",
  "condition_label": "Sunny",
  "rooms": [
    {"name": "Office", "temperature_f": 70, "humidity": 39},
    {"name": "Living", "temperature_f": 73, "humidity": 42}
  ]
}
```

Confirm that Home Assistant discovers the House Cat device and its Level, Bond, Mood, Screen, Wi-Fi Signal, and Pet entities. Then install and customize `homeassistant/housecat_package.yaml` for outbound automations.

## 12. Save the test evidence

Complete [`HARDWARE_ACCEPTANCE.md`](HARDWARE_ACCEPTANCE.md) with:

- board and panel markings
- host operating system
- COM/serial port
- `crowpanel_idf5` environment result
- full refresh duration
- ghosting observations
- button results
- flash and PSRAM readings
- Wi-Fi and MQTT reconnect results
- Home Assistant discovery results

That record confirms the qualified direct JD79661 profile against the installed panel batch.

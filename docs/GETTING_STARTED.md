# Getting started

This is the shortest path from a clean checkout to a working House Cat.

## What you need

- Elecrow CrowPanel ESP32-S3 2.13-inch 122×250 e-paper (N8R8)
- USB data cable and Python 3.10+
- Wi-Fi with 2.4 GHz enabled
- optional MQTT broker/Home Assistant and optional Tailscale account

## Build and flash

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\scripts\bootstrap.ps1
py tools\verify_repo.py
.\scripts\build.ps1 crowpanel_idf5
py -m platformio device list
.\scripts\flash.ps1 -Port COM5 -Environment crowpanel_idf5
.\scripts\monitor.ps1 -Port COM5
```

On macOS/Linux, use the matching `.sh` scripts and `/dev/ttyUSB0` (or the
port shown by PlatformIO). The comprehensive hardware procedure is in
[FIRST_FLASH.md](FIRST_FLASH.md).

## Provision networking

With no saved network, the panel opens `HouseCat-Setup-XXXXXX`. Join it with
the password displayed on the panel and visit `http://192.168.4.1/`. Enter
Wi-Fi credentials and, optionally, a one-use pre-authorized Tailscale auth key.
The portal closes after saving. Hold **Menu + Back** during boot to reopen it.

For compile-time provisioning, copy `include/secrets.example.h` to the ignored
`include/secrets.h`. Never commit or share that local file.

## Connect Home Assistant

Enable Home Assistant's MQTT integration, give the panel a least-privilege
broker account, then install `homeassistant/housecat_package.yaml`. Restart
Home Assistant and confirm the discovered House Cat device and availability
entity. See [homeassistant/README.md](../homeassistant/README.md) and the
[MQTT contract](MQTT_CONTRACT.md).

## Confirm success

- Home, Rooms, Mission, My Day, Library, Playground, and Settings navigate.
- Wi-Fi and MQTT report connected in diagnostics.
- Room temperature/humidity and weather update from Home Assistant.
- After changing to another Wi-Fi network, the panel reconnects and its
  configured VPN path can still reach only the permitted remote MQTT endpoint.

Continue with the [User Guide](USER_GUIDE.md), or use
[Troubleshooting](TROUBLESHOOTING.md) if any check fails.

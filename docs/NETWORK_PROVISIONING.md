# Network provisioning and Tailscale

House Cat normally joins its saved Wi-Fi network at boot. If it cannot connect
for 30 seconds, it starts a WPA2 setup access point named
`HouseCat-Setup-XXXXXX`. The panel displays the access-point name, password,
and setup address.

1. Join the setup access point using the password shown on the panel.
2. Open `http://192.168.4.1/`. Common captive-portal probes are redirected
   there automatically.
3. Enter the target Wi-Fi SSID and password. Leaving the password blank while
   keeping the same SSID preserves the stored password.
4. Optionally enter a one-use, pre-authorized Tailscale auth key beginning with
   `tskey-auth-`.
5. Choose **Save and connect**. Credentials are stored in ESP32 NVS and are
   never printed, rendered, or published over MQTT.

To open the portal while Wi-Fi is working, hold **Menu + Back** during boot.
The access point remains available for up to ten minutes or until settings are
saved. A factory reset clears Wi-Fi and Tailscale enrollment data as well as the
normal House Cat preferences.

## Tailscale behavior

The shipping `crowpanel_idf5` environment uses Arduino 3.3.9 / ESP-IDF 5.5.4
and a pinned MicroLink revision. Once Wi-Fi and an auth key are available, the
client enrolls as `housecat-desk-01`, persists its machine keys, and reconnects
to the tailnet after reboot without reusing the enrollment key. Home Assistant
discovery exposes diagnostic `Tailscale state` and `Tailscale IP` sensors.

Use a one-use, pre-authorized key. Do not put an auth key in source control;
the `HOUSECAT_TAILSCALE_AUTH_KEY` compile-time option is intended only for
controlled bring-up. The setup portal is HTTP inside its dedicated WPA2 access
point, so configure it only while physically near the device and close it after
use.

MicroLink is a third-party ESP32 implementation rather than an official
Tailscale client. Its source is fetched at the revision pinned in
`tools/pio_microlink.py` and compiled into the firmware.

To route public internet traffic through a tailnet exit node, set its Tailscale
IPv4 address locally as `HOUSECAT_TAILSCALE_EXIT_NODE_IP`. To reach a private
MQTT broker while away, set `HOUSECAT_MQTT_REMOTE_HOST` and the same IPv4 value
encoded as `HOUSECAT_MQTT_REMOTE_IPV4` (for `10.0.0.10`, `0x0A00000AUL`). The
`HOUSECAT_HOME_NETWORK_PREFIX` value (for example `"192.168.50."`) selects the
local broker only when the panel has a home-LAN address; elsewhere it selects
the remote broker. The
VPN policy permits only TCP to `HOUSECAT_MQTT_PORT` at that one address and
rejects all other RFC1918/link-local destinations. This defense complements,
but does not replace, IoT-VLAN firewall rules that deny IoT-to-main initiation.

## Build and recovery

```powershell
python -m platformio run -e crowpanel_idf5
python -m platformio run -e crowpanel_idf5 -t upload --upload-port COM5
```

The legacy, non-Tailscale rollback image remains buildable with:

```powershell
python -m platformio run -e crowpanel
```

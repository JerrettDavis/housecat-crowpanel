# Troubleshooting

## Display is stale, inverted, mirrored, or scrambled

Build `crowpanel_idf5`, power-cycle once after flashing, and allow the full
e-paper waveform to finish. Do not switch driver polarity, scan direction, or
partial-refresh constants for this qualified panel profile. Run `status` over
115200-baud serial and record the panel markings if the issue persists.

## Controls flash the next screen then return

Use current firmware and let each refresh finish. Inputs are queued while the
panel is busy. Check `buttons` in the serial console for a stuck active-low
control, then use `factory-reset` only if persisted navigation state is corrupt.

## Wi-Fi shows N/A

Wait 30 seconds for `HouseCat-Setup-XXXXXX`, join it, and open
`http://192.168.4.1/`. Hold Menu + Back during boot to force the portal. The
CrowPanel uses 2.4 GHz Wi-Fi; WPA enterprise networks are unsupported.

## MQTT or room sensors are offline

Confirm Wi-Fi first, then broker host, port, username, ACLs, and Home Assistant
MQTT integration. The device needs publish/subscribe access only to its own
`housecat/<device-id>/...` topics. Verify the entity IDs in
`homeassistant/housecat_package.yaml` and inspect the retained home snapshot.

## Tailscale will not enroll away from home

Use a fresh one-use pre-authorized auth key, ensure device time has synchronized,
and check serial states (`time-sync`, `registering`, `connected`, or `error`).
An exit node is optional; if configured, use its Tailscale IPv4 address. The
remote MQTT endpoint must also be explicitly allowlisted as documented in
[NETWORK_PROVISIONING.md](NETWORK_PROVISIONING.md).

## Build or upload fails

Run `python tools/verify_repo.py`, reinstall `requirements-platformio.txt`, and
try a direct USB data cable. Lower upload speed if writes are unreliable. See
[FIRST_FLASH.md](FIRST_FLASH.md) for port and driver diagnostics.

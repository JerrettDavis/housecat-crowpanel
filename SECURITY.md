# Security policy

## Reporting a vulnerability

Please use GitHub's **Security > Report a vulnerability** private-reporting
feature. Do not open a public issue with credentials, network details, or an
unpatched exploit. Include affected versions, impact, reproduction steps, and
a proposed mitigation when possible. Maintainers should acknowledge a report
within seven days and coordinate disclosure after a fix is available.

Only the newest release receives security fixes.

## Device security model

House Cat keeps credentials in ESP32 NVS and never intentionally renders,
logs, or publishes them. Its setup portal is a temporary WPA2 access point and
uses HTTP, so provision only while physically near the device. Use a dedicated
IoT VLAN, a least-privilege MQTT account, and a one-use Tailscale enrollment
key. The VPN patch blocks private IPv4 destinations except the single MQTT
endpoint explicitly configured by the owner.

Local secrets belong only in ignored `include/secrets.h`; release archives
exclude that file. CI scans commits with Gitleaks. Before publishing an
existing repository, scan its entire original Git history—not only its current
files—and revoke every credential ever committed.

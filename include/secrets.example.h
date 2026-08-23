#pragma once

// Safe checked-in defaults. Fill these values locally or inject equivalent
// -D build flags. Empty Wi-Fi credentials leave House Cat fully usable offline.
#define HOUSECAT_WIFI_SSID ""
#define HOUSECAT_WIFI_PASSWORD ""
#define HOUSECAT_SETUP_PASSWORD "housecat-setup"
#define HOUSECAT_TAILSCALE_AUTH_KEY ""
// Optional Tailscale IPv4 address of an exit node, for example "100.64.0.10".
#define HOUSECAT_TAILSCALE_EXIT_NODE_IP ""
#define HOUSECAT_MQTT_HOST ""
#define HOUSECAT_MQTT_PORT 1883
#define HOUSECAT_MQTT_REMOTE_HOST ""
// Prefix of the panel's home LAN address; when absent, the remote host wins.
#define HOUSECAT_HOME_NETWORK_PREFIX ""
// Same address as HOUSECAT_MQTT_REMOTE_HOST encoded as 0xAABBCCDDUL.
// It permits only TCP/1883 to that private address through the VPN.
#define HOUSECAT_MQTT_REMOTE_IPV4 0UL
#define HOUSECAT_MQTT_USERNAME ""
#define HOUSECAT_MQTT_PASSWORD ""
#define HOUSECAT_DEVICE_ID "housecat-desk-01"
#define HOUSECAT_DEVICE_NAME "House Cat"
#define HOUSECAT_CAT_NAME "Kitty"

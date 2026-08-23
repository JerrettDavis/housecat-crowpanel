#pragma once

#include <cstdint>

#if __has_include("secrets.h")
#include "secrets.h"
#endif

#ifndef HOUSECAT_WIFI_SSID
#define HOUSECAT_WIFI_SSID ""
#endif
#ifndef HOUSECAT_WIFI_PASSWORD
#define HOUSECAT_WIFI_PASSWORD ""
#endif
#ifndef HOUSECAT_SETUP_PASSWORD
#define HOUSECAT_SETUP_PASSWORD "housecat-setup"
#endif
#ifndef HOUSECAT_TAILSCALE_AUTH_KEY
#define HOUSECAT_TAILSCALE_AUTH_KEY ""
#endif
#ifndef HOUSECAT_TAILSCALE_EXIT_NODE_IP
#define HOUSECAT_TAILSCALE_EXIT_NODE_IP ""
#endif
#ifndef HOUSECAT_MQTT_HOST
#define HOUSECAT_MQTT_HOST ""
#endif
#ifndef HOUSECAT_MQTT_PORT
#define HOUSECAT_MQTT_PORT 1883
#endif
#ifndef HOUSECAT_MQTT_REMOTE_HOST
#define HOUSECAT_MQTT_REMOTE_HOST ""
#endif
#ifndef HOUSECAT_HOME_NETWORK_PREFIX
#define HOUSECAT_HOME_NETWORK_PREFIX ""
#endif
#ifndef HOUSECAT_MQTT_USERNAME
#define HOUSECAT_MQTT_USERNAME ""
#endif
#ifndef HOUSECAT_MQTT_PASSWORD
#define HOUSECAT_MQTT_PASSWORD ""
#endif
#ifndef HOUSECAT_DEVICE_ID
#define HOUSECAT_DEVICE_ID "housecat-desk-01"
#endif
#ifndef HOUSECAT_DEVICE_NAME
#define HOUSECAT_DEVICE_NAME "House Cat"
#endif
#ifndef HOUSECAT_CAT_NAME
#define HOUSECAT_CAT_NAME "Kitty"
#endif

namespace housecat::config {

inline constexpr const char* kFirmwareVersion = "0.1.0-alpha.1";
inline constexpr const char* kWifiSsid = HOUSECAT_WIFI_SSID;
inline constexpr const char* kWifiPassword = HOUSECAT_WIFI_PASSWORD;
inline constexpr const char* kSetupPassword = HOUSECAT_SETUP_PASSWORD;
inline constexpr const char* kTailscaleAuthKey = HOUSECAT_TAILSCALE_AUTH_KEY;
inline constexpr const char* kTailscaleExitNodeIp = HOUSECAT_TAILSCALE_EXIT_NODE_IP;
inline constexpr const char* kMqttHost = HOUSECAT_MQTT_HOST;
inline constexpr std::uint16_t kMqttPort = HOUSECAT_MQTT_PORT;
inline constexpr const char* kMqttRemoteHost = HOUSECAT_MQTT_REMOTE_HOST;
inline constexpr const char* kHomeNetworkPrefix = HOUSECAT_HOME_NETWORK_PREFIX;
inline constexpr const char* kMqttUsername = HOUSECAT_MQTT_USERNAME;
inline constexpr const char* kMqttPassword = HOUSECAT_MQTT_PASSWORD;
inline constexpr const char* kDeviceId = HOUSECAT_DEVICE_ID;
inline constexpr const char* kDeviceName = HOUSECAT_DEVICE_NAME;
inline constexpr const char* kDefaultCatName = HOUSECAT_CAT_NAME;

inline constexpr std::uint64_t kRenderSettleMs = 150;
inline constexpr std::uint64_t kPersistenceSettleMs = 2500;
inline constexpr std::uint64_t kMqttReconnectMs = 5000;
inline constexpr std::uint64_t kStatePublishMs = 60000;

}  // namespace housecat::config

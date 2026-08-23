#pragma once

// C-compatible network policy used by the patched VPN transport. Keep private
// destinations denied unless a single MQTT endpoint is explicitly configured.
#if __has_include("secrets.h")
#include "secrets.h"
#endif

#ifndef HOUSECAT_MQTT_REMOTE_IPV4
#define HOUSECAT_MQTT_REMOTE_IPV4 0UL
#endif

#ifndef HOUSECAT_MQTT_PORT
#define HOUSECAT_MQTT_PORT 1883
#endif

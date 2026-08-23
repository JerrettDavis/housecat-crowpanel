#pragma once

#include "Arduino.h"

class WiFiClass {
public:
    [[nodiscard]] int status() const { return 0; }
    [[nodiscard]] int RSSI() const { return -60; }
    void mode(int) {}
    void setSleep(bool) {}
    bool setHostname(const char*) { return true; }
    void begin(const char*, const char*) {}
    void disconnect(bool = false, bool = false) {}
    bool softAP(const char*, const char*) { return true; }
    bool softAPdisconnect(bool = false) { return true; }
    bool reconnect() { return true; }
    [[nodiscard]] String SSID() const { return "stub"; }
    [[nodiscard]] String SSID(int) const { return "stub"; }
    [[nodiscard]] int RSSI(int) const { return -60; }
    [[nodiscard]] int channel(int) const { return 1; }
    [[nodiscard]] int encryptionType(int) const { return 0; }
    int scanNetworks(bool, bool, bool = false, unsigned long = 300) { return -1; }
    int scanComplete() const { return -2; }
    void scanDelete() {}
    [[nodiscard]] IPAddress localIP() const { return {}; }
    [[nodiscard]] IPAddress softAPIP() const { return {}; }
};
inline WiFiClass WiFi{};

inline constexpr int WIFI_SCAN_RUNNING = -1;
inline constexpr int WIFI_SCAN_FAILED = -2;
inline constexpr int WIFI_AUTH_OPEN = 0;

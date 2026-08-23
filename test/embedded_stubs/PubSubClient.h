#pragma once

#include <cstdint>
#include <functional>

#include "WiFiClient.h"

class PubSubClient {
public:
    explicit PubSubClient(WiFiClient&) {}
    void setServer(const char*, std::uint16_t) {}
    bool setBufferSize(std::uint16_t) { return true; }
    PubSubClient& setKeepAlive(std::uint16_t) { return *this; }

    template <typename Callback>
    PubSubClient& setCallback(Callback&&) { return *this; }

    [[nodiscard]] bool connected() const { return false; }
    [[nodiscard]] int state() const { return -1; }
    bool connect(const char*, const char*, std::uint8_t, bool, const char*) { return true; }
    bool connect(const char*, const char*, const char*, const char*, std::uint8_t, bool, const char*) { return true; }
    bool publish(const char*, const char*, bool = false) { return true; }
    bool subscribe(const char*) { return true; }
    bool loop() { return true; }
};

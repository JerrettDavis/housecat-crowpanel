#pragma once
#include <functional>
#include "Arduino.h"
inline constexpr int HTTP_GET = 0;
inline constexpr int HTTP_POST = 1;
class WebServer {
public:
    explicit WebServer(int) {}
    template <typename Handler> void on(const char*, int, Handler) {}
    template <typename Handler> void onNotFound(Handler) {}
    void begin() {}
    void stop() {}
    void handleClient() {}
    void send(int, const char*, const char*) {}
    void sendHeader(const char*, const char*, bool = false) {}
    [[nodiscard]] String arg(const char*) const { return {}; }
};

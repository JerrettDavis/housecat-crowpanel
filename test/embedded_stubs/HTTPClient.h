#pragma once

#include "WiFiClient.h"
#include "WiFiClientSecure.h"

inline constexpr int HTTP_CODE_OK = 200;

class HTTPClient {
public:
    void setUserAgent(const char*) {}
    void setTimeout(int) {}
    bool begin(WiFiClientSecure&, const char*) { return true; }
    int GET() { return HTTP_CODE_OK; }
    int getSize() const { return 0; }
    bool connected() const { return false; }
    WiFiClient* getStreamPtr() { return &client_; }
    void end() {}

private:
    WiFiClient client_{};
};

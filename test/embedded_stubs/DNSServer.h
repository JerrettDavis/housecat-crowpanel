#pragma once
#include "Arduino.h"
class DNSServer {
public:
    void start(unsigned short, const char*, IPAddress) {}
    void processNextRequest() {}
    void stop() {}
};

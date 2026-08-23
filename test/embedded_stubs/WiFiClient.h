#pragma once

#include <cstddef>
#include <cstdint>

class WiFiClient {
public:
    std::size_t available() const { return 0; }
    std::size_t readBytes(std::uint8_t*, std::size_t) { return 0; }
};

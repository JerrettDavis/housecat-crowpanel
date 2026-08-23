#pragma once

class SPIClass {
public:
    void begin(int, int, int, int) {}
};
inline SPIClass SPI{};

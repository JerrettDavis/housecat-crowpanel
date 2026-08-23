#pragma once

#include <cstddef>
#include <cstdint>

#include "Arduino.h"

inline constexpr const char* FILE_READ = "r";
inline constexpr const char* FILE_WRITE = "w";

class File {
public:
    explicit operator bool() const { return true; }
    std::size_t size() const { return 0; }
    std::size_t position() const { return 0; }
    int available() const { return 0; }
    int read() { return -1; }
    bool seek(std::uint32_t) { return true; }
    std::size_t write(const std::uint8_t*, std::size_t count) { return count; }
    String readStringUntil(char) { return {}; }
    void close() {}
};

class LittleFSClass {
public:
    bool begin(bool, const char* = "/littlefs", std::uint8_t = 10, const char* = "littlefs") { return true; }
    File open(const char*, const char*) { return {}; }
    bool exists(const char*) const { return true; }
    bool remove(const char*) { return true; }
    bool rename(const char*, const char*) { return true; }
};

inline LittleFSClass LittleFS{};

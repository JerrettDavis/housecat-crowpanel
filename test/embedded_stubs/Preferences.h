#pragma once

#include <cstddef>

#include "Arduino.h"

class Preferences {
public:
    bool begin(const char*, bool = false) { return true; }
    [[nodiscard]] String getString(const char*, const char* fallback = "") const { return fallback; }
    [[nodiscard]] std::size_t putString(const char*, const String& value) { return value.length(); }
    bool clear() { return true; }
    void end() {}
};

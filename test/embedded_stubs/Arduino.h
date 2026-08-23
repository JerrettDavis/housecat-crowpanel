#pragma once

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

inline constexpr int LOW = 0;
inline constexpr int HIGH = 1;
inline constexpr int INPUT_PULLUP = 2;
inline constexpr int OUTPUT = 3;
inline constexpr int INPUT = 4;
inline constexpr int WIFI_STA = 1;
inline constexpr int WIFI_AP_STA = 3;
inline constexpr int WL_CONNECTED = 3;

inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
inline int digitalRead(int) { return HIGH; }
inline void delay(unsigned long) {}
inline unsigned long millis() { return 0; }

class String {
public:
    String() = default;
    String(const char* value) : value_(value == nullptr ? "" : value) {}
    String(const std::string& value) : value_(value) {}
    String(std::string&& value) : value_(std::move(value)) {}

    String& operator=(const char* value) {
        value_ = value == nullptr ? "" : value;
        return *this;
    }
    String& operator+=(char value) {
        value_ += value;
        return *this;
    }

    [[nodiscard]] const char* c_str() const noexcept { return value_.c_str(); }
    [[nodiscard]] char operator[](std::size_t index) const { return value_[index]; }
    [[nodiscard]] std::size_t length() const noexcept { return value_.length(); }
    bool reserve(std::size_t size) {
        value_.reserve(size);
        return true;
    }
    void trim() {
        const auto first = value_.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            value_.clear();
            return;
        }
        const auto last = value_.find_last_not_of(" \t\r\n");
        value_ = value_.substr(first, last - first + 1);
    }
    void toLowerCase() {
        std::transform(value_.begin(), value_.end(), value_.begin(), [](unsigned char value) {
            return static_cast<char>(std::tolower(value));
        });
    }
    [[nodiscard]] int indexOf(char value, unsigned int from = 0) const {
        const auto found = value_.find(value, from);
        return found == std::string::npos ? -1 : static_cast<int>(found);
    }
    [[nodiscard]] String substring(unsigned int from) const {
        return from >= value_.size() ? String{} : String{value_.substr(from)};
    }
    [[nodiscard]] String substring(unsigned int from, unsigned int to) const {
        if (from >= value_.size() || to <= from) return {};
        return String{value_.substr(from, to - from)};
    }
    void remove(unsigned int index) {
        if (index < value_.size()) value_.erase(index);
    }
    void remove(unsigned int index, unsigned int count) {
        if (index < value_.size()) value_.erase(index, count);
    }
    [[nodiscard]] long toInt() const { return std::strtol(value_.c_str(), nullptr, 10); }
    [[nodiscard]] bool startsWith(const char* prefix) const {
        return prefix != nullptr && value_.rfind(prefix, 0) == 0;
    }

    friend bool operator==(const String& left, const String& right) { return left.value_ == right.value_; }
    friend bool operator==(const String& left, const char* right) { return left.value_ == (right == nullptr ? "" : right); }
    friend bool operator==(const char* left, const String& right) { return right == left; }
    friend bool operator!=(const String& left, const char* right) { return !(left == right); }

private:
    std::string value_{};
};

class IPAddress {
public:
    [[nodiscard]] String toString() const { return "0.0.0.0"; }
};

class SerialClass {
public:
    void begin(unsigned long) {}
    [[nodiscard]] int available() const { return 0; }
    [[nodiscard]] int read() { return -1; }
    void flush() {}
    void print(const char*) {}
    void print(const String&) {}
    void println() {}
    void println(const char*) {}
    void println(const String&) {}
    int printf(const char*, ...) { return 0; }
};
inline SerialClass Serial{};

class ESPClass {
public:
    [[nodiscard]] const char* getChipModel() const { return "ESP32-S3"; }
    [[nodiscard]] std::uint8_t getChipRevision() const { return 0; }
    [[nodiscard]] std::uint8_t getChipCores() const { return 2; }
    [[nodiscard]] std::uint32_t getCpuFreqMHz() const { return 240; }
    [[nodiscard]] const char* getSdkVersion() const { return "stub"; }
    [[nodiscard]] std::uint32_t getFlashChipSize() const { return 8U * 1024U * 1024U; }
    [[nodiscard]] std::uint32_t getFlashChipSpeed() const { return 80U * 1000U * 1000U; }
    [[nodiscard]] std::uint32_t getHeapSize() const { return 327680; }
    [[nodiscard]] std::uint32_t getFreeHeap() const { return 200000; }
    [[nodiscard]] std::uint32_t getMinFreeHeap() const { return 190000; }
    [[nodiscard]] std::uint32_t getPsramSize() const { return 8U * 1024U * 1024U; }
    [[nodiscard]] std::uint32_t getFreePsram() const { return 7U * 1024U * 1024U; }
    [[nodiscard]] std::uint64_t getEfuseMac() const { return 0xD405927BCAA0ULL; }
    [[noreturn]] void restart() const { std::abort(); }
};
inline ESPClass ESP{};

inline bool psramFound() { return true; }

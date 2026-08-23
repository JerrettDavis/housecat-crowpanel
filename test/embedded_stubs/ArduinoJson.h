#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>

#include "Arduino.h"

class JsonVariant;

class JsonArrayConst {
public:
    class iterator {
    public:
        explicit iterator(bool end) : end_(end) {}
        JsonVariant operator*() const;
        iterator& operator++() { end_ = true; return *this; }
        friend bool operator!=(const iterator& left, const iterator& right) { return left.end_ != right.end_; }
    private:
        bool end_;
    };

    [[nodiscard]] iterator begin() const { return iterator{true}; }
    [[nodiscard]] iterator end() const { return iterator{true}; }
};

class JsonVariant {
public:
    JsonVariant operator[](const char*) const { return {}; }

    template <typename T>
    JsonVariant& operator=(T&&) { return *this; }

    [[nodiscard]] const char* operator|(const char* fallback) const { return fallback; }

    template <std::size_t N>
    [[nodiscard]] const char* operator|(const char (&fallback)[N]) const { return fallback; }

    template <typename T, typename = std::enable_if_t<!std::is_array_v<T> && !std::is_pointer_v<T>>>
    [[nodiscard]] T operator|(T fallback) const { return fallback; }

    explicit operator const char*() const { return ""; }

    template <typename T>
    [[nodiscard]] bool is() const { return false; }

    template <typename T>
    [[nodiscard]] T as() const { return T{}; }

    template <typename T>
    [[nodiscard]] T to() const { return T{}; }
};

inline JsonVariant JsonArrayConst::iterator::operator*() const { return {}; }

using JsonObject = JsonVariant;
using JsonObjectConst = JsonVariant;

class JsonDocument : public JsonVariant {};

class DeserializationError {
public:
    enum Code { Ok };
    constexpr DeserializationError(Code code = Ok) : code_(code) {}
    friend constexpr bool operator!=(DeserializationError left, Code right) { return left.code_ != right; }
    friend constexpr bool operator==(DeserializationError left, Code right) { return left.code_ == right; }
private:
    Code code_;
};

template <typename Input>
DeserializationError deserializeJson(JsonDocument&, const Input&) { return {}; }

inline std::size_t serializeJson(const JsonDocument&, String& output) {
    output = "{}";
    return output.length();
}

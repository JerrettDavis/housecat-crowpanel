#pragma once

#include <cstdint>

using BaseType_t = int;
using UBaseType_t = unsigned int;
using TickType_t = std::uint32_t;
using QueueHandle_t = void*;
using TaskHandle_t = void*;

inline constexpr BaseType_t pdTRUE = 1;
inline constexpr BaseType_t pdPASS = pdTRUE;
inline constexpr BaseType_t pdFALSE = 0;
inline constexpr TickType_t portMAX_DELAY = 0xFFFFFFFFU;

#define pdMS_TO_TICKS(ms) static_cast<TickType_t>(ms)

#pragma once

#include <cstdint>

#include "freertos/FreeRTOS.h"

using TaskFunction_t = void (*)(void*);

inline BaseType_t xTaskCreatePinnedToCore(
    TaskFunction_t,
    const char*,
    std::uint32_t,
    void*,
    UBaseType_t,
    TaskHandle_t*,
    BaseType_t) {
    return pdTRUE;
}

inline void vTaskDelay(TickType_t) {}

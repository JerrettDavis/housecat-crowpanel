#pragma once

#include <cstddef>

#include "freertos/FreeRTOS.h"

inline QueueHandle_t xQueueCreate(UBaseType_t, UBaseType_t) { return reinterpret_cast<void*>(1); }
inline BaseType_t xQueueReceive(QueueHandle_t, void*, TickType_t) { return pdFALSE; }
inline BaseType_t xQueueSend(QueueHandle_t, const void*, TickType_t) { return pdTRUE; }

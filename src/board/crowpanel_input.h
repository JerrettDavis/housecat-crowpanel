#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "housecat/app/housecat_app.h"

namespace housecat::board {

class CrowPanelInput final {
public:
    CrowPanelInput() = default;
    ~CrowPanelInput() = default;
    CrowPanelInput(const CrowPanelInput&) = delete;
    CrowPanelInput& operator=(const CrowPanelInput&) = delete;

    void begin();
    [[nodiscard]] bool next(InputAction& action) const;

private:
    struct Button {
        std::uint8_t pin;
        InputAction action;
        bool repeatable;
        bool rawPressed{false};
        bool stablePressed{false};
        std::uint64_t rawChangedAt{0};
        std::uint64_t nextRepeatAt{0};
    };

    static void taskEntry(void* context);
    void run();
    void sample(std::uint64_t nowMs);
    void emit(InputAction action) const;

    static constexpr std::uint64_t kDebounceMs = 45;
    static constexpr std::uint64_t kFirstRepeatMs = 520;
    static constexpr std::uint64_t kRepeatMs = 170;

    std::array<Button, 5> buttons_{};
    QueueHandle_t queue_{nullptr};
    TaskHandle_t task_{nullptr};
};

}  // namespace housecat::board

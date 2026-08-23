#include "board/crowpanel_input.h"

#include <Arduino.h>
#include <esp_timer.h>

#include "board/crowpanel_pins.h"

namespace housecat::board {
namespace {

std::uint64_t nowMs() noexcept {
    return static_cast<std::uint64_t>(esp_timer_get_time()) / 1000ULL;
}

}  // namespace

void CrowPanelInput::begin() {
    buttons_ = {{
        {pins::kMenu, InputAction::Menu, false},
        {pins::kBack, InputAction::Back, false},
        // A physical refresh takes roughly 725 ms. Auto-repeat used to queue
        // several moves while the display was busy, so the requested screen
        // appeared briefly and was immediately replaced by a later move.
        {pins::kRockerUp, InputAction::Up, false},
        {pins::kRockerDown, InputAction::Down, false},
        {pins::kRockerClick, InputAction::Select, false},
    }};

    for (const auto& button : buttons_) {
        pinMode(button.pin, INPUT_PULLUP);
    }

    queue_ = xQueueCreate(12, sizeof(InputAction));
    if (queue_ == nullptr) {
        Serial.println("[input] ERROR: failed to allocate the input queue");
        return;
    }

    const BaseType_t taskCreated = xTaskCreatePinnedToCore(
        taskEntry,
        "housecat-input",
        3072,
        this,
        2,
        &task_,
        0);
    if (taskCreated != pdPASS) {
        task_ = nullptr;
        Serial.println("[input] ERROR: failed to start the input sampling task");
        return;
    }
    Serial.println("[input] five active-low controls ready on core 0");
}

bool CrowPanelInput::next(InputAction& action) const {
    return queue_ != nullptr && xQueueReceive(queue_, &action, 0) == pdTRUE;
}

void CrowPanelInput::taskEntry(void* context) {
    static_cast<CrowPanelInput*>(context)->run();
}

void CrowPanelInput::run() {
    while (true) {
        sample(nowMs());
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void CrowPanelInput::sample(const std::uint64_t currentMs) {
    for (auto& button : buttons_) {
        const bool pressed = digitalRead(button.pin) == LOW;
        if (pressed != button.rawPressed) {
            button.rawPressed = pressed;
            button.rawChangedAt = currentMs;
        }

        if (button.rawPressed != button.stablePressed && currentMs - button.rawChangedAt >= kDebounceMs) {
            button.stablePressed = button.rawPressed;
            if (button.stablePressed) {
                emit(button.action);
                button.nextRepeatAt = currentMs + kFirstRepeatMs;
            }
        }

        if (button.stablePressed && button.repeatable && currentMs >= button.nextRepeatAt) {
            emit(button.action);
            button.nextRepeatAt = currentMs + kRepeatMs;
        }
    }
}

void CrowPanelInput::emit(const InputAction action) const {
    if (queue_ == nullptr) {
        return;
    }
    (void)xQueueSend(queue_, &action, 0);
}

}  // namespace housecat::board

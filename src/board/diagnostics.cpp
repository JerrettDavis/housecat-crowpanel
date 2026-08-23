#include "board/diagnostics.h"

#include <Arduino.h>
#include <esp_system.h>

#include "board/crowpanel_pins.h"
#include "housecat/config.h"

namespace housecat::board {
namespace {

const char* resetReasonName(const esp_reset_reason_t reason) noexcept {
    switch (reason) {
        case ESP_RST_POWERON: return "power-on";
        case ESP_RST_EXT: return "external-pin";
        case ESP_RST_SW: return "software";
        case ESP_RST_PANIC: return "panic";
        case ESP_RST_INT_WDT: return "interrupt-watchdog";
        case ESP_RST_TASK_WDT: return "task-watchdog";
        case ESP_RST_WDT: return "other-watchdog";
        case ESP_RST_DEEPSLEEP: return "deep-sleep";
        case ESP_RST_BROWNOUT: return "brownout";
        case ESP_RST_SDIO: return "sdio";
        case ESP_RST_UNKNOWN:
        default: return "unknown";
    }
}

void configureButtonPins() {
    pinMode(pins::kMenu, INPUT_PULLUP);
    pinMode(pins::kBack, INPUT_PULLUP);
    pinMode(pins::kRockerUp, INPUT_PULLUP);
    pinMode(pins::kRockerDown, INPUT_PULLUP);
    pinMode(pins::kRockerClick, INPUT_PULLUP);
}

const char* pressedLabel(const std::uint8_t pin) {
    return digitalRead(pin) == LOW ? "PRESSED" : "released";
}

}  // namespace

void printButtonStates() {
    configureButtonPins();
    delay(2);
    Serial.println("[buttons] active-low input state");
    Serial.printf("  HOME/MENU GPIO%u: %s\n", static_cast<unsigned>(pins::kMenu), pressedLabel(pins::kMenu));
    Serial.printf("  BACK/EXIT GPIO%u: %s\n", static_cast<unsigned>(pins::kBack), pressedLabel(pins::kBack));
    Serial.printf("  ROCKER UP GPIO%u: %s\n", static_cast<unsigned>(pins::kRockerUp), pressedLabel(pins::kRockerUp));
    Serial.printf("  ROCKER DOWN GPIO%u: %s\n", static_cast<unsigned>(pins::kRockerDown), pressedLabel(pins::kRockerDown));
    Serial.printf("  ROCKER CLICK GPIO%u: %s\n", static_cast<unsigned>(pins::kRockerClick), pressedLabel(pins::kRockerClick));
}

void printStartupDiagnostics() {
    Serial.println();
    Serial.println("============================================================");
    Serial.printf("House Cat firmware %s\n", config::kFirmwareVersion);
    Serial.println("Target: Elecrow CrowPanel ESP32-S3 2.13-inch e-paper N8R8");
    Serial.println("Display profile: Elecrow JD79661 factory sequence, 122x250 visible");
    Serial.println("Refresh profile: qualified full JD79661 waveform");
    Serial.printf("Reset reason: %s (%d)\n", resetReasonName(esp_reset_reason()), static_cast<int>(esp_reset_reason()));
    Serial.printf("Chip: %s rev %u, %u core(s), %u MHz\n",
                  ESP.getChipModel(),
                  static_cast<unsigned>(ESP.getChipRevision()),
                  static_cast<unsigned>(ESP.getChipCores()),
                  static_cast<unsigned>(ESP.getCpuFreqMHz()));
    Serial.printf("ESP-IDF/Arduino SDK: %s\n", ESP.getSdkVersion());
    Serial.printf("Flash: %u bytes @ %u Hz\n",
                  static_cast<unsigned>(ESP.getFlashChipSize()),
                  static_cast<unsigned>(ESP.getFlashChipSpeed()));
    Serial.printf("Heap: %u total, %u free, %u minimum free\n",
                  static_cast<unsigned>(ESP.getHeapSize()),
                  static_cast<unsigned>(ESP.getFreeHeap()),
                  static_cast<unsigned>(ESP.getMinFreeHeap()));
    Serial.printf("PSRAM: %s, %u total, %u free\n",
                  psramFound() ? "available" : "NOT FOUND",
                  static_cast<unsigned>(ESP.getPsramSize()),
                  static_cast<unsigned>(ESP.getFreePsram()));
    Serial.printf("Wi-Fi configured: %s\n", config::kWifiSsid[0] == '\0' ? "no (offline mode)" : "yes");
    Serial.printf("MQTT configured: %s\n", config::kMqttHost[0] == '\0' ? "no" : "yes");
    Serial.printf("EPD pins: BUSY=%u RST=%u MOSI=%u SCK=%u DC=%u CS=%u POWER=%u\n",
                  static_cast<unsigned>(pins::kEpaperBusy),
                  static_cast<unsigned>(pins::kEpaperReset),
                  static_cast<unsigned>(pins::kEpaperMosi),
                  static_cast<unsigned>(pins::kEpaperClock),
                  static_cast<unsigned>(pins::kEpaperDc),
                  static_cast<unsigned>(pins::kEpaperCs),
                  static_cast<unsigned>(pins::kDisplayPower));
    Serial.printf("Expansion pins: GPIO%u, GPIO%u\n",
                  static_cast<unsigned>(pins::kExpansionA),
                  static_cast<unsigned>(pins::kExpansionB));
    printButtonStates();
    Serial.println("Type 'help' in the serial monitor for offline test commands.");
    Serial.println("============================================================");
}

}  // namespace housecat::board

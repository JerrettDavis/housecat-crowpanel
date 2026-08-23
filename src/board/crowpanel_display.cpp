#include "board/crowpanel_display.h"

#include <Arduino.h>

#include <array>

#include "board/crowpanel_pins.h"

namespace housecat::board {

namespace {

// Elecrow's JD79661 full-refresh waveforms for the CrowPanel 2.13-inch panel.
// Unspecified elements are zero-initialized; the controller expects 56 bytes
// for each waveform register.
constexpr std::array<std::uint8_t, 56> kLut20{
    0x01, 0x00, 0x14, 0x14, 0x01, 0x00, 0x00, 0x01};
constexpr std::array<std::uint8_t, 56> kLut21{
    0x01, 0x60, 0x14, 0x14, 0x01, 0x00, 0x00, 0x01};
constexpr std::array<std::uint8_t, 56> kLut22{
    0x01, 0x20, 0x14, 0x14, 0x01, 0x00, 0x00, 0x01};
constexpr std::array<std::uint8_t, 56> kLut23{
    0x01, 0x10, 0x14, 0x14, 0x01, 0x00, 0x00, 0x01};
constexpr std::array<std::uint8_t, 56> kLut24{
    0x01, 0x90, 0x14, 0x14, 0x01, 0x00, 0x00, 0x01};

void writeBusByte(std::uint8_t value) {
    digitalWrite(pins::kEpaperCs, LOW);
    for (int bit = 0; bit < 8; ++bit) {
        digitalWrite(pins::kEpaperClock, LOW);
        digitalWrite(pins::kEpaperMosi, (value & 0x80U) != 0 ? HIGH : LOW);
        digitalWrite(pins::kEpaperClock, HIGH);
        value = static_cast<std::uint8_t>(value << 1U);
    }
    digitalWrite(pins::kEpaperCs, HIGH);
}

}  // namespace

void CrowPanelDisplay::writeCommand(const std::uint8_t command) {
    digitalWrite(pins::kEpaperDc, LOW);
    writeBusByte(command);
    digitalWrite(pins::kEpaperDc, HIGH);
}

void CrowPanelDisplay::writeData(const std::uint8_t data) {
    digitalWrite(pins::kEpaperDc, HIGH);
    writeBusByte(data);
}

void CrowPanelDisplay::writeData(const std::uint8_t* data, const std::size_t size) {
    for (std::size_t index = 0; index < size; ++index) {
        writeData(data[index]);
    }
}

void CrowPanelDisplay::resetPanel() {
    digitalWrite(pins::kEpaperReset, HIGH);
    delay(10);
    digitalWrite(pins::kEpaperReset, LOW);
    delay(100);
    digitalWrite(pins::kEpaperReset, HIGH);
    delay(100);
}

bool CrowPanelDisplay::waitUntilIdle(const std::uint32_t timeoutMs, const char* operation) {
    // This CrowPanel's JD79661 BUSY output is active-low, unlike the
    // active-high SSD1680 profile that was previously selected.
    const auto transitionStartedAt = millis();
    while (digitalRead(pins::kEpaperBusy) == HIGH) {
        if (millis() - transitionStartedAt >= 250) {
            Serial.printf("[display] %s did not assert BUSY; busy=%d\n",
                          operation,
                          digitalRead(pins::kEpaperBusy));
            return false;
        }
        delay(1);
    }

    const auto startedAt = millis();
    while (digitalRead(pins::kEpaperBusy) == LOW) {
        if (millis() - startedAt >= timeoutMs) {
            Serial.printf("[display] %s timed out after %lu ms; busy=%d\n",
                          operation,
                          static_cast<unsigned long>(timeoutMs),
                          digitalRead(pins::kEpaperBusy));
            return false;
        }
        delay(1);
    }
    return true;
}

void CrowPanelDisplay::initializeController() {
    resetPanel();

    writeCommand(0x00);
    writeData(0xF7);
    writeData(0x8A);

    writeCommand(0x01);
    constexpr std::array<std::uint8_t, 5> power{0x03, 0x00, 0x3F, 0x3F, 0x03};
    writeData(power.data(), power.size());

    writeCommand(0x03);
    writeData(0x00);

    writeCommand(0x06);
    constexpr std::array<std::uint8_t, 3> booster{0x27, 0x27, 0x2F};
    writeData(booster.data(), booster.size());

    writeCommand(0x30);
    writeData(0x0D);
    writeCommand(0x60);
    writeData(0x22);
    writeCommand(0x82);
    writeData(0x07);
    writeCommand(0xE3);
    writeData(0x88);
    writeCommand(0x41);
    writeData(0x00);

    writeCommand(0x61);
    constexpr std::array<std::uint8_t, 3> resolution{0x80, 0x00, 0xFA};
    writeData(resolution.data(), resolution.size());

    writeCommand(0x65);
    constexpr std::array<std::uint8_t, 3> gateSourceStart{0x00, 0x00, 0x00};
    writeData(gateSourceStart.data(), gateSourceStart.size());

    writeCommand(0x50);
    writeData(0xB7);
}

void CrowPanelDisplay::writeFullLut() {
    writeCommand(0x20);
    writeData(kLut20.data(), kLut20.size());
    writeCommand(0x21);
    writeData(kLut21.data(), kLut21.size());
    writeCommand(0x24);
    writeData(kLut24.data(), kLut24.size());

    // Elecrow's JD79661 reference alternates the R22/R23 waveform mapping
    // after every global refresh. Keeping a fixed mapping makes alternate
    // updates briefly show the new frame and then settle back to the old one.
    writeCommand(fullLutPhase_ ? 0x23 : 0x22);
    writeData(kLut22.data(), kLut22.size());
    writeCommand(fullLutPhase_ ? 0x22 : 0x23);
    writeData(kLut23.data(), kLut23.size());
    fullLutPhase_ = !fullLutPhase_;
}

void CrowPanelDisplay::begin() {
    pinMode(pins::kPowerLed, OUTPUT);
    pinMode(pins::kDisplayPower, OUTPUT);
    digitalWrite(pins::kPowerLed, HIGH);
    digitalWrite(pins::kDisplayPower, HIGH);
    delay(50);

    pinMode(pins::kEpaperBusy, INPUT);
    pinMode(pins::kEpaperReset, OUTPUT);
    pinMode(pins::kEpaperDc, OUTPUT);
    pinMode(pins::kEpaperCs, OUTPUT);
    pinMode(pins::kEpaperClock, OUTPUT);
    pinMode(pins::kEpaperMosi, OUTPUT);
    digitalWrite(pins::kEpaperCs, HIGH);
    digitalWrite(pins::kEpaperDc, HIGH);
    digitalWrite(pins::kEpaperClock, LOW);
    initializeController();

    // Clear the controller's OLD image plane left behind by prior firmware.
    // The first present() call supplies the matching NEW plane before refresh.
    writeCommand(0x10);
    for (std::size_t index = 0; index < MonoCanvas::kBufferBytes; ++index) {
        writeData(0xFF);
    }
    fullLutPhase_ = false;
    initialized_ = true;
}

void CrowPanelDisplay::present(const MonoCanvas& canvas, const RefreshKind requestedRefresh) {
    if (!initialized_ || requestedRefresh == RefreshKind::None) {
        return;
    }

    const auto startedAt = millis();
    Serial.printf("[display] full refresh started (requested=%s); busy=%d\n",
                  requestedRefresh == RefreshKind::Full ? "full" : "partial",
                  digitalRead(pins::kEpaperBusy));

    // The JD79661 panel scans House Cat's rows in reverse order and represents
    // white pixels with 1 bits. Reverse the row order and invert polarity, but
    // retain the bit order within each row so Latin text remains left-to-right.
    // Keep the controller's six hidden padding pixels white.
    encodeJd79661Frame(canvas, panelFrame_);

    // Until the exact partial waveform is qualified on hardware, use the
    // factory full-refresh sequence for every repaint. This keeps controls
    // functional and avoids driving the panel with an incompatible LUT.
    writeCommand(0x50);
    writeData(0xD7);
    writeCommand(0x13);
    writeData(panelFrame_.data(), panelFrame_.size());
    writeFullLut();
    writeCommand(0x17);
    writeData(0xA5);
    const bool completed = waitUntilIdle(15000, "full refresh");

    Serial.printf("[display] refresh %s in %lu ms; busy=%d\n",
                  completed ? "finished" : "failed",
                  static_cast<unsigned long>(millis() - startedAt),
                  digitalRead(pins::kEpaperBusy));
}

void CrowPanelDisplay::sleep() {
    if (!initialized_) {
        return;
    }
    writeCommand(0x07);
    writeData(0xA5);
    delay(20);
    digitalWrite(pins::kDisplayPower, LOW);
    initialized_ = false;
}

}  // namespace housecat::board

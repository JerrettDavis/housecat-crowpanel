#include "board/playground_lab.h"

#include <Arduino.h>
#include <WiFi.h>

#include <algorithm>
#include <utility>

#include "board/crowpanel_pins.h"

namespace housecat::board {

void PlaygroundLab::begin() {
    // The Lab is intentionally read-only. External accessories may drive these
    // pins, so the firmware must never source voltage onto them.
    pinMode(pins::kExpansionA, INPUT_PULLUP);
    pinMode(pins::kExpansionB, INPUT_PULLUP);
}

void PlaygroundLab::loop(const std::uint64_t nowMs) {
    if (app_.state().playground.scanning && !scanRunning_) {
        startWifiScan();
    }
    if (scanRunning_) {
        const int result = WiFi.scanComplete();
        if (result >= 0 || result == WIFI_SCAN_FAILED) {
            finishWifiScan(std::max(result, 0), nowMs);
        }
    }
    if (app_.state().lab.probeRequested) {
        sampleLab(nowMs);
    }
}

std::optional<DispatchResult> PlaygroundLab::takeDispatchResult() {
    auto result = pendingResult_;
    pendingResult_.reset();
    return result;
}

void PlaygroundLab::startWifiScan() {
    wasConnectedBeforeScan_ = WiFi.status() == WL_CONNECTED;
    WiFi.scanDelete();
    // Passive mode listens for access-point beacons and never sends probe
    // requests. A slightly longer dwell improves results while associated.
    const int result = WiFi.scanNetworks(true, true, true, 500);
    scanRunning_ = result == WIFI_SCAN_RUNNING;
    Serial.println(scanRunning_ ? "[playground] Wi-Fi scan started" : "[playground] Wi-Fi scan failed to start");
    if (!scanRunning_) {
        auto state = app_.state().playground;
        ++state.scanGeneration;
        schedule(app_.updatePlayground(std::move(state), millis()));
    }
}

void PlaygroundLab::finishWifiScan(const int count, const std::uint64_t nowMs) {
    auto state = app_.state().playground;
    state.networkCount = std::min<std::size_t>(static_cast<std::size_t>(count), state.networks.size());
    state.selectedIndex = 0;
    ++state.scanGeneration;
    for (std::size_t index = 0; index < state.networkCount; ++index) {
        auto& network = state.networks[index];
        network.name = WiFi.SSID(static_cast<int>(index)).c_str();
        if (network.name.empty()) {
            network.name = "Hidden network";
        }
        network.rssi = WiFi.RSSI(static_cast<int>(index));
        network.channel = static_cast<std::uint8_t>(WiFi.channel(static_cast<int>(index)));
        network.secured = WiFi.encryptionType(static_cast<int>(index)) != WIFI_AUTH_OPEN;
    }
    WiFi.scanDelete();
    scanRunning_ = false;
    Serial.printf("[playground] found %u Wi-Fi network(s)\n", static_cast<unsigned>(state.networkCount));
    if (wasConnectedBeforeScan_) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("[playground] Wi-Fi association preserved after scan");
        } else {
            Serial.println("[playground] restoring Wi-Fi association after scan");
            (void)WiFi.reconnect();
        }
    }
    wasConnectedBeforeScan_ = false;
    schedule(app_.updatePlayground(std::move(state), nowMs));
}

void PlaygroundLab::sampleLab(const std::uint64_t nowMs) {
    auto state = app_.state().lab;
    state.pins[0] = {pins::kExpansionA, digitalRead(pins::kExpansionA) == HIGH};
    state.pins[1] = {pins::kExpansionB, digitalRead(pins::kExpansionB) == HIGH};
    ++state.sampleCount;
    Serial.printf("[lab] sample %lu: GPIO40=%s GPIO41=%s\n",
                  static_cast<unsigned long>(state.sampleCount),
                  state.pins[0].high ? "HIGH" : "LOW",
                  state.pins[1].high ? "HIGH" : "LOW");
    schedule(app_.updateLab(std::move(state), nowMs));
}

void PlaygroundLab::schedule(DispatchResult result) {
    pendingResult_ = std::move(result);
}

}  // namespace housecat::board

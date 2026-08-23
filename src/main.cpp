#include <Arduino.h>
#include <esp_timer.h>

#include <algorithm>
#include <cstdint>
#include <utility>

#include "board/crowpanel_display.h"
#include "board/crowpanel_pins.h"
#include "board/diagnostics.h"
#include "board/crowpanel_input.h"
#include "board/preferences_store.h"
#include "board/playground_lab.h"
#include "board/library_reader.h"
#include "housecat/app/housecat_app.h"
#include "housecat/config.h"
#include "housecat/ui/mono_canvas.h"
#include "housecat/ui/renderer.h"
#include "integrations/mqtt_bridge.h"
#include "integrations/tailscale_manager.h"
#include "integrations/wifi_portal.h"
#include "integrations/serial_console.h"

namespace {

std::uint64_t nowMs() noexcept {
    return static_cast<std::uint64_t>(esp_timer_get_time()) / 1000ULL;
}

housecat::HouseCatApp app;
housecat::MonoCanvas canvas;
housecat::UiRenderer renderer;
housecat::board::CrowPanelDisplay display;
housecat::board::CrowPanelInput input;
housecat::board::PreferencesStore preferences;
housecat::board::PlaygroundLab playgroundLab(app);
housecat::board::LibraryReader libraryReader(app);
housecat::integrations::WifiPortal wifiPortal(app);
housecat::integrations::TailscaleManager tailscale(app, wifiPortal);
housecat::integrations::MqttBridge mqtt(app);
housecat::integrations::SerialConsole serialConsole(app, preferences);

housecat::RefreshKind pendingRefresh = housecat::RefreshKind::None;
std::uint64_t renderFirstPendingAt = 0;
std::uint64_t renderDueAt = 0;
bool persistenceDirty = false;
std::uint64_t persistenceDueAt = 0;

bool eventChangesPersistentState(const housecat::AppEventType type) noexcept {
    return type == housecat::AppEventType::Pet
        || type == housecat::AppEventType::LevelUp
        || type == housecat::AppEventType::MissionCompleted
        || type == housecat::AppEventType::MissionUpdated
        || type == housecat::AppEventType::OrientationChanged
        || type == housecat::AppEventType::LibraryUpdated
        || type == housecat::AppEventType::MealLogged
        || type == housecat::AppEventType::ActivityChanged
        || type == housecat::AppEventType::NeedsUpdated;
}

void schedule(housecat::DispatchResult result, const std::uint64_t currentMs) {
    if (!result.changed() && result.event.type == housecat::AppEventType::None) {
        return;
    }

    if (result.event.type != housecat::AppEventType::None) {
        mqtt.publishAppEvent(result.event);
        if (eventChangesPersistentState(result.event.type)) {
            persistenceDirty = true;
            persistenceDueAt = currentMs + housecat::config::kPersistenceSettleMs;
        }
    }

    if (!result.changed()) {
        return;
    }

    pendingRefresh = housecat::strongerRefresh(pendingRefresh, result.refresh);
    if (renderFirstPendingAt == 0) {
        renderFirstPendingAt = currentMs;
    }

    if (pendingRefresh == housecat::RefreshKind::Full) {
        renderDueAt = currentMs;
    } else {
        const auto settled = currentMs + housecat::config::kRenderSettleMs;
        const auto maximumWait = renderFirstPendingAt + 450;
        renderDueAt = std::min(settled, maximumWait);
    }
}

void renderNow(const housecat::RefreshKind refresh, const std::uint64_t currentMs) {
    renderer.render(app, currentMs, canvas);
    display.present(canvas, refresh);
    mqtt.publishState();
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(50);
    delay(250);  // Give the CrowPanel USB-to-serial bridge time to enumerate.
    housecat::board::printStartupDiagnostics();

    app.mutableState().cat.name = housecat::config::kDefaultCatName;
    (void)preferences.load(app.mutableState());

    Serial.println("[boot] initializing e-paper display...");
    display.begin();
    Serial.println("[boot] display initialized");
    input.begin();
    playgroundLab.begin();
    libraryReader.begin();
    serialConsole.begin();
    const bool forcePortal = digitalRead(housecat::board::pins::kMenu) == LOW
        && digitalRead(housecat::board::pins::kBack) == LOW;
    wifiPortal.begin(nowMs(), forcePortal);
    tailscale.begin(nowMs());
    mqtt.begin(nowMs());
    renderNow(housecat::RefreshKind::Full, nowMs());
}

void loop() {
    const auto currentMs = nowMs();

    serialConsole.loop(currentMs);
    if (auto result = serialConsole.takeDispatchResult(); result.has_value()) {
        schedule(std::move(*result), currentMs);
    }

    // Drain the buffered input queue. The input task continues sampling while
    // the e-paper driver is busy, so quick rocker presses are not discarded.
    housecat::InputAction action{};
    int drained = 0;
    while (drained < 12 && input.next(action)) {
        Serial.printf("[input] action=%u\n", static_cast<unsigned>(action));
        schedule(app.dispatch(action, currentMs), currentMs);
        ++drained;
    }

    wifiPortal.loop(currentMs, app.state().playground.scanning);
    if (auto result = wifiPortal.takeDispatchResult(); result.has_value()) {
        schedule(std::move(*result), currentMs);
    }
    tailscale.loop(currentMs);
    if (auto result = tailscale.takeDispatchResult(); result.has_value()) {
        schedule(std::move(*result), currentMs);
    }
    mqtt.loop(currentMs);
    if (auto result = mqtt.takeDispatchResult(); result.has_value()) {
        schedule(std::move(*result), currentMs);
    }

    playgroundLab.loop(currentMs);
    if (auto result = playgroundLab.takeDispatchResult(); result.has_value()) {
        schedule(std::move(*result), currentMs);
    }

    libraryReader.loop(currentMs);
    if (auto result = libraryReader.takeDispatchResult(); result.has_value()) {
        schedule(std::move(*result), currentMs);
    }

    schedule(app.tick(currentMs), currentMs);

    if (pendingRefresh != housecat::RefreshKind::None && currentMs >= renderDueAt) {
        const auto refresh = pendingRefresh;
        pendingRefresh = housecat::RefreshKind::None;
        renderFirstPendingAt = 0;
        renderDueAt = 0;
        renderNow(refresh, currentMs);
    }

    if (persistenceDirty && currentMs >= persistenceDueAt) {
        if (preferences.save(app.state())) {
            persistenceDirty = false;
        } else {
            persistenceDueAt = currentMs + 5000;
        }
    }

    delay(5);
}

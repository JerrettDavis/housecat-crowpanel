#pragma once

#include <Arduino.h>

#include <cstdint>
#include <optional>

#include "board/preferences_store.h"
#include "housecat/app/housecat_app.h"

namespace housecat::integrations {

class SerialConsole final {
public:
    SerialConsole(HouseCatApp& app, board::PreferencesStore& preferences)
        : app_(app), preferences_(preferences) {}

    void begin();
    void loop(std::uint64_t nowMs);
    [[nodiscard]] std::optional<DispatchResult> takeDispatchResult();

private:
    void execute(String command, std::uint64_t nowMs);
    void schedule(DispatchResult result);
    void printHelp() const;
    void printStatus(std::uint64_t nowMs) const;
    void injectNotification(
        NotificationPriority priority,
        NotificationKind kind,
        const String& payload,
        bool requiresAcknowledgement,
        std::uint64_t nowMs);
    void injectWeather(const String& arguments, std::uint64_t nowMs);
    void injectMission(const String& arguments, std::uint64_t nowMs);
    void setOrientation(String argument);

    HouseCatApp& app_;
    board::PreferencesStore& preferences_;
    String line_{};
    std::optional<DispatchResult> pendingResult_{};
};

}  // namespace housecat::integrations

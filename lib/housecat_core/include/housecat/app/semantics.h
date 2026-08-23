#pragma once

#include <string_view>

#include "housecat/app/housecat_app.h"

namespace housecat {

[[nodiscard]] inline const char* semanticName(const ScreenId value) noexcept {
    switch (value) {
        case ScreenId::Home: return "home";
        case ScreenId::Menu: return "menu";
        case ScreenId::Cat: return "cat";
        case ScreenId::Missions: return "missions";
        case ScreenId::Whiskers: return "whiskers";
        case ScreenId::Library: return "library";
        case ScreenId::Lab: return "lab";
        case ScreenId::Routine: return "routine";
        case ScreenId::Settings: return "settings";
        case ScreenId::Notification: return "notification";
    }
    return "unknown";
}

[[nodiscard]] inline const char* semanticName(const Mood value) noexcept {
    switch (value) {
        case Mood::Content: return "content";
        case Mood::Happy: return "happy";
        case Mood::Alert: return "alert";
        case Mood::Sleepy: return "sleepy";
        case Mood::Curious: return "curious";
        case Mood::Worried: return "worried";
    }
    return "unknown";
}

[[nodiscard]] inline const char* semanticName(const AppEventType value) noexcept {
    switch (value) {
        case AppEventType::None: return "none";
        case AppEventType::Pet: return "pet";
        case AppEventType::LevelUp: return "level_up";
        case AppEventType::NotificationShown: return "notification_shown";
        case AppEventType::NotificationAcknowledged: return "notification_acknowledged";
        case AppEventType::NotificationDismissed: return "notification_dismissed";
        case AppEventType::MissionCompleted: return "mission_completed";
        case AppEventType::MissionUpdated: return "mission_updated";
        case AppEventType::OrientationChanged: return "orientation_changed";
        case AppEventType::ScreenChanged: return "screen_changed";
        case AppEventType::PlaygroundScanRequested: return "playground_scan_requested";
        case AppEventType::PlaygroundUpdated: return "playground_updated";
        case AppEventType::LabProbeRequested: return "lab_probe_requested";
        case AppEventType::LabUpdated: return "lab_updated";
        case AppEventType::LibraryDownloadRequested: return "library_download_requested";
        case AppEventType::LibraryPageRequested: return "library_page_requested";
        case AppEventType::LibraryUpdated: return "library_updated";
        case AppEventType::MealLogged: return "meal_logged";
        case AppEventType::ActivityChanged: return "activity_changed";
        case AppEventType::NeedsUpdated: return "needs_updated";
    }
    return "unknown";
}

[[nodiscard]] inline NotificationPriority notificationPriorityFrom(const std::string_view value) noexcept {
    if (value == "ambient") return NotificationPriority::Ambient;
    if (value == "info") return NotificationPriority::Info;
    if (value == "important") return NotificationPriority::Important;
    if (value == "urgent") return NotificationPriority::Urgent;
    if (value == "critical") return NotificationPriority::Critical;
    return NotificationPriority::Notice;
}

[[nodiscard]] inline NotificationKind notificationKindFrom(const std::string_view value) noexcept {
    if (value == "person") return NotificationKind::Person;
    if (value == "vehicle") return NotificationKind::Vehicle;
    if (value == "home") return NotificationKind::Home;
    if (value == "warning") return NotificationKind::Warning;
    if (value == "mission") return NotificationKind::Mission;
    return NotificationKind::Generic;
}

[[nodiscard]] inline WeatherCondition weatherConditionFrom(const std::string_view value) noexcept {
    if (value == "sunny" || value == "clear" || value == "clear-night") return WeatherCondition::Sunny;
    if (value == "rainy" || value == "rain" || value == "pouring") return WeatherCondition::Rainy;
    if (value == "cloudy" || value == "partlycloudy" || value == "partly-cloudy") return WeatherCondition::Cloudy;
    return WeatherCondition::Unknown;
}

[[nodiscard]] inline const char* weatherLabel(const WeatherCondition value) noexcept {
    switch (value) {
        case WeatherCondition::Sunny: return "Sunny";
        case WeatherCondition::Cloudy: return "Cloudy";
        case WeatherCondition::Rainy: return "Rainy";
        case WeatherCondition::Unknown: return "Unknown";
    }
    return "Unknown";
}

}  // namespace housecat

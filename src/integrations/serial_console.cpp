#include "integrations/serial_console.h"

#include <Arduino.h>
#include <WiFi.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>

#include "board/diagnostics.h"
#include "housecat/app/semantics.h"
#include "housecat/config.h"

namespace housecat::integrations {
namespace {

String firstField(const String& value) {
    const int separator = value.indexOf('|');
    return separator < 0 ? value : value.substring(0U, static_cast<unsigned int>(separator));
}

String remainingFields(const String& value) {
    const int separator = value.indexOf('|');
    return separator < 0 ? String{} : value.substring(static_cast<unsigned int>(separator + 1));
}

WeatherCondition parseWeatherCondition(String value) {
    value.trim();
    value.toLowerCase();
    return weatherConditionFrom(value.c_str());
}

}  // namespace

void SerialConsole::begin() {
    line_.reserve(256);
    Serial.println("[console] ready; type 'help' followed by Enter");
}

void SerialConsole::loop(const std::uint64_t nowMs) {
    while (Serial.available() > 0) {
        const char value = static_cast<char>(Serial.read());
        if (value == '\r') {
            continue;
        }
        if (value == '\n') {
            line_.trim();
            if (line_.length() != 0) {
                execute(line_, nowMs);
            }
            line_ = "";
            continue;
        }
        if (value == '\b' || value == 0x7F) {
            if (line_.length() != 0) {
                line_.remove(static_cast<unsigned int>(line_.length() - 1));
            }
            continue;
        }
        if (line_.length() < 384) {
            line_ += value;
        }
    }
}

std::optional<DispatchResult> SerialConsole::takeDispatchResult() {
    auto result = pendingResult_;
    pendingResult_.reset();
    return result;
}

void SerialConsole::execute(String command, const std::uint64_t nowMs) {
    command.trim();
    String verb = command;
    String arguments;
    const int space = command.indexOf(' ');
    if (space >= 0) {
        verb = command.substring(0U, static_cast<unsigned int>(space));
        arguments = command.substring(static_cast<unsigned int>(space + 1));
        arguments.trim();
    }
    verb.toLowerCase();

    if (verb == "help" || verb == "?") {
        printHelp();
    } else if (verb == "status") {
        printStatus(nowMs);
    } else if (verb == "buttons") {
        board::printButtonStates();
    } else if (verb == "diagnostics" || verb == "diag") {
        board::printStartupDiagnostics();
    } else if (verb == "up") {
        schedule(app_.dispatch(InputAction::Up, nowMs));
    } else if (verb == "down") {
        schedule(app_.dispatch(InputAction::Down, nowMs));
    } else if (verb == "select" || verb == "ok") {
        schedule(app_.dispatch(InputAction::Select, nowMs));
    } else if (verb == "menu" || verb == "home") {
        schedule(app_.dispatch(InputAction::Menu, nowMs));
    } else if (verb == "back") {
        schedule(app_.dispatch(InputAction::Back, nowMs));
    } else if (verb == "pet") {
        schedule(app_.pet(nowMs));
    } else if (verb == "notify") {
        injectNotification(NotificationPriority::Notice, NotificationKind::Generic, arguments, false, nowMs);
    } else if (verb == "person") {
        injectNotification(NotificationPriority::Notice, NotificationKind::Person,
                           arguments.length() == 0 ? "Sam is home!|Hoo-ray!" : arguments, false, nowMs);
    } else if (verb == "car") {
        injectNotification(NotificationPriority::Important, NotificationKind::Vehicle,
                           arguments.length() == 0 ? "Ioniq 5 N charged!|Ready to zoom." : arguments, false, nowMs);
    } else if (verb == "alert") {
        injectNotification(NotificationPriority::Urgent, NotificationKind::Warning,
                           arguments.length() == 0 ? "Water detected!|Check the kitchen." : arguments, true, nowMs);
    } else if (verb == "weather") {
        injectWeather(arguments, nowMs);
    } else if (verb == "mission") {
        injectMission(arguments, nowMs);
    } else if (verb == "rotate") {
        setOrientation(arguments);
    } else if (verb == "full" || verb == "repaint") {
        schedule({RefreshKind::Full, {}});
    } else if (verb == "partial") {
        schedule({RefreshKind::Partial, {}});
    } else if (verb == "factory-reset") {
        Serial.println("[console] clearing House Cat NVS state and rebooting...");
        if (!preferences_.clear()) {
            Serial.println("[console] NVS clear failed");
            return;
        }
        Serial.flush();
        delay(50);
        ESP.restart();
    } else if (verb == "reboot") {
        Serial.println("[console] rebooting...");
        Serial.flush();
        delay(50);
        ESP.restart();
    } else {
        Serial.printf("[console] unknown command: %s\n", verb.c_str());
        Serial.println("[console] type 'help' for commands");
    }
}

void SerialConsole::schedule(DispatchResult result) {
    if (!result.changed() && result.event.type == AppEventType::None) {
        return;
    }
    if (!pendingResult_.has_value()) {
        pendingResult_ = std::move(result);
        return;
    }
    pendingResult_->refresh = strongerRefresh(pendingResult_->refresh, result.refresh);
    if (result.event.type != AppEventType::None) {
        pendingResult_->event = std::move(result.event);
    }
}

void SerialConsole::printHelp() const {
    Serial.println();
    Serial.println("House Cat offline test console");
    Serial.println("  status                 show buddy, screen, network, and memory state");
    Serial.println("  up | down | select     emulate the rocker");
    Serial.println("  menu | back | pet      emulate dedicated controls / buddy action");
    Serial.println("  person [title|body]    show a friendly arrival notification");
    Serial.println("  car [title|body]       show a vehicle notification");
    Serial.println("  notify title|body      show a normal notification");
    Serial.println("  alert title|body       show an urgent acknowledgement-required alert");
    Serial.println("  weather 82 72 sunny    update outside, inside, and weather condition");
    Serial.println("  mission 2 5 Title|Detail  set mission progress and text");
    Serial.println("  rotate 0|90|180|270    apply and persist orientation");
    Serial.println("  full | partial         force a screen repaint");
    Serial.println("  buttons | diagnostics  inspect board state");
    Serial.println("  factory-reset          clear buddy/settings state and restart");
    Serial.println("  reboot                 restart the ESP32-S3");
    Serial.println();
}

void SerialConsole::printStatus(const std::uint64_t nowMs) const {
    const auto& state = app_.state();
    Serial.println();
    Serial.println("[status]");
    Serial.printf("  firmware: %s\n", config::kFirmwareVersion);
    Serial.printf("  cat: %s, level %u, XP %u, bond %u%%, mood %s\n",
                  state.cat.name.c_str(),
                  static_cast<unsigned>(state.cat.level),
                  static_cast<unsigned>(state.cat.totalXp),
                  static_cast<unsigned>(state.cat.bond),
                  semanticName(state.cat.mood));
    Serial.printf("  screen: %s, orientation %u degrees\n",
                  semanticName(state.ui.screen),
                  static_cast<unsigned>(state.settings.orientation) * 90U);
    Serial.printf("  mission: %s (%u/%u)%s\n",
                  state.mission.title.c_str(),
                  static_cast<unsigned>(state.mission.progress),
                  static_cast<unsigned>(state.mission.target),
                  state.mission.complete ? " complete" : "");
    Serial.printf("  notification: %s, queued %u\n",
                  state.activeNotification.has_value() ? state.activeNotification->title.c_str() : "none",
                  static_cast<unsigned>(state.notificationQueue.size()));
    if (state.home.updatedAtMs == 0) {
        Serial.println("  outside weather: unavailable (no live snapshot received)");
    } else {
        const auto ageSeconds = nowMs >= state.home.updatedAtMs
            ? (nowMs - state.home.updatedAtMs) / 1000ULL
            : 0ULL;
        Serial.printf("  outside weather: %.1f F, %s, age %llu s%s\n",
                      state.home.outsideTemperatureF,
                      state.home.conditionLabel.c_str(),
                      static_cast<unsigned long long>(ageSeconds),
                      state.home.stale ? " STALE" : "");
    }
    Serial.printf("  rooms: %u", static_cast<unsigned>(state.home.roomCount));
    for (std::size_t index = 0; index < state.home.roomCount; ++index) {
        const auto& room = state.home.rooms[index];
        Serial.printf("; %s %.1f F", room.name.c_str(), room.temperatureF);
        if (room.hasHumidity) {
            Serial.printf(" %.0f%% RH", room.humidityPercent);
        }
    }
    Serial.println();
    Serial.printf("  Wi-Fi: %s", WiFi.status() == WL_CONNECTED ? "connected" : "offline");
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf(", %s, RSSI %d dBm, IP %s", WiFi.SSID().c_str(), WiFi.RSSI(), WiFi.localIP().toString().c_str());
    }
    Serial.println();
    Serial.printf("  heap: %u free; PSRAM: %u/%u free\n",
                  static_cast<unsigned>(ESP.getFreeHeap()),
                  static_cast<unsigned>(ESP.getFreePsram()),
                  static_cast<unsigned>(ESP.getPsramSize()));
    Serial.println();
}

void SerialConsole::injectNotification(
    const NotificationPriority priority,
    const NotificationKind kind,
    const String& payload,
    const bool requiresAcknowledgement,
    const std::uint64_t nowMs) {
    String title = firstField(payload);
    String body = remainingFields(payload);
    title.trim();
    body.trim();
    if (title.length() == 0) title = "News from home";

    Notification notification{};
    notification.id = std::string("serial-") + std::to_string(nowMs);
    notification.title = title.c_str();
    notification.body = body.c_str();
    notification.kind = kind;
    notification.priority = priority;
    notification.createdAtMs = nowMs;
    notification.expiresAtMs = nowMs + 5ULL * 60ULL * 1000ULL;
    notification.requiresAcknowledgement = requiresAcknowledgement;
    schedule(app_.receiveNotification(std::move(notification), nowMs));
    Serial.printf("[console] notification queued: %s\n", title.c_str());
}

void SerialConsole::injectWeather(const String& arguments, const std::uint64_t nowMs) {
    float outside = app_.state().home.outsideTemperatureF;
    float inside = app_.state().home.insideTemperatureF;
    char conditionBuffer[24] = "sunny";
    const int parsed = std::sscanf(arguments.c_str(), "%f %f %23s", &outside, &inside, conditionBuffer);
    if (parsed < 2) {
        Serial.println("[console] usage: weather <outside-f> <inside-f> [sunny|cloudy|rainy]");
        return;
    }

    auto snapshot = app_.state().home;
    snapshot.outsideTemperatureF = outside;
    snapshot.insideTemperatureF = inside;
    snapshot.condition = parsed >= 3 ? parseWeatherCondition(conditionBuffer) : snapshot.condition;
    snapshot.conditionLabel = weatherLabel(snapshot.condition);
    schedule(app_.updateHome(std::move(snapshot), nowMs));
    Serial.printf("[console] weather updated: %.1fF outside, %.1fF inside, %s\n",
                  outside, inside, weatherLabel(app_.state().home.condition));
}

void SerialConsole::injectMission(const String& arguments, const std::uint64_t nowMs) {
    const int firstSpace = arguments.indexOf(' ');
    const int secondSpace = firstSpace < 0 ? -1 : arguments.indexOf(' ', static_cast<unsigned int>(firstSpace + 1));
    if (firstSpace < 0 || secondSpace < 0) {
        Serial.println("[console] usage: mission <progress> <target> <title>|<detail>");
        return;
    }

    const long progress = arguments.substring(0U, static_cast<unsigned int>(firstSpace)).toInt();
    const long target = arguments.substring(static_cast<unsigned int>(firstSpace + 1), static_cast<unsigned int>(secondSpace)).toInt();
    const String text = arguments.substring(static_cast<unsigned int>(secondSpace + 1));
    String title = firstField(text);
    String detail = remainingFields(text);
    title.trim();
    detail.trim();
    if (target <= 0 || title.length() == 0) {
        Serial.println("[console] mission target must be positive and title cannot be empty");
        return;
    }

    Mission mission{};
    mission.id = "serial-mission";
    mission.title = title.c_str();
    mission.detail = detail.c_str();
    mission.progress = static_cast<std::uint16_t>(std::clamp<long>(progress, 0, 65535));
    mission.target = static_cast<std::uint16_t>(std::clamp<long>(target, 1, 65535));
    mission.complete = mission.progress >= mission.target;
    schedule(app_.updateMission(std::move(mission), nowMs));
    Serial.printf("[console] mission updated: %s (%u/%u)\n",
                  title.c_str(),
                  static_cast<unsigned>(app_.state().mission.progress),
                  static_cast<unsigned>(app_.state().mission.target));
}

void SerialConsole::setOrientation(String argument) {
    argument.trim();
    Orientation orientation = Orientation::Deg0;
    if (argument == "0") {
        orientation = Orientation::Deg0;
    } else if (argument == "90") {
        orientation = Orientation::Deg90;
    } else if (argument == "180") {
        orientation = Orientation::Deg180;
    } else if (argument == "270") {
        orientation = Orientation::Deg270;
    } else {
        Serial.println("[console] usage: rotate 0|90|180|270");
        return;
    }

    app_.mutableState().settings.orientation = orientation;
    app_.mutableState().ui.draftOrientation = orientation;
    schedule({RefreshKind::Full, {AppEventType::OrientationChanged, argument.c_str()}});
    Serial.printf("[console] orientation set to %s degrees\n", argument.c_str());
}

}  // namespace housecat::integrations

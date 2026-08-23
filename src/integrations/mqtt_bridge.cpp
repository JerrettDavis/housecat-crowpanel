#include "integrations/mqtt_bridge.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <esp_timer.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>

#include "housecat/config.h"
#include "housecat/app/semantics.h"
#include "housecat/domain/routine.h"

namespace housecat::integrations {
namespace {

std::uint64_t monotonicNowMs() noexcept {
    return static_cast<std::uint64_t>(esp_timer_get_time()) / 1000ULL;
}

}  // namespace

MqttBridge::MqttBridge(HouseCatApp& app)
    : app_(app),
      baseTopic_(std::string("housecat/") + config::kDeviceId) {}

void MqttBridge::begin(const std::uint64_t nowMs) {
    client_.setServer(config::kMqttHost, config::kMqttPort);
    client_.setBufferSize(8192);
    client_.setKeepAlive(30);
    client_.setCallback([this](char* incomingTopic, std::uint8_t* payload, unsigned int length) {
        onMessage(incomingTopic, payload, length);
    });
}

void MqttBridge::loop(const std::uint64_t nowMs) {
    ensureMqtt(nowMs);
    if (client_.connected()) {
        (void)client_.loop();
        if (nowMs >= nextStatePublishMs_) {
            publishState();
            nextStatePublishMs_ = nowMs + config::kStatePublishMs;
        }
    }
}

std::optional<DispatchResult> MqttBridge::takeDispatchResult() {
    auto result = pendingResult_;
    pendingResult_.reset();
    return result;
}

void MqttBridge::ensureMqtt(const std::uint64_t nowMs) {
    if (WiFi.status() != WL_CONNECTED || config::kMqttHost[0] == '\0') {
        if (wifiWasConnected_) {
            wifiWasConnected_ = false;
            Serial.println("[wifi] connection lost");
        }
        schedule(app_.setConnection(ConnectionState::Offline, 0, nowMs));
        return;
    }
    if (!wifiWasConnected_) {
        wifiWasConnected_ = true;
        Serial.printf("[wifi] connected to %s; IP=%s; RSSI=%d dBm\n",
                      WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI());
    }
    if (!client_.connected()) schedule(app_.setConnection(ConnectionState::WifiOnly, WiFi.RSSI(), nowMs));
    if (client_.connected()) {
        mqttWasConnected_ = true;
        return;
    }
    if (mqttWasConnected_) {
        mqttWasConnected_ = false;
        Serial.printf("[mqtt] connection lost; state=%d\n", client_.state());
    }
    if (nowMs < nextMqttAttemptMs_) {
        return;
    }
    nextMqttAttemptMs_ = nowMs + config::kMqttReconnectMs;
    const bool onHomeIot = config::kHomeNetworkPrefix[0] != '\0' &&
        WiFi.localIP().toString().startsWith(config::kHomeNetworkPrefix);
    const char* brokerHost = !onHomeIot && config::kMqttRemoteHost[0] != '\0'
        ? config::kMqttRemoteHost
        : config::kMqttHost;
    client_.setServer(brokerHost, config::kMqttPort);
    Serial.printf("[mqtt] connecting to %s:%u...\n", brokerHost, static_cast<unsigned>(config::kMqttPort));

    const auto availability = topic("availability");
    bool connected = false;
    if (config::kMqttUsername[0] == '\0') {
        connected = client_.connect(
            config::kDeviceId,
            availability.c_str(),
            0,
            true,
            "offline");
    } else {
        connected = client_.connect(
            config::kDeviceId,
            config::kMqttUsername,
            config::kMqttPassword,
            availability.c_str(),
            0,
            true,
            "offline");
    }
    if (!connected) {
        Serial.printf("[mqtt] connection failed; state=%d\n", client_.state());
        return;
    }

    mqttWasConnected_ = true;
    Serial.printf("[mqtt] connected; base topic=%s\n", baseTopic_.c_str());
    (void)client_.publish(availability.c_str(), "online", true);
    (void)client_.subscribe(topic("command/notification").c_str());
    (void)client_.subscribe(topic("command/home").c_str());
    (void)client_.subscribe(topic("command/mission").c_str());
    (void)client_.subscribe(topic("command/action").c_str());
    (void)client_.subscribe("homeassistant/status");
    publishDiscovery();
    publishState();
    nextStatePublishMs_ = nowMs + config::kStatePublishMs;
    schedule(app_.setConnection(ConnectionState::HomeAssistant, WiFi.RSSI(), nowMs));
}

void MqttBridge::onMessage(char* incomingTopic, std::uint8_t* payload, const unsigned int length) {
    std::string message(reinterpret_cast<const char*>(payload), length);
    const auto now = monotonicNowMs();
    const std::string incoming(incomingTopic);

    if (incoming == "homeassistant/status") {
        if (message == "online") {
            publishDiscovery();
            publishState();
        }
        return;
    }
    if (incoming == topic("command/notification")) {
        handleNotification(message.c_str(), now);
    } else if (incoming == topic("command/home")) {
        handleHome(message.c_str(), now);
    } else if (incoming == topic("command/mission")) {
        handleMission(message.c_str(), now);
    } else if (incoming == topic("command/action")) {
        handleAction(message.c_str(), now);
    }
}

void MqttBridge::handleNotification(const char* payload, const std::uint64_t nowMs) {
    JsonDocument document;
    if (deserializeJson(document, payload) != DeserializationError::Ok) {
        Serial.println("[mqtt] ignored invalid notification JSON");
        return;
    }

    Notification notification{};
    notification.id = static_cast<const char*>(document["id"] | "");
    notification.title = static_cast<const char*>(document["title"] | "News from home");
    notification.body = static_cast<const char*>(document["body"] | "");
    notification.kind = notificationKindFrom(document["kind"] | "generic");
    notification.priority = notificationPriorityFrom(document["priority"] | "notice");
    notification.createdAtMs = nowMs;
    const std::uint32_t ttlSeconds = document["ttl_s"] | 180U;
    notification.expiresAtMs = ttlSeconds == 0 ? 0 : nowMs + static_cast<std::uint64_t>(ttlSeconds) * 1000ULL;
    notification.requiresAcknowledgement = document["ack"] | false;
    schedule(app_.receiveNotification(std::move(notification), nowMs));
}

void MqttBridge::handleHome(const char* payload, const std::uint64_t nowMs) {
    JsonDocument document;
    if (deserializeJson(document, payload) != DeserializationError::Ok) {
        Serial.println("[mqtt] ignored invalid home snapshot JSON");
        return;
    }

    const auto outsideValue = document["outside_f"];
    if (!(outsideValue.is<float>() || outsideValue.is<double>() || outsideValue.is<int>())) {
        Serial.println("[mqtt] ignored home snapshot without numeric outside_f");
        return;
    }
    const float outside = outsideValue.as<float>();
    if (!std::isfinite(outside) || outside < -100.0F || outside > 150.0F) {
        Serial.println("[mqtt] ignored implausible outside temperature");
        return;
    }

    HomeSnapshot snapshot = app_.state().home;
    snapshot.outsideTemperatureF = outside;
    const auto insideValue = document["inside_f"];
    if (insideValue.is<float>() || insideValue.is<double>() || insideValue.is<int>()) {
        const float inside = insideValue.as<float>();
        if (std::isfinite(inside) && inside >= -100.0F && inside <= 150.0F) {
            snapshot.insideTemperatureF = inside;
        }
    }
    const char* condition = document["condition"] | "unknown";
    snapshot.condition = weatherConditionFrom(condition);
    snapshot.conditionLabel = static_cast<const char*>(document["condition_label"] | condition);
    if (document["pal_label"].is<const char*>()) {
        snapshot.palLabel = document["pal_label"].as<const char*>();
    }
    if (document["pal_message"].is<const char*>()) {
        snapshot.palMessage = document["pal_message"].as<const char*>();
    }
    if (document["rooms"].is<JsonArrayConst>()) {
        snapshot.roomCount = 0;
        for (JsonObjectConst room : document["rooms"].as<JsonArrayConst>()) {
            if (snapshot.roomCount >= snapshot.rooms.size()) {
                break;
            }
            const auto temperatureValue = room["temperature_f"];
            if (!(temperatureValue.is<float>() || temperatureValue.is<double>() || temperatureValue.is<int>())) {
                continue;
            }
            const float roomTemperature = temperatureValue.as<float>();
            if (!std::isfinite(roomTemperature) || roomTemperature < -100.0F || roomTemperature > 150.0F) {
                continue;
            }
            auto& target = snapshot.rooms[snapshot.roomCount++];
            target.name = static_cast<const char*>(room["name"] | "Room");
            target.temperatureF = roomTemperature;
            target.hasHumidity = room["humidity"].is<float>() || room["humidity"].is<double>()
                || room["humidity"].is<int>();
            target.humidityPercent = target.hasHumidity ? room["humidity"].as<float>() : 0.0F;
        }
    }
    schedule(app_.updateHome(std::move(snapshot), nowMs));
    if (document["epoch_s"].is<std::uint64_t>() || document["epoch_s"].is<std::uint32_t>()) {
        schedule(app_.updateClock(document["epoch_s"].as<std::uint64_t>(), nowMs));
    }
}

void MqttBridge::handleMission(const char* payload, const std::uint64_t nowMs) {
    JsonDocument document;
    if (deserializeJson(document, payload) != DeserializationError::Ok) {
        Serial.println("[mqtt] ignored invalid mission JSON");
        return;
    }
    Mission mission{};
    mission.id = static_cast<const char*>(document["id"] | "remote");
    mission.title = static_cast<const char*>(document["title"] | "New mission");
    mission.detail = static_cast<const char*>(document["detail"] | "");
    const int progress = document["progress"] | 0;
    const int target = document["target"] | 1;
    mission.progress = static_cast<std::uint16_t>(std::clamp(progress, 0, 65535));
    mission.target = static_cast<std::uint16_t>(std::clamp(target, 1, 65535));
    mission.complete = document["complete"] | (mission.progress >= mission.target);
    schedule(app_.updateMission(std::move(mission), nowMs));
}

void MqttBridge::handleAction(const char* payload, const std::uint64_t nowMs) {
    if (std::strcmp(payload, "pet") == 0) {
        schedule(app_.pet(nowMs));
        return;
    }
    if (std::strcmp(payload, "feed") == 0) {
        schedule(app_.feed(nowMs));
        return;
    }
    if (std::strcmp(payload, "work") == 0) {
        schedule(app_.setActivity(ActivityMode::Work, nowMs));
        return;
    }
    if (std::strcmp(payload, "play") == 0) {
        schedule(app_.setActivity(ActivityMode::Play, nowMs));
        return;
    }
    if (std::strcmp(payload, "sleep") == 0) {
        schedule(app_.setActivity(ActivityMode::Sleep, nowMs));
        return;
    }
    if (std::strcmp(payload, "idle") == 0) {
        const auto active = app_.state().routine.activity;
        if (active != ActivityMode::Idle) schedule(app_.setActivity(active, nowMs));
        return;
    }
    if (std::strcmp(payload, "home") == 0) {
        if (app_.state().ui.screen != ScreenId::Home && app_.state().ui.screen != ScreenId::Notification) {
            schedule(app_.dispatch(InputAction::Menu, nowMs));
        }
        return;
    }
    if (std::strcmp(payload, "scan") == 0) {
        schedule(app_.requestPlaygroundScan(nowMs));
        return;
    }
    if (std::strcmp(payload, "probe") == 0) {
        schedule(app_.requestLabProbe(nowMs));
        return;
    }

    InputAction action = InputAction::Select;
    if (std::strcmp(payload, "select") == 0) {
        action = InputAction::Select;
    } else if (std::strcmp(payload, "up") == 0) {
        action = InputAction::Up;
    } else if (std::strcmp(payload, "down") == 0) {
        action = InputAction::Down;
    } else if (std::strcmp(payload, "menu") == 0) {
        action = InputAction::Menu;
    } else if (std::strcmp(payload, "back") == 0) {
        action = InputAction::Back;
    } else {
        return;
    }
    schedule(app_.dispatch(action, nowMs));
}

void MqttBridge::publishDiscovery() {
    if (!client_.connected()) {
        return;
    }

    JsonDocument document;
    document["dev"]["ids"] = config::kDeviceId;
    document["dev"]["name"] = config::kDeviceName;
    document["dev"]["mf"] = "House Cat Project";
    document["dev"]["mdl"] = "CrowPanel ESP32 2.13 E-Paper";
    document["dev"]["sw"] = config::kFirmwareVersion;
    document["o"]["name"] = "House Cat firmware";
    document["o"]["sw"] = config::kFirmwareVersion;
    document["state_topic"] = baseTopic_ + "/state";
    document["availability_topic"] = baseTopic_ + "/availability";

    auto components = document["cmps"].to<JsonObject>();
    auto level = components["level"].to<JsonObject>();
    level["p"] = "sensor";
    level["name"] = "Level";
    level["unique_id"] = std::string(config::kDeviceId) + "_level";
    level["value_template"] = "{{ value_json.level }}";
    level["icon"] = "mdi:star";

    auto bond = components["bond"].to<JsonObject>();
    bond["p"] = "sensor";
    bond["name"] = "Bond";
    bond["unique_id"] = std::string(config::kDeviceId) + "_bond";
    bond["value_template"] = "{{ value_json.bond }}";
    bond["unit_of_measurement"] = "%";
    bond["icon"] = "mdi:heart";

    const auto addNeedSensor = [&](const char* key, const char* name, const char* field, const char* icon) {
        auto component = components[key].to<JsonObject>();
        component["p"] = "sensor";
        component["name"] = name;
        component["unique_id"] = std::string(config::kDeviceId) + "_" + key;
        component["value_template"] = std::string("{{ value_json.") + field + " }}";
        component["unit_of_measurement"] = "%";
        component["state_class"] = "measurement";
        component["icon"] = icon;
    };
    addNeedSensor("food", "Food", "food", "mdi:food-apple");
    addNeedSensor("rest", "Rest", "rest", "mdi:sleep");
    addNeedSensor("fun", "Entertainment", "fun", "mdi:gamepad-variant");
    addNeedSensor("energy", "Energy", "energy", "mdi:lightning-bolt");

    const auto addDurationSensor = [&](const char* key, const char* name, const char* field, const char* icon) {
        auto component = components[key].to<JsonObject>();
        component["p"] = "sensor";
        component["name"] = name;
        component["unique_id"] = std::string(config::kDeviceId) + "_" + key;
        component["value_template"] = std::string("{{ value_json.") + field + " }}";
        component["unit_of_measurement"] = "s";
        component["device_class"] = "duration";
        component["state_class"] = "total_increasing";
        component["icon"] = icon;
    };
    addDurationSensor("work_time", "Work time", "work_s", "mdi:briefcase-clock");
    addDurationSensor("play_time", "Play time", "play_s", "mdi:gamepad-variant");
    addDurationSensor("sleep_time", "Sleep time", "sleep_s", "mdi:bed-clock");

    auto activity = components["activity"].to<JsonObject>();
    activity["p"] = "sensor";
    activity["name"] = "Activity";
    activity["unique_id"] = std::string(config::kDeviceId) + "_activity";
    activity["value_template"] = "{{ value_json.activity }}";
    activity["icon"] = "mdi:account-clock";

    auto body = components["body"].to<JsonObject>();
    body["p"] = "sensor";
    body["name"] = "Body condition";
    body["unique_id"] = std::string(config::kDeviceId) + "_body";
    body["value_template"] = "{{ value_json.body }}";
    body["icon"] = "mdi:scale-bathroom";

    auto focusPhase = components["focus_phase"].to<JsonObject>();
    focusPhase["p"] = "sensor";
    focusPhase["name"] = "Focus phase";
    focusPhase["unique_id"] = std::string(config::kDeviceId) + "_focus_phase";
    focusPhase["value_template"] = "{{ value_json.focus_phase }}";
    focusPhase["icon"] = "mdi:timer-outline";

    auto focusRemaining = components["focus_remaining"].to<JsonObject>();
    focusRemaining["p"] = "sensor";
    focusRemaining["name"] = "Focus remaining";
    focusRemaining["unique_id"] = std::string(config::kDeviceId) + "_focus_remaining";
    focusRemaining["value_template"] = "{{ value_json.focus_remaining_s }}";
    focusRemaining["unit_of_measurement"] = "s";
    focusRemaining["device_class"] = "duration";
    focusRemaining["icon"] = "mdi:timer-sand";

    auto meals = components["meals"].to<JsonObject>();
    meals["p"] = "sensor";
    meals["name"] = "Meals logged";
    meals["unique_id"] = std::string(config::kDeviceId) + "_meals";
    meals["value_template"] = "{{ value_json.meals }}";
    meals["state_class"] = "total_increasing";
    meals["icon"] = "mdi:counter";

    auto mood = components["mood"].to<JsonObject>();
    mood["p"] = "sensor";
    mood["name"] = "Mood";
    mood["unique_id"] = std::string(config::kDeviceId) + "_mood";
    mood["value_template"] = "{{ value_json.mood }}";
    mood["icon"] = "mdi:cat";

    auto screen = components["screen"].to<JsonObject>();
    screen["p"] = "sensor";
    screen["name"] = "Screen";
    screen["unique_id"] = std::string(config::kDeviceId) + "_screen";
    screen["value_template"] = "{{ value_json.screen }}";
    screen["entity_category"] = "diagnostic";

    auto signal = components["wifi_signal"].to<JsonObject>();
    signal["p"] = "sensor";
    signal["name"] = "Wi-Fi signal";
    signal["unique_id"] = std::string(config::kDeviceId) + "_wifi_signal";
    signal["value_template"] = "{{ value_json.wifi_rssi }}";
    signal["device_class"] = "signal_strength";
    signal["unit_of_measurement"] = "dBm";
    signal["entity_category"] = "diagnostic";

    auto tailscaleState = components["tailscale_state"].to<JsonObject>();
    tailscaleState["p"] = "sensor";
    tailscaleState["name"] = "Tailscale state";
    tailscaleState["unique_id"] = std::string(config::kDeviceId) + "_tailscale_state";
    tailscaleState["value_template"] = "{{ value_json.tailscale_state }}";
    tailscaleState["icon"] = "mdi:vpn";
    tailscaleState["entity_category"] = "diagnostic";

    auto tailscaleIp = components["tailscale_ip"].to<JsonObject>();
    tailscaleIp["p"] = "sensor";
    tailscaleIp["name"] = "Tailscale IP";
    tailscaleIp["unique_id"] = std::string(config::kDeviceId) + "_tailscale_ip";
    tailscaleIp["value_template"] = "{{ value_json.tailscale_ip }}";
    tailscaleIp["icon"] = "mdi:ip-network";
    tailscaleIp["entity_category"] = "diagnostic";

    auto outside = components["outside_temperature"].to<JsonObject>();
    outside["p"] = "sensor";
    outside["name"] = "Displayed outside temperature";
    outside["unique_id"] = std::string(config::kDeviceId) + "_outside_temperature";
    outside["value_template"] = "{{ value_json.outside_f if value_json.weather_valid else none }}";
    outside["device_class"] = "temperature";
    outside["unit_of_measurement"] = "°F";
    outside["state_class"] = "measurement";

    auto weatherStale = components["weather_stale"].to<JsonObject>();
    weatherStale["p"] = "binary_sensor";
    weatherStale["name"] = "Weather stale";
    weatherStale["unique_id"] = std::string(config::kDeviceId) + "_weather_stale";
    weatherStale["value_template"] = "{{ 'ON' if value_json.weather_stale else 'OFF' }}";
    weatherStale["device_class"] = "problem";
    weatherStale["entity_category"] = "diagnostic";

    auto weatherAge = components["weather_age"].to<JsonObject>();
    weatherAge["p"] = "sensor";
    weatherAge["name"] = "Weather age";
    weatherAge["unique_id"] = std::string(config::kDeviceId) + "_weather_age";
    weatherAge["value_template"] = "{{ value_json.weather_age_s }}";
    weatherAge["unit_of_measurement"] = "s";
    weatherAge["entity_category"] = "diagnostic";

    auto pet = components["pet"].to<JsonObject>();
    pet["p"] = "button";
    pet["name"] = "Pet";
    pet["unique_id"] = std::string(config::kDeviceId) + "_pet";
    pet["command_topic"] = baseTopic_ + "/command/action";
    pet["payload_press"] = "pet";
    pet["icon"] = "mdi:paw";

    const auto addActionButton = [&](const char* key, const char* name, const char* payload, const char* icon) {
        auto component = components[key].to<JsonObject>();
        component["p"] = "button";
        component["name"] = name;
        component["unique_id"] = std::string(config::kDeviceId) + "_" + key;
        component["command_topic"] = baseTopic_ + "/command/action";
        component["payload_press"] = payload;
        component["icon"] = icon;
    };
    addActionButton("feed", "Log meal", "feed", "mdi:food-apple");
    addActionButton("work", "Toggle work", "work", "mdi:briefcase-clock");
    addActionButton("play", "Toggle play", "play", "mdi:gamepad-variant");
    addActionButton("sleep", "Toggle sleep", "sleep", "mdi:bed");
    addActionButton("idle", "Stop activity", "idle", "mdi:stop-circle-outline");

    auto scan = components["scan_wifi"].to<JsonObject>();
    scan["p"] = "button";
    scan["name"] = "Scan nearby Wi-Fi";
    scan["unique_id"] = std::string(config::kDeviceId) + "_scan_wifi";
    scan["command_topic"] = baseTopic_ + "/command/action";
    scan["payload_press"] = "scan";
    scan["icon"] = "mdi:wifi-refresh";

    auto probe = components["probe_gpio"].to<JsonObject>();
    probe["p"] = "button";
    probe["name"] = "Probe GPIO";
    probe["unique_id"] = std::string(config::kDeviceId) + "_probe_gpio";
    probe["command_topic"] = baseTopic_ + "/command/action";
    probe["payload_press"] = "probe";
    probe["icon"] = "mdi:integrated-circuit-chip";

    auto nearby = components["nearby_wifi"].to<JsonObject>();
    nearby["p"] = "sensor";
    nearby["name"] = "Nearby Wi-Fi";
    nearby["unique_id"] = std::string(config::kDeviceId) + "_nearby_wifi";
    nearby["value_template"] = "{{ value_json.nearby_wifi }}";
    nearby["icon"] = "mdi:wifi-marker";

    auto gpio40 = components["gpio40"].to<JsonObject>();
    gpio40["p"] = "binary_sensor";
    gpio40["name"] = "GPIO 40";
    gpio40["unique_id"] = std::string(config::kDeviceId) + "_gpio40";
    gpio40["value_template"] = "{{ 'ON' if value_json.gpio40 else 'OFF' }}";
    gpio40["entity_category"] = "diagnostic";

    auto gpio41 = components["gpio41"].to<JsonObject>();
    gpio41["p"] = "binary_sensor";
    gpio41["name"] = "GPIO 41";
    gpio41["unique_id"] = std::string(config::kDeviceId) + "_gpio41";
    gpio41["value_template"] = "{{ 'ON' if value_json.gpio41 else 'OFF' }}";
    gpio41["entity_category"] = "diagnostic";

    String payload;
    serializeJson(document, payload);
    const std::string discoveryTopic = std::string("homeassistant/device/") + config::kDeviceId + "/config";
    (void)client_.publish(discoveryTopic.c_str(), payload.c_str(), true);
}

void MqttBridge::publishState() {
    if (!client_.connected()) {
        return;
    }
    const auto& state = app_.state();
    JsonDocument document;
    document["name"] = state.cat.name;
    document["level"] = state.cat.level;
    document["xp"] = state.cat.totalXp;
    document["bond"] = state.cat.bond;
    document["food"] = Routine::foodPercent(state.routine);
    document["rest"] = Routine::restPercent(state.routine);
    document["fun"] = Routine::funPercent(state.routine);
    document["energy"] = Routine::energyPercent(state.routine);
    document["body"] = Routine::bodyName(state.routine);
    document["body_balance"] = state.routine.bodyBalance;
    document["activity"] = Routine::activityName(state.routine.activity);
    document["meals"] = state.routine.mealsLogged;
    document["work_s"] = state.routine.workSeconds;
    document["play_s"] = state.routine.playSeconds;
    document["sleep_s"] = state.routine.sleepSeconds;
    document["session_s"] = state.routine.sessionSeconds;
    document["focus_phase"] = state.routine.activity == ActivityMode::Work
        ? (Routine::focusPhase(state.routine) ? "focus" : "break") : "off";
    document["focus_remaining_s"] = Routine::focusRemainingSeconds(state.routine);
    document["mood"] = semanticName(state.cat.mood);
    document["screen"] = semanticName(state.ui.screen);
    document["wifi_rssi"] = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
    document["setup_portal"] = state.device.setupPortalActive;
    document["tailscale_state"] = state.device.tailscaleState;
    document["tailscale_ip"] = state.device.tailscaleIp;
    document["notification_queue"] = state.notificationQueue.size();
    document["mission_complete"] = state.mission.complete;
    document["nearby_wifi"] = state.playground.networkCount;
    document["room_count"] = state.home.roomCount;
    document["pal_message"] = state.home.palMessage;
    if (state.home.roomCount != 0) {
        document["first_room"] = state.home.rooms[0].name;
        document["first_room_temperature_f"] = state.home.rooms[0].temperatureF;
        if (state.home.rooms[0].hasHumidity) {
            document["first_room_humidity"] = state.home.rooms[0].humidityPercent;
        }
    }
    document["gpio40"] = state.lab.pins[0].high;
    document["gpio41"] = state.lab.pins[1].high;
    const bool weatherValid = state.home.updatedAtMs != 0;
    const auto nowMs = monotonicNowMs();
    document["weather_valid"] = weatherValid;
    document["weather_stale"] = !weatherValid || state.home.stale;
    document["weather_age_s"] = weatherValid && nowMs >= state.home.updatedAtMs
        ? (nowMs - state.home.updatedAtMs) / 1000ULL
        : 0ULL;
    if (weatherValid) {
        document["outside_f"] = state.home.outsideTemperatureF;
        document["weather_condition"] = state.home.conditionLabel;
    }
    String payload;
    serializeJson(document, payload);
    (void)client_.publish(topic("state").c_str(), payload.c_str(), true);
}

void MqttBridge::publishAppEvent(const AppEvent& event) {
    if (!client_.connected() || event.type == AppEventType::None) {
        return;
    }
    JsonDocument document;
    document["type"] = semanticName(event.type);
    document["value"] = event.value;
    document["screen"] = semanticName(app_.state().ui.screen);
    String payload;
    serializeJson(document, payload);
    (void)client_.publish(topic("event").c_str(), payload.c_str(), false);
}

void MqttBridge::schedule(DispatchResult result) {
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

std::string MqttBridge::topic(const char* suffix) const {
    return baseTopic_ + '/' + suffix;
}

}  // namespace housecat::integrations

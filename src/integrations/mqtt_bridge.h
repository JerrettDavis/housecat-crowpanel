#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <PubSubClient.h>
#include <WiFiClient.h>

#include "housecat/app/housecat_app.h"

namespace housecat::integrations {

class MqttBridge final {
public:
    explicit MqttBridge(HouseCatApp& app);

    void begin(std::uint64_t nowMs);
    void loop(std::uint64_t nowMs);
    [[nodiscard]] std::optional<DispatchResult> takeDispatchResult();

    void publishState();
    void publishAppEvent(const AppEvent& event);

private:
    void ensureMqtt(std::uint64_t nowMs);
    void onMessage(char* topic, std::uint8_t* payload, unsigned int length);
    void handleNotification(const char* payload, std::uint64_t nowMs);
    void handleHome(const char* payload, std::uint64_t nowMs);
    void handleMission(const char* payload, std::uint64_t nowMs);
    void handleAction(const char* payload, std::uint64_t nowMs);
    void publishDiscovery();
    void schedule(DispatchResult result);

    [[nodiscard]] std::string topic(const char* suffix) const;
    HouseCatApp& app_;
    WiFiClient network_{};
    PubSubClient client_{network_};
    std::optional<DispatchResult> pendingResult_{};
    std::string baseTopic_{};
    std::uint64_t nextMqttAttemptMs_{0};
    std::uint64_t nextStatePublishMs_{0};
    bool wifiWasConnected_{false};
    bool mqttWasConnected_{false};
};

}  // namespace housecat::integrations

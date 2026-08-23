#pragma once

#include <cstdint>
#include <string>
#include <optional>

#include <DNSServer.h>
#include <WebServer.h>
#include "housecat/app/housecat_app.h"

namespace housecat::integrations {

class WifiPortal final {
public:
    explicit WifiPortal(HouseCatApp& app);

    void begin(std::uint64_t nowMs, bool forcePortal = false);
    void loop(std::uint64_t nowMs, bool scanInProgress);
    [[nodiscard]] std::optional<DispatchResult> takeDispatchResult();

    [[nodiscard]] const char* ssid() const noexcept { return ssid_.c_str(); }
    [[nodiscard]] const char* password() const noexcept { return password_.c_str(); }
    [[nodiscard]] const char* tailscaleAuthKey() const noexcept { return tailscaleAuthKey_.c_str(); }
    [[nodiscard]] bool active() const noexcept { return active_; }
    [[nodiscard]] const std::string& accessPointSsid() const noexcept { return accessPointSsid_; }

private:
    void loadCredentials();
    bool saveCredentials(
        const std::string& ssid, const std::string& password, const std::string& tailscaleAuthKey);
    void connect(std::uint64_t nowMs);
    void startPortal();
    void stopPortal();
    void configureRoutes();
    void serveIndex();
    void saveFromRequest();

    DNSServer dns_{};
    WebServer server_{80};
    HouseCatApp& app_;
    std::optional<DispatchResult> pendingResult_{};
    std::string ssid_{};
    std::string password_{};
    std::string fallbackSsid_{};
    std::string fallbackPassword_{};
    std::string tailscaleAuthKey_{};
    std::string accessPointSsid_{};
    std::uint64_t disconnectedSinceMs_{0};
    std::uint64_t nextConnectAttemptMs_{0};
    std::uint64_t applyCredentialsAtMs_{0};
    std::uint64_t forcedPortalUntilMs_{0};
    bool active_{false};
    bool routesConfigured_{false};
    bool nextUsesFallback_{false};
};

}  // namespace housecat::integrations

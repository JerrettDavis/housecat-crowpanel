#include "integrations/tailscale_manager.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include <utility>
#include <ctime>

#include "housecat/config.h"
#include "integrations/wifi_portal.h"

#if defined(HOUSECAT_TAILSCALE)
#include <microlink.h>
#endif

namespace housecat::integrations {
namespace {

constexpr std::uint64_t kRetryMs = 30'000;
constexpr std::uint64_t kPollMs = 1'000;
#if defined(HOUSECAT_TAILSCALE)
const char* stateName(const microlink_state_t state) noexcept {
    switch (state) {
        case ML_STATE_IDLE: return "idle";
        case ML_STATE_WIFI_WAIT: return "wifi-wait";
        case ML_STATE_CONNECTING: return "connecting";
        case ML_STATE_REGISTERING: return "registering";
        case ML_STATE_CONNECTED: return "connected";
        case ML_STATE_RECONNECTING: return "reconnecting";
        case ML_STATE_ERROR: return "error";
    }
    return "unknown";
}
#endif

}  // namespace

TailscaleManager::TailscaleManager(HouseCatApp& app, const WifiPortal& portal)
    : app_(app), portal_(portal) {}

TailscaleManager::~TailscaleManager() {
#if defined(HOUSECAT_TAILSCALE)
    if (handle_ != nullptr) {
        auto* handle = static_cast<microlink_t*>(handle_);
        (void)microlink_stop(handle);
        microlink_destroy(handle);
    }
#endif
}

void TailscaleManager::begin(const std::uint64_t nowMs) {
#if defined(HOUSECAT_TAILSCALE)
    publishStatus(portal_.tailscaleAuthKey()[0] == '\0' ? "needs-key" : "wifi-wait", "", nowMs);
#else
    publishStatus("disabled", "", nowMs);
#endif
}

void TailscaleManager::loop(const std::uint64_t nowMs) {
#if !defined(HOUSECAT_TAILSCALE)
    (void)nowMs;
    return;
#else
    if (nowMs < nextPollMs_) return;
    nextPollMs_ = nowMs + kPollMs;

    const char* authKey = portal_.tailscaleAuthKey();
    if (authKey[0] == '\0') {
        publishStatus("needs-key", "", nowMs);
        return;
    }
    if (WiFi.status() != WL_CONNECTED) {
        publishStatus(handle_ == nullptr ? "wifi-wait" : "reconnecting", "", nowMs);
        return;
    }

    if (handle_ == nullptr) {
        if (!timeSyncRequested_) {
            configTime(0, 0, "time.cloudflare.com", "pool.ntp.org");
            timeSyncRequested_ = true;
        }
        if (std::time(nullptr) < 1'700'000'000) {
            publishStatus("time-sync", "", nowMs);
            return;
        }
        if (nowMs < nextAttemptMs_) return;
        nextAttemptMs_ = nowMs + kRetryMs;
        microlink_config_t config{};
        config.auth_key = authKey;
        config.device_name = housecat::config::kDeviceId;
        config.enable_derp = true;
        config.enable_stun = true;
        config.enable_disco = true;
        config.max_peers = 8;
        config.priority_peer_ip = housecat::config::kTailscaleExitNodeIp[0] == '\0'
            ? 0 : microlink_parse_ip(housecat::config::kTailscaleExitNodeIp);
        config.wifi_tx_power_dbm = 0;
        auto* handle = microlink_init(&config);
        if (handle == nullptr) {
            Serial.println("[tailscale] initialization failed; retrying later");
            publishStatus("init-error", "", nowMs);
            return;
        }
        handle_ = handle;
        if (microlink_start(handle) != ESP_OK) {
            Serial.println("[tailscale] start failed; retrying later");
            microlink_destroy(handle);
            handle_ = nullptr;
            publishStatus("start-error", "", nowMs);
            return;
        }
        Serial.println("[tailscale] client started");
        publishStatus("connecting", "", nowMs);
    }

    auto* handle = static_cast<microlink_t*>(handle_);
    const auto state = microlink_get_state(handle);
    char vpnIp[16]{};
    if (state == ML_STATE_CONNECTED) {
        const auto address = microlink_get_vpn_ip(handle);
        if (address != 0) microlink_ip_to_str(address, vpnIp);
        if (connectedSinceMs_ == 0) connectedSinceMs_ = nowMs;
        if (housecat::config::kTailscaleExitNodeIp[0] != '\0' && !egressChecked_ &&
            nowMs - connectedSinceMs_ >= 10'000 &&
            nowMs >= nextEgressCheckMs_) {
            nextEgressCheckMs_ = nowMs + 30'000;
            HTTPClient probe;
            probe.setConnectTimeout(8'000);
            probe.setTimeout(8'000);
            if (probe.begin("http://1.1.1.1/cdn-cgi/trace")) {
                const int code = probe.GET();
                if (code > 0) {
                    String trace = probe.getString();
                    String publicIp = "unknown";
                    const int ipStart = trace.indexOf("ip=");
                    if (ipStart >= 0) {
                        const int ipEnd = trace.indexOf('\n', ipStart);
                        publicIp = trace.substring(ipStart + 3, ipEnd < 0 ? trace.length() : ipEnd);
                        publicIp.trim();
                    }
                    egressChecked_ = true;
                    Serial.printf("[tailscale] exit-node=%s egress-ip=%s HTTP=%d verified\n",
                                  housecat::config::kTailscaleExitNodeIp, publicIp.c_str(), code);
                } else {
                    Serial.printf("[tailscale] exit-node probe failed: HTTP %d (%s)\n",
                                  code, HTTPClient::errorToString(code).c_str());
                }
                probe.end();
            }
        }
    } else {
        connectedSinceMs_ = 0;
    }
    if (lastState_ != static_cast<int>(state)) {
        lastState_ = static_cast<int>(state);
        Serial.printf("[tailscale] state=%s%s%s\n", stateName(state), vpnIp[0] ? " ip=" : "", vpnIp);
    }
    publishStatus(stateName(state), vpnIp, nowMs);
#endif
}

std::optional<DispatchResult> TailscaleManager::takeDispatchResult() {
    auto result = pendingResult_;
    pendingResult_.reset();
    return result;
}

void TailscaleManager::publishStatus(
    const char* status,
    const char* ipAddress,
    const std::uint64_t nowMs) {
    auto result = app_.updateTailscale(status, ipAddress, nowMs);
    if (result.changed()) pendingResult_ = std::move(result);
}

}  // namespace housecat::integrations

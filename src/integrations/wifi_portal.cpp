#include "integrations/wifi_portal.h"

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>

#include <cstdio>
#include <utility>

#include "housecat/config.h"

namespace housecat::integrations {
namespace {

constexpr std::uint64_t kPortalDelayMs = 30'000;
constexpr std::uint64_t kReconnectMs = 10'000;
constexpr const char* kPreferencesNamespace = "housecat-net";

std::string htmlEscape(const String& input) {
    std::string output;
    output.reserve(input.length() + 16);
    for (std::size_t index = 0; index < input.length(); ++index) {
        const char value = input[index];
        switch (value) {
            case '&': output += "&amp;"; break;
            case '<': output += "&lt;"; break;
            case '>': output += "&gt;"; break;
            case '"': output += "&quot;"; break;
            case '\'': output += "&#39;"; break;
            default: output += value; break;
        }
    }
    return output;
}

}  // namespace

WifiPortal::WifiPortal(HouseCatApp& app) : app_(app) {}

std::optional<DispatchResult> WifiPortal::takeDispatchResult() {
    auto result = pendingResult_;
    pendingResult_.reset();
    return result;
}

void WifiPortal::begin(const std::uint64_t nowMs, const bool forcePortal) {
    loadCredentials();
    const auto suffix = static_cast<unsigned>(ESP.getEfuseMac() & 0xFFFFFFULL);
    char name[32]{};
    std::snprintf(name, sizeof(name), "HouseCat-Setup-%06X", suffix);
    accessPointSsid_ = name;
    configureRoutes();
    disconnectedSinceMs_ = nowMs;
    if (forcePortal) {
        forcedPortalUntilMs_ = nowMs + 10ULL * 60ULL * 1000ULL;
        startPortal();
        connect(nowMs);
    } else if (ssid_.empty()) {
        startPortal();
    } else {
        connect(nowMs);
    }
}

void WifiPortal::loop(const std::uint64_t nowMs, const bool scanInProgress) {
    if (active_) {
        dns_.processNextRequest();
        server_.handleClient();
    }
    if (applyCredentialsAtMs_ != 0 && nowMs >= applyCredentialsAtMs_) {
        applyCredentialsAtMs_ = 0;
        WiFi.disconnect(true, false);
        connect(nowMs);
    }
    if (WiFi.status() == WL_CONNECTED) {
        disconnectedSinceMs_ = 0;
        nextConnectAttemptMs_ = 0;
        if (active_ && (forcedPortalUntilMs_ == 0 || nowMs >= forcedPortalUntilMs_)) stopPortal();
        return;
    }
    if (disconnectedSinceMs_ == 0) disconnectedSinceMs_ = nowMs;
    if (!active_ && (ssid_.empty() || nowMs - disconnectedSinceMs_ >= kPortalDelayMs)) {
        startPortal();
    }
    if (!scanInProgress && !ssid_.empty() && nowMs >= nextConnectAttemptMs_) {
        connect(nowMs);
    }
}

void WifiPortal::loadCredentials() {
    Preferences preferences;
    if (preferences.begin(kPreferencesNamespace, true)) {
        ssid_ = preferences.getString("ssid", "").c_str();
        password_ = preferences.getString("password", "").c_str();
        tailscaleAuthKey_ = preferences.getString("ts_auth", "").c_str();
        preferences.end();
    }
    if (ssid_.empty()) {
        ssid_ = config::kWifiSsid;
        password_ = config::kWifiPassword;
    }
    if (config::kWifiSsid[0] != '\0' && ssid_ != config::kWifiSsid) {
        fallbackSsid_ = config::kWifiSsid;
        fallbackPassword_ = config::kWifiPassword;
    }
    if (tailscaleAuthKey_.empty()) tailscaleAuthKey_ = config::kTailscaleAuthKey;
}

bool WifiPortal::saveCredentials(
    const std::string& ssid,
    const std::string& password,
    const std::string& tailscaleAuthKey) {
    Preferences preferences;
    if (!preferences.begin(kPreferencesNamespace, false)) return false;
    const bool saved = preferences.putString("ssid", ssid.c_str()) == ssid.size()
        && preferences.putString("password", password.c_str()) == password.size()
        && (tailscaleAuthKey.empty()
            || preferences.putString("ts_auth", tailscaleAuthKey.c_str()) == tailscaleAuthKey.size());
    preferences.end();
    if (saved) {
        ssid_ = ssid;
        password_ = password;
        if (!tailscaleAuthKey.empty()) tailscaleAuthKey_ = tailscaleAuthKey;
        fallbackSsid_ = ssid_ == config::kWifiSsid ? std::string{} : config::kWifiSsid;
        fallbackPassword_ = fallbackSsid_.empty() ? std::string{} : config::kWifiPassword;
        nextUsesFallback_ = false;
    }
    return saved;
}

void WifiPortal::connect(const std::uint64_t nowMs) {
    if (ssid_.empty()) return;
    nextConnectAttemptMs_ = nowMs + kReconnectMs;
    WiFi.mode(active_ ? WIFI_AP_STA : WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setHostname(config::kDeviceId);
    const bool useFallback = nextUsesFallback_ && !fallbackSsid_.empty();
    const std::string& candidateSsid = useFallback ? fallbackSsid_ : ssid_;
    const std::string& candidatePassword = useFallback ? fallbackPassword_ : password_;
    nextUsesFallback_ = !fallbackSsid_.empty() && !nextUsesFallback_;
    Serial.printf("[wifi] connecting to %s...\n", candidateSsid.c_str());
    WiFi.begin(candidateSsid.c_str(), candidatePassword.c_str());
}

void WifiPortal::startPortal() {
    if (active_) return;
    WiFi.mode(WIFI_AP_STA);
    const bool started = WiFi.softAP(accessPointSsid_.c_str(), config::kSetupPassword);
    if (!started) {
        Serial.println("[portal] failed to start setup access point");
        return;
    }
    dns_.start(53, "*", WiFi.softAPIP());
    server_.begin();
    active_ = true;
    pendingResult_ = app_.updateProvisioning(
        true, accessPointSsid_, config::kSetupPassword, millis());
    Serial.printf("[portal] connect to %s and open http://192.168.4.1\n", accessPointSsid_.c_str());
}

void WifiPortal::stopPortal() {
    if (!active_) return;
    dns_.stop();
    server_.stop();
    WiFi.softAPdisconnect(true);
    active_ = false;
    pendingResult_ = app_.updateProvisioning(false, {}, {}, millis());
    Serial.println("[portal] setup access point stopped");
}

void WifiPortal::configureRoutes() {
    if (routesConfigured_) return;
    server_.on("/", HTTP_GET, [this] { serveIndex(); });
    server_.on("/save", HTTP_POST, [this] { saveFromRequest(); });
    server_.on("/generate_204", HTTP_GET, [this] { serveIndex(); });
    server_.on("/hotspot-detect.html", HTTP_GET, [this] { serveIndex(); });
    server_.on("/connecttest.txt", HTTP_GET, [this] { serveIndex(); });
    server_.onNotFound([this] {
        server_.sendHeader("Location", "http://192.168.4.1/", true);
        server_.send(302, "text/plain", "House Cat Wi-Fi setup");
    });
    routesConfigured_ = true;
}

void WifiPortal::serveIndex() {
    const std::string current = htmlEscape(WiFi.SSID());
    std::string page = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>House Cat Wi-Fi</title><style>body{font:18px system-ui;max-width:32rem;margin:3rem auto;padding:0 1rem;background:#f5f1e8;color:#20262b}main{background:white;padding:1.5rem;border-radius:1rem;box-shadow:0 4px 24px #0002}label{display:block;margin-top:1rem;font-weight:700}input,button{box-sizing:border-box;width:100%;padding:.8rem;font:inherit;border:1px solid #777;border-radius:.5rem}button{margin-top:1.4rem;background:#273d3b;color:white;font-weight:700}</style></head><body><main><h1>House Cat Wi-Fi</h1><p>Choose the network this pal should use. The setup access point closes after a successful connection.</p><form method="post" action="/save"><label>Wi-Fi name</label><input name="ssid" maxlength="32" required value=")HTML";
    page += current;
    page += R"HTML("><label>Password</label><input name="password" type="password" maxlength="63" autocomplete="new-password"><label>Tailscale auth key (optional)</label><input name="tailscale_key" type="password" maxlength="95" placeholder="Leave blank to keep current key" autocomplete="off"><button type="submit">Save and connect</button></form></main></body></html>)HTML";
    server_.send(200, "text/html; charset=utf-8", page.c_str());
}

void WifiPortal::saveFromRequest() {
    const String requestedSsid = server_.arg("ssid");
    const String requestedPassword = server_.arg("password");
    const String requestedTailscaleKey = server_.arg("tailscale_key");
    if (requestedSsid.length() == 0 || requestedSsid.length() > 32
        || requestedPassword.length() > 63 || requestedTailscaleKey.length() > 95
        || (requestedTailscaleKey.length() != 0 && !requestedTailscaleKey.startsWith("tskey-auth-"))) {
        server_.send(400, "text/plain", "Invalid Wi-Fi name or password length.");
        return;
    }
    // A blank password for the current network means "keep it". This lets a
    // user add or rotate the Tailscale key without having to re-enter the Wi-Fi
    // secret. A different SSID with a blank password remains a valid open
    // network configuration.
    const std::string password = requestedPassword.length() == 0
            && requestedSsid == ssid_.c_str()
        ? password_
        : std::string(requestedPassword.c_str());
    if (!saveCredentials(requestedSsid.c_str(), password, requestedTailscaleKey.c_str())) {
        server_.send(500, "text/plain", "Could not save credentials. Please try again.");
        return;
    }
    server_.send(200, "text/html; charset=utf-8",
        "<!doctype html><meta name=viewport content='width=device-width'><h1>Saved</h1><p>House Cat is connecting. You may close this page.</p>");
    applyCredentialsAtMs_ = millis() + 500ULL;
    forcedPortalUntilMs_ = 0;
}

}  // namespace housecat::integrations

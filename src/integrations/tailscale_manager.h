#pragma once

#include <cstdint>
#include <optional>

#include "housecat/app/housecat_app.h"

namespace housecat::integrations {

class WifiPortal;

class TailscaleManager final {
public:
    TailscaleManager(HouseCatApp& app, const WifiPortal& portal);
    ~TailscaleManager();

    void begin(std::uint64_t nowMs);
    void loop(std::uint64_t nowMs);
    [[nodiscard]] std::optional<DispatchResult> takeDispatchResult();

private:
    void publishStatus(const char* status, const char* ipAddress, std::uint64_t nowMs);

    HouseCatApp& app_;
    const WifiPortal& portal_;
    std::optional<DispatchResult> pendingResult_{};
    void* handle_{nullptr};
    std::uint64_t nextAttemptMs_{0};
    std::uint64_t nextPollMs_{0};
    std::uint64_t connectedSinceMs_{0};
    std::uint64_t nextEgressCheckMs_{0};
    bool egressChecked_{false};
    bool timeSyncRequested_{false};
    int lastState_{-1};
};

}  // namespace housecat::integrations

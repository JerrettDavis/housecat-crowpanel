#pragma once

#include <cstdint>
#include <optional>

#include "housecat/app/housecat_app.h"

namespace housecat::board {

class PlaygroundLab final {
public:
    explicit PlaygroundLab(HouseCatApp& app) : app_(app) {}

    void begin();
    void loop(std::uint64_t nowMs);
    [[nodiscard]] std::optional<DispatchResult> takeDispatchResult();

private:
    void startWifiScan();
    void finishWifiScan(int count, std::uint64_t nowMs);
    void sampleLab(std::uint64_t nowMs);
    void schedule(DispatchResult result);

    HouseCatApp& app_;
    std::optional<DispatchResult> pendingResult_{};
    bool scanRunning_{false};
    bool wasConnectedBeforeScan_{false};
};

}  // namespace housecat::board

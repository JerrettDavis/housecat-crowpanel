#pragma once

#include <cstdint>

#include "housecat/domain/models.h"

namespace housecat {

struct ProgressionResult {
    bool rewarded{false};
    bool levelledUp{false};
    std::uint16_t oldLevel{1};
    std::uint16_t newLevel{1};
};

class Progression final {
public:
    static constexpr std::uint64_t kInteractionRewardCooldownMs = 1800;

    [[nodiscard]] static std::uint32_t xpAtLevel(std::uint16_t level) noexcept;
    [[nodiscard]] static std::uint32_t xpToNextLevel(const CatState& cat) noexcept;
    [[nodiscard]] static std::uint8_t levelProgressPercent(const CatState& cat) noexcept;
    [[nodiscard]] static ProgressionResult pet(CatState& cat, std::uint64_t nowMs) noexcept;
    [[nodiscard]] static ProgressionResult award(
        CatState& cat,
        std::uint16_t xp,
        std::uint8_t bond,
        std::uint64_t nowMs,
        bool enforceCooldown = false) noexcept;

private:
    static void recalculateLevel(CatState& cat) noexcept;
};

}  // namespace housecat

#include "housecat/domain/progression.h"

#include <algorithm>
#include <limits>

namespace housecat {

std::uint32_t Progression::xpAtLevel(const std::uint16_t level) noexcept {
    if (level <= 1) {
        return 0;
    }
    const auto step = static_cast<std::uint64_t>(level - 1);
    const auto required = 25ULL * step * step;
    return static_cast<std::uint32_t>(std::min<std::uint64_t>(
        required, std::numeric_limits<std::uint32_t>::max()));
}

std::uint32_t Progression::xpToNextLevel(const CatState& cat) noexcept {
    if (cat.level >= 99) return 0;
    const auto next = xpAtLevel(static_cast<std::uint16_t>(cat.level + 1));
    return next > cat.totalXp ? next - cat.totalXp : 0;
}

std::uint8_t Progression::levelProgressPercent(const CatState& cat) noexcept {
    const auto floor = xpAtLevel(cat.level);
    const auto ceiling = xpAtLevel(static_cast<std::uint16_t>(cat.level + 1));
    if (ceiling <= floor) {
        return 100;
    }
    const auto clamped = std::min(std::max(cat.totalXp, floor), ceiling);
    return static_cast<std::uint8_t>((clamped - floor) * 100U / (ceiling - floor));
}

ProgressionResult Progression::pet(CatState& cat, const std::uint64_t nowMs) noexcept {
    ++cat.interactions;
    cat.mood = Mood::Happy;
    return award(cat, 3, 2, nowMs, true);
}

ProgressionResult Progression::award(
    CatState& cat,
    const std::uint16_t xp,
    const std::uint8_t bond,
    const std::uint64_t nowMs,
    const bool enforceCooldown) noexcept {
    ProgressionResult result{};
    result.oldLevel = cat.level;
    result.newLevel = cat.level;

    if (enforceCooldown && cat.lastRewardAtMs != 0
        && (nowMs < cat.lastRewardAtMs
            || nowMs - cat.lastRewardAtMs < kInteractionRewardCooldownMs)) {
        return result;
    }

    cat.totalXp = static_cast<std::uint32_t>(std::min<std::uint64_t>(
        static_cast<std::uint64_t>(cat.totalXp) + xp,
        std::numeric_limits<std::uint32_t>::max()));
    cat.bond = static_cast<std::uint8_t>(std::min(100, static_cast<int>(cat.bond) + bond));
    cat.lastRewardAtMs = nowMs;
    result.rewarded = true;

    recalculateLevel(cat);
    result.newLevel = cat.level;
    result.levelledUp = result.newLevel > result.oldLevel;
    return result;
}

void Progression::recalculateLevel(CatState& cat) noexcept {
    cat.level = std::clamp<std::uint16_t>(cat.level, 1, 99);
    while (cat.level < 99 && cat.totalXp >= xpAtLevel(static_cast<std::uint16_t>(cat.level + 1))) {
        ++cat.level;
    }
}

}  // namespace housecat

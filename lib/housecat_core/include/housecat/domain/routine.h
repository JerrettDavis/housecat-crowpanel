#pragma once

#include <cstdint>

#include "housecat/domain/models.h"

namespace housecat {

struct RoutineAdvanceResult {
    std::uint32_t minutesApplied{0};
    bool activityAutoStopped{false};
    bool focusPhaseChanged{false};
};

class Routine final {
public:
    static constexpr std::uint64_t kFocusSeconds = 25ULL * 60ULL;
    static constexpr std::uint64_t kBreakSeconds = 5ULL * 60ULL;
    static constexpr std::uint64_t kFocusCycleSeconds = kFocusSeconds + kBreakSeconds;

    [[nodiscard]] static RoutineAdvanceResult advance(RoutineState& state, std::uint32_t minutes) noexcept;
    static void feed(RoutineState& state) noexcept;
    static void pet(RoutineState& state) noexcept;
    static void setActivity(RoutineState& state, ActivityMode activity) noexcept;

    [[nodiscard]] static std::uint8_t foodPercent(const RoutineState& state) noexcept;
    [[nodiscard]] static std::uint8_t restPercent(const RoutineState& state) noexcept;
    [[nodiscard]] static std::uint8_t funPercent(const RoutineState& state) noexcept;
    [[nodiscard]] static std::uint8_t energyPercent(const RoutineState& state) noexcept;
    [[nodiscard]] static bool focusPhase(const RoutineState& state) noexcept;
    [[nodiscard]] static std::uint64_t focusRemainingSeconds(const RoutineState& state) noexcept;
    [[nodiscard]] static const char* activityName(ActivityMode activity) noexcept;
    [[nodiscard]] static const char* bodyName(const RoutineState& state) noexcept;
};

}  // namespace housecat

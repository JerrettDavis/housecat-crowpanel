#include "housecat/domain/routine.h"

#include <algorithm>
#include <limits>

namespace housecat {
namespace {

constexpr std::uint16_t kMaximumNeed = 10000;
constexpr std::int16_t kMaximumBalance = 10000;

std::uint16_t addNeed(const std::uint16_t value, const std::uint32_t amount) noexcept {
    return static_cast<std::uint16_t>(std::min<std::uint32_t>(kMaximumNeed, value + amount));
}

std::uint16_t removeNeed(const std::uint16_t value, const std::uint32_t amount) noexcept {
    return static_cast<std::uint16_t>(amount >= value ? 0 : value - amount);
}

std::int16_t clampBalance(const std::int32_t value) noexcept {
    return static_cast<std::int16_t>(std::clamp<std::int32_t>(value, -kMaximumBalance, kMaximumBalance));
}

std::uint64_t maximumSessionSeconds(const ActivityMode activity) noexcept {
    switch (activity) {
        case ActivityMode::Work: return 12ULL * 60ULL * 60ULL;
        case ActivityMode::Play: return 8ULL * 60ULL * 60ULL;
        case ActivityMode::Sleep: return 16ULL * 60ULL * 60ULL;
        case ActivityMode::Idle: return std::numeric_limits<std::uint64_t>::max();
    }
    return 0;
}

void applyMinutes(RoutineState& state, const std::uint32_t minutes) noexcept {
    if (minutes == 0) return;
    switch (state.activity) {
        case ActivityMode::Idle:
            state.food = removeNeed(state.food, 5U * minutes);
            state.rest = removeNeed(state.rest, 5U * minutes);
            state.fun = removeNeed(state.fun, 4U * minutes);
            break;
        case ActivityMode::Work:
            state.food = removeNeed(state.food, 6U * minutes);
            state.rest = removeNeed(state.rest, 7U * minutes);
            state.fun = removeNeed(state.fun, 7U * minutes);
            state.workSeconds += static_cast<std::uint64_t>(minutes) * 60ULL;
            break;
        case ActivityMode::Play:
            state.food = removeNeed(state.food, 6U * minutes);
            state.rest = removeNeed(state.rest, 5U * minutes);
            state.fun = addNeed(state.fun, 15U * minutes);
            state.playSeconds += static_cast<std::uint64_t>(minutes) * 60ULL;
            break;
        case ActivityMode::Sleep:
            state.food = removeNeed(state.food, 2U * minutes);
            state.rest = addNeed(state.rest, 20U * minutes);
            state.fun = removeNeed(state.fun, 1U * minutes);
            state.sleepSeconds += static_cast<std::uint64_t>(minutes) * 60ULL;
            break;
    }

    if (state.food < 2000) {
        state.bodyBalance = clampBalance(static_cast<std::int32_t>(state.bodyBalance) - 2 * static_cast<std::int32_t>(minutes));
    } else if (state.food > 9000) {
        state.bodyBalance = clampBalance(static_cast<std::int32_t>(state.bodyBalance) + static_cast<std::int32_t>(minutes));
    } else if (state.bodyBalance > 0 && (state.activity == ActivityMode::Work || state.activity == ActivityMode::Play)) {
        state.bodyBalance = clampBalance(static_cast<std::int32_t>(state.bodyBalance) - static_cast<std::int32_t>(minutes));
    } else if (state.bodyBalance < 0 && state.food >= 4000) {
        state.bodyBalance = clampBalance(static_cast<std::int32_t>(state.bodyBalance) + static_cast<std::int32_t>(minutes));
    }
}

}  // namespace

RoutineAdvanceResult Routine::advance(RoutineState& state, std::uint32_t minutes) noexcept {
    RoutineAdvanceResult result{};
    const bool wasFocus = focusPhase(state);
    while (minutes != 0) {
        const auto maximum = maximumSessionSeconds(state.activity);
        const auto remainingSessionSeconds = maximum > state.sessionSeconds
            ? maximum - state.sessionSeconds
            : 0;
        const auto remainingSessionMinutes = remainingSessionSeconds == std::numeric_limits<std::uint64_t>::max()
            ? minutes
            : static_cast<std::uint32_t>(std::min<std::uint64_t>(minutes, (remainingSessionSeconds + 59ULL) / 60ULL));
        if (remainingSessionMinutes == 0) {
            state.activity = ActivityMode::Idle;
            state.sessionSeconds = 0;
            result.activityAutoStopped = true;
            continue;
        }
        applyMinutes(state, remainingSessionMinutes);
        if (state.activity != ActivityMode::Idle) {
            state.sessionSeconds += static_cast<std::uint64_t>(remainingSessionMinutes) * 60ULL;
        }
        minutes -= remainingSessionMinutes;
        result.minutesApplied += remainingSessionMinutes;
        if (state.activity != ActivityMode::Idle && state.sessionSeconds >= maximum) {
            state.activity = ActivityMode::Idle;
            state.sessionSeconds = 0;
            result.activityAutoStopped = true;
        }
    }
    result.focusPhaseChanged = wasFocus != focusPhase(state);
    return result;
}

void Routine::feed(RoutineState& state) noexcept {
    const bool alreadyFull = state.food >= 8000;
    state.food = addNeed(state.food, 2500);
    state.bodyBalance = clampBalance(static_cast<std::int32_t>(state.bodyBalance) + (alreadyFull ? 1600 : 800));
    if (state.mealsLogged != std::numeric_limits<std::uint32_t>::max()) ++state.mealsLogged;
}

void Routine::pet(RoutineState& state) noexcept { state.fun = addNeed(state.fun, 1000); }

void Routine::setActivity(RoutineState& state, const ActivityMode activity) noexcept {
    state.activity = state.activity == activity ? ActivityMode::Idle : activity;
    state.sessionSeconds = 0;
    if (state.activity == ActivityMode::Play) state.fun = addNeed(state.fun, 500);
}

std::uint8_t Routine::foodPercent(const RoutineState& state) noexcept { return static_cast<std::uint8_t>(state.food / 100); }
std::uint8_t Routine::restPercent(const RoutineState& state) noexcept { return static_cast<std::uint8_t>(state.rest / 100); }
std::uint8_t Routine::funPercent(const RoutineState& state) noexcept { return static_cast<std::uint8_t>(state.fun / 100); }

std::uint8_t Routine::energyPercent(const RoutineState& state) noexcept {
    if (state.food == 0) return 0;
    const auto base = (static_cast<std::uint32_t>(state.food) + static_cast<std::uint32_t>(state.rest)) / 200U;
    const auto imbalance = static_cast<std::uint32_t>(state.bodyBalance < 0 ? -state.bodyBalance : state.bodyBalance) / 250U;
    return static_cast<std::uint8_t>(base > imbalance ? base - imbalance : 0);
}

bool Routine::focusPhase(const RoutineState& state) noexcept {
    return state.activity == ActivityMode::Work && state.sessionSeconds % kFocusCycleSeconds < kFocusSeconds;
}

std::uint64_t Routine::focusRemainingSeconds(const RoutineState& state) noexcept {
    if (state.activity != ActivityMode::Work) return 0;
    const auto position = state.sessionSeconds % kFocusCycleSeconds;
    return position < kFocusSeconds ? kFocusSeconds - position : kFocusCycleSeconds - position;
}

const char* Routine::activityName(const ActivityMode activity) noexcept {
    switch (activity) {
        case ActivityMode::Idle: return "idle";
        case ActivityMode::Work: return "work";
        case ActivityMode::Play: return "play";
        case ActivityMode::Sleep: return "sleep";
    }
    return "idle";
}

const char* Routine::bodyName(const RoutineState& state) noexcept {
    if (state.food == 0 || state.bodyBalance <= -7000) return "wasting";
    if (state.bodyBalance <= -3500) return "frail";
    if (state.bodyBalance >= 7000) return "slothy";
    if (state.bodyBalance >= 3500) return "chunky";
    return "balanced";
}

}  // namespace housecat

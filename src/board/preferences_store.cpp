#include "board/preferences_store.h"

#include <ArduinoJson.h>
#include <Preferences.h>

#include <algorithm>
#include <limits>

namespace housecat::board {
namespace {

void truncate(std::string& value, const std::size_t maximum) {
    if (value.size() > maximum) value.resize(maximum);
}

bool catalogContains(const LibraryState& library, const std::uint32_t id) {
    return std::any_of(library.books.begin(), library.books.end(), [id](const LibraryBook& book) {
        return book.gutenbergId == id;
    });
}

}  // namespace

bool PreferencesStore::load(AppState& state) {
    Preferences preferences;
    if (!preferences.begin("housecat", true)) {
        return false;
    }
    const String json = preferences.getString("state", "");
    preferences.end();
    if (json.length() == 0) {
        return false;
    }

    JsonDocument document;
    if (deserializeJson(document, json) != DeserializationError::Ok) {
        return false;
    }

    const int schema = document["schema"] | 1;
    if (schema < 1 || schema > 3) {
        return false;
    }

    state.cat.name = static_cast<const char*>(document["cat"]["name"] | state.cat.name.c_str());
    const auto level = document["cat"]["level"] | static_cast<int>(state.cat.level);
    const auto xp = document["cat"]["xp"] | static_cast<std::int64_t>(state.cat.totalXp);
    const auto bond = document["cat"]["bond"] | static_cast<int>(state.cat.bond);
    const auto interactions = document["cat"]["interactions"] | static_cast<std::int64_t>(state.cat.interactions);
    state.cat.level = static_cast<std::uint16_t>(std::clamp(level, 1, 99));
    state.cat.totalXp = static_cast<std::uint32_t>(std::clamp<std::int64_t>(
        xp, 0, std::numeric_limits<std::uint32_t>::max()));
    state.cat.bond = static_cast<std::uint8_t>(std::clamp(bond, 0, 100));
    state.cat.interactions = static_cast<std::uint32_t>(std::clamp<std::int64_t>(
        interactions, 0, std::numeric_limits<std::uint32_t>::max()));
    truncate(state.cat.name, limits::kName);

    const int orientation = document["settings"]["orientation"] | 0;
    const int normalizedOrientation = (orientation % 4 + 4) % 4;
    state.settings.orientation = static_cast<Orientation>(normalizedOrientation);
    state.settings.childMode = document["settings"]["child_mode"] | state.settings.childMode;
    state.settings.showControlHints = document["settings"]["control_hints"] | state.settings.showControlHints;
    state.ui.draftOrientation = state.settings.orientation;

    state.mission.id = static_cast<const char*>(document["mission"]["id"] | state.mission.id.c_str());
    state.mission.title = static_cast<const char*>(document["mission"]["title"] | state.mission.title.c_str());
    state.mission.detail = static_cast<const char*>(document["mission"]["detail"] | state.mission.detail.c_str());
    const auto missionProgress = document["mission"]["progress"] | static_cast<int>(state.mission.progress);
    const auto missionTarget = document["mission"]["target"] | static_cast<int>(state.mission.target);
    state.mission.target = static_cast<std::uint16_t>(std::clamp(missionTarget, 1, 65535));
    state.mission.progress = static_cast<std::uint16_t>(std::clamp(
        missionProgress, 0, static_cast<int>(state.mission.target)));
    state.mission.complete = document["mission"]["complete"] | state.mission.complete;
    truncate(state.mission.id, limits::kIdentifier);
    truncate(state.mission.title, limits::kLabel);
    truncate(state.mission.detail, limits::kMessage);
    state.mission.complete = state.mission.complete || state.mission.progress >= state.mission.target;

    const auto selectedBook = document["library"]["selected_book"] | static_cast<int>(state.library.selectedBook);
    state.library.selectedBook = selectedBook >= 0
            && static_cast<std::size_t>(selectedBook) < LibraryState::kCatalogSize
        ? static_cast<std::size_t>(selectedBook)
        : 0;
    state.library.cachedBookId = document["library"]["cached_book_id"] | 0U;
    state.library.bookmarkedBookId = document["library"]["bookmark_book_id"] | 0U;
    state.library.bookmarkedPage = document["library"]["bookmark_page"] | 0U;
    state.library.bookmarkValid = document["library"]["bookmark_valid"] | false;
    if (!catalogContains(state.library, state.library.cachedBookId)) {
        state.library.cachedBookId = 0;
    }
    if (!catalogContains(state.library, state.library.bookmarkedBookId)) {
        state.library.bookmarkValid = false;
        state.library.bookmarkedBookId = 0;
        state.library.bookmarkedPage = 0;
    }
    if (schema >= 3) {
        const auto food = document["routine"]["food"] | static_cast<int>(state.routine.food);
        const auto rest = document["routine"]["rest"] | static_cast<int>(state.routine.rest);
        const auto fun = document["routine"]["fun"] | static_cast<int>(state.routine.fun);
        const auto balance = document["routine"]["body_balance"] | static_cast<int>(state.routine.bodyBalance);
        const auto activity = document["routine"]["activity"] | 0;
        state.routine.food = static_cast<std::uint16_t>(std::clamp(food, 0, 10000));
        state.routine.rest = static_cast<std::uint16_t>(std::clamp(rest, 0, 10000));
        state.routine.fun = static_cast<std::uint16_t>(std::clamp(fun, 0, 10000));
        state.routine.bodyBalance = static_cast<std::int16_t>(std::clamp(balance, -10000, 10000));
        state.routine.activity = activity >= 0 && activity <= 3
            ? static_cast<ActivityMode>(activity) : ActivityMode::Idle;
        state.routine.mealsLogged = document["routine"]["meals"] | 0U;
        state.routine.workSeconds = document["routine"]["work_s"] | 0ULL;
        state.routine.playSeconds = document["routine"]["play_s"] | 0ULL;
        state.routine.sleepSeconds = document["routine"]["sleep_s"] | 0ULL;
        state.routine.sessionSeconds = document["routine"]["session_s"] | 0ULL;
        state.routine.lastUpdateEpochS = document["routine"]["updated_epoch_s"] | 0ULL;
    }
    return true;
}

bool PreferencesStore::save(const AppState& state) {
    JsonDocument document;
    document["schema"] = 3;
    document["cat"]["name"] = state.cat.name;
    document["cat"]["level"] = state.cat.level;
    document["cat"]["xp"] = state.cat.totalXp;
    document["cat"]["bond"] = state.cat.bond;
    document["cat"]["interactions"] = state.cat.interactions;
    document["settings"]["orientation"] = static_cast<int>(state.settings.orientation);
    document["settings"]["child_mode"] = state.settings.childMode;
    document["settings"]["control_hints"] = state.settings.showControlHints;
    document["mission"]["id"] = state.mission.id;
    document["mission"]["title"] = state.mission.title;
    document["mission"]["detail"] = state.mission.detail;
    document["mission"]["progress"] = state.mission.progress;
    document["mission"]["target"] = state.mission.target;
    document["mission"]["complete"] = state.mission.complete;
    document["library"]["selected_book"] = state.library.selectedBook;
    document["library"]["cached_book_id"] = state.library.cachedBookId;
    document["library"]["bookmark_book_id"] = state.library.bookmarkedBookId;
    document["library"]["bookmark_page"] = state.library.bookmarkedPage;
    document["library"]["bookmark_valid"] = state.library.bookmarkValid;
    document["routine"]["food"] = state.routine.food;
    document["routine"]["rest"] = state.routine.rest;
    document["routine"]["fun"] = state.routine.fun;
    document["routine"]["body_balance"] = state.routine.bodyBalance;
    document["routine"]["activity"] = static_cast<int>(state.routine.activity);
    document["routine"]["meals"] = state.routine.mealsLogged;
    document["routine"]["work_s"] = state.routine.workSeconds;
    document["routine"]["play_s"] = state.routine.playSeconds;
    document["routine"]["sleep_s"] = state.routine.sleepSeconds;
    document["routine"]["session_s"] = state.routine.sessionSeconds;
    document["routine"]["updated_epoch_s"] = state.routine.lastUpdateEpochS;

    String json;
    serializeJson(document, json);

    Preferences preferences;
    if (!preferences.begin("housecat", false)) {
        return false;
    }
    const bool saved = preferences.putString("state", json) == json.length();
    preferences.end();
    return saved;
}

bool PreferencesStore::clear() {
    Preferences preferences;
    if (!preferences.begin("housecat", false)) {
        return false;
    }
    const bool cleared = preferences.clear();
    preferences.end();
    Preferences networkPreferences;
    if (!networkPreferences.begin("housecat-net", false)) return false;
    const bool networkCleared = networkPreferences.clear();
    networkPreferences.end();
    return cleared && networkCleared;
}

}  // namespace housecat::board

#include "housecat/app/housecat_app.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <utility>

#include "housecat/domain/progression.h"
#include "housecat/domain/routine.h"

namespace housecat {
namespace {

constexpr std::array<MenuEntry, HouseCatApp::kMenuEntryCount> kMenuEntries{{
    {ScreenId::Cat, IconId::Paw, "MY CAT", "Bond, level, and traits"},
    {ScreenId::Missions, IconId::Mission, "MISSIONS", "Little jobs together"},
    {ScreenId::Whiskers, IconId::Radio, "PLAYGROUND", "Find nearby Wi-Fi"},
    {ScreenId::Library, IconId::Book, "LIBRARY", "Stories and useful cards"},
    {ScreenId::Lab, IconId::Flask, "LAB", "Hardware extensions"},
    {ScreenId::Routine, IconId::Heart, "MY DAY", "Meals, focus, play, and sleep"},
    {ScreenId::Settings, IconId::Gear, "SETTINGS", "Turn and tune your cat"},
}};

DispatchResult changed(const RefreshKind refresh, const AppEventType event = AppEventType::None, std::string value = {}) {
    return {refresh, {event, std::move(value)}};
}

Orientation rotateOrientation(const Orientation orientation, const int delta) noexcept {
    const int value = (static_cast<int>(orientation) + delta + 4) % 4;
    return static_cast<Orientation>(value);
}

void truncate(std::string& value, const std::size_t maximum) {
    if (value.size() > maximum) {
        value.resize(maximum);
    }
}

void normalizeMission(Mission& mission) {
    truncate(mission.id, limits::kIdentifier);
    truncate(mission.title, limits::kLabel);
    truncate(mission.detail, limits::kMessage);
    mission.target = std::max<std::uint16_t>(1, mission.target);
    mission.progress = std::min(mission.progress, mission.target);
    mission.complete = mission.complete || mission.progress >= mission.target;
}

void normalizeNotification(Notification& notification) {
    truncate(notification.id, limits::kIdentifier);
    truncate(notification.title, limits::kLabel);
    truncate(notification.body, limits::kMessage);
}

bool plausibleTemperature(const float value) noexcept {
    return std::isfinite(value) && value >= -100.0F && value <= 150.0F;
}

void normalizeHome(HomeSnapshot& snapshot, const HomeSnapshot& previous) {
    if (!plausibleTemperature(snapshot.outsideTemperatureF)) {
        snapshot.outsideTemperatureF = previous.outsideTemperatureF;
    }
    if (!plausibleTemperature(snapshot.insideTemperatureF)) {
        snapshot.insideTemperatureF = previous.insideTemperatureF;
    }
    snapshot.roomCount = std::min(snapshot.roomCount, snapshot.rooms.size());
    truncate(snapshot.conditionLabel, limits::kLabel);
    truncate(snapshot.palLabel, limits::kLabel);
    truncate(snapshot.palMessage, limits::kMessage);
    std::size_t validRooms = 0;
    for (std::size_t index = 0; index < snapshot.roomCount; ++index) {
        auto room = std::move(snapshot.rooms[index]);
        if (!plausibleTemperature(room.temperatureF)) {
            continue;
        }
        truncate(room.name, limits::kName);
        if (room.name.empty()) room.name = "Room";
        room.hasHumidity = room.hasHumidity && std::isfinite(room.humidityPercent);
        if (room.hasHumidity) {
            room.humidityPercent = std::clamp(room.humidityPercent, 0.0F, 100.0F);
        }
        snapshot.rooms[validRooms++] = std::move(room);
    }
    snapshot.roomCount = validRooms;
}

}  // namespace

HouseCatApp::HouseCatApp() {
    state_.ui.draftOrientation = state_.settings.orientation;
    state_.home.rooms[0] = {"Office", 70.0F, 38.0F, true};
    state_.home.rooms[1] = {"Living", 72.0F, 40.0F, true};
    state_.home.roomCount = 2;
}

const MenuEntry& HouseCatApp::menuEntry(const std::size_t index) noexcept {
    return kMenuEntries[index % kMenuEntries.size()];
}

RefreshKind HouseCatApp::budgetRefresh(const RefreshKind requested) noexcept {
    if (requested == RefreshKind::None) {
        return requested;
    }
    if (requested == RefreshKind::Full) {
        state_.ui.partialRefreshCount = 0;
        return RefreshKind::Full;
    }
    ++state_.ui.partialRefreshCount;
    if (state_.ui.partialRefreshCount >= state_.settings.fullRefreshEvery) {
        state_.ui.partialRefreshCount = 0;
        return RefreshKind::Full;
    }
    return RefreshKind::Partial;
}

DispatchResult HouseCatApp::dispatch(const InputAction action, const std::uint64_t nowMs) {
    DispatchResult result{};

    if (state_.ui.screen == ScreenId::Notification) {
        result = navigateNotification(action, nowMs);
    } else if (action == InputAction::Menu) {
        if (state_.ui.screen == ScreenId::Home) {
            state_.ui.screen = ScreenId::Menu;
            result = changed(RefreshKind::Partial, AppEventType::ScreenChanged, "menu");
        } else {
            state_.ui.screen = ScreenId::Home;
            result = changed(RefreshKind::Partial, AppEventType::ScreenChanged, "home");
        }
    } else if (state_.ui.screen == ScreenId::Home) {
        result = navigateHome(action, nowMs);
    } else if (state_.ui.screen == ScreenId::Menu) {
        result = navigateMenu(action, nowMs);
    } else if (state_.ui.screen == ScreenId::Settings) {
        result = navigateSettings(action, nowMs);
    } else {
        result = navigateDetail(action, nowMs);
    }

    result.refresh = budgetRefresh(result.refresh);
    return result;
}

DispatchResult HouseCatApp::pet(const std::uint64_t nowMs) {
    auto result = performPet(nowMs);
    result.refresh = budgetRefresh(result.refresh);
    return result;
}

DispatchResult HouseCatApp::performPet(const std::uint64_t nowMs) {
    if (state_.ui.screen == ScreenId::Notification) {
        return {};
    }
    const auto progression = Progression::pet(state_.cat, nowMs);
    Routine::pet(state_.routine);
    state_.ui.transientPose = CatPose::Pet;
    state_.ui.transientPoseUntilMs = nowMs + 2800;
    completeWelcomeMission();
    return {
        RefreshKind::Partial,
        {progression.levelledUp ? AppEventType::LevelUp : AppEventType::Pet, state_.cat.name}};
}

DispatchResult HouseCatApp::navigateHome(const InputAction action, const std::uint64_t nowMs) {
    switch (action) {
        case InputAction::Up:
            state_.ui.homeCardIndex = (state_.ui.homeCardIndex + kHomeCardCount - 1) % kHomeCardCount;
            return changed(RefreshKind::Partial);
        case InputAction::Down:
            state_.ui.homeCardIndex = (state_.ui.homeCardIndex + 1) % kHomeCardCount;
            return changed(RefreshKind::Partial);
        case InputAction::Select:
            return performPet(nowMs);
        case InputAction::Back:
        case InputAction::Menu:
            return {};
    }
    return {};
}

DispatchResult HouseCatApp::navigateMenu(const InputAction action, const std::uint64_t /*nowMs*/) {
    switch (action) {
        case InputAction::Up:
            state_.ui.menuIndex = (state_.ui.menuIndex + kMenuEntryCount - 1) % kMenuEntryCount;
            return changed(RefreshKind::Partial);
        case InputAction::Down:
            state_.ui.menuIndex = (state_.ui.menuIndex + 1) % kMenuEntryCount;
            return changed(RefreshKind::Partial);
        case InputAction::Select: {
            const auto& entry = menuEntry(state_.ui.menuIndex);
            state_.ui.screen = entry.screen;
            state_.ui.detailPage = 0;
            if (entry.screen == ScreenId::Settings) {
                state_.ui.draftOrientation = state_.settings.orientation;
            }
            return changed(RefreshKind::Partial, AppEventType::ScreenChanged, entry.title);
        }
        case InputAction::Back:
            state_.ui.screen = ScreenId::Home;
            return changed(RefreshKind::Partial, AppEventType::ScreenChanged, "home");
        case InputAction::Menu:
            return {};
    }
    return {};
}

DispatchResult HouseCatApp::navigateNotification(const InputAction action, const std::uint64_t nowMs) {
    if (!state_.activeNotification.has_value()) {
        state_.ui.screen = ScreenId::Home;
        return changed(RefreshKind::Partial);
    }

    const bool critical = state_.activeNotification->priority == NotificationPriority::Critical;
    if (action == InputAction::Select) {
        const std::string id = state_.activeNotification->id;
        state_.activeNotification.reset();
        state_.ui.screen = state_.ui.screenBeforeNotification;
        const auto next = showNextNotification(nowMs);
        if (next.changed()) {
            return next;
        }
        return changed(RefreshKind::Full, AppEventType::NotificationAcknowledged, id);
    }
    if (action == InputAction::Back && !critical && !state_.activeNotification->requiresAcknowledgement) {
        const std::string id = state_.activeNotification->id;
        state_.activeNotification.reset();
        state_.ui.screen = state_.ui.screenBeforeNotification;
        const auto next = showNextNotification(nowMs);
        if (next.changed()) {
            return next;
        }
        return changed(RefreshKind::Partial, AppEventType::NotificationDismissed, id);
    }
    return {};
}

DispatchResult HouseCatApp::navigateSettings(const InputAction action, const std::uint64_t /*nowMs*/) {
    switch (action) {
        case InputAction::Up:
            state_.ui.draftOrientation = rotateOrientation(state_.ui.draftOrientation, -1);
            return changed(RefreshKind::Partial);
        case InputAction::Down:
            state_.ui.draftOrientation = rotateOrientation(state_.ui.draftOrientation, 1);
            return changed(RefreshKind::Partial);
        case InputAction::Select:
            state_.settings.orientation = state_.ui.draftOrientation;
            state_.ui.screen = ScreenId::Home;
            return changed(RefreshKind::Full, AppEventType::OrientationChanged);
        case InputAction::Back:
            state_.ui.draftOrientation = state_.settings.orientation;
            state_.ui.screen = ScreenId::Menu;
            return changed(RefreshKind::Partial, AppEventType::ScreenChanged, "menu");
        case InputAction::Menu:
            return {};
    }
    return {};
}

DispatchResult HouseCatApp::navigateDetail(const InputAction action, const std::uint64_t nowMs) {
    if (state_.ui.screen == ScreenId::Routine) {
        if (action == InputAction::Up) {
            state_.ui.routineActionIndex =
                (state_.ui.routineActionIndex + kRoutineActionCount - 1) % kRoutineActionCount;
            return changed(RefreshKind::Partial);
        }
        if (action == InputAction::Down) {
            state_.ui.routineActionIndex = (state_.ui.routineActionIndex + 1) % kRoutineActionCount;
            return changed(RefreshKind::Partial);
        }
        if (action == InputAction::Select) {
            switch (state_.ui.routineActionIndex) {
                case 0: return feed(nowMs);
                case 1: return setActivity(ActivityMode::Work, nowMs);
                case 2: return setActivity(ActivityMode::Play, nowMs);
                case 3: return setActivity(ActivityMode::Sleep, nowMs);
            }
        }
        if (action == InputAction::Up || action == InputAction::Down || action == InputAction::Select) {
            return {};
        }
    }
    if (state_.ui.screen == ScreenId::Whiskers) {
        if (action == InputAction::Up && state_.playground.networkCount != 0) {
            state_.playground.selectedIndex =
                (state_.playground.selectedIndex + state_.playground.networkCount - 1)
                % state_.playground.networkCount;
            return changed(RefreshKind::Partial);
        }
        if (action == InputAction::Down && state_.playground.networkCount != 0) {
            state_.playground.selectedIndex =
                (state_.playground.selectedIndex + 1) % state_.playground.networkCount;
            return changed(RefreshKind::Partial);
        }
        if (action == InputAction::Select && !state_.playground.scanning) {
            return requestPlaygroundScan(nowMs);
        }
        if (action == InputAction::Up || action == InputAction::Down || action == InputAction::Select) {
            return {};
        }
    }
    if (state_.ui.screen == ScreenId::Lab) {
        if (action == InputAction::Up || action == InputAction::Down) {
            state_.lab.selectedIndex = state_.lab.selectedIndex == 0 ? 1 : 0;
            return changed(RefreshKind::Partial);
        }
        if (action == InputAction::Select && !state_.lab.probeRequested) {
            return requestLabProbe(nowMs);
        }
    }
    if (state_.ui.screen == ScreenId::Library) {
        auto& library = state_.library;
        if (library.view == LibraryView::Catalog) {
            if (action == InputAction::Up) {
                library.selectedBook = (library.selectedBook + LibraryState::kCatalogSize - 1)
                    % LibraryState::kCatalogSize;
                return changed(RefreshKind::Partial);
            }
            if (action == InputAction::Down) {
                library.selectedBook = (library.selectedBook + 1) % LibraryState::kCatalogSize;
                return changed(RefreshKind::Partial);
            }
            if (action == InputAction::Select) {
                library.view = LibraryView::Downloading;
                library.downloadRequested = true;
                library.error.clear();
                return changed(
                    RefreshKind::Partial,
                    AppEventType::LibraryDownloadRequested,
                    std::to_string(library.books[library.selectedBook].gutenbergId));
            }
        } else if (library.view == LibraryView::Reader) {
            if ((action == InputAction::Up || action == InputAction::Down) && !library.pageRequested) {
                const auto next = action == InputAction::Up
                    ? (library.pageIndex == 0 ? library.pageCount - 1 : library.pageIndex - 1)
                    : (library.pageIndex + 1) % library.pageCount;
                library.requestedPage = next;
                library.pageRequested = true;
                return changed(RefreshKind::Partial, AppEventType::LibraryPageRequested, std::to_string(next));
            }
            if (action == InputAction::Select) {
                library.view = LibraryView::Catalog;
                return changed(RefreshKind::Partial);
            }
        } else if (library.view == LibraryView::Error && action == InputAction::Select) {
            library.view = LibraryView::Catalog;
            library.error.clear();
            return changed(RefreshKind::Partial);
        }
        if (action == InputAction::Back && library.view != LibraryView::Catalog) {
            library.view = LibraryView::Catalog;
            library.downloadRequested = false;
            library.pageRequested = false;
            return changed(RefreshKind::Partial);
        }
        if (action == InputAction::Up || action == InputAction::Down || action == InputAction::Select) {
            return {};
        }
    }
    switch (action) {
        case InputAction::Up:
            state_.ui.detailPage = state_.ui.detailPage == 0 ? 1 : 0;
            return changed(RefreshKind::Partial);
        case InputAction::Down:
            state_.ui.detailPage = state_.ui.detailPage == 0 ? 1 : 0;
            return changed(RefreshKind::Partial);
        case InputAction::Select:
            if (state_.ui.screen == ScreenId::Cat) {
                return performPet(nowMs);
            }
            if (state_.ui.screen == ScreenId::Missions && !state_.mission.complete) {
                state_.mission.progress = std::min<std::uint16_t>(state_.mission.target, state_.mission.progress + 1);
                state_.mission.complete = state_.mission.progress >= state_.mission.target;
                if (state_.mission.complete) {
                    (void)Progression::award(state_.cat, 12, 4, nowMs);
                    state_.cat.mood = Mood::Happy;
                    return changed(RefreshKind::Full, AppEventType::MissionCompleted, state_.mission.id);
                }
                return changed(RefreshKind::Partial);
            }
            // The first-pass module pages deliberately remain safe demos. A click
            // changes the pose and confirms that the interaction was understood.
            state_.ui.transientPose = CatPose::Explorer;
            state_.ui.transientPoseUntilMs = nowMs + 2500;
            return changed(RefreshKind::Partial);
        case InputAction::Back:
            state_.ui.screen = ScreenId::Menu;
            return changed(RefreshKind::Partial, AppEventType::ScreenChanged, "menu");
        case InputAction::Menu:
            return {};
    }
    return {};
}

DispatchResult HouseCatApp::updatePlayground(
    PlaygroundState playground,
    const std::uint64_t /*nowMs*/) {
    playground.scanning = false;
    if (playground.networkCount == 0) {
        playground.selectedIndex = 0;
    } else if (playground.selectedIndex >= playground.networkCount) {
        playground.selectedIndex = playground.networkCount - 1;
    }
    state_.playground = std::move(playground);
    return {
        state_.ui.screen == ScreenId::Whiskers ? budgetRefresh(RefreshKind::Partial) : RefreshKind::None,
        {AppEventType::PlaygroundUpdated, std::to_string(state_.playground.networkCount)}};
}

DispatchResult HouseCatApp::requestPlaygroundScan(const std::uint64_t /*nowMs*/) {
    if (state_.playground.scanning) {
        return {};
    }
    state_.playground.scanning = true;
    return changed(
        state_.ui.screen == ScreenId::Whiskers ? budgetRefresh(RefreshKind::Partial) : RefreshKind::None,
        AppEventType::PlaygroundScanRequested,
        "wifi");
}

DispatchResult HouseCatApp::requestLabProbe(const std::uint64_t /*nowMs*/) {
    if (state_.lab.probeRequested) {
        return {};
    }
    state_.lab.probeRequested = true;
    return changed(
        state_.ui.screen == ScreenId::Lab ? budgetRefresh(RefreshKind::Partial) : RefreshKind::None,
        AppEventType::LabProbeRequested,
        "gpio");
}

DispatchResult HouseCatApp::updateLab(LabState lab, const std::uint64_t /*nowMs*/) {
    lab.probeRequested = false;
    state_.lab = std::move(lab);
    return {
        state_.ui.screen == ScreenId::Lab ? budgetRefresh(RefreshKind::Partial) : RefreshKind::None,
        {AppEventType::LabUpdated, std::to_string(state_.lab.sampleCount)}};
}

DispatchResult HouseCatApp::updateLibraryReady(
    std::string pageText,
    const std::size_t pageIndex,
    const std::size_t pageCount,
    const std::uint64_t /*nowMs*/) {
    if (pageCount == 0) {
        return updateLibraryError("Book has no readable pages.", 0);
    }
    const auto safePageIndex = std::min(pageIndex, pageCount - 1);
    state_.library.view = LibraryView::Reader;
    state_.library.pageText = std::move(pageText);
    state_.library.pageIndex = safePageIndex;
    state_.library.pageCount = pageCount;
    state_.library.requestedPage = safePageIndex;
    state_.library.bookmarkedBookId = state_.library.books[state_.library.selectedBook].gutenbergId;
    state_.library.bookmarkedPage = safePageIndex;
    state_.library.bookmarkValid = true;
    state_.library.downloadRequested = false;
    state_.library.pageRequested = false;
    return {
        state_.ui.screen == ScreenId::Library ? budgetRefresh(RefreshKind::Full) : RefreshKind::None,
        {AppEventType::LibraryUpdated, "ready"}};
}

DispatchResult HouseCatApp::updateLibraryPage(
    std::string pageText,
    const std::size_t pageIndex,
    const std::uint64_t /*nowMs*/) {
    if (state_.library.pageCount == 0) {
        return updateLibraryError("Book has no readable pages.", 0);
    }
    const auto safePageIndex = std::min(pageIndex, state_.library.pageCount - 1);
    state_.library.pageText = std::move(pageText);
    state_.library.pageIndex = safePageIndex;
    state_.library.requestedPage = safePageIndex;
    state_.library.bookmarkedBookId = state_.library.books[state_.library.selectedBook].gutenbergId;
    state_.library.bookmarkedPage = safePageIndex;
    state_.library.bookmarkValid = true;
    state_.library.pageRequested = false;
    return {
        state_.ui.screen == ScreenId::Library ? budgetRefresh(RefreshKind::Partial) : RefreshKind::None,
        {AppEventType::LibraryUpdated, std::to_string(safePageIndex)}};
}

DispatchResult HouseCatApp::updateLibraryError(std::string message, const std::uint64_t /*nowMs*/) {
    state_.library.view = LibraryView::Error;
    state_.library.error = std::move(message);
    state_.library.downloadRequested = false;
    state_.library.pageRequested = false;
    return {
        state_.ui.screen == ScreenId::Library ? budgetRefresh(RefreshKind::Partial) : RefreshKind::None,
        {AppEventType::LibraryUpdated, "error"}};
}

DispatchResult HouseCatApp::receiveNotification(Notification notification, const std::uint64_t nowMs) {
    normalizeNotification(notification);
    if (notification.createdAtMs == 0) {
        notification.createdAtMs = nowMs;
    }
    if (notification.isExpired(nowMs)) {
        return {};
    }

    if (state_.activeNotification.has_value()
        && !notification.id.empty()
        && state_.activeNotification->id == notification.id) {
        state_.activeNotification = std::move(notification);
        state_.cat.mood = state_.activeNotification->priority >= NotificationPriority::Urgent
            ? Mood::Worried
            : Mood::Alert;
        return {budgetRefresh(RefreshKind::Partial), {}};
    }

    if (!state_.activeNotification.has_value()) {
        state_.ui.screenBeforeNotification = state_.ui.screen;
        state_.activeNotification = std::move(notification);
        state_.ui.screen = ScreenId::Notification;
        state_.cat.mood = state_.activeNotification->priority >= NotificationPriority::Urgent
            ? Mood::Worried
            : Mood::Alert;
        return {budgetRefresh(RefreshKind::Full), {AppEventType::NotificationShown, state_.activeNotification->id}};
    }

    if (notification.priority > state_.activeNotification->priority) {
        (void)state_.notificationQueue.push(*state_.activeNotification);
        state_.activeNotification = std::move(notification);
        state_.cat.mood = state_.activeNotification->priority >= NotificationPriority::Urgent
            ? Mood::Worried
            : Mood::Alert;
        return {budgetRefresh(RefreshKind::Full), {AppEventType::NotificationShown, state_.activeNotification->id}};
    }

    (void)state_.notificationQueue.push(notification);
    return {};
}

DispatchResult HouseCatApp::showNextNotification(const std::uint64_t nowMs) {
    state_.notificationQueue.removeExpired(nowMs);
    auto next = state_.notificationQueue.pop();
    if (!next.has_value()) {
        state_.cat.mood = Mood::Content;
        return {};
    }
    state_.activeNotification = std::move(next);
    state_.ui.screen = ScreenId::Notification;
    state_.cat.mood = state_.activeNotification->priority >= NotificationPriority::Urgent
        ? Mood::Worried
        : Mood::Alert;
    return changed(RefreshKind::Full, AppEventType::NotificationShown, state_.activeNotification->id);
}

DispatchResult HouseCatApp::tick(const std::uint64_t nowMs) {
    DispatchResult routineTickResult{};
    if (lastRoutineTickMs_ == 0) {
        lastRoutineTickMs_ = nowMs;
    } else if (nowMs >= lastRoutineTickMs_ + 60000ULL) {
        const auto elapsedMinutes = static_cast<std::uint32_t>(
            std::min<std::uint64_t>((nowMs - lastRoutineTickMs_) / 60000ULL, 30ULL * 24ULL * 60ULL));
        lastRoutineTickMs_ += static_cast<std::uint64_t>(elapsedMinutes) * 60000ULL;
        routineTickResult = advanceRoutine(elapsedMinutes, nowMs);
    }
    state_.notificationQueue.removeExpired(nowMs);
    if (state_.activeNotification.has_value() && state_.activeNotification->isExpired(nowMs)
        && !state_.activeNotification->requiresAcknowledgement) {
        state_.activeNotification.reset();
        state_.ui.screen = state_.ui.screenBeforeNotification;
        const auto next = showNextNotification(nowMs);
        if (next.changed()) {
            return {budgetRefresh(next.refresh), next.event};
        }
        return {budgetRefresh(RefreshKind::Partial), {AppEventType::NotificationDismissed, "expired"}};
    }
    if (state_.ui.transientPoseUntilMs != 0 && nowMs >= state_.ui.transientPoseUntilMs) {
        state_.ui.transientPoseUntilMs = 0;
        state_.cat.mood = Mood::Content;
        return {
            strongerRefresh(budgetRefresh(RefreshKind::Partial), routineTickResult.refresh),
            routineTickResult.event};
    }
    constexpr std::uint64_t kHomeSnapshotMaxAgeMs = 15ULL * 60ULL * 1000ULL;
    if (!state_.home.stale && state_.home.updatedAtMs != 0 && nowMs >= state_.home.updatedAtMs
        && nowMs - state_.home.updatedAtMs >= kHomeSnapshotMaxAgeMs) {
        state_.home.stale = true;
        const bool weatherVisible = state_.ui.screen == ScreenId::Home
            && homeCardUsesHomeSnapshot(state_.ui.homeCardIndex);
        return {
            strongerRefresh(
                weatherVisible ? budgetRefresh(RefreshKind::Partial) : RefreshKind::None,
                routineTickResult.refresh),
            routineTickResult.event};
    }
    return routineTickResult;
}

DispatchResult HouseCatApp::feed(const std::uint64_t nowMs) {
    Routine::feed(state_.routine);
    state_.ui.transientPose = CatPose::Happy;
    state_.ui.transientPoseUntilMs = nowMs + 2500;
    return {budgetRefresh(RefreshKind::Partial), {AppEventType::MealLogged, std::to_string(state_.routine.mealsLogged)}};
}

DispatchResult HouseCatApp::setActivity(const ActivityMode activity, const std::uint64_t /*nowMs*/) {
    Routine::setActivity(state_.routine, activity);
    return {
        budgetRefresh(RefreshKind::Partial),
        {AppEventType::ActivityChanged, Routine::activityName(state_.routine.activity)}};
}

DispatchResult HouseCatApp::advanceRoutine(const std::uint32_t minutes, const std::uint64_t /*nowMs*/) {
    if (minutes == 0) return {};
    const auto result = Routine::advance(state_.routine, minutes);
    if (state_.routine.lastUpdateEpochS != 0) {
        state_.routine.lastUpdateEpochS += static_cast<std::uint64_t>(result.minutesApplied) * 60ULL;
    }
    routineMinutesSinceEvent_ += result.minutesApplied;
    const bool visible = state_.ui.screen == ScreenId::Routine;
    if (result.activityAutoStopped) {
        routineMinutesSinceEvent_ = 0;
        return {
            visible ? budgetRefresh(RefreshKind::Partial) : RefreshKind::None,
            {AppEventType::ActivityChanged, "idle"}};
    }
    if (routineMinutesSinceEvent_ >= 15 || result.focusPhaseChanged) {
        routineMinutesSinceEvent_ %= 15;
        return {
            visible ? budgetRefresh(RefreshKind::Partial) : RefreshKind::None,
            {AppEventType::NeedsUpdated, result.focusPhaseChanged ? "focus_phase" : "timer"}};
    }
    return {visible ? budgetRefresh(RefreshKind::Partial) : RefreshKind::None, {}};
}

DispatchResult HouseCatApp::updateClock(const std::uint64_t epochSeconds, const std::uint64_t nowMs) {
    if (epochSeconds == 0) return {};
    lastRoutineTickMs_ = nowMs;
    if (state_.routine.lastUpdateEpochS == 0 || epochSeconds <= state_.routine.lastUpdateEpochS) {
        state_.routine.lastUpdateEpochS = epochSeconds;
        return {};
    }
    const auto elapsedSeconds = std::min<std::uint64_t>(
        epochSeconds - state_.routine.lastUpdateEpochS, 30ULL * 24ULL * 60ULL * 60ULL);
    const auto elapsedMinutes = static_cast<std::uint32_t>(elapsedSeconds / 60ULL);
    auto result = advanceRoutine(elapsedMinutes, nowMs);
    state_.routine.lastUpdateEpochS = epochSeconds;
    return result;
}

DispatchResult HouseCatApp::updateHome(HomeSnapshot snapshot, const std::uint64_t nowMs) {
    normalizeHome(snapshot, state_.home);
    snapshot.updatedAtMs = nowMs;
    snapshot.stale = false;
    state_.home = std::move(snapshot);
    if (state_.ui.screen == ScreenId::Home && homeCardUsesHomeSnapshot(state_.ui.homeCardIndex)) {
        return {budgetRefresh(RefreshKind::Partial), {}};
    }
    return {};
}

DispatchResult HouseCatApp::updateMission(Mission mission, const std::uint64_t /*nowMs*/) {
    normalizeMission(mission);
    const bool becameComplete = mission.complete && !state_.mission.complete;
    const std::string missionId = mission.id;
    state_.mission = std::move(mission);
    const auto refresh = state_.ui.screen == ScreenId::Missions
            || (state_.ui.screen == ScreenId::Home && state_.ui.homeCardIndex == 2)
        ? budgetRefresh(RefreshKind::Partial)
        : RefreshKind::None;
    return {
        refresh,
        {becameComplete ? AppEventType::MissionCompleted : AppEventType::MissionUpdated, missionId}};
}

DispatchResult HouseCatApp::setConnection(
    const ConnectionState connection,
    const int rssi,
    const std::uint64_t /*nowMs*/) {
    if (state_.device.connection == connection
        && std::abs(state_.device.wifiRssi - rssi) < 5) {
        return {};
    }
    state_.device.connection = connection;
    state_.device.wifiRssi = rssi;
    return {budgetRefresh(RefreshKind::Partial), {}};
}

DispatchResult HouseCatApp::updateProvisioning(
    const bool active,
    std::string ssid,
    std::string password,
    const std::uint64_t /*nowMs*/) {
    truncate(ssid, 32);
    truncate(password, 63);
    if (state_.device.setupPortalActive == active
        && state_.device.setupSsid == ssid
        && state_.device.setupPassword == password) return {};
    state_.device.setupPortalActive = active;
    state_.device.setupSsid = std::move(ssid);
    state_.device.setupPassword = std::move(password);
    return {budgetRefresh(RefreshKind::Full), {}};
}

DispatchResult HouseCatApp::updateTailscale(
    std::string status,
    std::string ipAddress,
    const std::uint64_t /*nowMs*/) {
    truncate(status, 24);
    truncate(ipAddress, 15);
    if (state_.device.tailscaleState == status && state_.device.tailscaleIp == ipAddress) return {};
    state_.device.tailscaleState = std::move(status);
    state_.device.tailscaleIp = std::move(ipAddress);
    return {budgetRefresh(RefreshKind::Partial), {}};
}

CatPose HouseCatApp::currentPose(const std::uint64_t nowMs) const noexcept {
    if (state_.ui.transientPoseUntilMs != 0 && nowMs < state_.ui.transientPoseUntilMs) {
        return state_.ui.transientPose;
    }
    if (state_.routine.food < 1500 || state_.routine.rest < 1000 || state_.routine.fun < 1000) {
        return CatPose::Worried;
    }
    if (state_.routine.activity == ActivityMode::Sleep || state_.routine.bodyBalance >= 7000) {
        return CatPose::Sleepy;
    }
    if (state_.routine.activity == ActivityMode::Work) return CatPose::Curious;
    if (state_.routine.activity == ActivityMode::Play) return CatPose::Happy;
    switch (state_.cat.mood) {
        case Mood::Happy: return CatPose::Happy;
        case Mood::Alert: return CatPose::Alert;
        case Mood::Sleepy: return CatPose::Sleepy;
        case Mood::Curious: return CatPose::Curious;
        case Mood::Worried: return CatPose::Worried;
        case Mood::Content: return CatPose::Content;
    }
    return CatPose::Content;
}

void HouseCatApp::completeWelcomeMission() {
    if (state_.mission.id == "welcome" && !state_.mission.complete) {
        state_.mission.progress = state_.mission.target;
        state_.mission.complete = true;
    }
}

}  // namespace housecat

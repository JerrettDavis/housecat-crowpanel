#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include "housecat/domain/models.h"
#include "housecat/ui/assets.h"

namespace housecat {

enum class ScreenId : std::uint8_t {
    Home,
    Menu,
    Cat,
    Missions,
    Whiskers,
    Library,
    Lab,
    Routine,
    Settings,
    Notification,
};

enum class InputAction : std::uint8_t {
    Up,
    Down,
    Select,
    Menu,
    Back,
};

enum class RefreshKind : std::uint8_t {
    None,
    Partial,
    Full,
};

[[nodiscard]] constexpr RefreshKind strongerRefresh(
    const RefreshKind left,
    const RefreshKind right) noexcept {
    return static_cast<std::uint8_t>(left) >= static_cast<std::uint8_t>(right) ? left : right;
}

enum class AppEventType : std::uint8_t {
    None,
    Pet,
    LevelUp,
    NotificationShown,
    NotificationAcknowledged,
    NotificationDismissed,
    MissionCompleted,
    MissionUpdated,
    OrientationChanged,
    ScreenChanged,
    PlaygroundScanRequested,
    PlaygroundUpdated,
    LabProbeRequested,
    LabUpdated,
    LibraryDownloadRequested,
    LibraryPageRequested,
    LibraryUpdated,
    MealLogged,
    ActivityChanged,
    NeedsUpdated,
};

struct AppEvent {
    AppEventType type{AppEventType::None};
    std::string value;
};

struct DispatchResult {
    RefreshKind refresh{RefreshKind::None};
    AppEvent event{};

    [[nodiscard]] bool changed() const noexcept { return refresh != RefreshKind::None; }
};

struct UiState {
    ScreenId screen{ScreenId::Home};
    ScreenId screenBeforeNotification{ScreenId::Home};
    std::size_t menuIndex{0};
    std::size_t homeCardIndex{0};
    std::size_t detailPage{0};
    std::size_t routineActionIndex{0};
    Orientation draftOrientation{Orientation::Deg0};
    CatPose transientPose{CatPose::Content};
    std::uint64_t transientPoseUntilMs{0};
    std::uint8_t partialRefreshCount{0};
};

struct AppState {
    CatState cat{};
    HomeSnapshot home{};
    Mission mission{};
    PlaygroundState playground{};
    LabState lab{};
    LibraryState library{};
    RoutineState routine{};
    Settings settings{};
    DeviceState device{};
    UiState ui{};
    NotificationQueue notificationQueue{};
    std::optional<Notification> activeNotification{};
};

struct MenuEntry {
    ScreenId screen;
    IconId icon;
    const char* title;
    const char* subtitle;
};

class HouseCatApp final {
public:
    static constexpr std::size_t kMenuEntryCount = 7;
    static constexpr std::size_t kRoutineActionCount = 4;
    static constexpr std::size_t kHomeCardCount = 4;
    static constexpr std::size_t kMissionHomeCard = 2;

    HouseCatApp();

    [[nodiscard]] const AppState& state() const noexcept { return state_; }
    [[nodiscard]] AppState& mutableState() noexcept { return state_; }

    [[nodiscard]] DispatchResult dispatch(InputAction action, std::uint64_t nowMs);
    [[nodiscard]] DispatchResult pet(std::uint64_t nowMs);
    [[nodiscard]] DispatchResult tick(std::uint64_t nowMs);
    [[nodiscard]] DispatchResult receiveNotification(Notification notification, std::uint64_t nowMs);
    [[nodiscard]] DispatchResult updateHome(HomeSnapshot snapshot, std::uint64_t nowMs);
    [[nodiscard]] DispatchResult updateMission(Mission mission, std::uint64_t nowMs);
    [[nodiscard]] DispatchResult updatePlayground(PlaygroundState playground, std::uint64_t nowMs);
    [[nodiscard]] DispatchResult updateLab(LabState lab, std::uint64_t nowMs);
    [[nodiscard]] DispatchResult requestPlaygroundScan(std::uint64_t nowMs);
    [[nodiscard]] DispatchResult requestLabProbe(std::uint64_t nowMs);
    [[nodiscard]] DispatchResult updateLibraryReady(
        std::string pageText, std::size_t pageIndex, std::size_t pageCount, std::uint64_t nowMs);
    [[nodiscard]] DispatchResult updateLibraryPage(
        std::string pageText, std::size_t pageIndex, std::uint64_t nowMs);
    [[nodiscard]] DispatchResult updateLibraryError(std::string message, std::uint64_t nowMs);
    [[nodiscard]] DispatchResult feed(std::uint64_t nowMs);
    [[nodiscard]] DispatchResult setActivity(ActivityMode activity, std::uint64_t nowMs);
    [[nodiscard]] DispatchResult updateClock(std::uint64_t epochSeconds, std::uint64_t nowMs);
    [[nodiscard]] DispatchResult setConnection(ConnectionState connection, int rssi, std::uint64_t nowMs);
    [[nodiscard]] DispatchResult updateProvisioning(
        bool active, std::string ssid, std::string password, std::uint64_t nowMs);
    [[nodiscard]] DispatchResult updateTailscale(
        std::string status, std::string ipAddress, std::uint64_t nowMs);

    [[nodiscard]] static const MenuEntry& menuEntry(std::size_t index) noexcept;
    [[nodiscard]] CatPose currentPose(std::uint64_t nowMs) const noexcept;
    [[nodiscard]] RefreshKind budgetRefresh(RefreshKind requested) noexcept;
    [[nodiscard]] static constexpr bool homeCardUsesHomeSnapshot(const std::size_t index) noexcept {
        return index % kHomeCardCount != kMissionHomeCard;
    }

private:
    [[nodiscard]] DispatchResult performPet(std::uint64_t nowMs);
    [[nodiscard]] DispatchResult navigateHome(InputAction action, std::uint64_t nowMs);
    [[nodiscard]] DispatchResult navigateMenu(InputAction action, std::uint64_t nowMs);
    [[nodiscard]] DispatchResult navigateNotification(InputAction action, std::uint64_t nowMs);
    [[nodiscard]] DispatchResult navigateSettings(InputAction action, std::uint64_t nowMs);
    [[nodiscard]] DispatchResult navigateDetail(InputAction action, std::uint64_t nowMs);
    [[nodiscard]] DispatchResult showNextNotification(std::uint64_t nowMs);
    void completeWelcomeMission();
    [[nodiscard]] DispatchResult advanceRoutine(std::uint32_t minutes, std::uint64_t nowMs);

    AppState state_{};
    std::uint64_t lastRoutineTickMs_{0};
    std::uint32_t routineMinutesSinceEvent_{0};
};

}  // namespace housecat

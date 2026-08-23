#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include "housecat/ui/geometry.h"

namespace housecat {

namespace limits {
inline constexpr std::size_t kName = 24;
inline constexpr std::size_t kLabel = 32;
inline constexpr std::size_t kMessage = 160;
inline constexpr std::size_t kIdentifier = 64;
}  // namespace limits

enum class Mood : std::uint8_t {
    Content,
    Happy,
    Alert,
    Sleepy,
    Curious,
    Worried,
};

enum class WeatherCondition : std::uint8_t {
    Unknown,
    Sunny,
    Cloudy,
    Rainy,
};

enum class ConnectionState : std::uint8_t {
    Offline,
    WifiOnly,
    HomeAssistant,
};

enum class ActivityMode : std::uint8_t {
    Idle,
    Work,
    Play,
    Sleep,
};

enum class NotificationPriority : std::uint8_t {
    Ambient = 0,
    Info = 1,
    Notice = 2,
    Important = 3,
    Urgent = 4,
    Critical = 5,
};

enum class NotificationKind : std::uint8_t {
    Generic,
    Person,
    Vehicle,
    Home,
    Warning,
    Mission,
};

struct CatState {
    std::string name{"Kitty"};
    std::uint16_t level{1};
    std::uint32_t totalXp{0};
    std::uint8_t bond{4};
    std::uint32_t interactions{0};
    Mood mood{Mood::Content};
    std::uint64_t lastRewardAtMs{0};
};

struct RoutineState {
    // Hundredths of a percent retain smooth minute-scale degradation without
    // floating-point persistence drift.
    std::uint16_t food{7000};
    std::uint16_t rest{7000};
    std::uint16_t fun{7000};
    std::int16_t bodyBalance{0};  // -10000 frail, +10000 overfed
    ActivityMode activity{ActivityMode::Idle};
    std::uint32_t mealsLogged{0};
    std::uint64_t workSeconds{0};
    std::uint64_t playSeconds{0};
    std::uint64_t sleepSeconds{0};
    std::uint64_t sessionSeconds{0};
    std::uint64_t lastUpdateEpochS{0};
};

struct RoomReading {
    std::string name;
    float temperatureF{0.0F};
    float humidityPercent{0.0F};
    bool hasHumidity{false};
};

struct HomeSnapshot {
    float outsideTemperatureF{72.0F};
    float insideTemperatureF{71.0F};
    WeatherCondition condition{WeatherCondition::Sunny};
    std::string conditionLabel{"Sunny"};
    std::string palLabel{"PAL"};
    std::string palMessage{"Everything looks comfy."};
    std::array<RoomReading, 4> rooms{};
    std::size_t roomCount{0};
    std::uint64_t updatedAtMs{0};
    bool stale{true};
};

struct Mission {
    std::string id{"welcome"};
    std::string title{"Meet your cat"};
    std::string detail{"Give Kitty a little pet."};
    std::uint16_t progress{0};
    std::uint16_t target{1};
    bool complete{false};
};

struct WirelessNetwork {
    std::string name;
    int rssi{0};
    std::uint8_t channel{0};
    bool secured{false};
};

struct PlaygroundState {
    static constexpr std::size_t kCapacity = 6;
    std::array<WirelessNetwork, kCapacity> networks{};
    std::size_t networkCount{0};
    std::size_t selectedIndex{0};
    bool scanning{false};
    std::uint32_t scanGeneration{0};
};

struct LabPinState {
    std::uint8_t pin{0};
    bool high{true};
};

struct LabState {
    std::array<LabPinState, 2> pins{{{40, true}, {41, true}}};
    std::size_t selectedIndex{0};
    bool probeRequested{false};
    std::uint32_t sampleCount{0};
};

struct LibraryBook {
    std::uint32_t gutenbergId{0};
    std::string title;
    std::string author;
};

enum class LibraryView : std::uint8_t {
    Catalog,
    Downloading,
    Reader,
    Error,
};

struct LibraryState {
    static constexpr std::size_t kCatalogSize = 3;
    std::array<LibraryBook, kCatalogSize> books{{
        {11, "Alice in Wonderland", "Lewis Carroll"},
        {1342, "Pride and Prejudice", "Jane Austen"},
        {1661, "Sherlock Holmes", "Arthur Conan Doyle"},
    }};
    std::size_t selectedBook{0};
    LibraryView view{LibraryView::Catalog};
    std::string pageText;
    std::size_t pageIndex{0};
    std::size_t pageCount{0};
    std::size_t requestedPage{0};
    bool downloadRequested{false};
    bool pageRequested{false};
    std::uint32_t cachedBookId{0};
    std::uint32_t bookmarkedBookId{0};
    std::size_t bookmarkedPage{0};
    bool bookmarkValid{false};
    std::string error;
};

struct Notification {
    std::string id;
    std::string title;
    std::string body;
    NotificationKind kind{NotificationKind::Generic};
    NotificationPriority priority{NotificationPriority::Notice};
    std::uint64_t createdAtMs{0};
    std::uint64_t expiresAtMs{0};
    bool requiresAcknowledgement{false};

    [[nodiscard]] bool isExpired(const std::uint64_t nowMs) const noexcept {
        return expiresAtMs != 0 && nowMs >= expiresAtMs;
    }
};

struct Settings {
    Orientation orientation{Orientation::Deg0};
    bool childMode{true};
    bool showControlHints{true};
    std::uint8_t fullRefreshEvery{10};
};

struct DeviceState {
    ConnectionState connection{ConnectionState::Offline};
    int wifiRssi{0};
    std::uint8_t batteryPercent{100};
    bool externalPower{true};
    bool setupPortalActive{false};
    std::string setupSsid{};
    std::string setupPassword{};
    std::string tailscaleState{"disabled"};
    std::string tailscaleIp{};
};

class NotificationQueue final {
public:
    static constexpr std::size_t kCapacity = 8;

    bool push(const Notification& notification);
    [[nodiscard]] std::optional<Notification> pop() noexcept;
    void removeExpired(std::uint64_t nowMs) noexcept;
    [[nodiscard]] std::size_t size() const noexcept { return count_; }
    [[nodiscard]] bool empty() const noexcept { return count_ == 0; }

private:
    std::array<Notification, kCapacity> items_{};
    std::size_t count_{0};
};

}  // namespace housecat

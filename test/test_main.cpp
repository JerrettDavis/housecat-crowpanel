#include <cstdint>
#include <exception>
#include <limits>
#include <iostream>
#include <stdexcept>
#include <string>

#include "housecat/app/housecat_app.h"
#include "housecat/app/semantics.h"
#include "housecat/domain/progression.h"
#include "housecat/domain/routine.h"
#include "housecat/ui/mono_canvas.h"
#include "housecat/ui/renderer.h"
#include "board/jd79661_frame.h"

namespace {

int failures = 0;

void expect(const bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

std::size_t blackPixelCount(const housecat::MonoCanvas& canvas) {
    std::size_t count = 0;
    for (int y = 0; y < canvas.height(); ++y) {
        for (int x = 0; x < canvas.width(); ++x) {
            if (canvas.pixel(x, y)) {
                ++count;
            }
        }
    }
    return count;
}



std::size_t blackPixelsIn(const housecat::MonoCanvas& canvas, int x, int y, int width, int height) {
    std::size_t count = 0;
    for (int py = y; py < y + height && py < canvas.height(); ++py) {
        for (int px = x; px < x + width && px < canvas.width(); ++px) {
            if (px >= 0 && py >= 0 && canvas.pixel(px, py)) {
                ++count;
            }
        }
    }
    return count;
}

void testOrientation() {
    housecat::MonoCanvas canvas;
    canvas.clear(false);
    canvas.setOrientation(housecat::Orientation::Deg0);
    canvas.setPixel(0, 0, true);
    expect(canvas.pixel(0, 0), "0 degree origin should round-trip");

    canvas.clear(false);
    canvas.setOrientation(housecat::Orientation::Deg90);
    expect(canvas.width() == 250 && canvas.height() == 122, "90 degree logical dimensions");
    canvas.setPixel(249, 121, true);
    expect(canvas.pixel(249, 121), "90 degree lower-right should round-trip");

    canvas.clear(false);
    canvas.setOrientation(housecat::Orientation::Deg270);
    canvas.setPixel(0, 0, true);
    expect(canvas.pixel(0, 0), "270 degree origin should round-trip");
}

void testPanelEncoding() {
    housecat::MonoCanvas canvas;
    canvas.clear(false);
    canvas.setPixel(0, 0, true);
    canvas.setPixel(121, 249, true);
    housecat::board::Jd79661Frame frame{};
    housecat::board::encodeJd79661Frame(canvas, frame);

    const auto bottomRow = static_cast<std::size_t>(249 * housecat::MonoCanvas::kStride);
    expect((frame[bottomRow] & 0x80U) == 0, "top-left canvas ink should map to reversed bottom row");
    expect((frame[15] & 0x40U) == 0, "bottom-right visible ink should map to the top row");
    expect((frame[15] & 0x3FU) == 0x3FU, "hidden JD79661 columns should always remain white");
    expect(frame[0] == 0xFFU, "untouched visible panel bytes should remain white");
}

void testProgression() {
    housecat::CatState cat{};
    auto result = housecat::Progression::award(cat, 25, 5, 1000);
    expect(result.levelledUp, "25 XP should reach level 2");
    expect(cat.level == 2, "cat level should be 2");
    const auto firstXp = cat.totalXp;
    result = housecat::Progression::pet(cat, 1100);
    expect(!result.rewarded, "rapid pet should not farm XP");
    expect(cat.totalXp == firstXp, "cooldown should preserve XP");
    result = housecat::Progression::pet(cat, 4000);
    expect(result.rewarded, "pet after cooldown should reward XP");

    cat.totalXp = UINT32_MAX - 1;
    (void)housecat::Progression::award(cat, 25, 0, 5000);
    expect(cat.totalXp == UINT32_MAX, "XP awards should saturate instead of wrapping");
    cat.lastRewardAtMs = 10'000;
    const auto rewoundClock = housecat::Progression::pet(cat, 100);
    expect(!rewoundClock.rewarded, "a rewound monotonic clock must not bypass reward cooldown");
    expect(housecat::Progression::xpAtLevel(UINT16_MAX) == UINT32_MAX,
           "out-of-domain level calculations should saturate safely");
}


void testRefreshBudget() {
    expect(housecat::strongerRefresh(housecat::RefreshKind::Partial, housecat::RefreshKind::Full)
               == housecat::RefreshKind::Full,
           "refresh composition should retain the stronger request");
    housecat::HouseCatApp app;
    for (int index = 0; index < 9; ++index) {
        expect(app.budgetRefresh(housecat::RefreshKind::Partial) == housecat::RefreshKind::Partial,
               "the first nine partial updates should remain partial");
    }
    expect(app.budgetRefresh(housecat::RefreshKind::Partial) == housecat::RefreshKind::Full,
           "the configured tenth partial update should become a full refresh");
    expect(app.budgetRefresh(housecat::RefreshKind::Partial) == housecat::RefreshKind::Partial,
           "the refresh budget should restart after the automatic full refresh");
    expect(app.budgetRefresh(housecat::RefreshKind::Full) == housecat::RefreshKind::Full,
           "an explicit full refresh should remain full");
}

void testSemanticMappings() {
    expect(std::string(housecat::semanticName(housecat::ScreenId::Library)) == "library",
           "shared screen names should remain stable across adapters");
    expect(std::string(housecat::semanticName(housecat::AppEventType::LibraryUpdated)) == "library_updated",
           "shared event names should remain stable across adapters");
    expect(housecat::weatherConditionFrom("clear-night") == housecat::WeatherCondition::Sunny,
           "Home Assistant clear-night should use the clear-weather icon");
    expect(housecat::weatherConditionFrom("pouring") == housecat::WeatherCondition::Rainy,
           "Home Assistant pouring should use the rain icon");
    expect(housecat::notificationPriorityFrom("critical") == housecat::NotificationPriority::Critical,
           "semantic priority parsing should preserve critical alerts");
}

void testMissionUpdates() {
    housecat::HouseCatApp app;
    constexpr std::uint64_t now = 15000;

    housecat::Mission mission{};
    mission.id = "charging";
    mission.title = "Watch the Ioniq";
    mission.detail = "Wait until charging is complete.";
    mission.progress = 1;
    mission.target = 3;
    mission.complete = false;

    const auto updated = app.updateMission(mission, now);
    expect(updated.refresh == housecat::RefreshKind::None,
           "an off-screen mission update should avoid unnecessary e-paper refresh");
    expect(updated.event.type == housecat::AppEventType::MissionUpdated,
           "every mission mutation should emit a persistence-capable update event");
    expect(updated.event.value == "charging", "mission update event should carry the mission id");
    expect(app.state().mission.progress == 1, "mission state should be replaced by the update");

    mission.progress = 3;
    mission.complete = true;
    const auto completed = app.updateMission(mission, now + 1);
    expect(completed.event.type == housecat::AppEventType::MissionCompleted,
           "the first transition to complete should emit mission_completed");
}

void testNavigation() {
    housecat::HouseCatApp app;
    constexpr std::uint64_t now = 10000;
    expect(app.state().ui.screen == housecat::ScreenId::Home, "app starts at Home");
    (void)app.dispatch(housecat::InputAction::Menu, now);
    expect(app.state().ui.screen == housecat::ScreenId::Menu, "Menu key opens menu");
    (void)app.dispatch(housecat::InputAction::Down, now + 1);
    (void)app.dispatch(housecat::InputAction::Select, now + 2);
    expect(app.state().ui.screen == housecat::ScreenId::Missions, "rocker selects menu item");
    (void)app.dispatch(housecat::InputAction::Back, now + 3);
    expect(app.state().ui.screen == housecat::ScreenId::Menu, "Back retreats one level");
    (void)app.dispatch(housecat::InputAction::Menu, now + 4);
    expect(app.state().ui.screen == housecat::ScreenId::Home, "Menu returns home outside Home");

    for (std::size_t index = 0; index < housecat::HouseCatApp::kHomeCardCount; ++index) {
        (void)app.dispatch(housecat::InputAction::Down, now + 5 + index);
    }
    expect(app.state().ui.homeCardIndex == 0, "Home cards should form one declarative four-card cycle");
    (void)app.dispatch(housecat::InputAction::Up, now + 10);
    expect(app.state().ui.homeCardIndex == housecat::HouseCatApp::kHomeCardCount - 1,
           "Home rocker Up should wrap to the Pal card");
}

void testHomeFreshness() {
    housecat::HouseCatApp app;
    constexpr std::uint64_t now = 50'000;
    expect(app.state().home.stale, "weather starts stale until a real snapshot arrives");

    housecat::HomeSnapshot snapshot{};
    snapshot.outsideTemperatureF = 81.5F;
    (void)app.updateHome(snapshot, now);
    expect(!app.state().home.stale, "a live home snapshot clears the stale flag");
    expect(app.state().home.updatedAtMs == now, "a live home snapshot records receipt time");

    const auto earlierLoopTimestamp = app.tick(now - 1);
    expect(!app.state().home.stale && !earlierLoopTimestamp.changed(),
           "a loop timestamp just before an MQTT receipt must not underflow weather age");

    const auto fresh = app.tick(now + 14ULL * 60ULL * 1000ULL);
    expect(!app.state().home.stale && !fresh.changed(), "weather remains fresh for fourteen minutes");
    const auto expired = app.tick(now + 15ULL * 60ULL * 1000ULL);
    expect(app.state().home.stale, "weather is marked stale after fifteen minutes without an update");
    expect(expired.changed(), "visible weather repaints when it becomes stale");

    app.mutableState().ui.homeCardIndex = housecat::HouseCatApp::kHomeCardCount - 1;
    snapshot.palMessage = "Good evening! Home feels cozy.";
    const auto palUpdated = app.updateHome(snapshot, now + 16ULL * 60ULL * 1000ULL);
    expect(palUpdated.changed(), "a live snapshot should repaint the visible Pal card");

    app.mutableState().ui.homeCardIndex = housecat::HouseCatApp::kMissionHomeCard;
    const auto hiddenHomeUpdate = app.updateHome(snapshot, now + 17ULL * 60ULL * 1000ULL);
    expect(!hiddenHomeUpdate.changed(), "home telemetry should not repaint the independent Mission card");
}

void testDomainNormalization() {
    housecat::HouseCatApp app;
    housecat::HomeSnapshot home{};
    home.outsideTemperatureF = std::numeric_limits<float>::quiet_NaN();
    home.insideTemperatureF = 999.0F;
    home.palMessage.assign(housecat::limits::kMessage + 20, 'x');
    home.roomCount = 99;
    home.rooms[0] = {"", std::numeric_limits<float>::infinity(), 0.0F, false};
    home.rooms[1] = {std::string(housecat::limits::kName + 10, 'r'), 72.0F, 140.0F, true};
    home.rooms[2].temperatureF = std::numeric_limits<float>::infinity();
    home.rooms[3].temperatureF = std::numeric_limits<float>::infinity();
    (void)app.updateHome(home, 1'000);
    expect(app.state().home.outsideTemperatureF == 72.0F
               && app.state().home.insideTemperatureF == 71.0F,
           "invalid temperatures should retain the last plausible snapshot");
    expect(app.state().home.roomCount == 1 && app.state().home.rooms[0].name.size() == housecat::limits::kName,
           "invalid rooms should be removed and labels bounded");
    expect(app.state().home.rooms[0].humidityPercent == 100.0F
               && app.state().home.palMessage.size() == housecat::limits::kMessage,
           "humidity and messages should be normalized at the application boundary");

    housecat::Mission mission{};
    mission.title.assign(housecat::limits::kLabel + 5, 'm');
    mission.target = 0;
    mission.progress = 50;
    (void)app.updateMission(mission, 1'001);
    expect(app.state().mission.target == 1 && app.state().mission.progress == 1
               && app.state().mission.complete,
           "missions should have a positive target and internally consistent completion");

    housecat::Notification notification{};
    notification.id.assign(housecat::limits::kIdentifier + 5, 'i');
    notification.title.assign(housecat::limits::kLabel + 5, 't');
    notification.body.assign(housecat::limits::kMessage + 5, 'b');
    (void)app.receiveNotification(notification, 1'002);
    expect(app.state().activeNotification->id.size() == housecat::limits::kIdentifier
               && app.state().activeNotification->title.size() == housecat::limits::kLabel
               && app.state().activeNotification->body.size() == housecat::limits::kMessage,
           "notification payloads should be bounded before entering durable UI state");
}

void testPlaygroundAndLab() {
    housecat::HouseCatApp app;
    constexpr std::uint64_t now = 12000;

    app.mutableState().ui.screen = housecat::ScreenId::Whiskers;
    const auto scan = app.dispatch(housecat::InputAction::Select, now);
    expect(scan.event.type == housecat::AppEventType::PlaygroundScanRequested,
           "Playground click should request a passive Wi-Fi scan");
    expect(app.state().playground.scanning, "Playground should show its scanning state immediately");

    housecat::PlaygroundState playground{};
    playground.networkCount = 2;
    playground.networks[0] = {"HouseCat-IoT", -51, 6, true};
    playground.networks[1] = {"Guest", -72, 11, false};
    playground.scanGeneration = 1;
    const auto updated = app.updatePlayground(playground, now + 1);
    expect(updated.event.type == housecat::AppEventType::PlaygroundUpdated,
           "completed scan should emit a semantic update");
    (void)app.dispatch(housecat::InputAction::Down, now + 2);
    expect(app.state().playground.selectedIndex == 1,
           "rocker should browse discovered Playground networks");

    app.mutableState().ui.screen = housecat::ScreenId::Lab;
    const auto choose = app.dispatch(housecat::InputAction::Down, now + 3);
    expect(choose.changed() && app.state().lab.selectedIndex == 1,
           "Lab rocker should choose between GPIO 40 and 41");
    const auto probe = app.dispatch(housecat::InputAction::Select, now + 4);
    expect(probe.event.type == housecat::AppEventType::LabProbeRequested,
           "Lab click should request a read-only pin probe");
    expect(app.state().lab.probeRequested, "Lab should expose a pending probe state");

    auto lab = app.state().lab;
    lab.pins[0].high = false;
    lab.pins[1].high = true;
    lab.sampleCount = 1;
    const auto sampled = app.updateLab(lab, now + 5);
    expect(sampled.event.type == housecat::AppEventType::LabUpdated,
           "Lab sample should emit a semantic update");
    expect(!app.state().lab.probeRequested && !app.state().lab.pins[0].high,
           "Lab update should clear the request and retain sampled levels");

    app.mutableState().ui.screen = housecat::ScreenId::Home;
    const auto remoteScan = app.requestPlaygroundScan(now + 6);
    expect(remoteScan.event.type == housecat::AppEventType::PlaygroundScanRequested,
           "remote scan should work without navigating to Playground");
    expect(remoteScan.refresh == housecat::RefreshKind::None,
           "background scan should not repaint an unrelated screen");
    const auto remoteProbe = app.requestLabProbe(now + 7);
    expect(remoteProbe.event.type == housecat::AppEventType::LabProbeRequested,
           "remote probe should work without navigating to Lab");
}

void testLibraryReader() {
    housecat::HouseCatApp app;
    constexpr std::uint64_t now = 20'000;
    app.mutableState().ui.screen = housecat::ScreenId::Library;

    const auto nextBook = app.dispatch(housecat::InputAction::Down, now);
    expect(nextBook.changed() && app.state().library.selectedBook == 1,
           "Library rocker should browse the curated catalog");
    const auto open = app.dispatch(housecat::InputAction::Select, now + 1);
    expect(open.event.type == housecat::AppEventType::LibraryDownloadRequested,
           "Library click should request the selected public-domain book");
    expect(app.state().library.view == housecat::LibraryView::Downloading,
           "Library should immediately show download progress");

    const auto ready = app.updateLibraryReady("It is a truth universally acknowledged.", 0, 12, now + 2);
    expect(ready.event.type == housecat::AppEventType::LibraryUpdated,
           "download completion should open the reader");
    expect(app.state().library.pageCount == 12 && app.state().library.pageIndex == 0,
           "reader should start on the first indexed page");

    const auto request = app.dispatch(housecat::InputAction::Down, now + 3);
    expect(request.event.type == housecat::AppEventType::LibraryPageRequested,
           "reader rocker should request the next page");
    (void)app.updateLibraryPage("Some other page.", 1, now + 4);
    expect(app.state().library.pageIndex == 1 && app.state().library.pageText == "Some other page.",
           "loaded reader page should replace the visible text");
    expect(app.state().library.bookmarkValid
               && app.state().library.bookmarkedBookId == 1342
               && app.state().library.bookmarkedPage == 1,
           "reader progress should become a durable resume bookmark");

    (void)app.dispatch(housecat::InputAction::Back, now + 5);
    expect(app.state().library.view == housecat::LibraryView::Catalog,
           "Back should return from the reader to the catalog");

    const auto invalid = app.updateLibraryReady("", 99, 0, now + 6);
    expect(invalid.event.type == housecat::AppEventType::LibraryUpdated
               && app.state().library.view == housecat::LibraryView::Error,
           "an empty page index should fail safely instead of underflowing");

    const auto clamped = app.updateLibraryReady("Last page", 99, 3, now + 7);
    expect(clamped.changed() && app.state().library.pageIndex == 2,
           "reader page indices should be clamped to the indexed book");
}

void testNotificationQueue() {
    housecat::NotificationQueue queue;
    constexpr std::uint64_t now = 1'000;
    for (std::size_t index = 0; index < housecat::NotificationQueue::kCapacity; ++index) {
        housecat::Notification item{};
        item.id = "ambient-" + std::to_string(index);
        item.priority = housecat::NotificationPriority::Ambient;
        expect(queue.push(item), "queue should accept entries up to capacity");
    }

    housecat::Notification rejected{};
    rejected.id = "another-low";
    rejected.priority = housecat::NotificationPriority::Ambient;
    expect(!queue.push(rejected), "a full queue should reject a non-preempting item");

    housecat::Notification urgent{};
    urgent.id = "urgent";
    urgent.priority = housecat::NotificationPriority::Urgent;
    expect(queue.push(urgent), "a higher-priority item should displace the full queue tail");
    const auto first = queue.pop();
    expect(first.has_value() && first->id == "urgent", "queue should pop highest priority first");

    housecat::Notification expiring{};
    expiring.id = "expiring";
    expiring.expiresAtMs = now;
    expiring.priority = housecat::NotificationPriority::Important;
    expect(queue.push(expiring), "queue should accept an expiring item");
    queue.removeExpired(now);
    while (const auto item = queue.pop()) {
        expect(item->id != "expiring", "expiration should remove queued items deterministically");
    }

    housecat::Notification original{};
    original.id = "same";
    original.priority = housecat::NotificationPriority::Info;
    expect(queue.push(original), "queue should accept a deduplication seed");
    original.priority = housecat::NotificationPriority::Critical;
    original.title = "updated";
    expect(queue.push(original) && queue.size() == 1, "same-id retries should replace rather than duplicate");
    const auto updated = queue.pop();
    expect(updated.has_value() && updated->title == "updated",
           "same-id retry should retain its newest payload");
}

void testNotifications() {
    housecat::HouseCatApp app;
    constexpr std::uint64_t now = 20000;
    housecat::Notification low{"low", "Washer done", "Clothes are ready.", housecat::NotificationKind::Home,
                               housecat::NotificationPriority::Info, now, now + 10000, false};
    housecat::Notification high{"high", "Leak detected", "Check the kitchen.", housecat::NotificationKind::Warning,
                                housecat::NotificationPriority::Urgent, now, now + 10000, true};
    (void)app.receiveNotification(low, now);
    (void)app.receiveNotification(high, now + 1);
    expect(app.state().activeNotification.has_value(), "notification should be active");
    expect(app.state().activeNotification->id == "high", "higher priority should preempt");
    const auto xpBeforeAlertPet = app.state().cat.totalXp;
    const auto petDuringAlert = app.pet(now + 1);
    expect(!petDuringAlert.changed(), "remote pet must not acknowledge or visually replace an alert");
    expect(app.state().cat.totalXp == xpBeforeAlertPet, "alert-safe pet should not change progression");
    (void)app.dispatch(housecat::InputAction::Back, now + 2);
    expect(app.state().activeNotification->id == "high", "required urgent alert cannot be dismissed with Back");
    (void)app.dispatch(housecat::InputAction::Select, now + 3);
    expect(app.state().activeNotification.has_value() && app.state().activeNotification->id == "low",
           "acknowledging should reveal queued alert");

    auto updatedLow = low;
    updatedLow.body = "Updated clothes message.";
    (void)app.receiveNotification(updatedLow, now + 4);
    expect(app.state().notificationQueue.empty(), "retrying the active notification should update, not duplicate");
    expect(app.state().activeNotification->body == "Updated clothes message.", "active retry should refresh content");

    app.mutableState().activeNotification->expiresAtMs = now + 5;
    const auto expired = app.tick(now + 6);
    expect(expired.event.type == housecat::AppEventType::NotificationDismissed,
           "an expired non-required notification should dismiss itself");
    expect(!app.state().activeNotification.has_value(), "expired notification should leave the screen");
}

void testRendering() {
    housecat::HouseCatApp app;
    housecat::MonoCanvas canvas;
    housecat::UiRenderer renderer;
    renderer.render(app, 10000, canvas);
    expect(blackPixelCount(canvas) > 1800, "portrait Home should render substantial imagery");

    app.mutableState().settings.orientation = housecat::Orientation::Deg90;
    app.mutableState().ui.screen = housecat::ScreenId::Menu;
    renderer.render(app, 10000, canvas);
    expect(canvas.width() == 250 && canvas.height() == 122, "landscape render dimensions");
    expect(blackPixelCount(canvas) > 1500, "landscape Menu should render substantial imagery");

    app.mutableState().ui.screen = housecat::ScreenId::Home;
    app.mutableState().settings.orientation = housecat::Orientation::Deg0;
    app.mutableState().ui.homeCardIndex = housecat::HouseCatApp::kHomeCardCount - 1;
    app.mutableState().home.palMessage.clear();
    housecat::MonoCanvas emptyPal;
    renderer.render(app, 10000, emptyPal);
    app.mutableState().home.palMessage = "Supercalifragilisticexpialidocious still wraps safely.";
    housecat::MonoCanvas longPal;
    renderer.render(app, 10000, longPal);
    const housecat::Rect palTextBounds{13, 160, 96, 48};
    std::size_t changedInside = 0;
    for (int y = 0; y < longPal.height(); ++y) {
        for (int x = 0; x < longPal.width(); ++x) {
            if (emptyPal.pixel(x, y) == longPal.pixel(x, y)) continue;
            expect(palTextBounds.contains(x, y), "wrapped text must not paint outside its component bounds");
            if (palTextBounds.contains(x, y)) ++changedInside;
        }
    }
    expect(changedInside > 0, "long-word wrapping should still render visible text");

    for (const auto orientation : {housecat::Orientation::Deg0, housecat::Orientation::Deg90}) {
        app.mutableState().settings.orientation = orientation;
        for (std::size_t card = 0; card < housecat::HouseCatApp::kHomeCardCount; ++card) {
            app.mutableState().ui.homeCardIndex = card;
            renderer.render(app, 10000, canvas);
            expect(blackPixelCount(canvas) > 1200, "every Home card should render in both layout families");
        }
    }
}


void testCatRemainsVisible() {
    housecat::HouseCatApp app;
    housecat::MonoCanvas canvas;
    housecat::UiRenderer renderer;
    housecat::Notification notification{"visible", "Hello!", "A tiny message.", housecat::NotificationKind::Generic,
                                        housecat::NotificationPriority::Notice, 1000, 100000, false};
    (void)app.receiveNotification(notification, 1000);

    const housecat::ScreenId screens[] = {
        housecat::ScreenId::Home,
        housecat::ScreenId::Menu,
        housecat::ScreenId::Cat,
        housecat::ScreenId::Missions,
        housecat::ScreenId::Whiskers,
        housecat::ScreenId::Library,
        housecat::ScreenId::Lab,
        housecat::ScreenId::Routine,
        housecat::ScreenId::Settings,
        housecat::ScreenId::Notification,
    };

    for (const auto orientation : {housecat::Orientation::Deg0, housecat::Orientation::Deg90,
                                   housecat::Orientation::Deg180, housecat::Orientation::Deg270}) {
        app.mutableState().settings.orientation = orientation;
        for (const auto screen : screens) {
            app.mutableState().ui.screen = screen;
            renderer.render(app, 2000, canvas);
            const bool portrait = canvas.width() < canvas.height();
            const auto catInk = portrait
                ? blackPixelsIn(canvas, 0, 18, canvas.width(), 94)
                : blackPixelsIn(canvas, 0, 17, 88, 84);
            expect(catInk > 140, "every screen and orientation should preserve a visible cat region");
        }
    }
}

void testRoutineLifecycle() {
    housecat::RoutineState routine{};
    const auto originalFood = routine.food;
    const auto originalRest = routine.rest;
    const auto originalFun = routine.fun;
    (void)housecat::Routine::advance(routine, 60);
    expect(routine.food < originalFood && routine.rest < originalRest && routine.fun < originalFun,
           "idle needs should degrade over time");

    housecat::Routine::feed(routine);
    expect(routine.mealsLogged == 1 && routine.food > originalFood - 300,
           "feed should restore food and record a real meal");
    housecat::Routine::pet(routine);
    expect(routine.fun > originalFun - 240, "petting should restore entertainment");

    housecat::Routine::setActivity(routine, housecat::ActivityMode::Work);
    (void)housecat::Routine::advance(routine, 25);
    expect(routine.workSeconds == 1500 && !housecat::Routine::focusPhase(routine),
           "work should accumulate and enter the five-minute break after 25 minutes");
    (void)housecat::Routine::advance(routine, 5);
    expect(housecat::Routine::focusPhase(routine), "work should return to focus after the break");

    housecat::Routine::setActivity(routine, housecat::ActivityMode::Sleep);
    const auto tired = routine.rest;
    (void)housecat::Routine::advance(routine, 60);
    expect(routine.sleepSeconds == 3600 && routine.rest > tired, "sleep should accumulate and restore rest");

    housecat::RoutineState overfed{};
    overfed.food = 10000;
    for (int meal = 0; meal < 5; ++meal) housecat::Routine::feed(overfed);
    expect(overfed.bodyBalance >= 7000 && std::string(housecat::Routine::bodyName(overfed)) == "slothy",
           "repeated meals while full should create the slothy overfed state");

    housecat::RoutineState starving{};
    starving.food = 1;
    (void)housecat::Routine::advance(starving, 24 * 60);
    expect(starving.food == 0 && housecat::Routine::energyPercent(starving) == 0
               && std::string(housecat::Routine::bodyName(starving)) == "wasting",
           "too little food should drain food and energy without permanently killing the pal");
}

void testRoutineAppIntegration() {
    housecat::HouseCatApp app;
    app.mutableState().ui.screen = housecat::ScreenId::Routine;
    const auto food = app.state().routine.food;
    const auto feed = app.dispatch(housecat::InputAction::Select, 1000);
    expect(feed.event.type == housecat::AppEventType::MealLogged && app.state().routine.food > food,
           "the first My Day action should log a meal");
    (void)app.dispatch(housecat::InputAction::Down, 1001);
    const auto work = app.dispatch(housecat::InputAction::Select, 1002);
    expect(work.event.type == housecat::AppEventType::ActivityChanged
               && app.state().routine.activity == housecat::ActivityMode::Work,
           "My Day should start work from the physical controls");
    (void)app.tick(2000);
    const auto timer = app.tick(15ULL * 60ULL * 1000ULL + 2000ULL);
    expect(timer.event.type == housecat::AppEventType::NeedsUpdated
               && app.state().routine.workSeconds == 15ULL * 60ULL,
           "the app timer should degrade and publish durable need updates");

    app.mutableState().routine.lastUpdateEpochS = 1'000'000;
    const auto before = app.state().routine.workSeconds;
    (void)app.updateClock(1'003'600, 2'000'000);
    expect(app.state().routine.workSeconds == before + 3600,
           "Home Assistant wall clock should catch up activity after reboot/offline time");
}

void testProvisioningOverlay() {
    housecat::HouseCatApp app;
    const auto result = app.updateProvisioning(
        true, "HouseCat-Setup-7BCAA0", "housecat-setup", 1000);
    expect(result.refresh == housecat::RefreshKind::Full
               && app.state().device.setupPortalActive,
           "starting the captive portal should request a full setup-screen refresh");
    housecat::MonoCanvas canvas;
    housecat::UiRenderer renderer;
    renderer.render(app, 1000, canvas);
    expect(blackPixelsIn(canvas, 0, 18, 122, 209) > 500,
           "the portrait provisioning screen should visibly show recovery instructions");
    app.mutableState().settings.orientation = housecat::Orientation::Deg90;
    renderer.render(app, 1000, canvas);
    expect(blackPixelsIn(canvas, 0, 18, 250, 83) > 500,
           "the landscape provisioning screen should visibly show recovery instructions");
    (void)app.updateProvisioning(false, {}, {}, 2000);
    expect(!app.state().device.setupPortalActive,
           "successful Wi-Fi recovery should dismiss the provisioning screen");
}

void testTailscaleStatus() {
    housecat::HouseCatApp app;
    const auto connecting = app.updateTailscale("connecting", {}, 1000);
    expect(connecting.changed() && app.state().device.tailscaleState == "connecting",
           "Tailscale connection changes should update diagnostic state");
    const auto connected = app.updateTailscale("connected", "100.101.102.103", 2000);
    expect(connected.refresh == housecat::RefreshKind::Partial
               && app.state().device.tailscaleIp == "100.101.102.103",
           "a connected Tailscale client should retain its VPN address");
    expect(!app.updateTailscale("connected", "100.101.102.103", 3000).changed(),
           "polling an unchanged Tailscale state should not refresh e-paper");
}


}  // namespace

int main() {
    try {
        testOrientation();
        testPanelEncoding();
        testProgression();
        testRefreshBudget();
        testSemanticMappings();
        testMissionUpdates();
        testNavigation();
        testHomeFreshness();
        testDomainNormalization();
        testPlaygroundAndLab();
        testLibraryReader();
        testNotificationQueue();
        testNotifications();
        testRendering();
        testCatRemainsVisible();
        testRoutineLifecycle();
        testRoutineAppIntegration();
        testProvisioningOverlay();
        testTailscaleStatus();
    } catch (const std::exception& error) {
        std::cerr << "Unhandled exception: " << error.what() << '\n';
        return 2;
    }

    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }
    std::cout << "All House Cat tests passed\n";
    return 0;
}

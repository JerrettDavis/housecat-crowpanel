#include "housecat/ui/renderer.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "housecat/domain/progression.h"
#include "housecat/domain/routine.h"
#include "housecat/ui/assets.h"

namespace housecat {
namespace {

std::string temperature(const float value) {
    char buffer[16]{};
    std::snprintf(buffer, sizeof(buffer), "%d F", static_cast<int>(std::lround(value)));
    return buffer;
}

std::string levelLabel(const CatState& cat) {
    char buffer[16]{};
    std::snprintf(buffer, sizeof(buffer), "LV %u", static_cast<unsigned>(cat.level));
    return buffer;
}

std::string percentLabel(const std::uint8_t value) {
    char buffer[16]{};
    std::snprintf(buffer, sizeof(buffer), "%u%%", static_cast<unsigned>(value));
    return buffer;
}

std::uint8_t missionPercent(const Mission& mission) noexcept {
    if (mission.target == 0) {
        return mission.complete ? 100 : 0;
    }
    return static_cast<std::uint8_t>(std::min<std::uint32_t>(100, mission.progress * 100U / mission.target));
}

const char* routineActionLabel(const std::size_t index) noexcept {
    constexpr const char* labels[]{"FEED", "WORK", "PLAY", "SLEEP"};
    return labels[index % 4];
}

std::string focusLabel(const RoutineState& routine) {
    if (routine.activity != ActivityMode::Work) return std::string("ENERGY ") + percentLabel(Routine::energyPercent(routine));
    const auto seconds = Routine::focusRemainingSeconds(routine);
    char buffer[24]{};
    std::snprintf(buffer, sizeof(buffer), "%s %02u:%02u",
        Routine::focusPhase(routine) ? "FOCUS" : "BREAK",
        static_cast<unsigned>(seconds / 60ULL),
        static_cast<unsigned>(seconds % 60ULL));
    return buffer;
}

std::string compactFocusLabel(const RoutineState& routine) {
    if (routine.activity != ActivityMode::Work) return std::string("E ") + percentLabel(Routine::energyPercent(routine));
    const auto seconds = Routine::focusRemainingSeconds(routine);
    char buffer[16]{};
    std::snprintf(buffer, sizeof(buffer), "%c %02u:%02u",
        Routine::focusPhase(routine) ? 'F' : 'B',
        static_cast<unsigned>(seconds / 60ULL), static_cast<unsigned>(seconds % 60ULL));
    return buffer;
}

void drawInvertedText(
    MonoCanvas& canvas,
    const Rect& bounds,
    const std::string_view text,
    const BitmapFont& font,
    const int scale = 1) {
    canvas.fillRoundedRect(bounds, 4, true);
    canvas.drawTextCentered(bounds, text, font, false, scale);
}

void drawCat(MonoCanvas& canvas, const CatPose pose, const Rect& bounds) {
    canvas.drawBitmap(bounds.x, bounds.y, catBitmap(pose), true, bounds.width, bounds.height, true);
}

void drawWeatherIcon(MonoCanvas& canvas, const WeatherCondition condition, const int x, const int y, const int size) {
    IconId icon = IconId::Cloud;
    switch (condition) {
        case WeatherCondition::Sunny: icon = IconId::Sun; break;
        case WeatherCondition::Rainy: icon = IconId::Rain; break;
        case WeatherCondition::Cloudy:
        case WeatherCondition::Unknown: icon = IconId::Cloud; break;
    }
    canvas.drawBitmap(x, y, iconBitmap(icon), true, size, size, true);
}

const char* detailActionLabel(const AppState& state) noexcept {
    switch (state.ui.screen) {
        case ScreenId::Menu: return "OPEN";
        case ScreenId::Cat: return "PET";
        case ScreenId::Whiskers: return "SCAN";
        case ScreenId::Library:
            return state.library.view == LibraryView::Reader ? "LIST" : "READ";
        case ScreenId::Lab: return "PROBE";
        case ScreenId::Routine: return "DO";
        case ScreenId::Settings: return "SAVE";
        case ScreenId::Missions: return "PLAY";
        case ScreenId::Home:
        case ScreenId::Notification: return "OK";
    }
    return "OK";
}

}  // namespace

void UiRenderer::render(const HouseCatApp& app, const std::uint64_t nowMs, MonoCanvas& canvas) const {
    canvas.setOrientation(app.state().settings.orientation);
    canvas.clear(false);
    if (canvas.width() < canvas.height()) {
        renderPortrait(app, nowMs, canvas);
    } else {
        renderLandscape(app, nowMs, canvas);
    }
    if (app.state().device.setupPortalActive) {
        const auto& device = app.state().device;
        if (canvas.width() < canvas.height()) {
            canvas.fillRect({0, 18, 122, 209}, false);
            drawCat(canvas, CatPose::Curious, {25, 22, 72, 72});
            canvas.drawTextCentered({5, 99, 112, 16}, "WI-FI SETUP", boldFont());
            canvas.drawHorizontalLine(9, 119, 104, true);
            canvas.drawText(12, 128, "JOIN", smallFont());
            drawWrappedText(canvas, {12, 143, 98, 30}, device.setupSsid, boldFont(), 1, 2, 2, true);
            canvas.drawText(12, 178, "PASSWORD", smallFont());
            drawWrappedText(canvas, {12, 192, 98, 15}, device.setupPassword, boldFont(), 1, 2, 1, true);
            canvas.drawTextCentered({8, 212, 106, 10}, "OPEN 192.168.4.1", smallFont());
        } else {
            canvas.fillRect({0, 18, 250, 83}, false);
            drawCat(canvas, CatPose::Curious, {4, 20, 78, 78});
            canvas.drawText(91, 22, "WI-FI SETUP", boldFont());
            canvas.drawHorizontalLine(91, 38, 151, true);
            canvas.drawText(91, 45, "JOIN", smallFont());
            drawWrappedText(canvas, {125, 44, 117, 18}, device.setupSsid, boldFont(), 1, 2, 1);
            canvas.drawText(91, 66, "PASS", smallFont());
            canvas.drawText(125, 64, device.setupPassword, boldFont());
            canvas.drawText(91, 85, "OPEN 192.168.4.1", smallFont());
        }
    }
}

void UiRenderer::renderPortrait(const HouseCatApp& app, const std::uint64_t nowMs, MonoCanvas& canvas) const {
    drawStatusBar(app.state(), canvas, false);
    switch (app.state().ui.screen) {
        case ScreenId::Home: drawPortraitHome(app, nowMs, canvas); break;
        case ScreenId::Menu: drawPortraitMenu(app, nowMs, canvas); break;
        case ScreenId::Notification: drawPortraitNotification(app, nowMs, canvas); break;
        default: drawPortraitDetail(app, nowMs, canvas); break;
    }
    if (app.state().ui.screen != ScreenId::Notification) {
        drawPortraitFooter(app.state(), canvas);
    }
}

void UiRenderer::renderLandscape(const HouseCatApp& app, const std::uint64_t nowMs, MonoCanvas& canvas) const {
    drawStatusBar(app.state(), canvas, true);
    switch (app.state().ui.screen) {
        case ScreenId::Home: drawLandscapeHome(app, nowMs, canvas); break;
        case ScreenId::Menu: drawLandscapeMenu(app, nowMs, canvas); break;
        case ScreenId::Notification: drawLandscapeNotification(app, nowMs, canvas); break;
        default: drawLandscapeDetail(app, nowMs, canvas); break;
    }
    if (app.state().ui.screen != ScreenId::Notification) {
        drawLandscapeFooter(app.state(), canvas);
    }
}

void UiRenderer::drawStatusBar(const AppState& state, MonoCanvas& canvas, const bool landscape) const {
    const int width = canvas.width();
    const int barHeight = landscape ? 16 : 18;
    canvas.drawHorizontalLine(0, barHeight - 1, width, true);

    const char* connection = "OFF";
    if (state.device.connection == ConnectionState::HomeAssistant
        || state.device.connection == ConnectionState::WifiOnly) {
        connection = "WIFI";
    }
    canvas.drawBitmap(3, 2, iconBitmap(IconId::Wifi), true, 12, 12, true);
    canvas.drawText(18, 3, connection, smallFont(), true);

    const auto level = levelLabel(state.cat);
    const int labelWidth = canvas.measureText(level, smallFont());
    canvas.drawBitmap(width - labelWidth - 18, 2, iconBitmap(IconId::Star), true, 12, 12, true);
    canvas.drawText(width - labelWidth - 3, 3, level, smallFont(), true);
}

void UiRenderer::drawPortraitFooter(const AppState& state, MonoCanvas& canvas) const {
    if (!state.settings.showControlHints) {
        return;
    }
    constexpr int y = 227;
    canvas.drawHorizontalLine(0, y, canvas.width(), true);

    if (state.ui.screen == ScreenId::Home) {
        drawRockerGlyph(canvas, 4, 232, 12);
        canvas.drawText(19, 234, "LOOK", smallFont());
        canvas.drawBitmap(52, 231, iconBitmap(IconId::Paw), true, 15, 15, true);
        canvas.drawText(69, 234, "PET", smallFont());
        canvas.drawBitmap(101, 231, iconBitmap(IconId::Menu), true, 15, 15, true);
        return;
    }

    if (state.ui.screen == ScreenId::Menu) {
        drawRockerGlyph(canvas, 4, 232, 12);
        canvas.drawText(19, 234, "MOVE", smallFont());
        drawSelectGlyph(canvas, 55, 232, 12);
        canvas.drawText(70, 234, "OPEN", smallFont());
        return;
    }

    drawBackGlyph(canvas, 4, 232, 12);
    canvas.drawText(19, 234, "BACK", smallFont());
    drawSelectGlyph(canvas, 64, 232, 12);
    canvas.drawText(79, 234, detailActionLabel(state), smallFont());
}

void UiRenderer::drawLandscapeFooter(const AppState& state, MonoCanvas& canvas) const {
    if (!state.settings.showControlHints) {
        return;
    }
    constexpr int y = 101;
    canvas.drawHorizontalLine(0, y, canvas.width(), true);

    if (state.ui.screen == ScreenId::Home) {
        drawRockerGlyph(canvas, 6, 105, 12);
        canvas.drawText(22, 107, "LOOK", smallFont());
        canvas.drawBitmap(92, 104, iconBitmap(IconId::Paw), true, 15, 15, true);
        canvas.drawText(111, 107, "PET", smallFont());
        canvas.drawBitmap(205, 104, iconBitmap(IconId::Menu), true, 15, 15, true);
        canvas.drawText(224, 107, "MENU", smallFont());
        return;
    }

    drawBackGlyph(canvas, 6, 105, 12);
    canvas.drawText(22, 107, "BACK", smallFont());
    drawRockerGlyph(canvas, 92, 105, 12);
    canvas.drawText(108, 107, "MOVE", smallFont());
    drawSelectGlyph(canvas, 190, 105, 12);
    canvas.drawText(206, 107, detailActionLabel(state), smallFont());
}

void UiRenderer::drawPortraitHome(const HouseCatApp& app, const std::uint64_t nowMs, MonoCanvas& canvas) const {
    const auto& state = app.state();
    drawCat(canvas, app.currentPose(nowMs), {17, 20, 88, 88});

    const Rect card{5, 111, 112, 106};
    canvas.drawRoundedRect(card, 7, true);
    const auto index = state.ui.homeCardIndex % HouseCatApp::kHomeCardCount;
    if (index == 0) {
        drawWeatherIcon(canvas, state.home.condition, 13, 121, 27);
        const auto outside = state.home.updatedAtMs == 0 ? std::string("--") : temperature(state.home.outsideTemperatureF);
        canvas.drawText(45, 118, outside, boldFont(), true, 2);
        canvas.drawText(45, 145, state.home.conditionLabel, smallFont());
        canvas.drawHorizontalLine(13, 163, 96, true);
        canvas.drawBitmap(13, 172, iconBitmap(IconId::Thermometer), true, 21, 21, true);
        canvas.drawText(40, 170, "INSIDE", smallFont());
        canvas.drawText(40, 185, temperature(state.home.insideTemperatureF), boldFont());
        if (state.home.stale) {
            drawInvertedText(canvas, {74, 192, 35, 17}, "OLD", smallFont());
        }
    } else if (index == 1) {
        canvas.drawBitmap(13, 119, iconBitmap(IconId::Home), true, 26, 26, true);
        canvas.drawText(45, 121, "ROOMS", boldFont());
        canvas.drawHorizontalLine(13, 151, 96, true);
        if (state.home.roomCount == 0) {
            drawWrappedText(canvas, {13, 161, 96, 42}, "No room sensors yet.", smallFont(), 1, 3, 2);
        } else {
            const auto first = state.home.rooms[0];
            canvas.drawText(13, 160, first.name, smallFont());
            canvas.drawText(70, 160, temperature(first.temperatureF), boldFont());
            if (state.home.roomCount > 1) {
                const auto second = state.home.rooms[1];
                canvas.drawText(13, 184, second.name, smallFont());
                canvas.drawText(70, 184, temperature(second.temperatureF), boldFont());
            }
        }
    } else if (index == 2) {
        canvas.drawBitmap(13, 119, iconBitmap(IconId::Mission), true, 26, 26, true);
        canvas.drawText(45, 121, "MISSION", boldFont());
        drawWrappedText(canvas, {13, 153, 96, 28}, state.mission.title, boldFont(), 1, 2, 2);
        drawProgressBar(canvas, {13, 188, 96, 13}, missionPercent(state.mission));
        canvas.drawTextCentered({13, 202, 96, 12}, state.mission.complete ? "COMPLETE!" : "KEEP GOING", smallFont());
    } else {
        canvas.drawBitmap(13, 119, iconBitmap(IconId::Heart), true, 26, 26, true);
        canvas.drawText(45, 121, state.home.palLabel, boldFont());
        canvas.drawHorizontalLine(13, 151, 96, true);
        drawWrappedText(canvas, {13, 160, 96, 48}, state.home.palMessage, boldFont(), 1, 3, 4, true);
    }
    drawPageDots(canvas, 61, 221, HouseCatApp::kHomeCardCount, index);
}

void UiRenderer::drawLandscapeHome(const HouseCatApp& app, const std::uint64_t nowMs, MonoCanvas& canvas) const {
    const auto& state = app.state();
    drawCat(canvas, app.currentPose(nowMs), {4, 17, 84, 84});
    const Rect card{94, 20, 151, 75};
    canvas.drawRoundedRect(card, 7, true);
    const auto index = state.ui.homeCardIndex % HouseCatApp::kHomeCardCount;
    if (index == 0) {
        drawWeatherIcon(canvas, state.home.condition, 103, 28, 29);
        canvas.drawText(138, 25,
                        state.home.updatedAtMs == 0 ? "--" : temperature(state.home.outsideTemperatureF),
                        boldFont(), true, 2);
        canvas.drawText(140, 51, state.home.conditionLabel, smallFont());
        canvas.drawHorizontalLine(103, 66, 133, true);
        canvas.drawBitmap(103, 72, iconBitmap(IconId::Thermometer), true, 17, 17, true);
        canvas.drawText(125, 75, "HOME", smallFont());
        canvas.drawText(170, 72, temperature(state.home.insideTemperatureF), boldFont());
    } else if (index == 1) {
        canvas.drawBitmap(103, 28, iconBitmap(IconId::Home), true, 25, 25, true);
        canvas.drawText(135, 30, "ROOMS", boldFont());
        canvas.drawHorizontalLine(103, 58, 133, true);
        if (state.home.roomCount > 0) {
            canvas.drawText(103, 68, state.home.rooms[0].name, smallFont());
            canvas.drawText(174, 66, temperature(state.home.rooms[0].temperatureF), boldFont());
        }
        if (state.home.roomCount > 1) {
            canvas.drawText(103, 84, state.home.rooms[1].name, smallFont());
            canvas.drawText(174, 82, temperature(state.home.rooms[1].temperatureF), boldFont());
        }
    } else if (index == 2) {
        canvas.drawBitmap(103, 27, iconBitmap(IconId::Mission), true, 27, 27, true);
        canvas.drawText(137, 30, "MISSION", boldFont());
        drawWrappedText(canvas, {103, 57, 133, 22}, state.mission.title, smallFont(), 1, 2, 2);
        drawProgressBar(canvas, {103, 82, 133, 10}, missionPercent(state.mission));
    } else {
        canvas.drawBitmap(103, 27, iconBitmap(IconId::Heart), true, 27, 27, true);
        canvas.drawText(137, 30, state.home.palLabel, boldFont());
        drawWrappedText(canvas, {103, 59, 133, 32}, state.home.palMessage, boldFont(), 1, 2, 2, true);
    }
    drawPageDots(canvas, 169, 97, HouseCatApp::kHomeCardCount, index);
}

void UiRenderer::drawPortraitMenu(const HouseCatApp& app, const std::uint64_t nowMs, MonoCanvas& canvas) const {
    const auto& state = app.state();
    const auto& entry = HouseCatApp::menuEntry(state.ui.menuIndex);
    drawCat(canvas, app.currentPose(nowMs), {78, 19, 40, 40});
    canvas.drawBitmap(33, 35, iconBitmap(entry.icon), true, 56, 56, true);
    canvas.drawTextCentered({5, 98, 112, 18}, entry.title, boldFont());
    drawWrappedText(canvas, {12, 121, 98, 42}, entry.subtitle, smallFont(), 1, 3, 3, true);

    canvas.drawRoundedRect({14, 172, 94, 37}, 8, true);
    drawRockerGlyph(canvas, 23, 184, 13);
    canvas.drawText(42, 184, "CHOOSE", boldFont());
    drawPageDots(canvas, 61, 218, HouseCatApp::kMenuEntryCount, state.ui.menuIndex);
}

void UiRenderer::drawLandscapeMenu(const HouseCatApp& app, const std::uint64_t nowMs, MonoCanvas& canvas) const {
    const auto& state = app.state();
    const auto& entry = HouseCatApp::menuEntry(state.ui.menuIndex);
    drawCat(canvas, app.currentPose(nowMs), {4, 25, 69, 69});
    canvas.drawBitmap(84, 28, iconBitmap(entry.icon), true, 47, 47, true);
    canvas.drawText(140, 25, entry.title, boldFont());
    drawWrappedText(canvas, {140, 43, 102, 35}, entry.subtitle, smallFont(), 1, 2, 3);
    canvas.drawRoundedRect({140, 79, 92, 17}, 5, true);
    canvas.drawTextCentered({140, 79, 92, 17}, "CLICK TO OPEN", smallFont());
    drawPageDots(canvas, 108, 93, HouseCatApp::kMenuEntryCount, state.ui.menuIndex);
}

void UiRenderer::drawPortraitNotification(const HouseCatApp& app, const std::uint64_t nowMs, MonoCanvas& canvas) const {
    const auto& state = app.state();
    if (!state.activeNotification.has_value()) {
        return;
    }
    const auto& note = *state.activeNotification;
    drawCat(canvas, app.currentPose(nowMs), {8, 21, 73, 73});
    canvas.drawBitmap(88, 28, iconBitmap(notificationIcon(note.kind)), true, 27, 27, true);

    const bool urgent = note.priority >= NotificationPriority::Urgent;
    drawInvertedText(canvas, {84, 63, 33, 18}, urgent ? "HELP" : "NEWS", smallFont());

    const Rect message{5, 99, 112, 96};
    canvas.drawRoundedRect(message, 7, true);
    drawWrappedText(canvas, {12, 108, 98, 38}, note.title, boldFont(), 1, 3, 3, true);
    canvas.drawHorizontalLine(14, 151, 94, true);
    drawWrappedText(canvas, {12, 159, 98, 30}, note.body, smallFont(), 1, 2, 3, true);

    const Rect action{5, 202, 112, 39};
    canvas.fillRoundedRect(action, 8, true);
    canvas.drawBitmap(16, 211, iconBitmap(IconId::Check), false, 21, 21, true);
    canvas.drawText(44, 211, "CLICK OK", boldFont(), false);
}

void UiRenderer::drawLandscapeNotification(const HouseCatApp& app, const std::uint64_t nowMs, MonoCanvas& canvas) const {
    const auto& state = app.state();
    if (!state.activeNotification.has_value()) {
        return;
    }
    const auto& note = *state.activeNotification;
    drawCat(canvas, app.currentPose(nowMs), {3, 19, 79, 79});
    canvas.drawBitmap(88, 22, iconBitmap(notificationIcon(note.kind)), true, 31, 31, true);
    drawWrappedText(canvas, {124, 20, 119, 34}, note.title, boldFont(), 1, 2, 3);
    canvas.drawHorizontalLine(92, 58, 151, true);
    drawWrappedText(canvas, {92, 65, 151, 27}, note.body, smallFont(), 1, 2, 3);

    canvas.fillRoundedRect({92, 96, 151, 23}, 6, true);
    canvas.drawBitmap(104, 100, iconBitmap(IconId::Check), false, 15, 15, true);
    canvas.drawText(128, 102, "CLICK TO SAY OK", boldFont(), false);
}

void UiRenderer::drawPortraitDetail(const HouseCatApp& app, const std::uint64_t nowMs, MonoCanvas& canvas) const {
    const auto& state = app.state();
    const auto screen = state.ui.screen;
    drawCat(canvas, app.currentPose(nowMs), {8, 21, 70, 70});
    canvas.drawBitmap(87, 27, iconBitmap(screenIcon(screen)), true, 28, 28, true);
    canvas.drawTextCentered({4, 94, 114, 18}, screenTitle(screen), boldFont());

    const Rect panel{6, 117, 110, 101};
    canvas.drawRoundedRect(panel, 7, true);

    if (screen == ScreenId::Cat) {
        canvas.drawBitmap(14, 127, iconBitmap(IconId::Star), true, 20, 20, true);
        canvas.drawText(40, 128, levelLabel(state.cat), boldFont());
        drawProgressBar(canvas, {14, 152, 94, 12}, Progression::levelProgressPercent(state.cat));
        canvas.drawBitmap(14, 174, iconBitmap(IconId::Heart), true, 20, 20, true);
        canvas.drawText(40, 175, "BOND", smallFont());
        canvas.drawText(77, 173, percentLabel(state.cat.bond), boldFont());
        canvas.drawTextCentered({14, 199, 94, 13}, "CLICK TO PET", smallFont());
    } else if (screen == ScreenId::Missions) {
        drawWrappedText(canvas, {14, 126, 94, 28}, state.mission.title, boldFont(), 1, 2, 2, true);
        drawWrappedText(canvas, {14, 158, 94, 28}, state.mission.detail, smallFont(), 1, 2, 3, true);
        drawProgressBar(canvas, {14, 193, 94, 13}, missionPercent(state.mission));
    } else if (screen == ScreenId::Whiskers) {
        if (state.playground.scanning) {
            canvas.drawTextCentered({14, 132, 94, 18}, "LISTENING...", boldFont());
            drawWrappedText(canvas, {14, 158, 94, 30}, "Kitty is finding nearby Wi-Fi.", smallFont(), 1, 2, 3, true);
        } else if (state.playground.networkCount == 0) {
            canvas.drawTextCentered({14, 132, 94, 18}, "READY", boldFont());
            drawWrappedText(canvas, {14, 158, 94, 30}, "Click to scan nearby Wi-Fi.", smallFont(), 1, 2, 3, true);
        } else {
            const auto& network = state.playground.networks[state.playground.selectedIndex];
            drawWrappedText(canvas, {14, 127, 94, 26}, network.name, boldFont(), 1, 2, 2, true);
            canvas.drawText(14, 159, (std::to_string(network.rssi) + " dBm").c_str(), smallFont());
            canvas.drawText(14, 176, ("CH " + std::to_string(network.channel)).c_str(), smallFont());
            canvas.drawText(65, 176, network.secured ? "LOCK" : "OPEN", smallFont());
            drawPageDots(canvas, 61, 192, state.playground.networkCount, state.playground.selectedIndex);
        }
        drawInvertedText(canvas, {20, 199, 82, 17}, state.playground.scanning ? "PLEASE WAIT" : "CLICK SCAN", smallFont());
    } else if (screen == ScreenId::Library) {
        const auto& library = state.library;
        const auto& book = library.books[library.selectedBook];
        if (library.view == LibraryView::Catalog) {
            drawWrappedText(canvas, {14, 126, 94, 31}, book.title, boldFont(), 1, 2, 2, true);
            drawWrappedText(canvas, {14, 161, 94, 25}, book.author, smallFont(), 1, 2, 2, true);
            drawPageDots(canvas, 61, 192, LibraryState::kCatalogSize, library.selectedBook);
            const bool canResume = library.bookmarkValid
                && library.bookmarkedBookId == book.gutenbergId
                && library.cachedBookId == book.gutenbergId;
            drawInvertedText(canvas, {20, 199, 82, 17}, canResume ? "CLICK RESUME" : "CLICK READ", smallFont());
        } else if (library.view == LibraryView::Downloading) {
            canvas.drawTextCentered({14, 132, 94, 18}, "FETCHING...", boldFont());
            drawWrappedText(canvas, {14, 158, 94, 31}, book.title, smallFont(), 1, 2, 3, true);
            drawInvertedText(canvas, {20, 199, 82, 17}, "PLEASE WAIT", smallFont());
        } else if (library.view == LibraryView::Reader) {
            drawWrappedText(canvas, {13, 124, 96, 66}, library.pageText, readerFont(), 1, 2, 3, true);
            canvas.drawTextCentered(
                {14, 192, 94, 10},
                (std::to_string(library.pageIndex + 1) + " / " + std::to_string(library.pageCount)).c_str(),
                smallFont());
            drawInvertedText(canvas, {20, 202, 82, 14}, "UP/DOWN PAGE", smallFont());
        } else {
            canvas.drawTextCentered({14, 130, 94, 18}, "CAN'T OPEN", boldFont());
            drawWrappedText(canvas, {14, 155, 94, 34}, library.error, smallFont(), 1, 2, 3, true);
            drawInvertedText(canvas, {20, 199, 82, 17}, "CLICK BACK", smallFont());
        }
    } else if (screen == ScreenId::Lab) {
        const auto& pin = state.lab.pins[state.lab.selectedIndex];
        canvas.drawTextCentered({14, 128, 94, 18}, ("GPIO " + std::to_string(pin.pin)).c_str(), boldFont());
        canvas.drawTextCentered({14, 154, 94, 24}, pin.high ? "HIGH" : "LOW", boldFont());
        drawRockerGlyph(canvas, 20, 181, 14);
        canvas.drawText(42, 183, "CHOOSE PIN", smallFont());
        drawInvertedText(canvas, {20, 199, 82, 17}, "CLICK PROBE", smallFont());
    } else if (screen == ScreenId::Routine) {
        const auto& routine = state.routine;
        canvas.drawText(14, 124, "FOOD", smallFont());
        drawProgressBar(canvas, {52, 124, 54, 9}, Routine::foodPercent(routine));
        canvas.drawText(14, 139, "REST", smallFont());
        drawProgressBar(canvas, {52, 139, 54, 9}, Routine::restPercent(routine));
        canvas.drawText(14, 154, "FUN", smallFont());
        drawProgressBar(canvas, {52, 154, 54, 9}, Routine::funPercent(routine));
        canvas.drawTextCentered({13, 168, 96, 12}, focusLabel(routine), smallFont());
        drawRockerGlyph(canvas, 16, 184, 14);
        canvas.drawText(38, 186, routineActionLabel(state.ui.routineActionIndex), boldFont());
        const bool active = state.ui.routineActionIndex != 0
            && static_cast<std::size_t>(routine.activity) == state.ui.routineActionIndex;
        drawInvertedText(canvas, {20, 201, 82, 15},
            state.ui.routineActionIndex == 0 ? "CLICK LOG" : (active ? "CLICK STOP" : "CLICK START"), smallFont());
    } else if (screen == ScreenId::Settings) {
        canvas.drawBitmap(14, 128, iconBitmap(IconId::Rotation), true, 31, 31, true);
        canvas.drawText(51, 128, "TURN", boldFont());
        canvas.drawText(51, 148, orientationLabel(state.ui.draftOrientation), boldFont());
        drawRockerGlyph(canvas, 24, 178, 17);
        canvas.drawText(49, 181, "ROTATE", smallFont());
        drawInvertedText(canvas, {20, 199, 82, 17}, "CLICK SAVE", smallFont());
    } else {
        drawWrappedText(canvas, {14, 128, 94, 28}, "A NEW PLACE TO EXPLORE", boldFont(), 1, 2, 3, true);
        canvas.drawHorizontalLine(16, 163, 90, true);
        drawWrappedText(canvas, {14, 171, 94, 25}, "First-pass shell is ready.", smallFont(), 1, 2, 3, true);
        drawInvertedText(canvas, {20, 199, 82, 17}, "CLICK PLAY", smallFont());
    }
}

void UiRenderer::drawLandscapeDetail(const HouseCatApp& app, const std::uint64_t nowMs, MonoCanvas& canvas) const {
    const auto& state = app.state();
    const auto screen = state.ui.screen;
    drawCat(canvas, app.currentPose(nowMs), {3, 20, 78, 78});
    canvas.drawBitmap(89, 23, iconBitmap(screenIcon(screen)), true, 29, 29, true);
    canvas.drawText(126, 22, screenTitle(screen), boldFont());
    canvas.drawHorizontalLine(89, 56, 153, true);

    if (screen == ScreenId::Cat) {
        canvas.drawBitmap(91, 63, iconBitmap(IconId::Star), true, 18, 18, true);
        canvas.drawText(114, 64, levelLabel(state.cat), boldFont());
        drawProgressBar(canvas, {166, 65, 69, 11}, Progression::levelProgressPercent(state.cat));
        canvas.drawBitmap(91, 82, iconBitmap(IconId::Heart), true, 18, 18, true);
        canvas.drawText(114, 84, "BOND", smallFont());
        canvas.drawText(167, 81, percentLabel(state.cat.bond), boldFont());
    } else if (screen == ScreenId::Missions) {
        drawWrappedText(canvas, {91, 63, 144, 20}, state.mission.title, boldFont(), 1, 2, 2);
        drawProgressBar(canvas, {91, 86, 144, 11}, missionPercent(state.mission));
    } else if (screen == ScreenId::Whiskers) {
        if (state.playground.scanning) {
            canvas.drawText(91, 66, "LISTENING FOR WI-FI...", boldFont());
        } else if (state.playground.networkCount == 0) {
            canvas.drawText(91, 66, "CLICK TO SCAN", boldFont());
            canvas.drawText(91, 87, "Passive and safe", smallFont());
        } else {
            const auto& network = state.playground.networks[state.playground.selectedIndex];
            drawWrappedText(canvas, {91, 62, 144, 20}, network.name, boldFont(), 1, 2, 2);
            canvas.drawText(91, 86, (std::to_string(network.rssi) + " dBm").c_str(), smallFont());
            canvas.drawText(155, 86, ("CH " + std::to_string(network.channel)).c_str(), smallFont());
            canvas.drawText(207, 86, network.secured ? "LOCK" : "OPEN", smallFont());
        }
    } else if (screen == ScreenId::Library) {
        const auto& library = state.library;
        const auto& book = library.books[library.selectedBook];
        if (library.view == LibraryView::Catalog) {
            drawWrappedText(canvas, {91, 62, 144, 21}, book.title, boldFont(), 1, 2, 2);
            canvas.drawText(91, 87, book.author, smallFont());
        } else if (library.view == LibraryView::Downloading) {
            canvas.drawText(91, 65, "FETCHING BOOK...", boldFont());
            drawWrappedText(canvas, {91, 85, 144, 14}, book.title, smallFont(), 1, 2, 1);
        } else if (library.view == LibraryView::Reader) {
            drawWrappedText(canvas, {89, 60, 148, 40}, library.pageText, readerFont(), 1, 2, 2);
            canvas.drawText(
                202, 43,
                (std::to_string(library.pageIndex + 1) + "/" + std::to_string(library.pageCount)).c_str(),
                smallFont());
        } else {
            canvas.drawText(91, 64, "CAN'T OPEN", boldFont());
            drawWrappedText(canvas, {91, 84, 144, 16}, library.error, smallFont(), 1, 2, 2);
        }
    } else if (screen == ScreenId::Lab) {
        const auto& pin = state.lab.pins[state.lab.selectedIndex];
        canvas.drawText(91, 65, ("GPIO " + std::to_string(pin.pin)).c_str(), boldFont());
        canvas.drawText(168, 65, pin.high ? "HIGH" : "LOW", boldFont());
        canvas.drawText(91, 87, "ROCKER PIN - CLICK PROBE", smallFont());
    } else if (screen == ScreenId::Routine) {
        const auto& routine = state.routine;
        canvas.drawText(91, 62, "FOOD", smallFont());
        drawProgressBar(canvas, {126, 62, 45, 8}, Routine::foodPercent(routine));
        canvas.drawText(177, 62, "REST", smallFont());
        drawProgressBar(canvas, {210, 62, 34, 8}, Routine::restPercent(routine));
        canvas.drawText(91, 77, "FUN", smallFont());
        drawProgressBar(canvas, {126, 77, 45, 8}, Routine::funPercent(routine));
        canvas.drawText(177, 77, compactFocusLabel(routine), smallFont());
        canvas.drawText(91, 90, ">", smallFont());
        canvas.drawText(103, 90, routineActionLabel(state.ui.routineActionIndex), smallFont());
        canvas.drawText(168, 90, Routine::activityName(routine.activity), smallFont());
    } else if (screen == ScreenId::Settings) {
        canvas.drawBitmap(91, 65, iconBitmap(IconId::Rotation), true, 25, 25, true);
        canvas.drawText(124, 64, orientationLabel(state.ui.draftOrientation), boldFont());
        drawRockerGlyph(canvas, 124, 82, 14);
        canvas.drawText(144, 84, "TURN - CLICK SAVE", smallFont());
    } else {
        drawWrappedText(canvas, {91, 64, 144, 28}, "First-pass shell ready. Click to play!", smallFont(), 1, 2, 3);
    }
}

IconId UiRenderer::notificationIcon(const NotificationKind kind) noexcept {
    switch (kind) {
        case NotificationKind::Person: return IconId::Person;
        case NotificationKind::Vehicle: return IconId::CarCharge;
        case NotificationKind::Home: return IconId::Home;
        case NotificationKind::Warning: return IconId::Bell;
        case NotificationKind::Mission: return IconId::Mission;
        case NotificationKind::Generic: return IconId::Bell;
    }
    return IconId::Bell;
}

IconId UiRenderer::screenIcon(const ScreenId screen) noexcept {
    switch (screen) {
        case ScreenId::Home: return IconId::Home;
        case ScreenId::Menu: return IconId::Menu;
        case ScreenId::Cat: return IconId::Paw;
        case ScreenId::Missions: return IconId::Mission;
        case ScreenId::Whiskers: return IconId::Radio;
        case ScreenId::Library: return IconId::Book;
        case ScreenId::Lab: return IconId::Flask;
        case ScreenId::Routine: return IconId::Heart;
        case ScreenId::Settings: return IconId::Gear;
        case ScreenId::Notification: return IconId::Bell;
    }
    return IconId::Home;
}

const char* UiRenderer::screenTitle(const ScreenId screen) noexcept {
    switch (screen) {
        case ScreenId::Home: return "HOME";
        case ScreenId::Menu: return "MENU";
        case ScreenId::Cat: return "MY CAT";
        case ScreenId::Missions: return "MISSIONS";
        case ScreenId::Whiskers: return "PLAYGROUND";
        case ScreenId::Library: return "LIBRARY";
        case ScreenId::Lab: return "LAB";
        case ScreenId::Routine: return "MY DAY";
        case ScreenId::Settings: return "SETTINGS";
        case ScreenId::Notification: return "NEWS";
    }
    return "HOUSE CAT";
}

const char* UiRenderer::orientationLabel(const Orientation orientation) noexcept {
    switch (orientation) {
        case Orientation::Deg0: return "UPRIGHT";
        case Orientation::Deg90: return "RIGHT";
        case Orientation::Deg180: return "UPSIDE DOWN";
        case Orientation::Deg270: return "LEFT";
    }
    return "UPRIGHT";
}

int UiRenderer::drawWrappedText(
    MonoCanvas& canvas,
    const Rect& bounds,
    const std::string_view text,
    const BitmapFont& font,
    const int scale,
    const int lineSpacing,
    const int maxLines,
    const bool centered,
    const bool black) {
    const int characterWidth = font.width * scale;
    const int maxCharacters = std::max(1, bounds.width / characterWidth);
    const int lineHeight = font.height * scale;
    const int lineAdvance = lineHeight + lineSpacing;
    const int verticalCapacity = lineAdvance > 0
        ? std::max(0, (bounds.height + lineSpacing) / lineAdvance)
        : 0;
    const int allowedLines = std::min(maxLines, verticalCapacity);
    if (allowedLines <= 0) {
        return bounds.y;
    }
    std::vector<std::string> lines;
    lines.reserve(static_cast<std::size_t>(allowedLines + 1));
    std::string current;
    std::string word;

    const auto flushWord = [&]() {
        if (word.empty()) {
            return;
        }
        // Split identifiers, URLs, and unusually long words instead of
        // allowing them to paint outside the component bounds.
        while (static_cast<int>(word.size()) > maxCharacters) {
            if (!current.empty()) {
                lines.push_back(current);
                current.clear();
            }
            lines.push_back(word.substr(0, static_cast<std::size_t>(maxCharacters)));
            word.erase(0, static_cast<std::size_t>(maxCharacters));
        }
        if (word.empty()) {
            return;
        }
        if (current.empty()) {
            current = word;
        } else if (static_cast<int>(current.size() + 1 + word.size()) <= maxCharacters) {
            current += ' ';
            current += word;
        } else {
            lines.push_back(current);
            current = word;
        }
        word.clear();
    };

    for (const char ch : text) {
        if (ch == ' ' || ch == '\n') {
            flushWord();
            if (ch == '\n' && !current.empty()) {
                lines.push_back(current);
                current.clear();
            }
        } else {
            word.push_back(ch);
        }
    }
    flushWord();
    if (!current.empty()) {
        lines.push_back(current);
    }
    if (lines.empty()) {
        return bounds.y;
    }

    if (static_cast<int>(lines.size()) > allowedLines) {
        lines.resize(static_cast<std::size_t>(allowedLines));
        auto& last = lines.back();
        const auto ellipsisLength = static_cast<std::size_t>(std::min(3, maxCharacters));
        const auto contentLimit = static_cast<std::size_t>(maxCharacters) - ellipsisLength;
        while (last.size() > contentLimit) {
            last.pop_back();
        }
        last.append(ellipsisLength, '.');
    }

    int y = bounds.y;
    for (const auto& line : lines) {
        if (y + lineHeight > bounds.bottom()) {
            break;
        }
        const int lineWidth = canvas.measureText(line, font, scale);
        const int x = centered ? bounds.x + (bounds.width - lineWidth) / 2 : bounds.x;
        canvas.drawText(x, y, line, font, black, scale);
        y += lineAdvance;
        if (y >= bounds.bottom()) {
            break;
        }
    }
    return y;
}

void UiRenderer::drawProgressBar(MonoCanvas& canvas, const Rect& bounds, const std::uint8_t percent) {
    canvas.drawRoundedRect(bounds, std::min(4, bounds.height / 2), true);
    const int innerWidth = std::max(0, bounds.width - 4);
    const int filled = innerWidth * std::min<int>(100, percent) / 100;
    if (filled > 0) {
        canvas.fillRoundedRect({bounds.x + 2, bounds.y + 2, filled, std::max(1, bounds.height - 4)}, 2, true);
    }
}

void UiRenderer::drawPageDots(
    MonoCanvas& canvas,
    const int centerX,
    const int y,
    const std::size_t count,
    const std::size_t active) {
    if (count == 0) {
        return;
    }
    const int gap = count > 4 ? 9 : 11;
    const int start = centerX - static_cast<int>(count - 1) * gap / 2;
    for (std::size_t index = 0; index < count; ++index) {
        const int x = start + static_cast<int>(index) * gap;
        if (index == active) {
            canvas.fillCircle(x, y, 3, true);
        } else {
            canvas.drawCircle(x, y, 2, true);
        }
    }
}

void UiRenderer::drawRockerGlyph(MonoCanvas& canvas, const int x, const int y, const int size) {
    const int half = size / 2;
    canvas.drawRoundedRect({x, y, size, size}, 3, true);
    canvas.drawLine(x + half, y + 2, x + 3, y + half - 1, true);
    canvas.drawLine(x + half, y + 2, x + size - 3, y + half - 1, true);
    canvas.drawLine(x + half, y + size - 3, x + 3, y + half + 1, true);
    canvas.drawLine(x + half, y + size - 3, x + size - 3, y + half + 1, true);
}

void UiRenderer::drawSelectGlyph(MonoCanvas& canvas, const int x, const int y, const int size) {
    canvas.drawRoundedRect({x, y, size, size}, 3, true);
    canvas.fillCircle(x + size / 2, y + size / 2, std::max(2, size / 4), true);
}

void UiRenderer::drawBackGlyph(MonoCanvas& canvas, const int x, const int y, const int size) {
    canvas.drawRoundedRect({x, y, size, size}, 3, true);
    canvas.drawLine(x + 3, y + size / 2, x + size - 3, y + size / 2, true);
    canvas.drawLine(x + 3, y + size / 2, x + size / 2, y + 3, true);
    canvas.drawLine(x + 3, y + size / 2, x + size / 2, y + size - 3, true);
}

void UiRenderer::drawIconLabel(
    MonoCanvas& canvas,
    const IconId icon,
    const std::string_view label,
    const int x,
    const int y,
    const int iconSize,
    const bool inverted) {
    canvas.drawBitmap(x, y, iconBitmap(icon), !inverted, iconSize, iconSize, true);
    canvas.drawText(x + iconSize + 4, y + (iconSize - smallFont().height) / 2, label, smallFont(), !inverted);
}

}  // namespace housecat

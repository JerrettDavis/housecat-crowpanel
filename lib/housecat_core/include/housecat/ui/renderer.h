#pragma once

#include <cstdint>
#include <string_view>

#include "housecat/app/housecat_app.h"
#include "housecat/ui/mono_canvas.h"

namespace housecat {

class UiRenderer final {
public:
    void render(const HouseCatApp& app, std::uint64_t nowMs, MonoCanvas& canvas) const;

private:
    void renderPortrait(const HouseCatApp& app, std::uint64_t nowMs, MonoCanvas& canvas) const;
    void renderLandscape(const HouseCatApp& app, std::uint64_t nowMs, MonoCanvas& canvas) const;

    void drawStatusBar(const AppState& state, MonoCanvas& canvas, bool landscape) const;
    void drawPortraitFooter(const AppState& state, MonoCanvas& canvas) const;
    void drawLandscapeFooter(const AppState& state, MonoCanvas& canvas) const;

    void drawPortraitHome(const HouseCatApp& app, std::uint64_t nowMs, MonoCanvas& canvas) const;
    void drawLandscapeHome(const HouseCatApp& app, std::uint64_t nowMs, MonoCanvas& canvas) const;
    void drawPortraitMenu(const HouseCatApp& app, std::uint64_t nowMs, MonoCanvas& canvas) const;
    void drawLandscapeMenu(const HouseCatApp& app, std::uint64_t nowMs, MonoCanvas& canvas) const;
    void drawPortraitNotification(const HouseCatApp& app, std::uint64_t nowMs, MonoCanvas& canvas) const;
    void drawLandscapeNotification(const HouseCatApp& app, std::uint64_t nowMs, MonoCanvas& canvas) const;
    void drawPortraitDetail(const HouseCatApp& app, std::uint64_t nowMs, MonoCanvas& canvas) const;
    void drawLandscapeDetail(const HouseCatApp& app, std::uint64_t nowMs, MonoCanvas& canvas) const;

    static IconId notificationIcon(NotificationKind kind) noexcept;
    static IconId screenIcon(ScreenId screen) noexcept;
    static const char* screenTitle(ScreenId screen) noexcept;
    static const char* orientationLabel(Orientation orientation) noexcept;

    static int drawWrappedText(
        MonoCanvas& canvas,
        const Rect& bounds,
        std::string_view text,
        const BitmapFont& font,
        int scale = 1,
        int lineSpacing = 2,
        int maxLines = 3,
        bool centered = false,
        bool black = true);

    static void drawProgressBar(MonoCanvas& canvas, const Rect& bounds, std::uint8_t percent);
    static void drawPageDots(MonoCanvas& canvas, int centerX, int y, std::size_t count, std::size_t active);
    static void drawRockerGlyph(MonoCanvas& canvas, int x, int y, int size);
    static void drawSelectGlyph(MonoCanvas& canvas, int x, int y, int size);
    static void drawBackGlyph(MonoCanvas& canvas, int x, int y, int size);
    static void drawIconLabel(
        MonoCanvas& canvas,
        IconId icon,
        std::string_view label,
        int x,
        int y,
        int iconSize,
        bool inverted = false);
};

}  // namespace housecat

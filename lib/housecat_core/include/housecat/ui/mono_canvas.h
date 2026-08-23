#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "housecat/ui/bitmap.h"
#include "housecat/ui/font.h"
#include "housecat/ui/geometry.h"

namespace housecat {

class MonoCanvas final {
public:
    static constexpr int kPhysicalWidth = 122;
    static constexpr int kPhysicalHeight = 250;
    static constexpr int kStride = (kPhysicalWidth + 7) / 8;
    static constexpr int kBufferBytes = kStride * kPhysicalHeight;

    MonoCanvas();

    void setOrientation(Orientation orientation) noexcept;
    [[nodiscard]] Orientation orientation() const noexcept;
    [[nodiscard]] int width() const noexcept;
    [[nodiscard]] int height() const noexcept;

    void clear(bool black = false) noexcept;
    void setPixel(int x, int y, bool black = true) noexcept;
    [[nodiscard]] bool pixel(int x, int y) const noexcept;

    void drawLine(int x0, int y0, int x1, int y1, bool black = true) noexcept;
    void drawHorizontalLine(int x, int y, int width, bool black = true) noexcept;
    void drawVerticalLine(int x, int y, int height, bool black = true) noexcept;
    void drawRect(const Rect& rect, bool black = true) noexcept;
    void fillRect(const Rect& rect, bool black = true) noexcept;
    void drawCircle(int cx, int cy, int radius, bool black = true) noexcept;
    void fillCircle(int cx, int cy, int radius, bool black = true) noexcept;
    void drawRoundedRect(const Rect& rect, int radius, bool black = true) noexcept;
    void fillRoundedRect(const Rect& rect, int radius, bool black = true) noexcept;

    void drawBitmap(
        int x,
        int y,
        const Bitmap& bitmap,
        bool black = true,
        int targetWidth = 0,
        int targetHeight = 0,
        bool transparent = true) noexcept;

    [[nodiscard]] int measureText(
        std::string_view text,
        const BitmapFont& font,
        int scale = 1,
        int letterSpacing = 0) const noexcept;

    int drawText(
        int x,
        int y,
        std::string_view text,
        const BitmapFont& font,
        bool black = true,
        int scale = 1,
        int letterSpacing = 0) noexcept;

    void drawTextCentered(
        const Rect& bounds,
        std::string_view text,
        const BitmapFont& font,
        bool black = true,
        int scale = 1,
        int letterSpacing = 0) noexcept;

    [[nodiscard]] const std::uint8_t* data() const noexcept;
    [[nodiscard]] std::uint8_t* data() noexcept;
    [[nodiscard]] constexpr std::size_t size() const noexcept { return buffer_.size(); }

private:
    [[nodiscard]] Point toPhysical(int x, int y) const noexcept;
    [[nodiscard]] bool inLogicalBounds(int x, int y) const noexcept;
    void setPhysicalPixel(int x, int y, bool black) noexcept;
    [[nodiscard]] bool physicalPixel(int x, int y) const noexcept;

    Orientation orientation_{Orientation::Deg0};
    std::array<std::uint8_t, kBufferBytes> buffer_{};
};

}  // namespace housecat

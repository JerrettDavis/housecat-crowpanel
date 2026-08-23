#include "housecat/ui/mono_canvas.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace housecat {

MonoCanvas::MonoCanvas() { clear(false); }

void MonoCanvas::setOrientation(const Orientation orientation) noexcept { orientation_ = orientation; }
Orientation MonoCanvas::orientation() const noexcept { return orientation_; }

int MonoCanvas::width() const noexcept {
    return orientation_ == Orientation::Deg0 || orientation_ == Orientation::Deg180
        ? kPhysicalWidth
        : kPhysicalHeight;
}

int MonoCanvas::height() const noexcept {
    return orientation_ == Orientation::Deg0 || orientation_ == Orientation::Deg180
        ? kPhysicalHeight
        : kPhysicalWidth;
}

void MonoCanvas::clear(const bool black) noexcept { buffer_.fill(black ? 0xFF : 0x00); }

Point MonoCanvas::toPhysical(const int x, const int y) const noexcept {
    switch (orientation_) {
        case Orientation::Deg0:
            return {x, y};
        case Orientation::Deg90:
            return {kPhysicalWidth - 1 - y, x};
        case Orientation::Deg180:
            return {kPhysicalWidth - 1 - x, kPhysicalHeight - 1 - y};
        case Orientation::Deg270:
            return {y, kPhysicalHeight - 1 - x};
    }
    return {x, y};
}

bool MonoCanvas::inLogicalBounds(const int x, const int y) const noexcept {
    return x >= 0 && y >= 0 && x < width() && y < height();
}

void MonoCanvas::setPhysicalPixel(const int x, const int y, const bool black) noexcept {
    if (x < 0 || y < 0 || x >= kPhysicalWidth || y >= kPhysicalHeight) {
        return;
    }
    const std::size_t index = static_cast<std::size_t>(y * kStride + x / 8);
    const auto mask = static_cast<std::uint8_t>(0x80U >> (x % 8));
    if (black) {
        buffer_[index] |= mask;
    } else {
        buffer_[index] &= static_cast<std::uint8_t>(~mask);
    }
}

bool MonoCanvas::physicalPixel(const int x, const int y) const noexcept {
    if (x < 0 || y < 0 || x >= kPhysicalWidth || y >= kPhysicalHeight) {
        return false;
    }
    const std::size_t index = static_cast<std::size_t>(y * kStride + x / 8);
    const auto mask = static_cast<std::uint8_t>(0x80U >> (x % 8));
    return (buffer_[index] & mask) != 0;
}

void MonoCanvas::setPixel(const int x, const int y, const bool black) noexcept {
    if (!inLogicalBounds(x, y)) {
        return;
    }
    const auto physical = toPhysical(x, y);
    setPhysicalPixel(physical.x, physical.y, black);
}

bool MonoCanvas::pixel(const int x, const int y) const noexcept {
    if (!inLogicalBounds(x, y)) {
        return false;
    }
    const auto physical = toPhysical(x, y);
    return physicalPixel(physical.x, physical.y);
}

void MonoCanvas::drawLine(int x0, int y0, const int x1, const int y1, const bool black) noexcept {
    const int dx = std::abs(x1 - x0);
    const int sx = x0 < x1 ? 1 : -1;
    const int dy = -std::abs(y1 - y0);
    const int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;

    while (true) {
        setPixel(x0, y0, black);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        const int twice = 2 * error;
        if (twice >= dy) {
            error += dy;
            x0 += sx;
        }
        if (twice <= dx) {
            error += dx;
            y0 += sy;
        }
    }
}

void MonoCanvas::drawHorizontalLine(const int x, const int y, const int lineWidth, const bool black) noexcept {
    for (int px = x; px < x + lineWidth; ++px) {
        setPixel(px, y, black);
    }
}

void MonoCanvas::drawVerticalLine(const int x, const int y, const int lineHeight, const bool black) noexcept {
    for (int py = y; py < y + lineHeight; ++py) {
        setPixel(x, py, black);
    }
}

void MonoCanvas::drawRect(const Rect& rect, const bool black) noexcept {
    if (rect.width <= 0 || rect.height <= 0) {
        return;
    }
    drawHorizontalLine(rect.x, rect.y, rect.width, black);
    drawHorizontalLine(rect.x, rect.y + rect.height - 1, rect.width, black);
    drawVerticalLine(rect.x, rect.y, rect.height, black);
    drawVerticalLine(rect.x + rect.width - 1, rect.y, rect.height, black);
}

void MonoCanvas::fillRect(const Rect& rect, const bool black) noexcept {
    for (int py = rect.y; py < rect.y + rect.height; ++py) {
        drawHorizontalLine(rect.x, py, rect.width, black);
    }
}

void MonoCanvas::drawCircle(const int cx, const int cy, const int radius, const bool black) noexcept {
    int x = radius;
    int y = 0;
    int error = 0;
    while (x >= y) {
        setPixel(cx + x, cy + y, black);
        setPixel(cx + y, cy + x, black);
        setPixel(cx - y, cy + x, black);
        setPixel(cx - x, cy + y, black);
        setPixel(cx - x, cy - y, black);
        setPixel(cx - y, cy - x, black);
        setPixel(cx + y, cy - x, black);
        setPixel(cx + x, cy - y, black);
        ++y;
        if (error <= 0) {
            error += 2 * y + 1;
        }
        if (error > 0) {
            --x;
            error -= 2 * x + 1;
        }
    }
}

void MonoCanvas::fillCircle(const int cx, const int cy, const int radius, const bool black) noexcept {
    for (int y = -radius; y <= radius; ++y) {
        const int span = static_cast<int>(std::sqrt(static_cast<double>(radius * radius - y * y)));
        drawHorizontalLine(cx - span, cy + y, span * 2 + 1, black);
    }
}

void MonoCanvas::drawRoundedRect(const Rect& rect, int radius, const bool black) noexcept {
    radius = std::max(0, std::min(radius, std::min(rect.width, rect.height) / 2));
    if (radius == 0) {
        drawRect(rect, black);
        return;
    }
    drawHorizontalLine(rect.x + radius, rect.y, rect.width - 2 * radius, black);
    drawHorizontalLine(rect.x + radius, rect.bottom() - 1, rect.width - 2 * radius, black);
    drawVerticalLine(rect.x, rect.y + radius, rect.height - 2 * radius, black);
    drawVerticalLine(rect.right() - 1, rect.y + radius, rect.height - 2 * radius, black);
    for (int i = 0; i <= radius; ++i) {
        const int offset = static_cast<int>(std::sqrt(static_cast<double>(radius * radius - i * i)));
        setPixel(rect.x + radius - offset, rect.y + radius - i, black);
        setPixel(rect.right() - 1 - radius + offset, rect.y + radius - i, black);
        setPixel(rect.x + radius - offset, rect.bottom() - 1 - radius + i, black);
        setPixel(rect.right() - 1 - radius + offset, rect.bottom() - 1 - radius + i, black);
    }
}

void MonoCanvas::fillRoundedRect(const Rect& rect, int radius, const bool black) noexcept {
    if (rect.width <= 0 || rect.height <= 0) {
        return;
    }
    radius = std::max(0, std::min(radius, std::min(rect.width, rect.height) / 2));
    fillRect({rect.x + radius, rect.y, rect.width - 2 * radius, rect.height}, black);
    fillRect({rect.x, rect.y + radius, radius, rect.height - 2 * radius}, black);
    fillRect({rect.right() - radius, rect.y + radius, radius, rect.height - 2 * radius}, black);
    fillCircle(rect.x + radius, rect.y + radius, radius, black);
    fillCircle(rect.right() - 1 - radius, rect.y + radius, radius, black);
    fillCircle(rect.x + radius, rect.bottom() - 1 - radius, radius, black);
    fillCircle(rect.right() - 1 - radius, rect.bottom() - 1 - radius, radius, black);
}

void MonoCanvas::drawBitmap(
    const int x,
    const int y,
    const Bitmap& bitmap,
    const bool black,
    int targetWidth,
    int targetHeight,
    const bool transparent) noexcept {
    if (bitmap.data == nullptr || bitmap.width <= 0 || bitmap.height <= 0) {
        return;
    }
    targetWidth = targetWidth <= 0 ? bitmap.width : targetWidth;
    targetHeight = targetHeight <= 0 ? bitmap.height : targetHeight;
    const int sourceStride = (bitmap.width + 7) / 8;

    for (int dy = 0; dy < targetHeight; ++dy) {
        const int sy = dy * bitmap.height / targetHeight;
        for (int dx = 0; dx < targetWidth; ++dx) {
            const int sx = dx * bitmap.width / targetWidth;
            const auto source = bitmap.data[sy * sourceStride + sx / 8];
            const bool isInk = (source & (0x80U >> (sx % 8))) != 0;
            if (isInk) {
                setPixel(x + dx, y + dy, black);
            } else if (!transparent) {
                setPixel(x + dx, y + dy, !black);
            }
        }
    }
}

int MonoCanvas::measureText(
    const std::string_view text,
    const BitmapFont& font,
    const int scale,
    const int letterSpacing) const noexcept {
    if (text.empty()) {
        return 0;
    }
    return static_cast<int>(text.size()) * (font.width * scale + letterSpacing) - letterSpacing;
}

int MonoCanvas::drawText(
    int x,
    const int y,
    const std::string_view text,
    const BitmapFont& font,
    const bool black,
    const int scale,
    const int letterSpacing) noexcept {
    const int bytesPerRow = (font.width + 7) / 8;
    const int glyphBytes = bytesPerRow * font.height;

    for (const char rawCharacter : text) {
        const auto raw = static_cast<unsigned char>(rawCharacter);
        const unsigned char code = raw < font.first || raw > font.last ? static_cast<unsigned char>('?') : raw;
        const int glyphIndex = code - font.first;
        const std::uint8_t* glyph = font.data + glyphIndex * glyphBytes;
        for (int gy = 0; gy < font.height; ++gy) {
            for (int gx = 0; gx < font.width; ++gx) {
                const bool bit = (glyph[gy * bytesPerRow + gx / 8] & (0x80U >> (gx % 8))) != 0;
                if (!bit) {
                    continue;
                }
                fillRect({x + gx * scale, y + gy * scale, scale, scale}, black);
            }
        }
        x += font.width * scale + letterSpacing;
    }
    return x;
}

void MonoCanvas::drawTextCentered(
    const Rect& bounds,
    const std::string_view text,
    const BitmapFont& font,
    const bool black,
    const int scale,
    const int letterSpacing) noexcept {
    const int textWidth = measureText(text, font, scale, letterSpacing);
    const int textHeight = font.height * scale;
    drawText(
        bounds.x + (bounds.width - textWidth) / 2,
        bounds.y + (bounds.height - textHeight) / 2,
        text,
        font,
        black,
        scale,
        letterSpacing);
}

const std::uint8_t* MonoCanvas::data() const noexcept { return buffer_.data(); }
std::uint8_t* MonoCanvas::data() noexcept { return buffer_.data(); }

}  // namespace housecat

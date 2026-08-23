#pragma once

#include <algorithm>
#include <cstdint>

namespace housecat {

enum class Orientation : std::uint8_t {
    Deg0 = 0,
    Deg90 = 1,
    Deg180 = 2,
    Deg270 = 3,
};

struct Point {
    int x{};
    int y{};
};

struct Rect {
    int x{};
    int y{};
    int width{};
    int height{};

    [[nodiscard]] constexpr int right() const noexcept { return x + width; }
    [[nodiscard]] constexpr int bottom() const noexcept { return y + height; }
    [[nodiscard]] constexpr bool contains(int px, int py) const noexcept {
        return px >= x && py >= y && px < right() && py < bottom();
    }
};

[[nodiscard]] constexpr int clampInt(int value, int minValue, int maxValue) noexcept {
    return value < minValue ? minValue : (value > maxValue ? maxValue : value);
}

}  // namespace housecat

#pragma once

#include <cstdint>

namespace housecat {

struct BitmapFont {
    std::uint8_t first;
    std::uint8_t last;
    std::uint8_t width;
    std::uint8_t height;
    const std::uint8_t* data;
};

}  // namespace housecat

#pragma once

#include <cstdint>

namespace housecat {

struct Bitmap {
    std::uint16_t width;
    std::uint16_t height;
    const std::uint8_t* data;
};

}  // namespace housecat

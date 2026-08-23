#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "housecat/ui/mono_canvas.h"

namespace housecat::board {

using Jd79661Frame = std::array<std::uint8_t, MonoCanvas::kBufferBytes>;

inline void encodeJd79661Frame(const MonoCanvas& canvas, Jd79661Frame& output) noexcept {
    output.fill(0xFF);
    for (int sourceY = 0; sourceY < MonoCanvas::kPhysicalHeight; ++sourceY) {
        const int targetY = MonoCanvas::kPhysicalHeight - 1 - sourceY;
        const auto sourceOffset = static_cast<std::size_t>(sourceY * MonoCanvas::kStride);
        const auto targetOffset = static_cast<std::size_t>(targetY * MonoCanvas::kStride);
        for (int byte = 0; byte < MonoCanvas::kStride; ++byte) {
            output[targetOffset + static_cast<std::size_t>(byte)] =
                static_cast<std::uint8_t>(~canvas.data()[sourceOffset + static_cast<std::size_t>(byte)]);
        }
        // Only the first two bits of the final byte are visible (122 px).
        output[targetOffset + MonoCanvas::kStride - 1] |= 0x3FU;
    }
}

}  // namespace housecat::board

#pragma once

#include <cstdint>

namespace housecat::board::pins {

// CrowPanel ESP32 2.13-inch e-paper factory pin map.
inline constexpr std::uint8_t kEpaperBusy = 9;
inline constexpr std::uint8_t kEpaperReset = 10;
inline constexpr std::uint8_t kEpaperMosi = 11;
inline constexpr std::uint8_t kEpaperClock = 12;
inline constexpr std::uint8_t kEpaperDc = 13;
inline constexpr std::uint8_t kEpaperCs = 14;
inline constexpr std::uint8_t kDisplayPower = 7;
inline constexpr std::uint8_t kPowerLed = 19;

inline constexpr std::uint8_t kMenu = 2;
inline constexpr std::uint8_t kBack = 1;
inline constexpr std::uint8_t kRockerUp = 6;
inline constexpr std::uint8_t kRockerDown = 4;
inline constexpr std::uint8_t kRockerClick = 5;

inline constexpr std::uint8_t kExpansionA = 40;
inline constexpr std::uint8_t kExpansionB = 41;

}  // namespace housecat::board::pins

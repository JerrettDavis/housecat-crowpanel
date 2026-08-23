#pragma once

#include <cstdint>

#include "housecat/ui/bitmap.h"
#include "housecat/ui/font.h"

namespace housecat {

enum class CatPose : std::uint8_t {
    Content,
    Happy,
    Alert,
    Sleepy,
    Curious,
    Worried,
    Pet,
    Explorer,
};

enum class IconId : std::uint8_t {
    Home,
    Paw,
    Star,
    Radio,
    Book,
    Flask,
    Gear,
    Sun,
    Cloud,
    Rain,
    Thermometer,
    Wifi,
    Bell,
    Person,
    CarCharge,
    Heart,
    Check,
    Menu,
    Back,
    Mission,
    Rotation,
};

[[nodiscard]] const Bitmap& catBitmap(CatPose pose) noexcept;
[[nodiscard]] const Bitmap& iconBitmap(IconId icon) noexcept;
[[nodiscard]] const BitmapFont& smallFont() noexcept;
[[nodiscard]] const BitmapFont& boldFont() noexcept;
[[nodiscard]] const BitmapFont& readerFont() noexcept;

}  // namespace housecat

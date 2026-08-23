#include "housecat/ui/assets.h"

#include "housecat/generated/assets_generated.h"

namespace housecat {

const Bitmap& catBitmap(const CatPose pose) noexcept {
    using namespace generated;
    switch (pose) {
        case CatPose::Content: return kCatContent;
        case CatPose::Happy: return kCatHappy;
        case CatPose::Alert: return kCatAlert;
        case CatPose::Sleepy: return kCatSleepy;
        case CatPose::Curious: return kCatCurious;
        case CatPose::Worried: return kCatWorried;
        case CatPose::Pet: return kCatPet;
        case CatPose::Explorer: return kCatExplorer;
    }
    return kCatContent;
}

const Bitmap& iconBitmap(const IconId icon) noexcept {
    using namespace generated;
    switch (icon) {
        case IconId::Home: return kIconHome;
        case IconId::Paw: return kIconPaw;
        case IconId::Star: return kIconStar;
        case IconId::Radio: return kIconRadio;
        case IconId::Book: return kIconBook;
        case IconId::Flask: return kIconFlask;
        case IconId::Gear: return kIconGear;
        case IconId::Sun: return kIconSun;
        case IconId::Cloud: return kIconCloud;
        case IconId::Rain: return kIconRain;
        case IconId::Thermometer: return kIconThermometer;
        case IconId::Wifi: return kIconWifi;
        case IconId::Bell: return kIconBell;
        case IconId::Person: return kIconPerson;
        case IconId::CarCharge: return kIconCarCharge;
        case IconId::Heart: return kIconHeart;
        case IconId::Check: return kIconCheck;
        case IconId::Menu: return kIconMenu;
        case IconId::Back: return kIconBack;
        case IconId::Mission: return kIconMission;
        case IconId::Rotation: return kIconRotation;
    }
    return kIconHome;
}

const BitmapFont& smallFont() noexcept { return generated::kFontSmall; }
const BitmapFont& boldFont() noexcept { return generated::kFontBold; }
const BitmapFont& readerFont() noexcept { return generated::kFontReader; }

}  // namespace housecat

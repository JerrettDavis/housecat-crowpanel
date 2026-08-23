#pragma once

#include "housecat/app/housecat_app.h"

namespace housecat::board {

class PreferencesStore final {
public:
    [[nodiscard]] bool load(AppState& state);
    [[nodiscard]] bool save(const AppState& state);
    [[nodiscard]] bool clear();
};

}  // namespace housecat::board

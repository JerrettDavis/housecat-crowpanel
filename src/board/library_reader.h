#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "housecat/app/housecat_app.h"

namespace housecat::board {

class LibraryReader final {
public:
    explicit LibraryReader(HouseCatApp& app) : app_(app) {}

    void begin();
    void loop(std::uint64_t nowMs);
    [[nodiscard]] std::optional<DispatchResult> takeDispatchResult();

private:
    [[nodiscard]] bool downloadSelectedBook();
    [[nodiscard]] bool buildPageIndex(const char* path);
    [[nodiscard]] std::string loadPage(std::size_t pageIndex) const;
    [[nodiscard]] static const char* bookUrl(std::uint32_t gutenbergId) noexcept;
    void schedule(DispatchResult result);

    HouseCatApp& app_;
    std::optional<DispatchResult> pendingResult_{};
    std::vector<std::uint32_t> pageOffsets_{};
    std::uint64_t downloadNotBeforeMs_{0};
    bool storageReady_{false};
};

}  // namespace housecat::board

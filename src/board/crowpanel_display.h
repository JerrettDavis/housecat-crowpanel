#pragma once

#include <cstddef>
#include <cstdint>

#include "board/jd79661_frame.h"
#include "housecat/app/housecat_app.h"
#include "housecat/ui/mono_canvas.h"

namespace housecat::board {

class CrowPanelDisplay final {
public:
    void begin();
    void present(const MonoCanvas& canvas, RefreshKind refresh);
    void sleep();

private:
    void resetPanel();
    void writeCommand(std::uint8_t command);
    void writeData(std::uint8_t data);
    void writeData(const std::uint8_t* data, std::size_t size);
    bool waitUntilIdle(std::uint32_t timeoutMs, const char* operation);
    void initializeController();
    void writeFullLut();

    bool initialized_{false};
    bool fullLutPhase_{false};
    // Member storage avoids consuming half of the Arduino loop-task stack on
    // every refresh.
    Jd79661Frame panelFrame_{};
};

}  // namespace housecat::board

cmake_minimum_required(VERSION 3.20)
project(housecat_first_pass LANGUAGES CXX)

option(HOUSECAT_ENABLE_SANITIZERS "Enable address and undefined-behavior sanitizers" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(housecat_project_options INTERFACE)
target_compile_options(housecat_project_options INTERFACE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Wconversion -Wshadow>
)
if(HOUSECAT_ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(housecat_project_options INTERFACE -fno-omit-frame-pointer -fsanitize=address,undefined)
    target_link_options(housecat_project_options INTERFACE -fno-omit-frame-pointer -fsanitize=address,undefined)
endif()

add_library(housecat_core
    lib/housecat_core/src/assets.cpp
    lib/housecat_core/src/housecat_app.cpp
    lib/housecat_core/src/models.cpp
    lib/housecat_core/src/mono_canvas.cpp
    lib/housecat_core/src/progression.cpp
    lib/housecat_core/src/renderer.cpp
    lib/housecat_core/src/routine.cpp
)

target_include_directories(housecat_core PUBLIC
    lib/housecat_core/include
)

target_link_libraries(housecat_core PUBLIC housecat_project_options)

add_executable(housecat_simulator simulator/main.cpp)
target_link_libraries(housecat_simulator PRIVATE housecat_core)

add_executable(housecat_tests test/test_main.cpp)
target_link_libraries(housecat_tests PRIVATE housecat_core)
target_include_directories(housecat_tests PRIVATE src)

enable_testing()
add_test(NAME housecat_tests COMMAND housecat_tests)

# Host-only syntax and API-shape check for every embedded translation unit.
# These stubs are intentionally minimal; a successful check proves our code is
# internally well-formed against the Arduino/ESP32 APIs it uses, while the
# PlatformIO build remains the authoritative cross-compile.
add_library(housecat_embedded_compilecheck OBJECT
    src/main.cpp
    src/board/crowpanel_display.cpp
    src/board/crowpanel_input.cpp
    src/board/diagnostics.cpp
    src/board/preferences_store.cpp
    src/board/playground_lab.cpp
    src/board/library_reader.cpp
    src/integrations/mqtt_bridge.cpp
    src/integrations/tailscale_manager.cpp
    src/integrations/wifi_portal.cpp
    src/integrations/serial_console.cpp
)

target_include_directories(housecat_embedded_compilecheck PRIVATE
    test/embedded_stubs
    src
    include
    lib/housecat_core/include
)

target_compile_definitions(housecat_embedded_compilecheck PRIVATE
    ARDUINO=20017
    ESP32=1
    HOUSECAT_EMBEDDED_STUB=1
)
target_link_libraries(housecat_embedded_compilecheck PRIVATE housecat_project_options)

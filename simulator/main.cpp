#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "housecat/app/housecat_app.h"
#include "housecat/ui/mono_canvas.h"
#include "housecat/ui/renderer.h"

namespace {

void writePgm(const std::filesystem::path& path, const housecat::MonoCanvas& canvas) {
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("Could not write " + path.string());
    }
    output << "P5\n" << canvas.width() << ' ' << canvas.height() << "\n255\n";
    for (int y = 0; y < canvas.height(); ++y) {
        for (int x = 0; x < canvas.width(); ++x) {
            const unsigned char value = canvas.pixel(x, y) ? 0 : 255;
            output.write(reinterpret_cast<const char*>(&value), 1);
        }
    }
}

void render(
    housecat::HouseCatApp& app,
    const std::filesystem::path& output,
    const std::uint64_t nowMs = 10'000) {
    housecat::MonoCanvas canvas;
    housecat::UiRenderer renderer;
    renderer.render(app, nowMs, canvas);
    writePgm(output, canvas);
}

housecat::HomeSnapshot demoHome() {
    housecat::HomeSnapshot snapshot{};
    snapshot.outsideTemperatureF = 86.0F;
    snapshot.insideTemperatureF = 72.0F;
    snapshot.condition = housecat::WeatherCondition::Sunny;
    snapshot.conditionLabel = "Sunny";
    snapshot.palLabel = "PAL";
    snapshot.palMessage = "Everything looks comfy.";
    snapshot.rooms[0] = {"Office", 70.0F, 39.0F, true};
    snapshot.rooms[1] = {"Living", 73.0F, 42.0F, true};
    snapshot.roomCount = 2;
    return snapshot;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const std::filesystem::path outputDirectory = argc > 1 ? argv[1] : "previews/native";
        std::filesystem::create_directories(outputDirectory);

        constexpr std::uint64_t now = 10'000;
        housecat::HouseCatApp app;
        (void)app.setConnection(housecat::ConnectionState::HomeAssistant, -48, now);
        (void)app.updateHome(demoHome(), now);
        app.mutableState().cat.name = "Mochi";
        app.mutableState().cat.level = 4;
        app.mutableState().cat.totalXp = 260;
        app.mutableState().cat.bond = 67;
        app.mutableState().mission = {"charge", "Watch the Ioniq", "Tell me when charging is done.", 72, 100, false};

        render(app, outputDirectory / "01-home-weather-portrait.pgm", now);
        (void)app.dispatch(housecat::InputAction::Down, now + 1);
        render(app, outputDirectory / "02-home-rooms-portrait.pgm", now + 2);

        app.mutableState().ui.homeCardIndex = housecat::HouseCatApp::kHomeCardCount - 1;
        render(app, outputDirectory / "02b-home-pal-portrait.pgm", now + 2);
        (void)app.dispatch(housecat::InputAction::Menu, now + 3);
        (void)app.dispatch(housecat::InputAction::Down, now + 4);
        (void)app.dispatch(housecat::InputAction::Down, now + 5);
        render(app, outputDirectory / "03-menu-whiskers-portrait.pgm", now + 6);

        housecat::Notification person{};
        person.id = "sam-home";
        person.title = "Sam is home!";
        person.body = "Hoo-ray!";
        person.kind = housecat::NotificationKind::Person;
        person.priority = housecat::NotificationPriority::Notice;
        person.expiresAtMs = now + 120'000;
        (void)app.receiveNotification(person, now + 7);
        render(app, outputDirectory / "04-notification-person-portrait.pgm", now + 8);
        (void)app.dispatch(housecat::InputAction::Select, now + 9);

        app.mutableState().settings.orientation = housecat::Orientation::Deg90;
        app.mutableState().ui.screen = housecat::ScreenId::Home;
        app.mutableState().ui.homeCardIndex = 2;
        render(app, outputDirectory / "05-home-mission-landscape.pgm", now + 10);
        app.mutableState().ui.homeCardIndex = housecat::HouseCatApp::kHomeCardCount - 1;
        render(app, outputDirectory / "05b-home-pal-landscape.pgm", now + 10);

        app.mutableState().ui.screen = housecat::ScreenId::Cat;
        render(app, outputDirectory / "06-cat-profile-landscape.pgm", now + 11);

        housecat::Notification vehicle{};
        vehicle.id = "ioniq-ready";
        vehicle.title = "Ioniq 5 N is ready!";
        vehicle.body = "Charging is complete.";
        vehicle.kind = housecat::NotificationKind::Vehicle;
        vehicle.priority = housecat::NotificationPriority::Important;
        vehicle.expiresAtMs = now + 180'000;
        (void)app.receiveNotification(vehicle, now + 12);
        render(app, outputDirectory / "07-notification-car-landscape.pgm", now + 13);
        (void)app.dispatch(housecat::InputAction::Select, now + 14);

        app.mutableState().ui.screen = housecat::ScreenId::Settings;
        app.mutableState().ui.draftOrientation = housecat::Orientation::Deg270;
        render(app, outputDirectory / "08-settings-landscape.pgm", now + 15);

        housecat::PlaygroundState playground{};
        playground.networkCount = 3;
        playground.networks[0] = {"HouseCat-IoT", -48, 6, true};
        playground.networks[1] = {"Neighbor Cat", -67, 11, true};
        playground.networks[2] = {"Guest", -79, 1, false};
        (void)app.updatePlayground(playground, now + 16);
        app.mutableState().settings.orientation = housecat::Orientation::Deg0;
        app.mutableState().ui.screen = housecat::ScreenId::Whiskers;
        render(app, outputDirectory / "09-playground-portrait.pgm", now + 17);

        app.mutableState().settings.orientation = housecat::Orientation::Deg90;
        app.mutableState().ui.screen = housecat::ScreenId::Lab;
        app.mutableState().lab.pins[0].high = false;
        render(app, outputDirectory / "10-lab-landscape.pgm", now + 18);

        app.mutableState().settings.orientation = housecat::Orientation::Deg0;
        app.mutableState().ui.screen = housecat::ScreenId::Library;
        app.mutableState().library.view = housecat::LibraryView::Catalog;
        render(app, outputDirectory / "11-library-catalog-portrait.pgm", now + 19);

        (void)app.updateLibraryReady(
            "Alice was beginning to get very tired of sitting.",
            0,
            3200,
            now + 20);
        render(app, outputDirectory / "12-library-reader-portrait.pgm", now + 21);

        app.mutableState().settings.orientation = housecat::Orientation::Deg90;
        render(app, outputDirectory / "13-library-reader-landscape.pgm", now + 22);

        app.mutableState().settings.orientation = housecat::Orientation::Deg0;
        app.mutableState().ui.screen = housecat::ScreenId::Routine;
        app.mutableState().routine.activity = housecat::ActivityMode::Work;
        app.mutableState().routine.sessionSeconds = 21 * 60;
        render(app, outputDirectory / "14-routine-portrait.pgm", now + 23);
        app.mutableState().settings.orientation = housecat::Orientation::Deg90;
        render(app, outputDirectory / "15-routine-landscape.pgm", now + 24);

        std::cout << "Rendered House Cat snapshots to " << outputDirectory << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

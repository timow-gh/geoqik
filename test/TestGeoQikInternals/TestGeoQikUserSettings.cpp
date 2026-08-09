#include "GeoQikUserSettings.hpp"

#include <chrono>
#include <filesystem>
#include <gtest/gtest.h>
#include <string>

TEST(GeoQikUserSettingsTest, PersistsDefaultLogDirectory) {
    const auto uniqueSuffix = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path testDirectory =
        std::filesystem::temp_directory_path() / ("geoqik-user-settings-" + std::to_string(uniqueSuffix));
    const std::filesystem::path settingsPath = testDirectory / "settings.json";
    const std::filesystem::path logDirectory = testDirectory / std::filesystem::path{u8"l\u00f6gs"};

    geoqik::save_user_settings(settingsPath, geoqik::UserSettings{logDirectory});
    const geoqik::UserSettings loaded = geoqik::load_user_settings(settingsPath);

    EXPECT_EQ(loaded.defaultLogDirectory, logDirectory);

    std::error_code error;
    std::filesystem::remove_all(testDirectory, error);
}

#include "GeoQikUserSettings.hpp"

#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <string_view>

namespace geoqik {

namespace {

[[nodiscard]] std::filesystem::path current_directory() {
    std::error_code error;
    std::filesystem::path path = std::filesystem::current_path(error);
    return error ? std::filesystem::path{"."} : path;
}

#ifdef _WIN32
[[nodiscard]] std::filesystem::path environment_path(const wchar_t* name) {
    wchar_t* value = nullptr;
    std::size_t length = 0;
    if (::_wdupenv_s(&value, &length, name) != 0 || value == nullptr) {
        return {};
    }
    const std::filesystem::path path = value[0] == L'\0' ? std::filesystem::path{} : std::filesystem::path{value};
    std::free(value);
    return path;
}
#else
[[nodiscard]] std::filesystem::path environment_path(const char* name) {
    const char* value = std::getenv(name);
    return value == nullptr || value[0] == '\0' ? std::filesystem::path{} : std::filesystem::path{value};
}
#endif

[[nodiscard]] std::filesystem::path home_directory() {
#ifdef _WIN32
    return environment_path(L"USERPROFILE");
#else
    return environment_path("HOME");
#endif
}

[[nodiscard]] std::string path_to_utf8(const std::filesystem::path& path) {
    const std::u8string value = path.generic_u8string();
    return {reinterpret_cast<const char*>(value.data()), value.size()};
}

[[nodiscard]] std::filesystem::path path_from_utf8(std::string_view value) {
    const auto* begin = reinterpret_cast<const char8_t*>(value.data());
    return std::filesystem::path{std::u8string{begin, begin + value.size()}};
}

} // namespace

std::filesystem::path user_settings_file_path() {
    std::filesystem::path baseDirectory;
#ifdef _WIN32
    baseDirectory = environment_path(L"APPDATA");
    if (baseDirectory.empty()) {
        const std::filesystem::path home = home_directory();
        baseDirectory = home.empty() ? current_directory() : home / "AppData" / "Roaming";
    }
    return baseDirectory / "GeoQik" / "settings.json";
#elif defined(__APPLE__)
    const std::filesystem::path home = home_directory();
    baseDirectory = home.empty() ? current_directory() : home / "Library" / "Application Support";
    return baseDirectory / "GeoQik" / "settings.json";
#else
    baseDirectory = environment_path("XDG_CONFIG_HOME");
    if (baseDirectory.empty() || !baseDirectory.is_absolute()) {
        const std::filesystem::path home = home_directory();
        baseDirectory = home.empty() ? current_directory() : home / ".config";
    }
    return baseDirectory / "geoqik" / "settings.json";
#endif
}

std::filesystem::path default_log_directory() {
    const std::filesystem::path home = home_directory();
    return home.empty() ? current_directory() : home;
}

UserSettings load_user_settings(const std::filesystem::path& path) {
    UserSettings settings{default_log_directory()};
    std::error_code error;
    if (!std::filesystem::is_regular_file(path, error)) {
        return settings;
    }

    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("Failed to open GeoQik user settings");
    }

    const nlohmann::json json = nlohmann::json::parse(stream);
    const auto directory = json.find("defaultLogDirectory");
    if (directory != json.end() && directory->is_string()) {
        const std::string value = directory->get<std::string>();
        if (!value.empty()) {
            settings.defaultLogDirectory = path_from_utf8(value);
        }
    }
    return settings;
}

void save_user_settings(const std::filesystem::path& path, const UserSettings& settings) {
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }

    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        throw std::runtime_error("Failed to open GeoQik user settings for writing");
    }

    const nlohmann::json json{{"defaultLogDirectory", path_to_utf8(settings.defaultLogDirectory)}};
    stream << json.dump(2) << '\n';
    if (!stream) {
        throw std::runtime_error("Failed to write GeoQik user settings");
    }
}

} // namespace geoqik

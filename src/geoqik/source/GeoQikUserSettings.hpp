#ifndef GEOQIKUSERSETTINGS_HPP
#define GEOQIKUSERSETTINGS_HPP

#include <filesystem>

namespace geoqik {

struct UserSettings {
    std::filesystem::path defaultLogDirectory;
    /// User-chosen ffmpeg executable for video encoding. Empty means "auto-detect".
    std::filesystem::path ffmpegPath;
    /// Directory where recordings are written by default.
    std::filesystem::path recordingDirectory;
};

[[nodiscard]] std::filesystem::path user_settings_file_path();
[[nodiscard]] std::filesystem::path default_log_directory();
[[nodiscard]] std::filesystem::path default_recording_directory();
[[nodiscard]] UserSettings load_user_settings(const std::filesystem::path& path);
void save_user_settings(const std::filesystem::path& path, const UserSettings& settings);

} // namespace geoqik

#endif // GEOQIKUSERSETTINGS_HPP

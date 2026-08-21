#ifndef GEOQIK_VIDEO_FFMPEGLOCATOR_HPP
#define GEOQIK_VIDEO_FFMPEGLOCATOR_HPP

#include <filesystem>
#include <optional>

namespace geoqik::video {

/// Locates and validates an ffmpeg executable used for video encoding.
///
/// ffmpeg is invoked only as a separate process (never linked), so geoqik's license is unaffected
/// by ffmpeg's GPL components. Video encoding is unavailable until a working ffmpeg is found.
class FfmpegLocator {
  public:
    /// Runs "<ffmpeg> -version" and returns true when it launches and exits successfully.
    [[nodiscard]] static bool validate(const std::filesystem::path& ffmpegPath);

    /// Resolves an ffmpeg executable in priority order:
    ///   1. @p userConfiguredPath, when non-empty and valid,
    ///   2. "ffmpeg" resolved via PATH,
    ///   3. an "ffmpeg[.exe]" sibling of this application's executable,
    ///   4. common install locations (Windows: %ProgramFiles%\ffmpeg\bin, package-manager shims).
    /// Returns the first candidate that validate() accepts, or nullopt when none is found.
    [[nodiscard]] static std::optional<std::filesystem::path>
    find(const std::filesystem::path& userConfiguredPath = {});
};

} // namespace geoqik::video

#endif // GEOQIK_VIDEO_FFMPEGLOCATOR_HPP

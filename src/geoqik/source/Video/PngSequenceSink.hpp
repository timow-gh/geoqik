#ifndef GEOQIK_VIDEO_PNGSEQUENCESINK_HPP
#define GEOQIK_VIDEO_PNGSEQUENCESINK_HPP

#include "Video/VideoSink.hpp"

#include <cstdint>
#include <filesystem>

namespace geoqik::video {

/// Writes each frame as a numbered PNG (name_00001.png, name_00002.png, ...) via stb_image_write.
/// Requires no external tools, so it is always available as a fallback when ffmpeg is absent.
class PngSequenceSink final : public VideoSink {
  public:
    [[nodiscard]] bool open(int width, int height, int fps, const std::filesystem::path& outputPath) override;
    [[nodiscard]] bool write_frame(std::span<const std::uint8_t> rgb) override;
    [[nodiscard]] bool close() override;
    [[nodiscard]] std::filesystem::path output_path() const override { return m_directory; }

  private:
    std::filesystem::path m_directory;
    std::string m_stem;
    int m_width{0};
    int m_height{0};
    std::uint64_t m_frameIndex{0};
    bool m_open{false};
};

} // namespace geoqik::video

#endif // GEOQIK_VIDEO_PNGSEQUENCESINK_HPP

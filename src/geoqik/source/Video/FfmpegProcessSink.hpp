#ifndef GEOQIK_VIDEO_FFMPEGPROCESSSINK_HPP
#define GEOQIK_VIDEO_FFMPEGPROCESSSINK_HPP

#include "Video/VideoRecorder.hpp" // VideoQuality
#include "Video/VideoSink.hpp"

#include <filesystem>
#include <memory>

namespace geoqik::video {

/// Encodes frames by piping raw RGB to an external ffmpeg process' stdin.
///
/// ffmpeg is spawned as a separate process (never linked), so its GPL-licensed encoders do not
/// affect geoqik's license. The container/codec is selected from the output file extension, and
/// the CRF/pixel format from the requested quality.
class FfmpegProcessSink final : public VideoSink {
  public:
    explicit FfmpegProcessSink(std::filesystem::path ffmpegExecutable, VideoQuality quality = VideoQuality::High);
    ~FfmpegProcessSink() override;

    [[nodiscard]] bool open(int width, int height, int fps, const std::filesystem::path& outputPath) override;
    [[nodiscard]] bool write_frame(std::span<const std::uint8_t> rgb) override;
    [[nodiscard]] bool close() override;
    [[nodiscard]] std::filesystem::path output_path() const override { return m_outputPath; }

  private:
    struct Impl;

    std::filesystem::path m_ffmpegExecutable;
    std::filesystem::path m_outputPath;
    std::unique_ptr<Impl> m_impl;
    VideoQuality m_quality{VideoQuality::High};
    int m_width{0};
    int m_height{0};
    bool m_failed{false};
};

} // namespace geoqik::video

#endif // GEOQIK_VIDEO_FFMPEGPROCESSSINK_HPP

#ifndef GEOQIK_VIDEO_VIDEOSINK_HPP
#define GEOQIK_VIDEO_VIDEOSINK_HPP

#include <cstdint>
#include <filesystem>
#include <span>

namespace geoqik::video {

/// Consumes a stream of fixed-size RGB frames and writes them to disk in some container/format.
/// A sink is opened once with the frame geometry, fed frames, then closed to finalize output.
class VideoSink {
  public:
    VideoSink() = default;
    VideoSink(const VideoSink&) = delete;
    VideoSink& operator=(const VideoSink&) = delete;
    VideoSink(VideoSink&&) = delete;
    VideoSink& operator=(VideoSink&&) = delete;
    virtual ~VideoSink() = default;

    /// Prepares the sink to receive width*height RGB frames at @p fps. Returns false on failure.
    [[nodiscard]] virtual bool open(int width, int height, int fps, const std::filesystem::path& outputPath) = 0;

    /// Writes one RGB frame (width*height*3 bytes, top-row first). Returns false on failure; the
    /// caller should stop recording once a write fails.
    [[nodiscard]] virtual bool write_frame(std::span<const std::uint8_t> rgb) = 0;

    /// Finalizes and releases resources. Returns false if the output could not be completed.
    [[nodiscard]] virtual bool close() = 0;

    /// The path the finished output was (or will be) written to. For sinks that produce many files
    /// (e.g. a PNG sequence) this is the directory or pattern describing the output.
    [[nodiscard]] virtual std::filesystem::path output_path() const = 0;
};

} // namespace geoqik::video

#endif // GEOQIK_VIDEO_VIDEOSINK_HPP

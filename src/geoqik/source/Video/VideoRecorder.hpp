#ifndef GEOQIK_VIDEO_VIDEORECORDER_HPP
#define GEOQIK_VIDEO_VIDEORECORDER_HPP

#include "Video/FrameCapture.hpp"
#include "Video/VideoSink.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>

namespace geoqik::video {

enum class VideoFormat : std::uint8_t {
    Mp4,
    WebM,
    Gif,
    PngSequence,
};

/// Encoder to use. Video formats require a working ffmpeg; PngSequence never does.
[[nodiscard]] bool format_requires_ffmpeg(VideoFormat format);

/// File extension (including the dot) for a format, e.g. ".mp4".
[[nodiscard]] const char* format_extension(VideoFormat format);

/// Encode quality, mapped to a codec CRF by the sink. High is the default: it produces
/// visually-sharp output, which matters for geoqik's thin lines and edges.
enum class VideoQuality : std::uint8_t {
    High,     // near visually-lossless (default)
    Medium,   // smaller files, mild softening
    Low,      // smallest files, visibly softer
    Lossless, // largest files, pixel-perfect
};

/// How the offline log->video render paces log entries over time. Speed and Duration are mutually
/// exclusive; the field for the inactive mode is ignored.
enum class PacingMode : std::uint8_t {
    Speed,    // advance entriesPerSecond log entries per second of video
    Duration, // spread all entries across targetDurationSeconds seconds of video
};

struct VideoRecordOptions {
    /// Target dimensions in pixels. Zero means "use the current framebuffer/window size".
    int width{0};
    int height{0};
    int fps{60};
    VideoFormat format{VideoFormat::Mp4};
    VideoQuality quality{VideoQuality::High};
    /// Full output path. When empty, a timestamped name is generated under the recording directory.
    std::filesystem::path outputPath;
    /// Directory for the auto-generated filename when outputPath is empty.
    std::filesystem::path recordingDirectory;
    /// ffmpeg executable to use for video formats. Ignored for PngSequence.
    std::filesystem::path ffmpegExecutable;

    // --- Offline log->video pacing (ignored by live recording) ---
    PacingMode pacingMode{PacingMode::Speed};
    double entriesPerSecond{60.0};   // Speed mode: log entries applied per second of video.
    double targetDurationSeconds{0}; // Duration mode: total time for the log body (>0 to use).
    double holdStartSeconds{0.0};    // Freeze the initial frame this long before the log plays.
    double holdEndSeconds{0.0};      // Freeze the final frame this long after the log ends.
};

/// Owns a capture-and-encode session: reads frames from the GL front buffer and feeds them to a
/// sink. Resolution is fixed at start() so a mid-recording window resize cannot corrupt the stream.
///
/// This first implementation captures at the current window size only (the resolution-override
/// offscreen-FBO path is a planned follow-up); a requested size that differs from the window is
/// clamped to the window size.
class VideoRecorder {
  public:
    VideoRecorder();
    ~VideoRecorder();
    // Owns a live capture session and an open sink; copying or moving mid-recording is meaningless,
    // so all of copy/move are deleted explicitly (C.21 rule of five).
    VideoRecorder(const VideoRecorder&) = delete;
    VideoRecorder& operator=(const VideoRecorder&) = delete;
    VideoRecorder(VideoRecorder&&) = delete;
    VideoRecorder& operator=(VideoRecorder&&) = delete;

    /// Begins a recording. @p windowWidth/@p windowHeight are the current framebuffer pixel
    /// dimensions used when the options request the window size. Returns false if a recording is
    /// already active or the sink could not be opened.
    [[nodiscard]] bool start(const VideoRecordOptions& options, int windowWidth, int windowHeight);

    /// Captures one frame from the GL front buffer. Must be called with the GL context current,
    /// after the frame has been presented. A capture/encode failure stops the recording.
    void capture_frame();

    /// Writes the most recently captured frame @p extraFrames additional times without
    /// re-rendering. Used to freeze on the first/last frame for hold-at-start/hold-at-end. No-op
    /// if nothing has been captured yet.
    void hold_last_frame(std::size_t extraFrames);

    /// Finalizes the recording. Returns true on success. After stop(), final_output_path() and
    /// frame_count() describe the finished output.
    [[nodiscard]] bool stop();

    [[nodiscard]] bool is_recording() const { return m_recording; }
    [[nodiscard]] std::chrono::steady_clock::duration elapsed() const;
    [[nodiscard]] std::uint64_t frame_count() const { return m_frameCount; }
    [[nodiscard]] int width() const { return m_width; }
    [[nodiscard]] int height() const { return m_height; }
    [[nodiscard]] int fps() const { return m_fps; }
    [[nodiscard]] const std::filesystem::path& final_output_path() const { return m_outputPath; }

  private:
    FrameCapture m_capture;
    std::unique_ptr<VideoSink> m_sink;
    std::filesystem::path m_outputPath;
    int m_width{0};
    int m_height{0};
    int m_fps{60};
    std::uint64_t m_frameCount{0};
    bool m_recording{false};
    std::chrono::steady_clock::time_point m_startTime;
};

/// Builds a timestamped output path (e.g. geoqik_2026-08-11_14-30-05.mp4) under @p directory for
/// the given @p format. Ensures @p directory exists.
[[nodiscard]] std::filesystem::path
make_timestamped_output_path(const std::filesystem::path& directory, VideoFormat format);

} // namespace geoqik::video

#endif // GEOQIK_VIDEO_VIDEORECORDER_HPP

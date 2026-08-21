#include "Video/VideoRecorder.hpp"

#include "Video/FfmpegProcessSink.hpp"
#include "Video/PngSequenceSink.hpp"

#include <fmt/chrono.h>
#include <fmt/format.h>

#include <chrono>
#include <ctime>
#include <system_error>

namespace geoqik::video {

bool format_requires_ffmpeg(VideoFormat format) {
    return format != VideoFormat::PngSequence;
}

const char* format_extension(VideoFormat format) {
    switch (format) {
    case VideoFormat::Mp4: return ".mp4";
    case VideoFormat::WebM: return ".webm";
    case VideoFormat::Gif: return ".gif";
    case VideoFormat::PngSequence: return ".png";
    }
    return ".mp4";
}

std::filesystem::path make_timestamped_output_path(const std::filesystem::path& directory, VideoFormat format) {
    std::error_code error;
    if (!directory.empty()) {
        std::filesystem::create_directories(directory, error);
    }

    const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    const std::string name = fmt::format("geoqik_{:%Y-%m-%d_%H-%M-%S}{}", local, format_extension(format));
    return directory.empty() ? std::filesystem::path{name} : directory / name;
}

namespace {

[[nodiscard]] std::unique_ptr<VideoSink> make_sink(const VideoRecordOptions& options) {
    if (format_requires_ffmpeg(options.format)) {
        if (options.ffmpegExecutable.empty()) {
            return nullptr;
        }
        return std::make_unique<FfmpegProcessSink>(options.ffmpegExecutable, options.quality);
    }
    return std::make_unique<PngSequenceSink>();
}

} // namespace

VideoRecorder::VideoRecorder() = default;
VideoRecorder::~VideoRecorder() {
    if (m_recording) {
        (void)stop();
    }
}

bool VideoRecorder::start(const VideoRecordOptions& options, int windowWidth, int windowHeight) {
    if (m_recording) {
        return false;
    }
    if (windowWidth <= 0 || windowHeight <= 0 || options.fps <= 0) {
        return false;
    }

    // Resolution is locked here for the lifetime of the recording. Until the offscreen-FBO override
    // lands, a requested size is honoured only when it matches the window; otherwise we fall back to
    // the current window size so the captured pixels always match the requested dimensions.
    m_width = (options.width > 0 && options.width == windowWidth) ? options.width : windowWidth;
    m_height = (options.height > 0 && options.height == windowHeight) ? options.height : windowHeight;
    m_fps = options.fps;

    m_outputPath = options.outputPath.empty()
        ? make_timestamped_output_path(options.recordingDirectory, options.format)
        : options.outputPath;

    m_sink = make_sink(options);
    if (!m_sink) {
        return false;
    }
    if (!m_sink->open(m_width, m_height, m_fps, m_outputPath)) {
        m_sink.reset();
        return false;
    }
    // Reflect the sink's actual output location (e.g. the PNG sequence directory).
    m_outputPath = m_sink->output_path();

    m_frameCount = 0;
    m_recording = true;
    m_startTime = std::chrono::steady_clock::now();
    return true;
}

void VideoRecorder::capture_frame() {
    if (!m_recording || !m_sink) {
        return;
    }

    const std::vector<std::uint8_t>& pixels = m_capture.capture_front(m_width, m_height);
    if (!m_capture.last_capture_valid()) {
        (void)stop();
        return;
    }
    if (!m_sink->write_frame(pixels)) {
        (void)stop();
        return;
    }
    ++m_frameCount;
}

void VideoRecorder::hold_last_frame(std::size_t extraFrames) {
    if (!m_recording || !m_sink || !m_capture.last_capture_valid() || extraFrames == 0) {
        return;
    }

    const std::vector<std::uint8_t>& pixels = m_capture.last_pixels();
    for (std::size_t i = 0; i < extraFrames; ++i) {
        if (!m_sink->write_frame(pixels)) {
            (void)stop();
            return;
        }
        ++m_frameCount;
    }
}

bool VideoRecorder::stop() {
    if (!m_recording) {
        return false;
    }
    m_recording = false;
    bool success = false;
    if (m_sink) {
        success = m_sink->close();
        m_sink.reset();
    }
    return success;
}

std::chrono::steady_clock::duration VideoRecorder::elapsed() const {
    if (!m_recording) {
        return std::chrono::steady_clock::duration::zero();
    }
    return std::chrono::steady_clock::now() - m_startTime;
}

} // namespace geoqik::video

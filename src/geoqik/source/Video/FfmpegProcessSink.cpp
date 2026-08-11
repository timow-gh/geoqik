#include "Video/FfmpegProcessSink.hpp"

#include <boost/process/v1/args.hpp>
#include <boost/process/v1/child.hpp>
#include <boost/process/v1/exe.hpp>
#include <boost/process/v1/io.hpp>
#include <boost/process/v1/pipe.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>
#include <vector>

namespace bp = boost::process::v1;

namespace geoqik::video {

struct FfmpegProcessSink::Impl {
    bp::opstream input;
    bp::child child;
};

namespace {

[[nodiscard]] std::string lowercase_extension(const std::filesystem::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

/// CRF value for an x264/x265-style encoder. Lower is sharper/larger. These values keep geoqik's
/// thin lines and edges crisp at High while still allowing smaller files at Low.
[[nodiscard]] int crf_for_quality(VideoQuality quality) {
    switch (quality) {
    case VideoQuality::Lossless: return 0;
    case VideoQuality::High: return 18;
    case VideoQuality::Medium: return 23;
    case VideoQuality::Low: return 30;
    }
    return 18;
}

/// Container/codec arguments keyed by output extension, parameterised by quality. Values are
/// appended after the raw-video input specification and before the output path. Defaults to
/// H.264/MP4 for unknown extensions.
///
/// Uses yuv444p (full-resolution color) rather than yuv420p: geoqik draws thin, saturated lines
/// and edges on a dark background, and 4:2:0 chroma subsampling smears their color, making output
/// look coarse even at high resolution. yuv444p keeps edges crisp; if broad player compatibility
/// is ever required, switch this back to yuv420p.
[[nodiscard]] std::vector<std::string> container_args(const std::string& extension, VideoQuality quality) {
    const std::string crf = std::to_string(crf_for_quality(quality));

    if (extension == ".webm") {
        // VP9 with constant quality: -b:v 0 selects CRF-driven rate control.
        return {"-c:v", "libvpx-vp9", "-pix_fmt", "yuv444p", "-b:v", "0", "-crf", crf};
    }
    if (extension == ".gif") {
        // Two-pass palette generation in a single filtergraph for good-quality GIFs.
        return {"-vf", "split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse"};
    }
    // .mp4, .mov and everything else: H.264. +faststart moves the moov atom to the front so the
    // file starts playing before it is fully downloaded.
    return {"-c:v", "libx264", "-pix_fmt", "yuv444p", "-crf", crf, "-movflags", "+faststart"};
}

} // namespace

FfmpegProcessSink::FfmpegProcessSink(std::filesystem::path ffmpegExecutable, VideoQuality quality)
    : m_ffmpegExecutable(std::move(ffmpegExecutable)), m_quality(quality) {}

FfmpegProcessSink::~FfmpegProcessSink() {
    if (m_impl) {
        // Best-effort cleanup if the caller never called close().
        (void)close();
    }
}

bool FfmpegProcessSink::open(int width, int height, int fps, const std::filesystem::path& outputPath) {
    if (width <= 0 || height <= 0 || fps <= 0 || outputPath.empty() || m_ffmpegExecutable.empty()) {
        return false;
    }

    m_width = width;
    m_height = height;
    m_outputPath = outputPath;
    m_failed = false;

    // ffmpeg reads raw RGB frames from stdin ("-i -") and writes to the output file. stdout/stderr
    // are discarded so a synchronous stdin write cannot deadlock on an unread output pipe.
    std::vector<std::string> args;
    args.emplace_back("-y");
    args.emplace_back("-f");
    args.emplace_back("rawvideo");
    args.emplace_back("-pixel_format");
    args.emplace_back("rgb24");
    args.emplace_back("-video_size");
    args.emplace_back(fmt::format("{}x{}", width, height));
    args.emplace_back("-framerate");
    args.emplace_back(fmt::format("{}", fps));
    args.emplace_back("-i");
    args.emplace_back("-");
    for (std::string& arg : container_args(lowercase_extension(outputPath), m_quality)) {
        args.emplace_back(std::move(arg));
    }
    args.emplace_back(outputPath.string());

    try {
        m_impl = std::make_unique<Impl>();
        std::error_code error;
        m_impl->child = bp::child(bp::exe = m_ffmpegExecutable.string(),
                                  bp::args = args,
                                  bp::std_in < m_impl->input,
                                  bp::std_out > bp::null,
                                  bp::std_err > bp::null,
                                  error);
        if (error || !m_impl->child.running()) {
            m_impl.reset();
            return false;
        }
    } catch (...) {
        m_impl.reset();
        return false;
    }
    return true;
}

bool FfmpegProcessSink::write_frame(std::span<const std::uint8_t> rgb) {
    if (!m_impl || m_failed) {
        return false;
    }
    const std::size_t expected = static_cast<std::size_t>(m_width) * static_cast<std::size_t>(m_height) * 3U;
    if (rgb.size() < expected) {
        m_failed = true;
        return false;
    }

    m_impl->input.write(reinterpret_cast<const char*>(rgb.data()), static_cast<std::streamsize>(expected));
    if (!m_impl->input) {
        m_failed = true;
        return false;
    }
    return true;
}

bool FfmpegProcessSink::close() {
    if (!m_impl) {
        return false;
    }

    bool success = !m_failed;
    try {
        m_impl->input.flush();
        m_impl->input.pipe().close(); // EOF on stdin tells ffmpeg to finalize the file.
        std::error_code error;
        m_impl->child.wait(error);
        if (error || m_impl->child.exit_code() != 0) {
            success = false;
        }
    } catch (...) {
        success = false;
    }
    m_impl.reset();
    return success;
}

} // namespace geoqik::video

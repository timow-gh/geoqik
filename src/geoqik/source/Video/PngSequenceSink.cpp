#include "Video/PngSequenceSink.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <fmt/format.h>

#include <cstddef>
#include <system_error>

namespace geoqik::video {

bool PngSequenceSink::open(int width, int height, int /*fps*/, const std::filesystem::path& outputPath) {
    if (width <= 0 || height <= 0 || outputPath.empty()) {
        return false;
    }

    // The output path names a representative file (e.g. capture.png); frames are written next to it
    // in a dedicated directory sharing the file stem so the sequence stays grouped and discoverable.
    m_stem = outputPath.stem().string();
    if (m_stem.empty()) {
        m_stem = "frame";
    }
    const std::filesystem::path parent = outputPath.has_parent_path() ? outputPath.parent_path() : ".";
    m_directory = parent / fmt::format("{}_frames", m_stem);

    std::error_code error;
    std::filesystem::create_directories(m_directory, error);
    if (error) {
        return false;
    }

    m_width = width;
    m_height = height;
    m_frameIndex = 0;
    m_open = true;
    return true;
}

bool PngSequenceSink::write_frame(std::span<const std::uint8_t> rgb) {
    if (!m_open) {
        return false;
    }
    const std::size_t expected = static_cast<std::size_t>(m_width) * static_cast<std::size_t>(m_height) * 3U;
    if (rgb.size() < expected) {
        return false;
    }

    const std::filesystem::path file = m_directory / fmt::format("{}_{:05d}.png", m_stem, m_frameIndex + 1);
    const std::string filePath = file.string();
    const int rowStride = m_width * 3;
    const int result =
        stbi_write_png(filePath.c_str(), m_width, m_height, 3, rgb.data(), rowStride);
    if (result == 0) {
        return false;
    }
    ++m_frameIndex;
    return true;
}

bool PngSequenceSink::close() {
    m_open = false;
    // Every frame is flushed on write, so there is nothing to finalize. Report success only when at
    // least one frame was written, so an empty recording surfaces as a failure to the caller.
    return m_frameIndex > 0;
}

} // namespace geoqik::video

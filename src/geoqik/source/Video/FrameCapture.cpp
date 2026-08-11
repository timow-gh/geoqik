#include "Video/FrameCapture.hpp"

#include <glad/glad.h>

#include <cstddef>

namespace geoqik::video {

namespace {

/// Flips the image vertically in place, one row at a time. glReadPixels returns rows
/// bottom-to-top; encoders and PNG writers expect top-to-bottom.
void flip_rows(std::vector<std::uint8_t>& pixels, int width, int height, std::vector<std::uint8_t>& rowScratch) {
    const std::size_t stride = static_cast<std::size_t>(width) * 3U;
    rowScratch.resize(stride);
    for (int row = 0; row < height / 2; ++row) {
        std::uint8_t* top = pixels.data() + static_cast<std::size_t>(row) * stride;
        std::uint8_t* bottom = pixels.data() + static_cast<std::size_t>(height - 1 - row) * stride;
        std::copy(top, top + stride, rowScratch.data());
        std::copy(bottom, bottom + stride, top);
        std::copy(rowScratch.data(), rowScratch.data() + stride, bottom);
    }
}

} // namespace

bool FrameCapture::capture(int width, int height, std::vector<std::uint8_t>& out) {
    m_lastValid = false;
    if (width <= 0 || height <= 0) {
        return false;
    }

    const std::size_t byteCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3U;
    out.resize(byteCount);

    // Tightly pack rows so each row is exactly width*3 bytes with no padding.
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, out.data());

    flip_rows(out, width, height, m_rowScratch);
    m_lastValid = true;
    return true;
}

const std::vector<std::uint8_t>& FrameCapture::capture_front(int width, int height) {
    // Read from the front buffer: plinth's end_frame() has already swapped, so the just-presented
    // frame lives there. Reading the front buffer is well-defined on desktop GL immediately after
    // the swap and avoids any modification to plinth's render loop.
    glReadBuffer(GL_FRONT);
    if (!capture(width, height, m_buffer)) {
        m_buffer.clear();
    }
    return m_buffer;
}

} // namespace geoqik::video

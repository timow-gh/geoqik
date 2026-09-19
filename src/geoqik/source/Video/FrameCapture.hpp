#ifndef GEOQIK_VIDEO_FRAMECAPTURE_HPP
#define GEOQIK_VIDEO_FRAMECAPTURE_HPP

#include <cstdint>
#include <vector>

namespace renderer {
class Renderer;
} // namespace renderer

namespace geoqik::video {

/// Reads RGB pixels back from an OpenGL framebuffer into a CPU buffer.
///
/// OpenGL's window-space origin is bottom-left, whereas video encoders and image writers
/// expect a top-left origin, so captured rows are flipped vertically. The internal buffer is
/// reused across frames to avoid per-frame allocations.
class FrameCapture {
  public:
    /// Reads a @p width by @p height RGB image from the buffer currently selected for reading
    /// (see capture_front) and stores it, top-row first, in @p out (sized to width*height*3).
    /// Must be called on the thread owning the GL context, with that context current.
    /// Returns false when width or height is not positive.
    [[nodiscard]] bool capture(int width, int height, std::vector<std::uint8_t>& out);

    /// Convenience overload capturing into the reusable internal buffer. The returned span is
    /// valid until the next capture() call. Empty on failure.
    [[nodiscard]] const std::vector<std::uint8_t>& capture_front(int width, int height);

    /// Captures only the 3D scene image (no UI, no overlay) from @p renderer into the reusable
    /// internal buffer, flipped to top-row-first like capture_front. Reads the renderer's
    /// post-processed scene target, which is valid after the frame's end_frame(). @p width and
    /// @p height are the expected scene dimensions; a mismatch or read failure yields an empty
    /// span (last_capture_valid() then returns false). The returned span is valid until the next
    /// capture call.
    [[nodiscard]] const std::vector<std::uint8_t>&
    capture_scene(const renderer::Renderer& renderer, int width, int height);

    /// The pixels from the most recent capture_front() call. Valid until the next capture.
    [[nodiscard]] const std::vector<std::uint8_t>& last_pixels() const { return m_buffer; }

    [[nodiscard]] bool last_capture_valid() const { return m_lastValid; }

  private:
    std::vector<std::uint8_t> m_buffer;
    std::vector<std::uint8_t> m_rowScratch;
    bool m_lastValid{false};
};

} // namespace geoqik::video

#endif // GEOQIK_VIDEO_FRAMECAPTURE_HPP

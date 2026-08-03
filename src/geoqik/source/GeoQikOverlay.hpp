#ifndef GEOQIKOVERLAY_HPP
#define GEOQIKOVERLAY_HPP

#include <plinth/IOverlay.hpp>
#include <plinth/ImGuiOverlay.hpp>
#include <plinth/InputState.hpp>

#include <cstdint>
#include <functional>
#include <utility>

namespace geoqik {

// Thin IOverlay wrapper around plinth's built-in ImGuiOverlay. Every IOverlay method delegates to
// the inner overlay EXCEPT build_controls, which is routed to an application-provided callback so
// geoqik can compose its own single unified control panel (instead of plinth's built-in
// Camera/post-processing layout). The inner ImGuiOverlay still owns the ImGui context, backends,
// input arbitration and the control-panel window/layout.
class GeoQikOverlay final : public renderer::IOverlay {
    renderer::ImGuiOverlay m_inner;
    std::function<void(renderer::OverlayFrameContext&)> m_buildControls;

  public:
    explicit GeoQikOverlay(void* nativeWindow)
        : m_inner(nativeWindow) {}

    [[nodiscard]] renderer::ImGuiOverlay& inner() { return m_inner; }

    void set_build_controls(std::function<void(renderer::OverlayFrameContext&)> fn) {
        m_buildControls = std::move(fn);
    }

    void new_frame() override { m_inner.new_frame(); }

    void build_controls(renderer::OverlayFrameContext& ctx) override {
        if (m_buildControls) {
            m_buildControls(ctx);
        }
    }

    void render() override { m_inner.render(); }
    void end_frame() override { m_inner.end_frame(); }

    [[nodiscard]] bool wants_mouse() const override { return m_inner.wants_mouse(); }
    [[nodiscard]] bool wants_keyboard() const override { return m_inner.wants_keyboard(); }

    [[nodiscard]] bool handle_cursor_position(double xpos, double ypos) override {
        return m_inner.handle_cursor_position(xpos, ypos);
    }
    [[nodiscard]] bool handle_mouse_button(int button, renderer::Action action, renderer::Mods mods) override {
        return m_inner.handle_mouse_button(button, action, mods);
    }
    [[nodiscard]] bool handle_scroll(double xoffset, double yoffset) override {
        return m_inner.handle_scroll(xoffset, yoffset);
    }
    [[nodiscard]] bool
    handle_key(renderer::Key key, renderer::Scancode scancode, renderer::Action action, renderer::Mods mods) override {
        return m_inner.handle_key(key, scancode, action, mods);
    }
    void handle_char(std::uint32_t codepoint) override { m_inner.handle_char(codepoint); }
};

} // namespace geoqik

#endif // GEOQIKOVERLAY_HPP

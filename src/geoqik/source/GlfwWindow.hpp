#ifndef GEOQIK_SOURCE_GLFWWINDOW_HPP
#define GEOQIK_SOURCE_GLFWWINDOW_HPP

#include "InputState.hpp"
#include "WindowSettings.hpp"
#include <utility>

namespace geoqik
{

class GlfwWindow
{
  GLFWwindow* m_window{nullptr};

public:
  GlfwWindow() = default;
  GlfwWindow(const GlfwWindow&) = delete;
  GlfwWindow& operator=(const GlfwWindow&) = delete;
  GlfwWindow(GlfwWindow&&) = delete;
  GlfwWindow& operator=(GlfwWindow&&) = delete;
  ~GlfwWindow();

  [[nodiscard]] bool create(const WindowSettings& settings);
  void destroy();

  [[nodiscard]] bool is_initialized() const { return m_window != nullptr; }

  void make_context_current() const;
  void poll_events() const;
  void swap_buffers() const;

  [[nodiscard]] bool should_close() const;
  [[nodiscard]] bool is_escape_pressed() const;
  [[nodiscard]] std::pair<int, int> get_framebuffer_size() const;

  [[nodiscard]] InputState& get_input_state() const;

  void set_key_callback(KeyCB cb);
  void set_cursor_pos_callback(CursorPosCB cb);
  void set_scroll_callback(ScrollCB cb);
  void set_mouse_button_callback(MouseBtnCB cb);
  void set_framebuffer_size_callback(FramebufferSizeCB cb);

private:
  [[nodiscard]] static int window_hint_to_glfw_hint(bool hint) { return hint ? 1 : 0; }
  static void set_window_hints(const WindowSettings& hints);
};

} // namespace geoqik

#endif // GEOQIK_SOURCE_GLFWWINDOW_HPP

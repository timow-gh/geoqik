#ifndef IMGUIOVERLAY_HPP
#define IMGUIOVERLAY_HPP

#include "CameraProjectionType.hpp"
#include "InputCaptureState.hpp"
#include <cstdint>

struct GLFWwindow;

namespace geoqik
{

class ImGuiOverlay
{
  GLFWwindow* m_window{nullptr};
  InputCaptureState m_inputCaptureState;

public:
  explicit ImGuiOverlay(GLFWwindow* window);
  ImGuiOverlay(const ImGuiOverlay&) = delete;
  ImGuiOverlay& operator=(const ImGuiOverlay&) = delete;
  ImGuiOverlay(ImGuiOverlay&&) = delete;
  ImGuiOverlay& operator=(ImGuiOverlay&&) = delete;
  ~ImGuiOverlay();
  void new_frame();
  void draw_controls(bool& autoZoomEnabled, CameraProjectionType& projectionType);
  void render();

  [[nodiscard]] bool wants_keyboard() const;
  [[nodiscard]] bool handle_cursor_position(double xpos, double ypos);
  [[nodiscard]] bool handle_mouse_button(int button, Action action, Mods mods);
  [[nodiscard]] bool handle_scroll(double xoffset, double yoffset);
  [[nodiscard]] bool handle_key(Key key, Scancode scancode, Action action, Mods mods);
  void handle_char(std::uint32_t codepoint);
};

} // namespace geoqik

#endif // IMGUIOVERLAY_HPP

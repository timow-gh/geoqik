#ifndef RENDERER_RENDERER_HPP
#define RENDERER_RENDERER_HPP

#include <Renderer/CameraInteractor.hpp>
#include <Renderer/GlfwWindow.hpp>
#include <Renderer/ImGuiOverlay.hpp>
#include <Renderer/InputState.hpp>
#include <Renderer/WindowSettings.hpp>
#include <OpenGL/BufferAccessPattern.hpp>
#include <OpenGL/Drawable/DrawablesManager.hpp>
#include <OpenGL/FrameState.hpp>
#include <OpenGL/LineType.hpp>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace renderer
{

class Renderer
{
public:
  Renderer() = delete;
  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;
  Renderer(Renderer&&) = delete;
  Renderer& operator=(Renderer&&) = delete;
  ~Renderer() = default;

  [[nodiscard]] static std::unique_ptr<Renderer> create(const WindowSettings& settings);

  // --- Geometry ---
  void add_point_drawable(std::span<const float> vertices,
                          std::span<const float> colors,
                          std::span<const std::uint32_t> indices,
                          float pointSize,
                          opengl::BufferAccessPattern accessPattern = opengl::BufferAccessPattern::STATIC_DRAW);

  void add_line_drawable(std::span<const float> vertices,
                         std::span<const std::uint32_t> indices,
                         std::span<const float> colors,
                         opengl::LineType lineType,
                         float lineWidth,
                         float pointSize = 0.0F,
                         opengl::BufferAccessPattern accessPattern = opengl::BufferAccessPattern::STATIC_DRAW);

  void add_mesh_drawable(std::span<const float> vertices,
                         std::span<const float> normals,
                         std::span<const float> colors,
                         std::span<const std::uint32_t> triangleIndices,
                         opengl::BufferAccessPattern accessPattern = opengl::BufferAccessPattern::STATIC_DRAW);

  void update_last_point_drawable(std::span<const float> vertices,
                                  std::span<const float> colors,
                                  std::span<const std::uint32_t> indices,
                                  opengl::BufferAccessPattern accessPattern);

  void update_last_line_drawable(std::span<const float> vertices,
                                 std::span<const float> colors,
                                 std::span<const std::uint32_t> indices,
                                 opengl::BufferAccessPattern accessPattern);

  void clear_point_drawables();
  void clear_line_drawables();
  void clear_mesh_drawables();
  void clear_drawables();

  // --- Frame lifecycle ---
  static void poll_events();
  [[nodiscard]] bool should_close() const;
  [[nodiscard]] bool is_escape_pressed() const;

  void begin_frame(const opengl::ClearColor& clearColor = defaultClearColor);
  void draw();
  void end_frame();

  // --- Callback extension points ---
  void add_cursor_pos_callback(CursorPosCB cb);
  void add_scroll_callback(ScrollCB cb);
  void add_mouse_button_callback(MouseBtnCB cb);
  void add_key_callback(KeyCB cb);

  // --- Accessors ---
  [[nodiscard]] GlfwWindow& window() { return m_window; }
  [[nodiscard]] const GlfwWindow& window() const { return m_window; }
  [[nodiscard]] CameraInteractor& camera() { return m_camera; }
  [[nodiscard]] const CameraInteractor& camera() const { return m_camera; }
  [[nodiscard]] ImGuiOverlay& imgui() { return *m_imgui; }

  static constexpr opengl::ClearColor defaultClearColor{0.05F, 0.05F, 0.08F, 1.0F};

private:
  Renderer(GlfwWindow window,
           opengl::DrawablesManager drawablesManager,
           CameraInteractor camera,
           std::unique_ptr<ImGuiOverlay> imgui);

  void wire_callbacks();

  GlfwWindow m_window;
  opengl::DrawablesManager m_drawablesManager;
  CameraInteractor m_camera;
  std::unique_ptr<ImGuiOverlay> m_imgui;

  std::vector<CursorPosCB> m_cursorPosCallbacks;
  std::vector<ScrollCB> m_scrollCallbacks;
  std::vector<MouseBtnCB> m_mouseButtonCallbacks;
  std::vector<KeyCB> m_keyCallbacks;
};

} // namespace renderer

#endif // RENDERER_RENDERER_HPP

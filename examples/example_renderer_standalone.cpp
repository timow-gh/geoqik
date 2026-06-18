#include <Renderer/Camera.hpp>
#include <Renderer/CameraInteractor.hpp>
#include <Renderer/GlfwWindow.hpp>
#include <Renderer/ImGuiOverlay.hpp>
#include <Renderer/WindowSettings.hpp>

#include <OpenGL/BufferAccessPattern.hpp>
#include <OpenGL/Drawable/DrawablesManager.hpp>
#include <OpenGL/FrameState.hpp>
#include <OpenGL/LineType.hpp>
#include <OpenGL/Programs/ProgramManager.hpp>

#include <array>
#include <cstdint>

int main()
{
  renderer::WindowSettings windowSettings;
  windowSettings.title = "renderer standalone example";
  windowSettings.width = 1024;
  windowSettings.height = 768;

  renderer::GlfwWindow window;
  if (!window.create(windowSettings))
  {
    return 1;
  }

  opengl::ProgramManager programManager;
  programManager.compile();
  opengl::DrawablesManager drawablesManager(&programManager.get_point_program(), &programManager.get_line_program());

  // One point at the origin.
  const std::array<float, 3> pointVertices{0.0f, 0.0f, 0.0f};
  const std::array<float, 4> pointColors{1.0f, 1.0f, 0.0f, 1.0f}; // yellow
  const std::array<std::uint32_t, 1> pointIndices{0};
  drawablesManager.add_point_drawable(pointVertices, 3, pointColors, 4, pointIndices, /*pointSize=*/12.0f, opengl::BufferAccessPattern::STATIC_DRAW);

  // A cross made of two line segments through the origin.
  const std::array<float, 12> lineVertices{
      -1.0f, 0.0f, 0.0f, // v0
      1.0f,  0.0f, 0.0f, // v1
      0.0f,  -1.0f, 0.0f, // v2
      0.0f,  1.0f,  0.0f  // v3
  };
  const std::array<float, 16> lineColors{
      1.0f, 0.0f, 0.0f, 1.0f, // v0 red
      1.0f, 0.0f, 0.0f, 1.0f, // v1 red
      0.0f, 1.0f, 0.0f, 1.0f, // v2 green
      0.0f, 1.0f, 0.0f, 1.0f  // v3 green
  };
  const std::array<std::uint32_t, 4> lineIndices{0, 1, 2, 3};
  drawablesManager.add_line_drawable(lineVertices,
                                     3,
                                     lineIndices,
                                     lineColors,
                                     4,
                                     opengl::LineType::lines(),
                                     /*lineWidth=*/2.0f,
                                     /*pointSize=*/1.0f,
                                     opengl::BufferAccessPattern::STATIC_DRAW);

  const auto [framebufferWidth, framebufferHeight] = window.get_framebuffer_size();
  renderer::CameraSettings cameraSettings;
  cameraSettings.m_camera.set_viewport(0, 0, static_cast<std::uint32_t>(framebufferWidth), static_cast<std::uint32_t>(framebufferHeight));
  cameraSettings.m_defaultPosition = linal::double3{5.0, 5.0, 5.0};
  renderer::CameraInteractor cameraInteractor(window.get_input_state(), cameraSettings);

  renderer::ImGuiOverlay imguiOverlay(window.get_native_handle());
  bool autoZoomEnabled = false; // unused by this example, draw_controls still needs a bound value
  renderer::CameraProjectionType projectionType = renderer::CameraProjectionType::PERSPECTIVE;

  window.set_cursor_pos_callback(
      [&](double xpos, double ypos)
      {
        if (!imguiOverlay.handle_cursor_position(xpos, ypos))
        {
          return;
        }
        cameraInteractor.on_cursor_position(xpos, ypos);
      });
  window.set_scroll_callback(
      [&](double xoff, double yoff)
      {
        if (!imguiOverlay.handle_scroll(xoff, yoff))
        {
          return;
        }
        cameraInteractor.on_scroll(xoff, yoff);
      });
  window.set_mouse_button_callback(
      [&](int button, renderer::Action action, renderer::Mods mods)
      {
        if (!imguiOverlay.handle_mouse_button(button, action, mods))
        {
          return;
        }
        cameraInteractor.on_mouse_button(button, action, mods);
      });
  window.set_key_callback(
      [&](renderer::Key key, renderer::Scancode scancode, renderer::Action action, renderer::Mods mods)
      { (void)imguiOverlay.handle_key(key, scancode, action, mods); });
  window.set_char_callback([&](std::uint32_t codepoint) { imguiOverlay.handle_char(codepoint); });
  window.set_framebuffer_size_callback(
      [&](std::uint32_t width, std::uint32_t height) { cameraInteractor.on_framebuffer_size(width, height); });

  while (!window.should_close())
  {
    renderer::GlfwWindow::poll_events();
    if (!imguiOverlay.wants_keyboard() && window.is_escape_pressed())
    {
      break;
    }

    const auto [width, height] = window.get_framebuffer_size();
    opengl::begin_frame(opengl::ClearColor{0.05f, 0.05f, 0.08f, 1.0f}, opengl::ViewportRect{0, 0, width, height});

    drawablesManager.draw_lines_and_points(cameraInteractor.get_current_MVP(), cameraInteractor.get_position());

    imguiOverlay.new_frame();
    imguiOverlay.draw_controls(autoZoomEnabled, projectionType);
    if (projectionType != cameraInteractor.get_projection_type())
    {
      cameraInteractor.set_projection_type(projectionType);
    }
    imguiOverlay.render();
    imguiOverlay.end_frame();

    window.swap_buffers();
  }

  window.destroy();
  return 0;
}

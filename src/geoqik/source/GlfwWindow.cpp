#include "GlfwWindow.hpp"
#include "Core/FmtIncludeHelper.hpp"
#include <Core/Assert.hpp>

namespace geoqik
{

namespace
{

void* load_glfw_proc(const char* procName)
{
  // GLAD v1 expects void* while GLFW returns an opaque function pointer.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<void*>(glfwGetProcAddress(procName));
}

} // namespace

GlfwWindow::~GlfwWindow()
{
  destroy();
}

bool GlfwWindow::create(const WindowSettings& settings)
{
  if (m_window != nullptr)
  {
    fmt::print("GLFW window is already initialized.\n");
    CORE_ASSERT(false);
    return false;
  }

  if (glfwInit() == 0)
  {
    const char* description = nullptr;
    glfwGetError(&description);
    fmt::print("Error: {}\n", description);
    return false;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  set_window_hints(settings);

  m_window = glfwCreateWindow(static_cast<int>(settings.width),
                              static_cast<int>(settings.height),
                              settings.title,
                              nullptr,
                              nullptr);

  if (m_window == nullptr)
  {
    const char* description = nullptr;
    glfwGetError(&description);
    fmt::print("Error: {}\n", description);
    glfwTerminate();
    return false;
  }

  make_context_current();
  if (gladLoadGLLoader(load_glfw_proc) == 0)
  {
    fmt::print("Failed to initialize OpenGL context\n");
    destroy();
    return false;
  }

  return true;
}

void GlfwWindow::destroy()
{
  if (m_window == nullptr)
  {
    return;
  }

  clear_callbacks(m_window);
  glfwDestroyWindow(m_window);
  m_window = nullptr;
  glfwTerminate();
}

void GlfwWindow::make_context_current() const
{
  CORE_ASSERT(m_window);
  glfwMakeContextCurrent(m_window);
}

void GlfwWindow::poll_events()
{
  glfwPollEvents();
}

void GlfwWindow::swap_buffers() const
{
  CORE_ASSERT(m_window);
  glfwSwapBuffers(m_window);
}

bool GlfwWindow::should_close() const
{
  CORE_ASSERT(m_window);
  return glfwWindowShouldClose(m_window) != 0;
}

bool GlfwWindow::is_escape_pressed() const
{
  CORE_ASSERT(m_window);
  return glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
}

std::pair<int, int> GlfwWindow::get_framebuffer_size() const
{
  CORE_ASSERT(m_window);

  int framebufferWidth{0};
  int framebufferHeight{0};
  glfwGetFramebufferSize(m_window, &framebufferWidth, &framebufferHeight);

  return {framebufferWidth, framebufferHeight};
}

InputState& GlfwWindow::get_input_state() const
{
  CORE_ASSERT(m_window);
  return *geoqik::get_input_state(m_window);
}

void GlfwWindow::set_key_callback(KeyCB cb)
{
  CORE_ASSERT(m_window);
  geoqik::set_key_callback(m_window, std::move(cb));
}

void GlfwWindow::set_char_callback(CharCB cb)
{
  CORE_ASSERT(m_window);
  geoqik::set_char_callback(m_window, std::move(cb));
}

void GlfwWindow::set_cursor_pos_callback(CursorPosCB cb)
{
  CORE_ASSERT(m_window);
  geoqik::set_cursor_pos_callback(m_window, std::move(cb));
}

void GlfwWindow::set_scroll_callback(ScrollCB cb)
{
  CORE_ASSERT(m_window);
  geoqik::set_scroll_callback(m_window, std::move(cb));
}

void GlfwWindow::set_mouse_button_callback(MouseBtnCB cb)
{
  CORE_ASSERT(m_window);
  geoqik::set_mouse_button_callback(m_window, std::move(cb));
}

void GlfwWindow::set_framebuffer_size_callback(FramebufferSizeCB cb)
{
  CORE_ASSERT(m_window);
  geoqik::set_framebuffer_size_callback(m_window, std::move(cb));
}

void GlfwWindow::set_window_hints(const WindowSettings& hints)
{
  glfwWindowHint(GLFW_RED_BITS, hints.red_bits);
  glfwWindowHint(GLFW_GREEN_BITS, hints.green_bits);
  glfwWindowHint(GLFW_BLUE_BITS, hints.blue_bits);
  glfwWindowHint(GLFW_ALPHA_BITS, hints.alpha_bits);
  glfwWindowHint(GLFW_DEPTH_BITS, hints.depth_bits);
  glfwWindowHint(GLFW_STENCIL_BITS, hints.stencil_bits);
  glfwWindowHint(GLFW_ACCUM_RED_BITS, hints.accum_red_bits);
  glfwWindowHint(GLFW_ACCUM_GREEN_BITS, hints.accum_green_bits);
  glfwWindowHint(GLFW_ACCUM_BLUE_BITS, hints.accum_blue_bits);
  glfwWindowHint(GLFW_ACCUM_ALPHA_BITS, hints.accum_alpha_bits);
  glfwWindowHint(GLFW_AUX_BUFFERS, hints.aux_buffers);
  glfwWindowHint(GLFW_SAMPLES, hints.samples);
  glfwWindowHint(GLFW_REFRESH_RATE, hints.refresh_rate);
  glfwWindowHint(GLFW_STEREO, window_hint_to_glfw_hint(hints.stereo));
  glfwWindowHint(GLFW_SRGB_CAPABLE, window_hint_to_glfw_hint(hints.srgb_capable));
  glfwWindowHint(GLFW_DOUBLEBUFFER, window_hint_to_glfw_hint(hints.double_buffer));
  glfwWindowHint(GLFW_RESIZABLE, window_hint_to_glfw_hint(hints.resizable));
  glfwWindowHint(GLFW_VISIBLE, window_hint_to_glfw_hint(hints.visible));
  glfwWindowHint(GLFW_DECORATED, window_hint_to_glfw_hint(hints.decorated));
  glfwWindowHint(GLFW_FOCUSED, window_hint_to_glfw_hint(hints.focused));
  glfwWindowHint(GLFW_AUTO_ICONIFY, window_hint_to_glfw_hint(hints.auto_iconify));
  glfwWindowHint(GLFW_FLOATING, window_hint_to_glfw_hint(hints.floating));
  glfwWindowHint(GLFW_MAXIMIZED, window_hint_to_glfw_hint(hints.maximized));
  glfwWindowHint(GLFW_CENTER_CURSOR, window_hint_to_glfw_hint(hints.center_cursor));
  glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, window_hint_to_glfw_hint(hints.transparent_framebuffer));
  glfwWindowHint(GLFW_FOCUS_ON_SHOW, window_hint_to_glfw_hint(hints.focus_on_show));
  glfwWindowHint(GLFW_SCALE_TO_MONITOR, window_hint_to_glfw_hint(hints.scale_to_monitor));
}

} // namespace geoqik

#include "InputState.hpp"

namespace geoqik
{

namespace detail
{

struct WindowCallbacks
{
  KeyCB key;
  CharCB ch;
  CharModsCB chMods;
  MouseBtnCB mouseBtn;
  CursorPosCB cursorPos;
  CursorEnterCB cursorEnter;
  ScrollCB scroll;
  DropCB drop;

  InputState inputState; // This is not a callback, but a place to store the current input state.

  FramebufferSizeCB framebufferSizeCB;
};

void set_window_callbacks(GLFWwindow* glfwWindow);

WindowCallbacks* get_window_callbacks(GLFWwindow* glfwWindow)
{
  CORE_ASSERT(glfwWindow);
  if (auto* wCallbacks = static_cast<WindowCallbacks*>(glfwGetWindowUserPointer(glfwWindow)); wCallbacks)
  {
    return wCallbacks;
  }

  auto* newWindowCallbacks = new WindowCallbacks{};
  glfwSetWindowUserPointer(glfwWindow, newWindowCallbacks);
  set_window_callbacks(glfwWindow);
  return newWindowCallbacks;
}

} // namespace detail

static void keyT(GLFWwindow* glfwWindow, int key, int sc, int act, int mods)
{
  if (auto* wCallbacks = detail::get_window_callbacks(glfwWindow); wCallbacks->key)
  {
    wCallbacks->key(static_cast<Key>(key), Scancode(sc), static_cast<Action>(act), static_cast<Mods>(mods));
  }
}
static void charT(GLFWwindow* glfwWindow, unsigned int cp)
{
  if (auto* wCallbacks = detail::get_window_callbacks(glfwWindow); wCallbacks->ch)
  {
    wCallbacks->ch(cp);
  }
}
static void charModsT(GLFWwindow* glfwWindow, unsigned int cp, int mods)
{
  if (auto* wCallbacks = detail::get_window_callbacks(glfwWindow); wCallbacks->chMods)
  {
    wCallbacks->chMods(cp, static_cast<Mods>(mods));
  }
}
static void mouseBtnT(GLFWwindow* glfwWindow, int btn, int act, int mods)
{
  if (auto* wCallbacks = detail::get_window_callbacks(glfwWindow); wCallbacks->mouseBtn)
  {
    wCallbacks->mouseBtn(btn, static_cast<Action>(act), static_cast<Mods>(mods));
  }
}
static void cursorPosT(GLFWwindow* glfwWindow, double x, double y)
{
  if (auto* wCallbacks = detail::get_window_callbacks(glfwWindow); wCallbacks->cursorPos)
  {
    wCallbacks->cursorPos(x, y);
  }
}
static void cursorEnterT(GLFWwindow* glfwWindow, int entered)
{
  if (auto* wCallbacks = detail::get_window_callbacks(glfwWindow); wCallbacks->cursorEnter)
  {
    wCallbacks->cursorEnter(entered != 0);
  }
}
static void scrollT(GLFWwindow* glfwWindow, double xo, double yo)
{
  if (auto* wCallbacks = detail::get_window_callbacks(glfwWindow); wCallbacks->scroll)
  {
    wCallbacks->scroll(xo, yo);
  }
}
static void dropT(GLFWwindow* glfwWindow, int cnt, const char** paths)
{
  if (auto* wCallbacks = detail::get_window_callbacks(glfwWindow); wCallbacks->drop)
  {
    wCallbacks->drop(cnt, paths);
  }
}

static void framebufferSizeT(GLFWwindow* glfwWindow, int width, int height)
{
  if (auto* wCallbacks = detail::get_window_callbacks(glfwWindow); wCallbacks->framebufferSizeCB)
  {
    wCallbacks->framebufferSizeCB(static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
  }
}

namespace detail
{

void set_window_callbacks(GLFWwindow* glfwWindow)
{
  CORE_ASSERT(glfwWindow);
  glfwSetKeyCallback(glfwWindow, keyT);
  glfwSetCharCallback(glfwWindow, charT);
  glfwSetCharModsCallback(glfwWindow, charModsT);
  glfwSetMouseButtonCallback(glfwWindow, mouseBtnT);
  glfwSetCursorPosCallback(glfwWindow, cursorPosT);
  glfwSetCursorEnterCallback(glfwWindow, cursorEnterT);
  glfwSetScrollCallback(glfwWindow, scrollT);
  glfwSetDropCallback(glfwWindow, dropT);

  glfwSetFramebufferSizeCallback(glfwWindow, framebufferSizeT);
}

} // namespace detail

InputState* get_input_state(GLFWwindow* glfwWindow)
{
  CORE_ASSERT(glfwWindow);
  return &detail::get_window_callbacks(glfwWindow)->inputState;
}

void set_key_callback(GLFWwindow* glfwWindow, KeyCB cb)
{
  CORE_ASSERT(glfwWindow);
  detail::get_window_callbacks(glfwWindow)->key = std::move(cb);
}
void set_char_callback(GLFWwindow* glfwWindow, CharCB cb)
{
  CORE_ASSERT(glfwWindow);
  detail::get_window_callbacks(glfwWindow)->ch = std::move(cb);
}
void set_char_mods_callback(GLFWwindow* glfwWindow, CharModsCB cb)
{
  CORE_ASSERT(glfwWindow);
  detail::get_window_callbacks(glfwWindow)->chMods = std::move(cb);
}
void set_mouse_button_callback(GLFWwindow* glfwWindow, MouseBtnCB cb)
{
  CORE_ASSERT(glfwWindow);
  detail::get_window_callbacks(glfwWindow)->mouseBtn = std::move(cb);
}
void set_cursor_pos_callback(GLFWwindow* glfwWindow, CursorPosCB cb)
{
  CORE_ASSERT(glfwWindow);
  detail::get_window_callbacks(glfwWindow)->cursorPos = std::move(cb);
}
void set_cursor_enter_callback(GLFWwindow* glfwWindow, CursorEnterCB cb)
{
  CORE_ASSERT(glfwWindow);
  detail::get_window_callbacks(glfwWindow)->cursorEnter = std::move(cb);
}
void set_scroll_callback(GLFWwindow* glfwWindow, ScrollCB cb)
{
  CORE_ASSERT(glfwWindow);
  detail::get_window_callbacks(glfwWindow)->scroll = std::move(cb);
}
void set_drop_callback(GLFWwindow* glfwWindow, DropCB cb)
{
  CORE_ASSERT(glfwWindow);
  detail::get_window_callbacks(glfwWindow)->drop = std::move(cb);
}

void set_framebuffer_size_callback(GLFWwindow* glfwWindow, FramebufferSizeCB cb)
{
  CORE_ASSERT(glfwWindow);
  detail::get_window_callbacks(glfwWindow)->framebufferSizeCB = std::move(cb);
}

void clear_callbacks(GLFWwindow* glfwWindow)
{
  CORE_ASSERT(glfwWindow);
  if (auto* wCallbacks = static_cast<detail::WindowCallbacks*>(glfwGetWindowUserPointer(glfwWindow)))
  {
    delete wCallbacks;
  }
  glfwSetWindowUserPointer(glfwWindow, nullptr);
}

} // namespace geoqik
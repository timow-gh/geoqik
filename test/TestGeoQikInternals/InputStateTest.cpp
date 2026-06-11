#include "InputState.hpp"
#include <gtest/gtest.h>

namespace
{

class GlfwWindowFixture : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_EQ(GLFW_TRUE, glfwInit());
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    m_window = glfwCreateWindow(64, 64, "GeoQik InputState test", nullptr, nullptr);
    ASSERT_NE(nullptr, m_window);
  }

  void TearDown() override
  {
    if (m_window != nullptr)
    {
      geoqik::clear_callbacks(m_window);
      glfwDestroyWindow(m_window);
      m_window = nullptr;
    }
    glfwTerminate();
  }

  GLFWwindow* m_window{nullptr};
};

} // namespace

TEST(InputStateTest, ValueWrappersExposeValidityAndOrdering)
{
  const geoqik::Scancode defaultScancode;
  const geoqik::Scancode scancode{7};

  EXPECT_FALSE(defaultScancode.is_valid());
  EXPECT_TRUE(scancode.is_valid());
  EXPECT_EQ(7, scancode.get_value());
  EXPECT_LT(defaultScancode, scancode);

  const geoqik::Codepoint defaultCodepoint;
  const geoqik::Codepoint codepoint{65};

  EXPECT_FALSE(defaultCodepoint.is_valid());
  EXPECT_TRUE(codepoint.is_valid());
  EXPECT_EQ(65U, codepoint.get_value());
  EXPECT_LT(defaultCodepoint, codepoint);
}

TEST_F(GlfwWindowFixture, TrampolinesForwardRegisteredCallbacks)
{
  geoqik::Key receivedKey = geoqik::Key::KEY_UNKNOWN;
  geoqik::Scancode receivedScancode;
  geoqik::Action receivedAction = geoqik::Action::UNDEFINED;
  geoqik::Mods receivedKeyMods = geoqik::Mods::MOD_NONE;
  geoqik::set_key_callback(
      m_window,
      [&](geoqik::Key key, geoqik::Scancode scancode, geoqik::Action action, geoqik::Mods mods)
      {
        receivedKey = key;
        receivedScancode = scancode;
        receivedAction = action;
        receivedKeyMods = mods;
      });
  GLFWkeyfun keyCallback = glfwSetKeyCallback(m_window, nullptr);
  ASSERT_NE(nullptr, keyCallback);
  keyCallback(m_window, GLFW_KEY_A, 12, GLFW_PRESS, GLFW_MOD_SHIFT);
  EXPECT_EQ(geoqik::Key::KEY_A, receivedKey);
  EXPECT_EQ(12, receivedScancode.get_value());
  EXPECT_EQ(geoqik::Action::PRESS, receivedAction);
  EXPECT_EQ(geoqik::Mods::MOD_SHIFT, receivedKeyMods);

  std::uint32_t receivedChar = 0;
  geoqik::set_char_callback(m_window, [&](std::uint32_t codepoint) { receivedChar = codepoint; });
  GLFWcharfun charCallback = glfwSetCharCallback(m_window, nullptr);
  ASSERT_NE(nullptr, charCallback);
  charCallback(m_window, 66);
  EXPECT_EQ(66U, receivedChar);

  std::uint32_t receivedCharModsCodepoint = 0;
  geoqik::Mods receivedCharMods = geoqik::Mods::MOD_NONE;
  geoqik::set_char_mods_callback(
      m_window,
      [&](std::uint32_t codepoint, geoqik::Mods mods)
      {
        receivedCharModsCodepoint = codepoint;
        receivedCharMods = mods;
      });
  GLFWcharmodsfun charModsCallback = glfwSetCharModsCallback(m_window, nullptr);
  ASSERT_NE(nullptr, charModsCallback);
  charModsCallback(m_window, 67, GLFW_MOD_ALT);
  EXPECT_EQ(67U, receivedCharModsCodepoint);
  EXPECT_EQ(geoqik::Mods::MOD_ALT, receivedCharMods);

  int receivedButton = -1;
  geoqik::Action receivedButtonAction = geoqik::Action::UNDEFINED;
  geoqik::Mods receivedButtonMods = geoqik::Mods::MOD_NONE;
  geoqik::set_mouse_button_callback(
      m_window,
      [&](int button, geoqik::Action action, geoqik::Mods mods)
      {
        receivedButton = button;
        receivedButtonAction = action;
        receivedButtonMods = mods;
      });
  GLFWmousebuttonfun mouseButtonCallback = glfwSetMouseButtonCallback(m_window, nullptr);
  ASSERT_NE(nullptr, mouseButtonCallback);
  mouseButtonCallback(m_window, GLFW_MOUSE_BUTTON_RIGHT, GLFW_RELEASE, GLFW_MOD_CONTROL);
  EXPECT_EQ(GLFW_MOUSE_BUTTON_RIGHT, receivedButton);
  EXPECT_EQ(geoqik::Action::RELEASE, receivedButtonAction);
  EXPECT_EQ(geoqik::Mods::MOD_CONTROL, receivedButtonMods);

  bool receivedCursorEnter = false;
  geoqik::set_cursor_enter_callback(m_window, [&](bool entered) { receivedCursorEnter = entered; });
  GLFWcursorenterfun cursorEnterCallback = glfwSetCursorEnterCallback(m_window, nullptr);
  ASSERT_NE(nullptr, cursorEnterCallback);
  cursorEnterCallback(m_window, GLFW_TRUE);
  EXPECT_TRUE(receivedCursorEnter);

  double receivedScrollX = 0.0;
  double receivedScrollY = 0.0;
  geoqik::set_scroll_callback(
      m_window,
      [&](double xoff, double yoff)
      {
        receivedScrollX = xoff;
        receivedScrollY = yoff;
      });
  GLFWscrollfun scrollCallback = glfwSetScrollCallback(m_window, nullptr);
  ASSERT_NE(nullptr, scrollCallback);
  scrollCallback(m_window, 1.5, -2.5);
  EXPECT_DOUBLE_EQ(1.5, receivedScrollX);
  EXPECT_DOUBLE_EQ(-2.5, receivedScrollY);

  int receivedDropCount = 0;
  const char** receivedDropPaths = nullptr;
  geoqik::set_drop_callback(
      m_window,
      [&](int count, const char** paths)
      {
        receivedDropCount = count;
        receivedDropPaths = paths;
      });
  GLFWdropfun dropCallback = glfwSetDropCallback(m_window, nullptr);
  ASSERT_NE(nullptr, dropCallback);
  const char* paths[] = {"first", "second"};
  dropCallback(m_window, 2, paths);
  EXPECT_EQ(2, receivedDropCount);
  EXPECT_EQ(paths, receivedDropPaths);

  std::uint32_t framebufferWidth = 0;
  std::uint32_t framebufferHeight = 0;
  geoqik::set_framebuffer_size_callback(
      m_window,
      [&](std::uint32_t width, std::uint32_t height)
      {
        framebufferWidth = width;
        framebufferHeight = height;
      });
  GLFWframebuffersizefun framebufferSizeCallback = glfwSetFramebufferSizeCallback(m_window, nullptr);
  ASSERT_NE(nullptr, framebufferSizeCallback);
  framebufferSizeCallback(m_window, 320, 240);
  EXPECT_EQ(320U, framebufferWidth);
  EXPECT_EQ(240U, framebufferHeight);
}

TEST_F(GlfwWindowFixture, InputStateStoragePersistsUntilCleared)
{
  geoqik::InputState* firstState = geoqik::get_input_state(m_window);
  ASSERT_NE(nullptr, firstState);
  firstState->cursorPosState = geoqik::CursorPosState{10.0, 20.0};

  geoqik::InputState* secondState = geoqik::get_input_state(m_window);
  EXPECT_EQ(firstState, secondState);
  EXPECT_DOUBLE_EQ(10.0, secondState->cursorPosState.xpos);
  EXPECT_DOUBLE_EQ(20.0, secondState->cursorPosState.ypos);

  geoqik::clear_callbacks(m_window);
  geoqik::InputState* resetState = geoqik::get_input_state(m_window);
  ASSERT_NE(nullptr, resetState);
  EXPECT_DOUBLE_EQ(0.0, resetState->cursorPosState.xpos);
  EXPECT_DOUBLE_EQ(0.0, resetState->cursorPosState.ypos);
}

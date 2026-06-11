#include "InputCaptureState.hpp"
#include <GLFW/glfw3.h>
#include <gtest/gtest.h>

TEST(InputCaptureStateTest, MousePressCapturedByGuiBlocksDragAndRelease)
{
  geoqik::InputCaptureState state;

  EXPECT_FALSE(state.should_forward_mouse_button(GLFW_MOUSE_BUTTON_RIGHT, geoqik::Action::PRESS, true));
  EXPECT_FALSE(state.should_forward_cursor_position(false));
  EXPECT_FALSE(state.should_forward_mouse_button(GLFW_MOUSE_BUTTON_RIGHT, geoqik::Action::RELEASE, false));
}

TEST(InputCaptureStateTest, MousePressOutsideGuiOwnsDragUntilRelease)
{
  geoqik::InputCaptureState state;

  EXPECT_TRUE(state.should_forward_mouse_button(GLFW_MOUSE_BUTTON_RIGHT, geoqik::Action::PRESS, false));
  EXPECT_TRUE(state.should_forward_cursor_position(true));
  EXPECT_TRUE(state.should_forward_mouse_button(GLFW_MOUSE_BUTTON_RIGHT, geoqik::Action::RELEASE, true));
  EXPECT_FALSE(state.should_forward_cursor_position(true));
}

TEST(InputCaptureStateTest, HoveredGuiBlocksMouseAndKeyboardCaptureBlocksKeys)
{
  geoqik::InputCaptureState state;

  EXPECT_FALSE(state.should_forward_cursor_position(true));
  EXPECT_FALSE(state.should_forward_scroll(true));
  EXPECT_TRUE(state.should_forward_scroll(false));
  EXPECT_FALSE(geoqik::InputCaptureState::should_forward_key(true));
  EXPECT_TRUE(geoqik::InputCaptureState::should_forward_key(false));
}

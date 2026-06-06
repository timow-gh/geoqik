#include "CameraInteractor.hpp"
#include <gtest/gtest.h>

namespace
{

constexpr double tolerance = 1.0e-6;

geoqik::CameraInteractor make_interactor(geoqik::InputState& inputState)
{
  geoqik::CameraSettings settings;
  settings.m_defaultPosition = linal::double3{0.0, -10.0, 10.0};
  settings.m_defaultTarget = linal::double3{0.0, 0.0, 0.0};
  settings.m_defaultUp = linal::double3{0.0, 0.0, 1.0};
  geoqik::CameraInteractor interactor(inputState, settings);
  interactor.set_viewport(0, 0, 800, 600);
  return interactor;
}

} // namespace

TEST(CameraInteractorTest, AppliesProjectionAndClippingSettings)
{
  geoqik::InputState inputState;
  geoqik::CameraInteractor interactor = make_interactor(inputState);

  interactor.set_near_plane(0.5);
  interactor.set_far_plane(500.0);
  EXPECT_DOUBLE_EQ(interactor.get_near_plane(), 0.5);
  EXPECT_DOUBLE_EQ(interactor.get_far_plane(), 500.0);

  interactor.set_projection_type(geoqik::CameraProjectionType::ORTHOGRAPHIC);
  interactor.set_near_plane(0.25);
  interactor.set_far_plane(250.0);
  EXPECT_EQ(interactor.get_projection_type(), geoqik::CameraProjectionType::ORTHOGRAPHIC);
  EXPECT_DOUBLE_EQ(interactor.get_near_plane(), 0.25);
  EXPECT_DOUBLE_EQ(interactor.get_far_plane(), 250.0);

  const geoqik::PickRay ray = interactor.get_pick_ray(400.0, 300.0);
  EXPECT_NEAR(linal::length(ray.direction), 1.0, tolerance);
  (void)interactor.get_projection_matrix();
  (void)interactor.get_current_MVP();
  (void)interactor.get_normal_matrix();
}

TEST(CameraInteractorTest, AppliesAutoFitResultToOrthographicCamera)
{
  geoqik::InputState inputState;
  geoqik::CameraInteractor interactor = make_interactor(inputState);
  interactor.set_projection_type(geoqik::CameraProjectionType::ORTHOGRAPHIC);

  geoqik::CameraAutoFitResult result;
  result.position = linal::double3{1.0, 2.0, 30.0};
  result.target = linal::double3{1.0, 2.0, 0.0};
  result.vertical = linal::double3{0.0, 1.0, 0.0};
  result.orthographicWidth = 25.0;
  result.orthographicHeight = 15.0;
  result.farPlane = 750.0;

  interactor.apply_auto_fit_result(result);

  EXPECT_EQ(interactor.get_position(), result.position);
  EXPECT_EQ(interactor.get_target(), result.target);
  EXPECT_EQ(interactor.get_vertical(), result.vertical);
  EXPECT_DOUBLE_EQ(interactor.get_orthographic_params().width, 25.0);
  EXPECT_DOUBLE_EQ(interactor.get_orthographic_params().height, 15.0);
  EXPECT_DOUBLE_EQ(interactor.get_far_plane(), 750.0);
}

TEST(CameraInteractorTest, ScrollZoomsPerspectiveCameraAndSetsBlocking)
{
  geoqik::InputState inputState;
  geoqik::CameraInteractor interactor = make_interactor(inputState);
  inputState.cursorPosState = geoqik::CursorPosState{400.0, 300.0};

  const linal::double3 initialPosition = interactor.get_position();
  interactor.on_scroll(0.0, 1.0);

  EXPECT_TRUE(interactor.get_was_blocking());
  EXPECT_LT(linal::length(interactor.get_position() - interactor.get_target()), linal::length(initialPosition - interactor.get_target()));

  interactor.reset_was_blocking();
  EXPECT_FALSE(interactor.get_was_blocking());
}

TEST(CameraInteractorTest, ScrollZoomsOrthographicCamera)
{
  geoqik::InputState inputState;
  geoqik::CameraInteractor interactor = make_interactor(inputState);
  interactor.set_projection_type(geoqik::CameraProjectionType::ORTHOGRAPHIC);
  inputState.cursorPosState = geoqik::CursorPosState{400.0, 300.0};

  const double initialWidth = interactor.get_orthographic_params().width;
  interactor.on_scroll(0.0, 1.0);

  EXPECT_TRUE(interactor.get_was_blocking());
  EXPECT_LT(interactor.get_orthographic_params().width, initialWidth);
}

TEST(CameraInteractorTest, MiddleMouseDragPansCamera)
{
  geoqik::InputState inputState;
  geoqik::CameraInteractor interactor = make_interactor(inputState);

  interactor.on_cursor_position(400.0, 300.0);
  const linal::double3 initialPosition = interactor.get_position();
  const linal::double3 initialTarget = interactor.get_target();

  interactor.on_mouse_button(GLFW_MOUSE_BUTTON_MIDDLE, geoqik::Action::PRESS, geoqik::Mods::MOD_NONE);
  interactor.on_cursor_position(460.0, 300.0);
  interactor.on_mouse_button(GLFW_MOUSE_BUTTON_MIDDLE, geoqik::Action::RELEASE, geoqik::Mods::MOD_NONE);

  EXPECT_TRUE(interactor.get_was_blocking());
  EXPECT_NE(interactor.get_position(), initialPosition);
  EXPECT_NE(interactor.get_target(), initialTarget);
}

TEST(CameraInteractorTest, RightMouseDragOrbitsCamera)
{
  geoqik::InputState inputState;
  geoqik::CameraInteractor interactor = make_interactor(inputState);

  interactor.on_cursor_position(400.0, 300.0);
  const linal::double3 initialPosition = interactor.get_position();

  interactor.on_mouse_button(GLFW_MOUSE_BUTTON_RIGHT, geoqik::Action::PRESS, geoqik::Mods::MOD_NONE);
  interactor.on_cursor_position(410.0, 300.0);
  interactor.on_cursor_position(440.0, 300.0);
  interactor.on_mouse_button(GLFW_MOUSE_BUTTON_RIGHT, geoqik::Action::RELEASE, geoqik::Mods::MOD_NONE);

  EXPECT_TRUE(interactor.get_was_blocking());
  EXPECT_NE(interactor.get_position(), initialPosition);
}

TEST(CameraInteractorTest, UnsupportedMouseButtonDoesNotBlock)
{
  geoqik::InputState inputState;
  geoqik::CameraInteractor interactor = make_interactor(inputState);

  interactor.on_mouse_button(GLFW_MOUSE_BUTTON_LEFT, geoqik::Action::PRESS, geoqik::Mods::MOD_NONE);

  EXPECT_FALSE(interactor.get_was_blocking());
}

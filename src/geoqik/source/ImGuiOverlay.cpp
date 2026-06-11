#include "ImGuiOverlay.hpp"
#include "Core/Warnings.hpp"
#include <Core/Assert.hpp>

DISABLE_ALL_WARNINGS
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
ENABLE_ALL_WARNINGS

namespace geoqik
{

ImGuiOverlay::ImGuiOverlay(GLFWwindow* window)
    : m_window(window)
{
  CORE_ASSERT(m_window != nullptr);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(m_window, false);
  ImGui_ImplOpenGL3_Init("#version 330");
}

ImGuiOverlay::~ImGuiOverlay()
{
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void ImGuiOverlay::new_frame()
{
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void ImGuiOverlay::draw_controls(bool& autoZoomEnabled, CameraProjectionType& projectionType)
{
  ImGui::Begin("Camera");
  ImGui::Checkbox("Auto Zoom", &autoZoomEnabled);

  const char* projectionItems[] = {"Perspective", "Orthographic"};
  int currentItem = static_cast<int>(projectionType);
  if (ImGui::Combo("Projection", &currentItem, projectionItems, 2))
    projectionType = static_cast<CameraProjectionType>(currentItem);

  ImGui::End();
}

void ImGuiOverlay::render()
{
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool ImGuiOverlay::wants_keyboard() const
{
  return ImGui::GetIO().WantCaptureKeyboard;
}

bool ImGuiOverlay::handle_cursor_position(double xpos, double ypos)
{
  ImGui_ImplGlfw_CursorPosCallback(m_window, xpos, ypos);
  return m_inputCaptureState.should_forward_cursor_position(ImGui::GetIO().WantCaptureMouse);
}

bool ImGuiOverlay::handle_mouse_button(int button, Action action, Mods mods)
{
  ImGui_ImplGlfw_MouseButtonCallback(m_window, button, static_cast<int>(action), static_cast<int>(mods));
  return m_inputCaptureState.should_forward_mouse_button(button, action, ImGui::GetIO().WantCaptureMouse);
}

bool ImGuiOverlay::handle_scroll(double xoffset, double yoffset)
{
  ImGui_ImplGlfw_ScrollCallback(m_window, xoffset, yoffset);
  return m_inputCaptureState.should_forward_scroll(ImGui::GetIO().WantCaptureMouse);
}

bool ImGuiOverlay::handle_key(Key key, Scancode scancode, Action action, Mods mods)
{
  ImGui_ImplGlfw_KeyCallback(m_window,
                             static_cast<int>(key),
                             scancode.get_value(),
                             static_cast<int>(action),
                             static_cast<int>(mods));
  return InputCaptureState::should_forward_key(ImGui::GetIO().WantCaptureKeyboard);
}

void ImGuiOverlay::handle_char(std::uint32_t codepoint)
{
  ImGui_ImplGlfw_CharCallback(m_window, codepoint);
}

} // namespace geoqik

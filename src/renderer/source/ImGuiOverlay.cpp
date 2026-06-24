#include <Renderer/Assert.hpp>
#include <Renderer/ImGuiOverlay.hpp>
#include <Renderer/Warnings.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

RENDERER_DISABLE_ALL_WARNINGS
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
RENDERER_ENABLE_ALL_WARNINGS

namespace renderer
{

namespace
{

constexpr float controlPanelMargin = 8.0F;
constexpr float controlPanelMinWidth = 280.0F;
constexpr float controlPanelMaxWidth = 480.0F;
constexpr float controlPanelTinyViewportMinWidth = 160.0F;
constexpr float resizeGripWidth = 8.0F;

float panel_max_width(float viewportWidth)
{
  const float visibleWidth = std::max(controlPanelTinyViewportMinWidth,
                                      viewportWidth - 2.0F * controlPanelMargin);
  return std::min(controlPanelMaxWidth, visibleWidth);
}

float panel_min_width(float viewportWidth)
{
  return std::min(controlPanelMinWidth, panel_max_width(viewportWidth));
}

float clamp_panel_width(float width, float viewportWidth)
{
  return std::clamp(width, panel_min_width(viewportWidth), panel_max_width(viewportWidth));
}

float equal_button_width(int buttonCount)
{
  const float availableWidth = ImGui::GetContentRegionAvail().x;
  const float spacing = ImGui::GetStyle().ItemSpacing.x * static_cast<float>(buttonCount - 1);
  return std::max(1.0F, (availableWidth - spacing) / static_cast<float>(buttonCount));
}

bool full_width_button(const char* label)
{
  return ImGui::Button(label, ImVec2{-1.0F, 0.0F});
}

bool equal_width_button(const char* label, float width)
{
  return ImGui::Button(label, ImVec2{width, 0.0F});
}

void render_speed_controls(ReplayGuiState& replayState)
{
  constexpr std::array<double, 4> speedOptions{1.0, 2.0, 4.0, 8.0};
  constexpr std::array<const char*, 4> speedLabels{"1x", "2x", "4x", "8x"};

  ImGui::TextUnformatted("Speed");
  const float buttonWidth = equal_button_width(static_cast<int>(speedOptions.size()));
  for (std::size_t i = 0; i < speedOptions.size(); ++i)
  {
    if (i != 0)
    {
      ImGui::SameLine();
    }

    const bool isCurrent = std::abs(replayState.speedMultiplier - speedOptions[i]) < 0.01;
    if (isCurrent)
    {
      ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    }
    if (equal_width_button(speedLabels[i], buttonWidth))
    {
      replayState.requestedSpeedMultiplier = speedOptions[i];
    }
    if (isCurrent)
    {
      ImGui::PopStyleColor();
    }
  }
}

void render_transport_controls(ReplayGuiState& replayState)
{
  if (full_width_button("End replay"))
  {
    replayState.command = ReplayGuiState::Command::Finish;
  }

  ImGui::Separator();

  const bool canStepBack = replayState.currentEntry > 0;
  const bool canStepForward = replayState.currentEntry < replayState.totalEntries;
  const float twoButtonWidth = equal_button_width(2);

  if (!canStepBack)
  {
    ImGui::BeginDisabled();
  }
  if (equal_width_button("Step Back", twoButtonWidth))
  {
    replayState.command = ReplayGuiState::Command::StepBackward;
  }
  ImGui::SameLine();
  if (equal_width_button(replayState.isBackward && !replayState.isPaused ? "Reverse *" : "Reverse",
                         twoButtonWidth))
  {
    replayState.command = ReplayGuiState::Command::PlayReverse;
  }
  if (!canStepBack)
  {
    ImGui::EndDisabled();
  }

  const char* playPauseLabel = replayState.isPaused ? "Play" : "Pause";
  if (full_width_button(playPauseLabel))
  {
    replayState.command =
        replayState.isPaused ? ReplayGuiState::Command::Play : ReplayGuiState::Command::Pause;
  }

  if (!canStepForward)
  {
    ImGui::BeginDisabled();
  }
  if (equal_width_button(!replayState.isBackward && !replayState.isPaused ? "Forward *" : "Forward",
                         twoButtonWidth))
  {
    replayState.command = ReplayGuiState::Command::Play;
  }
  ImGui::SameLine();
  if (equal_width_button("Step Forward", twoButtonWidth))
  {
    replayState.command = ReplayGuiState::Command::StepForward;
  }
  if (!canStepForward)
  {
    ImGui::EndDisabled();
  }
}

void render_panel_resize_grip(float& panelWidth, float viewportWidth, float height)
{
  ImGui::InvisibleButton("##ControlPanelResizeGrip", ImVec2{resizeGripWidth, height});
  const bool hovered = ImGui::IsItemHovered();
  const bool active = ImGui::IsItemActive();
  if (hovered || active)
  {
    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
  }
  if (active)
  {
    panelWidth = clamp_panel_width(panelWidth + ImGui::GetIO().MouseDelta.x, viewportWidth);
  }

  const ImU32 gripColor =
      ImGui::GetColorU32(active ? ImGuiCol_ResizeGripActive
                                : (hovered ? ImGuiCol_ResizeGripHovered : ImGuiCol_Border));
  const ImVec2 gripMin = ImGui::GetItemRectMin();
  const ImVec2 gripMax = ImGui::GetItemRectMax();
  const float gripCenterX = (gripMin.x + gripMax.x) * 0.5F;
  ImDrawList* drawList = ImGui::GetWindowDrawList();
  drawList->AddLine(ImVec2{gripCenterX, gripMin.y + 2.0F},
                    ImVec2{gripCenterX, gripMax.y - 2.0F},
                    gripColor,
                    1.0F);
}

} // namespace

ImGuiOverlay::ImGuiOverlay(GLFWwindow* window)
    : m_window(window)
{
  RENDERER_ASSERT(m_window != nullptr);

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

void ImGuiOverlay::new_frame() // NOLINT(readability-convert-member-functions-to-static)
{
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void ImGuiOverlay::add_control(std::function<void()> controlFunc)
{
  m_controls.emplace_back(std::move(controlFunc));
}

void ImGuiOverlay::add_camera_controls(bool& autoZoomEnabled, CameraProjectionType& projectionType, bool& homeRequested)
{
  m_controls.emplace_back(
      [&autoZoomEnabled, &projectionType, &homeRequested]()
      {
        if (!ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
        {
          return;
        }

        ImGui::Checkbox("Auto Zoom", &autoZoomEnabled);

        constexpr std::array<const char*, 2> projectionItems = {
            "Perspective",
            "Orthographic"};
        int currentItem = static_cast<int>(projectionType);
        ImGui::TextUnformatted("Projection");
        ImGui::SetNextItemWidth(-1.0F);
        if (ImGui::Combo("##Projection",
                         &currentItem,
                         projectionItems.data(),
                         static_cast<int>(projectionItems.size())))
        {
          projectionType = static_cast<CameraProjectionType>(currentItem);
        }

        if (full_width_button("Home"))
        {
          homeRequested = true;
        }
      });
}

void ImGuiOverlay::add_replay_controls(ReplayGuiState& replayState)
{
  if (!replayState.isActive)
  {
    return;
  }

  m_controls.emplace_back(
      [&replayState]()
      {
        if (ImGui::CollapsingHeader("Replay", ImGuiTreeNodeFlags_DefaultOpen))
        {
          const float progress =
              replayState.totalEntries > 0
                  ? static_cast<float>(std::min(replayState.currentEntry, replayState.totalEntries)) /
                        static_cast<float>(replayState.totalEntries)
                  : 0.0F;
          ImGui::Text("Entry %zu / %zu", replayState.currentEntry, replayState.totalEntries);
          ImGui::ProgressBar(progress, ImVec2{-1.0F, 0.0F});

          render_speed_controls(replayState);
          render_transport_controls(replayState);

          const std::size_t remainingEntryCount =
              replayState.totalEntries > replayState.currentEntry
                  ? replayState.totalEntries - replayState.currentEntry
                  : 0U;
          const int remainingEntries = static_cast<int>(remainingEntryCount);
          const int sliderMax = std::max(1, remainingEntries);
          int stepSize = static_cast<int>(replayState.entriesPerStep);
          stepSize = std::max(1, std::min(stepSize, sliderMax));
          ImGui::TextUnformatted("Step size");
          ImGui::SetNextItemWidth(-1.0F);
          if (ImGui::SliderInt("##StepSize", &stepSize, 1, sliderMax))
          {
            replayState.requestedEntriesPerStep = static_cast<std::size_t>(stepSize);
          }
        }

        if (ImGui::CollapsingHeader("Shortcuts"))
        {
          ImGui::TextWrapped("Play: %s, pause: %s",
                             replayState.resumeKeysLabel.c_str(),
                             replayState.pauseKeysLabel.c_str());
          ImGui::TextWrapped("Step: forward %s, back %s",
                             replayState.stepForwardKeysLabel.c_str(),
                             replayState.stepBackwardKeysLabel.c_str());
          ImGui::TextWrapped("Step size: + %s, - %s",
                             replayState.increaseStepKeysLabel.c_str(),
                             replayState.decreaseStepKeysLabel.c_str());
        }
      });
}

void ImGuiOverlay::render()
{
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  if (viewport != nullptr)
  {
    m_controlPanelWidth = clamp_panel_width(m_controlPanelWidth, viewport->WorkSize.x);

    ImGui::SetNextWindowPos(ImVec2{viewport->WorkPos.x + controlPanelMargin,
                                   viewport->WorkPos.y + controlPanelMargin},
                            ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2{m_controlPanelWidth,
                                    std::max(0.0F, viewport->WorkSize.y - 2.0F * controlPanelMargin)},
                             ImGuiCond_Always);

    constexpr ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings;

    if (ImGui::Begin("GeoQik Controls", nullptr, windowFlags))
    {
      const ImVec2 availableContentSize = ImGui::GetContentRegionAvail();
      const float contentWidth = std::max(1.0F, availableContentSize.x - resizeGripWidth);
      if (ImGui::BeginChild("##ControlPanelContent", ImVec2{contentWidth, 0.0F}, false))
      {
        for (const auto& control : m_controls)
        {
          control();
        }
      }
      ImGui::EndChild();

      ImGui::SameLine(0.0F, 0.0F);
      render_panel_resize_grip(m_controlPanelWidth, viewport->WorkSize.x, availableContentSize.y);
    }
    ImGui::End();
  }
  m_controls.clear();

  ImGui::Render();
  ImDrawData* drawData = ImGui::GetDrawData();
  if (drawData != nullptr && drawData->Valid)
  {
    ImGui_ImplOpenGL3_RenderDrawData(drawData);
  }
}

void ImGuiOverlay::end_frame() // NOLINT(readability-convert-member-functions-to-static)
{
  ImGui::EndFrame();
}

float ImGuiOverlay::get_reserved_control_panel_width() const
{
  return m_controlPanelWidth + 2.0F * controlPanelMargin;
}

bool ImGuiOverlay::wants_mouse() const // NOLINT(readability-convert-member-functions-to-static)
{
  return ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiOverlay::wants_keyboard() const // NOLINT(readability-convert-member-functions-to-static)
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
  ImGui_ImplGlfw_MouseButtonCallback(m_window,
                                     button,
                                     static_cast<int>(action),
                                     static_cast<int>(mods));
  return m_inputCaptureState
      .should_forward_mouse_button(button, action, ImGui::GetIO().WantCaptureMouse);
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

} // namespace renderer

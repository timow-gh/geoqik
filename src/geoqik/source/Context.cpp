#include "Context.hpp"

#include "Core/FmtIncludeHelper.hpp"
#include "GeoQikMessages.hpp"
#include "GeoQikOverlay.hpp"

#include <Core/Assert.hpp>

#include <plinth/CameraAutoFit.hpp>
#include <plinth/CameraProjectionType.hpp>
#include <plinth/FrameState.hpp>
#include <plinth/IOverlay.hpp>
#include <plinth/LogicalViewportRect.hpp>
#include <plinth/Renderer.hpp>
#include <plinth/Warnings.hpp>
#include <plinth/WindowSettings.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>

RENDERER_DISABLE_ALL_WARNINGS
#include <imgui.h>
RENDERER_ENABLE_ALL_WARNINGS

namespace geoqik {

using renderer::CameraAutoFitSettings;
using renderer::Viewport;

struct ReplayGuiState {
    bool isActive{false};
    bool isPaused{false};
    bool isBackward{false};
    std::size_t currentEntry{0};
    std::size_t totalEntries{0};
    double speedMultiplier{1.0};
    std::size_t entriesPerStep{1};
    std::string pauseKeysLabel;
    std::string resumeKeysLabel;
    std::string stepForwardKeysLabel;
    std::string stepBackwardKeysLabel;
    std::string increaseStepKeysLabel;
    std::string decreaseStepKeysLabel;

    enum class Command : std::uint8_t {
        None,
        Play,
        PlayReverse,
        Pause,
        Finish,
        StepForward,
        StepBackward,
    };
    // One-shot requests filled in by the UI and drained (reset) after rendering, mirroring the
    // std::optional requests in CameraGuiState. Command::None is the "no request" sentinel.
    Command command{Command::None};
    std::optional<double> requestedSpeedMultiplier;
    std::optional<std::size_t> requestedEntriesPerStep;
};

struct CameraGuiState {
    renderer::CameraInteractor::NavigationStyle navigationStyle{renderer::CameraInteractor::NavigationStyle::ORBIT};
    std::optional<renderer::PresetView> activePreset; // nullopt == free navigation

    // Built-in camera controls (driven straight through the OverlayFrameContext each frame).
    bool autoZoom{false};
    renderer::CameraProjectionType projectionType{renderer::CameraProjectionType::PERSPECTIVE};

    // Read-only interaction-lock display (decoded from CameraViewMode bit flags).
    bool fixRotate{false};
    bool fixPan{false};
    bool fixZoom{false};

    // Requests filled in by the UI and applied after rendering.
    std::optional<renderer::CameraInteractor::NavigationStyle> requestedNavigationStyle;
    std::optional<renderer::PresetView> requestedPreset;
    std::optional<renderer::CameraProjectionType> requestedProjection;
    bool requestHome{false};
};

namespace {

constexpr std::size_t lineCoordinateCount = 6;
constexpr std::size_t frameInfoPrintInterval = 10;

[[nodiscard]] std::vector<float> expand_vertex_colors(std::span<const float> colors,
                                                      std::size_t vertexCount,
                                                      const Color& fallback,
                                                      bool duplicateLineColors = false) {
    std::vector<float> expanded;
    expanded.reserve(vertexCount * ColorChannelCount);
    if (colors.empty() || colors.size() == ColorChannelCount) {
        const auto source = colors.empty() ? std::span<const float>(fallback) : colors;
        for (std::size_t i = 0; i < vertexCount; ++i) {
            expanded.insert(expanded.end(), source.begin(), source.end());
        }
        return expanded;
    }
    if (colors.size() == vertexCount * ColorChannelCount) {
        expanded.assign(colors.begin(), colors.end());
        return expanded;
    }
    if (duplicateLineColors && vertexCount % 2 == 0 && colors.size() == vertexCount / 2 * ColorChannelCount) {
        for (std::size_t i = 0; i < vertexCount / 2; ++i) {
            const auto begin = colors.begin() + static_cast<std::ptrdiff_t>(i * ColorChannelCount);
            expanded.insert(expanded.end(), begin, begin + static_cast<std::ptrdiff_t>(ColorChannelCount));
            expanded.insert(expanded.end(), begin, begin + static_cast<std::ptrdiff_t>(ColorChannelCount));
        }
    }
    return expanded;
}

ConcurrentQueue<GeoQikMessage>& message_queue_storage() {
    static ConcurrentQueue<GeoQikMessage> messageQueue; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    return messageQueue;
}

std::atomic<bool>& replay_cancel_requested_storage() {
    static std::atomic<bool> replayCancelRequested{false}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    return replayCancelRequested;
}

bool is_existing_regular_file(const char* path) {
    std::error_code ec;
    return std::filesystem::is_regular_file(path, ec);
}

std::string key_label(Key key) {
    const int value = static_cast<int>(key);
    if (value >= static_cast<int>(Key::KEY_A) && value <= static_cast<int>(Key::KEY_Z)) {
        return {1, static_cast<char>('A' + value - static_cast<int>(Key::KEY_A))};
    }

    if (value >= static_cast<int>(Key::KEY_0) && value <= static_cast<int>(Key::KEY_9)) {
        return {1, static_cast<char>('0' + value - static_cast<int>(Key::KEY_0))};
    }

    switch (key) {
    case Key::KEY_SPACE:     return "Space";
    case Key::KEY_LEFT:      return "Left";
    case Key::KEY_RIGHT:     return "Right";
    case Key::KEY_UP:        return "Up";
    case Key::KEY_DOWN:      return "Down";
    case Key::KEY_PAGE_UP:   return "Page Up";
    case Key::KEY_PAGE_DOWN: return "Page Down";
    case Key::KEY_HOME:      return "Home";
    case Key::KEY_END:       return "End";
    default:                 return "Key " + std::to_string(value);
    }
}

std::string key_labels(const std::vector<Key>& keys) {
    std::string result;
    for (const Key key: keys) {
        if (!result.empty()) {
            result += "/";
        }
        result += key_label(key);
    }
    return result;
}

float equal_button_width(int buttonCount) {
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const float spacing = ImGui::GetStyle().ItemSpacing.x * static_cast<float>(buttonCount - 1);
    return std::max(1.0F, (availableWidth - spacing) / static_cast<float>(buttonCount));
}

bool full_width_button(const char* label) {
    return ImGui::Button(label, ImVec2{-1.0F, 0.0F});
}

bool equal_width_button(const char* label, float width) {
    return ImGui::Button(label, ImVec2{width, 0.0F});
}

// Draws a button that appears "active" (highlighted) when isActive is true, matching the
// highlight idiom used by the replay speed controls. Returns true when clicked.
bool highlighted_button(const char* label, float width, bool isActive) {
    if (isActive) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    }
    const bool clicked = equal_width_button(label, width);
    if (isActive) {
        ImGui::PopStyleColor();
    }
    return clicked;
}

void render_camera_controls(CameraGuiState& cameraState) {
    if (!ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    // Auto Zoom and Projection mirror plinth's built-in camera controls, so everything camera
    // related lives under this single header.
    ImGui::Checkbox("Auto Zoom", &cameraState.autoZoom);

    constexpr std::array<const char*, 2> projectionItems{"Perspective", "Orthographic"};
    int projectionItem = static_cast<int>(cameraState.projectionType);
    ImGui::TextUnformatted("Projection");
    ImGui::SetNextItemWidth(-1.0F);
    if (ImGui::Combo("##Projection",
                     &projectionItem,
                     projectionItems.data(),
                     static_cast<int>(projectionItems.size()))) {
        cameraState.requestedProjection = static_cast<renderer::CameraProjectionType>(projectionItem);
    }

    ImGui::Separator();

    // Navigation style. The current style is highlighted only when no preset view is active, so
    // the navigation and preset selections read as mutually exclusive.
    ImGui::TextUnformatted("Navigation");
    const bool isFree = !cameraState.activePreset.has_value();
    const float navButtonWidth = equal_button_width(2);
    const bool orbitActive =
        isFree && cameraState.navigationStyle == renderer::CameraInteractor::NavigationStyle::ORBIT;
    if (highlighted_button("Orbit", navButtonWidth, orbitActive)) {
        cameraState.requestedNavigationStyle = renderer::CameraInteractor::NavigationStyle::ORBIT;
    }
    ImGui::SameLine();
    const bool flyActive = isFree && cameraState.navigationStyle == renderer::CameraInteractor::NavigationStyle::FLY;
    if (highlighted_button("Fly", navButtonWidth, flyActive)) {
        cameraState.requestedNavigationStyle = renderer::CameraInteractor::NavigationStyle::FLY;
    }

    ImGui::Separator();

    // Preset views. Two rows of buttons; the active preset (if any) is highlighted.
    ImGui::TextUnformatted("Preset view");
    struct PresetButton {
        const char* label;
        renderer::PresetView view;
    };
    constexpr std::array<PresetButton, 7> presets{{
        {"Front", renderer::PresetView::FRONT},
        {"Back", renderer::PresetView::BACK},
        {"Left", renderer::PresetView::LEFT},
        {"Right", renderer::PresetView::RIGHT},
        {"Top", renderer::PresetView::TOP},
        {"Bottom", renderer::PresetView::BOTTOM},
        {"Iso", renderer::PresetView::ISO},
    }};
    constexpr int presetsPerRow = 4;
    const float presetButtonWidth = equal_button_width(presetsPerRow);
    for (std::size_t i = 0; i < presets.size(); ++i) {
        if (i % presetsPerRow != 0) {
            ImGui::SameLine();
        }
        const bool active = cameraState.activePreset.has_value() && *cameraState.activePreset == presets[i].view;
        if (highlighted_button(presets[i].label, presetButtonWidth, active)) {
            cameraState.requestedPreset = presets[i].view;
        }
    }

    // Home refits all geometry into view along the current viewing direction, so the active
    // preset (and its interaction locks) are preserved.
    if (full_width_button("Home")) {
        cameraState.requestHome = true;
    }

    ImGui::Separator();

    // Interaction locks (read-only): shows which movements are currently fixed for mouse/keyboard.
    ImGui::TextUnformatted("Interaction locks");
    const auto lockLabel = [](bool locked) { return locked ? "Locked" : "Unlocked"; };
    ImGui::TextUnformatted(fmt::format("Rotate: {}", lockLabel(cameraState.fixRotate)).c_str());
    ImGui::TextUnformatted(fmt::format("Pan:    {}", lockLabel(cameraState.fixPan)).c_str());
    ImGui::TextUnformatted(fmt::format("Zoom:   {}", lockLabel(cameraState.fixZoom)).c_str());
}

void render_replay_speed_controls(ReplayGuiState& replayState) {
    constexpr std::array<double, 4> speedOptions{1.0, 2.0, 4.0, 8.0};
    constexpr std::array<const char*, 4> speedLabels{"1x", "2x", "4x", "8x"};

    ImGui::TextUnformatted("Speed");
    const float buttonWidth = equal_button_width(static_cast<int>(speedOptions.size()));
    for (std::size_t i = 0; i < speedOptions.size(); ++i) {
        if (i != 0) {
            ImGui::SameLine();
        }

        const bool isCurrent = std::abs(replayState.speedMultiplier - speedOptions[i]) < 0.01;
        if (isCurrent) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        }
        if (equal_width_button(speedLabels[i], buttonWidth)) {
            replayState.requestedSpeedMultiplier = speedOptions[i];
        }
        if (isCurrent) {
            ImGui::PopStyleColor();
        }
    }
}

void render_replay_transport_controls(ReplayGuiState& replayState) {
    if (full_width_button("End replay")) {
        replayState.command = ReplayGuiState::Command::Finish;
    }

    ImGui::Separator();

    const bool canStepBack = replayState.currentEntry > 0;
    const bool canStepForward = replayState.currentEntry < replayState.totalEntries;
    const float twoButtonWidth = equal_button_width(2);

    if (!canStepBack) {
        ImGui::BeginDisabled();
    }
    if (equal_width_button("Step Back", twoButtonWidth)) {
        replayState.command = ReplayGuiState::Command::StepBackward;
    }
    ImGui::SameLine();
    if (equal_width_button(replayState.isBackward && !replayState.isPaused ? "Reverse *" : "Reverse", twoButtonWidth)) {
        replayState.command = ReplayGuiState::Command::PlayReverse;
    }
    if (!canStepBack) {
        ImGui::EndDisabled();
    }

    const char* playPauseLabel = replayState.isPaused ? "Play" : "Pause";
    if (full_width_button(playPauseLabel)) {
        replayState.command = replayState.isPaused ? ReplayGuiState::Command::Play : ReplayGuiState::Command::Pause;
    }

    if (!canStepForward) {
        ImGui::BeginDisabled();
    }
    if (equal_width_button(!replayState.isBackward && !replayState.isPaused ? "Forward *" : "Forward",
                           twoButtonWidth)) {
        replayState.command = ReplayGuiState::Command::Play;
    }
    ImGui::SameLine();
    if (equal_width_button("Step Forward", twoButtonWidth)) {
        replayState.command = ReplayGuiState::Command::StepForward;
    }
    if (!canStepForward) {
        ImGui::EndDisabled();
    }
}

void render_replay_controls(ReplayGuiState& replayState) {
    if (!replayState.isActive) {
        return;
    }

    if (ImGui::CollapsingHeader("Replay", ImGuiTreeNodeFlags_DefaultOpen)) {
        const float progress = replayState.totalEntries > 0
                                   ? static_cast<float>(std::min(replayState.currentEntry, replayState.totalEntries)) /
                                         static_cast<float>(replayState.totalEntries)
                                   : 0.0F;
        ImGui::TextUnformatted(
            fmt::format("Entry {} / {}", replayState.currentEntry, replayState.totalEntries).c_str());
        ImGui::ProgressBar(progress, ImVec2{-1.0F, 0.0F});

        render_replay_speed_controls(replayState);
        render_replay_transport_controls(replayState);

        const std::size_t remainingEntryCount = replayState.totalEntries > replayState.currentEntry
                                                    ? replayState.totalEntries - replayState.currentEntry
                                                    : 0U;
        const int remainingEntries = static_cast<int>(remainingEntryCount);
        const int sliderMax = std::max(1, remainingEntries);
        int stepSize = static_cast<int>(replayState.entriesPerStep);
        stepSize = std::max(1, std::min(stepSize, sliderMax));
        ImGui::TextUnformatted("Step size");
        ImGui::SetNextItemWidth(-1.0F);
        if (ImGui::SliderInt("##StepSize", &stepSize, 1, sliderMax)) {
            replayState.requestedEntriesPerStep = static_cast<std::size_t>(stepSize);
        }
    }

    if (ImGui::CollapsingHeader("Shortcuts")) {
        ImGui::PushTextWrapPos(0.0F);
        ImGui::TextUnformatted(
            fmt::format("Play: {}, pause: {}", replayState.resumeKeysLabel, replayState.pauseKeysLabel).c_str());
        ImGui::TextUnformatted(fmt::format("Step: forward {}, back {}",
                                           replayState.stepForwardKeysLabel,
                                           replayState.stepBackwardKeysLabel)
                                   .c_str());
        ImGui::TextUnformatted(
            fmt::format("Step size: + {}, - {}", replayState.increaseStepKeysLabel, replayState.decreaseStepKeysLabel)
                .c_str());
        ImGui::PopTextWrapPos();
    }
}

} // namespace

void init_message_queue(ConcurrentQueue<GeoQikMessage>&& messageQueue) {
    message_queue_storage() = std::move(messageQueue);
}

ConcurrentQueue<GeoQikMessage>& get_message_queue() {
    return message_queue_storage();
}

void request_replay_cancel() {
    replay_cancel_requested_storage().store(true, std::memory_order_release);
}

// Default tone-mapping exposure applied at startup, in stops. A slight negative bias avoids
// over-bright highlights with the default lighting.
static constexpr float defaultExposureStops = -1.5F;

static CameraAutoFitSettings make_camera_auto_fit_settings(const GeoQikSettings& settings) {
    CameraAutoFitSettings autoFitSettings;
    autoFitSettings.enabled = settings.autoFitCameraEnabled;
    autoFitSettings.zoomInEnabled = settings.autoFitZoomInEnabled;
    autoFitSettings.zoomOutPadding = settings.autoFitZoomOutPadding;
    autoFitSettings.minViewportOccupancy = settings.autoFitMinViewportOccupancy;
    autoFitSettings.targetViewportOccupancy = settings.autoFitTargetViewportOccupancy;
    autoFitSettings.suppressAfterUserCameraInteraction = settings.autoFitSuppressAfterUserCameraInteraction;
    return autoFitSettings;
}

static linal::float3 scale_rgb(const std::array<float, 3>& color, float intensity) {
    const float clampedIntensity = std::max(0.0F, intensity);
    return linal::float3{color[0] * clampedIntensity, color[1] * clampedIntensity, color[2] * clampedIntensity};
}

static linal::float3 to_float3(const std::array<float, 3>& values) {
    return linal::float3{values[0], values[1], values[2]};
}

Context::Context() = default;

Context::~Context() {
    if (m_renderer) {
        cleanup();
    }
}

bool Context::init_window(const GeoQikSettings& geoqikSettings, const WindowSettings& settings) {
    if (m_renderer) {
        fmt::print("GeoQik context is already initialized.\n");
        CORE_ASSERT(false);
        return false;
    }

    m_geoqikSettings = geoqikSettings;
    m_windowSettings = std::make_unique<WindowSettings>(settings);

    m_scene = Scene::create(geoqikSettings);

    m_backgroundColor[0] = m_geoqikSettings.backgroundColor[0];
    m_backgroundColor[1] = m_geoqikSettings.backgroundColor[1];
    m_backgroundColor[2] = m_geoqikSettings.backgroundColor[2];
    m_backgroundColor[3] = m_geoqikSettings.backgroundColor[3];

    // geoqik owns the overlay so it can render a single unified control panel, so the renderer is
    // created without the built-in ImGui overlay.
    m_windowSettings->overlay = renderer::OverlayKind::None;

    m_renderer = renderer::Renderer::create(*m_windowSettings);
    if (!m_renderer) {
        return false;
    }
    m_renderer->set_camera_auto_fit_settings(make_camera_auto_fit_settings(m_geoqikSettings));
    m_renderer->set_camera_far_plane_multiplier(m_geoqikSettings.cameraFarPlaneMultiplier);
    m_renderer->set_exposure_stops(defaultExposureStops);

    m_overlay = std::make_shared<GeoQikOverlay>(m_renderer->window().get_native_handle());
    m_overlay->inner().set_ui_mode(renderer::UiMode::Release);
    m_overlay->set_build_controls([this](renderer::OverlayFrameContext& ctx) { build_overlay(ctx); });
    m_renderer->set_overlay(m_overlay);

    m_cameraGuiState = std::make_unique<CameraGuiState>();
    m_replayGuiState = std::make_unique<ReplayGuiState>();

    m_sceneRenderer = std::make_unique<GeoQikSceneRenderer>(*m_renderer);

    setup_window_callbacks();

    return true;
}

void Context::setup_window_callbacks() {
    m_keyCallback = m_renderer->add_key_callback(
        [this](Key key, Scancode scancode, Action action, Mods mods) { on_key(key, scancode, action, mods); });

    // A preset view stays active (and its interaction locks stay in effect) through pan/zoom, so
    // it is only left by explicitly choosing a navigation style or another preset in the UI. Scene
    // drags/scrolls therefore do not clear the active preset, and no scroll/mouse-button callbacks
    // are registered here for that purpose.
}

float Context::get_point_size() {
    return m_scene.get_point_size();
}

void Context::set_point_size(float pointSize) {
    m_scene.set_point_size(pointSize);
    m_sceneRenderer->recreate_point_drawables(m_scene);
    ++m_geometryMessagesProcessedThisFrame;
}

Color Context::get_point_color() {
    return m_scene.get_default_point_color();
}

void Context::set_default_point_color(Color color) {
    m_scene.set_default_point_color(color[0], color[1], color[2], color[3]);
    ++m_geometryMessagesProcessedThisFrame;
}

float Context::get_line_width() {
    return m_scene.get_line_width();
}

void Context::set_line_width(float lineWidth) {
    m_scene.set_line_width(lineWidth);
    m_sceneRenderer->recreate_line_drawables(m_scene);
    ++m_geometryMessagesProcessedThisFrame;
}

Color Context::get_line_color() {
    return m_scene.get_default_line_color();
}

void Context::set_line_color(Color color) {
    m_scene.set_default_line_color(color[0], color[1], color[2], color[3]);
    ++m_geometryMessagesProcessedThisFrame;
}

Color Context::get_mesh_color() {
    return m_scene.get_default_mesh_color();
}

void Context::set_mesh_color(Color color) {
    m_scene.set_default_mesh_color(color[0], color[1], color[2], color[3]);
    ++m_geometryMessagesProcessedThisFrame;
}

void Context::add_point_with_opts(float x, float y, float z, const GeoQikMessageCommonData& commonData) {
    if (is_known_idempotency_key(&commonData.idempotencyId)) {
        return;
    }
    if (m_scene.ensure_point_capacity(1)) {
        m_sceneRenderer->recreate_point_drawables(m_scene);
    }
    if (commonData.rgba.size() >= ColorChannelCount) {
        m_scene.add_point(x,
                          y,
                          z,
                          commonData.rgba[0],
                          commonData.rgba[1],
                          commonData.rgba[2],
                          commonData.rgba[3],
                          &commonData.geometryId);
    } else {
        m_scene.add_point(x, y, z, &commonData.geometryId);
    }
    ++m_geometryMessagesProcessedThisFrame;
}

void Context::add_points_with_opts(std::span<const float> points, const GeoQikMessageCommonData& commonData) {
    if (is_known_idempotency_key(&commonData.idempotencyId)) {
        return;
    }
    if (m_scene.ensure_point_capacity(points.size() / 3)) {
        m_sceneRenderer->recreate_point_drawables(m_scene);
    }
    m_scene.add_points(points, std::span<const float>(commonData.rgba), &commonData.geometryId);
    ++m_geometryMessagesProcessedThisFrame;
}

void Context::update_point_with_opts(const core::UUID& handle,
                                     float x,
                                     float y,
                                     float z,
                                     std::span<const float> colors) {
    if (m_scene.update_point(handle, x, y, z, colors)) {
        ++m_geometryMessagesProcessedThisFrame;
    }
}

void Context::update_points_with_opts(const core::UUID& handle,
                                      std::span<const float> points,
                                      std::span<const float> colors) {
    if (m_scene.update_points(handle, points, colors)) {
        ++m_geometryMessagesProcessedThisFrame;
    }
}

void Context::remove_point(const core::UUID& handle) {
    m_scene.remove_point(handle);
    ++m_geometryMessagesProcessedThisFrame;
}

void Context::add_line(float x1,
                       float y1,
                       float z1,
                       float x2,
                       float y2,
                       float z2,
                       const core::UUID* handle,
                       const core::UUID* idempotencyKey) {
    if (is_known_idempotency_key(idempotencyKey)) {
        return;
    }
    if (m_scene.ensure_line_capacity(1)) {
        m_sceneRenderer->recreate_line_drawables(m_scene);
    }
    m_scene.add_line(x1, y1, z1, x2, y2, z2, handle);
    ++m_geometryMessagesProcessedThisFrame;
}

void Context::add_line(float x1,
                       float y1,
                       float z1,
                       float x2,
                       float y2,
                       float z2,
                       float r,
                       float g,
                       float b,
                       float a,
                       const core::UUID* handle,
                       const core::UUID* idempotencyKey) {
    if (is_known_idempotency_key(idempotencyKey)) {
        return;
    }
    if (m_scene.ensure_line_capacity(1)) {
        m_sceneRenderer->recreate_line_drawables(m_scene);
    }
    m_scene.add_line(x1, y1, z1, x2, y2, z2, r, g, b, a, handle);
    ++m_geometryMessagesProcessedThisFrame;
}

void Context::add_line_with_opts(float x1,
                                 float y1,
                                 float z1,
                                 float x2,
                                 float y2,
                                 float z2,
                                 const GeoQikMessageCommonData& commonData) {
    if (is_known_idempotency_key(&commonData.idempotencyId)) {
        return;
    }
    if (m_scene.ensure_line_capacity(1)) {
        m_sceneRenderer->recreate_line_drawables(m_scene);
    }
    if (commonData.rgba.size() >= ColorChannelCount) {
        m_scene.add_line(x1,
                         y1,
                         z1,
                         x2,
                         y2,
                         z2,
                         commonData.rgba[0],
                         commonData.rgba[1],
                         commonData.rgba[2],
                         commonData.rgba[3],
                         &commonData.geometryId);
    } else {
        m_scene.add_line(x1, y1, z1, x2, y2, z2, &commonData.geometryId);
    }
    ++m_geometryMessagesProcessedThisFrame;
}

void Context::add_lines_with_opts(std::span<const float> lines, const GeoQikMessageCommonData& commonData) {
    if (is_known_idempotency_key(&commonData.idempotencyId)) {
        return;
    }
    if (m_scene.ensure_line_capacity(lines.size() / lineCoordinateCount)) {
        m_sceneRenderer->recreate_line_drawables(m_scene);
    }
    m_scene.add_lines(lines, std::span<const float>(commonData.rgba), &commonData.geometryId);
    ++m_geometryMessagesProcessedThisFrame;
}

void Context::update_line_with_opts(const core::UUID& handle,
                                    float x1,
                                    float y1,
                                    float z1,
                                    float x2,
                                    float y2,
                                    float z2,
                                    std::span<const float> colors) {
    if (m_scene.update_line(handle, x1, y1, z1, x2, y2, z2, colors)) {
        ++m_geometryMessagesProcessedThisFrame;
    }
}

void Context::update_lines_with_opts(const core::UUID& handle,
                                     std::span<const float> lines,
                                     std::span<const float> colors) {
    if (m_scene.update_lines(handle, lines, colors)) {
        ++m_geometryMessagesProcessedThisFrame;
    }
}

void Context::remove_line(const core::UUID& handle) {
    m_scene.remove_line(handle);
    ++m_geometryMessagesProcessedThisFrame;
}

void Context::remove_all_geometry() {
    m_scene.clear();
    m_sceneRenderer->clear_drawables();
    ++m_geometryMessagesProcessedThisFrame;
}

void Context::translate_geometry(const core::UUID& handle, float dx, float dy, float dz) {
    m_scene.translate_geometry(handle, dx, dy, dz);
    ++m_geometryMessagesProcessedThisFrame;
}

void Context::rotate_geometry(const core::UUID& handle,
                              float centerX,
                              float centerY,
                              float centerZ,
                              float axisX,
                              float axisY,
                              float axisZ,
                              float angle) {
    m_scene.rotate_geometry(handle, centerX, centerY, centerZ, axisX, axisY, axisZ, angle);
    ++m_geometryMessagesProcessedThisFrame;
}

void Context::scale_geometry(const core::UUID& handle, float cx, float cy, float cz, float sx, float sy, float sz) {
    m_scene.scale_geometry(handle, cx, cy, cz, sx, sy, sz);
    ++m_geometryMessagesProcessedThisFrame;
}

void Context::set_geometry_color(const core::UUID& handle, float r, float g, float b, float a) {
    m_scene.set_geometry_color(handle, r, g, b, a);
    ++m_geometryMessagesProcessedThisFrame;
}

const Viewport& Context::get_viewport() {
    return m_renderer->get_camera().lock()->get_viewport();
}

// #define PRINT_FRAME_INFO

void Context::run_event_loop() {
    assert(m_renderer);

    auto& messageQueue = get_message_queue();
    m_lastReplayTick = std::chrono::high_resolution_clock::now();
    while (!m_windowShouldClose) {
        std::chrono::high_resolution_clock::time_point frameStartTime = std::chrono::high_resolution_clock::now();

        m_renderer->window().make_context_current();

        renderer::Renderer::poll_events();
        if (should_close_event_loop()) {
            break;
        }

        const renderer::ClearColor clearColor{m_backgroundColor[0],
                                              m_backgroundColor[1],
                                              m_backgroundColor[2],
                                              m_backgroundColor[3]};
        m_renderer->begin_frame(clearColor);

        m_sceneRenderer->sync_scene(m_scene);

        renderer::LightingConfig lighting;
        lighting.lightColor = scale_rgb(m_geoqikSettings.meshHeadLightColor, m_geoqikSettings.meshHeadLightIntensity);
        lighting.fillLightDir = to_float3(m_geoqikSettings.meshFillLightDirection);
        lighting.fillLightColor =
            scale_rgb(m_geoqikSettings.meshFillLightColor, m_geoqikSettings.meshFillLightIntensity);
        lighting.ambientColor = scale_rgb(m_geoqikSettings.meshAmbientColor, m_geoqikSettings.meshAmbientIntensity);
        lighting.shininess = std::max(0.0F, m_geoqikSettings.meshShininess);

        m_renderer->draw(lighting);
        // The overlay's build_controls (invoked inside end_frame) populates m_cameraGuiState /
        // m_replayGuiState and renders the unified panel. Widget edits are applied afterwards in
        // consume_camera_gui_commands (a Home click there re-enables auto-fit for its fit).
        bool autoFitEnabled = m_geoqikSettings.autoFitCameraEnabled;
        m_renderer->end_frame(autoFitEnabled);
        consume_camera_gui_commands(*m_cameraGuiState);
        consume_replay_gui_commands(*m_replayGuiState);

        process_replay_entries(std::chrono::high_resolution_clock::now());
        if (!is_replaying()) {
            process_deferred_messages();
        }
        if (m_windowShouldClose) {
            return;
        }

#ifdef PRINT_FRAME_INFO
        const std::chrono::high_resolution_clock::time_point messageProcessingStartTime =
            std::chrono::high_resolution_clock::now();
#endif
        process_message_queue(messageQueue, frameStartTime);
        if (m_windowShouldClose) {
            return;
        }

#ifdef PRINT_FRAME_INFO
        std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
        print_frame_info(frameStartTime, messageProcessingStartTime, endTime);
#endif
    }
}

bool Context::should_close_event_loop() {
    if (m_renderer->should_close()) {
        m_windowShouldClose.store(true);
        return true;
    }

    const bool keyboardCaptured = m_overlay->wants_keyboard();
    if (!keyboardCaptured && m_renderer->is_escape_pressed()) {
        m_windowShouldClose.store(true);
        return true;
    }

    return false;
}

void Context::populate_replay_gui_state(ReplayGuiState& state) const {
    state.isActive = is_replaying();
    state.isPaused = m_isReplayPaused;
    state.isBackward = m_isReplayBackward;
    const auto [current, total] = get_replay_progress();
    state.currentEntry = current;
    state.totalEntries = total;
    state.speedMultiplier = m_currentSpeedMultiplier;
    state.entriesPerStep = m_replayOptions.entriesPerStep;
    state.pauseKeysLabel = key_labels(m_replayOptions.pauseKeys);
    state.resumeKeysLabel = key_labels(m_replayOptions.resumeKeys);
    state.stepForwardKeysLabel = key_labels(m_replayOptions.stepKeys);
    state.stepBackwardKeysLabel = key_labels(m_replayOptions.backwardStepKeys);
    state.increaseStepKeysLabel = key_labels(m_replayOptions.increaseEntriesPerStepKeys);
    state.decreaseStepKeysLabel = key_labels(m_replayOptions.decreaseEntriesPerStepKeys);
}

void Context::consume_replay_gui_commands(ReplayGuiState& state) {
    // m_replayGuiState is a persistent member reused every frame, so drain these one-shot requests
    // as they are consumed - otherwise a lingering command (e.g. Play) re-runs every frame,
    // repeatedly zeroing m_replayEntryBudget / m_lastReplayTick so the budget never accumulates and
    // playback never advances. Mirrors how consume_camera_gui_commands resets its optionals.
    const ReplayGuiState::Command command = std::exchange(state.command, ReplayGuiState::Command::None);
    const std::optional<double> requestedSpeedMultiplier = std::exchange(state.requestedSpeedMultiplier, std::nullopt);
    const std::optional<std::size_t> requestedEntriesPerStep =
        std::exchange(state.requestedEntriesPerStep, std::nullopt);

    if (!is_replaying()) {
        return;
    }

    if (requestedSpeedMultiplier.has_value()) {
        m_currentSpeedMultiplier = *requestedSpeedMultiplier;
        m_replayOptions.entriesPerSecond = m_baseEntriesPerSecond * m_currentSpeedMultiplier;
    }

    if (requestedEntriesPerStep.has_value()) {
        m_replayOptions.entriesPerStep = *requestedEntriesPerStep;
    }

    switch (command) {
    case ReplayGuiState::Command::Play:
        m_isReplayBackward = false;
        m_isReplayPaused = false;
        m_replayEntryBudget = 0.0;
        m_lastReplayTick = std::chrono::high_resolution_clock::now();
        break;

    case ReplayGuiState::Command::PlayReverse:
        m_isReplayBackward = true;
        m_isReplayPaused = false;
        m_replayEntryBudget = 0.0;
        m_lastReplayTick = std::chrono::high_resolution_clock::now();
        break;

    case ReplayGuiState::Command::Pause: m_isReplayPaused = true; break;

    case ReplayGuiState::Command::Finish: {
        m_isReplayBackward = false;
        m_isReplayPaused = false;
        const std::size_t remaining = m_replayEntries.size() - m_replayEntryIndex;
        if (remaining > 0) {
            apply_replay_entries(remaining);
        }
        finish_replay();
        break;
    }

    case ReplayGuiState::Command::StepForward:
        m_isReplayPaused = true;
        step_replay_entries(m_replayOptions.entriesPerStep);
        break;

    case ReplayGuiState::Command::StepBackward:
        m_isReplayPaused = true;
        step_replay_entries_backward(m_replayOptions.entriesPerStep);
        break;

    case ReplayGuiState::Command::None: break;
    }
}

void Context::populate_camera_gui_state(CameraGuiState& state) const {
    state.activePreset = m_activePresetView;
    if (auto camera = m_renderer->get_camera().lock()) {
        state.navigationStyle = camera->get_navigation_style();
        const auto viewMode = camera->get_view_mode();
        using ViewMode = renderer::CameraInteractor::CameraViewMode;
        const auto isFixed = [viewMode](ViewMode flag) {
            return (static_cast<std::uint8_t>(viewMode) & static_cast<std::uint8_t>(flag)) != 0U;
        };
        state.fixRotate = isFixed(ViewMode::FIX_ROTATE);
        state.fixPan = isFixed(ViewMode::FIX_PAN);
        state.fixZoom = isFixed(ViewMode::FIX_ZOOM);
    }
}

void Context::consume_camera_gui_commands(CameraGuiState& state) {
    // The widget callbacks ran during the overlay's render() (inside end_frame), so their edits are
    // now visible. Apply them here and clear the one-shot requests so they are not re-applied every
    // frame (m_cameraGuiState is a persistent member, not a per-frame local).

    // Auto Zoom: persist the checkbox into the settings; it takes effect from the next frame.
    m_geoqikSettings.autoFitCameraEnabled = state.autoZoom;

    if (state.requestedProjection.has_value()) {
        if (auto camera = m_renderer->get_camera().lock()) {
            camera->set_projection_type(*state.requestedProjection);
        }
        state.requestedProjection.reset();
    }
    if (state.requestedNavigationStyle.has_value()) {
        apply_navigation_style(*state.requestedNavigationStyle);
        state.requestedNavigationStyle.reset();
    }
    if (state.requestedPreset.has_value()) {
        apply_preset_view(*state.requestedPreset);
        state.requestedPreset.reset();
    }
    if (state.requestHome) {
        request_fit_all_geometry();
        state.requestHome = false;
    }
}

void Context::request_fit_all_geometry() {
    // Re-frame the geometry along the current viewing direction, so it stays within the restrictions
    // of the active preset view and does not change the navigation style or view mode. This moves the
    // camera immediately and unconditionally - it does not depend on the persistent Auto Zoom setting
    // and is not suppressed right after a user camera interaction, so the Home button always acts.
    m_renderer->refit_current_view();
}

void Context::build_overlay(renderer::OverlayFrameContext& ctx) {
    auto& ui = m_overlay->inner();

    // Reserve the left control-panel strip for the UI and hand the remaining window region to the
    // 3D scene, so the ImGui overlay no longer draws on top of the scene. The panel geometry mirrors
    // plinth's ImGuiOverlay layout (an 8px margin on each side of a fixed-width panel).
    if (const ImGuiViewport* viewport = ImGui::GetMainViewport(); viewport != nullptr) {
        constexpr float controlPanelMargin = 8.0F;
        constexpr float controlPanelWidth = 320.0F;
        const float reservedLeft = (2.0F * controlPanelMargin) + controlPanelWidth;
        const float sceneWidth = std::max(1.0F, viewport->WorkSize.x - reservedLeft);
        ctx.sceneViewportHint = renderer::LogicalViewportRect{static_cast<double>(viewport->WorkPos.x + reservedLeft),
                                                              static_cast<double>(viewport->WorkPos.y),
                                                              static_cast<double>(sceneWidth),
                                                              static_cast<double>(viewport->WorkSize.y)};
    }

    // Snapshot the current camera state into the GUI struct so the widgets display it. The widget
    // callbacks (which record the user's edits) run later, inside ui.render(); those edits are
    // applied afterwards in consume_camera_gui_commands, once end_frame has returned.
    populate_camera_gui_state(*m_cameraGuiState);
    m_cameraGuiState->autoZoom = m_geoqikSettings.autoFitCameraEnabled;
    m_cameraGuiState->projectionType = ctx.projectionType;
    ui.add_control([this]() { render_camera_controls(*m_cameraGuiState); });

    populate_replay_gui_state(*m_replayGuiState);
    if (m_replayGuiState->isActive) {
        ui.add_control([this]() { render_replay_controls(*m_replayGuiState); });
    }

    // Retain plinth's post-processing panel, choosing Debug vs Release like plinth's own overlay.
    if (ui.ui_mode() == renderer::UiMode::Debug) {
        ui.add_post_processing_controls(ctx.renderer);
    } else {
        ui.add_release_post_processing_controls(ctx.renderer);
    }
}

bool Context::should_stop_processing_messages(
    const std::chrono::high_resolution_clock::time_point& frameStartTime,
    const std::chrono::high_resolution_clock::time_point& messageProcessingStartTime) const {
    if (!m_isDrawing) {
        return false;
    }

    const auto now = std::chrono::high_resolution_clock::now();
    const auto elapsedMessageTime =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - messageProcessingStartTime);

    bool minimumProcessingTimeReached = true;
    if (m_geoqikSettings.minGeometryProcessingTime > std::chrono::milliseconds(0)) {
        minimumProcessingTimeReached = elapsedMessageTime >= m_geoqikSettings.minGeometryProcessingTime;
    }

    return minimumProcessingTimeReached && (now - frameStartTime) >= m_geoqikSettings.maxFrameProcessingTime;
}

void Context::process_message_queue(ConcurrentQueue<GeoQikMessage>& messageQueue,
                                    const std::chrono::high_resolution_clock::time_point& frameStartTime) {
    m_geometryMessagesProcessedThisFrame = 0;
    const std::chrono::high_resolution_clock::time_point messageProcessingStartTime =
        std::chrono::high_resolution_clock::now();
    while (!messageQueue.empty()) {
        if (should_stop_processing_messages(frameStartTime, messageProcessingStartTime)) {
            return;
        }

        std::optional<GeoQikMessage> message = messageQueue.dequeue();
        if (!message.has_value()) {
            continue;
        }

        defer_or_handle_message(std::move(*message));
        if (m_windowShouldClose) {
            return;
        }
    }
}

bool Context::cleanup() {
    if (!m_renderer) {
        fmt::print("GLFW window is not initialized.\n");
        return false;
    }

    m_renderer->window().make_context_current();
    m_sceneRenderer.reset();
    m_renderer.reset();

    return true;
}

geoqik_error_code_t Context::save_log(const char* path, geoqik_log_format_t format) const {
    if (path == nullptr || path[0] == '\0' ||
        (format != GEOQIK_LOG_FORMAT_BINARY && format != GEOQIK_LOG_FORMAT_JSON)) {
        return GEOQIK_ERROR_INVALID_PARAMETER;
    }

    try {
        if (format == GEOQIK_LOG_FORMAT_JSON) {
            save_log_json(path, m_messageLog);
        } else {
            save_log_binary(path, m_messageLog);
        }
        return GEOQIK_SUCCESS;
    } catch (const std::bad_alloc&) {
        return GEOQIK_ERROR_MEMORY_ALLOCATION;
    } catch (...) {
        return GEOQIK_ERROR_UNKNOWN;
    }
}

geoqik_error_code_t Context::load_log(const char* path, geoqik_log_format_t format) {
    if (path == nullptr || path[0] == '\0' ||
        (format != GEOQIK_LOG_FORMAT_BINARY && format != GEOQIK_LOG_FORMAT_JSON)) {
        return GEOQIK_ERROR_INVALID_PARAMETER;
    }

    try {
        if (!is_existing_regular_file(path)) {
            return GEOQIK_ERROR_UNKNOWN;
        }

        std::vector<GeoQikLogEntry> loadedEntries =
            format == GEOQIK_LOG_FORMAT_JSON ? load_log_json(path) : load_log_binary(path);
        remove_all_geometry();
        m_idempotencySet.clear();
        replay_log_entries(loadedEntries);
        m_messageLog = std::move(loadedEntries);
        return GEOQIK_SUCCESS;
    } catch (const std::bad_alloc&) {
        return GEOQIK_ERROR_MEMORY_ALLOCATION;
    } catch (...) {
        return GEOQIK_ERROR_UNKNOWN;
    }
}

geoqik_error_code_t Context::replay_log(const char* path, geoqik_log_format_t format, const ReplayOptions& options) {
    if (path == nullptr || path[0] == '\0' ||
        (format != GEOQIK_LOG_FORMAT_BINARY && format != GEOQIK_LOG_FORMAT_JSON)) {
        return GEOQIK_ERROR_INVALID_PARAMETER;
    }

    try {
        if (!is_existing_regular_file(path)) {
            return GEOQIK_ERROR_UNKNOWN;
        }

        std::vector<GeoQikLogEntry> loadedEntries =
            format == GEOQIK_LOG_FORMAT_JSON ? load_log_json(path) : load_log_binary(path);
        start_replay(std::move(loadedEntries), options);
        return GEOQIK_SUCCESS;
    } catch (const std::bad_alloc&) {
        return GEOQIK_ERROR_MEMORY_ALLOCATION;
    } catch (...) {
        return GEOQIK_ERROR_UNKNOWN;
    }
}

geoqik_error_code_t Context::replay_current_log(const ReplayOptions& options) {
    try {
        start_replay(m_messageLog, options);
        return GEOQIK_SUCCESS;
    } catch (const std::bad_alloc&) {
        return GEOQIK_ERROR_MEMORY_ALLOCATION;
    } catch (...) {
        return GEOQIK_ERROR_UNKNOWN;
    }
}

void Context::cancel_replay() {
    if (!is_replaying()) {
        m_isReplayPaused = false;
        m_isReplayActive = false;
        return;
    }

    m_replayEntries.clear();
    m_replayUndoStack.clear();
    m_replayEntryIndex = 0;
    m_replayEntryBudget = 0.0;
    m_isReplayActive = false;
    m_isReplayPaused = false;
    m_isReplayBackward = false;
    m_currentSpeedMultiplier = 1.0;
}

void Context::pause_replay() {
    if (is_replaying()) {
        m_isReplayPaused = true;
    }
}

void Context::resume_replay() {
    if (is_replaying() && m_replayEntryIndex < m_replayEntries.size()) {
        m_isReplayPaused = false;
        m_replayEntryBudget = 0.0;
        m_lastReplayTick = std::chrono::high_resolution_clock::now();
    }
}

void Context::step_replay_entries(std::size_t count) {
    if (!is_replaying() || m_replayEntryIndex >= m_replayEntries.size()) {
        return;
    }

    m_isReplayPaused = true;
    m_replayEntryBudget = 0.0;
    apply_replay_entries(count);
}

void Context::step_replay_entries_backward(std::size_t count) {
    if (!is_replaying() || m_replayEntryIndex == 0) {
        return;
    }

    m_isReplayPaused = true;
    m_replayEntryBudget = 0.0;
    undo_replay_entries(count);
}

geoqik_replay_state_t Context::get_replay_state() const {
    if (!is_replaying()) {
        return GEOQIK_REPLAY_INACTIVE;
    }
    return m_isReplayPaused ? GEOQIK_REPLAY_PAUSED : GEOQIK_REPLAY_PLAYING;
}

std::pair<std::size_t, std::size_t> Context::get_replay_progress() const {
    if (!is_replaying()) {
        return {0, 0};
    }
    return {m_replayEntryIndex, m_replayEntries.size()};
}

void Context::handle_message(const AddPointWithOpts& message) {
    if (message.styled) {
        if (is_known_idempotency_key(&message.commonData.idempotencyId)) {
            return;
        }
        StyledPointData data;
        data.points = {message.x, message.y, message.z};
        data.colors = expand_vertex_colors(message.commonData.rgba, 1, m_scene.get_default_point_color());
        data.radii = message.radii;
        data.sizeSpace = message.sizeSpace;
        m_scene.add_styled_points(message.commonData.geometryId, std::move(data));
        ++m_geometryMessagesProcessedThisFrame;
        return;
    }
    add_point_with_opts(message.x, message.y, message.z, message.commonData);
}

void Context::handle_message(const AddPointsWithOpts& message) {
    if (message.styled) {
        if (is_known_idempotency_key(&message.commonData.idempotencyId)) {
            return;
        }
        StyledPointData data;
        data.points = message.points;
        data.colors =
            expand_vertex_colors(message.commonData.rgba, message.points.size() / 3, m_scene.get_default_point_color());
        data.radii = message.radii;
        data.sizeSpace = message.sizeSpace;
        m_scene.add_styled_points(message.commonData.geometryId, std::move(data));
        ++m_geometryMessagesProcessedThisFrame;
        return;
    }
    add_points_with_opts(message.points, message.commonData);
}

void Context::handle_message(const UpdatePointWithOpts& message) {
    if (m_scene.is_styled_point(message.handle)) {
        const std::array<float, 3> points{message.x, message.y, message.z};
        const auto colors = message.rgba.empty()
                                ? std::vector<float>{}
                                : expand_vertex_colors(message.rgba, 1, m_scene.get_default_point_color());
        if (m_scene.update_styled_points(message.handle,
                                         points,
                                         colors,
                                         message.radii,
                                         message.sizeSpace,
                                         message.styled)) {
            ++m_geometryMessagesProcessedThisFrame;
        }
        return;
    }
    update_point_with_opts(message.handle, message.x, message.y, message.z, message.rgba);
}

void Context::handle_message(const UpdatePointsWithOpts& message) {
    if (m_scene.is_styled_point(message.handle)) {
        const auto colors =
            message.rgba.empty()
                ? std::vector<float>{}
                : expand_vertex_colors(message.rgba, message.points.size() / 3, m_scene.get_default_point_color());
        if (m_scene.update_styled_points(message.handle,
                                         message.points,
                                         colors,
                                         message.radii,
                                         message.sizeSpace,
                                         message.styled)) {
            ++m_geometryMessagesProcessedThisFrame;
        }
        return;
    }
    update_points_with_opts(message.handle, message.points, message.rgba);
}

void Context::handle_message(const RemovePoint& message) {
    remove_point(message.handle);
}

void Context::handle_message(const SetPointSize& message) {
    set_point_size(message.size);
}

void Context::handle_message(const SetPointColor& message) {
    set_default_point_color(message.color);
}

void Context::handle_message(const AddLineWithOpts& message) {
    if (message.styleSet) {
        if (is_known_idempotency_key(&message.commonData.idempotencyId)) {
            return;
        }
        StyledLineData data;
        data.vertices = {message.x1, message.y1, message.z1, message.x2, message.y2, message.z2};
        data.colors = expand_vertex_colors(message.commonData.rgba, 2, m_scene.get_default_line_color(), true);
        data.style = message.style;
        data.lineType = message.lineType;
        data.perVertexDashFlags = message.perVertexDashFlags;
        m_scene.add_styled_line(message.commonData.geometryId, std::move(data));
        ++m_geometryMessagesProcessedThisFrame;
        return;
    }
    add_line_with_opts(message.x1, message.y1, message.z1, message.x2, message.y2, message.z2, message.commonData);
}

void Context::handle_message(const AddLinesWithOpts& message) {
    if (message.styleSet) {
        if (is_known_idempotency_key(&message.commonData.idempotencyId)) {
            return;
        }
        StyledLineData data;
        data.vertices = message.lines;
        data.colors = expand_vertex_colors(message.commonData.rgba,
                                           message.lines.size() / 3,
                                           m_scene.get_default_line_color(),
                                           true);
        data.style = message.style;
        data.lineType = message.lineType;
        data.perVertexDashFlags = message.perVertexDashFlags;
        m_scene.add_styled_line(message.commonData.geometryId, std::move(data));
        ++m_geometryMessagesProcessedThisFrame;
        return;
    }
    add_lines_with_opts(message.lines, message.commonData);
}

void Context::handle_message(const UpdateLineWithOpts& message) {
    if (m_scene.is_styled_line(message.handle)) {
        const std::array<float, 6> vertices{message.x1, message.y1, message.z1, message.x2, message.y2, message.z2};
        const auto colors = message.rgba.empty()
                                ? std::vector<float>{}
                                : expand_vertex_colors(message.rgba, 2, m_scene.get_default_line_color(), true);
        if (m_scene.update_styled_line(message.handle,
                                       vertices,
                                       colors,
                                       message.style,
                                       message.lineType,
                                       message.perVertexDashFlags,
                                       message.styleSet)) {
            ++m_geometryMessagesProcessedThisFrame;
        }
        return;
    }
    update_line_with_opts(message.handle,
                          message.x1,
                          message.y1,
                          message.z1,
                          message.x2,
                          message.y2,
                          message.z2,
                          message.rgba);
}

void Context::handle_message(const UpdateLinesWithOpts& message) {
    if (m_scene.is_styled_line(message.handle)) {
        const auto colors =
            message.rgba.empty()
                ? std::vector<float>{}
                : expand_vertex_colors(message.rgba, message.lines.size() / 3, m_scene.get_default_line_color(), true);
        if (m_scene.update_styled_line(message.handle,
                                       message.lines,
                                       colors,
                                       message.style,
                                       message.lineType,
                                       message.perVertexDashFlags,
                                       message.styleSet)) {
            ++m_geometryMessagesProcessedThisFrame;
        }
        return;
    }
    update_lines_with_opts(message.handle, message.lines, message.rgba);
}

void Context::handle_message(const RemoveLine& message) {
    remove_line(message.handle);
}

void Context::handle_message(const SetLineWidth& message) {
    set_line_width(message.width);
}

void Context::handle_message(const SetLineColor& message) {
    set_line_color(message.color);
}

void Context::handle_message([[maybe_unused]] const RemoveAllGeometry& message) {
    remove_all_geometry();
}

void Context::handle_message(const TranslateGeometry& message) {
    translate_geometry(message.handle, message.dx, message.dy, message.dz);
}

void Context::handle_message(const RotateGeometry& message) {
    rotate_geometry(message.handle,
                    message.centerX,
                    message.centerY,
                    message.centerZ,
                    message.axisX,
                    message.axisY,
                    message.axisZ,
                    message.angle);
}

void Context::handle_message(const ScaleGeometry& message) {
    scale_geometry(message.handle,
                   message.centerX,
                   message.centerY,
                   message.centerZ,
                   message.scaleX,
                   message.scaleY,
                   message.scaleZ);
}

void Context::handle_message(const SetGeometryColor& message) {
    set_geometry_color(message.handle, message.color[0], message.color[1], message.color[2], message.color[3]);
}

void Context::handle_message(const AddMeshWithOpts& message) {
    if (is_known_idempotency_key(&message.commonData.idempotencyId)) {
        return;
    }
    add_mesh_with_opts(message.vertices,
                       message.normals,
                       message.commonData.rgba,
                       message.triangleIndices,
                       message.commonData);

    // Wire up overlay data if the message carries segment or vertex data.
    const bool hasSegmentData = !message.segmentIndices.empty() || message.showSegments;
    const bool hasVertexData = message.showVertices || !message.vertexColors.empty();
    if (hasSegmentData || hasVertexData) {
        const core::UUID& uuid = message.commonData.geometryId;
        if (!uuid.is_nil()) {
            geoqik::PerMeshOverlayData overlayData;

            // Segment overlay
            overlayData.showSegments = message.showSegments;
            overlayData.segmentLineWidth = message.segmentLineWidth;

            auto verts = m_scene.get_mesh_buffer().get_mesh_vertices(uuid);
            overlayData.segmentPositions.assign(verts.begin(), verts.end());

            if (message.segmentIndices.empty()) {
                auto localTris = m_scene.get_mesh_buffer().get_local_triangle_indices(uuid);
                overlayData.segmentIndices = geoqik::derive_segment_indices_from_triangles(localTris);
            } else {
                overlayData.segmentIndices = message.segmentIndices;
            }

            if (message.segmentColors.size() == 4) {
                overlayData.segmentColor = {message.segmentColors[0],
                                            message.segmentColors[1],
                                            message.segmentColors[2],
                                            message.segmentColors[3]};
            }

            // Vertex overlay
            overlayData.showVertices = message.showVertices;
            overlayData.vertexPointSize = message.vertexPointSize;
            if (message.vertexColors.size() == 4) {
                overlayData.vertexColor = {message.vertexColors[0],
                                           message.vertexColors[1],
                                           message.vertexColors[2],
                                           message.vertexColors[3]};
            }

            m_scene.get_mesh_buffer().set_mesh_overlay_data(uuid, std::move(overlayData));
        }
    }
}

void Context::add_mesh_with_opts(std::span<const float> vertices,
                                 std::span<const float> normals,
                                 std::span<const float> colors,
                                 std::span<const std::uint32_t> triangleIndices,
                                 const GeoQikMessageCommonData& commonData) {
    const core::UUID* handlePtr = commonData.geometryId.is_nil() ? nullptr : &commonData.geometryId;
    m_scene.add_mesh(vertices, normals, colors, triangleIndices, handlePtr);
    ++m_geometryMessagesProcessedThisFrame;
}

void Context::handle_message(const SetMeshOverlayOpts& message) {
    m_scene.set_mesh_overlay_opts(message.handle, message.showSegments, message.showVertices);
}

void Context::handle_message(const SetMeshRenderingOpts& message) {
    m_scene.set_mesh_rendering_opts(message.handle, {message.cullMode, message.surfaceVisible});
}

void Context::handle_message(const RemoveMesh& message) {
    m_scene.remove_mesh(message.handle);
    ++m_geometryMessagesProcessedThisFrame;
}

void Context::update_mesh_with_opts(const core::UUID& handle,
                                    std::span<const float> vertices,
                                    std::span<const float> normals,
                                    std::span<const float> colors) {
    if (m_scene.update_mesh(handle, vertices, normals, colors)) {
        ++m_geometryMessagesProcessedThisFrame;
    }
}

void Context::handle_message(const UpdateMeshWithOpts& message) {
    update_mesh_with_opts(message.handle, message.vertices, message.normals, message.colors);
}

void Context::handle_message([[maybe_unused]] const Draw& message) {
    m_isDrawing = true;
}

void Context::handle_message([[maybe_unused]] const StopDraw& message) {
    m_isDrawing = false;
}

void Context::handle_message(const SaveLog& message) {
    CORE_ASSERT(message.callback);
    message.callback(*this);
}

void Context::handle_message(const LoadLog& message) {
    CORE_ASSERT(message.callback);
    message.callback(*this);
}

void Context::handle_message(const ReplayLog& message) {
    CORE_ASSERT(message.callback);
    message.callback(*this);
}

void Context::handle_message(const ReplayCurrentLog& message) {
    CORE_ASSERT(message.callback);
    message.callback(*this);
}

void Context::handle_message([[maybe_unused]] const PauseReplay& message) {
    pause_replay();
}

void Context::handle_message([[maybe_unused]] const ResumeReplay& message) {
    resume_replay();
}

void Context::handle_message(const StepReplay& message) {
    step_replay_entries(message.count);
}

void Context::handle_message(const StepReplayBackward& message) {
    step_replay_entries_backward(message.count);
}

void Context::handle_message(const GetReplayState& message) {
    CORE_ASSERT(message.callback);
    message.callback(*this);
}

void Context::handle_message(const GetReplayProgress& message) {
    CORE_ASSERT(message.callback);
    message.callback(*this);
}

void Context::handle_message(const GetPointSize& message) {
    CORE_ASSERT(message.callback);
    message.callback(*this);
}

void Context::handle_message(const GetPointColor& message) {
    CORE_ASSERT(message.callback);
    message.callback(*this);
}

void Context::handle_message(const GetLineWidth& message) {
    CORE_ASSERT(message.callback);
    message.callback(*this);
}

void Context::handle_message(const GetLineColor& message) {
    CORE_ASSERT(message.callback);
    message.callback(*this);
}

void Context::handle_message(const SetMeshColor& message) {
    set_mesh_color(message.color);
}

void Context::handle_message(const GetMeshColor& message) {
    message.callback(*this);
}

void Context::handle_message([[maybe_unused]] const Cleanup& message) {
    cleanup();
    m_windowShouldClose.store(true);
}

bool Context::is_replaying() const {
    return m_isReplayActive;
}

bool Context::is_control_message(const GeoQikMessage& message) {
    return std::holds_alternative<Cleanup>(message) || std::holds_alternative<PauseReplay>(message) ||
           std::holds_alternative<ResumeReplay>(message) || std::holds_alternative<StepReplay>(message) ||
           std::holds_alternative<StepReplayBackward>(message) || std::holds_alternative<GetReplayState>(message) ||
           std::holds_alternative<GetReplayProgress>(message);
}

void Context::apply_preset_view(renderer::PresetView view) {
    // ISO is a free 3D vantage point, so it leaves all interactive movement unlocked. The
    // orthographic presets lock rotation so the fixed view is not accidentally orbited away.
    const auto viewMode = view == renderer::PresetView::ISO ? renderer::CameraInteractor::CameraViewMode::NONE
                                                            : renderer::CameraInteractor::CameraViewMode::FIX_ROTATE;
    m_renderer->go_to_preset_view(view);
    if (auto camera = m_renderer->get_camera().lock()) {
        camera->set_view_mode(viewMode);
    }
    m_activePresetView = view;
}

void Context::apply_navigation_style(renderer::CameraInteractor::NavigationStyle style) {
    if (auto camera = m_renderer->get_camera().lock()) {
        camera->set_navigation_style(style);
        // Choosing a navigation style means free navigation, so release any interaction locks a
        // preset view had applied (e.g. the FIX_ROTATE that the orthographic presets set). Without
        // this, switching to Orbit/Fly would leave rotation dead until the lock was cleared some
        // other way.
        camera->set_view_mode(renderer::CameraInteractor::CameraViewMode::NONE);
    }
    m_activePresetView.reset();
}

void Context::handle_camera_key(Key key, Action action) {
    if (action != Action::PRESS) {
        return;
    }

    // F1 toggles between the game-like Release control panel (the default) and the full Debug
    // panel exposing every post-processing and visualization control.
    if (key == Key::KEY_F1) {
        auto& ui = m_overlay->inner();
        ui.set_ui_mode(ui.ui_mode() == renderer::UiMode::Release ? renderer::UiMode::Debug : renderer::UiMode::Release);
        return;
    }

    // Tab toggles between orbit navigation and fly (WASD+QE) navigation.
    if (key == Key::KEY_TAB) {
        if (auto camera = m_renderer->get_camera().lock()) {
            apply_navigation_style(camera->get_navigation_style() == renderer::CameraInteractor::NavigationStyle::ORBIT
                                       ? renderer::CameraInteractor::NavigationStyle::FLY
                                       : renderer::CameraInteractor::NavigationStyle::ORBIT);
        }
        return;
    }

    // Number keys 1-7 jump to named preset views, fitted to whatever geometry currently exists.
    switch (key) {
    case Key::KEY_1: apply_preset_view(renderer::PresetView::FRONT); break;
    case Key::KEY_2: apply_preset_view(renderer::PresetView::BACK); break;
    case Key::KEY_3: apply_preset_view(renderer::PresetView::LEFT); break;
    case Key::KEY_4: apply_preset_view(renderer::PresetView::RIGHT); break;
    case Key::KEY_5: apply_preset_view(renderer::PresetView::TOP); break;
    case Key::KEY_6: apply_preset_view(renderer::PresetView::BOTTOM); break;
    case Key::KEY_7: apply_preset_view(renderer::PresetView::ISO); break;
    default:         break;
    }
}

void Context::on_key(Key key, [[maybe_unused]] Scancode scancode, Action action, [[maybe_unused]] Mods mods) {
    handle_camera_key(key, action);

    if (!is_replaying()) {
        return;
    }

    if (action == Action::PRESS && m_isReplayPaused && has_replay_key(m_replayOptions.resumeKeys, key)) {
        resume_replay();
        return;
    }

    if (action == Action::PRESS && !m_isReplayPaused && has_replay_key(m_replayOptions.pauseKeys, key)) {
        pause_replay();
        return;
    }

    if ((action == Action::PRESS || action == Action::REPEAT) && has_replay_key(m_replayOptions.stepKeys, key)) {
        step_replay_entries(m_replayOptions.entriesPerStep);
        return;
    }

    if ((action == Action::PRESS || action == Action::REPEAT) &&
        has_replay_key(m_replayOptions.backwardStepKeys, key)) {
        step_replay_entries_backward(m_replayOptions.entriesPerStep);
        return;
    }

    if ((action == Action::PRESS || action == Action::REPEAT) &&
        has_replay_key(m_replayOptions.increaseEntriesPerStepKeys, key)) {
        ++m_replayOptions.entriesPerStep;
        return;
    }

    if ((action == Action::PRESS || action == Action::REPEAT) &&
        has_replay_key(m_replayOptions.decreaseEntriesPerStepKeys, key) && m_replayOptions.entriesPerStep > 1) {
        --m_replayOptions.entriesPerStep;
    }
}

bool Context::has_replay_key(const std::vector<Key>& keys, Key key) {
    return std::find(keys.begin(), keys.end(), key) != keys.end();
}

void Context::start_replay(std::vector<GeoQikLogEntry> entries, const ReplayOptions& options) {
    cancel_replay();
    remove_all_geometry();
    m_idempotencySet.clear();
    m_messageLog = entries;
    m_replayEntries = std::move(entries);
    m_replayUndoStack.clear();
    m_replayEntryIndex = 0;
    m_replayEntryBudget = 0.0;
    m_replayOptions = options;
    m_isReplayActive = !m_replayEntries.empty();
    m_isReplayPaused = options.startPaused;
    m_lastReplayTick = std::chrono::high_resolution_clock::now();
    m_baseEntriesPerSecond = m_replayOptions.entriesPerSecond;
    m_currentSpeedMultiplier = 1.0;
    m_isReplayBackward = false;
}

void Context::process_replay_entries(const std::chrono::high_resolution_clock::time_point& now) {
    if (!is_replaying()) {
        replay_cancel_requested_storage().store(false, std::memory_order_release);
        m_lastReplayTick = now;
        return;
    }

    if (replay_cancel_requested_storage().exchange(false, std::memory_order_acq_rel)) {
        cancel_replay();
        m_lastReplayTick = now;
        return;
    }

    if (m_isReplayPaused) {
        m_lastReplayTick = now;
        return;
    }

    if (m_isReplayBackward) {
        if (m_replayEntryIndex == 0) {
            m_isReplayPaused = true;
            m_lastReplayTick = now;
            return;
        }

        const std::chrono::duration<double> elapsed = now - m_lastReplayTick;
        m_lastReplayTick = now;
        m_replayEntryBudget += elapsed.count() * m_replayOptions.entriesPerSecond;

        auto entriesToUndo = static_cast<std::size_t>(m_replayEntryBudget);
        entriesToUndo = std::min(entriesToUndo, m_replayOptions.maxEntriesPerFrame);
        undo_replay_entries(entriesToUndo);
        if (m_replayEntryBudget >= 1.0) {
            m_replayEntryBudget -= static_cast<double>(entriesToUndo);
        }
        return;
    }

    if (m_replayEntryIndex >= m_replayEntries.size()) {
        m_isReplayPaused = true;
        m_replayEntryBudget = 0.0;
        m_lastReplayTick = now;
        return;
    }

    const std::chrono::duration<double> elapsed = now - m_lastReplayTick;
    m_lastReplayTick = now;
    m_replayEntryBudget += elapsed.count() * m_replayOptions.entriesPerSecond;

    auto entriesToApply = static_cast<std::size_t>(m_replayEntryBudget);
    entriesToApply = std::min(entriesToApply, m_replayOptions.maxEntriesPerFrame);

    apply_replay_entries(entriesToApply);

    if (is_replaying() && m_replayEntryIndex >= m_replayEntries.size()) {
        m_isReplayPaused = true;
        m_replayEntryBudget = 0.0;
    }
}

void Context::apply_replay_entries(std::size_t entriesToApply) {
    std::size_t appliedEntryCount = 0;
    for (std::size_t i = 0; i < entriesToApply && is_replaying() && m_replayEntryIndex < m_replayEntries.size(); ++i) {
        m_replayUndoStack.push_back(create_replay_undo_frame(m_replayEntries[m_replayEntryIndex]));
        apply_log_entry(m_replayEntries[m_replayEntryIndex]);
        ++m_replayEntryIndex;
        ++appliedEntryCount;
        if (m_replayEntryBudget >= 1.0) {
            m_replayEntryBudget -= 1.0;
        }
    }

    m_geometryMessagesProcessedThisFrame += appliedEntryCount;

    if (is_replaying() && m_replayEntryIndex >= m_replayEntries.size()) {
        m_isReplayPaused = true;
        m_replayEntryBudget = 0.0;
    }
}

void Context::undo_replay_entries(std::size_t entriesToUndo) {
    for (std::size_t i = 0; i < entriesToUndo && is_replaying() && m_replayEntryIndex > 0 && !m_replayUndoStack.empty();
         ++i) {
        --m_replayEntryIndex;
        restore_replay_undo_frame(m_replayUndoStack.back());
        m_replayUndoStack.pop_back();
    }

    m_geometryMessagesProcessedThisFrame += entriesToUndo;
}

ReplayUndoFrame Context::create_replay_undo_frame(const GeoQikLogEntry& entry) const {
    return geoqik::create_replay_undo_frame(entry, ReplayUndoContext{m_scene, m_idempotencySet});
}

void Context::restore_replay_undo_frame(const ReplayUndoFrame& frame) {
    std::visit(
        [this](const auto& action) {
            using T = std::decay_t<decltype(action)>;
            if constexpr (std::is_same_v<T, GeoQikLogEntry>) {
                apply_log_entry(action);
            } else if constexpr (std::is_same_v<T, ReplayUndoFrame::RestoreScene>) {
                m_scene.restore_snapshot(action.scene);
                m_sceneRenderer->clear_drawables();
            }
        },
        frame.action);

    if (!frame.idempotencyKeyToErase.is_nil()) {
        m_idempotencySet.erase(IdempotencyData{frame.idempotencyKeyToErase, {}});
    }
}

void Context::finish_replay() {
    m_replayEntries.clear();
    m_replayUndoStack.clear();
    m_replayEntryIndex = 0;
    m_replayEntryBudget = 0.0;
    m_isReplayActive = false;
    m_isReplayPaused = false;
    m_isReplayBackward = false;
    m_currentSpeedMultiplier = 1.0;
}

void Context::defer_or_handle_message(GeoQikMessage&& message) {
    if (is_replaying() && !is_control_message(message)) {
        m_deferredMessages.push_back(std::move(message));
        return;
    }

    // Cleanup is a control message (see is_control_message), so it always
    // takes this branch rather than being deferred above ΓÇö drain whatever is
    // still pending before shutting down.
    if (std::holds_alternative<Cleanup>(message)) {
        cancel_replay();
        process_deferred_messages_before_cleanup();
        if (m_windowShouldClose) {
            return;
        }
    }

    process_message(message);
    if (!m_windowShouldClose && !is_replaying()) {
        process_deferred_messages();
    }
}

void Context::process_message(const GeoQikMessage& message) {
    if (auto logEntry = create_log_entry(message)) {
        m_messageLog.push_back(std::move(*logEntry));
    }

    std::visit([this](const auto& value) { handle_message(value); }, message);
}

void Context::process_one_deferred_message() {
    GeoQikMessage message = std::move(m_deferredMessages.front());
    m_deferredMessages.pop_front();
    process_message(message);
}

void Context::process_deferred_messages() {
    while (!m_deferredMessages.empty() && !is_replaying() && !m_windowShouldClose) {
        process_one_deferred_message();
    }
}

void Context::process_deferred_messages_before_cleanup() {
    while (!m_deferredMessages.empty() && !m_windowShouldClose) {
        if (is_replaying()) {
            cancel_replay();
        }
        process_one_deferred_message();
    }
}

bool Context::is_known_idempotency_key(const core::UUID* key) {
    if (key != nullptr && !key->is_nil()) {
        IdempotencyData idempotencyData{*key, std::chrono::high_resolution_clock::now()};
        auto [it, inserted] = m_idempotencySet.insert(idempotencyData);
        if (!inserted) {
            return true;
        }
    }
    return false;
}

void Context::replay_log_entries(const std::vector<GeoQikLogEntry>& entries) {
    for (const GeoQikLogEntry& entry: entries) {
        apply_log_entry(entry);
    }
}

void Context::apply_log_entry(const GeoQikLogEntry& entry) {
    std::visit([this](const auto& value) { handle_message(value); }, entry);
}

void Context::print_frame_info(const std::chrono::high_resolution_clock::time_point& startTime,
                               const std::chrono::high_resolution_clock::time_point& messageProcessingStartTime,
                               const std::chrono::high_resolution_clock::time_point& endTime) const {
    if (m_frameCount % frameInfoPrintInterval != 0) {
        return;
    }

    if (m_frameCount > 0) {
        fmt::print("");
    }
    fmt::print("Frame count: {}\n", m_frameCount);

    std::chrono::duration<double, std::milli> messageProcessingDuration = endTime - messageProcessingStartTime;
    fmt::print("Message processing took {:.2f} ms\n", messageProcessingDuration.count());

    std::chrono::duration<double, std::milli> frameDrawingTime = messageProcessingStartTime - startTime;
    fmt::print("Frame drawing took {:.2f} ms\n", frameDrawingTime.count());

    std::chrono::duration<double, std::milli> totalFrameTime = endTime - startTime;
    fmt::print("Total frame took {:.2f} ms\n", totalFrameTime.count());
}

} // namespace geoqik

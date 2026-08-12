#include "Context.hpp"

#include "Core/FmtIncludeHelper.hpp"
#include "GeoQikMessages.hpp"
#include "GeoQikOverlay.hpp"
#include "GeoQikUserSettings.hpp"
#include "Video/FfmpegLocator.hpp"

#include <Core/Assert.hpp>

#include <plinth/CameraAutoFit.hpp>
#include <plinth/CameraProjectionType.hpp>
#include <plinth/FrameState.hpp>
#include <plinth/IOverlay.hpp>
#include <plinth/Renderer.hpp>
#include <plinth/WindowSettings.hpp>

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

namespace geoqik {

using renderer::CameraAutoFitSettings;
using renderer::Viewport;

namespace {

constexpr std::size_t lineCoordinateCount = 6;
constexpr std::size_t frameInfoPrintInterval = 10;
constexpr int defaultVideoFps = 60;
constexpr double defaultEntriesPerSecond = 60.0;
constexpr double minimumEntriesPerFrame = 1e-6;

/// RAII guard that temporarily resizes the GLFW window and restores its original size on
/// destruction (covering normal and exceptional exits). Used to render a log to video at a
/// resolution preset different from the current window without any offscreen-render support.
class ScopedWindowSize {
  public:
    explicit ScopedWindowSize(renderer::Renderer& renderer)
        : m_window(static_cast<GLFWwindow*>(renderer.window().get_native_handle())) {
        if (m_window != nullptr) {
            glfwGetWindowSize(m_window, &m_originalWidth, &m_originalHeight);
        }
    }

    ScopedWindowSize(const ScopedWindowSize&) = delete;
    ScopedWindowSize& operator=(const ScopedWindowSize&) = delete;
    ScopedWindowSize(ScopedWindowSize&&) = delete;
    ScopedWindowSize& operator=(ScopedWindowSize&&) = delete;

    ~ScopedWindowSize() {
        if (m_window != nullptr && m_resized) {
            glfwSetWindowSize(m_window, m_originalWidth, m_originalHeight);
            // Pump the resize so the framebuffer/scene targets return to the original size.
            renderer::Renderer::poll_events();
        }
    }

    void resize_to(int width, int height) {
        if (m_window == nullptr || (width == m_originalWidth && height == m_originalHeight)) {
            return;
        }
        glfwSetWindowSize(m_window, width, height);
        m_resized = true;
        // Process the resize event so plinth rebuilds its scene targets before we render.
        renderer::Renderer::poll_events();
    }

  private:
    GLFWwindow* m_window{nullptr};
    int m_originalWidth{0};
    int m_originalHeight{0};
    bool m_resized{false};
};

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

bool is_existing_regular_file(const std::filesystem::path& path) {
    std::error_code ec;
    return std::filesystem::is_regular_file(path, ec);
}

std::string path_to_utf8(const std::filesystem::path& path) {
    const std::u8string value = path.u8string();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return {reinterpret_cast<const char*>(value.data()), value.size()};
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
    m_overlay->set_ui_mode(renderer::UiMode::Release);
    m_renderer->set_overlay(m_overlay);

    m_sceneRenderer = std::make_unique<GeoQikSceneRenderer>(*m_renderer);

    setup_window_callbacks();

    // Load persisted recording preferences and resolve an ffmpeg executable (empty if none found).
    try {
        const UserSettings userSettings = load_user_settings(user_settings_file_path());
        m_recordingDirectory =
            userSettings.recordingDirectory.empty() ? default_recording_directory() : userSettings.recordingDirectory;
        set_ffmpeg_path(userSettings.ffmpegPath);
    } catch (...) {
        m_recordingDirectory = default_recording_directory();
        set_ffmpeg_path({});
    }

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

void Context::render_single_frame() {
    const renderer::ClearColor clearColor{m_backgroundColor[0],
                                          m_backgroundColor[1],
                                          m_backgroundColor[2],
                                          m_backgroundColor[3]};
    m_renderer->begin_frame(clearColor);

    m_sceneRenderer->sync_scene(m_scene);

    renderer::LightingConfig lighting;
    lighting.lightColor = scale_rgb(m_geoqikSettings.meshHeadLightColor, m_geoqikSettings.meshHeadLightIntensity);
    lighting.fillLightDir = to_float3(m_geoqikSettings.meshFillLightDirection);
    lighting.fillLightColor = scale_rgb(m_geoqikSettings.meshFillLightColor, m_geoqikSettings.meshFillLightIntensity);
    lighting.ambientColor = scale_rgb(m_geoqikSettings.meshAmbientColor, m_geoqikSettings.meshAmbientIntensity);
    lighting.shininess = std::max(0.0F, m_geoqikSettings.meshShininess);

    m_renderer->draw(lighting);
    populate_replay_gui_state(m_overlay->replay_state());
    populate_video_gui_state(m_overlay->video_state());
    auto& cameraState = m_overlay->camera_state();
    populate_camera_gui_state(cameraState);
    cameraState.autoZoom = m_geoqikSettings.autoFitCameraEnabled;

    // The overlay builds and renders the complete UI inside end_frame. Apply widget commands
    // after rendering has completed.
    bool autoFitEnabled = m_geoqikSettings.autoFitCameraEnabled;
    m_renderer->end_frame(autoFitEnabled);
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

        render_single_frame();

        // Capture the just-presented frame from the front buffer (end_frame has already swapped).
        if (m_recorder.is_recording()) {
            m_recorder.capture_frame();
        }

        consume_camera_gui_commands(m_overlay->camera_state());
        consume_replay_gui_commands(m_overlay->replay_state());
        consume_file_gui_commands(m_overlay->file_state());
        consume_video_gui_commands(m_overlay->video_state());

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
    state.isMaxSpeed = m_isReplayMaxSpeed;
    state.entriesPerStep = m_replayOptions.entriesPerStep;
    state.pauseKeysLabel = key_labels(m_replayOptions.pauseKeys);
    state.resumeKeysLabel = key_labels(m_replayOptions.resumeKeys);
    state.stepForwardKeysLabel = key_labels(m_replayOptions.stepKeys);
    state.stepBackwardKeysLabel = key_labels(m_replayOptions.backwardStepKeys);
    state.increaseStepKeysLabel = key_labels(m_replayOptions.increaseEntriesPerStepKeys);
    state.decreaseStepKeysLabel = key_labels(m_replayOptions.decreaseEntriesPerStepKeys);
}

void Context::consume_replay_gui_commands(ReplayGuiState& state) {
    // The overlay state persists across frames, so drain these one-shot requests
    // as they are consumed - otherwise a lingering command (e.g. Play) re-runs every frame,
    // repeatedly zeroing m_replayEntryBudget / m_lastReplayTick so the budget never accumulates and
    // playback never advances. Mirrors how consume_camera_gui_commands resets its optionals.
    const ReplayGuiState::Command command = std::exchange(state.command, ReplayGuiState::Command::None);
    const std::optional<double> requestedSpeedMultiplier = std::exchange(state.requestedSpeedMultiplier, std::nullopt);
    const bool requestMaxSpeed = std::exchange(state.requestMaxSpeed, false);
    const std::optional<std::size_t> requestedEntriesPerStep =
        std::exchange(state.requestedEntriesPerStep, std::nullopt);
    const std::optional<std::size_t> requestedEntry = std::exchange(state.requestedEntry, std::nullopt);

    if (!is_replaying()) {
        return;
    }

    if (requestedSpeedMultiplier.has_value()) {
        m_isReplayMaxSpeed = false;
        m_currentSpeedMultiplier = *requestedSpeedMultiplier;
        m_replayOptions.entriesPerSecond = m_baseEntriesPerSecond * m_currentSpeedMultiplier;
        m_replayEntryBudget = 0.0;
    } else if (requestMaxSpeed) {
        m_isReplayMaxSpeed = true;
        m_replayEntryBudget = 0.0;
    }

    if (requestedEntriesPerStep.has_value()) {
        m_replayOptions.entriesPerStep = *requestedEntriesPerStep;
    }

    if (requestedEntry.has_value()) {
        const std::size_t targetEntry = std::min(*requestedEntry, m_replayEntries.size());
        m_isReplayPaused = true;
        m_replayEntryBudget = 0.0;
        if (targetEntry > m_replayEntryIndex) {
            apply_replay_entries(targetEntry - m_replayEntryIndex);
        } else if (targetEntry < m_replayEntryIndex) {
            undo_replay_entries(m_replayEntryIndex - targetEntry);
        }
    }

    const auto skipToEnd = [this]() {
        m_isReplayBackward = false;
        m_isReplayPaused = true;
        const std::size_t remaining = m_replayEntries.size() - m_replayEntryIndex;
        if (remaining > 0) {
            apply_replay_entries(remaining);
        }
    };

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

    case ReplayGuiState::Command::SkipToEnd: skipToEnd(); break;

    case ReplayGuiState::Command::EndReplay:
        skipToEnd();
        finish_replay();
        break;

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
    }
}

void Context::consume_camera_gui_commands(CameraGuiState& state) {
    // The widget callbacks ran during the overlay's render() (inside end_frame), so their edits are
    // now visible. Apply them here and clear the one-shot requests so they are not re-applied every
    // frame (the overlay state is persistent, not a per-frame local).

    // Auto Fit: persist the checkbox into the settings; it takes effect from the next frame.
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
    // camera immediately and unconditionally - it does not depend on the persistent Auto Fit setting
    // and is not suppressed right after a user camera interaction, so Fit scene always acts.
    m_renderer->refit_current_view();
}

void Context::consume_file_gui_commands(FileGuiState& state) {
    const FileGuiState::Command command = std::exchange(state.command, FileGuiState::Command::None);
    if (command == FileGuiState::Command::None) {
        return;
    }

    if (command == FileGuiState::Command::SetDefaultDirectory) {
        try {
            // Preserve the other persisted fields (ffmpeg path, recording directory).
            UserSettings settings = load_user_settings(state.settingsFilePath);
            settings.defaultLogDirectory = state.requestedPath;
            save_user_settings(state.settingsFilePath, settings);
            state.defaultLogDirectory = state.requestedPath;
        } catch (const std::exception& exception) {
            state.errorMessage = fmt::format("Could not save the default log directory: {}", exception.what());
            state.openErrorPopup = true;
        }
        return;
    }

    geoqik_error_code_t result = GEOQIK_SUCCESS;
    switch (command) {
    case FileGuiState::Command::Save: result = save_log_path(state.requestedPath, state.requestedFormat); break;
    case FileGuiState::Command::Load: result = load_log_path(state.requestedPath, state.requestedFormat); break;
    case FileGuiState::Command::Replay:
        result = replay_log_path(state.requestedPath, state.requestedFormat, ReplayOptions{});
        break;
    case FileGuiState::Command::None:
    case FileGuiState::Command::SetDefaultDirectory: break;
    }
    if (result != GEOQIK_SUCCESS) {
        const char* operation = "load";
        if (command == FileGuiState::Command::Save) {
            operation = "save";
        } else if (command == FileGuiState::Command::Replay) {
            operation = "replay";
        }
        state.errorMessage = fmt::format("Could not {} log '{}'. Error code: {}",
                                         operation,
                                         path_to_utf8(state.requestedPath),
                                         static_cast<int>(result));
        state.openErrorPopup = true;
    }
}

namespace {

[[nodiscard]] video::VideoFormat to_video_format(VideoGuiState::Format format) {
    switch (format) {
    case VideoGuiState::Format::Mp4: return video::VideoFormat::Mp4;
    case VideoGuiState::Format::WebM: return video::VideoFormat::WebM;
    case VideoGuiState::Format::Gif: return video::VideoFormat::Gif;
    case VideoGuiState::Format::PngSequence: return video::VideoFormat::PngSequence;
    }
    return video::VideoFormat::Mp4;
}

[[nodiscard]] video::VideoQuality to_video_quality(VideoGuiState::Quality quality) {
    switch (quality) {
    case VideoGuiState::Quality::High: return video::VideoQuality::High;
    case VideoGuiState::Quality::Medium: return video::VideoQuality::Medium;
    case VideoGuiState::Quality::Low: return video::VideoQuality::Low;
    case VideoGuiState::Quality::Lossless: return video::VideoQuality::Lossless;
    }
    return video::VideoQuality::High;
}

/// Maps a recording failure to a short, actionable message. Kept out of the UI layer so the same
/// wording is reused for the live-record, stop, and log->video paths. @p action names the operation
/// (e.g. "start recording") for the generic fallback.
[[nodiscard]] std::string recording_error_message(geoqik_error_code_t code, std::string_view action) {
    switch (code) {
    case GEOQIK_ERROR_UNSUPPORTED_FORMAT:
        return "ffmpeg was not found. Set ffmpeg.exe under Record \xE2\x96\xB8 Set ffmpeg.exe Path\xE2\x80\xA6";
    case GEOQIK_ERROR_IO:
        return "Could not write the video file. Check the recording folder is writable and has free space.";
    case GEOQIK_ERROR_INVALID_STATE:
        return "A recording is already in progress.";
    case GEOQIK_ERROR_INVALID_PARAMETER:
        return "Invalid recording settings. Check the resolution and frame rate.";
    default:
        return fmt::format("Could not {} (code {}).", action, static_cast<int>(code));
    }
}

/// Opens a folder / selects a file in the platform file browser so the user can find the output.
void reveal_in_file_browser(const std::filesystem::path& path) {
    if (path.empty()) {
        return;
    }
#ifdef _WIN32
    // "explorer /select,<file>" highlights the file; for a directory it simply opens it.
    std::error_code error;
    const bool isDirectory = std::filesystem::is_directory(path, error);
    const std::string command =
        isDirectory ? fmt::format("explorer \"{}\"", path.string())
                    : fmt::format("explorer /select,\"{}\"", path.string());
    (void)std::system(command.c_str());
#elif defined(__APPLE__)
    (void)std::system(fmt::format("open \"{}\"", path.string()).c_str());
#else
    (void)std::system(fmt::format("xdg-open \"{}\"", path.string()).c_str());
#endif
}

} // namespace

void Context::populate_video_gui_state(VideoGuiState& state) const {
    state.isRecording = m_recorder.is_recording();
    state.ffmpegAvailable = m_ffmpegAvailable;
    state.ffmpegPath = m_ffmpegPath;
    state.recordingDirectory = m_recordingDirectory;
    if (m_recorder.is_recording()) {
        state.width = m_recorder.width();
        state.height = m_recorder.height();
        state.fps = m_recorder.fps();
        state.elapsedSeconds =
            std::chrono::duration<double>(m_recorder.elapsed()).count();
    }
}

video::VideoRecordOptions Context::make_record_options_from_gui(const VideoGuiState& state) const {
    video::VideoRecordOptions options;
    options.format = to_video_format(state.requestedFormat);
    options.quality = to_video_quality(state.quality);
    options.fps = state.requestedFps > 0 ? state.requestedFps : defaultVideoFps;
    // Resolution preset: {0,0} means current window size, resolved by the recorder.
    const auto [presetWidth, presetHeight] = video_resolution_preset(state.resolutionPresetIndex);
    options.width = presetWidth;
    options.height = presetHeight;
    options.recordingDirectory = m_recordingDirectory;

    // Offline pacing/holds (ignored by live recording).
    options.pacingMode =
        state.pacing == VideoGuiState::Pacing::Duration ? video::PacingMode::Duration : video::PacingMode::Speed;
    options.entriesPerSecond =
        state.entriesPerSecond > 0.0F ? static_cast<double>(state.entriesPerSecond) : defaultEntriesPerSecond;
    options.targetDurationSeconds = static_cast<double>(state.targetDurationSeconds);
    options.holdStartSeconds = static_cast<double>(state.holdStartSeconds);
    options.holdEndSeconds = static_cast<double>(state.holdEndSeconds);
    return options;
}

void Context::consume_video_gui_commands(VideoGuiState& state) {
    const VideoGuiState::Command command = std::exchange(state.command, VideoGuiState::Command::None);

    switch (command) {
    case VideoGuiState::Command::None:
        break;
    case VideoGuiState::Command::StartRecording: {
        const geoqik_error_code_t result = start_recording(make_record_options_from_gui(state));
        if (result == GEOQIK_SUCCESS) {
            state.set_status(VideoGuiState::StatusKind::Info, "Recording\xE2\x80\xA6");
        } else {
            state.set_status(VideoGuiState::StatusKind::Error, recording_error_message(result, "start recording"));
        }
        break;
    }
    case VideoGuiState::Command::StopRecording: {
        const std::filesystem::path output = m_recorder.final_output_path();
        const geoqik_error_code_t result = stop_recording();
        if (result == GEOQIK_SUCCESS) {
            state.lastOutputPath = output;
            state.set_status(VideoGuiState::StatusKind::Success,
                             fmt::format("Saved {}", path_to_utf8(output.filename())));
        } else {
            state.set_status(VideoGuiState::StatusKind::Error, recording_error_message(result, "finish recording"));
        }
        break;
    }
    case VideoGuiState::Command::RenderLogToVideo: {
        video::VideoRecordOptions options = make_record_options_from_gui(state);
        const geoqik_error_code_t result =
            render_log_to_video_path(state.requestedPath, state.requestedLogFormat, options);
        if (result == GEOQIK_SUCCESS) {
            state.lastOutputPath = m_recorder.final_output_path();
            state.set_status(VideoGuiState::StatusKind::Success,
                             fmt::format("Saved {}", path_to_utf8(state.lastOutputPath.filename())));
        } else {
            state.set_status(VideoGuiState::StatusKind::Error, recording_error_message(result, "render log to video"));
        }
        break;
    }
    case VideoGuiState::Command::SetFfmpegPath:
        set_ffmpeg_path(state.requestedPath);
        persist_recording_settings();
        break;
    case VideoGuiState::Command::SetRecordingDirectory:
        m_recordingDirectory = state.requestedPath;
        persist_recording_settings();
        break;
    case VideoGuiState::Command::RevealLastOutput:
        reveal_in_file_browser(state.lastOutputPath);
        break;
    case VideoGuiState::Command::OpenRecordingDirectory:
        reveal_in_file_browser(m_recordingDirectory);
        break;
    }
}

void Context::persist_recording_settings() {
    try {
        const std::filesystem::path settingsPath = user_settings_file_path();
        UserSettings settings = load_user_settings(settingsPath);
        settings.ffmpegPath = m_ffmpegPath;
        settings.recordingDirectory = m_recordingDirectory;
        save_user_settings(settingsPath, settings);
    } catch (...) {
        // Persisting preferences is best-effort; a failure must not disrupt recording.
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
    if (m_overlay) {
        m_overlay->shutdown_native_dialogs();
        m_renderer->set_overlay(nullptr);
        m_overlay.reset();
    }
    m_sceneRenderer.reset();
    m_renderer.reset();

    return true;
}

geoqik_error_code_t Context::save_log(const char* path, geoqik_log_format_t format) const {
    if (path == nullptr || path[0] == '\0' ||
        (format != GEOQIK_LOG_FORMAT_BINARY && format != GEOQIK_LOG_FORMAT_JSON)) {
        return GEOQIK_ERROR_INVALID_PARAMETER;
    }

    return save_log_path(std::filesystem::path{path}, format);
}

geoqik_error_code_t Context::save_log_path(const std::filesystem::path& path, geoqik_log_format_t format) const {
    if (path.empty() || (format != GEOQIK_LOG_FORMAT_BINARY && format != GEOQIK_LOG_FORMAT_JSON)) {
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

    return load_log_path(std::filesystem::path{path}, format);
}

geoqik_error_code_t Context::load_log_path(const std::filesystem::path& path, geoqik_log_format_t format) {
    if (path.empty() || (format != GEOQIK_LOG_FORMAT_BINARY && format != GEOQIK_LOG_FORMAT_JSON)) {
        return GEOQIK_ERROR_INVALID_PARAMETER;
    }

    try {
        if (!is_existing_regular_file(path)) {
            return GEOQIK_ERROR_UNKNOWN;
        }

        std::vector<GeoQikLogEntry> loadedEntries =
            format == GEOQIK_LOG_FORMAT_JSON ? load_log_json(path) : load_log_binary(path);
        // A successful load replaces the replayed session. Apply any live messages that arrived
        // during replay afterwards so switching logs cannot discard them.
        cancel_replay();
        remove_all_geometry();
        m_idempotencySet.clear();
        replay_log_entries(loadedEntries);
        m_messageLog = std::move(loadedEntries);
        process_deferred_messages();
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

    return replay_log_path(std::filesystem::path{path}, format, options);
}

geoqik_error_code_t
Context::replay_log_path(const std::filesystem::path& path, geoqik_log_format_t format, const ReplayOptions& options) {
    if (path.empty() || (format != GEOQIK_LOG_FORMAT_BINARY && format != GEOQIK_LOG_FORMAT_JSON)) {
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

void Context::set_ffmpeg_path(const std::filesystem::path& path) {
    m_ffmpegPath = video::FfmpegLocator::find(path).value_or(std::filesystem::path{});
    m_ffmpegAvailable = !m_ffmpegPath.empty();
}

geoqik_error_code_t Context::start_recording(const video::VideoRecordOptions& options) {
    if (m_recorder.is_recording() || is_replaying()) {
        return GEOQIK_ERROR_INVALID_PARAMETER;
    }
    if (video::format_requires_ffmpeg(options.format) && !m_ffmpegAvailable) {
        return GEOQIK_ERROR_INVALID_PARAMETER;
    }

    try {
        video::VideoRecordOptions opts = options;
        if (opts.recordingDirectory.empty()) {
            opts.recordingDirectory = m_recordingDirectory;
        }
        if (video::format_requires_ffmpeg(opts.format)) {
            opts.ffmpegExecutable = m_ffmpegPath;
        }

        m_renderer->window().make_context_current();
        const auto [fbWidth, fbHeight] = m_renderer->window().get_framebuffer_size();
        if (!m_recorder.start(opts, fbWidth, fbHeight)) {
            return GEOQIK_ERROR_UNKNOWN;
        }
        return GEOQIK_SUCCESS;
    } catch (const std::bad_alloc&) {
        return GEOQIK_ERROR_MEMORY_ALLOCATION;
    } catch (...) {
        return GEOQIK_ERROR_UNKNOWN;
    }
}

geoqik_error_code_t Context::stop_recording() {
    if (!m_recorder.is_recording()) {
        return GEOQIK_ERROR_INVALID_PARAMETER;
    }
    return m_recorder.stop() ? GEOQIK_SUCCESS : GEOQIK_ERROR_UNKNOWN;
}

geoqik_error_code_t Context::render_log_to_video(const char* logPath,
                                                 geoqik_log_format_t format,
                                                 const video::VideoRecordOptions& options) {
    if (logPath == nullptr || logPath[0] == '\0' ||
        (format != GEOQIK_LOG_FORMAT_BINARY && format != GEOQIK_LOG_FORMAT_JSON)) {
        return GEOQIK_ERROR_INVALID_PARAMETER;
    }
    return render_log_to_video_path(std::filesystem::path{logPath}, format, options);
}

geoqik_error_code_t Context::render_log_to_video_path(const std::filesystem::path& logPath,
                                                      geoqik_log_format_t format,
                                                      const video::VideoRecordOptions& options) {
    if (m_recorder.is_recording()) {
        return GEOQIK_ERROR_INVALID_PARAMETER;
    }
    if (video::format_requires_ffmpeg(options.format) && !m_ffmpegAvailable) {
        return GEOQIK_ERROR_INVALID_PARAMETER;
    }

    // A resolution preset that differs from the current window is produced by temporarily resizing
    // the GLFW window to the target size for the duration of the render, then restoring it. This
    // needs no offscreen-render support from plinth. The guard restores the size on every exit.
    ScopedWindowSize windowSizeGuard{*m_renderer};

    try {
        if (!is_existing_regular_file(logPath)) {
            return GEOQIK_ERROR_UNKNOWN;
        }

        std::vector<GeoQikLogEntry> loadedEntries =
            format == GEOQIK_LOG_FORMAT_JSON ? load_log_json(logPath) : load_log_binary(logPath);

        // Deterministic offline render: apply the log paced by options, rendering and capturing a
        // frame per step so the resulting video is smooth and machine-speed independent.
        ReplayOptions replayOptions;
        replayOptions.startPaused = true; // We drive stepping manually below.
        start_replay(loadedEntries, replayOptions);

        video::VideoRecordOptions opts = options;
        if (opts.recordingDirectory.empty()) {
            opts.recordingDirectory = m_recordingDirectory;
        }
        if (video::format_requires_ffmpeg(opts.format)) {
            opts.ffmpegExecutable = m_ffmpegPath;
        }

        m_renderer->window().make_context_current();
        // Apply the requested resolution by resizing the window; the framebuffer-size callback
        // rebuilds the scene targets. Pump one frame so the framebuffer reflects the new size
        // before we start capturing.
        if (opts.width > 0 && opts.height > 0) {
            windowSizeGuard.resize_to(opts.width, opts.height);
            render_single_frame();
        }
        const auto [fbWidth, fbHeight] = m_renderer->window().get_framebuffer_size();
        if (!m_recorder.start(opts, fbWidth, fbHeight)) {
            cancel_replay();
            return GEOQIK_ERROR_UNKNOWN;
        }

        render_log_frames(opts);

        finish_replay();
        const bool ok = m_recorder.stop();
        return ok ? GEOQIK_SUCCESS : GEOQIK_ERROR_UNKNOWN;
    } catch (const std::bad_alloc&) {
        if (m_recorder.is_recording()) {
            (void)m_recorder.stop();
        }
        return GEOQIK_ERROR_MEMORY_ALLOCATION;
    } catch (...) {
        if (m_recorder.is_recording()) {
            (void)m_recorder.stop();
        }
        return GEOQIK_ERROR_UNKNOWN;
    }
}

void Context::render_log_frames(const video::VideoRecordOptions& options) {
    const int fps = options.fps > 0 ? options.fps : defaultVideoFps;
    const std::size_t entryCount = m_replayEntries.size();

    // Render the initial (empty) state and hold it for hold-at-start.
    render_single_frame();
    m_recorder.capture_frame();
    const auto secondsToExtraFrames = [fps](double seconds) -> std::size_t {
        return seconds > 0.0 ? static_cast<std::size_t>(std::llround(seconds * fps)) : 0;
    };
    m_recorder.hold_last_frame(secondsToExtraFrames(options.holdStartSeconds));

    if (entryCount > 0) {
        // How many entries to advance per captured frame. Speed sets it directly; Duration derives
        // it so all entries land within the target time. A fractional accumulator carries the
        // remainder across frames so pacing stays exact over the whole video.
        double entriesPerFrame = 0.0;
        if (options.pacingMode == video::PacingMode::Duration && options.targetDurationSeconds > 0.0) {
            const double bodyFrames = std::max(1.0, std::round(options.targetDurationSeconds * fps));
            entriesPerFrame = static_cast<double>(entryCount) / bodyFrames;
        } else {
            const double entriesPerSecond =
                options.entriesPerSecond > 0.0 ? options.entriesPerSecond : defaultEntriesPerSecond;
            entriesPerFrame = entriesPerSecond / fps;
        }
        entriesPerFrame = std::max(entriesPerFrame, minimumEntriesPerFrame); // Never stall.

        double accumulator = 0.0;
        while (m_replayEntryIndex < m_replayEntries.size()) {
            accumulator += entriesPerFrame;
            auto entriesThisFrame = static_cast<std::size_t>(accumulator);
            if (entriesThisFrame == 0) {
                // Slow pacing (<1 entry/frame): the frame holds the current state; keep accumulating.
                render_single_frame();
                m_recorder.capture_frame();
                continue;
            }
            accumulator -= static_cast<double>(entriesThisFrame);
            apply_replay_entries(entriesThisFrame);
            render_single_frame();
            m_recorder.capture_frame();
        }
    }

    // Freeze the final frame for hold-at-end.
    m_recorder.hold_last_frame(seconds_to_extra_frames(options.holdEndSeconds));
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
    m_isReplayMaxSpeed = false;
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

void Context::handle_message(const VideoCommand& message) {
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
           std::holds_alternative<GetReplayProgress>(message) || std::holds_alternative<VideoCommand>(message);
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
        auto& ui = *m_overlay;
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
    m_isReplayMaxSpeed = false;
    // Keep m_deferredMessages intact. Live messages received after replay starts are queued there
    // and applied in order once replay mode ends, so starting a replay cannot discard them.
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
        std::size_t entriesToUndo = m_replayOptions.maxEntriesPerFrame;
        if (!m_isReplayMaxSpeed) {
            m_replayEntryBudget += elapsed.count() * m_replayOptions.entriesPerSecond;
            entriesToUndo = std::min(static_cast<std::size_t>(m_replayEntryBudget), m_replayOptions.maxEntriesPerFrame);
        }
        undo_replay_entries(entriesToUndo);
        if (!m_isReplayMaxSpeed && m_replayEntryBudget >= 1.0) {
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
    std::size_t entriesToApply = m_replayOptions.maxEntriesPerFrame;
    if (!m_isReplayMaxSpeed) {
        m_replayEntryBudget += elapsed.count() * m_replayOptions.entriesPerSecond;
        entriesToApply = std::min(static_cast<std::size_t>(m_replayEntryBudget), m_replayOptions.maxEntriesPerFrame);
    }

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
    m_isReplayMaxSpeed = false;
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

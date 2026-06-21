#include "Context.hpp"
#include "Core/FmtIncludeHelper.hpp"
#include "GeoQikMessages.hpp"
#include <Core/Assert.hpp>
#include <OpenGL/FrameState.hpp>
#include <Renderer/CameraAutoFit.hpp>
#include <Renderer/Renderer.hpp>
#include <algorithm>
#include <array>
#include <filesystem>
#include <system_error>
#include <type_traits>
#include <utility>

namespace geoqik
{

using renderer::CameraAutoFitSettings;
using renderer::CameraAutoFitInput;
using renderer::CameraAutoFitResult;
using renderer::Viewport;
using renderer::CameraInteractor;

namespace
{

constexpr std::size_t lineCoordinateCount = 6;
constexpr std::size_t frameInfoPrintInterval = 10;

ConcurrentQueue<GeoQikMessage>& message_queue_storage()
{
  static ConcurrentQueue<GeoQikMessage> messageQueue; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
  return messageQueue;
}

std::atomic<bool>& replay_cancel_requested_storage()
{
  static std::atomic<bool> replayCancelRequested{false}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
  return replayCancelRequested;
}

bool is_existing_regular_file(const char* path)
{
  std::error_code ec;
  return std::filesystem::is_regular_file(path, ec);
}

} // namespace

void init_message_queue(ConcurrentQueue<GeoQikMessage>&& messageQueue)
{
  message_queue_storage() = std::move(messageQueue);
}

ConcurrentQueue<GeoQikMessage>& get_message_queue()
{
  return message_queue_storage();
}

void request_replay_cancel()
{
  replay_cancel_requested_storage().store(true, std::memory_order_release);
}

static CameraAutoFitSettings make_camera_auto_fit_settings(const GeoQikSettings& settings)
{
  CameraAutoFitSettings autoFitSettings;
  autoFitSettings.enabled = settings.autoFitCameraEnabled;
  autoFitSettings.zoomInEnabled = settings.autoFitZoomInEnabled;
  autoFitSettings.zoomOutPadding = settings.autoFitZoomOutPadding;
  autoFitSettings.minViewportOccupancy = settings.autoFitMinViewportOccupancy;
  autoFitSettings.targetViewportOccupancy = settings.autoFitTargetViewportOccupancy;
  autoFitSettings.suppressAfterUserCameraInteraction = settings.autoFitSuppressAfterUserCameraInteraction;
  return autoFitSettings;
}

static CameraAutoFitInput make_camera_auto_fit_input(const CameraInteractor& cameraInteractor,
                                                     const GeoQikSettings& settings,
                                                     bool suppressZoomIn)
{
  const auto orthographicParams = cameraInteractor.get_orthographic_params();
  CameraAutoFitInput input;
  input.position = cameraInteractor.get_position();
  input.target = cameraInteractor.get_target();
  input.vertical = cameraInteractor.get_vertical();
  input.projectionType = cameraInteractor.get_projection_type();
  input.verticalFovDegrees = cameraInteractor.get_fov();
  input.orthographicWidth = orthographicParams.width;
  input.orthographicHeight = orthographicParams.height;
  input.aspectRatio = cameraInteractor.get_viewport().get_aspect_ratio();
  input.nearPlane = cameraInteractor.get_near_plane();
  input.farPlaneMultiplier = settings.cameraFarPlaneMultiplier;
  input.suppressZoomIn = suppressZoomIn;
  input.settings = make_camera_auto_fit_settings(settings);
  return input;
}

static linal::float3 scale_rgb(const std::array<float, 3>& color, float intensity)
{
  const float clampedIntensity = std::max(0.0F, intensity);
  return linal::float3{color[0] * clampedIntensity, color[1] * clampedIntensity, color[2] * clampedIntensity};
}

static linal::float3 to_float3(const std::array<float, 3>& values)
{
  return linal::float3{values[0], values[1], values[2]};
}

Context::Context() = default;

Context::~Context()
{
  if (m_renderer)
  {
    cleanup();
  }
}

bool Context::init_window(const GeoQikSettings& geoqikSettings, const WindowSettings& settings)
{
  if (m_renderer)
  {
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

  m_renderer = renderer::Renderer::create(settings);
  if (!m_renderer)
  {
    return false;
  }

  m_sceneRenderer = std::make_unique<GeoQikSceneRenderer>(*m_renderer);

  setup_window_callbacks();

  return true;
}

void Context::setup_window_callbacks()
{
  m_renderer->add_key_callback(
      [this](Key key, Scancode scancode, Action action, Mods mods)
      {
        on_key(key, scancode, action, mods);
      });
}

float Context::get_point_size()
{
  return m_scene.get_point_size();
}

void Context::set_point_size(float pointSize)
{
  m_scene.set_point_size(pointSize);
  m_sceneRenderer->clear_drawables();
  ++m_geometryMessagesProcessedThisFrame;
}

Color Context::get_point_color()
{
  return m_scene.get_default_point_color();
}

void Context::set_default_point_color(Color color)
{
  m_scene.set_default_point_color(color[0], color[1], color[2], color[3]);
  ++m_geometryMessagesProcessedThisFrame;
}

float Context::get_line_width()
{
  return m_scene.get_line_width();
}

void Context::set_line_width(float lineWidth)
{
  m_scene.set_line_width(lineWidth);
  m_sceneRenderer->clear_drawables();
  ++m_geometryMessagesProcessedThisFrame;
}

Color Context::get_line_color()
{
  return m_scene.get_default_line_color();
}

void Context::set_line_color(Color color)
{
  m_scene.set_default_line_color(color[0], color[1], color[2], color[3]);
  ++m_geometryMessagesProcessedThisFrame;
}

Color Context::get_mesh_color()
{
  return m_scene.get_default_mesh_color();
}

void Context::set_mesh_color(Color color)
{
  m_scene.set_default_mesh_color(color[0], color[1], color[2], color[3]);
  ++m_geometryMessagesProcessedThisFrame;
}

void Context::add_point_with_opts(float x, float y, float z, const GeoQikMessageCommonData& commonData)
{
  if (is_known_idempotency_key(&commonData.idempotencyId))
  {
    return;
  }
  if (m_scene.ensure_point_capacity(1))
  {
    m_sceneRenderer->recreate_point_drawables(m_scene);
  }
  if (commonData.rgba.size() >= ColorChannelCount)
  {
    m_scene.add_point(x, y, z, commonData.rgba[0], commonData.rgba[1], commonData.rgba[2], commonData.rgba[3], &commonData.geometryId);
  }
  else
  {
    m_scene.add_point(x, y, z, &commonData.geometryId);
  }
  ++m_geometryMessagesProcessedThisFrame;
}

void Context::add_points_with_opts(std::span<const float> points, const GeoQikMessageCommonData& commonData)
{
  if (is_known_idempotency_key(&commonData.idempotencyId))
  {
    return;
  }
  if (m_scene.ensure_point_capacity(points.size() / 3))
  {
    m_sceneRenderer->recreate_point_drawables(m_scene);
  }
  m_scene.add_points(points, std::span<const float>(commonData.rgba), &commonData.geometryId);
  ++m_geometryMessagesProcessedThisFrame;
}

void Context::update_point_with_opts(const core::UUID& handle, float x, float y, float z, std::span<const float> colors)
{
  if (m_scene.update_point(handle, x, y, z, colors))
  {
    ++m_geometryMessagesProcessedThisFrame;
  }
}

void Context::update_points_with_opts(const core::UUID& handle, std::span<const float> points, std::span<const float> colors)
{
  if (m_scene.update_points(handle, points, colors))
  {
    ++m_geometryMessagesProcessedThisFrame;
  }
}

void Context::remove_point(const core::UUID& handle)
{
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
                       const core::UUID* idempotencyKey)
{
  if (is_known_idempotency_key(idempotencyKey))
  {
    return;
  }
  if (m_scene.ensure_line_capacity(1))
  {
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
                       const core::UUID* idempotencyKey)
{
  if (is_known_idempotency_key(idempotencyKey))
  {
    return;
  }
  if (m_scene.ensure_line_capacity(1))
  {
    m_sceneRenderer->recreate_line_drawables(m_scene);
  }
  m_scene.add_line(x1, y1, z1, x2, y2, z2, r, g, b, a, handle);
  ++m_geometryMessagesProcessedThisFrame;
}

void Context::add_line_with_opts(float x1, float y1, float z1, float x2, float y2, float z2, const GeoQikMessageCommonData& commonData)
{
  if (is_known_idempotency_key(&commonData.idempotencyId))
  {
    return;
  }
  if (m_scene.ensure_line_capacity(1))
  {
    m_sceneRenderer->recreate_line_drawables(m_scene);
  }
  if (commonData.rgba.size() >= ColorChannelCount)
  {
    m_scene.add_line(x1, y1, z1, x2, y2, z2, commonData.rgba[0], commonData.rgba[1], commonData.rgba[2], commonData.rgba[3], &commonData.geometryId);
  }
  else
  {
    m_scene.add_line(x1, y1, z1, x2, y2, z2, &commonData.geometryId);
  }
  ++m_geometryMessagesProcessedThisFrame;
}

void Context::add_lines_with_opts(std::span<const float> lines, const GeoQikMessageCommonData& commonData)
{
  if (is_known_idempotency_key(&commonData.idempotencyId))
  {
    return;
  }
  if (m_scene.ensure_line_capacity(lines.size() / lineCoordinateCount))
  {
    m_sceneRenderer->recreate_line_drawables(m_scene);
  }
  m_scene.add_lines(lines,
                    std::span<const float>(commonData.rgba),
                    &commonData.geometryId);
  ++m_geometryMessagesProcessedThisFrame;
}

void Context::update_line_with_opts(const core::UUID& handle,
                                    float x1,
                                    float y1,
                                    float z1,
                                    float x2,
                                    float y2,
                                    float z2,
                                    std::span<const float> colors)
{
  if (m_scene.update_line(handle, x1, y1, z1, x2, y2, z2, colors))
  {
    ++m_geometryMessagesProcessedThisFrame;
  }
}

void Context::update_lines_with_opts(const core::UUID& handle, std::span<const float> lines, std::span<const float> colors)
{
  if (m_scene.update_lines(handle, lines, colors))
  {
    ++m_geometryMessagesProcessedThisFrame;
  }
}

void Context::remove_line(const core::UUID& handle)
{
  m_scene.remove_line(handle);
  ++m_geometryMessagesProcessedThisFrame;
}

void Context::remove_all_geometry()
{
  m_scene.clear();
  m_sceneRenderer->clear_drawables();
  ++m_geometryMessagesProcessedThisFrame;
}

void Context::translate_geometry(const core::UUID& handle, float dx, float dy, float dz)
{
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
                              float angle)
{
  m_scene.rotate_geometry(handle, centerX, centerY, centerZ, axisX, axisY, axisZ, angle);
  ++m_geometryMessagesProcessedThisFrame;
}

const Viewport& Context::get_viewport()
{
  return m_renderer->get_camera().lock()->get_viewport();
}

// #define PRINT_FRAME_INFO

void Context::run_event_loop()
{
  assert(m_renderer);

  auto& messageQueue = get_message_queue();
  m_lastReplayTick = std::chrono::high_resolution_clock::now();
  while (!m_windowShouldClose)
  {
    std::chrono::high_resolution_clock::time_point frameStartTime = std::chrono::high_resolution_clock::now();

    m_renderer->window().make_context_current();

    renderer::Renderer::poll_events();
    update_camera_interaction_state();
    if (should_close_event_loop())
    {
      break;
    }

    const opengl::ClearColor clearColor{m_backgroundColor[0], m_backgroundColor[1], m_backgroundColor[2], m_backgroundColor[3]};
    m_renderer->begin_frame(clearColor);

    sync_scene_and_auto_fit();

    opengl::LightingConfig lighting;
    lighting.lightColor      = scale_rgb(m_geoqikSettings.meshHeadLightColor, m_geoqikSettings.meshHeadLightIntensity);
    lighting.fillLightDir    = to_float3(m_geoqikSettings.meshFillLightDirection);
    lighting.fillLightColor  = scale_rgb(m_geoqikSettings.meshFillLightColor, m_geoqikSettings.meshFillLightIntensity);
    lighting.ambientColor    = scale_rgb(m_geoqikSettings.meshAmbientColor, m_geoqikSettings.meshAmbientIntensity);
    lighting.shininess       = std::max(0.0F, m_geoqikSettings.meshShininess);

    m_renderer->draw(lighting);
    m_renderer->end_frame(m_geoqikSettings.autoFitCameraEnabled);

    process_replay_entries(std::chrono::high_resolution_clock::now());
    if (!is_replaying() && process_deferred_messages())
    {
      return;
    }

#ifdef PRINT_FRAME_INFO
    const std::chrono::high_resolution_clock::time_point messageProcessingStartTime = std::chrono::high_resolution_clock::now();
#endif
    if (process_message_queue(messageQueue, frameStartTime))
    {
      return;
    }

#ifdef PRINT_FRAME_INFO
    std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
    print_frame_info(frameStartTime, messageProcessingStartTime, endTime);
#endif
  }
}

bool Context::should_close_event_loop()
{
  if (m_renderer->should_close())
  {
    m_windowShouldClose.store(true);
    return true;
  }

  const auto imgui = m_renderer->get_imgui().lock();
  const bool keyboardCaptured = imgui && imgui->wants_keyboard();
  if (!keyboardCaptured && m_renderer->is_escape_pressed())
  {
    m_windowShouldClose.store(true);
    return true;
  }

  return false;
}

void Context::update_camera_interaction_state()
{
  const auto camera = m_renderer->get_camera().lock();
  if (!camera || !camera->get_was_blocking())
  {
    return;
  }

  m_lastCameraInteractionTime = std::chrono::high_resolution_clock::now();
  camera->reset_was_blocking();
}

void Context::sync_scene_and_auto_fit()
{
  if (!m_sceneRenderer->sync_scene(m_scene))
  {
    return;
  }

  const auto camera = m_renderer->get_camera().lock();
  if (!camera)
  {
    return;
  }

  const auto now = std::chrono::high_resolution_clock::now();
  const bool hasRecentCameraInteraction =
      m_lastCameraInteractionTime.time_since_epoch().count() != 0 &&
      now - m_lastCameraInteractionTime < m_geoqikSettings.autoFitSuppressAfterUserCameraInteraction;

  const CameraAutoFitInput autoFitInput =
      make_camera_auto_fit_input(*camera, m_geoqikSettings, hasRecentCameraInteraction);
  const std::array<std::span<const float>, 2> vertexPositionBuffers{m_scene.get_point_buffer().get_points(), m_scene.get_line_buffer().get_lines()};
  const CameraAutoFitResult autoFitResult = renderer::calculate_camera_auto_fit(std::span<const std::span<const float>>{vertexPositionBuffers}, autoFitInput);
  if (autoFitResult.hasGeometry)
  {
    camera->apply_auto_fit_result(autoFitResult);
  }
}

bool Context::should_stop_processing_messages(const std::chrono::high_resolution_clock::time_point& frameStartTime,
                                              const std::chrono::high_resolution_clock::time_point& messageProcessingStartTime) const
{
  if (!m_isDrawing)
  {
    return false;
  }

  const auto now = std::chrono::high_resolution_clock::now();
  const auto elapsedMessageTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - messageProcessingStartTime);

  bool minimumProcessingTimeReached = true;
  if (m_geoqikSettings.minGeometryProcessingTime > std::chrono::milliseconds(0))
  {
    minimumProcessingTimeReached = elapsedMessageTime >= m_geoqikSettings.minGeometryProcessingTime;
  }

  return minimumProcessingTimeReached && (now - frameStartTime) >= m_geoqikSettings.maxFrameProcessingTime;
}

bool Context::process_message_queue(ConcurrentQueue<GeoQikMessage>& messageQueue,
                                    const std::chrono::high_resolution_clock::time_point& frameStartTime)
{
  m_geometryMessagesProcessedThisFrame = 0;
  const std::chrono::high_resolution_clock::time_point messageProcessingStartTime = std::chrono::high_resolution_clock::now();
  while (!messageQueue.empty())
  {
    if (should_stop_processing_messages(frameStartTime, messageProcessingStartTime))
    {
      return false;
    }

    std::optional<GeoQikMessage> message = messageQueue.dequeue();
    if (!message.has_value())
    {
      continue;
    }

    defer_or_handle_message(std::move(*message));
    if (m_windowShouldClose)
    {
      return true;
    }
  }

  return false;
}

bool Context::cleanup()
{
  if (!m_renderer)
  {
    fmt::print("GLFW window is not initialized.\n");
    return false;
  }

  m_renderer->window().make_context_current();
  m_sceneRenderer.reset();
  m_renderer.reset();

  return true;
}

geoqik_error_code_t Context::save_log(const char* path, geoqik_log_format_t format) const
{
  if (path == nullptr || path[0] == '\0' || format != GEOQIK_LOG_FORMAT_BINARY)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  try
  {
    save_log_binary(path, m_messageLog);
    return GEOQIK_SUCCESS;
  }
  catch (const std::bad_alloc&)
  {
    return GEOQIK_ERROR_MEMORY_ALLOCATION;
  }
  catch (...)
  {
    return GEOQIK_ERROR_UNKNOWN;
  }
}

geoqik_error_code_t Context::load_log(const char* path, geoqik_log_format_t format)
{
  if (path == nullptr || path[0] == '\0' || format != GEOQIK_LOG_FORMAT_BINARY)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  try
  {
    if (!is_existing_regular_file(path))
    {
      return GEOQIK_ERROR_UNKNOWN;
    }

    std::vector<GeoQikLogEntry> loadedEntries = load_log_binary(path);
    remove_all_geometry();
    m_idempotencySet.clear();
    replay_log_entries(loadedEntries);
    m_messageLog = std::move(loadedEntries);
    return GEOQIK_SUCCESS;
  }
  catch (const std::bad_alloc&)
  {
    return GEOQIK_ERROR_MEMORY_ALLOCATION;
  }
  catch (...)
  {
    return GEOQIK_ERROR_UNKNOWN;
  }
}

geoqik_error_code_t Context::replay_log(const char* path, geoqik_log_format_t format, const ReplayOptions& options)
{
  if (path == nullptr || path[0] == '\0' || format != GEOQIK_LOG_FORMAT_BINARY)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  try
  {
    if (!is_existing_regular_file(path))
    {
      return GEOQIK_ERROR_UNKNOWN;
    }

    std::vector<GeoQikLogEntry> loadedEntries = load_log_binary(path);
    start_replay(std::move(loadedEntries), options);
    return GEOQIK_SUCCESS;
  }
  catch (const std::bad_alloc&)
  {
    return GEOQIK_ERROR_MEMORY_ALLOCATION;
  }
  catch (...)
  {
    return GEOQIK_ERROR_UNKNOWN;
  }
}

geoqik_error_code_t Context::replay_current_log(const ReplayOptions& options)
{
  try
  {
    start_replay(m_messageLog, options);
    return GEOQIK_SUCCESS;
  }
  catch (const std::bad_alloc&)
  {
    return GEOQIK_ERROR_MEMORY_ALLOCATION;
  }
  catch (...)
  {
    return GEOQIK_ERROR_UNKNOWN;
  }
}

void Context::cancel_replay()
{
  if (!is_replaying())
  {
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
}

void Context::pause_replay()
{
  if (is_replaying())
  {
    m_isReplayPaused = true;
  }
}

void Context::resume_replay()
{
  if (is_replaying() && m_replayEntryIndex < m_replayEntries.size())
  {
    m_isReplayPaused = false;
    m_replayEntryBudget = 0.0;
    m_lastReplayTick = std::chrono::high_resolution_clock::now();
  }
}

void Context::step_replay_entries(std::size_t count)
{
  if (!is_replaying() || m_replayEntryIndex >= m_replayEntries.size())
  {
    return;
  }

  m_isReplayPaused = true;
  m_replayEntryBudget = 0.0;
  apply_replay_entries(count);
}

void Context::step_replay_entries_backward(std::size_t count)
{
  if (!is_replaying() || m_replayEntryIndex == 0)
  {
    return;
  }

  m_isReplayPaused = true;
  m_replayEntryBudget = 0.0;
  undo_replay_entries(count);
}

geoqik_replay_state_t Context::get_replay_state() const
{
  if (!is_replaying())
  {
    return GEOQIK_REPLAY_INACTIVE;
  }
  return m_isReplayPaused ? GEOQIK_REPLAY_PAUSED : GEOQIK_REPLAY_PLAYING;
}

std::pair<std::size_t, std::size_t> Context::get_replay_progress() const
{
  if (!is_replaying())
  {
    return {0, 0};
  }
  return {m_replayEntryIndex, m_replayEntries.size()};
}

void Context::handle_message(const AddPointWithOpts& message)
{
  add_point_with_opts(message.x, message.y, message.z, message.commonData);
}

void Context::handle_message(const AddPointsWithOpts& message)
{
  add_points_with_opts(message.points, message.commonData);
}

void Context::handle_message(const UpdatePointWithOpts& message)
{
  update_point_with_opts(message.handle, message.x, message.y, message.z, message.rgba);
}

void Context::handle_message(const UpdatePointsWithOpts& message)
{
  update_points_with_opts(message.handle, message.points, message.rgba);
}

void Context::handle_message(const RemovePoint& message)
{
  remove_point(message.handle);
}

void Context::handle_message(const SetPointSize& message)
{
  set_point_size(message.size);
}

void Context::handle_message(const SetPointColor& message)
{
  set_default_point_color(message.color);
}

void Context::handle_message(const AddLineWithOpts& message)
{
  add_line_with_opts(message.x1, message.y1, message.z1, message.x2, message.y2, message.z2, message.commonData);
}

void Context::handle_message(const AddLinesWithOpts& message)
{
  add_lines_with_opts(message.lines, message.commonData);
}

void Context::handle_message(const UpdateLineWithOpts& message)
{
  update_line_with_opts(message.handle, message.x1, message.y1, message.z1, message.x2, message.y2, message.z2, message.rgba);
}

void Context::handle_message(const UpdateLinesWithOpts& message)
{
  update_lines_with_opts(message.handle, message.lines, message.rgba);
}

void Context::handle_message(const RemoveLine& message)
{
  remove_line(message.handle);
}

void Context::handle_message(const SetLineWidth& message)
{
  set_line_width(message.width);
}

void Context::handle_message(const SetLineColor& message)
{
  set_line_color(message.color);
}

void Context::handle_message([[maybe_unused]] const RemoveAllGeometry& message)
{
  remove_all_geometry();
}

void Context::handle_message(const TranslateGeometry& message)
{
  translate_geometry(message.handle, message.dx, message.dy, message.dz);
}

void Context::handle_message(const RotateGeometry& message)
{
  rotate_geometry(message.handle,
                  message.centerX,
                  message.centerY,
                  message.centerZ,
                  message.axisX,
                  message.axisY,
                  message.axisZ,
                  message.angle);
}

void Context::handle_message(const AddMeshWithOpts& message)
{
  if (is_known_idempotency_key(&message.commonData.idempotencyId))
  {
    return;
  }
  add_mesh_with_opts(message.vertices, message.normals, message.commonData.rgba,
                     message.triangleIndices, message.commonData);
  // Mesh messages not added to m_messageLog in this phase.
}

void Context::add_mesh_with_opts(std::span<const float> vertices,
                                  std::span<const float> normals,
                                  std::span<const float> colors,
                                  std::span<const std::uint32_t> triangleIndices,
                                  const GeoQikMessageCommonData& commonData)
{
  const core::UUID* handlePtr = commonData.geometryId.is_nil() ? nullptr : &commonData.geometryId;
  m_scene.add_mesh(vertices, normals, colors, triangleIndices, handlePtr);
  ++m_geometryMessagesProcessedThisFrame;
}

void Context::handle_message(const RemoveMesh& message)
{
  m_scene.remove_mesh(message.handle);
  ++m_geometryMessagesProcessedThisFrame;
}

void Context::handle_message([[maybe_unused]] const Draw& message)
{
  m_isDrawing = true;
}

void Context::handle_message([[maybe_unused]] const StopDraw& message)
{
  m_isDrawing = false;
}

void Context::handle_message(const SaveLog& message)
{
  CORE_ASSERT(message.callback);
  message.callback(*this);
}

void Context::handle_message(const LoadLog& message)
{
  CORE_ASSERT(message.callback);
  message.callback(*this);
}

void Context::handle_message(const ReplayLog& message)
{
  CORE_ASSERT(message.callback);
  message.callback(*this);
}

void Context::handle_message(const ReplayCurrentLog& message)
{
  CORE_ASSERT(message.callback);
  message.callback(*this);
}

void Context::handle_message([[maybe_unused]] const PauseReplay& message)
{
  pause_replay();
}

void Context::handle_message([[maybe_unused]] const ResumeReplay& message)
{
  resume_replay();
}

void Context::handle_message(const StepReplay& message)
{
  step_replay_entries(message.count);
}

void Context::handle_message(const StepReplayBackward& message)
{
  step_replay_entries_backward(message.count);
}

void Context::handle_message(const GetReplayState& message)
{
  CORE_ASSERT(message.callback);
  message.callback(*this);
}

void Context::handle_message(const GetReplayProgress& message)
{
  CORE_ASSERT(message.callback);
  message.callback(*this);
}

void Context::handle_message(const GetPointSize& message)
{
  CORE_ASSERT(message.callback);
  message.callback(*this);
}

void Context::handle_message(const GetPointColor& message)
{
  CORE_ASSERT(message.callback);
  message.callback(*this);
}

void Context::handle_message(const GetLineWidth& message)
{
  CORE_ASSERT(message.callback);
  message.callback(*this);
}

void Context::handle_message(const GetLineColor& message)
{
  CORE_ASSERT(message.callback);
  message.callback(*this);
}

void Context::handle_message(const SetMeshColor& message)
{
  set_mesh_color(message.color);
}

void Context::handle_message(const GetMeshColor& message)
{
  message.callback(*this);
}

void Context::handle_message([[maybe_unused]] const Cleanup& message)
{
  cleanup();
  m_windowShouldClose.store(true);
}

bool Context::is_replaying() const
{
  return m_isReplayActive;
}

bool Context::is_control_message(const GeoQikMessage& message)
{
  return std::holds_alternative<Cleanup>(message) ||
         std::holds_alternative<PauseReplay>(message) ||
         std::holds_alternative<ResumeReplay>(message) ||
         std::holds_alternative<StepReplay>(message) ||
         std::holds_alternative<StepReplayBackward>(message) ||
         std::holds_alternative<GetReplayState>(message) ||
         std::holds_alternative<GetReplayProgress>(message);
}

void Context::on_key(Key key, [[maybe_unused]] Scancode scancode, Action action, [[maybe_unused]] Mods mods)
{
  if (!is_replaying())
  {
    return;
  }

  if (action == Action::PRESS && m_isReplayPaused && has_replay_key(m_replayOptions.resumeKeys, key))
  {
    resume_replay();
    return;
  }

  if (action == Action::PRESS && !m_isReplayPaused && has_replay_key(m_replayOptions.pauseKeys, key))
  {
    pause_replay();
    return;
  }

  if ((action == Action::PRESS || action == Action::REPEAT) && has_replay_key(m_replayOptions.stepKeys, key))
  {
    step_replay_entries(m_replayOptions.entriesPerStep);
    return;
  }

  if ((action == Action::PRESS || action == Action::REPEAT) && has_replay_key(m_replayOptions.backwardStepKeys, key))
  {
    step_replay_entries_backward(m_replayOptions.entriesPerStep);
    return;
  }

  if (action == Action::PRESS && has_replay_key(m_replayOptions.increaseEntriesPerStepKeys, key))
  {
    ++m_replayOptions.entriesPerStep;
    return;
  }

  if (action == Action::PRESS && has_replay_key(m_replayOptions.decreaseEntriesPerStepKeys, key) && m_replayOptions.entriesPerStep > 1)
  {
    --m_replayOptions.entriesPerStep;
  }
}

bool Context::has_replay_key(const std::vector<Key>& keys, Key key)
{
  return std::find(keys.begin(), keys.end(), key) != keys.end();
}

void Context::start_replay(std::vector<GeoQikLogEntry> entries, const ReplayOptions& options)
{
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
}

void Context::process_replay_entries(const std::chrono::high_resolution_clock::time_point& now)
{
  if (!is_replaying())
  {
    replay_cancel_requested_storage().store(false, std::memory_order_release);
    m_lastReplayTick = now;
    return;
  }

  if (replay_cancel_requested_storage().exchange(false, std::memory_order_acq_rel))
  {
    cancel_replay();
    m_lastReplayTick = now;
    return;
  }

  if (m_isReplayPaused)
  {
    m_lastReplayTick = now;
    return;
  }

  if (m_replayEntryIndex >= m_replayEntries.size())
  {
    finish_replay();
    m_lastReplayTick = now;
    return;
  }

  const std::chrono::duration<double> elapsed = now - m_lastReplayTick;
  m_lastReplayTick = now;
  m_replayEntryBudget += elapsed.count() * m_replayOptions.entriesPerSecond;

  auto entriesToApply = static_cast<std::size_t>(m_replayEntryBudget);
  entriesToApply = std::min(entriesToApply, m_replayOptions.maxEntriesPerFrame);

  apply_replay_entries(entriesToApply);

  if (is_replaying() && m_replayEntryIndex >= m_replayEntries.size())
  {
    finish_replay();
  }
}

void Context::apply_replay_entries(std::size_t entriesToApply)
{
  for (std::size_t i = 0; i < entriesToApply && is_replaying() && m_replayEntryIndex < m_replayEntries.size(); ++i)
  {
    m_replayUndoStack.push_back(create_replay_undo_frame(m_replayEntries[m_replayEntryIndex]));
    apply_log_entry(m_replayEntries[m_replayEntryIndex]);
    ++m_replayEntryIndex;
    if (m_replayEntryBudget >= 1.0)
    {
      m_replayEntryBudget -= 1.0;
    }
  }

  m_geometryMessagesProcessedThisFrame += entriesToApply;

  if (is_replaying() && m_replayEntryIndex >= m_replayEntries.size())
  {
    finish_replay();
  }
}

void Context::undo_replay_entries(std::size_t entriesToUndo)
{
  for (std::size_t i = 0; i < entriesToUndo && is_replaying() && m_replayEntryIndex > 0 && !m_replayUndoStack.empty(); ++i)
  {
    --m_replayEntryIndex;
    restore_replay_undo_frame(m_replayUndoStack.back());
    m_replayUndoStack.pop_back();
  }

  m_geometryMessagesProcessedThisFrame += entriesToUndo;
}

ReplayUndoFrame Context::create_replay_undo_frame(const GeoQikLogEntry& entry) const
{
  return geoqik::create_replay_undo_frame(entry, ReplayUndoContext{m_scene, m_idempotencySet});
}

void Context::restore_replay_undo_frame(const ReplayUndoFrame& frame)
{
  std::visit(
      [this](const auto& action)
      {
        using T = std::decay_t<decltype(action)>;
        if constexpr (std::is_same_v<T, GeoQikLogEntry>)
        {
          apply_log_entry(action);
        }
        else if constexpr (std::is_same_v<T, ReplayUndoFrame::RestoreScene>)
        {
          m_scene.restore_snapshot(action.scene);
          m_sceneRenderer->clear_drawables();
        }
      },
      frame.action);

  if (!frame.idempotencyKeyToErase.is_nil())
  {
    m_idempotencySet.erase(IdempotencyData{frame.idempotencyKeyToErase, {}});
  }
}

void Context::finish_replay()
{
  m_replayEntries.clear();
  m_replayUndoStack.clear();
  m_replayEntryIndex = 0;
  m_replayEntryBudget = 0.0;
  m_isReplayActive = false;
  m_isReplayPaused = false;
}

void Context::defer_or_handle_message(GeoQikMessage&& message)
{
  if (is_replaying() && !is_control_message(message))
  {
    m_deferredMessages.push_back(std::move(message));
    return;
  }

  if (std::holds_alternative<Cleanup>(message))
  {
    cancel_replay();
    if (process_deferred_messages_before_cleanup())
    {
      return;
    }
  }

  if (process_message(message, true))
  {
    m_windowShouldClose.store(true);
  }
  else if (!is_replaying())
  {
    process_deferred_messages();
  }
}

bool Context::process_message(const GeoQikMessage& message, bool recordLogEntry)
{
  if (recordLogEntry)
  {
    if (auto logEntry = create_log_entry(message))
    {
      m_messageLog.push_back(std::move(*logEntry));
    }
  }

  return std::visit(
      [this](const auto& value)
      {
        using T = std::decay_t<decltype(value)>;
        handle_message(value);
        return std::is_same_v<T, Cleanup>;
      },
      message);
}

bool Context::process_deferred_messages()
{
  while (!m_deferredMessages.empty() && !is_replaying())
  {
    GeoQikMessage message = std::move(m_deferredMessages.front());
    m_deferredMessages.pop_front();

    if (process_message(message, true))
    {
      m_windowShouldClose.store(true);
      return true;
    }
  }
  return false;
}

bool Context::process_deferred_messages_before_cleanup()
{
  while (!m_deferredMessages.empty())
  {
    if (is_replaying())
    {
      cancel_replay();
    }

    GeoQikMessage message = std::move(m_deferredMessages.front());
    m_deferredMessages.pop_front();

    if (process_message(message, true))
    {
      m_windowShouldClose.store(true);
      return true;
    }
  }
  return false;
}

bool Context::is_known_idempotency_key(const core::UUID* key)
{
  if (key != nullptr && !key->is_nil())
  {
    IdempotencyData idempotencyData{*key, std::chrono::high_resolution_clock::now()};
    auto [it, inserted] = m_idempotencySet.insert(idempotencyData);
    if (!inserted)
    {
      return true;
    }
  }
  return false;
}

void Context::replay_log_entries(const std::vector<GeoQikLogEntry>& entries)
{
  for (const GeoQikLogEntry& entry: entries)
  {
    apply_log_entry(entry);
  }
}

void Context::apply_log_entry(const GeoQikLogEntry& entry)
{
  std::visit(
      [this](const auto& value)
      {
        handle_message(value);
      },
      entry);
}

void Context::print_frame_info(const std::chrono::high_resolution_clock::time_point& startTime,
                               const std::chrono::high_resolution_clock::time_point& messageProcessingStartTime,
                               const std::chrono::high_resolution_clock::time_point& endTime) const
{
  if (m_frameCount % frameInfoPrintInterval != 0)
  {
    return;
  }

  if (m_frameCount > 0)
  {
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

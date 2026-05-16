#include "Context.hpp"
#include "GlfwWindow.hpp"
#include "GeoQikMessages.hpp"
#include "Rendering/OpenGLSceneRenderer.hpp"
#include <Core/Assert.hpp>
#include <fmt/format.h>
#include <type_traits>
#include <utility>

namespace geoqik
{

static ConcurrentQueue<GeoQikMessage> g_messageQueue;

void init_message_queue(ConcurrentQueue<GeoQikMessage>&& messageQueue)
{
  g_messageQueue = std::move(messageQueue);
}

ConcurrentQueue<GeoQikMessage>& get_message_queue()
{
  return g_messageQueue;
}

Context::Context() = default;

Context::~Context() = default;

bool Context::init_window(const GeoQikSettings& geoqikSettings, const WindowSettings& settings)
{
  if (m_window && m_window->is_initialized())
  {
    fmt::print("GeoQik context is already initialized.\n");
    CORE_ASSERT(false);
    return false;
  }

  m_geoqikSettings = geoqikSettings;
  m_windowSettings = std::make_unique<WindowSettings>(settings);

  m_scene = Scene::create(geoqikSettings);

  m_window = std::make_unique<GlfwWindow>();
  if (!m_window->create(*m_windowSettings))
  {
    m_window.reset();
    return false;
  }

  m_backgroundColor[0] = m_geoqikSettings.backgroundColor[0];
  m_backgroundColor[1] = m_geoqikSettings.backgroundColor[1];
  m_backgroundColor[2] = m_geoqikSettings.backgroundColor[2];
  m_backgroundColor[3] = m_geoqikSettings.backgroundColor[3];

  m_renderer = std::make_unique<OpenGLSceneRenderer>();
  m_renderer->compile_programs();

  // For high DPI, the frame buffer size may be different from the window size.
  const auto [framebufferWidth, framebufferHeight] = m_window->get_framebuffer_size();
  if (framebufferWidth <= 0 || framebufferHeight <= 0)
  {
    fmt::print("Error: Invalid framebuffer size: {}x{}\n", framebufferWidth, framebufferHeight);
    CORE_ASSERT(false);
    return false;
  }

  CameraSettings cameraSettings;
  cameraSettings.m_camera.set_viewport(0, 0, static_cast<std::uint32_t>(framebufferWidth), static_cast<std::uint32_t>(framebufferHeight));

  m_cameraInteractor = std::make_unique<CameraInteractor>(m_window->get_input_state(), cameraSettings);

  m_window->set_cursor_pos_callback([this](double xpos, double ypos) { m_cameraInteractor->on_cursor_position(xpos, ypos); });
  m_window->set_scroll_callback([this](double xoff, double yoff) { m_cameraInteractor->on_scroll(xoff, yoff); });
  m_window->set_mouse_button_callback([this](int button, Action action, Mods mods) { m_cameraInteractor->on_mouse_button(button, action, mods); });
  m_window->set_framebuffer_size_callback([this](std::uint32_t width, std::uint32_t height) { m_cameraInteractor->on_framebuffer_size(width, height); });

  return true;
}

float Context::get_point_size()
{
  return m_scene.get_point_size();
}

void Context::set_point_size(float pointSize)
{
  m_scene.set_point_size(pointSize);
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

void Context::add_point_with_opts(float x, float y, float z, const GeoQikMessageCommonData& commonData)
{
  if (is_known_idempotency_key(&commonData.idempotencyId))
  {
    return;
  }
  if (m_scene.ensure_point_capacity(1))
  {
    m_renderer->recreate_point_drawables(m_scene);
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
    m_renderer->recreate_point_drawables(m_scene);
  }
  m_scene.add_points(points, std::span<const float>(commonData.rgba), &commonData.geometryId);
  ++m_geometryMessagesProcessedThisFrame;
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
    m_renderer->recreate_line_drawables(m_scene);
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
    m_renderer->recreate_line_drawables(m_scene);
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
    m_renderer->recreate_line_drawables(m_scene);
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
  if (m_scene.ensure_line_capacity(lines.size() / 6))
  {
    m_renderer->recreate_line_drawables(m_scene);
  }
  m_scene.add_lines(lines,
                    std::span<const float>(commonData.rgba),
                    &commonData.geometryId);
  ++m_geometryMessagesProcessedThisFrame;
}

void Context::remove_line(const core::UUID& handle)
{
  m_scene.remove_line(handle);
  ++m_geometryMessagesProcessedThisFrame;
}

void Context::remove_all_geometry()
{
  m_scene.clear();
  m_renderer->clear_drawables();
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
  return m_cameraInteractor->get_viewport();
}

// #define PRINT_FRAME_INFO

void Context::run_event_loop()
{
  assert(m_window && m_window->is_initialized());

  auto& messageQueue = get_message_queue();
  while (!m_windowShouldClose)
  {
    std::chrono::high_resolution_clock::time_point frameStartTime = std::chrono::high_resolution_clock::now();

    m_window->make_context_current();

    linal::hmatf mvp;
    mvp = m_cameraInteractor->get_current_MVP();

    m_window->poll_events();

    // Check for escape key or window close event to stop drawing
    if (m_window->is_escape_pressed())
    {
      m_windowShouldClose.store(true);
      break;
    }

    // Check if glfw got a request to close the window via the close button
    if (m_window->should_close())
    {
      m_windowShouldClose.store(true);
      break;
    }

    const auto& viewport = m_cameraInteractor->get_viewport();
    m_renderer->begin_frame(m_backgroundColor, viewport);

    if (m_renderer->sync_scene(m_scene))
    {
      // Update the camera, since the scene geometry has changed
      // First get the current bounding sphere of the scene
      Geometry::Sphere<float> boundingSphere = m_scene.calc_bounding_sphere(m_cameraInteractor->get_target());
      // Then check if the camera pos is inside the bounding sphere

      boundingSphere.set_radius(boundingSphere.get_radius() * 1.6f);

      if (boundingSphere.contains(m_cameraInteractor->get_position()))
      {
        // If it is, we move the camera along the opposite direction it's current view direction until it is outside
        // the bounding sphere.
        linal::double3 cameraPos = m_cameraInteractor->get_position();
        linal::double3 cameraTarget = m_cameraInteractor->get_target();
        linal::double3 cameraVertical = m_cameraInteractor->get_vertical();
        linal::float3 origin = boundingSphere.get_origin();
        linal::double3 cTarget{origin[0], origin[1], origin[2]};
        linal::double3 cameraDir = linal::normalize(cTarget - cameraPos);
        double radius = static_cast<double>(boundingSphere.get_radius());
        linal::double3 newCameraPos = cameraPos - cameraDir * radius;
        m_cameraInteractor->look_at(newCameraPos, cameraTarget, cameraVertical);
        // Update the near and the far plane of the camera
        // Increase the far plane to ensure the whole scene is visible when the camera is zoomed out
        m_cameraInteractor->set_far_plane(static_cast<double>(boundingSphere.get_radius()) * 2 * m_geoqikSettings.cameraFarPlaneMultiplier);
      }
    }

    mvp = m_cameraInteractor->get_current_MVP();
    m_renderer->draw(mvp, m_cameraInteractor->get_position());
    m_window->swap_buffers();

    ////////////-------------------------------------
    // Process messages in the queue
    ////////////-------------------------------------

    m_geometryMessagesProcessedThisFrame = 0;
    std::size_t messagesProcessedThisFrame = 0;
    std::chrono::high_resolution_clock::time_point messageProcessingStartTime = std::chrono::high_resolution_clock::now();
    while (true)
    {
      if (messageQueue.empty())
      {
        break;
      }

      std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();

      // Stop processing messages if enough messages have been processed or if the frame processing time
      // exceeds the maximum allowed time.
      // The maximum frame processing time is used to ensure the fps does not drop too low.
      // Checking the frame processing time is only done if the window is drawing geometry already.
      if (m_isDrawing)
      {
        auto elapsedMessageTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - messageProcessingStartTime);

        bool minimumProcessingTimeReached = true;
        if (m_geoqikSettings.minGeometryProcessingTime > std::chrono::milliseconds(0))
        {
          minimumProcessingTimeReached =
              elapsedMessageTime >= m_geoqikSettings.minGeometryProcessingTime;
        }

        if (minimumProcessingTimeReached &&
            ((now - frameStartTime) >= m_geoqikSettings.maxFrameProcessingTime))
        {
          break;
        }
      }

      std::optional<GeoQikMessage> message = messageQueue.dequeue();
      if (message.has_value())
      {
        ++messagesProcessedThisFrame;

        if (auto logEntry = create_log_entry(message.value()))
        {
          m_messageLog.push_back(std::move(*logEntry));
        }

        const bool shouldReturn = std::visit(
            [this](const auto& value)
            {
              using T = std::decay_t<decltype(value)>;
              handle_message(value);
              return std::is_same_v<T, Cleanup>;
            },
            message.value());
        if (shouldReturn)
        {
          return;
        }
      }
    }

#ifdef PRINT_FRAME_INFO
    std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
    print_frame_info(frameStartTime, messageProcessingStartTime, endTime);
#endif
  }
}

bool Context::cleanup()
{
  if (!m_window || !m_window->is_initialized())
  {
    fmt::print("GLFW window is not initialized.\n");
    return false;
  }

  m_window->make_context_current();

  if (m_renderer)
  {
    m_renderer->clear_drawables();
    m_renderer->reset_programs();
    m_renderer.reset();
  }

  m_window->destroy();
  m_window.reset();

  return true;
}

geoqik_error_code_t Context::save_log(const char* path, geoqik_log_format_t format) const
{
  if (!path || path[0] == '\0' || format != GEOQIK_LOG_FORMAT_BINARY)
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
  if (!path || path[0] == '\0' || format != GEOQIK_LOG_FORMAT_BINARY)
  {
    return GEOQIK_ERROR_INVALID_PARAMETER;
  }

  try
  {
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

void Context::handle_message(const AddPointWithOpts& message)
{
  add_point_with_opts(message.x, message.y, message.z, message.commonData);
}

void Context::handle_message(const AddPointsWithOpts& message)
{
  add_points_with_opts(message.points, message.commonData);
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

void Context::handle_message([[maybe_unused]] const Cleanup& message)
{
  cleanup();
}

bool Context::is_known_idempotency_key(const core::UUID* key)
{
  if (key && !key->is_nil())
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
  if (m_frameCount % 10 != 0)
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

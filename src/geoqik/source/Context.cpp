#include "Context.hpp"
#include "GlfwWindow.hpp"
#include "GeoQikMessages.hpp"
#include "Rendering/OpenGLSceneRenderer.hpp"
#include <Core/Assert.hpp>
#include <fmt/format.h>

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

  initialize_message_handlers();

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

void Context::add_point_with_opts(float x, float y, float z, const GeoQikMessageData::CommonMessageData& commonData)
{
  if (is_known_idempotency_key(&commonData.idempotencyId))
  {
    return;
  }
  if (m_scene.ensure_point_capacity(1))
  {
    m_renderer->recreate_point_drawables(m_scene);
  }
  if (commonData.rgba && commonData.rgbaCount >= ColorChannelCount)
  {
    m_scene.add_point(x, y, z, commonData.rgba[0], commonData.rgba[1], commonData.rgba[2], commonData.rgba[3], &commonData.geometryId);
  }
  else
  {
    m_scene.add_point(x, y, z, &commonData.geometryId);
  }
  ++m_geometryMessagesProcessedThisFrame;
}

void Context::add_points_with_opts(const float* points, std::size_t count, const GeoQikMessageData::CommonMessageData& commonData)
{
  if (is_known_idempotency_key(&commonData.idempotencyId))
  {
    return;
  }
  if (m_scene.ensure_point_capacity(count / 3))
  {
    m_renderer->recreate_point_drawables(m_scene);
  }
  m_scene.add_points(std::span<const float>(points, count), std::span<const float>(commonData.rgba, commonData.rgbaCount), &commonData.geometryId);
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

void Context::add_line_with_opts(float x1, float y1, float z1, float x2, float y2, float z2, const GeoQikMessageData::CommonMessageData& commonData)
{
  if (is_known_idempotency_key(&commonData.idempotencyId))
  {
    return;
  }
  if (m_scene.ensure_line_capacity(1))
  {
    m_renderer->recreate_line_drawables(m_scene);
  }
  if (commonData.rgba && commonData.rgbaCount >= ColorChannelCount)
  {
    m_scene.add_line(x1, y1, z1, x2, y2, z2, commonData.rgba[0], commonData.rgba[1], commonData.rgba[2], commonData.rgba[3], &commonData.geometryId);
  }
  else
  {
    m_scene.add_line(x1, y1, z1, x2, y2, z2, &commonData.geometryId);
  }
  ++m_geometryMessagesProcessedThisFrame;
}

void Context::add_lines_with_opts(const float* lines, std::size_t count, const GeoQikMessageData::CommonMessageData& commonData)
{
  if (is_known_idempotency_key(&commonData.idempotencyId))
  {
    return;
  }
  if (m_scene.ensure_line_capacity(count / 6))
  {
    m_renderer->recreate_line_drawables(m_scene);
  }
  m_scene.add_lines(std::span<const float>(lines, count),
                    std::span<const float>(commonData.rgba, commonData.rgbaCount),
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

        // Special handling for CLEANUP message - needs to return early
        if (message->type == GeoQikMessageType::CLEANUP)
        {
          cleanup();
          return;
        }

        // Look up and invoke the appropriate message handler
        auto it = m_messageHandlers.find(message->type);
        if (it != m_messageHandlers.end())
        {
          it->second(*this, message.value());
        }
        else
        {
          fmt::print("Warning: Unhandled message type\n");
          CORE_ASSERT(false);
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

void Context::initialize_message_handlers()
{
  // Draw control messages
  m_messageHandlers[GeoQikMessageType::CLEANUP] = [](Context& ctx, [[maybe_unused]] const GeoQikMessage& msg)
  {
    ctx.cleanup();
    // Note: This handler needs special treatment in the main loop to return early
  };

  m_messageHandlers[GeoQikMessageType::DRAW] = [](Context& ctx, [[maybe_unused]] const GeoQikMessage& msg) { ctx.m_isDrawing = true; };

  m_messageHandlers[GeoQikMessageType::STOP_DRAW] = [](Context& ctx, [[maybe_unused]] const GeoQikMessage& msg) { ctx.m_isDrawing = false; };

  m_messageHandlers[GeoQikMessageType::ADD_POINT_WITH_OPTS] = [](Context& ctx, const GeoQikMessage& msg)
  {
    const auto& pointWithOpts = msg.data.pointWithOpts;
    ctx.add_point_with_opts(pointWithOpts.x, pointWithOpts.y, pointWithOpts.z, pointWithOpts.commonData);
    delete[] pointWithOpts.commonData.rgba;
  };

  m_messageHandlers[GeoQikMessageType::ADD_POINTS_WITH_OPTS] = [](Context& ctx, const GeoQikMessage& msg)
  {
    const auto& pointsWithOpts = msg.data.pointsWithOpts;
    ctx.add_points_with_opts(pointsWithOpts.points, pointsWithOpts.size, pointsWithOpts.commonData);
    delete[] pointsWithOpts.points;
    delete[] pointsWithOpts.commonData.rgba;
  };

  m_messageHandlers[GeoQikMessageType::REMOVE_POINT] = [](Context& ctx, const GeoQikMessage& msg)
  {
    const auto& removePoint = msg.data.removePoint;
    ctx.remove_point(removePoint.handle);
  };

  m_messageHandlers[GeoQikMessageType::SET_POINT_SIZE] = [](Context& ctx, const GeoQikMessage& msg)
  { ctx.set_point_size(msg.data.pointSize.size); };

  m_messageHandlers[GeoQikMessageType::GET_POINT_SIZE] = [](Context& ctx, const GeoQikMessage& msg)
  {
    CORE_ASSERT(msg.callback);
    msg.callback(ctx);
  };

  m_messageHandlers[GeoQikMessageType::SET_POINT_COLOR] = [](Context& ctx, const GeoQikMessage& msg)
  {
    ctx.set_default_point_color(msg.data.color);
  };

  m_messageHandlers[GeoQikMessageType::GET_POINT_COLOR] = [](Context& ctx, const GeoQikMessage& msg)
  {
    CORE_ASSERT(msg.callback);
    msg.callback(ctx);
  };

  m_messageHandlers[GeoQikMessageType::ADD_LINE_WITH_OPTS] = [](Context& ctx, const GeoQikMessage& msg)
  {
    const auto& lineWithOpts = msg.data.lineWithOpts;
    ctx.add_line_with_opts(lineWithOpts.x1, lineWithOpts.y1, lineWithOpts.z1,
                           lineWithOpts.x2, lineWithOpts.y2, lineWithOpts.z2,
                           lineWithOpts.commonData);
    delete[] lineWithOpts.commonData.rgba;
  };

  m_messageHandlers[GeoQikMessageType::ADD_LINES_WITH_OPTS] = [](Context& ctx, const GeoQikMessage& msg)
  {
    const auto& linesWithOpts = msg.data.linesWithOpts;
    ctx.add_lines_with_opts(linesWithOpts.lines, linesWithOpts.size, linesWithOpts.commonData);
    delete[] linesWithOpts.lines;
    delete[] linesWithOpts.commonData.rgba;
  };

  m_messageHandlers[GeoQikMessageType::REMOVE_LINE] = [](Context& ctx, const GeoQikMessage& msg)
  {
    const auto& removeLine = msg.data.removeLine;
    ctx.remove_line(removeLine.handle);
  };

  m_messageHandlers[GeoQikMessageType::SET_LINE_WIDTH] = [](Context& ctx, const GeoQikMessage& msg)
  { ctx.set_line_width(msg.data.lineWidth.width); };

  m_messageHandlers[GeoQikMessageType::GET_LINE_WIDTH] = [](Context& ctx, const GeoQikMessage& msg)
  {
    CORE_ASSERT(msg.callback);
    msg.callback(ctx);
  };

  m_messageHandlers[GeoQikMessageType::SET_LINE_COLOR] = [](Context& ctx, const GeoQikMessage& msg)
  {
    ctx.set_line_color(msg.data.color);
  };

  m_messageHandlers[GeoQikMessageType::GET_LINE_COLOR] = [](Context& ctx, const GeoQikMessage& msg)
  {
    CORE_ASSERT(msg.callback);
    msg.callback(ctx);
  };


  m_messageHandlers[GeoQikMessageType::REMOVE_ALL_GEOMETRY] = [](Context& ctx, [[maybe_unused]] const GeoQikMessage& msg)
  {
    ctx.remove_all_geometry();
  };

  m_messageHandlers[GeoQikMessageType::TRANSLATE_GEOMETRY] = [](Context& ctx, const GeoQikMessage& msg)
  {
    const auto& translateMsg = msg.data.translateGeometry;
    ctx.translate_geometry(translateMsg.handle, translateMsg.dx, translateMsg.dy, translateMsg.dz);
  };

  m_messageHandlers[GeoQikMessageType::ROTATE_GEOMETRY] = [](Context& ctx, const GeoQikMessage& msg)
  {
    const auto& rotateMsg = msg.data.rotateGeometry;
    ctx.rotate_geometry(rotateMsg.handle,
                        rotateMsg.centerX,
                        rotateMsg.centerY,
                        rotateMsg.centerZ,
                        rotateMsg.axisX,
                        rotateMsg.axisY,
                        rotateMsg.axisZ,
                        rotateMsg.angle);
  };
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

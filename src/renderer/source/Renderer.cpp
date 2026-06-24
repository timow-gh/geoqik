#include <Renderer/Renderer.hpp>

namespace renderer
{

namespace
{

constexpr linal::double3 defaultCameraPosition{5.0, 5.0, 5.0};

} // namespace

std::unique_ptr<Renderer> Renderer::create(const WindowSettings& settings)
{
  auto window = GlfwWindow::create(settings);
  if (!window.has_value())
  {
    return nullptr;
  }

  const auto [fbWidth, fbHeight] = window->get_framebuffer_size();
  CameraSettings cameraSettings;
  cameraSettings.m_defaultPosition = defaultCameraPosition;
  cameraSettings.m_defaultTarget   = linal::double3{0.0, 0.0, 0.0};
  cameraSettings.m_defaultUp       = linal::double3{0.0, 0.0, 1.0};
  cameraSettings.m_camera.set_viewport(0,
                                       0,
                                       static_cast<std::uint32_t>(fbWidth),
                                       static_cast<std::uint32_t>(fbHeight));
  auto camera = std::make_shared<CameraInteractor>(window->get_input_state(), cameraSettings);

  auto imgui = std::make_shared<ImGuiOverlay>(window->get_native_handle());

  auto drawablesManager = opengl::DrawablesManager::create();
  if (!drawablesManager.has_value())
  {
    return nullptr;
  }

  // Use raw new because the constructor is private and Renderer is non-movable.
  auto* raw = new Renderer(std::move(window.value()),
                           std::move(drawablesManager.value()),
                           std::move(camera),
                           std::move(imgui));
  std::unique_ptr<Renderer> renderer(raw);

  raw->wire_callbacks();

  return renderer;
}

Renderer::Renderer(GlfwWindow window,
                   opengl::DrawablesManager drawables,
                   std::shared_ptr<CameraInteractor> camera,
                   std::shared_ptr<ImGuiOverlay> imgui)
    : m_window(std::move(window))
    , m_drawablesManager(std::move(drawables))
    , m_camera(std::move(camera))
    , m_imgui(std::move(imgui))
{
}

void Renderer::wire_callbacks()
{
  m_window.set_cursor_pos_callback(
      [this](double xpos, double ypos)
      {
        if (!m_imgui->handle_cursor_position(xpos, ypos))
        {
          return;
        }
        m_camera->on_cursor_position(xpos, ypos);
        for (const auto& cb : m_cursorPosCallbacks)
        {
          cb(xpos, ypos);
        }
      });

  m_window.set_scroll_callback(
      [this](double xoff, double yoff)
      {
        if (!m_imgui->handle_scroll(xoff, yoff))
        {
          return;
        }
        m_camera->on_scroll(xoff, yoff);
        for (const auto& cb : m_scrollCallbacks)
        {
          cb(xoff, yoff);
        }
      });

  m_window.set_mouse_button_callback(
      [this](int button, Action action, Mods mods)
      {
        if (!m_imgui->handle_mouse_button(button, action, mods))
        {
          return;
        }
        m_camera->on_mouse_button(button, action, mods);
        for (const auto& cb : m_mouseButtonCallbacks)
        {
          cb(button, action, mods);
        }
      });

  m_window.set_key_callback(
      [this](Key key, Scancode scancode, Action action, Mods mods)
      {
        (void)m_imgui->handle_key(key, scancode, action, mods);
        for (const auto& cb : m_keyCallbacks)
        {
          cb(key, scancode, action, mods);
        }
      });

  m_window.set_char_callback(
      [this](std::uint32_t codepoint) { m_imgui->handle_char(codepoint); });

  m_window.set_framebuffer_size_callback(
      [this](std::uint32_t width, std::uint32_t height)
      { m_camera->on_framebuffer_size(width, height); });
}

// --- Geometry ---

DrawableHandle Renderer::add_point_drawable(std::span<const float> vertices,
                                            std::span<const float> colors,
                                            std::span<const std::uint32_t> indices,
                                            float pointSize,
                                            opengl::BufferAccessPattern accessPattern)
{
  const auto id = m_drawablesManager.add_point_drawable(vertices, colors, indices, pointSize, accessPattern);
  return DrawableHandle{DrawableKind::point, id};
}

DrawableHandle Renderer::add_line_drawable(std::span<const float> vertices,
                                           std::span<const std::uint32_t> indices,
                                           std::span<const float> colors,
                                           opengl::LineType lineType,
                                           float lineWidth,
                                           float pointSize,
                                           opengl::BufferAccessPattern accessPattern)
{
  const auto id = m_drawablesManager.add_line_drawable(vertices, indices, colors, lineType, lineWidth, pointSize, accessPattern);
  return DrawableHandle{DrawableKind::line, id};
}

DrawableHandle Renderer::add_mesh_drawable(std::span<const float> vertices,
                                           std::span<const float> normals,
                                           std::span<const float> colors,
                                           std::span<const std::uint32_t> triangleIndices,
                                           opengl::BufferAccessPattern accessPattern)
{
  const auto id = m_drawablesManager.add_mesh_drawable(vertices, 3, normals, colors, 4, triangleIndices, accessPattern);
  return DrawableHandle{DrawableKind::mesh, id};
}

DrawableHandle Renderer::add_mesh_segment_drawable(std::span<const float> positions,
                                                    std::span<const std::uint32_t> indices,
                                                    std::span<const float> color,
                                                    float lineWidth)
{
  const auto id = m_drawablesManager.add_mesh_segment_drawable(positions, indices, color, lineWidth);
  return DrawableHandle{DrawableKind::meshSegment, id};
}

DrawableHandle Renderer::add_mesh_vertex_drawable(std::span<const float> positions,
                                                   std::span<const float> color,
                                                   float pointSize)
{
  const auto id = m_drawablesManager.add_mesh_vertex_drawable(positions, color, pointSize);
  return DrawableHandle{DrawableKind::meshVertex, id};
}

void Renderer::set_mesh_drawable_cull_mode(DrawableHandle handle, opengl::MeshCullFaceMode mode)
{
  if (handle.kind == DrawableKind::mesh && handle.id != 0U)
  {
    m_drawablesManager.set_mesh_drawable_cull_mode(handle.id, mode);
  }
}

bool Renderer::remove_drawable(DrawableHandle handle)
{
  if (!handle.is_valid())
  {
    return false;
  }

  switch (handle.kind)
  {
  case DrawableKind::point:
    return m_drawablesManager.remove_point_drawable(handle.id);
  case DrawableKind::line:
    return m_drawablesManager.remove_line_drawable(handle.id);
  case DrawableKind::mesh:
    return m_drawablesManager.remove_mesh_drawable(handle.id);
  case DrawableKind::meshSegment:
    return m_drawablesManager.remove_mesh_segment_drawable(handle.id);
  case DrawableKind::meshVertex:
    return m_drawablesManager.remove_mesh_vertex_drawable(handle.id);
  case DrawableKind::invalid:
    return false;
  }

  return false;
}

void Renderer::update_last_point_drawable(std::span<const float> vertices,
                                          std::span<const float> colors,
                                          std::span<const std::uint32_t> indices,
                                          opengl::BufferAccessPattern accessPattern)
{
  m_drawablesManager.update_last_point_drawable(vertices, colors, indices, accessPattern);
}

void Renderer::update_last_line_drawable(std::span<const float> vertices,
                                         std::span<const float> colors,
                                         std::span<const std::uint32_t> indices,
                                         opengl::BufferAccessPattern accessPattern)
{
  m_drawablesManager.update_last_line_drawable(vertices, colors, indices, accessPattern);
}

void Renderer::clear_point_drawables() { m_drawablesManager.clear_point_drawables(); }
void Renderer::clear_line_drawables()  { m_drawablesManager.clear_line_drawables(); }
void Renderer::clear_mesh_drawables()  { m_drawablesManager.clear_mesh_drawables(); }
void Renderer::clear_drawables()       { m_drawablesManager.clear_drawables(); }

bool Renderer::has_point_drawables() const { return m_drawablesManager.has_point_drawables(); }
bool Renderer::has_line_drawables()  const { return m_drawablesManager.has_line_drawables(); }
bool Renderer::has_mesh_drawables()  const { return m_drawablesManager.has_mesh_drawables(); }

// --- Frame lifecycle ---

void Renderer::poll_events()
{
  GlfwWindow::poll_events();
}

bool Renderer::should_close() const
{
  return m_window.should_close();
}

bool Renderer::is_escape_pressed() const
{
  return m_window.is_escape_pressed();
}

void Renderer::begin_frame(const opengl::ClearColor& clearColor)
{
  const auto [width, height] = m_window.get_framebuffer_size();
  opengl::begin_frame(clearColor, opengl::ViewportRect{0, 0, width, height});
  m_imgui->new_frame();
}

void Renderer::draw()
{
  const opengl::LightingConfig lighting;
  draw(lighting);
}

void Renderer::draw(const opengl::LightingConfig& lighting)
{
  m_drawablesManager.draw_lines_and_points(m_camera->get_current_MVP(), m_camera->get_position());

  if (m_drawablesManager.has_mesh_drawables())
  {
    const linal::float3 viewPosF{
        static_cast<float>(m_camera->get_position()[0]),
        static_cast<float>(m_camera->get_position()[1]),
        static_cast<float>(m_camera->get_position()[2])};

    opengl::LightingConfig effectiveLighting = lighting;
    effectiveLighting.lightPosition = viewPosF;
    m_drawablesManager.draw_meshes(m_camera->get_model_matrix(),
                                   m_camera->get_view_matrix(),
                                   m_camera->get_projection_matrix(),
                                   m_camera->get_normal_matrix(),
                                   viewPosF,
                                   effectiveLighting);
  }

  // Draw segment overlays on top of meshes.
  if (m_drawablesManager.has_mesh_segment_drawables())
  {
    m_drawablesManager.draw_mesh_segment_overlays(m_camera->get_current_MVP());
  }

  // Draw vertex overlays on top of meshes.
  if (m_drawablesManager.has_mesh_vertex_drawables())
  {
    m_drawablesManager.draw_mesh_vertex_overlays(m_camera->get_current_MVP());
  }
}

void Renderer::end_frame()
{
  bool autoFitEnabled = false;
  bool homeRequested = false;
  end_frame(autoFitEnabled, homeRequested);
}

void Renderer::end_frame(bool& autoFitEnabled)
{
  bool homeRequested = false;
  end_frame(autoFitEnabled, homeRequested);
}

void Renderer::end_frame(bool& autoFitEnabled, bool& homeRequested)
{
  CameraProjectionType projectionType = m_camera->get_projection_type();
  m_imgui->add_camera_controls(autoFitEnabled, projectionType, homeRequested);
  m_imgui->render();
  m_imgui->end_frame();

  if (projectionType != m_camera->get_projection_type())
  {
    m_camera->set_projection_type(projectionType);
  }

  m_window.swap_buffers();
}

// --- Callback extension ---

void Renderer::add_cursor_pos_callback(CursorPosCB cb)   { m_cursorPosCallbacks.push_back(std::move(cb)); }
void Renderer::add_scroll_callback(ScrollCB cb)           { m_scrollCallbacks.push_back(std::move(cb)); }
void Renderer::add_mouse_button_callback(MouseBtnCB cb)   { m_mouseButtonCallbacks.push_back(std::move(cb)); }
void Renderer::add_key_callback(KeyCB cb)                 { m_keyCallbacks.push_back(std::move(cb)); }

} // namespace renderer

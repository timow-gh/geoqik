#include "Rendering/OpenGLSceneRenderer.hpp"
#include <OpenGL/BufferAccessPattern.hpp>
#include <OpenGL/FrameState.hpp>

namespace geoqik
{

OpenGLSceneRenderer::OpenGLSceneRenderer()
    : m_drawablesManager(&m_programManager.get_point_program(), &m_programManager.get_line_program())
{
}

void OpenGLSceneRenderer::compile_programs()
{
  m_programManager.compile();
}

void OpenGLSceneRenderer::reset_programs() noexcept
{
  m_programManager.reset();
}

void OpenGLSceneRenderer::begin_frame(const Color& backgroundColor, const Viewport& viewport)
{
  opengl::begin_frame(opengl::ClearColor{backgroundColor[0], backgroundColor[1], backgroundColor[2], backgroundColor[3]},
                      opengl::ViewportRect{static_cast<int>(viewport.get_xpos()),
                                           static_cast<int>(viewport.get_ypos()),
                                           static_cast<int>(viewport.get_width()),
                                           static_cast<int>(viewport.get_height())});
}

void OpenGLSceneRenderer::recreate_point_drawables(const Scene& scene)
{
  clear_drawables();
  create_point_drawable(scene);
}

void OpenGLSceneRenderer::recreate_line_drawables(const Scene& scene)
{
  clear_drawables();
  create_point_drawable(scene);
  create_line_drawable(scene);
}

bool OpenGLSceneRenderer::sync_scene(Scene& scene)
{
  bool updateOccurred = false;
  auto& pointBuffer = scene.get_point_buffer();
  auto& lineBuffer = scene.get_line_buffer();

  if (!m_drawablesManager.has_point_drawables() && !pointBuffer.get_points().empty())
  {
    create_point_drawable(scene);
    updateOccurred = true;
  }
  else if (pointBuffer.points_have_changed())
  {
    m_drawablesManager.update_last_point_drawable(pointBuffer.get_points(),
                                                  pointBuffer.get_point_colors(),
                                                  pointBuffer.get_point_indices(),
                                                  opengl::BufferAccessPattern::STATIC_DRAW);
    pointBuffer.reset_points_have_changed();
    updateOccurred = true;
  }

  if (!m_drawablesManager.has_line_drawables() && !lineBuffer.get_lines().empty())
  {
    create_line_drawable(scene);
    updateOccurred = true;
  }
  else if (lineBuffer.lines_have_changed())
  {
    m_drawablesManager.update_last_line_drawable(lineBuffer.get_lines(),
                                                 lineBuffer.get_line_colors(),
                                                 lineBuffer.get_line_indices(),
                                                 opengl::BufferAccessPattern::STATIC_DRAW);
    lineBuffer.reset_lines_have_changed();
    updateOccurred = true;
  }

  return updateOccurred;
}

void OpenGLSceneRenderer::draw(const linal::hmatf& mvp, const linal::double3& viewPosition)
{
  m_drawablesManager.draw_lines_and_points(mvp, viewPosition);
}

void OpenGLSceneRenderer::clear_drawables()
{
  m_drawablesManager.clear_drawables();
}

void OpenGLSceneRenderer::create_point_drawable(const Scene& scene)
{
  const auto& pointBuffer = scene.get_point_buffer();
  if (pointBuffer.get_points().empty())
  {
    return;
  }

  m_drawablesManager.add_point_drawable(pointBuffer.get_points(),
                                        PointBuffer::get_point_dimension(),
                                        pointBuffer.get_point_colors(),
                                        PointBuffer::get_color_dimension(),
                                        pointBuffer.get_point_indices(),
                                        scene.get_point_size(),
                                        opengl::BufferAccessPattern::STATIC_DRAW);
}

void OpenGLSceneRenderer::create_line_drawable(const Scene& scene)
{
  const auto& lineBuffer = scene.get_line_buffer();
  if (lineBuffer.get_lines().empty())
  {
    return;
  }

  m_drawablesManager.add_line_drawable(lineBuffer.get_lines(),
                                       LineBuffer::get_point_dimension(),
                                       lineBuffer.get_line_indices(),
                                       lineBuffer.get_line_colors(),
                                       LineBuffer::get_color_dimension(),
                                       m_lineType,
                                       scene.get_line_width(),
                                       scene.get_point_size(),
                                       opengl::BufferAccessPattern::STATIC_DRAW);
}

} // namespace geoqik

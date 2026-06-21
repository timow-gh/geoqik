#include <Renderer/Renderer.hpp>
#include "Rendering/GeoQikSceneRenderer.hpp"
#include "GeometryBuffers/MeshBuffer.hpp"
#include <OpenGL/BufferAccessPattern.hpp>

namespace geoqik
{

bool GeoQikSceneRenderer::sync_scene(Scene& scene)
{
  bool updateOccurred = false;
  auto& pointBuffer = scene.get_point_buffer();
  auto& lineBuffer = scene.get_line_buffer();

  if (!m_renderer.has_point_drawables() && !pointBuffer.get_points().empty())
  {
    m_renderer.add_point_drawable(pointBuffer.get_points(),
                                  pointBuffer.get_point_colors(),
                                  pointBuffer.get_point_indices(),
                                  scene.get_point_size(),
                                  opengl::BufferAccessPattern::STATIC_DRAW);
    updateOccurred = true;
  }
  else if (pointBuffer.points_have_changed())
  {
    m_renderer.update_last_point_drawable(pointBuffer.get_points(),
                                          pointBuffer.get_point_colors(),
                                          pointBuffer.get_point_indices(),
                                          opengl::BufferAccessPattern::STATIC_DRAW);
    pointBuffer.reset_points_have_changed();
    updateOccurred = true;
  }

  if (!m_renderer.has_line_drawables() && !lineBuffer.get_lines().empty())
  {
    m_renderer.add_line_drawable(lineBuffer.get_lines(),
                                 lineBuffer.get_line_indices(),
                                 lineBuffer.get_line_colors(),
                                 m_lineType,
                                 scene.get_line_width(),
                                 scene.get_point_size(),
                                 opengl::BufferAccessPattern::STATIC_DRAW);
    updateOccurred = true;
  }
  else if (lineBuffer.lines_have_changed())
  {
    m_renderer.update_last_line_drawable(lineBuffer.get_lines(),
                                         lineBuffer.get_line_colors(),
                                         lineBuffer.get_line_indices(),
                                         opengl::BufferAccessPattern::STATIC_DRAW);
    lineBuffer.reset_lines_have_changed();
    updateOccurred = true;
  }

  auto& meshBuffer = scene.get_mesh_buffer();
  if (!m_renderer.has_mesh_drawables() && !meshBuffer.empty())
  {
    m_renderer.add_mesh_drawable(meshBuffer.get_vertices(),
                                 meshBuffer.get_normals(),
                                 meshBuffer.get_colors(),
                                 meshBuffer.get_triangle_indices(),
                                 opengl::BufferAccessPattern::STATIC_DRAW);
    updateOccurred = true;
  }
  else if (meshBuffer.has_changed())
  {
    m_renderer.clear_mesh_drawables();
    m_renderer.add_mesh_drawable(meshBuffer.get_vertices(),
                                 meshBuffer.get_normals(),
                                 meshBuffer.get_colors(),
                                 meshBuffer.get_triangle_indices(),
                                 opengl::BufferAccessPattern::STATIC_DRAW);
    meshBuffer.reset_changed_flag();
    updateOccurred = true;
  }

  return updateOccurred;
}

void GeoQikSceneRenderer::recreate_point_drawables(const Scene& scene)
{
  m_renderer.clear_drawables();
  const auto& pointBuffer = scene.get_point_buffer();
  if (pointBuffer.get_points().empty())
  {
    return;
  }
  m_renderer.add_point_drawable(pointBuffer.get_points(),
                                pointBuffer.get_point_colors(),
                                pointBuffer.get_point_indices(),
                                scene.get_point_size(),
                                opengl::BufferAccessPattern::STATIC_DRAW);
}

void GeoQikSceneRenderer::recreate_line_drawables(const Scene& scene)
{
  m_renderer.clear_drawables();
  const auto& pointBuffer = scene.get_point_buffer();
  if (!pointBuffer.get_points().empty())
  {
    m_renderer.add_point_drawable(pointBuffer.get_points(),
                                  pointBuffer.get_point_colors(),
                                  pointBuffer.get_point_indices(),
                                  scene.get_point_size(),
                                  opengl::BufferAccessPattern::STATIC_DRAW);
  }
  const auto& lineBuffer = scene.get_line_buffer();
  if (!lineBuffer.get_lines().empty())
  {
    m_renderer.add_line_drawable(lineBuffer.get_lines(),
                                 lineBuffer.get_line_indices(),
                                 lineBuffer.get_line_colors(),
                                 m_lineType,
                                 scene.get_line_width(),
                                 scene.get_point_size(),
                                 opengl::BufferAccessPattern::STATIC_DRAW);
  }
}

void GeoQikSceneRenderer::recreate_mesh_drawables(const Scene& scene)
{
  m_renderer.clear_mesh_drawables();
  const auto& meshBuffer = scene.get_mesh_buffer();
  if (!meshBuffer.empty())
  {
    m_renderer.add_mesh_drawable(meshBuffer.get_vertices(),
                                 meshBuffer.get_normals(),
                                 meshBuffer.get_colors(),
                                 meshBuffer.get_triangle_indices(),
                                 opengl::BufferAccessPattern::STATIC_DRAW);
  }
}

void GeoQikSceneRenderer::clear_drawables()
{
  m_renderer.clear_drawables();
}

} // namespace geoqik

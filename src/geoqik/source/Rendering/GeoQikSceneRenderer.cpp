#include "Rendering/GeoQikSceneRenderer.hpp"
#include "GeometryBuffers/MeshBuffer.hpp"
#include <OpenGL/BufferAccessPattern.hpp>
#include <Renderer/Renderer.hpp>

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

  if (meshBuffer.is_full_rebuild_needed())
  {
    for (auto& [uuid, bundle] : m_meshBundles)
    {
      if (bundle.surface.is_valid())  m_renderer.remove_drawable(bundle.surface);
      if (bundle.segments.is_valid()) m_renderer.remove_drawable(bundle.segments);
      if (bundle.vertices.is_valid()) m_renderer.remove_drawable(bundle.vertices);
    }
    m_meshBundles.clear();

    for (const core::UUID& uuid : meshBuffer.get_all_mesh_uuids())
      create_surface_bundle(uuid, meshBuffer);

    updateOccurred = true;
  }
  else
  {
    for (const core::UUID& uuid : meshBuffer.get_removed_meshes())
      remove_bundle(uuid);
    if (!meshBuffer.get_removed_meshes().empty())
      updateOccurred = true;

    for (const core::UUID& uuid : meshBuffer.get_updated_meshes())
    {
      remove_bundle(uuid);
      create_surface_bundle(uuid, meshBuffer);
    }
    if (!meshBuffer.get_updated_meshes().empty())
      updateOccurred = true;

    for (const core::UUID& uuid : meshBuffer.get_added_meshes())
      create_surface_bundle(uuid, meshBuffer);
    if (!meshBuffer.get_added_meshes().empty())
      updateOccurred = true;
  }

  meshBuffer.clear_change_tracking();

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
  for (auto& [uuid, bundle] : m_meshBundles)
  {
    if (bundle.surface.is_valid())  m_renderer.remove_drawable(bundle.surface);
    if (bundle.segments.is_valid()) m_renderer.remove_drawable(bundle.segments);
    if (bundle.vertices.is_valid()) m_renderer.remove_drawable(bundle.vertices);
  }
  m_meshBundles.clear();

  for (const core::UUID& uuid : scene.get_mesh_buffer().get_all_mesh_uuids())
    create_surface_bundle(uuid, scene.get_mesh_buffer());
}

void GeoQikSceneRenderer::clear_drawables()
{
  m_renderer.clear_drawables();
}

void GeoQikSceneRenderer::create_surface_bundle(const core::UUID& uuid, const MeshBuffer& meshBuffer)
{
  MeshDrawableBundle bundle;
  bundle.surface = m_renderer.add_mesh_drawable(meshBuffer.get_mesh_vertices(uuid),
                                                meshBuffer.get_mesh_normals(uuid),
                                                meshBuffer.get_mesh_colors(uuid),
                                                meshBuffer.get_local_triangle_indices(uuid),
                                                opengl::BufferAccessPattern::STATIC_DRAW);
  m_meshBundles.emplace(uuid, std::move(bundle));
}

void GeoQikSceneRenderer::remove_bundle(const core::UUID& uuid)
{
  auto it = m_meshBundles.find(uuid);
  if (it == m_meshBundles.end()) return;
  auto& bundle = it->second;
  if (bundle.surface.is_valid())  m_renderer.remove_drawable(bundle.surface);
  if (bundle.segments.is_valid()) m_renderer.remove_drawable(bundle.segments);
  if (bundle.vertices.is_valid()) m_renderer.remove_drawable(bundle.vertices);
  m_meshBundles.erase(it);
}

} // namespace geoqik

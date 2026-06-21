#include "Scene.hpp"
#include "GeometryHelpers.hpp"
#include <algorithm>
#include <cmath>

namespace geoqik
{

Scene Scene::create(const GeoQikSettings& geoqikSettings)
{
  Scene scene;
  scene.m_pointBuffer = PointBuffer::create(geoqikSettings);
  scene.m_lineBuffer = LineBuffer::create(geoqikSettings);
  scene.m_meshBuffer = MeshBuffer::create(geoqikSettings);
  scene.m_geomBufferGrowthFactor = std::max<std::size_t>(2, geoqikSettings.capacityGrowthFactor);
  scene.m_pointSize = geoqikSettings.defaultPointSize;
  scene.m_lineWidth = geoqikSettings.defaultLineWidth;
  return scene;
}

bool Scene::ensure_point_capacity(std::size_t pointCount)
{
  if (m_pointBuffer->has_space_for_points(pointCount))
  {
    return false;
  }

  const std::size_t growthFactor =
      calc_growth_factor(m_pointBuffer->get_point_capacity(), m_pointBuffer->get_free_point_capacity(), pointCount);
  m_pointBuffer = PointBuffer::create_from(*m_pointBuffer, growthFactor);
  return true;
}

bool Scene::ensure_line_capacity(std::size_t lineCount)
{
  if (m_lineBuffer->has_space_for_lines(lineCount))
  {
    return false;
  }

  const std::size_t growthFactor =
      calc_growth_factor(m_lineBuffer->get_line_capacity(), m_lineBuffer->get_free_line_capacity(), lineCount);
  m_lineBuffer = LineBuffer::create_from(*m_lineBuffer, growthFactor);
  return true;
}

void Scene::add_point(float x, float y, float z, const core::UUID* handle)
{
  m_pointBuffer->add_point(x, y, z, handle);
}

void Scene::add_point(float x, float y, float z, float r, float g, float b, float a, const core::UUID* handle)
{
  m_pointBuffer->add_point(x, y, z, r, g, b, a, handle);
}

void Scene::add_points(std::span<const float> points, std::span<const float> colors, const core::UUID* handle)
{
  m_pointBuffer->add_points(points, colors, handle);
}

bool Scene::update_point(core::UUID handle, float x, float y, float z, std::span<const float> colors)
{
  return m_pointBuffer->update_point(handle, x, y, z, colors);
}

bool Scene::update_points(core::UUID handle, std::span<const float> points, std::span<const float> colors)
{
  return m_pointBuffer->update_points(handle, points, colors);
}

void Scene::remove_point(core::UUID handle)
{
  m_pointBuffer->remove_point(handle);
}

void Scene::add_line(float x1, float y1, float z1, float x2, float y2, float z2, const core::UUID* handle)
{
  m_lineBuffer->add_line(x1, y1, z1, x2, y2, z2, handle);
}

void Scene::add_line(float x1, float y1, float z1, float x2, float y2, float z2, float r, float g, float b, float a, const core::UUID* handle)
{
  m_lineBuffer->add_line(x1, y1, z1, x2, y2, z2, r, g, b, a, handle);
}

void Scene::add_lines(std::span<const float> lines, std::span<const float> colors, const core::UUID* handle)
{
  m_lineBuffer->add_lines(lines, colors, handle);
}

bool Scene::update_line(core::UUID handle,
                        float x1,
                        float y1,
                        float z1,
                        float x2,
                        float y2,
                        float z2,
                        std::span<const float> colors)
{
  return m_lineBuffer->update_line(handle, x1, y1, z1, x2, y2, z2, colors);
}

bool Scene::update_lines(core::UUID handle, std::span<const float> lines, std::span<const float> colors)
{
  return m_lineBuffer->update_lines(handle, lines, colors);
}

void Scene::remove_line(core::UUID handle)
{
  m_lineBuffer->remove_line(handle);
}

void Scene::add_mesh(std::span<const float> vertices,
                     std::span<const float> normals,
                     std::span<const float> colors,
                     std::span<const std::uint32_t> triangleIndices,
                     const core::UUID* handle)
{
  m_meshBuffer->add_mesh(vertices, normals, colors, triangleIndices, handle);
}

void Scene::remove_mesh(core::UUID handle)
{
  m_meshBuffer->remove_mesh(handle);
}

bool Scene::update_mesh(core::UUID handle,
                        std::span<const float> vertices,
                        std::span<const float> normals,
                        std::span<const float> colors)
{
  return m_meshBuffer->update_mesh(handle, vertices, normals, colors);
}

void Scene::translate_geometry(core::UUID handle, float dx, float dy, float dz)
{
  m_pointBuffer->translate_geometry(handle, dx, dy, dz);
  m_lineBuffer->translate_geometry(handle, dx, dy, dz);
}

void Scene::rotate_geometry(core::UUID handle, float centerX, float centerY, float centerZ, float axisX, float axisY, float axisZ, float angle)
{
  m_pointBuffer->rotate_geometry(handle, centerX, centerY, centerZ, axisX, axisY, axisZ, angle);
  m_lineBuffer->rotate_geometry(handle, centerX, centerY, centerZ, axisX, axisY, axisZ, angle);
}

void Scene::clear()
{
  m_pointBuffer->clear();
  m_lineBuffer->clear();
  m_meshBuffer->clear();
}

SceneSnapshot Scene::create_snapshot() const
{
  SceneSnapshot snapshot;
  snapshot.pointBuffer = m_pointBuffer->create_snapshot();
  snapshot.lineBuffer = m_lineBuffer->create_snapshot();
  snapshot.pointSize = m_pointSize;
  snapshot.lineWidth = m_lineWidth;
  return snapshot;
}

void Scene::restore_snapshot(const SceneSnapshot& snapshot)
{
  m_pointBuffer->restore_snapshot(snapshot.pointBuffer);
  m_lineBuffer->restore_snapshot(snapshot.lineBuffer);
  m_pointSize = snapshot.pointSize;
  m_lineWidth = snapshot.lineWidth;
}

std::optional<PointBufferGeometry> Scene::get_point_geometry(core::UUID handle) const
{
  return m_pointBuffer->get_geometry(handle);
}

std::optional<LineBufferGeometry> Scene::get_line_geometry(core::UUID handle) const
{
  return m_lineBuffer->get_geometry(handle);
}

Color Scene::get_default_point_color() const
{
  return m_pointBuffer->get_default_point_color();
}

void Scene::set_default_point_color(float r, float g, float b, float a)
{
  m_pointBuffer->set_default_point_color(r, g, b, a);
}

Color Scene::get_default_line_color() const
{
  return m_lineBuffer->get_default_color();
}

void Scene::set_default_line_color(float r, float g, float b, float a)
{
  m_lineBuffer->set_default_color(r, g, b, a);
}

Color Scene::get_default_mesh_color() const
{
  return m_meshBuffer->get_default_color();
}

void Scene::set_default_mesh_color(float r, float g, float b, float a)
{
  m_meshBuffer->set_default_color(r, g, b, a);
}

Geometry::Sphere<float> Scene::calc_bounding_sphere(const linal::float3& center) const
{
  float maxRadiusSq = 0.0F;

  std::span<const float> points = m_pointBuffer->get_points();
  if (!points.empty())
  {
    calc_max_radius_squared(points, center, maxRadiusSq);
  }

  std::span<const float> lines = m_lineBuffer->get_lines();
  if (!lines.empty())
  {
    calc_max_radius_squared(lines, center, maxRadiusSq);
  }

  std::span<const float> meshVertices = m_meshBuffer->get_vertices();
  if (!meshVertices.empty())
  {
    calc_max_radius_squared(meshVertices, center, maxRadiusSq);
  }

  return Geometry::Sphere<float>{center, std::sqrt(maxRadiusSq)};
}

std::size_t Scene::calc_growth_factor(std::size_t currentCapacity, std::size_t freeCapacity, std::size_t requestedCount) const
{
  if (currentCapacity == 0)
  {
    return m_geomBufferGrowthFactor;
  }

  const std::size_t usedCapacity = currentCapacity - freeCapacity;
  const std::size_t requiredCapacity = usedCapacity + requestedCount;
  const std::size_t targetCapacity = std::max(m_geomBufferGrowthFactor * currentCapacity, requiredCapacity);
  const std::size_t additionalCapacity = targetCapacity - currentCapacity;
  return (additionalCapacity + currentCapacity - 1) / currentCapacity;
}

} // namespace geoqik

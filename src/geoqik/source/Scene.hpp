#ifndef GEOQIK_SOURCE_SCENE_HPP
#define GEOQIK_SOURCE_SCENE_HPP

#include "GeometryBuffers/LineBuffer.hpp"
#include "GeometryBuffers/PointBuffer.hpp"
#include "GeometryHelpers.hpp"
#include "GeoQikSettings.hpp"
#include <Geometry/Sphere.hpp>
#include <cmath>
#include <memory>
#include <span>

namespace geoqik
{

class Scene
{
  std::unique_ptr<PointBuffer> m_pointBuffer;
  std::unique_ptr<LineBuffer> m_lineBuffer;
  std::size_t m_geomBufferGrowthFactor{3};
  float m_pointSize{3.0f};
  float m_lineWidth{1.0f};

public:
  Scene() = default;
  Scene(const Scene&) = delete;
  Scene& operator=(const Scene&) = delete;
  Scene(Scene&&) noexcept = default;
  Scene& operator=(Scene&&) noexcept = default;
  ~Scene() = default;

  static Scene create(const GeoQikSettings& geoqikSettings)
  {
    Scene scene;
    scene.m_pointBuffer = PointBuffer::create(geoqikSettings);
    scene.m_pointSize = geoqikSettings.defaultPointSize;
    scene.m_lineBuffer = LineBuffer::create(geoqikSettings);
    scene.m_lineWidth = geoqikSettings.defaultLineWidth;
    return scene;
  }

  [[nodiscard]] bool ensure_point_capacity(std::size_t pointCount)
  {
    if (m_pointBuffer->has_space_for_points(pointCount))
    {
      return false;
    }

    if (pointCount > 0)
    {
      const std::size_t freePointCapacity = m_pointBuffer->get_free_point_capacity();
      const std::size_t minNewPointCapacity = pointCount - freePointCapacity;
      const std::size_t newPointCapacity = m_geomBufferGrowthFactor * m_pointBuffer->get_point_capacity() + minNewPointCapacity;
      const std::size_t growthFactor = newPointCapacity / m_pointBuffer->get_point_capacity();
      m_pointBuffer = PointBuffer::create_from(std::move(*m_pointBuffer), growthFactor);
    }
    else
    {
      m_pointBuffer = PointBuffer::create_from(std::move(*m_pointBuffer), m_geomBufferGrowthFactor);
    }

    return true;
  }

  [[nodiscard]] bool ensure_line_capacity(std::size_t lineCount)
  {
    if (m_lineBuffer->has_space_for_lines(lineCount))
    {
      return false;
    }

    if (lineCount > 0)
    {
      const std::size_t freeLineCapacity = m_lineBuffer->get_free_line_capacity();
      const std::size_t minNewLineCapacity = lineCount - freeLineCapacity;
      const std::size_t newLineCapacity = m_geomBufferGrowthFactor * m_lineBuffer->get_line_capacity() + minNewLineCapacity;
      const std::size_t growthFactor = newLineCapacity / m_lineBuffer->get_line_capacity();
      m_lineBuffer = LineBuffer::create_from(std::move(*m_lineBuffer), growthFactor);
    }
    else
    {
      m_lineBuffer = LineBuffer::create_from(std::move(*m_lineBuffer), m_geomBufferGrowthFactor);
    }

    return true;
  }

  void add_point(float x, float y, float z, const core::UUID* handle = nullptr) { m_pointBuffer->add_point(x, y, z, handle); }

  void add_point(float x, float y, float z, float r, float g, float b, float a, const core::UUID* handle = nullptr)
  {
    m_pointBuffer->add_point(x, y, z, r, g, b, a, handle);
  }

  void add_points(std::span<const float> points, std::span<const float> colors, const core::UUID* handle = nullptr)
  {
    m_pointBuffer->add_points(points, colors, handle);
  }

  void remove_point(core::UUID handle) { m_pointBuffer->remove_point(handle); }

  void add_line(float x1, float y1, float z1, float x2, float y2, float z2, const core::UUID* handle = nullptr)
  {
    m_lineBuffer->add_line(x1, y1, z1, x2, y2, z2, handle);
  }

  void add_line(float x1, float y1, float z1, float x2, float y2, float z2, float r, float g, float b, float a, const core::UUID* handle = nullptr)
  {
    m_lineBuffer->add_line(x1, y1, z1, x2, y2, z2, r, g, b, a, handle);
  }

  void add_lines(std::span<const float> lines, std::span<const float> colors, const core::UUID* handle = nullptr)
  {
    m_lineBuffer->add_lines(lines, colors, handle);
  }

  void remove_line(core::UUID handle) { m_lineBuffer->remove_line(handle); }

  void translate_geometry(core::UUID handle, float dx, float dy, float dz)
  {
    m_pointBuffer->translate_geometry(handle, dx, dy, dz);
    m_lineBuffer->translate_geometry(handle, dx, dy, dz);
  }

  void rotate_geometry(core::UUID handle, float centerX, float centerY, float centerZ, float axisX, float axisY, float axisZ, float angle)
  {
    m_pointBuffer->rotate_geometry(handle, centerX, centerY, centerZ, axisX, axisY, axisZ, angle);
    m_lineBuffer->rotate_geometry(handle, centerX, centerY, centerZ, axisX, axisY, axisZ, angle);
  }

  void clear()
  {
    m_pointBuffer->clear();
    m_lineBuffer->clear();
  }

  [[nodiscard]] float get_point_size() const { return m_pointSize; }
  void set_point_size(float pointSize) { m_pointSize = pointSize; }

  [[nodiscard]] Color get_default_point_color() const { return m_pointBuffer->get_default_point_color(); }
  void set_default_point_color(float r, float g, float b, float a) { m_pointBuffer->set_default_point_color(r, g, b, a); }

  [[nodiscard]] float get_line_width() const { return m_lineWidth; }
  void set_line_width(float lineWidth) { m_lineWidth = lineWidth; }

  [[nodiscard]] Color get_default_line_color() const { return m_lineBuffer->get_default_color(); }
  void set_default_line_color(float r, float g, float b, float a) { m_lineBuffer->set_default_color(r, g, b, a); }

  [[nodiscard]] const PointBuffer& get_point_buffer() const { return *m_pointBuffer; }
  [[nodiscard]] PointBuffer& get_point_buffer() { return *m_pointBuffer; }
  [[nodiscard]] const LineBuffer& get_line_buffer() const { return *m_lineBuffer; }
  [[nodiscard]] LineBuffer& get_line_buffer() { return *m_lineBuffer; }

  [[nodiscard]] Geometry::Sphere<float> calc_bounding_sphere(const linal::float3& center) const
  {
    float maxRadiusSq = 0.0f;

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

    return Geometry::Sphere<float>{center, std::sqrt(maxRadiusSq)};
  }
};

} // namespace geoqik

#endif // GEOQIK_SOURCE_SCENE_HPP

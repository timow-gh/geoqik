#ifndef GEOQIK_SOURCE_SCENE_HPP
#define GEOQIK_SOURCE_SCENE_HPP

#include "GeometryBuffers/LineBuffer.hpp"
#include "GeometryBuffers/PointBuffer.hpp"
#include "GeoQikSettings.hpp"
#include <Geometry/Sphere.hpp>
#include <cstddef>
#include <memory>
#include <span>

namespace geoqik
{

class Scene
{
  std::unique_ptr<PointBuffer> m_pointBuffer;
  std::unique_ptr<LineBuffer> m_lineBuffer;
  std::size_t m_geomBufferGrowthFactor{2};
  float m_pointSize{3.0f};
  float m_lineWidth{1.0f};

public:
  Scene() = default;
  Scene(const Scene&) = delete;
  Scene& operator=(const Scene&) = delete;
  Scene(Scene&&) noexcept = default;
  Scene& operator=(Scene&&) noexcept = default;
  ~Scene() = default;

  [[nodiscard]] static Scene create(const GeoQikSettings& geoqikSettings);

  [[nodiscard]] bool ensure_point_capacity(std::size_t pointCount);
  [[nodiscard]] bool ensure_line_capacity(std::size_t lineCount);

  void add_point(float x, float y, float z, const core::UUID* handle = nullptr);
  void add_point(float x, float y, float z, float r, float g, float b, float a, const core::UUID* handle = nullptr);
  void add_points(std::span<const float> points, std::span<const float> colors, const core::UUID* handle = nullptr);
  void remove_point(core::UUID handle);

  void add_line(float x1, float y1, float z1, float x2, float y2, float z2, const core::UUID* handle = nullptr);
  void add_line(float x1, float y1, float z1, float x2, float y2, float z2, float r, float g, float b, float a, const core::UUID* handle = nullptr);
  void add_lines(std::span<const float> lines, std::span<const float> colors, const core::UUID* handle = nullptr);
  void remove_line(core::UUID handle);

  void translate_geometry(core::UUID handle, float dx, float dy, float dz);
  void rotate_geometry(core::UUID handle, float centerX, float centerY, float centerZ, float axisX, float axisY, float axisZ, float angle);

  void clear();

  [[nodiscard]] float get_point_size() const { return m_pointSize; }
  void set_point_size(float pointSize) { m_pointSize = pointSize; }

  [[nodiscard]] Color get_default_point_color() const;
  void set_default_point_color(float r, float g, float b, float a);

  [[nodiscard]] float get_line_width() const { return m_lineWidth; }
  void set_line_width(float lineWidth) { m_lineWidth = lineWidth; }

  [[nodiscard]] Color get_default_line_color() const;
  void set_default_line_color(float r, float g, float b, float a);

  [[nodiscard]] const PointBuffer& get_point_buffer() const { return *m_pointBuffer; }
  [[nodiscard]] PointBuffer& get_point_buffer() { return *m_pointBuffer; }
  [[nodiscard]] const LineBuffer& get_line_buffer() const { return *m_lineBuffer; }
  [[nodiscard]] LineBuffer& get_line_buffer() { return *m_lineBuffer; }

  [[nodiscard]] Geometry::Sphere<float> calc_bounding_sphere(const linal::float3& center) const;

private:
  [[nodiscard]] std::size_t calc_growth_factor(std::size_t currentCapacity, std::size_t freeCapacity, std::size_t requestedCount) const;
};

} // namespace geoqik

#endif // GEOQIK_SOURCE_SCENE_HPP

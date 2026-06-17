#ifndef GEOQIK_SOURCE_SCENE_HPP
#define GEOQIK_SOURCE_SCENE_HPP

#include "GeometryBuffers/LineBuffer.hpp"
#include "GeometryBuffers/MeshBuffer.hpp"
#include "GeometryBuffers/PointBuffer.hpp"
#include "GeoQikSettings.hpp"
#include <Geometry/Sphere.hpp>
#include <cstddef>
#include <memory>
#include <optional>
#include <span>

namespace geoqik
{

struct SceneSnapshot
{
  PointBufferSnapshot pointBuffer;
  LineBufferSnapshot lineBuffer;
  float pointSize{3.0f};
  float lineWidth{1.0f};
  // Mesh buffer state is intentionally not included. The serialization follow-up plan must add MeshBufferSnapshot here.
};

class Scene
{
  std::unique_ptr<PointBuffer> m_pointBuffer;
  std::unique_ptr<LineBuffer> m_lineBuffer;
  std::unique_ptr<MeshBuffer> m_meshBuffer;
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
  [[nodiscard]] bool update_point(core::UUID handle, float x, float y, float z, std::span<const float> colors = {});
  [[nodiscard]] bool update_points(core::UUID handle, std::span<const float> points, std::span<const float> colors = {});
  void remove_point(core::UUID handle);

  void add_line(float x1, float y1, float z1, float x2, float y2, float z2, const core::UUID* handle = nullptr);
  void add_line(float x1, float y1, float z1, float x2, float y2, float z2, float r, float g, float b, float a, const core::UUID* handle = nullptr);
  void add_lines(std::span<const float> lines, std::span<const float> colors, const core::UUID* handle = nullptr);
  [[nodiscard]] bool update_line(core::UUID handle,
                                 float x1,
                                 float y1,
                                 float z1,
                                 float x2,
                                 float y2,
                                 float z2,
                                 std::span<const float> colors = {});
  [[nodiscard]] bool update_lines(core::UUID handle, std::span<const float> lines, std::span<const float> colors = {});
  void remove_line(core::UUID handle);

  void add_mesh(std::span<const float> vertices,
                std::span<const float> normals,
                std::span<const float> colors,
                std::span<const std::uint32_t> triangleIndices,
                const core::UUID* handle = nullptr);
  void remove_mesh(core::UUID handle);

  [[nodiscard]] const MeshBuffer& get_mesh_buffer() const { return *m_meshBuffer; }
  [[nodiscard]] MeshBuffer& get_mesh_buffer() { return *m_meshBuffer; }

  void translate_geometry(core::UUID handle, float dx, float dy, float dz);
  void rotate_geometry(core::UUID handle, float centerX, float centerY, float centerZ, float axisX, float axisY, float axisZ, float angle);

  void clear();

  [[nodiscard]] SceneSnapshot create_snapshot() const;
  void restore_snapshot(const SceneSnapshot& snapshot);
  [[nodiscard]] std::optional<PointBufferGeometry> get_point_geometry(core::UUID handle) const;
  [[nodiscard]] std::optional<LineBufferGeometry> get_line_geometry(core::UUID handle) const;

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

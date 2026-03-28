#ifndef GEOQIK_SOURCE_SCENE_HPP
#define GEOQIK_SOURCE_SCENE_HPP

#include "GeometryBuffer.hpp"
#include "OpenGL/BufferAccessPattern.hpp"
#include "OpenGLDrawablesManager.hpp"
#include <Geometry/ExtremePointsInDirection.hpp>
#include <Geometry/Sphere.hpp>
#include <linal/vec.hpp>

namespace geoqik
{

class Scene
{
  std::unique_ptr<GeometryBuffer> m_geoBuffer;
  std::size_t m_geomBufferGrowthFactor{3};
  OpenGLDrawablesManager m_drawablesManager;
  float m_pointSize{3.0f};
  float m_lineWidth{1.0f};

  opengl::LineType m_lineType{opengl::LineType::lines()};
  // Access pattern to be used for the OpenGL buffers
  opengl::BufferAccessPattern m_accessPattern{opengl::BufferAccessPattern::STREAM_DRAW};

public:
  Scene() = default;
  Scene(const Scene&) = delete;
  Scene& operator=(const Scene&) = delete;
  Scene(Scene&& other) noexcept
      : m_geoBuffer(std::move(other.m_geoBuffer))
      , m_drawablesManager(std::move(other.m_drawablesManager))
      , m_lineType(other.m_lineType)
      , m_accessPattern(other.m_accessPattern)
  {
    m_pointSize = other.m_pointSize;
    m_lineWidth = other.m_lineWidth;
    other.m_geoBuffer = nullptr;
  }
  Scene& operator=(Scene&& other) noexcept
  {
    m_geoBuffer = std::move(other.m_geoBuffer);
    m_drawablesManager = std::move(other.m_drawablesManager);
    m_pointSize = other.m_pointSize;
    m_lineWidth = other.m_lineWidth;
    m_lineType = other.m_lineType;
    m_accessPattern = other.m_accessPattern;
    return *this;
  }
  ~Scene() = default;

  static Scene create(const GeoQikSettings& geoqikSettings, opengl::PointProgram* pointProgram, opengl::LineProgram* lineProgram)
  {
    Scene scene(pointProgram, lineProgram);
    scene.m_geoBuffer = GeometryBuffer::create(geoqikSettings);
    scene.m_pointSize = geoqikSettings.defaultPointSize;
    scene.m_lineWidth = geoqikSettings.defaultLineWidth;
    return scene;
  }

  void add_point(float x, float y, float z, const core::UUID* handle = nullptr)
  {
    if (m_geoBuffer->has_space_for_points(1) == false)
    {
      recreated_drawables();
      m_geoBuffer->add_point(x, y, z, handle);
      return;
    }
    m_geoBuffer->add_point(x, y, z, handle);
  }

  void add_point(float x, float y, float z, float r, float g, float b, const core::UUID* handle = nullptr)
  {
    if (m_geoBuffer->has_space_for_points(1) == false)
    {
      recreated_drawables();
      m_geoBuffer->add_point(x, y, z, r, g, b, handle);
      return;
    }
    m_geoBuffer->add_point(x, y, z, r, g, b, handle);
  }

  void add_points(std::span<const float> points, std::span<const float> colors, const core::UUID* handle = nullptr)
  {
    if (m_geoBuffer->has_space_for_points(points.size() / 3) == false)
    {
      recreated_drawables(points.size() / 3);
      m_geoBuffer->add_points(points, colors, handle);
      return;
    }
    m_geoBuffer->add_points(points, colors, handle);
  }

  void remove_point(core::UUID handle) { m_geoBuffer->remove_point(handle); }

  void add_line(float x1, float y1, float z1, float x2, float y2, float z2, const core::UUID* handle = nullptr)
  {
    if (m_geoBuffer->has_space_for_lines(1) == false)
    {
      recreated_drawables();
      m_geoBuffer->add_line(x1, y1, z1, x2, y2, z2, handle);
      return;
    }
    m_geoBuffer->add_line(x1, y1, z1, x2, y2, z2, handle);
  }

  void add_line(float x1, float y1, float z1, float x2, float y2, float z2, float r, float g, float b, const core::UUID* handle = nullptr)
  {
    if (m_geoBuffer->has_space_for_lines(1) == false)
    {
      recreated_drawables();
      m_geoBuffer->add_line(x1, y1, z1, x2, y2, z2, r, g, b, handle);
      return;
    }
    m_geoBuffer->add_line(x1, y1, z1, x2, y2, z2, r, g, b, handle);
  }

  void remove_line(core::UUID handle) { m_geoBuffer->remove_line(handle); }

  void translate_geometry(core::UUID handle, float dx, float dy, float dz) { m_geoBuffer->translate_geometry(handle, dx, dy, dz); }
  void rotate_geometry(core::UUID handle, float centerX, float centerY, float centerZ, float axisX, float axisY, float axisZ, float angle)
  {
    m_geoBuffer->rotate_geometry(handle, centerX, centerY, centerZ, axisX, axisY, axisZ, angle);
  }

  [[nodiscard]] const OpenGLDrawablesManager& get_drawables_manager() const { return m_drawablesManager; }

  [[nodiscard]] const GeometryBuffer& get_geometry_buffer() const { return *m_geoBuffer; }

  void clear()
  {
    m_geoBuffer->clear();
    m_drawablesManager.clear_drawables();
  }

  void draw(const linal::hmatf& mvp) const
  {
    m_drawablesManager.draw_lines(mvp);
    m_drawablesManager.draw_points(mvp);
  }

  float get_point_size() const { return m_pointSize; }
  void set_point_size(float pointSize) { m_pointSize = pointSize; }

  std::array<float, 3> get_point_color() const { return m_geoBuffer->get_point_color(); }
  void set_point_color(float r, float g, float b) { m_geoBuffer->set_point_color(r, g, b); }

  float get_line_width() const { return m_lineWidth; }
  void set_line_width(float lineWidth) { m_lineWidth = lineWidth; }

  std::array<float, 3> get_line_color() const { return m_geoBuffer->get_line_color(); }
  void set_line_color(float r, float g, float b) { m_geoBuffer->set_line_color(r, g, b); }

  void create_point_drawable()
  {
    if (m_geoBuffer->get_points().empty())
    {
      return;
    }

    m_drawablesManager.add_point_drawable(m_geoBuffer->get_points(),
                                          m_geoBuffer->get_point_dimension(),
                                          m_geoBuffer->get_point_colors(),
                                          m_geoBuffer->get_color_dimension(),
                                          m_geoBuffer->get_point_indices(),
                                          m_pointSize,
                                          opengl::BufferAccessPattern::STATIC_DRAW);
  }

  void create_line_drawable()
  {
    if (m_geoBuffer->get_lines().empty())
    {
      return;
    }

    m_drawablesManager.add_line_drawable(m_geoBuffer->get_lines(),
                                         m_geoBuffer->get_point_dimension(),
                                         m_geoBuffer->get_line_indices(),
                                         m_geoBuffer->get_line_colors(),
                                         m_geoBuffer->get_color_dimension(),
                                         m_lineType,
                                         m_lineWidth,
                                         m_pointSize,
                                         opengl::BufferAccessPattern::STATIC_DRAW);
  }

  /** \brief Update the drawable buffers if the geometry buffer has changed
   *
   * \return true if the drawable buffers were updated, false otherwise.
   */
  bool update_drawable_buffers()
  {
    bool updateOccurred = false;

    if (!m_drawablesManager.has_point_drawables() && !m_geoBuffer->get_points().empty())
    {
      create_point_drawable();
      updateOccurred = true;
    }
    else if (m_geoBuffer->points_have_changed())
    {
      m_drawablesManager.update_last_point_drawable(m_geoBuffer->get_points(),
                                                    m_geoBuffer->get_point_colors(),
                                                    m_geoBuffer->get_point_indices(),
                                                    opengl::BufferAccessPattern::STATIC_DRAW);
      m_geoBuffer->reset_points_have_changed();
      updateOccurred = true;
    }

    if (!m_drawablesManager.has_line_drawables() && !m_geoBuffer->get_lines().empty())
    {
      create_line_drawable();
      updateOccurred = true;
    }
    else if (m_geoBuffer->lines_have_changed())
    {
      m_drawablesManager.update_last_line_drawable(m_geoBuffer->get_lines(),
                                                   m_geoBuffer->get_line_colors(),
                                                   m_geoBuffer->get_line_indices(),
                                                   opengl::BufferAccessPattern::STATIC_DRAW);
      m_geoBuffer->reset_lines_have_changed();
      updateOccurred = true;
    }

    return updateOccurred;
  }

  void clear_drawables() { m_drawablesManager.clear_drawables(); }

  Geometry::Sphere<float> calc_bounding_sphere(const linal::float3& center) const
  {
    float maxRadiusSq = 0.0f;

    std::span<const float> points = m_geoBuffer->get_points();
    if (!points.empty())
    {
      calc_max_radius_squared(points, center, maxRadiusSq);
    }

    std::span<const float> lines = m_geoBuffer->get_lines();
    if (!lines.empty())
    {
      calc_max_radius_squared(lines, center, maxRadiusSq);
    }

    return Geometry::Sphere<float>{center, std::sqrt(maxRadiusSq)};
  }

private:
  Scene(opengl::PointProgram* pointProgram, opengl::LineProgram* lineProgram)
      : m_drawablesManager(pointProgram, lineProgram)
  {
  }

  void recreated_drawables(std::size_t numberOfNewElements = 0)
  {
    clear_drawables();

    if (numberOfNewElements > 0)
    {
      // Calc the new growth factor based on the current capacity and the missing capacity
      std::size_t freePointCapacity = m_geoBuffer->get_free_point_capacity();
      std::size_t freeLineCapacity = m_geoBuffer->get_free_line_capacity();
      std::size_t minNewPointCapacity = numberOfNewElements - freePointCapacity;
      std::size_t minNewLineCapacity = numberOfNewElements - freeLineCapacity;
      std::size_t newPointCapacity = m_geomBufferGrowthFactor * m_geoBuffer->get_point_capacity() + minNewPointCapacity;
      std::size_t newLineCapacity = m_geomBufferGrowthFactor * m_geoBuffer->get_line_capacity() + minNewLineCapacity;
      std::size_t growthFactor = std::max(newPointCapacity / m_geoBuffer->get_point_capacity(), newLineCapacity / m_geoBuffer->get_line_capacity());
      m_geoBuffer = GeometryBuffer::create_from(std::move(*m_geoBuffer), growthFactor);
    }
    else
    {
      m_geoBuffer = GeometryBuffer::create_from(std::move(*m_geoBuffer), m_geomBufferGrowthFactor);
    }

    create_point_drawable();
    create_line_drawable();
  }

  void calc_max_radius_squared(std::span<const float> points, const linal::float3& center, float& maxRadiusSq) const
  {
    for (std::size_t i = 0; i < points.size(); i += 3)
    {
      linal::float3 point{points[i], points[i + 1], points[i + 2]};
      float distSq = linal::length_squared(center - point);
      if (distSq > maxRadiusSq)
      {
        maxRadiusSq = distSq;
      }
    }
  }
};

} // namespace geoqik

#endif // GEOQIK_SOURCE_SCENE_HPP
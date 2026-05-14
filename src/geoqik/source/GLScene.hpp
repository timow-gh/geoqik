#ifndef GEOQIK_SOURCE_GLSCENE_HPP
#define GEOQIK_SOURCE_GLSCENE_HPP

#include "Scene.hpp"
#include "OpenGL/BufferAccessPattern.hpp"
#include "OpenGL/Drawable/DrawablesManager.hpp"
#include <Geometry/Sphere.hpp>
#include <linal/hmat.hpp>
#include <linal/vec.hpp>
#include <span>

namespace geoqik
{

class GLScene
{
  Scene m_scene;
  opengl::DrawablesManager m_drawablesManager;
  opengl::LineType m_lineType{opengl::LineType::lines()};

public:
  GLScene() = default;
  GLScene(const GLScene&) = delete;
  GLScene& operator=(const GLScene&) = delete;
  GLScene(GLScene&&) noexcept = default;
  GLScene& operator=(GLScene&&) noexcept = default;
  ~GLScene() = default;

  static GLScene create(const GeoQikSettings& geoqikSettings, opengl::PointProgram* pointProgram, opengl::LineProgram* lineProgram)
  {
    return GLScene(Scene::create(geoqikSettings), pointProgram, lineProgram);
  }

  void add_point(float x, float y, float z, const core::UUID* handle = nullptr)
  {
    if (m_scene.ensure_point_capacity(1))
    {
      recreate_point_drawables();
    }
    m_scene.add_point(x, y, z, handle);
  }

  void add_point(float x, float y, float z, float r, float g, float b, float a, const core::UUID* handle = nullptr)
  {
    if (m_scene.ensure_point_capacity(1))
    {
      recreate_point_drawables();
    }
    m_scene.add_point(x, y, z, r, g, b, a, handle);
  }

  void add_points(std::span<const float> points, std::span<const float> colors, const core::UUID* handle = nullptr)
  {
    if (m_scene.ensure_point_capacity(points.size() / 3))
    {
      recreate_point_drawables();
    }
    m_scene.add_points(points, colors, handle);
  }

  void remove_point(core::UUID handle) { m_scene.remove_point(handle); }

  void add_line(float x1, float y1, float z1, float x2, float y2, float z2, const core::UUID* handle = nullptr)
  {
    if (m_scene.ensure_line_capacity(1))
    {
      recreated_line_drawables();
    }
    m_scene.add_line(x1, y1, z1, x2, y2, z2, handle);
  }

  void add_line(float x1, float y1, float z1, float x2, float y2, float z2, float r, float g, float b, float a, const core::UUID* handle = nullptr)
  {
    if (m_scene.ensure_line_capacity(1))
    {
      recreated_line_drawables();
    }
    m_scene.add_line(x1, y1, z1, x2, y2, z2, r, g, b, a, handle);
  }

  void add_lines(std::span<const float> lines, std::span<const float> colors, const core::UUID* handle = nullptr)
  {
    if (m_scene.ensure_line_capacity(lines.size() / 6))
    {
      recreated_line_drawables();
    }
    m_scene.add_lines(lines, colors, handle);
  }

  void remove_line(core::UUID handle) { m_scene.remove_line(handle); }

  void translate_geometry(core::UUID handle, float dx, float dy, float dz) { m_scene.translate_geometry(handle, dx, dy, dz); }

  void rotate_geometry(core::UUID handle, float centerX, float centerY, float centerZ, float axisX, float axisY, float axisZ, float angle)
  {
    m_scene.rotate_geometry(handle, centerX, centerY, centerZ, axisX, axisY, axisZ, angle);
  }

  [[nodiscard]] const opengl::DrawablesManager& get_drawables_manager() const { return m_drawablesManager; }

  [[nodiscard]] const LineBuffer& get_geometry_buffer() const { return m_scene.get_line_buffer(); }

  void clear()
  {
    m_scene.clear();
    m_drawablesManager.clear_drawables();
  }

  void draw(const linal::hmatf& mvp, const linal::double3& viewPosition) { m_drawablesManager.draw_lines_and_points(mvp, viewPosition); }

  [[nodiscard]] float get_point_size() const { return m_scene.get_point_size(); }
  void set_point_size(float pointSize) { m_scene.set_point_size(pointSize); }

  [[nodiscard]] Color get_default_point_color() const { return m_scene.get_default_point_color(); }
  void set_default_point_color(float r, float g, float b, float a) { m_scene.set_default_point_color(r, g, b, a); }

  [[nodiscard]] float get_line_width() const { return m_scene.get_line_width(); }
  void set_line_width(float lineWidth) { m_scene.set_line_width(lineWidth); }

  [[nodiscard]] Color get_default_line_color() const { return m_scene.get_default_line_color(); }
  void set_default_line_color(float r, float g, float b, float a) { m_scene.set_default_line_color(r, g, b, a); }

  void create_point_drawable()
  {
    const auto& pointBuffer = m_scene.get_point_buffer();
    if (pointBuffer.get_points().empty())
    {
      return;
    }

    m_drawablesManager.add_point_drawable(pointBuffer.get_points(),
                                          pointBuffer.get_point_dimension(),
                                          pointBuffer.get_point_colors(),
                                          pointBuffer.get_color_dimension(),
                                          pointBuffer.get_point_indices(),
                                          m_scene.get_point_size(),
                                          opengl::BufferAccessPattern::STATIC_DRAW);
  }

  void create_line_drawable()
  {
    const auto& lineBuffer = m_scene.get_line_buffer();
    if (lineBuffer.get_lines().empty())
    {
      return;
    }

    m_drawablesManager.add_line_drawable(lineBuffer.get_lines(),
                                         lineBuffer.get_point_dimension(),
                                         lineBuffer.get_line_indices(),
                                         lineBuffer.get_line_colors(),
                                         lineBuffer.get_color_dimension(),
                                         m_lineType,
                                         m_scene.get_line_width(),
                                         m_scene.get_point_size(),
                                         opengl::BufferAccessPattern::STATIC_DRAW);
  }

  /** \brief Update the drawable buffers if the geometry buffer has changed
   *
   * \return true if the drawable buffers were updated, false otherwise.
   */
  bool update_drawable_buffers()
  {
    bool updateOccurred = false;
    auto& pointBuffer = m_scene.get_point_buffer();
    auto& lineBuffer = m_scene.get_line_buffer();

    if (!m_drawablesManager.has_point_drawables() && !pointBuffer.get_points().empty())
    {
      create_point_drawable();
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
      create_line_drawable();
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

  void clear_drawables() { m_drawablesManager.clear_drawables(); }

  [[nodiscard]] Geometry::Sphere<float> calc_bounding_sphere(const linal::float3& center) const
  {
    return m_scene.calc_bounding_sphere(center);
  }

private:
  GLScene(Scene&& scene, opengl::PointProgram* pointProgram, opengl::LineProgram* lineProgram)
      : m_scene(std::move(scene))
      , m_drawablesManager(pointProgram, lineProgram)
  {
  }

  void recreated_line_drawables()
  {
    clear_drawables();
    create_point_drawable();
    create_line_drawable();
  }

  void recreate_point_drawables()
  {
    clear_drawables();
    create_point_drawable();
  }
};

} // namespace geoqik

#endif // GEOQIK_SOURCE_GLSCENE_HPP

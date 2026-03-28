#ifndef OPENGLDRAWABLESMANAGER_HPP
#define OPENGLDRAWABLESMANAGER_HPP

#include <OpenGL/Drawable/LineDrawable.hpp>
#include <OpenGL/Drawable/MeshDrawable.hpp>
#include <OpenGL/Drawable/PointDrawable.hpp>
#include <vector>
#include <linal/hmat.hpp>

namespace geoqik
{

class OpenGLDrawablesManager
{
  opengl::PointProgram* m_pointProgram{nullptr};
  opengl::LineProgram* m_lineProgram{nullptr};
  
  std::vector<opengl::PointDrawable> m_pointDrawables;
  std::vector<opengl::LineDrawable> m_lineDrawables;
  std::vector<opengl::MeshDrawable> m_meshDrawables;

public:
  OpenGLDrawablesManager() = default;
  OpenGLDrawablesManager(opengl::PointProgram* pointProgram, opengl::LineProgram* lineProgram)
      : m_pointProgram(pointProgram)
      , m_lineProgram(lineProgram)
  {
  }

  [[nodiscard]] bool has_drawables() const
  {
    return !m_pointDrawables.empty() || !m_lineDrawables.empty() || !m_meshDrawables.empty();
  }

  [[nodiscard]] bool has_point_drawables() const { return !m_pointDrawables.empty(); }
  [[nodiscard]] bool has_line_drawables() const { return !m_lineDrawables.empty(); }
  [[nodiscard]] bool has_mesh_drawables() const { return !m_meshDrawables.empty(); }

  void add_point_drawable(const opengl::PointDrawable& drawable) { m_pointDrawables.push_back(drawable); }
  void add_line_drawable(const opengl::LineDrawable& drawable) { m_lineDrawables.push_back(drawable); }
  void add_mesh_drawable(const opengl::MeshDrawable& drawable) { m_meshDrawables.push_back(drawable); }

  void add_point_drawable(std::span<const float> vertices,
                          std::int32_t vertexDimension,
                          std::span<const float> colors,
                          std::int32_t colorDimension,
                          std::span<const std::uint32_t> indices,
                          float pointSize,
                          opengl::BufferAccessPattern accessPattern)
  {
    m_pointDrawables.push_back(
        opengl::make_point_drawable(*m_pointProgram, vertices, vertexDimension, colors, colorDimension, indices, pointSize, accessPattern));
  }

  void add_line_drawable(std::span<const float> lineVertices,
                         std::int32_t lineVertexDimension,
                         std::span<const std::uint32_t> lineIndices,
                         std::span<const float> lineColors,
                         std::int32_t colorDimension,
                         opengl::LineType lineType,
                         float lineWidth,
                         float pointSize,
                         opengl::BufferAccessPattern accessPattern)
  {
    m_lineDrawables.push_back(opengl::make_line_drawable(*m_lineProgram,
                                                         lineVertices,
                                                         lineVertexDimension,
                                                         lineIndices,
                                                         lineColors,
                                                         colorDimension,
                                                         lineType,
                                                         lineWidth,
                                                         pointSize,
                                                         accessPattern));
  }

  void update_last_point_drawable(std::span<const float> vertices,
                                  std::span<const float> colors,
                                  std::span<const std::uint32_t> indices,
                                  opengl::BufferAccessPattern accessPattern)
  {
    if (m_pointDrawables.empty())
    {
      return;
    }
    m_pointDrawables.back().update_vertex_buffer(vertices, accessPattern);
    m_pointDrawables.back().update_color_buffer(colors, accessPattern);
    m_pointDrawables.back().update_indices_buffer(indices, accessPattern);
  }

  void update_last_line_drawable(std::span<const float> vertices,
                                 std::span<const float> colors,
                                 std::span<const std::uint32_t> indices,
                                 opengl::BufferAccessPattern accessPattern)
  {
    if (m_lineDrawables.empty())
    {
      return;
    }
    m_lineDrawables.back().update_vertex_buffer(vertices, accessPattern);
    m_lineDrawables.back().update_color_buffer(colors, accessPattern);
    m_lineDrawables.back().update_indices_buffer(indices, accessPattern);
  }

  void clear_point_drawables()
  {
    for (auto& drawable: m_pointDrawables)
    {
      drawable.destroy();
    }
    m_pointDrawables.clear();
  }

  void clear_line_drawables()
  {
    for (auto& drawable: m_lineDrawables)
    {
      drawable.destroy();
    }
    m_lineDrawables.clear();
  }

  void clear_mesh_drawables()
  {
    for (auto& drawable: m_meshDrawables)
    {
      drawable.destroy();
    }
    m_meshDrawables.clear();
  }

  void clear_drawables()
  {
    clear_point_drawables();
    clear_line_drawables();
    clear_mesh_drawables();
  }

  void draw_points(const linal::hmatf& mvp) const
  {
    for (const auto& drawable: m_pointDrawables)
    {
      drawable.draw(mvp);
    }
  }

  void draw_lines(const linal::hmatf& mvp) const
  {
    for (const auto& drawable: m_lineDrawables)
    {
      drawable.draw(mvp);
    }
  }

  void draw_meshes(const linal::hmatf& modelMatrix,
                   const linal::hmatf& viewMatrix,
                   const linal::hmatf& projectionMatrix,
                   const linal::hmatf& normalMatrix,
                   const linal::float3& lightPosition,
                   const linal::float3& viewPos,
                   const linal::float3& lightColor,
                   const linal::float3& ambientColor,
                   float shininess) const
  {
    for (const auto& drawable: m_meshDrawables)
    {
      drawable.draw(modelMatrix, viewMatrix, projectionMatrix, normalMatrix, lightPosition, viewPos, lightColor, ambientColor, shininess);
    }
  }
};

} // namespace geoqik

#endif // OPENGLDRAWABLESMANAGER_HPP
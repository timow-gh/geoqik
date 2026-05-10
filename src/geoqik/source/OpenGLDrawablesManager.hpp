#ifndef OPENGLDRAWABLESMANAGER_HPP
#define OPENGLDRAWABLESMANAGER_HPP

#include <OpenGL/Drawable/LineDrawable.hpp>
#include <OpenGL/Drawable/MeshDrawable.hpp>
#include <OpenGL/Drawable/PointDrawable.hpp>
#include <OpenGL/OpenGL.hpp>
#include <linal/hmat.hpp>
#include <linal/vec.hpp>
#include <algorithm>
#include <cstddef>
#include <vector>

namespace geoqik
{

class OpenGLDrawablesManager
{
  struct ScopedDepthMask
  {
    GLboolean previousValue{GL_TRUE};

    explicit ScopedDepthMask(GLboolean value)
    {
      glGetBooleanv(GL_DEPTH_WRITEMASK, &previousValue);
      glDepthMask(value);
    }

    ~ScopedDepthMask() { glDepthMask(previousValue); }
  };

  struct ScopedCullFaceDisabled
  {
    GLboolean wasEnabled{GL_FALSE};

    ScopedCullFaceDisabled()
        : wasEnabled(glIsEnabled(GL_CULL_FACE))
    {
      glDisable(GL_CULL_FACE);
    }

    ~ScopedCullFaceDisabled()
    {
      if (wasEnabled == GL_TRUE)
      {
        glEnable(GL_CULL_FACE);
      }
      else
      {
        glDisable(GL_CULL_FACE);
      }
    }
  };

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

  [[nodiscard]] bool has_drawables() const { return !m_pointDrawables.empty() || !m_lineDrawables.empty() || !m_meshDrawables.empty(); }

  [[nodiscard]] bool has_point_drawables() const { return !m_pointDrawables.empty(); }
  [[nodiscard]] bool has_line_drawables() const { return !m_lineDrawables.empty(); }
  [[nodiscard]] bool has_mesh_drawables() const { return !m_meshDrawables.empty(); }

  void add_point_drawable(opengl::PointDrawable drawable) { m_pointDrawables.emplace_back(std::move(drawable)); }
  void add_line_drawable(opengl::LineDrawable drawable) { m_lineDrawables.emplace_back(std::move(drawable)); }
  void add_mesh_drawable(opengl::MeshDrawable drawable) { m_meshDrawables.emplace_back(std::move(drawable)); }

  void add_point_drawable(std::span<const float> vertices,
                          std::int32_t vertexDimension,
                          std::span<const float> colors,
                          std::int32_t colorDimension,
                          std::span<const std::uint32_t> indices,
                          float pointSize,
                          opengl::BufferAccessPattern accessPattern)
  {
    auto drawable =
        opengl::make_point_drawable(*m_pointProgram, vertices, vertexDimension, colors, colorDimension, indices, pointSize, accessPattern);
    if (!drawable.has_value())
    {
      throw std::runtime_error("Failed to create point drawable");
    }

    add_point_drawable(std::move(drawable.value()));
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
    auto drawable = opengl::make_line_drawable(*m_lineProgram,
                                               lineVertices,
                                               lineVertexDimension,
                                               lineIndices,
                                               lineColors,
                                               colorDimension,
                                               lineType,
                                               lineWidth,
                                               pointSize,
                                               accessPattern);
    if (!drawable.has_value())
    {
      throw std::runtime_error("Failed to create line drawable");
    }

    add_line_drawable(std::move(drawable.value()));
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

  void clear_point_drawables() { m_pointDrawables.clear(); }

  void clear_line_drawables() { m_lineDrawables.clear(); }

  void clear_mesh_drawables() { m_meshDrawables.clear(); }

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

  void draw_lines_and_points(const linal::hmatf& mvp, const linal::double3& viewPosition)
  {
    struct RenderCommand
    {
      enum class Type
      {
        line,
        point
      };

      Type type{};
      std::size_t index{};
    };

    struct TransparentRenderCommand
    {
      RenderCommand::Type type{};
      std::size_t index{};
      double distanceSquared{};
    };

    struct RenderQueue
    {
      std::vector<RenderCommand> opaqueCommands;
      std::vector<TransparentRenderCommand> transparentCommands;
    };

    RenderQueue renderQueue;
    renderQueue.opaqueCommands.reserve(m_lineDrawables.size() + m_pointDrawables.size());
    renderQueue.transparentCommands.reserve(m_lineDrawables.size() + m_pointDrawables.size());

    for (std::size_t i = 0; i < m_lineDrawables.size(); ++i)
    {
      const auto& drawable = m_lineDrawables[i];
      if (drawable.has_opaque_primitives())
      {
        renderQueue.opaqueCommands.push_back({RenderCommand::Type::line, i});
      }
      if (drawable.has_translucent_primitives())
      {
        renderQueue.transparentCommands.push_back({RenderCommand::Type::line, i, drawable.distance_squared_to(viewPosition)});
      }
    }

    for (std::size_t i = 0; i < m_pointDrawables.size(); ++i)
    {
      const auto& drawable = m_pointDrawables[i];
      if (drawable.has_opaque_primitives())
      {
        renderQueue.opaqueCommands.push_back({RenderCommand::Type::point, i});
      }
      if (drawable.has_translucent_primitives())
      {
        renderQueue.transparentCommands.push_back({RenderCommand::Type::point, i, drawable.distance_squared_to(viewPosition)});
      }
    }

    for (const auto& opaqueCommand: renderQueue.opaqueCommands)
    {
      if (opaqueCommand.type == RenderCommand::Type::line)
      {
        m_lineDrawables[opaqueCommand.index].draw_opaque(mvp);
      }
      else
      {
        m_pointDrawables[opaqueCommand.index].draw_opaque(mvp);
      }
    }

    std::sort(renderQueue.transparentCommands.begin(),
              renderQueue.transparentCommands.end(),
              [](const TransparentRenderCommand& lhs, const TransparentRenderCommand& rhs)
              { return lhs.distanceSquared > rhs.distanceSquared; });

    if (renderQueue.transparentCommands.empty())
    {
      return;
    }

    const ScopedDepthMask depthMask(GL_FALSE);
    for (const auto& transparentCommand: renderQueue.transparentCommands)
    {
      if (transparentCommand.type == RenderCommand::Type::line)
      {
        m_lineDrawables[transparentCommand.index].draw_translucent(mvp, viewPosition);
      }
      else
      {
        m_pointDrawables[transparentCommand.index].draw_translucent(mvp, viewPosition);
      }
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
    struct TransparentMesh
    {
      std::size_t index{};
      double distanceSquared{};
    };

    const linal::double3 viewPositionDouble{static_cast<double>(viewPos[0]), static_cast<double>(viewPos[1]), static_cast<double>(viewPos[2])};
    std::vector<TransparentMesh> transparentMeshes;
    transparentMeshes.reserve(m_meshDrawables.size());

    for (std::size_t i = 0; i < m_meshDrawables.size(); ++i)
    {
      const auto& drawable = m_meshDrawables[i];
      if (drawable.is_translucent())
      {
        transparentMeshes.push_back({i, drawable.distance_squared_to(viewPositionDouble)});
      }
      else
      {
        drawable.draw(modelMatrix, viewMatrix, projectionMatrix, normalMatrix, lightPosition, viewPos, lightColor, ambientColor, shininess);
      }
    }

    std::sort(transparentMeshes.begin(),
              transparentMeshes.end(),
              [](const TransparentMesh& lhs, const TransparentMesh& rhs) { return lhs.distanceSquared > rhs.distanceSquared; });

    if (transparentMeshes.empty())
    {
      return;
    }

    const ScopedDepthMask depthMask(GL_FALSE);
    const ScopedCullFaceDisabled cullFace;
    for (const auto& transparentMesh: transparentMeshes)
    {
      m_meshDrawables[transparentMesh.index].draw(modelMatrix, viewMatrix, projectionMatrix, normalMatrix, lightPosition, viewPos, lightColor, ambientColor, shininess);
    }
  }
};

} // namespace geoqik

#endif // OPENGLDRAWABLESMANAGER_HPP

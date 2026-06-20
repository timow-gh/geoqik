#ifndef OPENGL_DRAWABLE_DRAWABLESMANAGER_HPP
#define OPENGL_DRAWABLE_DRAWABLESMANAGER_HPP

#include <OpenGL/Drawable/LineDrawable.hpp>
#include <OpenGL/Drawable/MeshDrawable.hpp>
#include <OpenGL/Drawable/PointDrawable.hpp>
#include <OpenGL/Programs/ProgramManager.hpp>
#include <OpenGL/OpenGL.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <linal/hmat.hpp>
#include <linal/vec.hpp>
#include <stdexcept>
#include <utility>
#include <vector>

namespace opengl
{

class DrawablesManager
{
public:
  using DrawableId = std::uint64_t;

private:
  template <typename Drawable>
  struct DrawableEntry
  {
    DrawableId id{};
    Drawable drawable;
  };

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

  ProgramManager programManager;
  DrawableId m_nextDrawableId{1U};

  std::vector<DrawableEntry<opengl::PointDrawable>> m_pointDrawables;
  std::vector<DrawableEntry<opengl::LineDrawable>> m_lineDrawables;
  std::vector<DrawableEntry<opengl::MeshDrawable>> m_meshDrawables;

public:
  DrawablesManager(const DrawablesManager&) = delete;
  DrawablesManager& operator=(const DrawablesManager&) = delete;
  DrawablesManager(DrawablesManager&& other) noexcept
  {
    programManager = std::move(other.programManager);
    m_nextDrawableId = other.m_nextDrawableId;
    m_pointDrawables = std::move(other.m_pointDrawables);
    m_lineDrawables = std::move(other.m_lineDrawables);
    m_meshDrawables = std::move(other.m_meshDrawables);
  }
  DrawablesManager& operator=(DrawablesManager&& other) noexcept
  {
    if (this != &other)
    {
      programManager = std::move(other.programManager);
      m_nextDrawableId = other.m_nextDrawableId;
      m_pointDrawables = std::move(other.m_pointDrawables);
      m_lineDrawables = std::move(other.m_lineDrawables);
      m_meshDrawables = std::move(other.m_meshDrawables);
    }
    return *this;
  }
  ~DrawablesManager()
  {
    clear_drawables();
  }

  [[nodiscard]] static std::optional<DrawablesManager> create()
  {
    ProgramManager programManager;
    programManager.compile();

    if (!programManager.is_compiled())
    {
      return std::nullopt;
    }

    return DrawablesManager(std::move(programManager));
  }

  [[nodiscard]] bool has_drawables() const
  {
    return !m_pointDrawables.empty() || !m_lineDrawables.empty() || !m_meshDrawables.empty();
  }

  [[nodiscard]] bool has_point_drawables() const { return !m_pointDrawables.empty(); }
  [[nodiscard]] bool has_line_drawables() const { return !m_lineDrawables.empty(); }
  [[nodiscard]] bool has_mesh_drawables() const { return !m_meshDrawables.empty(); }

  DrawableId add_point_drawable(opengl::PointDrawable drawable)
  {
    const DrawableId id = next_drawable_id();
    m_pointDrawables.emplace_back(DrawableEntry<opengl::PointDrawable>{id, std::move(drawable)});
    return id;
  }
  DrawableId add_line_drawable(opengl::LineDrawable drawable)
  {
    const DrawableId id = next_drawable_id();
    m_lineDrawables.emplace_back(DrawableEntry<opengl::LineDrawable>{id, std::move(drawable)});
    return id;
  }
  DrawableId add_point_drawable(std::span<const float> vertices,
                                std::int32_t vertexDimension,
                                std::span<const float> colors,
                                std::int32_t colorDimension,
                                std::span<const std::uint32_t> indices,
                                float pointSize,
                                opengl::BufferAccessPattern accessPattern)
  {
    auto drawable = opengl::make_point_drawable(get_point_program(),
                                                vertices,
                                                vertexDimension,
                                                colors,
                                                colorDimension,
                                                indices,
                                                pointSize,
                                                accessPattern);
    if (!drawable.has_value())
    {
      throw std::runtime_error("Failed to create point drawable");
    }

    return add_point_drawable(std::move(drawable.value()));
  }

  DrawableId add_line_drawable(std::span<const float> lineVertices,
                               std::int32_t lineVertexDimension,
                               std::span<const std::uint32_t> lineIndices,
                               std::span<const float> lineColors,
                               std::int32_t colorDimension,
                               opengl::LineType lineType,
                               float lineWidth,
                               float pointSize,
                               opengl::BufferAccessPattern accessPattern)
  {
    auto drawable = opengl::make_line_drawable(get_line_program(),
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

    return add_line_drawable(std::move(drawable.value()));
  }

  DrawableId add_mesh_drawable(std::span<const float> vertices,
                               std::int32_t vertexDimension,
                               std::span<const float> normals,
                               std::span<const float> colors,
                               std::int32_t colorDimension,
                               std::span<const std::uint32_t> triangleIndices,
                               opengl::BufferAccessPattern accessPattern)
  {
    auto drawable = opengl::make_mesh_soup(get_mesh_program(),
                                           vertices,
                                           vertexDimension,
                                           normals,
                                           colors,
                                           colorDimension,
                                           triangleIndices,
                                           accessPattern);
    if (!drawable.has_value())
    {
      throw std::runtime_error("Failed to create mesh drawable");
    }

    const DrawableId id = next_drawable_id();
    m_meshDrawables.emplace_back(DrawableEntry<opengl::MeshDrawable>{id, std::move(drawable.value())});
    return id;
  }

  bool remove_point_drawable(DrawableId id) { return remove_drawable_by_id(m_pointDrawables, id); }

  bool remove_line_drawable(DrawableId id) { return remove_drawable_by_id(m_lineDrawables, id); }

  bool remove_mesh_drawable(DrawableId id) { return remove_drawable_by_id(m_meshDrawables, id); }

  void update_last_point_drawable(std::span<const float> vertices,
                                  std::span<const float> colors,
                                  std::span<const std::uint32_t> indices,
                                  opengl::BufferAccessPattern accessPattern)
  {
    if (m_pointDrawables.empty())
    {
      return;
    }
    m_pointDrawables.back().drawable.update_vertex_buffer(vertices, accessPattern);
    m_pointDrawables.back().drawable.update_color_buffer(colors, accessPattern);
    m_pointDrawables.back().drawable.update_indices_buffer(indices, accessPattern);
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
    m_lineDrawables.back().drawable.update_vertex_buffer(vertices, accessPattern);
    m_lineDrawables.back().drawable.update_color_buffer(colors, accessPattern);
    m_lineDrawables.back().drawable.update_indices_buffer(indices, accessPattern);
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
    for (const auto& entry: m_pointDrawables)
    {
      entry.drawable.draw(mvp);
    }
  }

  void draw_lines(const linal::hmatf& mvp) const
  {
    for (const auto& entry: m_lineDrawables)
    {
      entry.drawable.draw(mvp);
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
      const auto& drawable = m_lineDrawables[i].drawable;
      if (drawable.has_opaque_primitives())
      {
        renderQueue.opaqueCommands.push_back({RenderCommand::Type::line, i});
      }
      if (drawable.has_translucent_primitives())
      {
        renderQueue.transparentCommands
            .push_back({RenderCommand::Type::line, i, drawable.distance_squared_to(viewPosition)});
      }
    }

    for (std::size_t i = 0; i < m_pointDrawables.size(); ++i)
    {
      const auto& drawable = m_pointDrawables[i].drawable;
      if (drawable.has_opaque_primitives())
      {
        renderQueue.opaqueCommands.push_back({RenderCommand::Type::point, i});
      }
      if (drawable.has_translucent_primitives())
      {
        renderQueue.transparentCommands
            .push_back({RenderCommand::Type::point, i, drawable.distance_squared_to(viewPosition)});
      }
    }

    for (const auto& opaqueCommand: renderQueue.opaqueCommands)
    {
      if (opaqueCommand.type == RenderCommand::Type::line)
      {
        m_lineDrawables[opaqueCommand.index].drawable.draw_opaque(mvp);
      }
      else
      {
        m_pointDrawables[opaqueCommand.index].drawable.draw_opaque(mvp);
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
        m_lineDrawables[transparentCommand.index].drawable.draw_translucent(mvp, viewPosition);
      }
      else
      {
        m_pointDrawables[transparentCommand.index].drawable.draw_translucent(mvp, viewPosition);
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
                   const linal::float3& fillLightDirection,
                   const linal::float3& fillLightColor,
                   const linal::float3& ambientColor,
                   float shininess) const
  {
    struct TransparentMesh
    {
      std::size_t index{};
      double distanceSquared{};
    };

    const linal::double3 viewPositionDouble{static_cast<double>(viewPos[0]),
                                            static_cast<double>(viewPos[1]),
                                            static_cast<double>(viewPos[2])};
    std::vector<TransparentMesh> transparentMeshes;
    transparentMeshes.reserve(m_meshDrawables.size());

    for (std::size_t i = 0; i < m_meshDrawables.size(); ++i)
    {
      const auto& drawable = m_meshDrawables[i].drawable;
      if (drawable.is_translucent())
      {
        transparentMeshes.push_back({i, drawable.distance_squared_to(viewPositionDouble)});
      }
      else
      {
        drawable.draw(modelMatrix,
                      viewMatrix,
                      projectionMatrix,
                      normalMatrix,
                      lightPosition,
                      viewPos,
                      lightColor,
                      fillLightDirection,
                      fillLightColor,
                      ambientColor,
                      shininess);
      }
    }

    std::sort(transparentMeshes.begin(),
              transparentMeshes.end(),
              [](const TransparentMesh& lhs, const TransparentMesh& rhs)
              { return lhs.distanceSquared > rhs.distanceSquared; });

    if (transparentMeshes.empty())
    {
      return;
    }

    const ScopedDepthMask depthMask(GL_FALSE);
    const ScopedCullFaceDisabled cullFace;
    for (const auto& transparentMesh: transparentMeshes)
    {
      m_meshDrawables[transparentMesh.index].drawable.draw(modelMatrix,
                                                           viewMatrix,
                                                           projectionMatrix,
                                                           normalMatrix,
                                                           lightPosition,
                                                           viewPos,
                                                           lightColor,
                                                           fillLightDirection,
                                                           fillLightColor,
                                                           ambientColor,
                                                           shininess);
    }
  }

private:
  DrawablesManager(opengl::ProgramManager manager)
      : programManager(std::move(manager))
  {
  }

  LineProgram& get_line_program() { return programManager.get_line_program(); }
  PointProgram& get_point_program() { return programManager.get_point_program(); }
  MeshProgram& get_mesh_program() { return programManager.get_mesh_program(); }

  DrawableId next_drawable_id() { return m_nextDrawableId++; }

  template <typename Drawable>
  static bool remove_drawable_by_id(std::vector<DrawableEntry<Drawable>>& drawables, DrawableId id)
  {
    if (id == 0U)
    {
      return false;
    }

    const auto it = std::find_if(drawables.begin(),
                                 drawables.end(),
                                 [id](const DrawableEntry<Drawable>& entry) { return entry.id == id; });
    if (it == drawables.end())
    {
      return false;
    }

    drawables.erase(it);
    return true;
  }
};

} // namespace opengl

#endif // OPENGL_DRAWABLE_DRAWABLESMANAGER_HPP

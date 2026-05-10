#ifndef OPENGL_MESHSOUP_HPP
#define OPENGL_MESHSOUP_HPP

#include "OpenGL/Drawable/DrawableTransparencyInfo.hpp"
#include "OpenGL/IndexBuffer.hpp"
#include "OpenGL/Programs/MeshProgram.hpp"
#include "OpenGL/VertexArray.hpp"
#include "OpenGL/VertexBuffer.hpp"
#include "OpenGL/opengl_export.h"
#include <Core/Warnings.hpp>
#include <linal/hmat.hpp>
#include <span>
DISABLE_ALL_WARNINGS

namespace opengl
{

class OPENGL_EXPORT MeshDrawable
{
  MeshProgram* m_program{nullptr};
  VertexArray m_vertexArray;
  VertexBuffer m_vertexBuffer;
  VertexBuffer m_vertexNormalsBuffer;
  VertexBuffer m_colorBuffer;
  IndexBuffer m_triangleIndicesBuffer;
  DrawableTransparencyInfo m_transparencyInfo;

public:
  MeshDrawable(MeshProgram& program,
               VertexArray vertexArray,
               VertexBuffer vertexBuffer,
               VertexBuffer vertexNormalsBuffer,
               VertexBuffer colorBuffer,
               IndexBuffer triangleIndicesBuffer,
               DrawableTransparencyInfo transparencyInfo = {})
      : m_program(&program)
      , m_vertexArray(std::move(vertexArray))
      , m_vertexBuffer(std::move(vertexBuffer))
      , m_vertexNormalsBuffer(std::move(vertexNormalsBuffer))
      , m_colorBuffer(std::move(colorBuffer))
      , m_triangleIndicesBuffer(std::move(triangleIndicesBuffer))
      , m_transparencyInfo(transparencyInfo)
  {
  }

  MeshDrawable(const MeshDrawable&) = delete;
  MeshDrawable& operator=(const MeshDrawable&) = delete;
  MeshDrawable(MeshDrawable&& other) noexcept
      : m_program(std::exchange(other.m_program, nullptr))
      , m_vertexArray(std::move(other.m_vertexArray))
      , m_vertexBuffer(std::move(other.m_vertexBuffer))
      , m_vertexNormalsBuffer(std::move(other.m_vertexNormalsBuffer))
      , m_colorBuffer(std::move(other.m_colorBuffer))
      , m_triangleIndicesBuffer(std::move(other.m_triangleIndicesBuffer))
      , m_transparencyInfo(other.m_transparencyInfo)
  {
  }
  MeshDrawable& operator=(MeshDrawable&& other) noexcept
  {
    if (this != &other)
    {
      m_program = std::exchange(other.m_program, nullptr);
      m_vertexArray = std::move(other.m_vertexArray);
      m_vertexBuffer = std::move(other.m_vertexBuffer);
      m_vertexNormalsBuffer = std::move(other.m_vertexNormalsBuffer);
      m_colorBuffer = std::move(other.m_colorBuffer);
      m_triangleIndicesBuffer = std::move(other.m_triangleIndicesBuffer);
      m_transparencyInfo = other.m_transparencyInfo;
    }
    return *this;
  }
  ~MeshDrawable() = default;

    void update_color_buffer(std::span<const float> colors, BufferAccessPattern accessPattern)
    {
      glBindBuffer(GL_ARRAY_BUFFER, m_colorBuffer.get_buffer_id().get_value());
      const auto size = static_cast<std::uint64_t>(colors.size()) * sizeof(float);
      glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizei>(size), colors.data(), get_enum_value(accessPattern));
      m_transparencyInfo.isTranslucent = contains_translucent_alpha(colors, 4);
    }

    /** Draw the mesh Drawable.
     * @param mvp Model-View-Projection matrix.
     * @param viewPos View position (camera position) in world space.
     */
    void draw(const linal::hmatf& modelMatrix,
              const linal::hmatf& viewMatrix,
              const linal::hmatf& projectionMatrix,
              const linal::hmatf& normalMatrix,
              const linal::float3& lightPosition,
              const linal::float3& viewPos,
              const linal::float3& lightColor,
              const linal::float3& ambientColor,
              float shininess) const
    {
      const auto& prog = *m_program;
      prog.use();

      // Vertex shader uniforms
      glUniformMatrix4fv(prog.get_model_matrix_location().get_value(), 1, GL_FALSE, (const GLfloat*)modelMatrix.data());
      glUniformMatrix4fv(prog.get_view_matrix_location().get_value(), 1, GL_FALSE, (const GLfloat*)viewMatrix.data());
      glUniformMatrix4fv(prog.get_projection_matrix_location().get_value(), 1, GL_FALSE, (const GLfloat*)projectionMatrix.data());
      glUniformMatrix4fv(prog.get_normal_matrix_location().get_value(), 1, GL_FALSE, (const GLfloat*)normalMatrix.data());

      // Fragment shader uniforms
      glUniform3fv(prog.get_light_pos_location().get_value(), 1, lightPosition.data());
      glUniform3fv(prog.get_view_pos_location().get_value(), 1, viewPos.data());
      glUniform3fv(prog.get_light_color_location().get_value(), 1, lightColor.data());
      glUniform3fv(prog.get_ambient_color_location().get_value(), 1, ambientColor.data());
      glUniform1f(prog.get_shininess_location().get_value(), shininess);

      m_vertexArray.bind();
      m_triangleIndicesBuffer.bind();

      glDrawElements(GL_TRIANGLES, m_triangleIndicesBuffer.get_index_count(), GL_UNSIGNED_INT, nullptr);
    }

    [[nodiscard]] bool is_translucent() const noexcept { return m_transparencyInfo.isTranslucent; }

    [[nodiscard]] double distance_squared_to(const linal::double3& viewPosition) const noexcept
    {
      return m_transparencyInfo.distance_squared_to(viewPosition);
    }
  };

  OPENGL_EXPORT std::optional<MeshDrawable> make_mesh_soup(MeshProgram& program,
                                                           std::span<const float> vertices,
                                                           std::int32_t vertexDimension,
                                                           std::span<const float> normals,
                                                           std::span<const float> colors,
                                                           std::int32_t colorDimension,
                                                           std::span<const std::uint32_t> triangleIndices,
                                                           BufferAccessPattern accessPattern);

  ENABLE_ALL_WARNINGS

} // namespace opengl

#endif // OPENGL_MESHSOUP_HPP

#ifndef OPENGL_MESHSOUP_HPP
#define OPENGL_MESHSOUP_HPP

#include "OpenGL/IndexBuffer.hpp"
#include "OpenGL/Programs/MeshProgram.hpp"
#include "OpenGL/VertexArray.hpp"
#include "OpenGL/VertexBuffer.hpp"
#include "OpenGL/opengl_export.h"
#include <Core/Warnings.hpp>
#include <linal/hmat.hpp>
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

public:
  MeshDrawable() = default;
  MeshDrawable(MeshProgram& program,
           VertexArray vertexArray,
           VertexBuffer vertexBuffer,
           VertexBuffer vertexNormalsBuffer,
           VertexBuffer colorBuffer,
           IndexBuffer triangleIndicesBuffer)
      : m_program(&program)
      , m_vertexArray(vertexArray)
      , m_vertexBuffer(vertexBuffer)
      , m_vertexNormalsBuffer(vertexNormalsBuffer)
      , m_colorBuffer(colorBuffer)
      , m_triangleIndicesBuffer(triangleIndicesBuffer)
  {
  }

  void update_color_buffer(std::span<const float> colors, BufferAccessPattern accessPattern)
  {
    glBindBuffer(GL_ARRAY_BUFFER, m_colorBuffer.get_buffer_id().get_value());
    const auto size = static_cast<std::uint64_t>(colors.size()) * sizeof(float);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizei>(size), colors.data(), get_enum_value(accessPattern));
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

  void destroy() const
  {
    m_vertexArray.destroy();
    m_vertexBuffer.destroy();
    m_vertexNormalsBuffer.destroy();
    m_colorBuffer.destroy();
    m_triangleIndicesBuffer.destroy();
  }
};

OPENGL_EXPORT MeshDrawable make_mesh_soup(MeshProgram& program,
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

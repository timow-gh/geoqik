#ifndef OPENGL_POINTSOUP_HPP
#define OPENGL_POINTSOUP_HPP

#include "OpenGL/BufferAccessPattern.hpp"
#include "OpenGL/IndexBuffer.hpp"
#include "OpenGL/Programs/PointProgram.hpp"
#include "OpenGL/UpdateBuffer.hpp"
#include "OpenGL/VertexArray.hpp"
#include "OpenGL/VertexBuffer.hpp"
#include "OpenGL/opengl_export.h"
#include <Core/Warnings.hpp>
#include <cstdint>
#include <linal/hmat.hpp>
#include <span>
#include <utility>

DISABLE_ALL_WARNINGS

namespace opengl
{

class OPENGL_EXPORT PointDrawable
{
  PointProgram* m_program{nullptr};
  opengl::VertexArray m_vertexArray;
  opengl::VertexBuffer m_vertexBuffer;
  opengl::VertexBuffer m_colorBuffer;
  opengl::IndexBuffer m_pointIndicesBuffer;
  float m_pointSize{1.0F};

public:
  PointDrawable(PointProgram& program,
                VertexArray vertexArray,
                VertexBuffer vertexBuffer,
                VertexBuffer colorBuffer,
                IndexBuffer pointIndicesBuffer,
                float pointSize = 1.0F) noexcept
      : m_program(&program)
      , m_vertexArray(std::move(vertexArray))
      , m_vertexBuffer(std::move(vertexBuffer))
      , m_colorBuffer(std::move(colorBuffer))
      , m_pointIndicesBuffer(std::move(pointIndicesBuffer))
      , m_pointSize(pointSize)
  {
  }

  constexpr PointDrawable(const PointDrawable& other) = delete;
  constexpr PointDrawable& operator=(const PointDrawable& other) = delete;
  PointDrawable(PointDrawable&& other) noexcept
      : m_program(std::exchange(other.m_program, nullptr))
      , m_vertexArray(std::move(other.m_vertexArray))
      , m_vertexBuffer(std::move(other.m_vertexBuffer))
      , m_colorBuffer(std::move(other.m_colorBuffer))
      , m_pointIndicesBuffer(std::move(other.m_pointIndicesBuffer))
      , m_pointSize(std::move(other.m_pointSize))
  {
  }
  PointDrawable& operator=(PointDrawable&& other) noexcept
  {
    if (this != &other)
    {
      m_program = std::exchange(other.m_program, nullptr);
      m_vertexArray = std::move(other.m_vertexArray);
      m_vertexBuffer = std::move(other.m_vertexBuffer);
      m_colorBuffer = std::move(other.m_colorBuffer);
      m_pointIndicesBuffer = std::move(other.m_pointIndicesBuffer);
      m_pointSize = std::move(other.m_pointSize);
    }
    return *this;
  }
  ~PointDrawable() = default;

  constexpr void set_point_size(float pointSize) noexcept { m_pointSize = pointSize; }

  void update_vertex_buffer(std::span<const float> vertices, BufferAccessPattern accessPattern)
  {
    m_vertexBuffer.update_buffer(vertices, accessPattern);
  }

  void update_color_buffer(std::span<const float> colors, BufferAccessPattern accessPattern)
  {
    m_colorBuffer.update_buffer(colors, accessPattern);
  }

  void update_indices_buffer(std::span<const std::uint32_t> indices, BufferAccessPattern accessPattern)
  {
    m_pointIndicesBuffer.update_indices_buffer(indices, accessPattern);
  }

  void update_point_drawable(std::span<const float> vertices,
                             std::span<const float> colors,
                             std::span<const std::uint32_t> indices,
                             BufferAccessPattern accessPattern)
  {
    update_vertex_buffer(vertices, accessPattern);
    update_color_buffer(colors, accessPattern);
    update_indices_buffer(indices, accessPattern);
  }

  void draw(const linal::hmatf& mvp) const
  {
    CORE_ASSERT(m_program != nullptr);
    auto& prog = *m_program;
    prog.use();
    glUniformMatrix4fv(prog.get_mvp_location().get_value(), 1, GL_FALSE, (const GLfloat*)mvp.data());
    glPointSize(m_pointSize);
    m_vertexArray.bind();

    glDrawElements(GL_POINTS, m_pointIndicesBuffer.get_index_count(), GL_UNSIGNED_INT, nullptr);
  }
};

OPENGL_EXPORT std::optional<PointDrawable> make_point_drawable(PointProgram& program,
                                                               std::span<const float> vertices,
                                                               std::int32_t vertexDimension,
                                                               std::span<const float> colors,
                                                               std::int32_t colorDimension,
                                                               std::span<const std::uint32_t> indices,
                                                               float pointSize,
                                                               BufferAccessPattern accessPattern);

} // namespace opengl

ENABLE_ALL_WARNINGS

#endif // OPENGL_POINTSOUP_HPP

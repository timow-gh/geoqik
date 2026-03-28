#ifndef OPENGL_LINESOUP_HPP
#define OPENGL_LINESOUP_HPP

#include "OpenGL/BufferAccessPattern.hpp"
#include "OpenGL/IndexBuffer.hpp"
#include "OpenGL/LineType.hpp"
#include "OpenGL/Programs/LineProgram.hpp"
#include "OpenGL/UpdateBuffer.hpp"
#include "OpenGL/VertexArray.hpp"
#include "OpenGL/VertexBuffer.hpp"
#include "OpenGL/opengl_export.h"
#include <Core/Warnings.hpp>
#include <cstdint>
#include <linal/hmat.hpp>
#include <span>
#include <vector>
DISABLE_ALL_WARNINGS
namespace opengl
{

class OPENGL_EXPORT LineDrawable
{
  LineProgram* m_program{nullptr};
  VertexArray m_vertexArray;
  VertexBuffer m_vertexBuffer;
  VertexBuffer m_colorBuffer;
  IndexBuffer m_lineIndicesBuffer;
  float m_lineThickness{1.0F};
  LineType m_lineType{};
  float m_pointSize{1.0F};

public:
  LineDrawable(LineProgram& program,
               VertexArray vertexArray,
               VertexBuffer vertexBuffer,
               VertexBuffer colorBuffer,
               IndexBuffer lineIndicesBuffer,
               float lineThickness,
               float pointSize = 1.0F,
               LineType lineType = LineType::lines())
      : m_program(&program)
      , m_vertexArray(std::move(vertexArray))
      , m_vertexBuffer(std::move(vertexBuffer))
      , m_colorBuffer(std::move(colorBuffer))
      , m_lineIndicesBuffer(std::move(lineIndicesBuffer))
      , m_lineThickness(lineThickness)
      , m_lineType(lineType)
      , m_pointSize(pointSize)
  {
  }

  LineDrawable(const LineDrawable&) = delete;
  LineDrawable& operator=(const LineDrawable&) = delete;
  LineDrawable(LineDrawable&& other) noexcept
  : m_program(std::exchange(other.m_program, nullptr))
      , m_vertexArray(std::move(other.m_vertexArray))
      , m_vertexBuffer(std::move(other.m_vertexBuffer))
      , m_colorBuffer(std::move(other.m_colorBuffer))
      , m_lineIndicesBuffer(std::move(other.m_lineIndicesBuffer))
      , m_lineThickness(other.m_lineThickness)
      , m_lineType(other.m_lineType)
      , m_pointSize(other.m_pointSize)
  {
  }
  LineDrawable& operator=(LineDrawable&& other) noexcept 
  {
    if (this != &other)
    {
      m_program = std::exchange(other.m_program, nullptr);
      m_vertexArray = std::move(other.m_vertexArray);
      m_vertexBuffer = std::move(other.m_vertexBuffer);
      m_colorBuffer = std::move(other.m_colorBuffer);
      m_lineIndicesBuffer = std::move(other.m_lineIndicesBuffer);
      m_lineThickness = other.m_lineThickness;
      m_lineType = other.m_lineType;
      m_pointSize = other.m_pointSize;
    }
    return *this;
  }
  ~LineDrawable() = default;

  [[nodiscard]] float get_line_thickness() const { return m_lineThickness; }
  void set_line_thickness(float lineThickness) { m_lineThickness = lineThickness; }

  [[nodiscard]] float get_point_size() const { return m_pointSize; }
  void set_point_size(float pointSize) { m_pointSize = pointSize; }

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
    m_lineIndicesBuffer.update_indices_buffer(indices, accessPattern);
  }

  void update_line_drawable(std::span<const float> vertices,
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
    glLineWidth(m_lineThickness);
    m_vertexArray.bind();
    glDrawElements(m_lineType.get_gl_type(), m_lineIndicesBuffer.get_index_count(), GL_UNSIGNED_INT, nullptr);

    if (m_pointSize != 0.0F)
    {
      glPointSize(m_pointSize);
      glDrawElements(GL_POINTS, m_lineIndicesBuffer.get_index_count(), GL_UNSIGNED_INT, nullptr);
    }
  }
};

OPENGL_EXPORT std::optional<LineDrawable> make_line_drawable(LineProgram& program,
                                                             std::span<const float> lineVertices,
                                                             std::int32_t lineVertexDimension,
                                                             std::span<const std::uint32_t> lineIndices,
                                                             std::span<const float> lineColors,
                                                             std::int32_t lineColorDimension,
                                                             LineType lineType,
                                                             float lineThickness,
                                                             float pointThickness,
                                                             opengl::BufferAccessPattern accessPattern);

} // namespace opengl

ENABLE_ALL_WARNINGS

#endif // OPENGL_LINESOUP_HPP

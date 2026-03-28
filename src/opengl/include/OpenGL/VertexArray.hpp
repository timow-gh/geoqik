#ifndef OPENGL_VERTEXARRAY_HPP
#define OPENGL_VERTEXARRAY_HPP

#include "OpenGL/OpenGL.hpp"
#include "OpenGL/opengl_export.h"
#include <optional>

namespace opengl
{

class OPENGL_EXPORT VertexArray
{
  std::optional<GLuint> m_id;

public:
  VertexArray(GLuint id)
      : m_id(id)
  {
  }
  VertexArray(const VertexArray&) = delete;
  VertexArray& operator=(const VertexArray&) = delete;
  VertexArray(VertexArray&& other) noexcept
      : m_id(std::exchange(other.m_id, std::nullopt))
  {
  }
  VertexArray& operator=(VertexArray&& other) noexcept
  {
    if (this == &other)
    {
      return *this;
    }

    m_id = std::exchange(other.m_id, std::nullopt);
    return *this;
  }

  ~VertexArray()
  {
    if (m_id.has_value())
    {
      glDeleteVertexArrays(1, &m_id.value());
    }
  }

  static VertexArray create()
  {
    GLuint id;
    glGenVertexArrays(1, &id);
    VertexArray array{id};
    array.bind();
    return array;
  }

  void bind() const
  {
    CORE_ASSERT(m_id.has_value());
    glBindVertexArray(m_id.value());
  }
  void unbind() const
  {
    CORE_ASSERT(m_id.has_value());
    glBindVertexArray(0);
  }
};

} // namespace opengl

#endif // OPENGL_VERTEXARRAY_HPP

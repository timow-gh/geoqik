#ifndef OPENGL_VERTEXARRAY_HPP
#define OPENGL_VERTEXARRAY_HPP

#include "OpenGL/OpenGL.hpp"
#include "OpenGL/opengl_export.h"

namespace opengl
{

class OPENGL_EXPORT VertexArray {
  GLuint m_id{0};

public:
  constexpr VertexArray() noexcept = default;

  static VertexArray create() {
    VertexArray array;
    glGenVertexArrays(1, &array.m_id);
    array.bind();
    return array;
  }

  void bind() const { glBindVertexArray(m_id); }
  void unbind() const { glBindVertexArray(0); }
  void destroy() const { glDeleteVertexArrays(1, &m_id); }
};

} // namespace opengl

#endif // OPENGL_VERTEXARRAY_HPP

#ifndef OPENGL_VERTEXBUFFER_HPP
#define OPENGL_VERTEXBUFFER_HPP

#include "OpenGL/BufferAccessPattern.hpp"
#include "OpenGL/BufferId.hpp"
#include "OpenGL/Location.hpp"
#include "OpenGL/OpenGL.hpp"
#include "OpenGL/UnsignedIntId.hpp"
#include "OpenGL/UpdateBuffer.hpp"
#include "OpenGL/opengl_export.h"
#include <Core/Assert.hpp>
#include <cstddef>
#include <optional>
#include <span>

namespace opengl
{

/** @brief An opengl buffer object. */
class OPENGL_EXPORT VertexBuffer
{
  std::optional<BufferId> m_bufferId;
  GLsizei m_stride;
  Location m_bufferLocation;

public:
  VertexBuffer(const VertexBuffer&) = delete;
  VertexBuffer& operator=(const VertexBuffer&) = delete;
  VertexBuffer(VertexBuffer&& other) noexcept
      : m_bufferId{std::exchange(other.m_bufferId, std::nullopt)}
      , m_stride{other.m_stride}
      , m_bufferLocation{other.m_bufferLocation}
  {
  }
  VertexBuffer& operator=(VertexBuffer&& other) noexcept
  {
    if (this != &other)
    {
      m_bufferId = std::exchange(other.m_bufferId, std::nullopt);
      m_stride = other.m_stride;
      m_bufferLocation = other.m_bufferLocation;
    }
    return *this;
  }
  ~VertexBuffer()
  {
    if (m_bufferId.has_value())
    {
      const auto id = m_bufferId->get_value();
      glDeleteBuffers(1, &id);
    }
  }

  static std::optional<VertexBuffer>
  create(std::span<const float> vectors, GLsizei vectorDimension, Location bufferLocation, BufferAccessPattern accessPattern)
  {
    GLuint bufferId;
    glGenBuffers(1, &bufferId);
    if (bufferId == 0)
    {
      CORE_ASSERT(false);
      glDeleteBuffers(1, &bufferId);
      return std::nullopt;
    }
    glBindBuffer(GL_ARRAY_BUFFER, bufferId);
    const auto size = vectors.size() * sizeof(float);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizei>(size), vectors.data(), get_enum_value(accessPattern));
    glEnableVertexAttribArray(bufferLocation.get_as_unsigned());
    const GLsizei stride = vectorDimension * static_cast<GLsizei>(sizeof(float));
    glVertexAttribPointer(bufferLocation.get_as_unsigned(),
                          vectorDimension,
                          GL_FLOAT,
                          GL_FALSE,
                          static_cast<GLsizei>(stride),
                          static_cast<void*>(0));
    /*
     * 0 is the location of the vertex attribute in the vertex shader
     * 3 is the number of components per vertex attribute
     * GL_FLOAT is the type of the components
     * GL_FALSE is whether the data should be normalized
     * 3 * sizeof(float) is the stride (byte offset between consecutive vertex attributes)
     * (void*)0 is the offset of the first component
     */
    return VertexBuffer{BufferId{bufferId}, stride, bufferLocation};
  }

  [[nodiscard]] const BufferId& get_buffer_id() const
  {
    CORE_ASSERT(m_bufferId.has_value());
    return m_bufferId.value();
  }

  void bind() const
  {
    CORE_ASSERT(m_bufferId.has_value());
    glBindBuffer(GL_ARRAY_BUFFER, m_bufferId.value().get_value());
    glEnableVertexAttribArray(m_bufferLocation.get_as_unsigned());
    glVertexAttribPointer(m_bufferLocation.get_as_unsigned(),
                          m_stride,
                          GL_FLOAT,
                          GL_FALSE,
                          static_cast<GLsizei>(m_stride),
                          static_cast<void*>(0));
  }

  void unbind() const { glBindBuffer(GL_ARRAY_BUFFER, 0); }

  void update_buffer(std::span<const float> vertices, BufferAccessPattern accessPattern)
  {
    ::opengl::update_buffer(*this, vertices, accessPattern);
  }

private:
  constexpr VertexBuffer(BufferId bufferId, GLsizei stride, Location bufferLocation)
      : m_bufferId{bufferId}
      , m_stride{stride}
      , m_bufferLocation{bufferLocation}
  {
  }
};

} // namespace opengl

#endif // OPENGL_VERTEXBUFFER_HPP

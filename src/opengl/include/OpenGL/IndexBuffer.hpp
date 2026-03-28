#ifndef OPENGL_INDEXBUFFER_HPP
#define OPENGL_INDEXBUFFER_HPP

#include "OpenGL/BufferAccessPattern.hpp"
#include "OpenGL/BufferId.hpp"
#include "OpenGL/OpenGL.hpp"
#include "OpenGL/opengl_export.h"
#include <Core/Warnings.hpp>
#include <cstdint>
#include <optional>
#include <span>

DISABLE_ALL_WARNINGS

namespace opengl
{

/** @brief An opengl buffer object for indices.
 *
 * @details Represents the data on the GPU.
 */
class OPENGL_EXPORT IndexBuffer
{
  std::optional<BufferId> m_id;
  GLsizei m_indexCount{0};

public:
  IndexBuffer(const IndexBuffer&) = delete;
  IndexBuffer& operator=(const IndexBuffer&) = delete;
  IndexBuffer(IndexBuffer&& other) noexcept 
      : m_id(std::exchange(other.m_id, std::nullopt))
      , m_indexCount(std::exchange(other.m_indexCount, 0))
  {
  }
  IndexBuffer& operator=(IndexBuffer&& other) noexcept
  {
    if (this != &other)
    {
      m_id = std::exchange(other.m_id, std::nullopt);
      m_indexCount = std::exchange(other.m_indexCount, 0);
    }
    return *this;
  }
  ~IndexBuffer() 
  {
    if (m_id.has_value())
    {
      glDeleteBuffers(1, &m_id.value().get_value());
    }
  }

  static std::optional<IndexBuffer> create(const unsigned int indices[], GLsizei indexCount, BufferAccessPattern accessPattern)
  {
    BufferId id;
    glGenBuffers(1, &id.get_value());
    if (id.get_value() == 0)
    {
      glDeleteBuffers(1, &id.get_value());
      return std::nullopt;
    }

    IndexBuffer buffer{std::move(id), indexCount};
    buffer.bind();
    const auto size = static_cast<std::uint64_t>(indexCount) * sizeof(unsigned int);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizei>(size), indices, get_enum_value(accessPattern));

    return buffer;
  }

  [[nodiscard]] const BufferId& get_buffer_id() const
  {
    CORE_ASSERT(m_id.has_value());
    return m_id.value();
  }

  [[nodiscard]] GLsizei get_index_count() const { return m_indexCount; }
  void set_index_count(GLsizei indexCount) { m_indexCount = indexCount; }

  void bind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id.value().get_value()); }
  void unbind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }

  void update_indices_buffer(std::span<const std::uint32_t> indices, BufferAccessPattern accessPattern)
  {
    bind();
    const GLsizeiptr size = static_cast<GLsizeiptr>(indices.size() * sizeof(std::uint32_t));
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, NULL, get_enum_value(accessPattern));
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices.data(), get_enum_value(accessPattern));
    set_index_count(static_cast<GLsizei>(indices.size()));
  }

  private:
    IndexBuffer(BufferId id, GLsizei indexCount)
        : m_id(std::move(id))
        , m_indexCount(indexCount)
    {
    }
};

} // namespace opengl

ENABLE_ALL_WARNINGS

#endif // OPENGL_INDEXBUFFER_HPP

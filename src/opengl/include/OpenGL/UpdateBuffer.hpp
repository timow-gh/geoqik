#ifndef OPENGL_UPDATEBUFFER_HPP
#define OPENGL_UPDATEBUFFER_HPP

#include "OpenGL/BufferAccessPattern.hpp"
#include <Core/Assert.hpp>
#include <limits>
#include <span>

namespace opengl
{

/** @brief Updates a buffer using buffer orphaning.
 *
 * This function binds the buffer, allocates new memory for it, and fills it with the new data.
 */
template<typename TBuffer, typename T>
void update_buffer(TBuffer& buffer, std::span<const T> newBufferData, opengl::BufferAccessPattern accessPattern)
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer.get_buffer_id().get_value());
    const GLsizeiptr size = static_cast<GLsizeiptr>(newBufferData.size() * sizeof(T));
    CORE_ASSERT(size <= std::numeric_limits<GLsizeiptr>::max());
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(size), NULL, get_enum_value(accessPattern));
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(size), newBufferData.data(), get_enum_value(accessPattern));
}

} // namespace opengl

#endif // OPENGL_UPDATEBUFFER_HPP

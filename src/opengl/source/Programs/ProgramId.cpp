#include "OpenGL/Programs/ProgramId.hpp"

#include <Core/Assert.hpp>
#include <utility>

namespace opengl
{

ProgramHandle::ProgramHandle(ProgramId id) noexcept
    : m_id{id}
{
}

ProgramHandle::ProgramHandle(GLuint id) noexcept
    : ProgramHandle(ProgramId{id})
{
}

ProgramHandle::ProgramHandle(ProgramHandle&& other) noexcept
    : m_id(std::exchange(other.m_id, std::nullopt))
{
}

ProgramHandle& ProgramHandle::operator=(ProgramHandle&& other) noexcept
{
  if (this != &other)
  {
    reset();
    m_id = std::exchange(other.m_id, std::nullopt);
  }
  return *this;
}

ProgramHandle::~ProgramHandle()
{
  reset();
}

void ProgramHandle::reset() noexcept
{
  if (m_id.has_value())
  {
    glDeleteProgram(m_id->get_value());
    m_id = std::nullopt;
  }
}

ProgramId ProgramHandle::get_id() const
{
  CORE_ASSERT(m_id.has_value());
  return m_id.value();
}

GLuint ProgramHandle::get_value() const
{
  CORE_ASSERT(m_id.has_value());
  return m_id->get_value();
}

} // namespace opengl

#include "OpenGL/Uniform.hpp"

#include <Core/Assert.hpp>
#include <cstdlib>
#include <print>

namespace opengl
{

Uniform::Uniform(std::string_view name, Location location) noexcept
    : m_name{name}
    , m_location{location} {
  CORE_ASSERT(!name.empty());
  CORE_ASSERT(location.get_value() != -1);
}

Uniform make_uniform(std::string_view name, ProgramId program) {
  CORE_ASSERT(!name.empty());
  CORE_ASSERT(program.get_value() != 0);

  Location location{glGetUniformLocation(program.get_value(), name.data())};
  if (location.get_value() == -1)
  {
    CORE_ASSERT(false);
    std::print("Uniform {} location not found\n", name);
    std::abort();
  }

  return Uniform{name, location};
}

} // namespace opengl

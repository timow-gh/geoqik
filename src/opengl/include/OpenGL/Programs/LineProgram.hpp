#ifndef OPENGL_LINEPROGRAM_HPP
#define OPENGL_LINEPROGRAM_HPP

#include "OpenGL/Attribute.hpp"
#include "OpenGL/OpenGL.hpp"
#include "OpenGL/Programs/ProgramId.hpp"
#include "OpenGL/Uniform.hpp"
#include "OpenGL/opengl_export.h"
#include <Core/Assert.hpp>

namespace opengl
{

class OPENGL_EXPORT LineProgram
{
  ProgramId m_id;
  Uniform m_mvpLocation;
  Attribute m_vertexLocation;
  Attribute m_colorLocation;

public:
  constexpr LineProgram() noexcept = default;
  constexpr LineProgram(ProgramId programId, Uniform mvpLocation, Attribute vertexLocation, Attribute colorLocation) noexcept
      : m_id{programId}
      , m_mvpLocation{mvpLocation}
      , m_vertexLocation{vertexLocation}
      , m_colorLocation{colorLocation}
  {
    CORE_ASSERT(programId.get_value() != 0);
    CORE_ASSERT(mvpLocation.get_location().get_value() != -1);
    CORE_ASSERT(vertexLocation.get_location().get_value() != -1);
    CORE_ASSERT(colorLocation.get_location().get_value() != -1);
  }

  [[nodiscard]] constexpr Location get_mvp_location() const { return m_mvpLocation.get_location(); }
  [[nodiscard]] constexpr Location get_vertex_location() const { return m_vertexLocation.get_location(); }
  [[nodiscard]] constexpr Location get_color_location() const { return m_colorLocation.get_location(); }

  void use() const { glUseProgram(m_id.get_value()); }
};

OPENGL_EXPORT LineProgram make_line_program();

} // namespace opengl

#endif // OPENGL_LINEPROGRAM_HPP

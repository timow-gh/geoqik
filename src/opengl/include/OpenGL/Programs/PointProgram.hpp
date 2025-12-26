#ifndef OPENGL_POINTPROGRAM_HPP
#define OPENGL_POINTPROGRAM_HPP

#include "OpenGL/Attribute.hpp"
#include "OpenGL/OpenGL.hpp"
#include "OpenGL/Programs/ProgramId.hpp"
#include "OpenGL/Uniform.hpp"
#include "OpenGL/opengl_export.h"

namespace opengl
{

class OPENGL_EXPORT PointProgram
{
  ProgramId m_id;

  Uniform m_mvpLocation;
  Attribute m_vertexLocation;
  Attribute m_colorLocation;

public:
  constexpr PointProgram() noexcept = default;
  constexpr PointProgram(ProgramId programId, Uniform mvpLocation, Attribute vertexLocation, Attribute colorLocation) noexcept
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

  [[nodiscard]] constexpr Location get_mvp_location() const noexcept { return m_mvpLocation.get_location(); }
  [[nodiscard]] constexpr Location get_pos_location() const noexcept { return m_vertexLocation.get_location(); }
  [[nodiscard]] constexpr Location get_color_location() const noexcept { return m_colorLocation.get_location(); }

  void use() const noexcept { glUseProgram(m_id.get_value()); }
};

OPENGL_EXPORT PointProgram make_point_program();

} // namespace opengl

#endif // OPENGL_POINTPROGRAM_HPP

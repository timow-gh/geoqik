#include "OpenGL/Programs/PointProgram.hpp"

#include "OpenGL/FmtIncludeHelper.hpp"
#include "OpenGL/Programs/CreateProgram.hpp"
#include "OpenGL/ShaderSources.hpp"
#include <Core/Assert.hpp>
#include <string>
#include <utility>

namespace opengl
{

PointProgram::PointProgram(ProgramHandle program, Uniform mvpLocation, Attribute vertexLocation, Attribute colorLocation) noexcept
    : m_program{std::move(program)}
    , m_mvpLocation{mvpLocation}
    , m_vertexLocation{vertexLocation}
    , m_colorLocation{colorLocation}
{
  CORE_ASSERT(m_program.is_valid());
  CORE_ASSERT(mvpLocation.get_location().get_value() != -1);
  CORE_ASSERT(vertexLocation.get_location().get_value() != -1);
  CORE_ASSERT(colorLocation.get_location().get_value() != -1);
}

void PointProgram::use() const
{
  glUseProgram(m_program.get_value());
}

PointProgram make_point_program() {
  const std::string vertexShaderSource = point_color_vertex_shader_source();
  const std::string fragmentShaderSource = point_color_fragment_shader_source();

  ProgramHandle program = create_program(vertexShaderSource.c_str(), fragmentShaderSource.c_str());
  if (!program.is_valid())
  {
    CORE_ASSERT(false);
    return {};
  }
  ProgramId programId = program.get_id();
  Uniform mvpLocation = make_uniform("u_MVP", programId);
  Attribute vertexLocation = make_attribute("a_vertex", programId);
  Attribute colorLocation = make_attribute("a_color", programId);

  return PointProgram{std::move(program), mvpLocation, vertexLocation, colorLocation};
}

} // namespace opengl

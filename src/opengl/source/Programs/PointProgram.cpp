#include "OpenGL/Programs/PointProgram.hpp"
#include "OpenGL/Programs/CreateProgram.hpp"
#include "OpenGL/ShaderSources.hpp"
#include "OpenGL/FmtIncludeHelper.hpp"
#include <string>

namespace opengl
{

PointProgram make_point_program() {
  const std::string vertexShaderSource = point_color_vertex_shader_source();
  const std::string fragmentShaderSource = point_color_fragment_shader_source();

  ProgramId programId = create_program(vertexShaderSource.c_str(), fragmentShaderSource.c_str());
  Uniform mvpLocation = make_uniform("u_MVP", programId);
  Attribute vertexLocation = make_attribute("a_vertex", programId);
  Attribute colorLocation = make_attribute("a_color", programId);

  return PointProgram{programId, mvpLocation, vertexLocation, colorLocation};
}

} // namespace opengl

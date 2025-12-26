#include "OpenGL/Programs/LineProgram.hpp"
#include "OpenGL/Programs/CreateProgram.hpp"
#include "OpenGL/ShaderSources.hpp"
#include <string>

namespace opengl
{

opengl::LineProgram make_line_program() {
  const std::string vertexShaderSource = line_vertex_shader_source();
  const std::string fragmentShaderSource = line_fragment_shader_source();
  ProgramId id{create_program(vertexShaderSource.c_str(), fragmentShaderSource.c_str())};
  CORE_ASSERT(id.get_value() != 0);
  Uniform mvpLocation = make_uniform("u_MVP", id);
  Attribute vertexLocation = make_attribute("a_vertex", id);
  Attribute colorLocation = make_attribute("a_color", id);
  return LineProgram{id, mvpLocation, vertexLocation, colorLocation};
}

} // namespace opengl

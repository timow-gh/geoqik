#include "OpenGL/Programs/MeshProgram.hpp"
#include "OpenGL/Programs/CreateProgram.hpp"
#include "OpenGL/ShaderSources.hpp"

namespace opengl
{

MeshProgram make_mesh_program() {
  ProgramId id{
      create_program(mesh_vertex_shader_source().data(), mesh_fragment_shader_source().data())};
  CORE_ASSERT(id.get_value() != 0);

  MeshProgramInput input = {.m_modelMatrix = make_uniform("u_model", id),
                            .m_viewMatrix = make_uniform("u_view", id),
                            .m_projectionMatrix = make_uniform("u_projection", id),
                            .m_normalMatrix = make_uniform("u_normalMatrix", id),
                            .m_posLocation = make_attribute("a_vertex", id),
                            .m_colorLocation = make_attribute("a_color", id),
                            .m_normalLocation = make_attribute("a_normal", id),
                            .m_lightPos = make_uniform("u_lightPos", id),
                            .viewPos = make_uniform("u_viewPos", id),
                            .lightColor = make_uniform("u_lightColor", id),
                            .ambientColor = make_uniform("u_ambientColor", id),
                            .shininess = make_uniform("u_shininess", id)};

  return MeshProgram{id, input};
}

} // namespace opengl
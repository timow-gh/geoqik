#include "OpenGL/Programs/ProgramManager.hpp"

namespace opengl
{

void ProgramManager::compile() {
  m_lineProgram = make_line_program();
  m_pointProgram = make_point_program();
  m_meshProgram = make_mesh_program();
}

void ProgramManager::reset() noexcept
{
  m_lineProgram = LineProgram{};
  m_pointProgram = PointProgram{};
  m_meshProgram = MeshProgram{};
}

} // namespace opengl

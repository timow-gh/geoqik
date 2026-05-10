#ifndef OPENGL_CREATEPROGRAM_HPP
#define OPENGL_CREATEPROGRAM_HPP

#include "OpenGL/OpenGL.hpp"
#include "OpenGL/Programs/ProgramId.hpp"

namespace opengl
{

OPENGL_EXPORT ProgramHandle create_program(const char* vertexShaderSource, const char* fragmentShaderSource);

} // namespace opengl

#endif // OPENGL_CREATEPROGRAM_HPP

#include "OpenGL/Programs/CreateProgram.hpp"
#include "Core/ScopedAction.hpp"
#include "OpenGL/FmtIncludeHelper.hpp"

namespace opengl
{

ProgramId create_program(const char* vertexShaderSource, const char* fragmentShaderSource)
{
  // compile vertex shader
  unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
  core::ScopedAction vertexSaderDeleter = core::make_scoped_action([&vertexShader]() { glDeleteShader(vertexShader); });
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);

  // check vertex shader compile errors
  int success;
  char infoLog[512];
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    fmt::print("Vertex shader compilation failed:\n{}\n", infoLog);
    CORE_ASSERT(success == GL_TRUE);
    return {};
  }

  // compile fragment shader
  unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  core::ScopedAction fragmentShaderDeleter = core::make_scoped_action([&fragmentShader]() { glDeleteShader(fragmentShader); });
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);

  // check fragment shader compile errors
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
    fmt::print("Fragment shader compilation failed:\n{}\n", infoLog);
    CORE_ASSERT(success == GL_TRUE);
    return {};
  }

  GLuint programId = glCreateProgram();

  glAttachShader(programId, vertexShader);
  glAttachShader(programId, fragmentShader);
  glLinkProgram(programId);

  // check shader program link errors
  glGetProgramiv(programId, GL_LINK_STATUS, &success);
  if (!success)
  {
    glDeleteProgram(programId);
    glGetProgramInfoLog(programId, 512, NULL, infoLog);
    fmt::print("Shader program link failed:\n{}\n", infoLog);
    CORE_ASSERT(success == GL_TRUE);
    return {};
  }

  return ProgramId{programId};
}
} // namespace opengl

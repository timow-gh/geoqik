#include "OpenGL/Programs/CreateProgram.hpp"

#include "Core/ScopedAction.hpp"
#include "OpenGL/FmtIncludeHelper.hpp"
#include <Core/Assert.hpp>
#include <array>

namespace opengl
{

namespace
{

constexpr GLsizei shaderInfoLogSize = 512;

} // namespace

ProgramHandle create_program(const char* vertexShaderSource, const char* fragmentShaderSource)
{
  // compile vertex shader
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  core::ScopedAction vertexSaderDeleter = core::make_scoped_action([&vertexShader]() { glDeleteShader(vertexShader); });
  glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
  glCompileShader(vertexShader);

  // check vertex shader compile errors
  GLint success{GL_FALSE};
  std::array<GLchar, shaderInfoLogSize> infoLog{};
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (success == GL_FALSE)
  {
    glGetShaderInfoLog(vertexShader, shaderInfoLogSize, nullptr, infoLog.data());
    fmt::print("Vertex shader compilation failed:\n{}\n", infoLog.data());
    CORE_ASSERT(success == GL_TRUE);
    return ProgramHandle{};
  }

  // compile fragment shader
  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  core::ScopedAction fragmentShaderDeleter = core::make_scoped_action([&fragmentShader]() { glDeleteShader(fragmentShader); });
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
  glCompileShader(fragmentShader);

  // check fragment shader compile errors
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if (success == GL_FALSE)
  {
    glGetShaderInfoLog(fragmentShader, shaderInfoLogSize, nullptr, infoLog.data());
    fmt::print("Fragment shader compilation failed:\n{}\n", infoLog.data());
    CORE_ASSERT(success == GL_TRUE);
    return ProgramHandle{};
  }

  GLuint programId = glCreateProgram();

  glAttachShader(programId, vertexShader);
  glAttachShader(programId, fragmentShader);
  glLinkProgram(programId);

  // check shader program link errors
  glGetProgramiv(programId, GL_LINK_STATUS, &success);
  if (success == GL_FALSE)
  {
    glGetProgramInfoLog(programId, shaderInfoLogSize, nullptr, infoLog.data());
    glDeleteProgram(programId);
    fmt::print("Shader program link failed:\n{}\n", infoLog.data());
    CORE_ASSERT(success == GL_TRUE);
    return ProgramHandle{};
  }

  return ProgramHandle{ProgramId{programId}};
}
} // namespace opengl

#ifndef OPENGL_OPENGL_HPP
#define OPENGL_OPENGL_HPP

#include "OpenGL/opengl_export.h"
#include <Core/DLLWarnings.hpp>
#include <glad/glad.h>
#include <string_view>

namespace opengl
{

SUPPRESS_STL_DLL_WARNINGS_BEGIN

struct OPENGL_EXPORT OpenGLConfig {
  int majorVersion = 4;
  int minorVersion = 3;
  std::string_view glsl_version =
      "#version 430"; // The target Ubuntu Version only supports opengl verion 4.3
  bool debug{false};
};

SUPPRESS_STL_DLL_WARNINGS_END

} // namespace opengl

#endif // OPENGL_OPENGL_HPP

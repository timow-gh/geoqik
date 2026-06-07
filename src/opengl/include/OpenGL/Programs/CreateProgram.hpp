#ifndef OPENGL_CREATEPROGRAM_HPP
#define OPENGL_CREATEPROGRAM_HPP

#include "OpenGL/OpenGL.hpp"
#include "OpenGL/Programs/ProgramId.hpp"
#include <expected>
#include <string>
#include <system_error>
#include <type_traits>

namespace opengl
{

enum class ProgramCreationFailureStage
{
  Success = 0,
  VertexShaderCreation = 1,
  VertexShaderCompilation = 2,
  FragmentShaderCreation = 3,
  FragmentShaderCompilation = 4,
  ProgramCreation = 5,
  ProgramLinking = 6
};

OPENGL_EXPORT const std::error_category& program_creation_error_category() noexcept;
OPENGL_EXPORT std::error_code make_error_code(ProgramCreationFailureStage stage);

using ProgramCreationResult = std::expected<ProgramHandle, std::error_code>;

OPENGL_EXPORT ProgramCreationResult create_program(const char* vertexShaderSource, const char* fragmentShaderSource);

} // namespace opengl

namespace std
{
template <>
struct is_error_code_enum<opengl::ProgramCreationFailureStage> : true_type {};
} // namespace std

#endif // OPENGL_CREATEPROGRAM_HPP

#ifndef OPENGL_PROGRAMID_HPP
#define OPENGL_PROGRAMID_HPP

#include "OpenGL/OpenGL.hpp"
#include "OpenGL/UnsignedIntId.hpp"
#include "OpenGL/opengl_export.h"

namespace opengl
{

class OPENGL_EXPORT ProgramId : public UnsignedIntId {
public:
  using UnsignedIntId::UnsignedIntId;
};

} // namespace opengl

#endif // OPENGL_PROGRAMID_HPP

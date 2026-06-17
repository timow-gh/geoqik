#ifndef PICKRAY_HPP
#define PICKRAY_HPP

#include <linal/linal.hpp>

namespace geoqik
{

struct PickRay {
  linal::double3 origin;
  linal::double3 direction;
};

} // namespace geoqik

#endif // PICKRAY_HPP

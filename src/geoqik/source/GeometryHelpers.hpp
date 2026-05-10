#ifndef GEOMETRYHELPERS_HPP
#define GEOMETRYHELPERS_HPP

#include <linal/vec.hpp>
#include <span>

namespace geoqik
{

  inline void calc_max_radius_squared(std::span<const float> points, const linal::float3& center, float& maxRadiusSq)
  {
    for (std::size_t i = 0; i < points.size(); i += 3)
    {
      linal::float3 point{points[i], points[i + 1], points[i + 2]};
      float distSq = linal::length_squared(center - point);
      if (distSq > maxRadiusSq)
      {
        maxRadiusSq = distSq;
      }
    }
  }

} // namespace geoqik

#endif // GEOMETRYHELPERS_HPP
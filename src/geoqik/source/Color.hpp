#ifndef GEOQIK_COLOR_HPP
#define GEOQIK_COLOR_HPP

#include <array>
#include <cstddef>

namespace geoqik
{

inline constexpr std::size_t ColorChannelCount = 4;
using Color = std::array<float, ColorChannelCount>;

} // namespace geoqik

#endif // GEOQIK_COLOR_HPP

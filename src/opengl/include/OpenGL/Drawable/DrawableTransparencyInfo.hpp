#ifndef OPENGL_DRAWABLETRANSPARENCYINFO_HPP
#define OPENGL_DRAWABLETRANSPARENCYINFO_HPP

#include <cstdint>
#include <linal/vec.hpp>
#include <span>

namespace opengl
{

struct DrawableTransparencyInfo
{
  bool isTranslucent{false};
  linal::float3 sortCenter{0.0F, 0.0F, 0.0F};

  [[nodiscard]] double distance_squared_to(const linal::double3& viewPosition) const noexcept
  {
    const double dx = static_cast<double>(sortCenter[0]) - viewPosition[0];
    const double dy = static_cast<double>(sortCenter[1]) - viewPosition[1];
    const double dz = static_cast<double>(sortCenter[2]) - viewPosition[2];
    return dx * dx + dy * dy + dz * dz;
  }
};

[[nodiscard]] inline bool contains_translucent_alpha(std::span<const float> colors, std::int32_t colorDimension) noexcept
{
  if (colorDimension < 4)
  {
    return false;
  }

  for (std::size_t alphaIndex = 3; alphaIndex < colors.size(); alphaIndex += static_cast<std::size_t>(colorDimension))
  {
    if (colors[alphaIndex] < 1.0F)
    {
      return true;
    }
  }

  return false;
}

[[nodiscard]] inline linal::float3 calc_sort_center(std::span<const float> vertices, std::int32_t vertexDimension) noexcept
{
  if (vertexDimension < 3 || vertices.empty())
  {
    return linal::float3{0.0F, 0.0F, 0.0F};
  }

  linal::float3 center{0.0F, 0.0F, 0.0F};
  const std::size_t pointCount = vertices.size() / static_cast<std::size_t>(vertexDimension);
  if (pointCount == 0)
  {
    return center;
  }

  for (std::size_t i = 0; i < pointCount; ++i)
  {
    const std::size_t base = i * static_cast<std::size_t>(vertexDimension);
    center[0] += vertices[base];
    center[1] += vertices[base + 1];
    center[2] += vertices[base + 2];
  }

  const float invPointCount = 1.0F / static_cast<float>(pointCount);
  center[0] *= invPointCount;
  center[1] *= invPointCount;
  center[2] *= invPointCount;
  return center;
}

[[nodiscard]] inline DrawableTransparencyInfo make_drawable_transparency_info(std::span<const float> vertices,
                                                                              std::int32_t vertexDimension,
                                                                              std::span<const float> colors,
                                                                              std::int32_t colorDimension) noexcept
{
  return DrawableTransparencyInfo{contains_translucent_alpha(colors, colorDimension), calc_sort_center(vertices, vertexDimension)};
}

} // namespace opengl

#endif // OPENGL_DRAWABLETRANSPARENCYINFO_HPP

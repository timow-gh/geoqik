#include "Origin.hpp"
#include "sleep_helper.hpp"
#include <GeoQik/GeoQik.hpp>
#include <array>
#include <iostream>

using namespace geoqik::examples;

namespace
{

constexpr std::size_t coordinateDimension = 3;
constexpr std::size_t colorDimension = 4;
constexpr std::size_t axisPointCount = 10;
constexpr std::size_t xyAxisCount = 2;
constexpr std::size_t xyPointCount = axisPointCount * xyAxisCount;
constexpr std::size_t xyCoordinateCount = xyPointCount * coordinateDimension;
constexpr std::size_t xyColorCount = xyPointCount * colorDimension;
constexpr std::size_t zCoordinateCount = axisPointCount * coordinateDimension;
constexpr std::size_t zColorCount = axisPointCount * colorDimension;
constexpr std::size_t yCoordinateOffset = axisPointCount * coordinateDimension;
constexpr std::size_t yColorOffset = axisPointCount * colorDimension;
constexpr double gridExtent = 5.0;
constexpr double geometryDelaySeconds = 0.3;
constexpr double pauseSeconds = 0.5;
constexpr float pointSize = 5.0F;
constexpr float lineWidth = 2.0F;
constexpr float opaque = 1.0F;
constexpr float transparentLineAlpha = 0.75F;
constexpr float transparentPointAlpha = 0.35F;

} // namespace

static geoqik_uuid_t add_point_with_delay(double x, double y, double z, double delaySeconds)
{
  geoqik_uuid_t pointId = geoqik_add_point(x, y, z).geometryId;
  sleep_for_seconds(delaySeconds);
  return pointId;
}

static geoqik_uuid_t add_line_with_delay(double x1, double y1, double z1, double x2, double y2, double z2, double delaySeconds)
{
  geoqik_add_line_opts_t lineOpts{};
  std::array<float, colorDimension> lineColor{opaque, opaque, opaque, transparentLineAlpha};
  lineOpts.color = lineColor.data();
  lineOpts.colorCount = lineColor.size();
  geoqik_uuid_t lineId = geoqik_add_line_opts(x1, y1, z1, x2, y2, z2, &lineOpts).geometryId;
  sleep_for_seconds(delaySeconds);
  return lineId;
}

static void remove_line_with_delay(const geoqik_uuid_t* lineId, double delaySeconds)
{
  geoqik_remove_line(lineId);
  sleep_for_seconds(delaySeconds);
}

int main()
{
  geoqik_init();

  geoqik_set_point_size(pointSize);
  geoqik_set_line_width(lineWidth);

  geoqik_draw();

  geoqik_set_point_color(opaque, 0.0F, 0.0F, opaque); // Red
  [[maybe_unused]] geoqik_uuid_t pointIdA = add_point_with_delay(gridExtent, gridExtent, 0.0, geometryDelaySeconds);

  geoqik_set_point_color(0.0F, opaque, 0.0F, opaque); // Green
  [[maybe_unused]] geoqik_uuid_t pointIdB = add_point_with_delay(gridExtent, 0.0, 0.0, geometryDelaySeconds);

  geoqik_set_point_color(0.0F, 0.0F, opaque, opaque); // Blue
  [[maybe_unused]] geoqik_uuid_t pointIdC = add_point_with_delay(0.0, gridExtent, 0.0, geometryDelaySeconds);

  geoqik_uuid_t lineIdA = add_line_with_delay(gridExtent, gridExtent, 0.0, gridExtent, 0.0, 0.0, geometryDelaySeconds);
  geoqik_uuid_t lineIdB = add_line_with_delay(gridExtent, 0.0, 0.0, 0.0, gridExtent, 0.0, geometryDelaySeconds);
  geoqik_uuid_t lineIdC = add_line_with_delay(0.0, gridExtent, 0.0, gridExtent, gridExtent, 0.0, geometryDelaySeconds);

  sleep_for_seconds(pauseSeconds);

  remove_line_with_delay(&lineIdA, geometryDelaySeconds);
  remove_line_with_delay(&lineIdB, geometryDelaySeconds);
  remove_line_with_delay(&lineIdC, geometryDelaySeconds);

  sleep_for_seconds(pauseSeconds);

  geoqik_remove_all_geometry();

  std::array<double, xyCoordinateCount> xyPoints{};
  std::array<float, xyColorCount> xyColors{};
  std::array<double, zCoordinateCount> zPoints{};
  std::array<float, zColorCount> zColors{};
  for (std::size_t i = 0; i < axisPointCount; ++i)
  {
    const auto coord = static_cast<double>(i);

    // x points
    xyPoints[i * coordinateDimension] = coord;
    xyPoints[(i * coordinateDimension) + 1] = 0.0;
    xyPoints[(i * coordinateDimension) + 2] = 0.0;

    // x colors (red)
    xyColors[i * colorDimension] = opaque;
    xyColors[(i * colorDimension) + 1] = 0.0F;
    xyColors[(i * colorDimension) + 2] = 0.0F;
    xyColors[(i * colorDimension) + 3] = opaque;

    // y points
    xyPoints[yCoordinateOffset + (i * coordinateDimension)] = 0.0;
    xyPoints[yCoordinateOffset + (i * coordinateDimension) + 1] = coord;
    xyPoints[yCoordinateOffset + (i * coordinateDimension) + 2] = 0.0;

    // y colors (green)
    xyColors[yColorOffset + (i * colorDimension)] = 0.0F;
    xyColors[yColorOffset + (i * colorDimension) + 1] = opaque;
    xyColors[yColorOffset + (i * colorDimension) + 2] = 0.0F;
    xyColors[yColorOffset + (i * colorDimension) + 3] = opaque;

    // z points
    zPoints[i * coordinateDimension] = 0.0;
    zPoints[(i * coordinateDimension) + 1] = 0.0;
    zPoints[(i * coordinateDimension) + 2] = coord;

    // z colors (translucent blue)
    zColors[i * colorDimension] = 0.0F;
    zColors[(i * colorDimension) + 1] = 0.0F;
    zColors[(i * colorDimension) + 2] = opaque;
    zColors[(i * colorDimension) + 3] = transparentPointAlpha;
  }

  geoqik_add_points_options_t xyOptions{};
  xyOptions.color = xyColors.data();
  xyOptions.colorCount = xyColors.size();
  geoqik_result_t xyPointsRes = geoqik_add_points_opts(xyPoints.data(), xyPoints.size(), &xyOptions);
  if (xyPointsRes.err != GEOQIK_SUCCESS)
  {
    std::cerr << "Failed to add xy points: " << geoqik_get_error_string(xyPointsRes.err) << '\n';
  }

  sleep_for_seconds(pauseSeconds);

  geoqik_add_points_options_t zOptions{};
  zOptions.color = zColors.data();
  zOptions.colorCount = zColors.size();
  geoqik_result_t zPointsRes = geoqik_add_points_opts(zPoints.data(), zPoints.size(), &zOptions);
  if (zPointsRes.err != GEOQIK_SUCCESS)
  {
    std::cerr << "Failed to add z points: " << geoqik_get_error_string(zPointsRes.err) << '\n';
  }

  sleep_for_seconds(pauseSeconds);

  if (zPointsRes.err == GEOQIK_SUCCESS)
  {
    geoqik_remove_point(&zPointsRes.geometryId);
  }

  geoqik_wait_for_exit_and_cleanup();

  return 0;
}

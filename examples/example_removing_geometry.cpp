#include "Origin.hpp"
#include "sleep_helper.hpp"
#include <GeoQik/GeoQik.hpp>
#include <array>
#include <cstdio>

using namespace geoqik::examples;

static geoqik_uuid_t add_point_with_delay(double x, double y, double z, double delaySeconds)
{
  geoqik_uuid_t pointId = geoqik_add_point(x, y, z).geometryId;
  sleep_for_seconds(delaySeconds);
  return pointId;
}

static geoqik_uuid_t add_line_with_delay(double x1, double y1, double z1, double x2, double y2, double z2, double delaySeconds)
{
  geoqik_add_line_opts_t lineOpts{};
  float lineColor[4] = {1.0f, 1.0f, 1.0f, 0.75f};
  lineOpts.color = lineColor;
  lineOpts.colorCount = 4;
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

  geoqik_set_point_size(5.0f);
  geoqik_set_line_width(2.0f);

  geoqik_draw();

  constexpr double delaySeconds = 0.3;

  geoqik_set_point_color(1.0f, 0.0f, 0.0f, 1.0f); // Red
  geoqik_uuid_t pointIdA = add_point_with_delay(5.0, 5.0, 0.0, delaySeconds);

  geoqik_set_point_color(0.0f, 1.0f, 0.0f, 1.0f); // Green
  geoqik_uuid_t pointIdB = add_point_with_delay(5.0, 0.0, 0.0, delaySeconds);

  geoqik_set_point_color(0.0f, 0.0f, 1.0f, 1.0f); // Blue
  geoqik_uuid_t pointIdC = add_point_with_delay(0.0, 5.0, 0.0, delaySeconds);

  geoqik_uuid_t lineIdA = add_line_with_delay(5.0, 5.0, 0.0, 5.0, 0.0, 0.0, delaySeconds);
  geoqik_uuid_t lineIdB = add_line_with_delay(5.0, 0.0, 0.0, 0.0, 5.0, 0.0, delaySeconds);
  geoqik_uuid_t lineIdC = add_line_with_delay(0.0, 5.0, 0.0, 5.0, 5.0, 0.0, delaySeconds);

  sleep_for_seconds(0.5);

  remove_line_with_delay(&lineIdA, delaySeconds);
  remove_line_with_delay(&lineIdB, delaySeconds);
  remove_line_with_delay(&lineIdC, delaySeconds);

  sleep_for_seconds(0.5);

  geoqik_remove_all_geometry();

  std::array<double, 60> xyPoints;
  std::array<float, 80> xyColors;
  std::array<double, 30> zPoints;
  std::array<float, 40> zColors;
  for (int i = 0; i < 10; ++i)
  {
    double coord = static_cast<double>(i);

    // x points
    xyPoints[i * 3] = coord;
    xyPoints[i * 3 + 1] = 0.0;
    xyPoints[i * 3 + 2] = 0.0;

    // x colors (red)
    xyColors[i * 4] = 1.0f;
    xyColors[i * 4 + 1] = 0.0f;
    xyColors[i * 4 + 2] = 0.0f;
    xyColors[i * 4 + 3] = 1.0f;

    // y points
    xyPoints[30 + i * 3] = 0.0;
    xyPoints[30 + i * 3 + 1] = coord;
    xyPoints[30 + i * 3 + 2] = 0.0;

    // y colors (green)
    xyColors[40 + i * 4] = 0.0f;
    xyColors[40 + i * 4 + 1] = 1.0f;
    xyColors[40 + i * 4 + 2] = 0.0f;
    xyColors[40 + i * 4 + 3] = 1.0f;

    // z points
    zPoints[i * 3] = 0.0;
    zPoints[i * 3 + 1] = 0.0;
    zPoints[i * 3 + 2] = coord;

    // z colors (translucent blue)
    zColors[i * 4] = 0.0f;
    zColors[i * 4 + 1] = 0.0f;
    zColors[i * 4 + 2] = 1.0f;
    zColors[i * 4 + 3] = 0.35f;
  }

  geoqik_add_points_options_t xyOptions{};
  xyOptions.color = xyColors.data();
  xyOptions.colorCount = xyColors.size();
  geoqik_result_t xyPointsRes = geoqik_add_points_opts(xyPoints.data(), xyPoints.size(), &xyOptions);
  if (xyPointsRes.err != GEOQIK_SUCCESS)
  {
    std::fprintf(stderr, "Failed to add xy points: %s\n", geoqik_get_error_string(xyPointsRes.err));
  }

  sleep_for_seconds(0.5);

  geoqik_add_points_options_t zOptions{};
  zOptions.color = zColors.data();
  zOptions.colorCount = zColors.size();
  geoqik_result_t zPointsRes = geoqik_add_points_opts(zPoints.data(), zPoints.size(), &zOptions);
  if (zPointsRes.err != GEOQIK_SUCCESS)
  {
    std::fprintf(stderr, "Failed to add z points: %s\n", geoqik_get_error_string(zPointsRes.err));
  }

  sleep_for_seconds(0.5);

  if (zPointsRes.err == GEOQIK_SUCCESS)
  {
    geoqik_remove_point(&zPointsRes.geometryId);
  }

  geoqik_wait_for_exit_and_cleanup();

  return 0;
}
